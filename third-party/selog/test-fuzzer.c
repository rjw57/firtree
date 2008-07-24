/*
 * Selective logging fuzzer.
 *
 * Written by Tony Finch <dot@dotat.at> <fanf2@cam.ac.uk>
 * at the University of Cambridge Computing Service.
 * You may do anything with this, at your own risk.
 *
 * $Cambridge: users/fanf2/selog/test-fuzzer.c,v 1.2 2008/03/11 16:12:07 fanf2 Exp $
 */

#include <err.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>

#define RAND "/dev/urandom"
#define MAXLEN 100
#define FUZZY " <=>.@+-;abcd1234"

int
main(void) {
	static const char fuzzy[sizeof(FUZZY)-1] = FUZZY;
	unsigned char *in;
	char *out;
	size_t len, i;
	int fd;

	/* randomness APIs are all LAME */
	fd = open(RAND, O_RDONLY);
	if(fd < 0)
		err(1, "open " RAND);
	if(read(fd, &len, sizeof(len)) < (int)sizeof(len))
		err(1, "read " RAND);
	len %= MAXLEN;
	in = malloc(len);
	out = malloc(len);
	if(in == NULL || out == NULL)
		err(1, "malloc");
	if(read(fd, in, len) < (int)len)
		err(1, "read " RAND);
	for(i = 0; i < len; i++)
		out[i] = fuzzy[in[i] % sizeof(fuzzy)];
	if(write(1, out, len) < 0)
		err(1, "write");

	return(0);
}

/*
 *  eof test-fuzzer.c
 *
 ********************************************************************/
