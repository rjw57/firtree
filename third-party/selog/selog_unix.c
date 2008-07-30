/*
 * Selective logging - unix channels.
 *
 * Written by Tony Finch <dot@dotat.at> <fanf2@cam.ac.uk>
 * at the University of Cambridge Computing Service.
 * You may do anything with this, at your own risk.
 *
 * $Cambridge: users/fanf2/selog/selog_unix.c,v 1.28 2008/04/09 17:50:16 fanf2 Exp $
 */

#include <sys/types.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/uio.h>
#include <sys/un.h>
#include <sys/utsname.h>
#include <sys/wait.h>

#include <assert.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <fcntl.h>
#include <netdb.h>
#include <pthread.h>
#include <unistd.h>

#include "selog.h"

/*********************************************************************
 *
 *  private data structures
 *
 */

#define HOSTNAME_LEN 260
#define PIDLEN 30

static struct iovec hostname;
static struct iovec progname;

static struct selog_buffer *panic_message;

/*
 * Log option selectors.
 */
static selog_selector opt_msec = SELINIT("log_msec",   SELOG_OPTION_OFF);
static selog_selector opt_pid  = SELINIT("log_pid",    SELOG_OPTION_OFF);
static selog_selector opt_rot  = SELINIT("log_rotate", SELOG_OPTION_OFF);
static selog_selector opt_tz   = SELINIT("log_tz",     SELOG_OPTION_ON);
static selog_selector opt_usec = SELINIT("log_usec",   SELOG_OPTION_OFF);
static selog_selector opt_zulu = SELINIT("log_zulu",   SELOG_OPTION_OFF);

/*
 * When things go wrong.
 */
static selog_selector log_panic = SELINIT("log_panic", SELOG_FATAL(1));

static selog_write_fn write_stderr;

/*
 * Timestamp layouts.
 */
#define TIMELEN_SECONDS 19 /* "YYYY-MM-DD hh:mm:ss"          */
#define TIMELEN_SUFFIX  10 /* "YYYY-MM-DD"                   */
#define TIMELEN_SYSLOG  16 /*     "Mmm DD hh:mm:ss "         */
#define TIMELEN_CTIME   26 /* "Ddd Mmm DD hh:mm:ss YYYY\n\0" */
#define SYSLOG_IN_CTIME 4

/*********************************************************************
 *
 *  locking
 *
 * We assume that libc provides null-implementation stubs of the pthread
 * functions for use by thread-aware libraries linked to non-threaded
 * programs, so that the following code does not cause the whole program
 * to have to be linked with libpthread etc. and all that implies.
 */

static pthread_mutex_t mutex[1] = {PTHREAD_MUTEX_INITIALIZER};

void
selog_lock(void) {
	selassert(log_panic, pthread_mutex_lock(mutex) == 0,
	    "%s", strerror(errno));
}

void
selog_unlock(void) {
	selassert(log_panic, pthread_mutex_unlock(mutex) == 0,
	    "%s", strerror(errno));
}

/*
 * Override selog_on() and selog_write() with non-locked versions.
 */
#undef selog_on
#define selog_on(sel) \
	((sel)->enabled == 0 \
		? selog_nl_on(sel) \
		: SELOG_ON(sel))

#define selog_write(buf) selog_nl_write(buf)

/*********************************************************************
 *
 *  string handling
 *
 */

#define PTR iov_base
#define LEN iov_len

#define iov_str(iov)   ((char*)(iov).PTR)
#define iov_add(iov,n) (iov_str(iov) + (n))
#define iov_end(iov) iov_add(iov,(iov).LEN)

static inline struct iovec
iovec2(const char *str, size_t len) {
	struct iovec iov;
	union {
		const char *str;
		void *ptr;
	} u = { str };
	iov.PTR = u.ptr;
	iov.LEN = len;
	return(iov);
}

/* extra "" ensure that str is a literal string */
#define iovec1(str) iovec2(str, sizeof(""str"")-1)

static inline struct iovec
iovec(const char *str) {
	return(iovec2(str, strlen(str)));
}

#define writevc(fd,iov) writev(fd,(iov),sizeof(iov)/sizeof*(iov))

static inline struct iovec
fmtpid(char *buf) {
	if(selog_on(opt_pid))
		return(iovec2(buf, sprintf(buf, "[%d] ", getpid())));
	else
		return(iovec1(" "));
}

#define rfc3339(buf) ((buf)->b + TIMELEN_SYSLOG)

static inline struct iovec
iov3339(selog_buffer buf) {
	const char *ptr = rfc3339(buf);
	size_t len = buf->m - ptr;
	return(iovec2(ptr, len));
}

static inline struct iovec
iov3339msg(selog_buffer buf) {
	const char *ptr = rfc3339(buf);
	size_t len = buf->p - ptr;
	return(iovec2(ptr, len));
}

static inline struct iovec
iovmsg(selog_buffer buf) {
	const char *ptr = buf->m;
	size_t len = buf->p - ptr;
	return(iovec2(ptr, len));
}

/*
 * Configuration syntax lexical elements.
 */
#define SPACE " \t\r\n\\"
#define STOP  SPACE "@;"

static struct iovec
getarg(struct iovec *s) {
	struct iovec a;
	size_t n;

	a = iovec2(s->PTR, strcspn(s->PTR, STOP));
	n = a.LEN + strspn(iov_end(a), SPACE);
	/* bah, too early to use selassert() */
	assert(s->LEN >= n);
	s->PTR = iov_add(*s, n);
	s->LEN -= n;
	return(a);
}

/*********************************************************************
 *
 *  generic channel initialization
 *
 */

/*
 * Channels are specified in the configuration using a keyword
 * optionally followed with some arguments. The keyword is
 * used to look up a function to initialize the channel via
 * the following table.
 */

typedef int chan_open_fn(selog_chan *c, struct iovec a);

static chan_open_fn
    open_default,
    open_exit,
    open_file,
    open_hyperlog,
    open_init,
    open_pipe,
    open_prim,
    open_rotate,
    open_stderr,
    open_stdout,
    open_syslog;

static const struct chan_open {
	const char   *name;
	chan_open_fn *fun;
} chan_open[] = {
	{ "exit",       open_exit     },
	{ "file",       open_file     },
	{ "hyperlog",   open_hyperlog },
	{ "initialize", open_init     },
	{ "pipe",       open_pipe     },
	{ "primordial", open_prim     },
	{ "rotate",     open_rotate   },
	{ "stderr",     open_stderr   },
	{ "stdout",     open_stdout   },
	{ "syslog",     open_syslog   },
	{ NULL, NULL }
};

int
selog_chan_open(selog_chan *c, const char *ptr, size_t len) {
	const struct chan_open *i;
	struct iovec s, a;

	s = iovec2(ptr, len);
	a = getarg(&s);
	for(i = chan_open; i->name != NULL; i++)
		if(strlen(i->name) == a.LEN &&
		   strncmp(i->name, a.PTR, a.LEN) == 0)
			return(i->fun(c, s));
	/* by default assume the whole thing is a filename */
	return(open_default(c, iovec2(ptr, len)));
}

/*********************************************************************
 *
 *  error handling
 *
 */

static void
logerr(const char *func, const char *fmt, ...) {
	selog_buffer buf;
	va_list ap;
	va_start(ap, fmt);
	selog_on(selog_config_error);
	selog_prep(buf, selog_config_error);
	selog_add(buf, "%s: ", func);
	selog_addv(buf, fmt, ap);
	selog_write(buf);
	va_end(ap);
}

static void *
alloc(const char *func, size_t size) {
	void *ptr = malloc(size);
	if(ptr == NULL)
		logerr(func, "%s", strerror(errno));
	else
		memset(ptr, 0, size);
	return(ptr);
}

static int
no_more_args(const char *func, struct iovec a) {
	if(a.LEN == 0)
		return(0);
	logerr(func, "unexpected argument \"%.*s\"", a.LEN, iov_str(a));
	errno = EINVAL;
	return(-1);
}

static int
open_unknown(selog_chan *c, struct iovec a) {
	/* This circumlocution ensures we don't try to re-lock selog. */
	selog_buffer buf;
	selog_on(selog_config_error);
	selog_prep(buf, selog_config_error);
	selog_add(buf,
	    "unknown channel type \"%.*s\"", a.LEN, iov_str(a));
	selog_write(buf);
	c->write = write_stderr;
	c->conf = NULL;
	errno = EINVAL;
	return(-1);
}

static void
write_error(selog_buffer buf1, const char *where) {
	selog_buffer buf2;
	selog_on(log_panic);
	selog_prep(buf2, log_panic);
	selog_add(buf2, "writing %s message to %s: %s",
	    panic_message == NULL ? "following" : "panic",
	    where, strerror(errno));
	if(panic_message == NULL) {
		panic_message = buf1;
		buf1->sel = log_panic;
		selog_write(buf2);
		selog_write(buf1);
		/* the normal exit is disabled during a panic */
		exit(1);
	} else {
		/* double panic */
		write_stderr(buf2, NULL);
		write_stderr(buf1, NULL);
		write_stderr(panic_message, NULL);
		exit(1);
	}
}

/*
 * XXX
 */

#define OPEN_TODO(n) static int \
	open_##n(selog_chan *c, struct iovec a) { \
		a = a; return(open_unknown(c, iovec1(#n))); \
	} struct hack

OPEN_TODO(hyperlog);

/*********************************************************************
 *
 *  stderr / stdout
 *
 *  progname but no timestamp or hostname
 *
 *  XXX: configurable?
 *  XXX: compatibility with daemons that close stderr?
 */

static void
write_stderr(selog_buffer buf, void *conf) {
	int fd = conf == NULL ? STDERR_FILENO : STDOUT_FILENO;
	struct iovec iov[4];
	char pid[PIDLEN];
	iov[0] = progname;
	iov[1] = fmtpid(pid);
	iov[2] = iovmsg(buf);
	iov[3] = iovec1("\n");
	writevc(fd, iov);
}

static int
open_stderr(selog_chan *c, struct iovec a) {
	c->write = write_stderr;
	c->close = NULL;
	c->conf = NULL;
	return(no_more_args(__func__, a));
}

static int
open_stdout(selog_chan *c, struct iovec a) {
	c->write = write_stderr;
	c->close = NULL;
	c->conf = c;
	return(no_more_args(__func__, a));
}

/*********************************************************************
 *
 *  primordial
 *
 *  write to stderr since that needs no configuration
 *
 */

static int
open_prim(selog_chan *c, struct iovec a) {
	static bool opened;
	if(opened++)
		return(open_unknown(c, iovec1("prim")));
	c->write = write_stderr;
	c->close = NULL;
	c->conf = NULL;
	return(no_more_args(__func__, a));
}

/*********************************************************************
 *
 *  exit
 *
 */

static void
write_exit(selog_buffer buf, void *conf) {
	conf = conf;
	/* Disable the normal exit during a panic because our last
	gasp includes the failing message, the panic message, and
	possibly a double panic message if things are really bad. */
	if(panic_message != NULL)
		return;
	if(SELOG_LEVEL(buf->sel) == SELOG_EXIT)
		exit(SELOG_EXITVAL(buf->sel));
	else
		abort();
}

static int
open_exit(selog_chan *c, struct iovec a) {
	static bool opened;
	if(opened++)
		return(open_unknown(c, iovec1("exit")));
	c->write = write_exit;
	c->close = NULL;
	c->conf = NULL;
	return(no_more_args(__func__, a));
}

/*********************************************************************
 *
 *  do pre-write preparation, i.e. timestamp formatting
 *
 */

/* Why isn't there a sensible standard for this? */
extern const char *__progname;

static void
write_init(selog_buffer buf, void *conf) {
	struct timeval tv;
	struct tm tm;
	time_t t;

	conf = conf;

	gettimeofday(&tv, NULL);
	/* LAME: struct timeval doesn't contain a time_t */
	t = tv.tv_sec;
	if(selog_on(opt_zulu))
		gmtime_r(&t, &tm);
	else
		localtime_r(&t, &tm);

	char tb[TIMELEN_CTIME];
	selog_add(buf, "%.*s", TIMELEN_SYSLOG,
	    asctime_r(&tm, tb) + SYSLOG_IN_CTIME);

	/* variable-width ISO 8601 / RFC 3339 (ish) timestamp */
	/* "YYYY-MM-DD hh:mm:ss.ffffff -00:00 " */
	strftime(tb, sizeof(tb), "%Y-%m-%d %H:%M:%S", &tm);
	if(selog_on(opt_usec))
		selog_add(buf, "%s.%06ld", tb, tv.tv_usec);
	else
	if(selog_on(opt_msec))
		selog_add(buf, "%s.%03ld", tb, tv.tv_usec / 1000);
	else
		selog_add(buf, "%s", tb);
	if(!selog_on(opt_tz)) {
		selog_add(buf, " ");
	} else if(selog_on(opt_zulu)) {
		selog_add(buf, " Z ");
	} else {
		/* LAME: strftime %z is the wrong format */
		int sign, off;
		if(tm.tm_gmtoff < 0) {
			sign = '-';
			off = -tm.tm_gmtoff / 60;
		} else {
			sign = '+';
			off = +tm.tm_gmtoff / 60;
		}
		selog_add(buf, " %c%02d:%02d ", sign, off / 60, off % 60);
	}
}

static int
open_init(selog_chan *c, struct iovec a) {
	static bool opened;
	static struct utsname uts;
	struct hostent *hostent;
	const char *h;

	if(opened++)
		return(open_unknown(c, iovec1("initialize")));

	if(uname(&uts) < 0) {
		logerr("gethostname", "%s", strerror(errno));
		return(-1);
	}
	h = uts.nodename;
	if(strchr(h, '.') == NULL) {
		hostent = gethostbyname(h);
		if(hostent != NULL)
			h = strdup(hostent->h_name);
		if(h == NULL)
			h = uts.nodename;
	}

	hostname = iovec(h);
#ifdef __APPLE__
	// HACK: OSX doesn't have __progname
	progname = iovec("selog");
#else
	progname = iovec(__progname);
#endif

	c->write = write_init;
	c->close = NULL;
	c->conf = NULL;
	return(no_more_args(__func__, a));
}

/*********************************************************************
 *
 *  file and rotate
 *
 *  standard timestamp and no hostname or progname
 *
 */

struct conf_file {
	int fd;
	dev_t dev;
	ino_t ino;
	mode_t mode;
	char last[TIMELEN_SECONDS];
	char *suffix;
	char path[1];
};

static int
open_file_again(struct conf_file *cf) {
	struct stat st;
	int fd = open(cf->path, O_WRONLY|O_APPEND|O_CREAT, cf->mode);
	if(fd < 0 || fstat(fd, &st) < 0)
		return(-1);
	close(cf->fd);
	fcntl(fd, F_SETFD, FD_CLOEXEC);
	cf->fd = fd;
	cf->dev = st.st_dev;
	cf->ino = st.st_ino;
	return(0);
}

static void
close_file(void *conf) {
	struct conf_file *cf = conf;
	close(cf->fd);
	free(conf);
}

/*
 * detect file movement and write timestamp and message
 */
static void
write_file(selog_buffer buf, void *conf) {
	struct conf_file *cf = conf;
	struct iovec iov[5];
	struct stat st;
	size_t iovc;
	/* check at most once a second */
	if(0 != strncmp(cf->last, rfc3339(buf), TIMELEN_SECONDS)) {
		strncpy(cf->last, rfc3339(buf), TIMELEN_SECONDS);
		if(stat(cf->path, &st) < 0 ||
		    cf->dev != st.st_dev ||
		    cf->ino != st.st_ino)
			if(open_file_again(cf) < 0)
				write_error(buf, cf->path);
	}
	if(selog_on(opt_pid)) {
		char pid[PIDLEN];
		iov[0] = iov3339(buf);
		iov[1] = progname;
		iov[2] = fmtpid(pid);
		iov[3] = iovmsg(buf);
		iov[4] = iovec1("\n");
		iovc = 5;
	} else {
		iov[0] = iov3339msg(buf);
		iov[1] = iovec1("\n");
		iovc = 2;
	}
	if(writev(cf->fd, iov, iovc) < 0)
		write_error(buf, cf->path);
}

/*
 * once a day change the suffix and ensure file is re-opened
 */
static void
write_rotate(selog_buffer buf, void *conf) {
	struct conf_file *cf = conf;
	if(0 != strncmp(cf->suffix, rfc3339(buf), TIMELEN_SUFFIX)) {
		strncpy(cf->suffix, rfc3339(buf), TIMELEN_SUFFIX);
		/* suppress rotation check in file_write() */
		strncpy(cf->last, rfc3339(buf), TIMELEN_SECONDS);
		cf->dev = cf->ino = 0;
		if(open_file_again(cf) < 0)
			write_error(buf, cf->path);
	}
	write_file(buf, conf);
}

static int
open_file_common(selog_chan *c, struct iovec a,
    const char *func, selog_write_fn *writer, const char *suffix) {
	struct conf_file *cf;
	struct iovec f = getarg(&a);
	struct iovec m = getarg(&a);

	/* just in case */
	c->write = write_stderr;
	c->close = NULL;
	c->conf = NULL;
	if(no_more_args(func, a) < 0)
		return(-1);
	if(iov_str(f)[0] != '/') {
		logerr(func, "%.*s: Filename must be absolute",
		    f.LEN, iov_str(f));
		errno = EINVAL;
		return(-1);
	}
	if(suffix == NULL) {
		cf = alloc(func, sizeof(struct conf_file) + f.LEN);
		if(cf == NULL) return(-1);
		strncpy(cf->path, f.PTR, f.LEN);
		cf->path[f.LEN] = '\0';
		cf->suffix = NULL;
	} else {
		cf = alloc(func, sizeof(struct conf_file) +
		    f.LEN + 1 + TIMELEN_SUFFIX);
		if(cf == NULL) return(-1);
		strncpy(cf->path, f.PTR, f.LEN);
		cf->path[f.LEN] = '.';
		cf->suffix = cf->path + f.LEN + 1;
		strcpy(cf->suffix, suffix);
	}
	cf->fd = -1;
	cf->dev = cf->ino = 0;
	if(m.LEN > 0)
		cf->mode = strtoul(m.PTR, NULL, 0);
	else
		cf->mode = 0666;
	if(open_file_again(cf) < 0) {
		logerr(func, "%.*s: %s",
		    f.LEN, iov_str(f), strerror(errno));
		free(cf);
		return(-1);
	} else {
		c->write = writer;
		c->close = close_file;
		c->conf = cf;
		return(0);
	}
}

static int
open_rotate(selog_chan *c, struct iovec a) {
	selog_buffer buf;
	selog_bufinit(buf, NULL);
	rfc3339(buf)[TIMELEN_SUFFIX] = '\0';
	return(open_file_common(c, a, __func__, write_rotate, rfc3339(buf)));
}

static int
open_file(selog_chan *c, struct iovec a) {
	if(selog_on(opt_rot))
		return(open_rotate(c, a));
	else
		return(open_file_common(c, a, __func__, write_file, NULL));
}

/*********************************************************************
 *
 *  pipes
 *
 *  standard timestamp with hostname and progname
 *
 */

struct conf_pipe {
	int fd;
	int pid;
	char cmd[1];
};

static void
close_pipe(void *conf) {
	struct conf_pipe *cf = conf;
	int status;
	close(cf->fd);
	waitpid(cf->pid, &status, 0);
	free(conf);
}

static void
write_pipe(selog_buffer buf, void *conf) {
	struct conf_pipe *cf = conf;
	struct iovec iov[7];
	char pid[PIDLEN];
	iov[0] = iov3339(buf);
	iov[1] = hostname;
	iov[2] = iovec1(" ");
	iov[3] = progname;
	iov[4] = fmtpid(pid);
	iov[5] = iovmsg(buf);
	iov[6] = iovec1("\n");
	if(writevc(cf->fd, iov) < 0)
		write_error(buf, cf->cmd);
}

static int
open_pipe(selog_chan *c, struct iovec a) {
	struct conf_pipe *cf;
	int fd[2];

	/* just in case */
	c->write = write_stderr;
	c->conf = NULL;
	/* keep fd and zero-terminated command */
	cf = alloc(__func__, sizeof(struct conf_file) + a.LEN);
	if(cf == NULL) return(-1);
	strncpy(cf->cmd, a.PTR, a.LEN);
	cf->cmd[a.LEN] = '\0';
	if(pipe(fd) < 0) {
		logerr(__func__, "%s: %s", cf->cmd, strerror(errno));
		free(cf);
		return(-1);
	}
	switch(cf->pid = fork()) {
	case(-1): /* error */
		logerr(__func__, "%s: %s", cf->cmd, strerror(errno));
		close(fd[0]);
		close(fd[1]);
		free(cf);
		return(-1);
	case(0): /* child */
		dup2(fd[0], STDIN_FILENO);
		close(fd[0]);
		close(fd[1]);
		execl("/bin/sh", "sh", "-c", cf->cmd, NULL);
		logerr(__func__, "%s: %s", cf->cmd, strerror(errno));
		/* parent will panic when write fails */
		_exit(127);
	default: /* parent */
		close(fd[0]);
		fcntl(fd[1], F_SETFD, FD_CLOEXEC);
		cf->fd = fd[1];
		c->write = write_pipe;
		c->close = close_pipe;
		c->conf = cf;
		return(0);
	}
}

/*********************************************************************
 *
 *  syslog
 *
 *  RFC 3164 <priority>, timestamp, progname
 *
 *  The RFC fails to mention that messages from a program to syslogd's
 *  unix domain socket on the same machine do not include the hostname.
 *
 */

/*
 * Mapping from selog levels to syslog severities - not one-to-one.
 * See RFC 3164 section 4.1.1.
 * [Giving a lower number to more severe messages is confusing.]
 */
static const int syslog_severity[] = {
	7, /* SELOG_TRACE    -> LOG_DEBUG   */
	7, /* SELOG_DEBUG    -> LOG_DEBUG   */
	6, /* SELOG_OPTION   -> LOG_INFO    */
	6, /* SELOG_VERBOSE  -> LOG_INFO    */
	6, /* SELOG_DEFAULT  -> LOG_INFO    */
	6, /* SELOG_INFO     -> LOG_INFO    */
	5, /* SELOG_NOTICE   -> LOG_NOTICE  */
	4, /* SELOG_WARNING  -> LOG_WARNING */
	3, /* SELOG_ERROR    -> LOG_ERROR   */
	2, /* SELOG_CRITICAL -> LOG_CRIT    */
	1, /* SELOG_ALERT    -> LOG_ALERT   */
	0, /* SELOG_EMERGENCY-> LOG_EMERG   */
	2, /* SELOG_FATAL    -> LOG_CRIT    */
	1, /* SELOG_ABORT    -> LOG_ALERT   */
};

/*
 * Translation from syslog facilities name to number.
 * See RFC 3164 section 4.1.1 and Berkeley syslog.h.
 */
#define SYSLOG_DEFAULT (1 << 3)
static const char *const syslog_facility[] = {
	"kernel",
	"user",
	"mail",
	"daemon",
	"auth",
	"syslog",
	"lpr",
	"news",
	"uucp",
	"cron",
	"authpriv",
	"ftp",
	"ntp",
	"security",
	"console",
	"reserved",
	"local0",
	"local1",
	"local2",
	"local3",
	"local4",
	"local5",
	"local6",
	"local7",
	NULL
};

/*
 * Syslog socket paths in order of priority. The last is traditional,
 * and the others are used by modern BSDs.
 */
static const char *const syslog_paths[] = {
	"/var/run/logpriv",
	"/var/run/syslog",
	"/var/run/log",
	"/dev/log",
	NULL
};

/*
 * This file descriptor is used by all syslog channels.
 */
static int syslog_fd = -1;
static int syslog_path;

static int
open_syslog_again(void) {
	struct sockaddr_un sun;
	int fd = socket(AF_UNIX, SOCK_DGRAM, 0);
	if(fd < 0)
		return(-1);
	fcntl(fd, F_SETFD, FD_CLOEXEC);
	memset(&sun, 0, sizeof(sun));
	sun.sun_family = AF_UNIX;
	for(int i = 0; syslog_paths[i] != NULL; i++) {
		strcpy(sun.sun_path, syslog_paths[i]);
		if(connect(fd, (void*)&sun, sizeof(sun)) == 0) {
			close(syslog_fd);
			syslog_fd = fd;
			syslog_path = i;
			return(0);
		}
	}
	return(-1);
}

static void
write_syslog(selog_buffer buf, void *conf) {
	int facility = (int)conf;
	struct iovec iov[5];
	char pid[PIDLEN];
	char pri[6]; /* "<NNN>\0" */
	iov[0] = iovec2(pri, snprintf(pri, sizeof(pri), "<%d>",
	    facility | syslog_severity[SELOG_LEVEL(buf->sel)]));
	iov[1] = iovec2(buf->b, TIMELEN_SYSLOG);
	iov[2] = progname;
	iov[3] = fmtpid(pid);
	iov[4] = iovmsg(buf);
	/*
	 * Try again after re-opening if something broke. Keep retrying
	 * if we're out of memory, but only once if we are privileged.
	 */
	for(int i = 0; writevc(syslog_fd, iov) < 0; i++) {
		if(errno != ENOBUFS &&
		    (i > 0 || open_syslog_again() < 0))
			write_error(buf, "syslog");
		if(i > 0 && syslog_path == 0)
			break;
		usleep(1);
	}
}

static int
open_syslog(selog_chan *c, struct iovec a) {
	struct iovec f = getarg(&a);
	int facility;

	/* just in case */
	c->write = write_stderr;
	c->conf = NULL;
	if(no_more_args(__func__, a) < 0)
		return(-1);
	if(f.LEN == 0) {
		facility = SYSLOG_DEFAULT;
	} else {
		facility = -1;
		for(int i = 0; syslog_facility[i] != NULL; i++)
			if(strlen(syslog_facility[i]) == f.LEN &&
			   strncmp(syslog_facility[i], f.PTR, f.LEN) == 0)
				facility = i << 3;
		if(facility == -1) {
			logerr(__func__,
			    "unrecognized facility \"%.*s\"", f.LEN, f.PTR);
			errno = EINVAL;
			return(-1);
		}
	}
	if(syslog_fd == -1 && open_syslog_again() < 0) {
		logerr(__func__, "%s", strerror(errno));
		return(-1);
	} else {
		c->write = write_syslog;
		c->close = NULL;
		c->conf = (void*)facility;
		return(0);
	}
}

/*********************************************************************
 *
 *  no explicit channel type
 *
 */

static int
open_default(selog_chan *c, struct iovec a) {
	if(iov_str(a)[0] == '/')
		return(open_file(c, a));
	else
	if(iov_str(a)[0] == '|')
		/* strip off | character */
		return(open_pipe(c, iovec2(iov_str(a) + 1, a.LEN - 1)));
	else
		return(open_unknown(c, a));
}

/*
 *  eof selog_unix.c
 *
 ********************************************************************/
