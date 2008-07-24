/*
 * Selective logging test harness.
 *
 * Written by Tony Finch <dot@dotat.at> <fanf2@cam.ac.uk>
 * at the University of Cambridge Computing Service.
 * You may do anything with this, at your own risk.
 *
 * $Cambridge: users/fanf2/selog/test-harness.c,v 1.14 2008/04/03 17:18:01 fanf2 Exp $
 */

#include <err.h>
#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "selog.h"

static int
usage(void) {
	fprintf(stderr,
	    "usage: test-harness [-a assertion] [-e status] [-s sleep] [-t] [config]\n");
	exit(1);
}

static const char *const names[] = {
	"selone", "seltwo", "optoff", "opton",
	"log_config", "log_panic",
	"log_pid", "log_msec", "log_usec",
	"log_rotate", "log_zulu", "log_tz", NULL
};

static selog_selector optoff = SELINIT("optoff", SELOG_OPTION_OFF);
static selog_selector opton  = SELINIT("opton",  SELOG_OPTION_ON);

int
main(int argc, char *argv[]) {
	int opt, level;
	int assert_val = 1;
	int exit_val = 0;
	int sleep_time = 0;
	bool trace = false;

	while((opt = getopt(argc, argv, "a:e:s:t")) != -1)
		switch(opt) {
		case('a'):
			assert_val = atoi(optarg);
			continue;
		case('e'):
			exit_val = atoi(optarg);
			continue;
		case('s'):
			sleep_time = atoi(optarg);
			continue;
		case('t'):
			trace = true;
			continue;
		default:
			usage();
		}
	if(argc > optind + 1)
		usage();
	if(selog_open(argv[optind], names) < 0)
		err(1, "selog_open \"%s\"", argv[optind]);

	for(level = SELOG_TRACE; level < SELOG_EXIT; level++) {
		if(level == SELOG_OPTION_OFF || level == SELOG_OPTION_ON)
			continue;
		else for(int i = 0; i < 2; i++) {
			selog_selector sel = SELINIT(names[i], level);
			if(trace)
				seltrace(sel,
				    "name=%d level=%d optoff=%s opton=%s",
				    i, level,
				    selog_on(optoff) ? "on" : "off",
				    selog_on(opton) ? "on" : "off");
			else
				selog(sel,
				    "name=%d level=%d optoff=%s opton=%s",
				    i, level,
				    selog_on(optoff) ? "on" : "off",
				    selog_on(opton) ? "on" : "off");
			usleep(sleep_time * 1000);
		}
	}
	{
		selog_selector sel = SELINIT("assertion", SELOG_ABORT);
		selassert(sel, assert_val, "tears in the rain");
	}
	{
		selog_selector sel = SELINIT("exit", SELOG_FATAL(exit_val));
		selog(sel, "time to die");
	}
}

/*
 *  eof test-harness.c
 *
 ********************************************************************/
