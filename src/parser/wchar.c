/*-------------------------------------------------------------------------
 *
 * wchar.c
 *      PostgreSQL connection pooler and load balancer
 *
 * Copyright (c) 2003-2021 PgPool Global Development Group
 * Copyright (c) 2024-2025, pgElephant, Inc.
 *
 *-------------------------------------------------------------------------
 */
#include "pool_parser.h"
#include "utils/elog.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "pg_wchar.h"

#ifndef WIN32
#define DEF_ENC2NAME(name, codepage) { #name, PG_##name }
#else
#define DEF_ENC2NAME(name, codepage) { #name, PG_##name, codepage }
#endif

const pg_enc2name pg_enc2name_tbl[] =
{
	[PG_SQL_ASCII] = DEF_ENC2NAME(SQL_ASCII, 0),
	[PG_EUC_JP] = DEF_ENC2NAME(EUC_JP, 20932),
	[PG_EUC_CN] = DEF_ENC2NAME(EUC_CN, 20936),
	[PG_EUC_KR] = DEF_ENC2NAME(EUC_KR, 51949),
	[PG_EUC_TW] = DEF_ENC2NAME(EUC_TW, 0),
	[PG_EUC_JIS_2004] = DEF_ENC2NAME(EUC_JIS_2004, 20932),
	[PG_UTF8] = DEF_ENC2NAME(UTF8, 65001),
	[PG_MULE_INTERNAL] = DEF_ENC2NAME(MULE_INTERNAL, 0),
	[PG_LATIN1] = DEF_ENC2NAME(LATIN1, 28591),
	[PG_LATIN2] = DEF_ENC2NAME(LATIN2, 28592),
	[PG_LATIN3] = DEF_ENC2NAME(LATIN3, 28593),
	[PG_LATIN4] = DEF_ENC2NAME(LATIN4, 28594),
	[PG_LATIN5] = DEF_ENC2NAME(LATIN5, 28599),
	[PG_LATIN6] = DEF_ENC2NAME(LATIN6, 0),
	[PG_LATIN7] = DEF_ENC2NAME(LATIN7, 0),
	[PG_LATIN8] = DEF_ENC2NAME(LATIN8, 0),
	[PG_LATIN9] = DEF_ENC2NAME(LATIN9, 28605),
	[PG_LATIN10] = DEF_ENC2NAME(LATIN10, 0),
	[PG_WIN1256] = DEF_ENC2NAME(WIN1256, 1256),
	[PG_WIN1258] = DEF_ENC2NAME(WIN1258, 1258),
	[PG_WIN866] = DEF_ENC2NAME(WIN866, 866),
	[PG_WIN874] = DEF_ENC2NAME(WIN874, 874),
	[PG_KOI8R] = DEF_ENC2NAME(KOI8R, 20866),
	[PG_WIN1251] = DEF_ENC2NAME(WIN1251, 1251),
	[PG_WIN1252] = DEF_ENC2NAME(WIN1252, 1252),
	[PG_ISO_8859_5] = DEF_ENC2NAME(ISO_8859_5, 28595),
	[PG_ISO_8859_6] = DEF_ENC2NAME(ISO_8859_6, 28596),
	[PG_ISO_8859_7] = DEF_ENC2NAME(ISO_8859_7, 28597),
	[PG_ISO_8859_8] = DEF_ENC2NAME(ISO_8859_8, 28598),
	[PG_WIN1250] = DEF_ENC2NAME(WIN1250, 1250),
	[PG_WIN1253] = DEF_ENC2NAME(WIN1253, 1253),
	[PG_WIN1254] = DEF_ENC2NAME(WIN1254, 1254),
	[PG_WIN1255] = DEF_ENC2NAME(WIN1255, 1255),
	[PG_WIN1257] = DEF_ENC2NAME(WIN1257, 1257),
	[PG_KOI8U] = DEF_ENC2NAME(KOI8U, 21866),
	[PG_SJIS] = DEF_ENC2NAME(SJIS, 932),
	[PG_BIG5] = DEF_ENC2NAME(BIG5, 950),
	[PG_GBK] = DEF_ENC2NAME(GBK, 936),
	[PG_UHC] = DEF_ENC2NAME(UHC, 949),
	[PG_GB18030] = DEF_ENC2NAME(GB18030, 54936),
	[PG_JOHAB] = DEF_ENC2NAME(JOHAB, 0),
	[PG_SHIFT_JIS_2004] = DEF_ENC2NAME(SHIFT_JIS_2004, 932),
};

/* ----------
 * These are encoding names for gettext.
 *
 * This covers all encodings except MULE_INTERNAL, which is alien to gettext.
 * ----------
 */
const char *pg_enc2gettext_tbl[] =
{
	[PG_SQL_ASCII] = "US-ASCII",
	[PG_UTF8] = "UTF-8",
	[PG_MULE_INTERNAL] = NULL,
	[PG_LATIN1] = "LATIN1",
	[PG_LATIN2] = "LATIN2",
	[PG_LATIN3] = "LATIN3",
	[PG_LATIN4] = "LATIN4",
	[PG_ISO_8859_5] = "ISO-8859-5",
	[PG_ISO_8859_6] = "ISO_8859-6",
	[PG_ISO_8859_7] = "ISO-8859-7",
	[PG_ISO_8859_8] = "ISO-8859-8",
	[PG_LATIN5] = "LATIN5",
	[PG_LATIN6] = "LATIN6",
	[PG_LATIN7] = "LATIN7",
	[PG_LATIN8] = "LATIN8",
	[PG_LATIN9] = "LATIN-9",
	[PG_LATIN10] = "LATIN10",
	[PG_KOI8R] = "KOI8-R",
	[PG_KOI8U] = "KOI8-U",
	[PG_WIN1250] = "CP1250",
	[PG_WIN1251] = "CP1251",
	[PG_WIN1252] = "CP1252",
	[PG_WIN1253] = "CP1253",
	[PG_WIN1254] = "CP1254",
	[PG_WIN1255] = "CP1255",
	[PG_WIN1256] = "CP1256",
	[PG_WIN1257] = "CP1257",
	[PG_WIN1258] = "CP1258",
	[PG_WIN866] = "CP866",
	[PG_WIN874] = "CP874",
	[PG_EUC_CN] = "EUC-CN",
	[PG_EUC_JP] = "EUC-JP",
	[PG_EUC_KR] = "EUC-KR",
	[PG_EUC_TW] = "EUC-TW",
	[PG_EUC_JIS_2004] = "EUC-JP",
	[PG_SJIS] = "SHIFT-JIS",
	[PG_BIG5] = "BIG5",
	[PG_GBK] = "GBK",
	[PG_UHC] = "UHC",
	[PG_GB18030] = "GB18030",
	[PG_JOHAB] = "JOHAB",
	[PG_SHIFT_JIS_2004] = "SHIFT_JISX0213",
};

/*
 * In today's multibyte encodings other than UTF8, this two-byte sequence
 * ensures pg_encoding_mblen() == 2 && pg_encoding_verifymbstr() == 0.
 *
 * For historical reasons, several verifychar implementations opt to reject
 * this pair specifically.  Byte pair range constraints, in encoding
 * originator documentation, always excluded this pair.  No core conversion
 * could translate it.  However, longstanding verifychar implementations
 * accepted any non-NUL byte.  big5_to_euc_tw and big5_to_mic even translate
 * pairs not valid per encoding originator documentation.  To avoid tightening
 * core or non-core conversions in a security patch, we sought this one pair.
 *
 * PQescapeString() historically used spaces for BYTE1; many other values
 * could suffice for BYTE1.
 */
#define NONUTF8_INVALID_BYTE0 (0x8d)
#define NONUTF8_INVALID_BYTE1 (' ')


/*
 * Operations on multi-byte encodings are driven by a table of helper
 * functions.
 *
 * To add an encoding support, define mblen(), dsplen(), verifychar() and
 * verifystr() for the encoding.  For server-encodings, also define mb2wchar()
 * and wchar2mb() conversion functions.
 *
 * These functions generally assume that their input is validly formed.
 * The "verifier" functions, further down in the file, have to be more
 * paranoid.
 *
 * We expect that mblen() does not need to examine more than the first byte
 * of the character to discover the correct length.  GB18030 is an exception
 * to that rule, though, as it also looks at second byte.  But even that
 * behaves in a predictable way, if you only pass the first byte: it will
 * treat 4-byte encoded characters as two 2-byte encoded characters, which is
 * good enough for all current uses.
 *
 * Note: for the display output of psql to work properly, the return values
 * of the dsplen functions must conform to the Unicode standard. In particular
 * the NUL character is zero width and control characters are generally
 * width -1. It is recommended that non-ASCII encodings refer their ASCII
 * subset to the ASCII routines to ensure consistency.
 */

/*
 * SQL/ASCII
 */
static int
pg_ascii2wchar_with_len(const unsigned char *from, pg_wchar *to, int len)
{
	int			cnt = 0;

	while (len > 0 && *from)
	{
		*to++ = *from++;
		len--;
		cnt++;
	}
	*to = 0;
	return cnt;
}

static int
pg_ascii_mblen(const unsigned char *s)
{
	return 1;
}

static int
pg_ascii_dsplen(const unsigned char *s)
{
	if (*s == '\0')
		return 0;
	if (*s < 0x20 || *s == 0x7f)
		return -1;

	return 1;
}

/*
 * EUC
 */
static int
pg_euc2wchar_with_len(const unsigned char *from, pg_wchar *to, int len)
{
	int			cnt = 0;

	while (len > 0 && *from)
	{
		if (*from == SS2 && len >= 2)	/* JIS X 0201 (so called "1 byte
										 * KANA") */
		{
			from++;
			*to = (SS2 << 8) | *from++;
			len -= 2;
		}
		else if (*from == SS3 && len >= 3)	/* JIS X 0212 KANJI */
		{
			from++;
			*to = (SS3 << 16) | (*from++ << 8);
			*to |= *from++;
			len -= 3;
		}
		else if (IS_HIGHBIT_SET(*from) && len >= 2) /* JIS X 0208 KANJI */
		{
			*to = *from++ << 8;
			*to |= *from++;
			len -= 2;
		}
		else					/* must be ASCII */
		{
			*to = *from++;
			len--;
		}
		to++;
		cnt++;
	}
	*to = 0;
	return cnt;
}

static inline int
pg_euc_mblen(const unsigned char *s)
{
	int			len;

	if (*s == SS2)
		len = 2;
	else if (*s == SS3)
		len = 3;
	else if (IS_HIGHBIT_SET(*s))
		len = 2;
	else
		len = 1;
	return len;
}

static inline int
pg_euc_dsplen(const unsigned char *s)
{
	int			len;

	if (*s == SS2)
		len = 2;
	else if (*s == SS3)
		len = 2;
	else if (IS_HIGHBIT_SET(*s))
		len = 2;
	else
		len = pg_ascii_dsplen(s);
	return len;
}

/*
 * EUC_JP
 */
static int
pg_eucjp2wchar_with_len(const unsigned char *from, pg_wchar *to, int len)
{
	return pg_euc2wchar_with_len(from, to, len);
}

static int
pg_eucjp_mblen(const unsigned char *s)
{
	return pg_euc_mblen(s);
}

static int
pg_eucjp_dsplen(const unsigned char *s)
{
	int			len;

	if (*s == SS2)
		len = 1;
	else if (*s == SS3)
		len = 2;
	else if (IS_HIGHBIT_SET(*s))
		len = 2;
	else
		len = pg_ascii_dsplen(s);
	return len;
}

/*
 * EUC_KR
 */
static int
pg_euckr2wchar_with_len(const unsigned char *from, pg_wchar *to, int len)
{
	return pg_euc2wchar_with_len(from, to, len);
}

static int
pg_euckr_mblen(const unsigned char *s)
{
	return pg_euc_mblen(s);
}

static int
pg_euckr_dsplen(const unsigned char *s)
{
	return pg_euc_dsplen(s);
}

/*
 * EUC_CN
 *
 */
static int
pg_euccn2wchar_with_len(const unsigned char *from, pg_wchar *to, int len)
{
	int			cnt = 0;

	while (len > 0 && *from)
	{
		if (*from == SS2 && len >= 3)	/* code set 2 (unused?) */
		{
			from++;
			*to = (SS2 << 16) | (*from++ << 8);
			*to |= *from++;
			len -= 3;
		}
		else if (*from == SS3 && len >= 3)	/* code set 3 (unused ?) */
		{
			from++;
			*to = (SS3 << 16) | (*from++ << 8);
			*to |= *from++;
			len -= 3;
		}
		else if (IS_HIGHBIT_SET(*from) && len >= 2) /* code set 1 */
		{
			*to = *from++ << 8;
			*to |= *from++;
			len -= 2;
		}
		else
		{
			*to = *from++;
			len--;
		}
		to++;
		cnt++;
	}
	*to = 0;
	return cnt;
}

static int
pg_euccn_mblen(const unsigned char *s)
{
	int			len;

	if (IS_HIGHBIT_SET(*s))
		len = 2;
	else
		len = 1;
	return len;
}

static int
pg_euccn_dsplen(const unsigned char *s)
{
	int			len;

	if (IS_HIGHBIT_SET(*s))
		len = 2;
	else
		len = pg_ascii_dsplen(s);
	return len;
}

/*
 * EUC_TW
 *
 */
static int
pg_euctw2wchar_with_len(const unsigned char *from, pg_wchar *to, int len)
{
	int			cnt = 0;

	while (len > 0 && *from)
	{
		if (*from == SS2 && len >= 4)	/* code set 2 */
		{
			from++;
			*to = (((uint32) SS2) << 24) | (*from++ << 16);
			*to |= *from++ << 8;
			*to |= *from++;
			len -= 4;
		}
		else if (*from == SS3 && len >= 3)	/* code set 3 (unused?) */
		{
			from++;
			*to = (SS3 << 16) | (*from++ << 8);
			*to |= *from++;
			len -= 3;
		}
		else if (IS_HIGHBIT_SET(*from) && len >= 2) /* code set 2 */
		{
			*to = *from++ << 8;
			*to |= *from++;
			len -= 2;
		}
		else
		{
			*to = *from++;
			len--;
		}
		to++;
		cnt++;
	}
	*to = 0;
	return cnt;
}

static int
pg_euctw_mblen(const unsigned char *s)
{
	int			len;

	if (*s == SS2)
		len = 4;
	else if (*s == SS3)
		len = 3;
	else if (IS_HIGHBIT_SET(*s))
		len = 2;
	else
		len = 1;
	return len;
}

static int
pg_euctw_dsplen(const unsigned char *s)
{
	int			len;

	if (*s == SS2)
		len = 2;
	else if (*s == SS3)
		len = 2;
	else if (IS_HIGHBIT_SET(*s))
		len = 2;
	else
		len = pg_ascii_dsplen(s);
	return len;
}

/*
 * Convert pg_wchar to EUC_* encoding.
 * caller must allocate enough space for "to", including a trailing zero!
 * len: length of from.
 * "from" not necessarily null terminated.
 */
static int
pg_wchar2euc_with_len(const pg_wchar *from, unsigned char *to, int len)
{
	int			cnt = 0;

	while (len > 0 && *from)
	{
		unsigned char c;

		if ((c = (*from >> 24)))
		{
			*to++ = c;
			*to++ = (*from >> 16) & 0xff;
			*to++ = (*from >> 8) & 0xff;
			*to++ = *from & 0xff;
			cnt += 4;
		}
		else if ((c = (*from >> 16)))
		{
			*to++ = c;
			*to++ = (*from >> 8) & 0xff;
			*to++ = *from & 0xff;
			cnt += 3;
		}
		else if ((c = (*from >> 8)))
		{
			*to++ = c;
			*to++ = *from & 0xff;
			cnt += 2;
		}
		else
		{
			*to++ = *from;
			cnt++;
		}
		from++;
		len--;
	}
	*to = 0;
	return cnt;
}


/*
 * JOHAB
 */
static int
pg_johab_mblen(const unsigned char *s)
{
	return pg_euc_mblen(s);
}

static int
pg_johab_dsplen(const unsigned char *s)
{
	return pg_euc_dsplen(s);
}

/*
 * convert UTF8 string to pg_wchar (UCS-4)
 * caller must allocate enough space for "to", including a trailing zero!
 * len: length of from.
 * "from" not necessarily null terminated.
 */
static int
pg_utf2wchar_with_len(const unsigned char *from, pg_wchar *to, int len)
{
	int			cnt = 0;
	uint32		c1,
				c2,
				c3,
				c4;

	while (len > 0 && *from)
	{
		if ((*from & 0x80) == 0)
		{
			*to = *from++;
			len--;
		}
		else if ((*from & 0xe0) == 0xc0)
		{
			if (len < 2)
				break;			/* drop trailing incomplete char */
			c1 = *from++ & 0x1f;
			c2 = *from++ & 0x3f;
			*to = (c1 << 6) | c2;
			len -= 2;
		}
		else if ((*from & 0xf0) == 0xe0)
		{
			if (len < 3)
				break;			/* drop trailing incomplete char */
			c1 = *from++ & 0x0f;
			c2 = *from++ & 0x3f;
			c3 = *from++ & 0x3f;
			*to = (c1 << 12) | (c2 << 6) | c3;
			len -= 3;
		}
		else if ((*from & 0xf8) == 0xf0)
		{
			if (len < 4)
				break;			/* drop trailing incomplete char */
			c1 = *from++ & 0x07;
			c2 = *from++ & 0x3f;
			c3 = *from++ & 0x3f;
			c4 = *from++ & 0x3f;
			*to = (c1 << 18) | (c2 << 12) | (c3 << 6) | c4;
			len -= 4;
		}
		else
		{
			/* treat a bogus char as length 1; not ours to raise error */
			*to = *from++;
			len--;
		}
		to++;
		cnt++;
	}
	*to = 0;
	return cnt;
}


/*
 * Trivial conversion from pg_wchar to UTF-8.
 * caller should allocate enough space for "to"
 * len: length of from.
 * "from" not necessarily null terminated.
 */
static int
pg_wchar2utf_with_len(const pg_wchar *from, unsigned char *to, int len)
{
	int			cnt = 0;

	while (len > 0 && *from)
	{
		int			char_len;

		unicode_to_utf8(*from, to);
		char_len = pg_utf_mblen(to);
		cnt += char_len;
		to += char_len;
		from++;
		len--;
	}
	*to = 0;
	return cnt;
}

/*
 * Return the byte length of a UTF8 character pointed to by s
 *
 * Note: in the current implementation we do not support UTF8 sequences
 * of more than 4 bytes; hence do NOT return a value larger than 4.
 * We return "1" for any leading byte that is either flat-out illegal or
 * indicates a length larger than we support.
 *
 * pg_utf2wchar_with_len(), utf8_to_unicode(), pg_utf8_islegal(), and perhaps
 * other places would need to be fixed to change this.
 */
int
pg_utf_mblen(const unsigned char *s)
{
	int			len;

	if ((*s & 0x80) == 0)
		len = 1;
	else if ((*s & 0xe0) == 0xc0)
		len = 2;
	else if ((*s & 0xf0) == 0xe0)
		len = 3;
	else if ((*s & 0xf8) == 0xf0)
		len = 4;
#ifdef NOT_USED
	else if ((*s & 0xfc) == 0xf8)
		len = 5;
	else if ((*s & 0xfe) == 0xfc)
		len = 6;
#endif
	else
		len = 1;
	return len;
}

/*
 * This is an implementation of wcwidth() and wcswidth() as defined in
 * "The Single UNIX Specification, Version 2, The Open Group, 1997"
 * <http://www.UNIX-systems.org/online.html>
 *
 * Markus Kuhn -- 2001-09-08 -- public domain
 *
 * customised for PostgreSQL
 *
 * original available at : http://www.cl.cam.ac.uk/~mgk25/ucs/wcwidth.c
 */

struct mbinterval
{
	unsigned int first;
	unsigned int last;
};

/* auxiliary function for binary search in interval table */
static int
mbbisearch(pg_wchar ucs, const struct mbinterval *table, int max)
{
	int			min = 0;
	int			mid;

	if (ucs < table[0].first || ucs > table[max].last)
		return 0;
	while (max >= min)
	{
		mid = (min + max) / 2;
		if (ucs > table[mid].last)
			min = mid + 1;
		else if (ucs < table[mid].first)
			max = mid - 1;
		else
			return 1;
	}

	return 0;
}


/* The following functions define the column width of an ISO 10646
 * character as follows:
 *
 *	  - The null character (U+0000) has a column width of 0.
 *
 *	  - Other C0/C1 control characters and DEL will lead to a return
 *		value of -1.
 *
 *	  - Non-spacing and enclosing combining characters (general
 *		category code Mn, Me or Cf in the Unicode database) have a
 *		column width of 0.
 *
 *	  - Spacing characters in the East Asian Wide (W) or East Asian
 *		FullWidth (F) category as defined in Unicode Technical
 *		Report #11 have a column width of 2.
 *
 *	  - All remaining characters (including all printable
 *		ISO 8859-1 and WGL4 characters, Unicode control characters,
 *		etc.) have a column width of 1.
 *
 * This implementation assumes that wchar_t characters are encoded
 * in ISO 10646.
 */

static int
ucs_wcwidth(pg_wchar ucs)
{
#include "unicode_nonspacing_table.h"
#include "unicode_east_asian_fw_table.h"

	/* test for 8-bit control characters */
	if (ucs == 0)
		return 0;

	if (ucs < 0x20 || (ucs >= 0x7f && ucs < 0xa0) || ucs > 0x0010ffff)
		return -1;

	/*
	 * binary search in table of non-spacing characters
	 *
	 * XXX: In the official Unicode sources, it is possible for a character to
	 * be described as both non-spacing and wide at the same time. As of
	 * Unicode 13.0, treating the non-spacing property as the determining
	 * factor for display width leads to the correct behavior, so do that
	 * search first.
	 */
	if (mbbisearch(ucs, nonspacing,
				   sizeof(nonspacing) / sizeof(struct mbinterval) - 1))
		return 0;

	/* binary search in table of wide characters */
	if (mbbisearch(ucs, east_asian_fw,
				   sizeof(east_asian_fw) / sizeof(struct mbinterval) - 1))
		return 2;

	return 1;
}

static int
pg_utf_dsplen(const unsigned char *s)
{
	return ucs_wcwidth(utf8_to_unicode(s));
}

/*
 * convert mule internal code to pg_wchar
 * caller should allocate enough space for "to"
 * len: length of from.
 * "from" not necessarily null terminated.
 */
static int
pg_mule2wchar_with_len(const unsigned char *from, pg_wchar *to, int len)
{
	int			cnt = 0;

	while (len > 0 && *from)
	{
		if (IS_LC1(*from) && len >= 2)
		{
			*to = *from++ << 16;
			*to |= *from++;
			len -= 2;
		}
		else if (IS_LCPRV1(*from) && len >= 3)
		{
			from++;
			*to = *from++ << 16;
			*to |= *from++;
			len -= 3;
		}
		else if (IS_LC2(*from) && len >= 3)
		{
			*to = *from++ << 16;
			*to |= *from++ << 8;
			*to |= *from++;
			len -= 3;
		}
		else if (IS_LCPRV2(*from) && len >= 4)
		{
			from++;
			*to = *from++ << 16;
			*to |= *from++ << 8;
			*to |= *from++;
			len -= 4;
		}
		else
		{						/* assume ASCII */
			*to = (unsigned char) *from++;
			len--;
		}
		to++;
		cnt++;
	}
	*to = 0;
	return cnt;
}

/*
 * convert pg_wchar to mule internal code
 * caller should allocate enough space for "to"
 * len: length of from.
 * "from" not necessarily null terminated.
 */
static int
pg_wchar2mule_with_len(const pg_wchar *from, unsigned char *to, int len)
{
	int			cnt = 0;

	while (len > 0 && *from)
	{
		unsigned char lb;

		lb = (*from >> 16) & 0xff;
		if (IS_LC1(lb))
		{
			*to++ = lb;
			*to++ = *from & 0xff;
			cnt += 2;
		}
		else if (IS_LC2(lb))
		{
			*to++ = lb;
			*to++ = (*from >> 8) & 0xff;
			*to++ = *from & 0xff;
			cnt += 3;
		}
		else if (IS_LCPRV1_A_RANGE(lb))
		{
			*to++ = LCPRV1_A;
			*to++ = lb;
			*to++ = *from & 0xff;
			cnt += 3;
		}
		else if (IS_LCPRV1_B_RANGE(lb))
		{
			*to++ = LCPRV1_B;
			*to++ = lb;
			*to++ = *from & 0xff;
			cnt += 3;
		}
		else if (IS_LCPRV2_A_RANGE(lb))
		{
			*to++ = LCPRV2_A;
			*to++ = lb;
			*to++ = (*from >> 8) & 0xff;
			*to++ = *from & 0xff;
			cnt += 4;
		}
		else if (IS_LCPRV2_B_RANGE(lb))
		{
			*to++ = LCPRV2_B;
			*to++ = lb;
			*to++ = (*from >> 8) & 0xff;
			*to++ = *from & 0xff;
			cnt += 4;
		}
		else
		{
			*to++ = *from & 0xff;
			cnt += 1;
		}
		from++;
		len--;
	}
	*to = 0;
	return cnt;
}

int
pg_mule_mblen(const unsigned char *s)
{
	int			len;

	if (IS_LC1(*s))
		len = 2;
	else if (IS_LCPRV1(*s))
		len = 3;
	else if (IS_LC2(*s))
		len = 3;
	else if (IS_LCPRV2(*s))
		len = 4;
	else
		len = 1;				/* assume ASCII */
	return len;
}

static int
pg_mule_dsplen(const unsigned char *s)
{
	int			len;

	/*
	 * Note: it's not really appropriate to assume that all multibyte charsets
	 * are double-wide on screen.  But this seems an okay approximation for
	 * the MULE charsets we currently support.
	 */

	if (IS_LC1(*s))
		len = 1;
	else if (IS_LCPRV1(*s))
		len = 1;
	else if (IS_LC2(*s))
		len = 2;
	else if (IS_LCPRV2(*s))
		len = 2;
	else
		len = 1;				/* assume ASCII */

	return len;
}

/*
 * ISO8859-1
 */
static int
pg_latin12wchar_with_len(const unsigned char *from, pg_wchar *to, int len)
{
	int			cnt = 0;

	while (len > 0 && *from)
	{
		*to++ = *from++;
		len--;
		cnt++;
	}
	*to = 0;
	return cnt;
}

/*
 * Trivial conversion from pg_wchar to single byte encoding. Just ignores
 * high bits.
 * caller should allocate enough space for "to"
 * len: length of from.
 * "from" not necessarily null terminated.
 */
static int
pg_wchar2single_with_len(const pg_wchar *from, unsigned char *to, int len)
{
	int			cnt = 0;

	while (len > 0 && *from)
	{
		*to++ = *from++;
		len--;
		cnt++;
	}
	*to = 0;
	return cnt;
}

static int
pg_latin1_mblen(const unsigned char *s)
{
	return 1;
}

static int
pg_latin1_dsplen(const unsigned char *s)
{
	return pg_ascii_dsplen(s);
}

/*
 * SJIS
 */
static int
pg_sjis_mblen(const unsigned char *s)
{
	int			len;

	if (*s >= 0xa1 && *s <= 0xdf)
		len = 1;				/* 1 byte kana? */
	else if (IS_HIGHBIT_SET(*s))
		len = 2;				/* kanji? */
	else
		len = 1;				/* should be ASCII */
	return len;
}

static int
pg_sjis_dsplen(const unsigned char *s)
{
	int			len;

	if (*s >= 0xa1 && *s <= 0xdf)
		len = 1;				/* 1 byte kana? */
	else if (IS_HIGHBIT_SET(*s))
		len = 2;				/* kanji? */
	else
		len = pg_ascii_dsplen(s);	/* should be ASCII */
	return len;
}

/*
 * Big5
 */
static int
pg_big5_mblen(const unsigned char *s)
{
	int			len;

	if (IS_HIGHBIT_SET(*s))
		len = 2;				/* kanji? */
	else
		len = 1;				/* should be ASCII */
	return len;
}

static int
pg_big5_dsplen(const unsigned char *s)
{
	int			len;

	if (IS_HIGHBIT_SET(*s))
		len = 2;				/* kanji? */
	else
		len = pg_ascii_dsplen(s);	/* should be ASCII */
	return len;
}

/*
 * GBK
 */
static int
pg_gbk_mblen(const unsigned char *s)
{
	int			len;

	if (IS_HIGHBIT_SET(*s))
		len = 2;				/* kanji? */
	else
		len = 1;				/* should be ASCII */
	return len;
}

static int
pg_gbk_dsplen(const unsigned char *s)
{
	int			len;

	if (IS_HIGHBIT_SET(*s))
		len = 2;				/* kanji? */
	else
		len = pg_ascii_dsplen(s);	/* should be ASCII */
	return len;
}

/*
 * UHC
 */
static int
pg_uhc_mblen(const unsigned char *s)
{
	int			len;

	if (IS_HIGHBIT_SET(*s))
		len = 2;				/* 2byte? */
	else
		len = 1;				/* should be ASCII */
	return len;
}

static int
pg_uhc_dsplen(const unsigned char *s)
{
	int			len;

	if (IS_HIGHBIT_SET(*s))
		len = 2;				/* 2byte? */
	else
		len = pg_ascii_dsplen(s);	/* should be ASCII */
	return len;
}

/*
 * GB18030
 *	Added by Bill Huang <bhuang@redhat.com>,<bill_huanghb@ybb.ne.jp>
 */

/*
 * Unlike all other mblen() functions, this also looks at the second byte of
 * the input.  However, if you only pass the first byte of a multi-byte
 * string, and \0 as the second byte, this still works in a predictable way:
 * a 4-byte character will be reported as two 2-byte characters.  That's
 * enough for all current uses, as a client-only encoding.  It works that
 * way, because in any valid 4-byte GB18030-encoded character, the third and
 * fourth byte look like a 2-byte encoded character, when looked at
 * separately.
 */
static int
pg_gb18030_mblen(const unsigned char *s)
{
	int			len;

	if (!IS_HIGHBIT_SET(*s))
		len = 1;				/* ASCII */
	else if (*(s + 1) >= 0x30 && *(s + 1) <= 0x39)
		len = 4;
	else
		len = 2;
	return len;
}

static int
pg_gb18030_dsplen(const unsigned char *s)
{
	int			len;

	if (IS_HIGHBIT_SET(*s))
		len = 2;
	else
		len = pg_ascii_dsplen(s);	/* ASCII */
	return len;
}

/*
 *-------------------------------------------------------------------
 * multibyte sequence validators
 *
 * The verifychar functions accept "s", a pointer to the first byte of a
 * string, and "len", the remaining length of the string.  If there is a
 * validly encoded character beginning at *s, return its length in bytes;
 * else return -1.
 *
 * The verifystr functions also accept "s", a pointer to a string and "len",
 * the length of the string.  They verify the whole string, and return the
 * number of input bytes (<= len) that are valid.  In other words, if the
 * whole string is valid, verifystr returns "len", otherwise it returns the
 * byte offset of the first invalid character.  The verifystr functions must
 * test for and reject zeroes in the input.
 *
 * The verifychar functions can assume that len > 0 and that *s != '\0', but
 * they must test for and reject zeroes in any additional bytes of a
 * multibyte character.  Note that this definition allows the function for a
 * single-byte encoding to be just "return 1".
 *-------------------------------------------------------------------
 */
static int
pg_ascii_verifychar(const unsigned char *s, int len)
{
	return 1;
}

static int
pg_ascii_verifystr(const unsigned char *s, int len)
{
	const unsigned char *nullpos = memchr(s, 0, len);

	if (nullpos == NULL)
		return len;
	else
		return nullpos - s;
}

#define IS_EUC_RANGE_VALID(c)	((c) >= 0xa1 && (c) <= 0xfe)

static int
pg_eucjp_verifychar(const unsigned char *s, int len)
{
	int			l;
	unsigned char c1,
				c2;

	c1 = *s++;

	switch (c1)
	{
		case SS2:				/* JIS X 0201 */
			l = 2;
			if (l > len)
				return -1;
			c2 = *s++;
			if (c2 < 0xa1 || c2 > 0xdf)
				return -1;
			break;

		case SS3:				/* JIS X 0212 */
			l = 3;
			if (l > len)
				return -1;
			c2 = *s++;
			if (!IS_EUC_RANGE_VALID(c2))
				return -1;
			c2 = *s++;
			if (!IS_EUC_RANGE_VALID(c2))
				return -1;
			break;

		default:
			if (IS_HIGHBIT_SET(c1)) /* JIS X 0208? */
			{
				l = 2;
				if (l > len)
					return -1;
				if (!IS_EUC_RANGE_VALID(c1))
					return -1;
				c2 = *s++;
				if (!IS_EUC_RANGE_VALID(c2))
					return -1;
			}
			else
				/* must be ASCII */
			{
				l = 1;
			}
			break;
	}

	return l;
}

static int
pg_eucjp_verifystr(const unsigned char *s, int len)
{
	const unsigned char *start = s;

	while (len > 0)
	{
		int			l;

		/* fast path for ASCII-subset characters */
		if (!IS_HIGHBIT_SET(*s))
		{
			if (*s == '\0')
				break;
			l = 1;
		}
		else
		{
			l = pg_eucjp_verifychar(s, len);
			if (l == -1)
				break;
		}
		s += l;
		len -= l;
	}

	return s - start;
}

static int
pg_euckr_verifychar(const unsigned char *s, int len)
{
	int			l;
	unsigned char c1,
				c2;

	c1 = *s++;

	if (IS_HIGHBIT_SET(c1))
	{
		l = 2;
		if (l > len)
			return -1;
		if (!IS_EUC_RANGE_VALID(c1))
			return -1;
		c2 = *s++;
		if (!IS_EUC_RANGE_VALID(c2))
			return -1;
	}
	else
		/* must be ASCII */
	{
		l = 1;
	}

	return l;
}

static int
pg_euckr_verifystr(const unsigned char *s, int len)
{
	const unsigned char *start = s;

	while (len > 0)
	{
		int			l;

		/* fast path for ASCII-subset characters */
		if (!IS_HIGHBIT_SET(*s))
		{
			if (*s == '\0')
				break;
			l = 1;
		}
		else
		{
			l = pg_euckr_verifychar(s, len);
			if (l == -1)
				break;
		}
		s += l;
		len -= l;
	}

	return s - start;
}

/* EUC-CN byte sequences are exactly same as EUC-KR */
#define pg_euccn_verifychar	pg_euckr_verifychar
#define pg_euccn_verifystr	pg_euckr_verifystr

static int
pg_euctw_verifychar(const unsigned char *s, int len)
{
	int			l;
	unsigned char c1,
				c2;

	c1 = *s++;

	switch (c1)
	{
		case SS2:				/* CNS 11643 Plane 1-7 */
			l = 4;
			if (l > len)
				return -1;
			c2 = *s++;
			if (c2 < 0xa1 || c2 > 0xa7)
				return -1;
			c2 = *s++;
			if (!IS_EUC_RANGE_VALID(c2))
				return -1;
			c2 = *s++;
			if (!IS_EUC_RANGE_VALID(c2))
				return -1;
			break;

		case SS3:				/* unused */
			return -1;

		default:
			if (IS_HIGHBIT_SET(c1)) /* CNS 11643 Plane 1 */
			{
				l = 2;
				if (l > len)
					return -1;
				/* no further range check on c1? */
				c2 = *s++;
				if (!IS_EUC_RANGE_VALID(c2))
					return -1;
			}
			else
				/* must be ASCII */
			{
				l = 1;
			}
			break;
	}
	return l;
}

static int
pg_euctw_verifystr(const unsigned char *s, int len)
{
	const unsigned char *start = s;

	while (len > 0)
	{
		int			l;

		/* fast path for ASCII-subset characters */
		if (!IS_HIGHBIT_SET(*s))
		{
			if (*s == '\0')
				break;
			l = 1;
		}
		else
		{
			l = pg_euctw_verifychar(s, len);
			if (l == -1)
				break;
		}
		s += l;
		len -= l;
	}

	return s - start;
}

static int
pg_johab_verifychar(const unsigned char *s, int len)
{
	int			l,
				mbl;
	unsigned char c;

	l = mbl = pg_johab_mblen(s);

	if (len < l)
		return -1;

	if (!IS_HIGHBIT_SET(*s))
		return mbl;

	while (--l > 0)
	{
		c = *++s;
		if (!IS_EUC_RANGE_VALID(c))
			return -1;
	}
	return mbl;
}

static int
pg_johab_verifystr(const unsigned char *s, int len)
{
	const unsigned char *start = s;

	while (len > 0)
	{
		int			l;

		/* fast path for ASCII-subset characters */
		if (!IS_HIGHBIT_SET(*s))
		{
			if (*s == '\0')
				break;
			l = 1;
		}
		else
		{
			l = pg_johab_verifychar(s, len);
			if (l == -1)
				break;
		}
		s += l;
		len -= l;
	}

	return s - start;
}

static int
pg_mule_verifychar(const unsigned char *s, int len)
{
	int			l,
				mbl;
	unsigned char c;

	l = mbl = pg_mule_mblen(s);

	if (len < l)
		return -1;

	while (--l > 0)
	{
		c = *++s;
		if (!IS_HIGHBIT_SET(c))
			return -1;
	}
	return mbl;
}

static int
pg_mule_verifystr(const unsigned char *s, int len)
{
	const unsigned char *start = s;

	while (len > 0)
	{
		int			l;

		/* fast path for ASCII-subset characters */
		if (!IS_HIGHBIT_SET(*s))
		{
			if (*s == '\0')
				break;
			l = 1;
		}
		else
		{
			l = pg_mule_verifychar(s, len);
			if (l == -1)
				break;
		}
		s += l;
		len -= l;
	}

	return s - start;
}

static int
pg_latin1_verifychar(const unsigned char *s, int len)
{
	return 1;
}

static int
pg_latin1_verifystr(const unsigned char *s, int len)
{
	const unsigned char *nullpos = memchr(s, 0, len);

	if (nullpos == NULL)
		return len;
	else
		return nullpos - s;
}

static int
pg_sjis_verifychar(const unsigned char *s, int len)
{
	int			l,
				mbl;
	unsigned char c1,
				c2;

	l = mbl = pg_sjis_mblen(s);

	if (len < l)
		return -1;

	if (l == 1)					/* pg_sjis_mblen already verified it */
		return mbl;

	c1 = *s++;
	c2 = *s;
	if (!ISSJISHEAD(c1) || !ISSJISTAIL(c2))
		return -1;
	return mbl;
}

static int
pg_sjis_verifystr(const unsigned char *s, int len)
{
	const unsigned char *start = s;

	while (len > 0)
	{
		int			l;

		/* fast path for ASCII-subset characters */
		if (!IS_HIGHBIT_SET(*s))
		{
			if (*s == '\0')
				break;
			l = 1;
		}
		else
		{
			l = pg_sjis_verifychar(s, len);
			if (l == -1)
				break;
		}
		s += l;
		len -= l;
	}

	return s - start;
}

static int
pg_big5_verifychar(const unsigned char *s, int len)
{
	int			l,
				mbl;

	l = mbl = pg_big5_mblen(s);

	if (len < l)
		return -1;

	if (l == 2 &&
		s[0] == NONUTF8_INVALID_BYTE0 &&
		s[1] == NONUTF8_INVALID_BYTE1)
		return -1;

	while (--l > 0)
	{
		if (*++s == '\0')
			return -1;
	}

	return mbl;
}

static int
pg_big5_verifystr(const unsigned char *s, int len)
{
	const unsigned char *start = s;

	while (len > 0)
	{
		int			l;

		/* fast path for ASCII-subset characters */
		if (!IS_HIGHBIT_SET(*s))
		{
			if (*s == '\0')
				break;
			l = 1;
		}
		else
		{
			l = pg_big5_verifychar(s, len);
			if (l == -1)
				break;
		}
		s += l;
		len -= l;
	}

	return s - start;
}

static int
pg_gbk_verifychar(const unsigned char *s, int len)
{
	int			l,
				mbl;

	l = mbl = pg_gbk_mblen(s);

	if (len < l)
		return -1;

	if (l == 2 &&
		s[0] == NONUTF8_INVALID_BYTE0 &&
		s[1] == NONUTF8_INVALID_BYTE1)
		return -1;

	while (--l > 0)
	{
		if (*++s == '\0')
			return -1;
	}

	return mbl;
}

static int
pg_gbk_verifystr(const unsigned char *s, int len)
{
	const unsigned char *start = s;

	while (len > 0)
	{
		int			l;

		/* fast path for ASCII-subset characters */
		if (!IS_HIGHBIT_SET(*s))
		{
			if (*s == '\0')
				break;
			l = 1;
		}
		else
		{
			l = pg_gbk_verifychar(s, len);
			if (l == -1)
				break;
		}
		s += l;
		len -= l;
	}

	return s - start;
}

static int
pg_uhc_verifychar(const unsigned char *s, int len)
{
	int			l,
				mbl;

	l = mbl = pg_uhc_mblen(s);

	if (len < l)
		return -1;

	if (l == 2 &&
		s[0] == NONUTF8_INVALID_BYTE0 &&
		s[1] == NONUTF8_INVALID_BYTE1)
		return -1;

	while (--l > 0)
	{
		if (*++s == '\0')
			return -1;
	}

	return mbl;
}

static int
pg_uhc_verifystr(const unsigned char *s, int len)
{
	const unsigned char *start = s;

	while (len > 0)
	{
		int			l;

		/* fast path for ASCII-subset characters */
		if (!IS_HIGHBIT_SET(*s))
		{
			if (*s == '\0')
				break;
			l = 1;
		}
		else
		{
			l = pg_uhc_verifychar(s, len);
			if (l == -1)
				break;
		}
		s += l;
		len -= l;
	}

	return s - start;
}

static int
pg_gb18030_verifychar(const unsigned char *s, int len)
{
	int			l;

	if (!IS_HIGHBIT_SET(*s))
		l = 1;					/* ASCII */
	else if (len >= 4 && *(s + 1) >= 0x30 && *(s + 1) <= 0x39)
	{
		/* Should be 4-byte, validate remaining bytes */
		if (*s >= 0x81 && *s <= 0xfe &&
			*(s + 2) >= 0x81 && *(s + 2) <= 0xfe &&
			*(s + 3) >= 0x30 && *(s + 3) <= 0x39)
			l = 4;
		else
			l = -1;
	}
	else if (len >= 2 && *s >= 0x81 && *s <= 0xfe)
	{
		/* Should be 2-byte, validate */
		if ((*(s + 1) >= 0x40 && *(s + 1) <= 0x7e) ||
			(*(s + 1) >= 0x80 && *(s + 1) <= 0xfe))
			l = 2;
		else
			l = -1;
	}
	else
		l = -1;
	return l;
}

static int
pg_gb18030_verifystr(const unsigned char *s, int len)
{
	const unsigned char *start = s;

	while (len > 0)
	{
		int			l;

		/* fast path for ASCII-subset characters */
		if (!IS_HIGHBIT_SET(*s))
		{
			if (*s == '\0')
				break;
			l = 1;
		}
		else
		{
			l = pg_gb18030_verifychar(s, len);
			if (l == -1)
				break;
		}
		s += l;
		len -= l;
	}

	return s - start;
}

static int
pg_utf8_verifychar(const unsigned char *s, int len)
{
	int			l;

	if ((*s & 0x80) == 0)
	{
		if (*s == '\0')
			return -1;
		return 1;
	}
	else if ((*s & 0xe0) == 0xc0)
		l = 2;
	else if ((*s & 0xf0) == 0xe0)
		l = 3;
	else if ((*s & 0xf8) == 0xf0)
		l = 4;
	else
		l = 1;

	if (l > len)
		return -1;

	if (!pg_utf8_islegal(s, l))
		return -1;

	return l;
}

/*
 * The fast path of the UTF-8 verifier uses a deterministic finite automaton
 * (DFA) for multibyte characters. In a traditional table-driven DFA, the
 * input byte and current state are used to compute an index into an array of
 * state transitions. Since the address of the next transition is dependent
 * on this computation, there is latency in executing the load instruction,
 * and the CPU is not kept busy.
 *
 * Instead, we use a "shift-based" DFA as described by Per Vognsen:
 *
 * https://gist.github.com/pervognsen/218ea17743e1442e59bb60d29b1aa725
 *
 * In a shift-based DFA, the input byte is an index into array of integers
 * whose bit pattern encodes the state transitions. To compute the next
 * state, we simply right-shift the integer by the current state and apply a
 * mask. In this scheme, the address of the transition only depends on the
 * input byte, so there is better pipelining.
 *
 * The naming convention for states and transitions was adopted from a UTF-8
 * to UTF-16/32 transcoder, whose table is reproduced below:
 *
 * https://github.com/BobSteagall/utf_utils/blob/6b7a465265de2f5fa6133d653df0c9bdd73bbcf8/src/utf_utils.cpp
 *
 * ILL  ASC  CR1  CR2  CR3  L2A  L3A  L3B  L3C  L4A  L4B  L4C CLASS / STATE
 * ==========================================================================
 * err, END, err, err, err, CS1, P3A, CS2, P3B, P4A, CS3, P4B,      | BGN/END
 * err, err, err, err, err, err, err, err, err, err, err, err,      | ERR
 *                                                                  |
 * err, err, END, END, END, err, err, err, err, err, err, err,      | CS1
 * err, err, CS1, CS1, CS1, err, err, err, err, err, err, err,      | CS2
 * err, err, CS2, CS2, CS2, err, err, err, err, err, err, err,      | CS3
 *                                                                  |
 * err, err, err, err, CS1, err, err, err, err, err, err, err,      | P3A
 * err, err, CS1, CS1, err, err, err, err, err, err, err, err,      | P3B
 *                                                                  |
 * err, err, err, CS2, CS2, err, err, err, err, err, err, err,      | P4A
 * err, err, CS2, err, err, err, err, err, err, err, err, err,      | P4B
 *
 * In the most straightforward implementation, a shift-based DFA for UTF-8
 * requires 64-bit integers to encode the transitions, but with an SMT solver
 * it's possible to find state numbers such that the transitions fit within
 * 32-bit integers, as Dougall Johnson demonstrated:
 *
 * https://gist.github.com/dougallj/166e326de6ad4cf2c94be97a204c025f
 *
 * This packed representation is the reason for the seemingly odd choice of
 * state values below.
 */

/* Error */
#define	ERR  0
/* Begin */
#define	BGN 11
/* Continuation states, expect 1/2/3 continuation bytes */
#define	CS1 16
#define	CS2  1
#define	CS3  5
/* Partial states, where the first continuation byte has a restricted range */
#define	P3A  6					/* Lead was E0, check for 3-byte overlong */
#define	P3B 20					/* Lead was ED, check for surrogate */
#define	P4A 25					/* Lead was F0, check for 4-byte overlong */
#define	P4B 30					/* Lead was F4, check for too-large */
/* Begin and End are the same state */
#define	END BGN

/* the encoded state transitions for the lookup table */

/* ASCII */
#define ASC (END << BGN)
/* 2-byte lead */
#define L2A (CS1 << BGN)
/* 3-byte lead */
#define L3A (P3A << BGN)
#define L3B (CS2 << BGN)
#define L3C (P3B << BGN)
/* 4-byte lead */
#define L4A (P4A << BGN)
#define L4B (CS3 << BGN)
#define L4C (P4B << BGN)
/* continuation byte */
#define CR1 (END << CS1) | (CS1 << CS2) | (CS2 << CS3) | (CS1 << P3B) | (CS2 << P4B)
#define CR2 (END << CS1) | (CS1 << CS2) | (CS2 << CS3) | (CS1 << P3B) | (CS2 << P4A)
#define CR3 (END << CS1) | (CS1 << CS2) | (CS2 << CS3) | (CS1 << P3A) | (CS2 << P4A)
/* invalid byte */
#define ILL ERR

static int
pg_utf8_verifystr(const unsigned char *s, int len)
{
	const unsigned char *start = s;

	while (len > 0)
	{
		int			l;

		/* fast path for ASCII-subset characters */
		if (!IS_HIGHBIT_SET(*s))
		{
			if (*s == '\0')
				break;
			l = 1;
		}
		else
		{
			l = pg_utf8_verifychar(s, len);
			if (l == -1)
				break;
		}
		s += l;
		len -= l;
	}

	return s - start;
}

/*
 * Check for validity of a single UTF-8 encoded character
 *
 * This directly implements the rules in RFC3629.  The bizarre-looking
 * restrictions on the second byte are meant to ensure that there isn't
 * more than one encoding of a given Unicode character point; that is,
 * you may not use a longer-than-necessary byte sequence with high order
 * zero bits to represent a character that would fit in fewer bytes.
 * To do otherwise is to create security hazards (eg, create an apparent
 * non-ASCII character that decodes to plain ASCII).
 *
 * length is assumed to have been obtained by pg_utf_mblen(), and the
 * caller must have checked that that many bytes are present in the buffer.
 */
bool
pg_utf8_islegal(const unsigned char *source, int length)
{
	unsigned char a;

	switch (length)
	{
		default:
			/* reject lengths 5 and 6 for now */
			return false;
		case 4:
			a = source[3];
			if (a < 0x80 || a > 0xBF)
				return false;
			/* FALL THRU */
		case 3:
			a = source[2];
			if (a < 0x80 || a > 0xBF)
				return false;
			/* FALL THRU */
		case 2:
			a = source[1];
			switch (*source)
			{
				case 0xE0:
					if (a < 0xA0 || a > 0xBF)
						return false;
					break;
				case 0xED:
					if (a < 0x80 || a > 0x9F)
						return false;
					break;
				case 0xF0:
					if (a < 0x90 || a > 0xBF)
						return false;
					break;
				case 0xF4:
					if (a < 0x80 || a > 0x8F)
						return false;
					break;
				default:
					if (a < 0x80 || a > 0xBF)
						return false;
					break;
			}
			/* FALL THRU */
		case 1:
			a = *source;
			if (a >= 0x80 && a < 0xC2)
				return false;
			if (a > 0xF4)
				return false;
			break;
	}
	return true;
}

#ifndef FRONTEND

/*
 * Generic character incrementer function.
 *
 * Not knowing anything about the properties of the encoding in use, we just
 * keep incrementing the last byte until we get a validly-encoded result,
 * or we run out of values to try.  We don't bother to try incrementing
 * higher-order bytes, so there's no growth in runtime for wider characters.
 * (If we did try to do that, we'd need to consider the likelihood that 255
 * is not a valid final byte in the encoding.)
 */
static bool
pg_generic_charinc(unsigned char *charptr, int len)
{
	unsigned char *lastbyte = charptr + len - 1;
	mbchar_verifier mbverify;

	/* We can just invoke the character verifier directly. */
	mbverify = pg_wchar_table[GetDatabaseEncoding()].mbverifychar;

	while (*lastbyte < (unsigned char) 255)
	{
		(*lastbyte)++;
		if ((*mbverify) (charptr, len) == len)
			return true;
	}

	return false;
}

/*
 * UTF-8 character incrementer function.
 *
 * For a one-byte character less than 0x7F, we just increment the byte.
 *
 * For a multibyte character, every byte but the first must fall between 0x80
 * and 0xBF; and the first byte must be between 0xC0 and 0xF4.  We increment
 * the last byte that's not already at its maximum value.  If we can't find a
 * byte that's less than the maximum allowable value, we simply fail.  We also
 * need some special-case logic to skip regions used for surrogate pair
 * handling, as those should not occur in valid UTF-8.
 *
 * Note that we don't reset lower-order bytes back to their minimums, since
 * we can't afford to make an exhaustive search (see make_greater_string).
 */
static bool
pg_utf8_increment(unsigned char *charptr, int length)
{
	unsigned char a;
	unsigned char limit;

	switch (length)
	{
		default:
			/* reject lengths 5 and 6 for now */
			return false;
		case 4:
			a = charptr[3];
			if (a < 0xBF)
			{
				charptr[3]++;
				break;
			}
			/* FALL THRU */
		case 3:
			a = charptr[2];
			if (a < 0xBF)
			{
				charptr[2]++;
				break;
			}
			/* FALL THRU */
		case 2:
			a = charptr[1];
			switch (*charptr)
			{
				case 0xED:
					limit = 0x9F;
					break;
				case 0xF4:
					limit = 0x8F;
					break;
				default:
					limit = 0xBF;
					break;
			}
			if (a < limit)
			{
				charptr[1]++;
				break;
			}
			/* FALL THRU */
		case 1:
			a = *charptr;
			if (a == 0x7F || a == 0xDF || a == 0xEF || a == 0xF4)
				return false;
			charptr[0]++;
			break;
	}

	return true;
}

/*
 * EUC-JP character incrementer function.
 *
 * If the sequence starts with SS2 (0x8e), it must be a two-byte sequence
 * representing JIS X 0201 characters with the second byte ranging between
 * 0xa1 and 0xdf.  We just increment the last byte if it's less than 0xdf,
 * and otherwise rewrite the whole sequence to 0xa1 0xa1.
 *
 * If the sequence starts with SS3 (0x8f), it must be a three-byte sequence
 * in which the last two bytes range between 0xa1 and 0xfe.  The last byte
 * is incremented if possible, otherwise the second-to-last byte.
 *
 * If the sequence starts with a value other than the above and its MSB
 * is set, it must be a two-byte sequence representing JIS X 0208 characters
 * with both bytes ranging between 0xa1 and 0xfe.  The last byte is
 * incremented if possible, otherwise the second-to-last byte.
 *
 * Otherwise, the sequence is a single-byte ASCII character. It is
 * incremented up to 0x7f.
 */
static bool
pg_eucjp_increment(unsigned char *charptr, int length)
{
	unsigned char c1,
				c2;
	int			i;

	c1 = *charptr;

	switch (c1)
	{
		case SS2:				/* JIS X 0201 */
			if (length != 2)
				return false;

			c2 = charptr[1];

			if (c2 >= 0xdf)
				charptr[0] = charptr[1] = 0xa1;
			else if (c2 < 0xa1)
				charptr[1] = 0xa1;
			else
				charptr[1]++;
			break;

		case SS3:				/* JIS X 0212 */
			if (length != 3)
				return false;

			for (i = 2; i > 0; i--)
			{
				c2 = charptr[i];
				if (c2 < 0xa1)
				{
					charptr[i] = 0xa1;
					return true;
				}
				else if (c2 < 0xfe)
				{
					charptr[i]++;
					return true;
				}
			}

			/* Out of 3-byte code region */
			return false;

		default:
			if (IS_HIGHBIT_SET(c1)) /* JIS X 0208? */
			{
				if (length != 2)
					return false;

				for (i = 1; i >= 0; i--)
				{
					c2 = charptr[i];
					if (c2 < 0xa1)
					{
						charptr[i] = 0xa1;
						return true;
					}
					else if (c2 < 0xfe)
					{
						charptr[i]++;
						return true;
					}
				}

				/* Out of 2 byte code region */
				return false;
			}
			else
			{					/* ASCII, single byte */
				if (c1 > 0x7e)
					return false;
				(*charptr)++;
			}
			break;
	}

	return true;
}
#endif							/* !FRONTEND */


/*
 * Fills the provided buffer with two bytes such that:
 *   pg_encoding_mblen(dst) == 2 && pg_encoding_verifymbstr(dst) == 0
 */
void
pg_encoding_set_invalid(int encoding, char *dst)
{
	Assert(pg_encoding_max_length(encoding) > 1);

	dst[0] = (encoding == PG_UTF8 ? 0xc0 : NONUTF8_INVALID_BYTE0);
	dst[1] = NONUTF8_INVALID_BYTE1;
}

/*
 *-------------------------------------------------------------------
 * encoding info table
 *-------------------------------------------------------------------
 */
const pg_wchar_tbl pg_wchar_table[] = {
	[PG_SQL_ASCII] = {pg_ascii2wchar_with_len, pg_wchar2single_with_len, pg_ascii_mblen, pg_ascii_dsplen, pg_ascii_verifychar, pg_ascii_verifystr, 1},
	[PG_EUC_JP] = {pg_eucjp2wchar_with_len, pg_wchar2euc_with_len, pg_eucjp_mblen, pg_eucjp_dsplen, pg_eucjp_verifychar, pg_eucjp_verifystr, 3},
	[PG_EUC_CN] = {pg_euccn2wchar_with_len, pg_wchar2euc_with_len, pg_euccn_mblen, pg_euccn_dsplen, pg_euccn_verifychar, pg_euccn_verifystr, 2},
	[PG_EUC_KR] = {pg_euckr2wchar_with_len, pg_wchar2euc_with_len, pg_euckr_mblen, pg_euckr_dsplen, pg_euckr_verifychar, pg_euckr_verifystr, 3},
	[PG_EUC_TW] = {pg_euctw2wchar_with_len, pg_wchar2euc_with_len, pg_euctw_mblen, pg_euctw_dsplen, pg_euctw_verifychar, pg_euctw_verifystr, 4},
	[PG_EUC_JIS_2004] = {pg_eucjp2wchar_with_len, pg_wchar2euc_with_len, pg_eucjp_mblen, pg_eucjp_dsplen, pg_eucjp_verifychar, pg_eucjp_verifystr, 3},
	[PG_UTF8] = {pg_utf2wchar_with_len, pg_wchar2utf_with_len, pg_utf_mblen, pg_utf_dsplen, pg_utf8_verifychar, pg_utf8_verifystr, 4},
	[PG_MULE_INTERNAL] = {pg_mule2wchar_with_len, pg_wchar2mule_with_len, pg_mule_mblen, pg_mule_dsplen, pg_mule_verifychar, pg_mule_verifystr, 4},
	[PG_LATIN1] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_LATIN2] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_LATIN3] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_LATIN4] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_LATIN5] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_LATIN6] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_LATIN7] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_LATIN8] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_LATIN9] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_LATIN10] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_WIN1256] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_WIN1258] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_WIN866] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_WIN874] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_KOI8R] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_WIN1251] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_WIN1252] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_ISO_8859_5] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_ISO_8859_6] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_ISO_8859_7] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_ISO_8859_8] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_WIN1250] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_WIN1253] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_WIN1254] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_WIN1255] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_WIN1257] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_KOI8U] = {pg_latin12wchar_with_len, pg_wchar2single_with_len, pg_latin1_mblen, pg_latin1_dsplen, pg_latin1_verifychar, pg_latin1_verifystr, 1},
	[PG_SJIS] = {0, 0, pg_sjis_mblen, pg_sjis_dsplen, pg_sjis_verifychar, pg_sjis_verifystr, 2},
	[PG_BIG5] = {0, 0, pg_big5_mblen, pg_big5_dsplen, pg_big5_verifychar, pg_big5_verifystr, 2},
	[PG_GBK] = {0, 0, pg_gbk_mblen, pg_gbk_dsplen, pg_gbk_verifychar, pg_gbk_verifystr, 2},
	[PG_UHC] = {0, 0, pg_uhc_mblen, pg_uhc_dsplen, pg_uhc_verifychar, pg_uhc_verifystr, 2},
	[PG_GB18030] = {0, 0, pg_gb18030_mblen, pg_gb18030_dsplen, pg_gb18030_verifychar, pg_gb18030_verifystr, 4},
	[PG_JOHAB] = {0, 0, pg_johab_mblen, pg_johab_dsplen, pg_johab_verifychar, pg_johab_verifystr, 3},
	[PG_SHIFT_JIS_2004] = {0, 0, pg_sjis_mblen, pg_sjis_dsplen, pg_sjis_verifychar, pg_sjis_verifystr, 2},
};

/*
 * Returns the byte length of a multibyte character.
 *
 * Choose "mblen" functions based on the input string characteristics.
 * pg_encoding_mblen() can be used when ANY of these conditions are met:
 *
 * - The input string is zero-terminated
 *
 * - The input string is known to be valid in the encoding (e.g., string
 *   converted from database encoding)
 *
 * - The encoding is not GB18030 (e.g., when only database encodings are
 *   passed to 'encoding' parameter)
 *
 * encoding==GB18030 requires examining up to two bytes to determine character
 * length.  Therefore, callers satisfying none of those conditions must use
 * pg_encoding_mblen_or_incomplete() instead, as access to mbstr[1] cannot be
 * guaranteed to be within allocation bounds.
 *
 * When dealing with text that is not certainly valid in the specified
 * encoding, the result may exceed the actual remaining string length.
 * Callers that are not prepared to deal with that should use Min(remaining,
 * pg_encoding_mblen_or_incomplete()).  For zero-terminated strings, that and
 * pg_encoding_mblen_bounded() are interchangeable.
 */
int
pg_encoding_mblen(int encoding, const char *mbstr)
{
	return (PG_VALID_ENCODING(encoding) ?
			pg_wchar_table[encoding].mblen((const unsigned char *) mbstr) :
			pg_wchar_table[PG_SQL_ASCII].mblen((const unsigned char *) mbstr));
}

/*
 * Returns the byte length of a multibyte character (possibly not
 * zero-terminated), or INT_MAX if too few bytes remain to determine a length.
 */
int
pg_encoding_mblen_or_incomplete(int encoding, const char *mbstr,
								size_t remaining)
{
	/*
	 * Define zero remaining as too few, even for single-byte encodings.
	 * pg_gb18030_mblen() reads one or two bytes; single-byte encodings read
	 * zero; others read one.
	 */
	if (remaining < 1 ||
		(encoding == PG_GB18030 && IS_HIGHBIT_SET(*mbstr) && remaining < 2))
		return INT_MAX;
	return pg_encoding_mblen(encoding, mbstr);
}

/*
 * Returns the byte length of a multibyte character; but not more than the
 * distance to the terminating zero byte.  For input that might lack a
 * terminating zero, use Min(remaining, pg_encoding_mblen_or_incomplete()).
 */
int
pg_encoding_mblen_bounded(int encoding, const char *mbstr)
{
	return strnlen(mbstr, pg_encoding_mblen(encoding, mbstr));
}

/*
 * Returns the display length of a multibyte character.
 */
int
pg_encoding_dsplen(int encoding, const char *mbstr)
{
	return (PG_VALID_ENCODING(encoding) ?
			pg_wchar_table[encoding].dsplen((const unsigned char *) mbstr) :
			pg_wchar_table[PG_SQL_ASCII].dsplen((const unsigned char *) mbstr));
}

/*
 * Verify the first multibyte character of the given string.
 * Return its byte length if good, -1 if bad.  (See comments above for
 * full details of the mbverifychar API.)
 */
int
pg_encoding_verifymbchar(int encoding, const char *mbstr, int len)
{
	return (PG_VALID_ENCODING(encoding) ?
			pg_wchar_table[encoding].mbverifychar((const unsigned char *) mbstr, len) :
			pg_wchar_table[PG_SQL_ASCII].mbverifychar((const unsigned char *) mbstr, len));
}

/*
 * Verify that a string is valid for the given encoding.
 * Returns the number of input bytes (<= len) that form a valid string.
 * (See comments above for full details of the mbverifystr API.)
 */
int
pg_encoding_verifymbstr(int encoding, const char *mbstr, int len)
{
	return (PG_VALID_ENCODING(encoding) ?
			pg_wchar_table[encoding].mbverifystr((const unsigned char *) mbstr, len) :
			pg_wchar_table[PG_SQL_ASCII].mbverifystr((const unsigned char *) mbstr, len));
}

/*
 * fetch maximum length of a given encoding
 */
int
pg_encoding_max_length(int encoding)
{
	Assert(PG_VALID_ENCODING(encoding));

	/*
	 * Check for the encoding despite the assert, due to some mingw versions
	 * otherwise issuing bogus warnings.
	 */
	return PG_VALID_ENCODING(encoding) ?
		pg_wchar_table[encoding].maxmblen :
		pg_wchar_table[PG_SQL_ASCII].maxmblen;
}

#ifndef FRONTEND

/*
 * fetch maximum length of the encoding for the current database
 */
int
pg_database_encoding_max_length(void)
{
	return pg_wchar_table[GetDatabaseEncoding()].maxmblen;
}

/*
 * get the character incrementer for the encoding for the current database
 */
mbcharacter_incrementer
pg_database_encoding_character_incrementer(void)
{
	/*
	 * Eventually it might be best to add a field to pg_wchar_table[], but for
	 * now we just use a switch.
	 */
	switch (GetDatabaseEncoding())
	{
		case PG_UTF8:
			return pg_utf8_increment;

		case PG_EUC_JP:
			return pg_eucjp_increment;

		default:
			return pg_generic_charinc;
	}
}

/*
 * Verify mbstr to make sure that it is validly encoded in the current
 * database encoding.  Otherwise same as pg_verify_mbstr().
 */
bool
pg_verifymbstr(const char *mbstr, int len, bool noError)
{
	return
		pg_verify_mbstr_len(GetDatabaseEncoding(), mbstr, len, noError) >= 0;
}

/*
 * Verify mbstr to make sure that it is validly encoded in the specified
 * encoding.
 */
bool
pg_verify_mbstr(int encoding, const char *mbstr, int len, bool noError)
{
	return pg_verify_mbstr_len(encoding, mbstr, len, noError) >= 0;
}

/*
 * Convert a single Unicode code point into a string in the server encoding.
 *
 * The code point given by "c" is converted and stored at *s, which must
 * have at least MAX_UNICODE_EQUIVALENT_STRING+1 bytes available.
 * The output will have a trailing '\0'.  Throws error if the conversion
 * cannot be performed.
 *
 * Note that this relies on having previously looked up any required
 * conversion function.  That's partly for speed but mostly because the parser
 * may call this outside any transaction, or in an aborted transaction.
 */
void
pg_unicode_to_server(pg_wchar c, unsigned char *s)
{
#ifdef NOT_USED
	unsigned char c_as_utf8[MAX_MULTIBYTE_CHAR_LEN + 1];
	int			c_as_utf8_len;
	int			server_encoding;

	/*
	 * Complain if invalid Unicode code point.  The choice of errcode here is
	 * debatable, but really our caller should have checked this anyway.
	 */
	if (!is_valid_unicode_codepoint(c))
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("invalid Unicode code point")));

	/* Otherwise, if it's in ASCII range, conversion is trivial */
	if (c <= 0x7F)
	{
		s[0] = (unsigned char) c;
		s[1] = '\0';
		return;
	}

	/* If the server encoding is UTF-8, we just need to reformat the code */
	server_encoding = GetDatabaseEncoding();
	if (server_encoding == PG_UTF8)
	{
		unicode_to_utf8(c, s);
		s[pg_utf_mblen(s)] = '\0';
		return;
	}

	/* For all other cases, we must have a conversion function available */
	if (Utf8ToServerConvProc == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("conversion between %s and %s is not supported",
						pg_enc2name_tbl[PG_UTF8].name,
						GetDatabaseEncodingName())));

	/* Construct UTF-8 source string */
	unicode_to_utf8(c, c_as_utf8);
	c_as_utf8_len = pg_utf_mblen(c_as_utf8);
	c_as_utf8[c_as_utf8_len] = '\0';

	/* Convert, or throw error if we can't */
	FunctionCall5(Utf8ToServerConvProc,
				  Int32GetDatum(PG_UTF8),
				  Int32GetDatum(server_encoding),
				  CStringGetDatum(c_as_utf8),
				  CStringGetDatum(s),
				  Int32GetDatum(c_as_utf8_len));
#endif
}

/*
 * Verify mbstr to make sure that it is validly encoded in the specified
 * encoding.
 *
 * mbstr is not necessarily zero terminated; length of mbstr is
 * specified by len.
 *
 * If OK, return length of string in the encoding.
 * If a problem is found, return -1 when noError is
 * true; when noError is false, ereport() a descriptive message.
 */
int
pg_verify_mbstr_len(int encoding, const char *mbstr, int len, bool noError)
{
	mbchar_verifier mbverify;
	int			mb_len;

	Assert(PG_VALID_ENCODING(encoding));

	/*
	 * In single-byte encodings, we need only reject nulls (\0).
	 */
	if (pg_encoding_max_length(encoding) <= 1)
	{
		const char *nullpos = memchr(mbstr, 0, len);

		if (nullpos == NULL)
			return len;
		if (noError)
			return -1;
		report_invalid_encoding(encoding, nullpos, 1);
	}

	/* fetch function pointer just once */
	mbverify = pg_wchar_table[encoding].mbverifychar;

	mb_len = 0;

	while (len > 0)
	{
		int			l;

		/* fast path for ASCII-subset characters */
		if (!IS_HIGHBIT_SET(*mbstr))
		{
			if (*mbstr != '\0')
			{
				mb_len++;
				mbstr++;
				len--;
				continue;
			}
			if (noError)
				return -1;
			report_invalid_encoding(encoding, mbstr, len);
		}

		l = (*mbverify) ((const unsigned char *) mbstr, len);

		if (l < 0)
		{
			if (noError)
				return -1;
			report_invalid_encoding(encoding, mbstr, len);
		}

		mbstr += l;
		len -= l;
		mb_len++;
	}
	return mb_len;
}

/*
 * check_encoding_conversion_args: check arguments of a conversion function
 *
 * "expected" arguments can be either an encoding ID or -1 to indicate that
 * the caller will check whether it accepts the ID.
 *
 * Note: the errors here are not really user-facing, so elog instead of
 * ereport seems sufficient.  Also, we trust that the "expected" encoding
 * arguments are valid encoding IDs, but we don't trust the actuals.
 */
void
check_encoding_conversion_args(int src_encoding,
							   int dest_encoding,
							   int len,
							   int expected_src_encoding,
							   int expected_dest_encoding)
{
	if (!PG_VALID_ENCODING(src_encoding))
		elog(ERROR, "invalid source encoding ID: %d", src_encoding);
	if (src_encoding != expected_src_encoding && expected_src_encoding >= 0)
		elog(ERROR, "expected source encoding \"%s\", but got \"%s\"",
			 pg_enc2name_tbl[expected_src_encoding].name,
			 pg_enc2name_tbl[src_encoding].name);
	if (!PG_VALID_ENCODING(dest_encoding))
		elog(ERROR, "invalid destination encoding ID: %d", dest_encoding);
	if (dest_encoding != expected_dest_encoding && expected_dest_encoding >= 0)
		elog(ERROR, "expected destination encoding \"%s\", but got \"%s\"",
			 pg_enc2name_tbl[expected_dest_encoding].name,
			 pg_enc2name_tbl[dest_encoding].name);
	if (len < 0)
		elog(ERROR, "encoding conversion length must not be negative");
}

/*
 * report_invalid_encoding: complain about invalid multibyte character
 *
 * note: len is remaining length of string, not length of character;
 * len must be greater than zero, as we always examine the first byte.
 */
void
report_invalid_encoding(int encoding, const char *mbstr, int len)
{
	int			l = pg_encoding_mblen(encoding, mbstr);
	char		buf[8 * 5 + 1];
	char	   *p = buf;
	int			j,
				jlimit;

	jlimit = Min(l, len);
	jlimit = Min(jlimit, 8);	/* prevent buffer overrun */

	for (j = 0; j < jlimit; j++)
	{
		p += sprintf(p, "0x%02x", (unsigned char) mbstr[j]);
		if (j < jlimit - 1)
			p += sprintf(p, " ");
	}

	ereport(ERROR,
			(errcode(ERRCODE_CHARACTER_NOT_IN_REPERTOIRE),
			 errmsg("invalid byte sequence for encoding \"%s\": %s",
					pg_enc2name_tbl[encoding].name,
					buf)));
}

/*
 * report_untranslatable_char: complain about untranslatable character
 *
 * note: len is remaining length of string, not length of character;
 * len must be greater than zero, as we always examine the first byte.
 */
void
report_untranslatable_char(int src_encoding, int dest_encoding,
						   const char *mbstr, int len)
{
	int			l = pg_encoding_mblen(src_encoding, mbstr);
	char		buf[8 * 5 + 1];
	char	   *p = buf;
	int			j,
				jlimit;

	jlimit = Min(l, len);
	jlimit = Min(jlimit, 8);	/* prevent buffer overrun */

	for (j = 0; j < jlimit; j++)
	{
		p += sprintf(p, "0x%02x", (unsigned char) mbstr[j]);
		if (j < jlimit - 1)
			p += sprintf(p, " ");
	}

	ereport(ERROR,
			(errcode(ERRCODE_UNTRANSLATABLE_CHARACTER),
			 errmsg("character with byte sequence %s in encoding \"%s\" has no equivalent in encoding \"%s\"",
					buf,
					pg_enc2name_tbl[src_encoding].name,
					pg_enc2name_tbl[dest_encoding].name)));
}

#endif							/* !FRONTEND */
