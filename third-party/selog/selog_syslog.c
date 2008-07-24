/*
 * Selective logging - alternative syslog implementation.
 *
 * Written by Tony Finch <dot@dotat.at> <fanf2@cam.ac.uk>
 * at the University of Cambridge Computing Service.
 * You may do anything with this, at your own risk.
 *
 * $Cambridge: users/fanf2/selog/selog_syslog.c,v 1.3 2008/04/07 15:14:12 fanf2 Exp $
 */

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <syslog.h>

#include "selog.h"

/*
 * This file contains a replacement implementation of
 * the syslog API that writes via selog.
 *
 * The setlogmask() function is currently not implemented. The pri
 * argument to syslog() cannot be used to change the facility on the
 * fly.
 */

#define DEBUG sel[7]

static selog_selector sel[] = {
	SELINIT("syslog", SELOG_EMERG),
	SELINIT("syslog", SELOG_ALERT),
	SELINIT("syslog", SELOG_CRIT),
	SELINIT("syslog", SELOG_ERR),
	SELINIT("syslog", SELOG_WARNING),
	SELINIT("syslog", SELOG_NOTICE),
	SELINIT("syslog", SELOG_INFO),
	SELINIT("syslog", SELOG_DEBUG),
};

#define LUMP 1023

void
vsyslog(int pri, const char *fmt, va_list ap) {
	const char *p, *q;
	int e = errno;
	selog_buffer buf;

	if(!selog_on(sel[pri & LOG_PRIMASK]))
		return;
	if(pri & ~LOG_PRIMASK)
		selog(DEBUG, "dynamic facility %d ignored by selog",
		    pri & ~LOG_PRIMASK);
	/*
	 * Add the message to the buffer, inserting %m where
	 * necessary. Splitting the format string is a pain...
	 */
	selog_prep(buf, sel[pri & LOG_PRIMASK]);
	p = q = fmt;
	for(;;) {
		q = strchr(q, '%');
		if(q == NULL) {
			selog_addv(buf, p, ap);
		} else if(q[1] == 'm') {
			char xfmt[LUMP+1];
			if(q-p > LUMP) {
				strncpy(xfmt, p, LUMP);
				xfmt[LUMP] = '\0';
				selog_addv(buf, xfmt, ap);
				p = q = p + LUMP;
			} else {
				strncpy(xfmt, p, q-p);
				xfmt[q-p] = '\0';
				selog_addv(buf, xfmt, ap);
				selog_add(buf, "%s", strerror(e));
				p = q = q + 2;
			}
		} else if(q[1] == '%') {
			q += 2;
		} else {
			q += 1;
		}
	}
	selog_write(buf);
}

void
syslog(int pri, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	vsyslog(pri, fmt, ap);
	va_end(ap);
}

extern const char *__progname;

void
openlog(const char *ident, int logopt, int facility) {
	static char config[256];

	if(ident != NULL)
		__progname = ident;
	snprintf(config, sizeof(config), "%s%s%s@syslog %d",
	    logopt & LOG_PID    ? "+log_pid"  : "",
	    logopt & LOG_PERROR ? "@stderr"   : "",
	    logopt & LOG_CONS   ? "@console"  : "",
	    facility);
	selog_open(config, NULL);
}

void
closelog(void) {
}

int
setlogmask(int mask) {
	selog(DEBUG, "setlogmask ignored by selog");
	return(mask);
}

/*
 *  eof selog_err.c
 *
 ********************************************************************/
