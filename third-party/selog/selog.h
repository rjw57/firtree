/*
 * Selective logging.
 *
 * Written by Tony Finch <dot@dotat.at> <fanf2@cam.ac.uk>
 * at the University of Cambridge Computing Service.
 * You may do anything with this, at your own risk.
 *
 * $Cambridge: users/fanf2/selog/selog.h,v 1.17 2008/04/07 19:06:54 fanf2 Exp $
 */

#ifndef SELOG_H
#define SELOG_H

/*
 * Message levels.
 */
enum {
	SELOG_TRACE,      /* verbose debug trace */
	SELOG_DEBUG,      /* general debug messages */
	SELOG_OPTION_OFF, /* default-disabled parts of messages */
	SELOG_VERBOSE,    /* additional messages */
	SELOG_OPTION_ON,  /* default-enabled parts of messages */
	SELOG_INFO,       /* informational messages */
	SELOG_NOTICE,     /* normal but significant condition */
	SELOG_WARN,       /* warning conditions */
	SELOG_ERROR,      /* error conditions */
	SELOG_CRITICAL,   /* critical conditions */
	SELOG_ALERT,      /* your country needs more lerts */
	SELOG_EMERGENCY,  /* catastrophic shortage of lerts */
	SELOG_EXIT,       /* program cannot continue */
	SELOG_ABORT,      /* assertion failure and core dump */
	/*
	 * Aliases for the option levels that correspond to how
	 * the user selects (parts of) messages. Levels below
	 * SELOG_DEFAULT are off unless configured otherwise,
	 * and if the user selects "all" messages then that
	 * does not include debugging messages.
	 */
	SELOG_ALL     = SELOG_OPTION_OFF,
	SELOG_OPTION  = SELOG_OPTION_OFF,
	SELOG_DEFAULT = SELOG_OPTION_ON,
	SELOG_OPT_OFF = SELOG_OPTION_OFF,
	SELOG_OPT_ON  = SELOG_OPTION_ON,
	/*
	 * Aliases for syslog compatibility.
	 */
	SELOG_WARNING = SELOG_WARN,
	SELOG_ERR     = SELOG_ERROR,
	SELOG_CRIT    = SELOG_CRITICAL,
	SELOG_EMERG   = SELOG_EMERGENCY,
	/*
	 * Special values. (Note: 14 isn't used.)
	 */
	SELOG_NOLEVEL = 15,
	SELOG_EXITVAL = 16,
	SELOG_FATAL   = SELOG_EXIT + SELOG_EXITVAL
};

/*
 * Specify the value a fatal selector passes to exit().
 */
#define SELOG_FATAL(v)     ((v) * SELOG_EXITVAL + SELOG_EXIT)
#define SELOG_LEVEL(sel)   ((sel)->level % SELOG_EXITVAL)
#define SELOG_EXITVAL(sel) ((sel)->level / SELOG_EXITVAL)

/*
 * Selectors have a tricky declaration. Instead of being a simple
 * structure type they are defined to be an array of one structure.
 * Variables of this type have the same layout in memory as a normal
 * structure, but they are automatically promoted to pointers so that
 * they can be passed to functions efficiently. (The downside is they
 * can't be assigned to straight-forwardly.) The array-of-one-struct
 * trick is also used by jmp_buf and gmp mpz_t.
 */
typedef struct selog_selector_s {
	const char  *name;
	unsigned int level;
	unsigned int enabled;
	struct selog_selector_s *chain;
} selog_selector[1];

#define SELINIT(name,level) {{ name, level, 0, NULL }}

/*
 * Buffer for constructing messages incrementally.
 * We use the array-of-one-struct trick again.
 *
 * p points to the end of the data in b
 * m points to the end of the data written by selog_bufinit()
 *
 * The message specified by the user, including any prefix added by
 * selog_prep() or selog_trace() etc., lies between m and p.
 */
typedef struct selog_buffer_s {
	struct selog_selector_s *sel;
	char *m, *p, b[1000];
} selog_buffer[1];

/* Ugly! */
#ifdef __GNUC__
#define prfmt(a,b) __attribute__((__format__(__printf__,a,b)))
#else
#define prfmt(a,b)
#endif
#define str const char *
#define uint unsigned

void selog(selog_selector, str, ...) prfmt(2,3);
void selogerr(selog_selector, str, ...) prfmt(2,3);
void selog_add(selog_buffer, str, ...) prfmt(2,3);
void selog_addv(selog_buffer, str, va_list) prfmt(2,0);
void selog_addopt(selog_selector, selog_buffer, str, ...) prfmt(3,4);
void selog_assert(selog_selector, str, str, str, str, ...) prfmt(5,6);
void selog_bufinit(selog_buffer, selog_selector);
str  selog_level(selog_selector);
str  selog_name(selog_selector);
bool selog_on(selog_selector);
int  selog_open(const char *config, const char *const spelling[]);
uint selog_parse_level(str, size_t);
void selog_prep(selog_buffer, selog_selector);
void selog_trace(selog_selector, str, str, str, ...) prfmt(4,5);
void selog_write(selog_buffer);

#undef prfmt
#undef str
#undef uint

/*
 * The initial value of sel->enabled is zero which means unconfigured.
 * A selector is configured the first time it is passed to a selog
 * function (except for selog_prep which must be guarded by selog_on).
 * Configuring is a relatively expensive operation which fills
 * sel->enabled with a flag for each output channel that the
 * selector's messages should be written to. If all the channel flags
 * are zero, then the DISABLED bit is set to quell reconfiguration.
 * The maximum number of channels is limited by the width of the
 * flag word minus space for the disabled bit.
 */
#define SELOG_CHAN_MAX (30)
#define SELOG_DISABLED (1 << SELOG_CHAN_MAX)

/*
 * This check must only be used to speed up code that calls selog_on().
 */
#define SELOG_ON(sel) ((sel)->enabled != SELOG_DISABLED)

/*
 * Macro wrappers to optimise the enabled check. These are not inline
 * functions because varargs functions can't be inlined, and it's
 * convenient if they have the same names as the functions they wrap.
 */

#define selog(sel, ...) \
	(SELOG_ON(sel) ? selog(sel, __VA_ARGS__) : (void)0)

#define selogerr(sel, ...) \
	(SELOG_ON(sel) ? selogerr(sel, __VA_ARGS__) : (void)0)

#define selog_addopt(sel, buf, ...) \
	(SELOG_ON(sel) ? selog_addopt(sel, buf, __VA_ARGS__) : (void)0)

#define selog_name(sel) ((sel)->name)

#define selog_on(sel) \
	((sel)->enabled == 0 \
		? selog_on(sel) \
		: SELOG_ON(sel))

/*
 * Macro-only calls.
 *
 * selassert() has no selector guard because the selector should
 * have level SELOG_ABORT which is always enabled.
 */

#define SELOG_CALLER __FILE__ ":" SELOG_STR2(__LINE__), __func__
#define SELOG_STR2(arg) SELOG_STR1(arg)
#define SELOG_STR1(arg) #arg

#define selassert(sel, pred, ...) \
	(!(pred)? selog_assert(sel, SELOG_CALLER, #pred, __VA_ARGS__) \
		: (void)0)

#define seltrace(sel, ...) \
	(SELOG_ON(sel) \
		? selog_trace(sel, SELOG_CALLER, __VA_ARGS__) \
		: (void)0)

/*
 * Internal interface between the ANSI C portable front-end and
 * non-portable channel code.
 *
 * Channels are stored in an array, represented as a function to
 * write a message to the channel, a function to close the channel,
 * and an opaque pointer to the channel configuration. The index of
 * a channel corresponds to the bit position of the channel's flag
 * in a selector's enabled word.
 */

typedef void selog_write_fn(selog_buffer, void *);
typedef void selog_close_fn(void *);

typedef struct selog_chan {
	selog_write_fn *write;
	selog_close_fn *close;
	void           *conf;
} selog_chan;

/*
 * A channel is initialized using this function, which is passed a
 * pointer to a channel structure and a fragment of the configuration
 * string corresponding to a channel specifier. It fills in the
 * channel and returns zero if all went well. If there was an error it
 * should report it using the selog_config_error selector, set errno,
 * and return -1, but it must still initialize the channel. At worst
 * the write function can be set to a no-op.
 *
 * There are some special channel specifiers. The "primordial" channel
 * is the last-ditch channel used to report errors (to stderr) before
 * selog is fully configured, and for any trailing selectors in the
 * configuration string that are not followed by channel specifiers.
 * The "initialize" channel is invoked by selog_bufinit() so it can
 * reserve space in the buffer for timestamps etc. The "exit" channel
 * is enabled for fatal and abort selectors and is called after the
 * message has been written by all the other channels so that it can
 * exit the program. The "initialize" and "exit" channels do not write
 * messages anywhere.
 */
int selog_chan_open(selog_chan *, const char *, size_t);

/*
 * Selog uses this selector to control configuration error reporting.
 * It has a special initialization so that it can be used before selog
 * is fully working. It can be used right from the start by the
 * channel code, though it mustn't be passed to selog_write() before
 * the first call to selog_chan_open() has returned. It should only be
 * used after selog_open() returns if no other channel works.
 */
extern selog_selector selog_config_error;

/*
 * Selog's shared data structures are protected using these functions.
 * The activity inside selog_open(), selog_on(), and selog_write() is
 * wrapped by calls to selog_lock() and selog_unlock(). The other
 * public functions call the locked functions to manipulate shared
 * data. Buffers are assumed not to be shared. The non-portable channel
 * code can assume it has exclusive access to its data structures.
 */
void selog_lock(void);
void selog_unlock(void);

/*
 * Unlocked versions of functions for internal use.
 */
bool selog_nl_on(selog_selector);
void selog_nl_write(selog_buffer);

#endif /* ! SELOG_H */
