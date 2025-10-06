/*-------------------------------------------------------------------------
 *
 * pool_parser.h
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#ifndef POOL_PARSER_H
#define POOL_PARSER_H

#include "../pool_type.h"
#include <setjmp.h>


extern jmp_buf jmpbuffer;
extern int	server_version_num;


/* from include/postgresql_ext.h */
#define InvalidOid		((Oid) 0)

/* from include/c.h */

/*
 * CppAsString
 *		Convert the argument to a string, using the C preprocessor.
 * CppConcat
 *		Concatenate two arguments together, using the C preprocessor.
 *
 * Note: There used to be support here for pre-ANSI C compilers that didn't
 * support # and ##.  Nowadays, these macros are just for clarity and/or
 * backward compatibility with existing PostgreSQL code.
 */
#define CppAsString(identifier) #identifier
#define CppConcat(x, y)			x##y

/*
 * Index
 *		Index into any memory resident array.
 *
 * Note:
 *		Indices are non negative.
 */
typedef unsigned int Index;

/*
 * lengthof
 *		Number of elements in an array.
 */
#define lengthof(array) (sizeof(array) / sizeof(((array)[0])))

/*
 * endof
 *		Address of the element one past the last in an array.
 */
#define endof(array) (&(array)[lengthof(array)])

/*
 * pg_noreturn corresponds to the C11 noreturn/_Noreturn function specifier.
 * We can't use the standard name "noreturn" because some third-party code
 * uses __attribute__((noreturn)) in headers, which would get confused if
 * "noreturn" is defined to "_Noreturn", as is done by <stdnoreturn.h>.
 *
 * In a declaration, function specifiers go before the function name.  The
 * common style is to put them before the return type.  (The MSVC fallback has
 * the same requirement.  The GCC fallback is more flexible.)
 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#define pg_noreturn _Noreturn
#elif defined(__GNUC__) || defined(__SUNPRO_C)
#define pg_noreturn __attribute__((noreturn))
#elif defined(_MSC_VER)
#define pg_noreturn __declspec(noreturn)
#else
#define pg_noreturn
#endif

/* GCC and XLC support format attributes */
#if defined(__GNUC__) || defined(__IBMC__)
#define pg_attribute_format_arg(a) __attribute__((format_arg(a)))
#define pg_attribute_printf(f,a) __attribute__((format(PG_PRINTF_ATTRIBUTE, f, a)))
#else
#define pg_attribute_format_arg(a)
#define pg_attribute_printf(f,a)
#endif

/* GCC, Sunpro and XLC support aligned, packed and noreturn */
#if defined(__GNUC__) || defined(__SUNPRO_C) || defined(__IBMC__)
#define pg_attribute_aligned(a) __attribute__((aligned(a)))
#define pg_attribute_packed() __attribute__((packed))
#else
/*
 * NB: aligned and packed are not given default definitions because they
 * affect code functionality; they *must* be implemented by the compiler
 * if they are to be used.
 */
#endif

/*
 * Max
 *		Return the maximum of two numbers.
 */
#define Max(x, y)		((x) > (y) ? (x) : (y))

/*
 * Min
 *		Return the minimum of two numbers.
 */
#define Min(x, y)		((x) < (y) ? (x) : (y))

/* msb for char */
#define HIGHBIT					(0x80)
#define IS_HIGHBIT_SET(ch)		((unsigned char)(ch) & HIGHBIT)


/* from include/access/attnum.h */
/*
 * user defined attribute numbers start at 1.   -ay 2/95
 */
typedef int16 AttrNumber;


/* from include/utils/datetime.h */
/* date and datetime */
#define RESERV	0
#define MONTH	1
#define YEAR	2
#define DAY		3
#define JULIAN	4
#define TZ		5
#define DTZ		6
#define DTZMOD	7
#define IGNORE_DTF	8
#define AMPM	9
#define HOUR	10
#define MINUTE	11
#define SECOND	12
#define DOY		13
#define DOW		14
#define UNITS	15
#define ADBC	16
/* these are only for relative dates */
#define AGO		17
#define ABS_BEFORE		18
#define ABS_AFTER		19
/* generic fields to help with parsing */
#define ISODATE 20
#define ISOTIME 21
/* reserved for unrecognized string values */
#define UNKNOWN_FIELD	31
#define MAX_TIMESTAMP_PRECISION 6
#define MAX_INTERVAL_PRECISION 6
#define INTERVAL_FULL_RANGE (0x7FFF)
#define INTERVAL_RANGE_MASK (0x7FFF)
#define INTERVAL_FULL_PRECISION (0xFFFF)
#define INTERVAL_PRECISION_MASK (0xFFFF)

#define INTERVAL_MASK(b) (1 << (b))
#define INTERVAL_TYPMOD(p,r) ((((r) & INTERVAL_RANGE_MASK) << 16) | ((p) & INTERVAL_PRECISION_MASK))
#define INTERVAL_PRECISION(t) ((t) & INTERVAL_PRECISION_MASK)
#define INTERVAL_RANGE(t) (((t) >> 16) & INTERVAL_RANGE_MASK)


/* from include/storage/lock.h */
/* lock */
/* NoLock is not a lock mode, but a flag value meaning "don't get a lock" */
#define NoLock					0

#define AccessShareLock			1	/* SELECT */
#define RowShareLock			2	/* SELECT FOR UPDATE/FOR SHARE */
#define RowExclusiveLock		3	/* INSERT, UPDATE, DELETE */
#define ShareUpdateExclusiveLock 4	/* VACUUM (non-FULL) */
#define ShareLock				5	/* CREATE INDEX */
#define ShareRowExclusiveLock	6	/* like EXCLUSIVE MODE, but allows ROW
									 * SHARE */
#define ExclusiveLock			7	/* blocks ROW SHARE/SELECT...FOR UPDATE */
#define AccessExclusiveLock		8	/* ALTER TABLE, DROP TABLE, VACUUM FULL,
									 * and unqualified LOCK TABLE */

#define DEFAULT_INDEX_TYPE	"btree"
#define NUMERIC_MAX_PRECISION		1000

#define VARHDRSZ		((int32) sizeof(int32))

#define MAX_TIME_PRECISION 6

/*
 * We require C99, hence the compiler should understand flexible array
 * members.  However, for documentation purposes we still consider it to be
 * project style to write "field[FLEXIBLE_ARRAY_MEMBER]" not just "field[]".
 * When computing the size of such an object, use "offsetof(struct s, f)"
 * for portability.  Don't use "offsetof(struct s, f[0])", as this doesn't
 * work with MSVC and with C++ compilers.
 */
#define FLEXIBLE_ARRAY_MEMBER	/* empty */

#endif							/* POOL_PARSER_H */
