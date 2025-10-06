/*-------------------------------------------------------------------------
 *
 * value.c
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#include <stdlib.h>
#include "parsenodes.h"
#include "utils/palloc.h"

/*
 *	makeInteger
 */
Integer *
makeInteger(int i)
{
	Integer    *v = makeNode(Integer);

	v->ival = i;
	return v;
}

/*
 *	makeFloat
 *
 * Caller is responsible for passing a palloc'd string.
 */
Float *
makeFloat(char *numericStr)
{
	Float	   *v = makeNode(Float);

	v->fval = numericStr;
	return v;
}

/*
 *	makeBoolean
 */
Boolean *
makeBoolean(bool val)
{
	Boolean    *v = makeNode(Boolean);

	v->boolval = val;
	return v;
}

/*
 *	makeString
 *
 * Caller is responsible for passing a palloc'd string.
 */
String *
makeString(char *str)
{
	String	   *v = makeNode(String);

	v->sval = str;
	return v;
}

/*
 *	makeBitString
 *
 * Caller is responsible for passing a palloc'd string.
 */
BitString *
makeBitString(char *str)
{
	BitString  *v = makeNode(BitString);

	v->bsval = str;
	return v;
}
