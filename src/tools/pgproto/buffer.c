/*-------------------------------------------------------------------------
 *
 * buffer.c
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#include "../../include/config.h"
#include "pgproto/pgproto.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <getopt.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include "pgproto/fe_memutils.h"
#include <libpq-fe.h>
#include "pgproto/read.h"
#include "pgproto/buffer.h"

/*
 * Read integer field and returns it.  The pointer in the buffer after reading
 * the data is returned to "bufp".  If the format is not follows above, exit
 * within this function.
 */
int
buffer_read_int(char *buf, char **bufp)
{
	char		intbuf[128];
	int			i;
	char	   *p;
	int			intval;
	char	   *endptr;

	p = intbuf;

	for (i = 0; i < sizeof(intbuf) - 1; i++)
	{
		if (*buf == '\t')
			break;

		*p++ = *buf++;
	}

	*p = '\0';

	errno = 0;
	intval = strtol(intbuf, &endptr, 10);
	if (errno != 0)
	{
		fprintf(stderr, "expected int field is not correct (%s)\n", intbuf);
		exit(1);
	}

	*bufp = buf;

	return intval;
}

/*
 * Read string and returns pg_malloced buffer.  The string must start with a
 * double quotation and end with a double quotation. The pointer in the buffer
 * after reading the data is returned to "bufp".  If the format is not follows
 * above, exit within this function.
 */
char *
buffer_read_string(char *buf, char **bufp)
{
	int			len;
	char	   *p,
			   *str;

	if (*buf != '"')
	{
		fprintf(stderr, "buffer_read_string: given string does not start with \" (%s)", buf);
		exit(1);
	}

	buf++;

	len = strlen(buf);
	p = str = pg_malloc(len + 1);

	do
	{
		/* skip '\' */
		if (*buf == '\\')
		{
			buf++;
		}

		*p++ = *buf++;
	} while (*buf && *buf != '\n' && *buf != '\t');

	if (p == str || *(p - 1) != '"')
	{
		fprintf(stderr, "buffer_read_string: given string does not end with \"(%c)", p == str ? ' ' : *(p - 1));
		exit(1);
	}

	*(p - 1) = '\0';

	*bufp = buf;

	return str;
}

/*
 * Read a character and returns it.  The character must start with a single
 * quotation and end with a single quotation. The pointer in the buffer after
 * reading the data is returned to "bufp".  If the format is not follows
 * above, exit within this function.
 */
char
buffer_read_char(char *buf, char **bufp)
{
	char		c;

	if (*buf != '\'')
	{
		fprintf(stderr, "buffer_read_char: given string does not start with '' (%s)", buf);
		exit(1);
	}

	buf++;

	c = *buf;

	buf++;

	if (*buf != '\'')
	{
		fprintf(stderr, "buffer_read_char: given string does not end with \'");
		exit(1);
	}

	buf++;

	*bufp = buf;

	return c;
}
