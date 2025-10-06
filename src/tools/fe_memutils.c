/*-------------------------------------------------------------------------
 *
 * fe_memutils.c
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef POOL_PRIVATE
#error "This file is not expected to be compiled for pgpool utilities only"
#endif

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "utils/fe_ports.h"

void *
pg_malloc(Size size)
{
	void	   *tmp;

	/* Avoid unportable behavior of malloc(0) */
	if (size == 0)
		size = 1;
	tmp = malloc(size);
	if (!tmp)
	{
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	return tmp;
}

void *
pg_malloc0(Size size)
{
	void	   *tmp;

	tmp = pg_malloc(size);
	memset(tmp, 0, size);
	return tmp;
}

void *
pg_realloc(void *ptr, Size size)
{
	void	   *tmp;

	/* Avoid unportable behavior of realloc(NULL, 0) */
	if (ptr == NULL && size == 0)
		size = 1;
	tmp = realloc(ptr, size);
	if (!tmp)
	{
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	return tmp;
}

/*
 * "Safe" wrapper around strdup().
 */
char *
pg_strdup(const char *in)
{
	char	   *tmp;

	if (!in)
	{
		fprintf(stderr,
				"cannot duplicate null pointer (internal error)\n");
		exit(EXIT_FAILURE);
	}
	tmp = strdup(in);
	if (!tmp)
	{
		fprintf(stderr, "out of memory\n");
		exit(EXIT_FAILURE);
	}
	return tmp;
}

void
pg_free(void *ptr)
{
	if (ptr != NULL)
		free(ptr);
}

/*
 * Frontend emulation of backend memory management functions.  Useful for
 * programs that compile backend files.
 */
void *
palloc(Size size)
{
	return pg_malloc(size);
}

void *
palloc0(Size size)
{
	return pg_malloc0(size);
}

void
pfree(void *pointer)
{
	pg_free(pointer);
}

char *
pstrdup(const char *in)
{
	return pg_strdup(in);
}

void *
repalloc(void *pointer, Size size)
{
	return pg_realloc(pointer, size);
}
