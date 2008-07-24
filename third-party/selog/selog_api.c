/*
 * Selective logging - portable front-end.
 *
 * Written by Tony Finch <dot@dotat.at> <fanf2@cam.ac.uk>
 * at the University of Cambridge Computing Service.
 * You may do anything with this, at your own risk.
 *
 * $Cambridge: users/fanf2/selog/selog_api.c,v 1.29 2008/04/09 21:08:47 fanf2 Exp $
 */

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "selog.h"

/*********************************************************************
 *
 *  private data structures
 *
 */

/*
 * The configuration string is used by selog_open() to initialize the
 * channel array, then subsequently to initialize each selector.
 */
static const char *configuration;

/*
 * Configuration errors are reported using this slightly special selector.
 */
selog_selector selog_config_error = SELINIT("log_config", SELOG_ERROR);

/*
 * Convenience macro for reporting errors.
 * This strays from strict ANSI C in its use of errno.
 */
#define ERROR(...) ( \
	selog(selog_config_error, __VA_ARGS__), \
	errno = EINVAL, \
	-1)

/*
 * Chain of selectors that must be decondifured when we are reconfigured.
 */
static struct selog_selector *sel_chain;

/*
 * The channel array.
 */
static selog_chan channel[SELOG_CHAN_MAX];

/*
 * Special channels.
 */
#define INIT_CHAN (SELOG_CHAN_MAX - 1) /* selog_on never sets this flag */
#define EXIT_CHAN (SELOG_CHAN_MAX - 2) /* must be after all except prep */
#define PRIM_CHAN (SELOG_CHAN_MAX - 3) /* to report configuration errors */
#define USER_CHAN (SELOG_CHAN_MAX - 3) /* maximum user-defined channels */

/*
 * Selectors with a lower level than this are disabled without scanning
 * the configuration string. There may be many debugging and tracing
 * selectors, and they will usually not be enabled. If any debugging is
 * requested then this quick disable threshold is lowered.
 */
static unsigned level_threshold = SELOG_ALL;

/*
 * Mapping from selog levels to strings. The LOG_OPTION_* levels
 * should never translated to strings because options only enable
 * parts of lines.
 */
static const char *const level_name[] = {
	"trace",
	"debug",
	"option_off",
	"verbose",
	"option_on",
	"info",
	"notice",
	"warning",
	"error",
	"critical",
	"alert",
	"emergency",
	"fatal",
	"abort",
	"-",
	"-",
};

/*
 * Mapping from strings to selog levels. This table is sorted by string.
 */
static const struct {
	const char *str;
	unsigned val;
} level_table[] = {
	{ "abort",      SELOG_ABORT      },
	{ "alert",      SELOG_ALERT      },
	{ "all",        SELOG_ALL        },
	{ "crit",       SELOG_CRIT       },
	{ "critical",   SELOG_CRITICAL   },
	{ "debug",      SELOG_DEBUG      },
	{ "default",    SELOG_DEFAULT    },
	{ "emerg",      SELOG_EMERG      },
	{ "emergency",  SELOG_EMERGENCY  },
	{ "err",        SELOG_ERR        },
	{ "error",      SELOG_ERROR      },
	{ "exit",       SELOG_EXIT       },
	{ "fatal",      SELOG_FATAL      },
	{ "info",       SELOG_INFO       },
	{ "notice",     SELOG_NOTICE     },
	{ "opt_off",    SELOG_OPT_OFF    },
	{ "opt_on",     SELOG_OPT_ON     },
	{ "option",     SELOG_OPTION     },
	{ "option_off", SELOG_OPTION_OFF },
	{ "option_on",  SELOG_OPTION_ON  },
	{ "trace",      SELOG_TRACE      },
	{ "verbose",    SELOG_VERBOSE    },
	{ "warn",       SELOG_WARN       },
	{ "warning",    SELOG_WARNING    },
};

const char *
selog_level(selog_selector sel) {
	return(level_name[SELOG_LEVEL(sel)]);
}

const char * /* macro */
(selog_name)(selog_selector sel) {
	return(sel->name);
}

static int
wordcmp(const char *s, const char *t, size_t n) {
	int ret = memcmp(s, t, n);
	if(ret != 0)
		return(ret);
	else
		return(s[n]);
}

unsigned
selog_parse_level(const char *str, size_t len) {
	int lo, mid, hi, cmp;
	lo = 0;
	hi = sizeof(level_table)/sizeof(*level_table) - 1;
	while(lo <= hi) {
		mid = (lo + hi) / 2;
		cmp = wordcmp(level_table[mid].str, str, len);
		if(cmp == 0)
			return(level_table[mid].val);
		if(cmp < 0)
			lo = mid + 1;
		if(cmp > 0)
			hi = mid - 1;
	}
	if(len > 7 && str[len-1] == ')' &&
	    memcmp(str, "fatal(", 6) == 0) {
		char *end;
		int status = strtol(str + 6, &end, 10);
		if(end == str + len - 1)
			return(SELOG_FATAL(status));
	}
	return(SELOG_NOLEVEL);
}

/*********************************************************************
 *
 *  internal non-locked functions
 *
 */

/*
 * Configuration syntax lexical elements.
 */
#define SPACE   " \t\r\n"
#define COMPARE "<=>."
#define START   "@+-"
#define END     ";"
#define DELIMITER COMPARE START END SPACE

/*
 * These functions have complementary implementations of the config
 * parser. In selog_nl_open(), channel specifiers are examined and their
 * open functions are called, and selector items are mostly ignored
 * except to check their spelling. In selog_nl_on(), channel specifiers
 * are mostly ignored except to count the channel index, and selector
 * items are examined to work out when to set the enabled flag. The
 * parser loops have the same shapes to help keep them in sync.
 */

static int
selog_nl_open(const char *config, const char *const *spelling) {
	const char *const *s;
	const char *p, *q;
	selog_chan *chan;
	unsigned lev;
	int ret = 0;

	/*
	 * Work out new configuration, and don't reconfigure
	 * if it would be a no-op.
	 */
	p = getenv("SELOG_CONFIG");
	if(p == NULL)
		p = config;
	if(p == NULL)
		p = "";
	if(configuration == p)
		return(0);

	/*
	 * Unconfigure any previously-configured channels and selectors.
	 */
	for(int i = 0; i < SELOG_CHAN_MAX; i++) {
		if(channel[i].close != NULL)
			channel[i].close(channel[i].conf);
		channel[i].write = NULL;
		channel[i].close = NULL;
		channel[i].conf = NULL;
	}
	while(sel_chain != NULL) {
		sel_chain->enabled = 0;
		sel_chain = sel_chain->chain;
	}

	/*
	 * Install new configuration.
	 */
	configuration = p;

	/*
	 * Find out if we should report configuration errors. We can do
	 * this very early because selog_nl_on() only looks at the config
	 * string and the level_threshold. This test is likely to enable
	 * various channels before they are opened, so we override it to
	 * force output to just the special primordial channel.
	 */
	if(selog_nl_on(selog_config_error))
		selog_config_error->enabled = 1 << PRIM_CHAN;

	/* Initalize special channels */
	ret |= selog_chan_open(channel + PRIM_CHAN, "primordial", 10);
	ret |= selog_chan_open(channel + INIT_CHAN, "initialize", 10);
	ret |= selog_chan_open(channel + EXIT_CHAN, "exit", 4);

	chan = channel;
	p += strspn(p, SPACE);
	for(;;) {
		switch(*p) {
		case('\0'):
			return(ret);
		default:
			/* Scan past previous item's end marker. */
			p += strspn(p, END);
			/* Skip to start of next item. */
			p += strcspn(q = p, END START);
			if(p > q)
				ret |= ERROR("probably missing +/- "
				    "before ignored text \"%.*s\"", p-q, q);
			/* Scan past ignored item's end marker. */
			p += strspn(p, END);
			continue;
		case('@'):
			/* Skip start character */
			p++; p += strspn(p, SPACE);
			/* Are we about to overwrite the last channel? */
			if(chan->write != NULL) {
				ret |= ERROR("too many output channels "
				    "(max %d)", USER_CHAN);
				errno = E2BIG;
				/* Skip to end of channel specifier. */
				p += strcspn(p, "@;");
				continue;
			}
			/* Initialize the channel. */
			p += strcspn(q = p, "@;");
			ret |= selog_chan_open(chan++, q, p-q);
			continue;
		case('+'):
		case('-'):
			break;
		}
		/* Skip the plus or minus. */
		p++; p += strspn(p, SPACE);
		/*
		 * Check an item's spelling.
		 */
		if(*p == '\0' || strchr(COMPARE, *p) == NULL) {
			/*
			 * Not an all-categories comparison clause,
			 * so check for a level or category name.
			 */
			p += strcspn(q = p, DELIMITER);
			lev = selog_parse_level(q, p-q);
			/* Maybe turn on full debug configuration. */
			if(level_threshold > lev)
				level_threshold = lev;
			/*
			 * Check category spelling if it isn't a
			 * level and if we have a dictionary.
			 */
			if(lev == SELOG_NOLEVEL && spelling != NULL) {
				for(s = spelling; *s != NULL; s++)
					if(wordcmp(*s, q, p-q) == 0)
						break;
				if(*s == NULL)
					ret |= ERROR(
					    "ignoring unrecognized category "
					    "\"%.*s\"", p-q, q);
			}
			p += strspn(p, SPACE);
			/* Followed by a comparison clause? */
			if(*p == '\0' || strchr(COMPARE, *p) == NULL)
				continue;
		}
		/* Skip comparison operator. */
		p++; p += strspn(p, SPACE);
		/* Check comparison level. */
		p += strcspn(q = p, DELIMITER);
		lev = selog_parse_level(q, p-q);
		/* Maybe turn on full debug configuration. */
		if(level_threshold > lev)
			level_threshold = lev;
		if(lev == SELOG_NOLEVEL)
			ret |= ERROR("ignoring unrecognized level "
			    "\"%.*s\"", p-q, q);
		continue;
	}
}

bool
selog_nl_on(selog_selector sel) {
	const char *p, *q;
	int chan;
	bool flag;
	unsigned lev, enabled;
	char sign, cmp;

	/* Nothing to do if it has already been done. */
	if(sel->enabled != 0)
		return(SELOG_ON(sel));
	/* Configure ourselves if the user didn't. */
	if(configuration == NULL)
		selog_nl_open(NULL, NULL);
	/* Add the selector to the reconfig chain. */
	sel->chain = sel_chain;
	sel_chain = sel;
	/* Quickly disable debugging selectors. */
	if(level_threshold > SELOG_LEVEL(sel)) {
		sel->enabled = SELOG_DISABLED;
		return(false);
	}
	/* These settings make sense if the configuration is empty. */
	flag = SELOG_LEVEL(sel) >= SELOG_DEFAULT;
	sign = '+';
	enabled = 0;
	chan = 0;
	p = configuration;
	for(;;) {
		switch(*p) {
		case('\0'):
			/* Trailing selectors go to the primordial channel */
			if(sign != '@')
				enabled |= flag << PRIM_CHAN;
			/* Fatal errors must always invoke exit. */
			if(SELOG_LEVEL(sel) >= SELOG_EXIT)
				enabled |= 1 << EXIT_CHAN;
			if(enabled == 0)
				/* Prevent re-configuration. */
				sel->enabled = SELOG_DISABLED;
			else
				/* Atomically enable the selector. */
				sel->enabled = enabled;
			return(SELOG_ON(sel));
		default:
			/* Ignore unrecognized characters. */
			p++;
			continue;
		case('@'):
			/* Remember the sign to detect trailing selectors. */
			sign = *p;
			/*
			 * We've found a channel specifier, so copy the
			 * selector's state to its flag bit if we haven't
			 * over-run. No need to examine the specifier
			 * itself because that was done at startup.
			 */
			if(chan < USER_CHAN)
				enabled |= flag << chan;
			chan++;
			/* Skip to end of channel specifier. */
			p++; p += strcspn(p, "@;");
			continue;
		case('+'):
		case('-'):
			sign = *p;
			break;
		}
		/* Skip the plus or minus. */
		p++; p += strspn(p, SPACE);
		/*
		 * Does the item match this selector?
		 */
		if(*p == '\0' || strchr(COMPARE, *p) == NULL) {
			/* Not an all-categories comparison clause. */
			p += strcspn(q = p, DELIMITER);
			if(wordcmp(sel->name, q, p-q) != 0) {
				/*
				 * This item doesn't match this selector's
				 * name. Check for "+level" or "-level".
				 */
				lev = selog_parse_level(q, p-q) % SELOG_EXITVAL;
				if(SELOG_LEVEL(sel) >= lev)
					flag = sign == '+';
				/*
				 * else if(lev == SELOG_NOLEVEL) then this item
				 * is for some other selector, and continuing
				 * the loop will skip any comparison clause.
				 */
				continue;
			}
			/*
			 * We have matched our selector name.
			 */
			p += strspn(p, SPACE);
			if(*p == '\0' || strchr(COMPARE, *p) == NULL) {
				/*
				 * If there's no comparison clause,
				 * enable "all" non-debug selectors.
				 */
				if(SELOG_LEVEL(sel) >= SELOG_ALL)
					flag = sign == '+';
				continue;
			}
		}
		/* Remember comparison operator. */
		cmp = *p;
		p++; p += strspn(p, SPACE);
		/* Compare with what level? */
		p += strcspn(q = p, DELIMITER);
		lev = selog_parse_level(q, p-q) % SELOG_EXITVAL;
		/* Skip syntax errors. */
		if(lev == SELOG_NOLEVEL)
			continue;
		/* Change the on state if the selector passes. */
		switch(cmp) {
		case('<'):
			if(SELOG_LEVEL(sel) <= lev)
				flag = sign == '+';
			break;
		case('='):
			if(SELOG_LEVEL(sel) == lev)
				flag = sign == '+';
			break;
		case('>'):
		case('.'):
			if(SELOG_LEVEL(sel) >= lev)
				flag = sign == '+';
			break;
		}
		continue;
	}
}

void
selog_nl_write(selog_buffer buf) {
	/* Strip trailing space and/or controls. */
	while(buf->p > buf->b && buf->p[-1] <= ' ')
		buf->p -= 1;
	for(int i = 0; i < SELOG_CHAN_MAX; i++)
		if(buf->sel->enabled & 1 << i)
			channel[i].write(buf, channel[i].conf);
}

/*********************************************************************
 *
 *  public functions
 *
 */

int
selog_open(const char *config, const char *const *spelling) {
	int ret;
	selog_lock();
	ret = selog_nl_open(config, spelling);
	selog_unlock();
	return(ret);
}

bool /* macro */
(selog_on)(selog_selector sel) {
	bool ret;
	selog_lock();
	ret = selog_nl_on(sel);
	selog_unlock();
	return(ret);
}

void
selog_write(selog_buffer buf) {
	/* Check that we have not been reconfigured since we started this
	log operation, and ensure that nothing changes while we work. */
	selog_lock();
	if(selog_nl_on(buf->sel))
		selog_nl_write(buf);
	selog_unlock();
}

void
selog_addv(selog_buffer buf, const char *fmt, va_list ap) {
	/* Ensure there's space for the '\0'. */
	char *e = buf->b + sizeof(buf->b) - 1;
	if(buf->p < e)
		buf->p += vsnprintf(buf->p, e - buf->p + 1,  fmt, ap);
	if(buf->p > e)
		buf->p = e;
}

void
selog_add(selog_buffer buf, const char *fmt, ...) {
	va_list ap;
	va_start(ap, fmt);
	selog_addv(buf, fmt, ap);
	va_end(ap);
}

void /* macro */
(selog_addopt)(selog_selector sel, selog_buffer buf, const char *fmt, ...) {
	va_list ap;
	if(!selog_on(sel))
		return;
	va_start(ap, fmt);
	selog_addv(buf, fmt, ap);
	va_end(ap);
}

void
selog_bufinit(selog_buffer buf, selog_selector sel) {
	/* Initialize the buffer. */
	buf->sel = sel;
	buf->m = NULL;
	buf->p = buf->b;
	/* Do non-portable setup using a special "channel". */
	channel[INIT_CHAN].write(buf, channel[INIT_CHAN].conf);
	/* Mark end of reserved prefix. */
	buf->m = buf->p;
}

void
selog_prep(selog_buffer buf, selog_selector sel) {
	selog_bufinit(buf, sel);
	selog_add(buf, "%s %s: ", selog_name(sel), selog_level(sel));
}

void
selog_assert(selog_selector sel, const char *file, const char *func,
    const char *pred, const char *fmt, ...) {
	selog_buffer buf;
	va_list ap;
	/* always true */
	selog_on(sel);
	va_start(ap, fmt);
	selog_bufinit(buf, sel);
	selog_add(buf, "%s %s() %s %s: assertion %s failed%s",
	    file, func, selog_name(sel), selog_level(sel),
	    pred, fmt[0] == '\0' ? "" : ": ");
	selog_addv(buf, fmt, ap);
	selog_write(buf);
	va_end(ap);
	/* just in case */
	abort();
}

void
selog_trace(selog_selector sel,
    const char *file, const char *func, const char *fmt, ...) {
	selog_buffer buf;
	va_list ap;
	if(!selog_on(sel))
		return;
	va_start(ap, fmt);
	selog_bufinit(buf, sel);
	selog_add(buf, "%s %s() %s %s: ", file, func,
	    selog_name(sel), selog_level(sel));
	selog_addv(buf, fmt, ap);
	selog_write(buf);
	va_end(ap);
}

void /* macro */
(selog)(selog_selector sel, const char *fmt, ...) {
	selog_buffer buf;
	va_list ap;
	if(!selog_on(sel))
		return;
	va_start(ap, fmt);
	selog_prep(buf, sel);
	selog_addv(buf, fmt, ap);
	selog_write(buf);
	va_end(ap);
}

void /* macro */
(selogerr)(selog_selector sel, const char *fmt, ...) {
	selog_buffer buf;
	va_list ap;
	if(!selog_on(sel))
		return;
	va_start(ap, fmt);
	selog_prep(buf, sel);
	selog_addv(buf, fmt, ap);
	selog_add(buf, ": %s", strerror(errno));
	selog_write(buf);
	va_end(ap);
}

/*
 *  eof selog_api.c
 *
 ********************************************************************/
