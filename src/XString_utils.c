/****************************************************************************
 *  Low-level manipulation of XString, XStringSet and XStringList objects   *
 *                           Author: Herve Pages                            *
 ****************************************************************************/
#include "Biostrings.h"


/****************************************************************************
 * Encoding/decoding XString data.
 */

#define STATIC_LKUP_LENGTH 256

static int DNA_enc_lkup[STATIC_LKUP_LENGTH], DNA_dec_lkup[STATIC_LKUP_LENGTH];
static int RNA_enc_lkup[STATIC_LKUP_LENGTH], RNA_dec_lkup[STATIC_LKUP_LENGTH];

static const int *get_enc_lkup(const char *class)
{
	if (strcmp(class, "DNAString") == 0)
		return DNA_enc_lkup;
	else if (strcmp(class, "RNAString") == 0)
		return RNA_enc_lkup;
	return NULL;
}

static void copy_lkup(const int *lkup1, int len1, int *lkup2, int len2)
{
	int i;

	if (len1 > len2)
		error("Biostrings internal error in copy_lkup(): len1 > len2");
	for (i = 0; i < len1; i++)
		lkup2[i] = lkup1[i];
	for ( ; i < len2; i++)
		lkup2[i] = NA_INTEGER;
	return;
}

SEXP init_DNAlkups(SEXP enc_lkup, SEXP dec_lkup)
{
	copy_lkup(INTEGER(enc_lkup), LENGTH(enc_lkup),
		  DNA_enc_lkup, STATIC_LKUP_LENGTH);
	copy_lkup(INTEGER(dec_lkup), LENGTH(dec_lkup),
		  DNA_dec_lkup, STATIC_LKUP_LENGTH);
	return R_NilValue;
}

char _DNAencode(char c)
{
	int code;

	code = DNA_enc_lkup[(unsigned char) c];
	if (code == NA_INTEGER)
		error("_DNAencode: key %d not in lookup table", (int) c);
	return code;
}

char _DNAdecode(char code)
{
	int c;

	c = DNA_dec_lkup[(unsigned char) code];
	if (c == NA_INTEGER)
		error("_DNAdecode: key %d not in lookup table", (int) code);
	return c;
}

SEXP init_RNAlkups(SEXP enc_lkup, SEXP dec_lkup)
{
	copy_lkup(INTEGER(enc_lkup), LENGTH(enc_lkup),
		  RNA_enc_lkup, STATIC_LKUP_LENGTH);
	copy_lkup(INTEGER(dec_lkup), LENGTH(dec_lkup),
		  RNA_dec_lkup, STATIC_LKUP_LENGTH);
	return R_NilValue;
}

char _RNAencode(char c)
{
	int code;

	code = RNA_enc_lkup[(unsigned char) c];
	if (code == NA_INTEGER)
		error("_RNAencode: key %d not in lookup table", (int) c);
	return code;
}

char _RNAdecode(char code)
{
	int c;

	c = RNA_dec_lkup[(unsigned char) code];
	if (c == NA_INTEGER)
		error("_RNAdecode: key %d not in lookup table", (int) code);
	return (char) c;
}


/****************************************************************************
 * Low-level manipulation of XString objects.
 */

const char *get_class(SEXP x)
{
	return CHAR(STRING_ELT(GET_CLASS(x), 0));
}

static SEXP getXString_data(SEXP x)
{
	return GET_SLOT(x, install("data"));
}

const char *_get_XString_charseq(SEXP x, int *length)
{
	SEXP xp;
	int offset;

	xp = GET_SLOT(getXString_data(x), install("xp"));
	offset = INTEGER(GET_SLOT(x, install("offset")))[0];
	*length = INTEGER(GET_SLOT(x, install("length")))[0];
	return (const char *) (RAW(R_ExternalPtrTag(xp)) + offset);
}

/*
 * NOT a .Call() entry point!
 */
SEXP _new_XString(const char *class, SEXP data, int offset, int length)
{
	SEXP class_def, ans;

	class_def = MAKE_CLASS(class);
	PROTECT(ans = NEW_OBJECT(class_def));
	SET_SLOT(ans, mkChar("data"), data);
	SET_SLOT(ans, mkChar("offset"), ScalarInteger(offset));
	SET_SLOT(ans, mkChar("length"), ScalarInteger(length));
	UNPROTECT(1);
	return ans;
}

SEXP _new_XString_from_CharAArr(const char *class, CharAArr seqs)
{
	const int *enc_lkup;
        SEXP lkup, data, ans;

	enc_lkup = get_enc_lkup(class);
	if (enc_lkup == NULL) {
		lkup = R_NilValue;
	} else {
		PROTECT(lkup = NEW_INTEGER(STATIC_LKUP_LENGTH));
		copy_lkup(enc_lkup, STATIC_LKUP_LENGTH,
			  INTEGER(lkup), LENGTH(lkup));
	}
	PROTECT(data = _new_XRaw_from_CharAArr(seqs, lkup));
	PROTECT(ans = _new_XString(class, data, 0, _get_XRaw_length(data)));
	if (enc_lkup == NULL)
		UNPROTECT(2);
	else
		UNPROTECT(3);
	return ans;
}


/****************************************************************************
 * Low-level manipulation of XStringSet objects.
 */

int _get_XStringSet_length(SEXP x)
{
	// Because an XStringSet object IS an .IRanges object
	return _get_IRanges_length(x);
}

const char *_get_XStringSet_charseq(SEXP x, int i, int *nchar)
{
	SEXP super;
	int start, super_length;
	const char *super_seq;

	start = _get_IRanges_start(x)[i];
	*nchar = _get_IRanges_width(x)[i];
	super = GET_SLOT(x, install("super"));
	super_seq = _get_XString_charseq(super, &super_length);
	return super_seq + start - 1;
}

/*
 * Assume that the sequences in 'seqs' are NOT already encoded.
 */
SEXP _new_XStringSet(const char *baseClass, CharAArr seqs)
{
	char classbuf[13]; // longest string will be "DNAStringSet"
	SEXP class_def, ans, super, tmp, ranges;

	snprintf(classbuf, sizeof(classbuf), "%sSet", baseClass);
	class_def = MAKE_CLASS(classbuf);
	PROTECT(ans = NEW_OBJECT(class_def));
	PROTECT(super = _new_XString_from_CharAArr(baseClass, seqs));
	SET_SLOT(ans, mkChar("super"), super);
	UNPROTECT(1);
	PROTECT(tmp = _new_IRanges_from_CharAArr(seqs));
	ranges = GET_SLOT(tmp, install("ranges"));
	SET_SLOT(ans, mkChar("ranges"), ranges);
        UNPROTECT(2);
        return ans;
}

void _set_XStringSet_names(SEXP x, CharAArr names)
{
	error("_set_XStringSet_names() is not ready yet");
	return;
}


/****************************************************************************
 * Low-level manipulation of XStringList objects.
 */

int _get_XStringList_length(SEXP x)
{
	return LENGTH(GET_SLOT(x, install("seqs")));
}

const char *_get_XStringList_charseq(SEXP x, int i, int *nchar)
{
	SEXP seqs, seq;

	seqs = GET_SLOT(x, install("seqs"));
	seq = VECTOR_ELT(seqs, i);
	return _get_XString_charseq(seq, nchar);
}

/* 'x_seqs' must be the list, NOT the XStringList object!
 * TODO: make this work directly on the XStringList object and use the
 * 2 helper functions above to simplify the code.
 */
SEXP XStrings_to_nchars(SEXP x_seqs)
{
	SEXP ans, x_seq;
	int nseq, i, *seq_length;

	nseq = LENGTH(x_seqs);
	PROTECT(ans = NEW_INTEGER(nseq));
	for (i = 0, seq_length = INTEGER(ans); i < nseq; i++, seq_length++) {
		x_seq = VECTOR_ELT(x_seqs, i);
		_get_XString_charseq(x_seq, seq_length);
	}
	UNPROTECT(1);
	return ans;
}
