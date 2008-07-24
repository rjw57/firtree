/*
 * Selective logging command-line tool.
 *
 * Written by Tony Finch <dot@dotat.at> <fanf2@cam.ac.uk>
 * at the University of Cambridge Computing Service.
 * You may do anything with this, at your own risk.
 *
 * $Cambridge: users/fanf2/selog/selog.c,v 1.4 2008/04/07 18:43:03 fanf2 Exp $
 */

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "selog.h"

static selog_selector usage = SELINIT("usage", SELOG_FATAL(1));

int
main(int argc, char *argv[]) {
	argv++, argc--;
	if(argv[0] != NULL && strchr("+-@", argv[0][0]) != NULL) {
		if(selog_open(argv[0], NULL) < 0)
			exit(1);
		argv++, argc--;
	}
	if(argc < 3)
		selog(usage, "selog [config] <selector> <level> <message>");
	selog_selector sel = SELINIT(argv[0],
	    selog_parse_level(argv[1], strlen(argv[1])));
	if(SELOG_LEVEL(sel) == SELOG_NOLEVEL ||
	   SELOG_LEVEL(sel) == SELOG_OPTION_ON ||
	   SELOG_LEVEL(sel) == SELOG_OPTION_OFF)
		selog(usage, "unknown level %s", argv[1]);
	if(selog_on(sel)) {
		selog_buffer buf;
		selog_prep(buf, sel);
		for(int i = 2; argv[i] != NULL; i++)
			selog_add(buf, "%s ", argv[i]);
		selog_write(buf);
	}
	exit(0);
}

/*
 *  eof selog.c
 *
 ********************************************************************/
