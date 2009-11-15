/****************************************************************************
 *                       RoSeqs low-level utilities                         *
 *                           Author: Herve Pages                            *
 ****************************************************************************/
#include "Biostrings.h"
#include "IRanges_interface.h"
#include <S.h> /* for Salloc() */

static int debug = 0;

SEXP debug_RoSeqs_utils()
{
#ifdef DEBUG_BIOSTRINGS
	debug = !debug;
	Rprintf("Debug mode turned %s in 'RoSeqs_utils.c'\n",
		debug ? "on" : "off");
#else
	Rprintf("Debug mode not available in 'RoSeqs_utils.c'\n");
#endif
	return R_NilValue;
}

RoSeqs _alloc_RoSeqs(int nelt)
{
	RoSeqs seqs;

	seqs.elts = Salloc((long) nelt, cachedCharSeq);
	seqs.nelt = nelt;
	return seqs;
}


/*****************************************************************************
 * "Narrowing" a RoSeqs struct.
 */

void _narrow_RoSeqs(RoSeqs *seqs, SEXP start, SEXP width)
{
	int i, s, w;
	const int *s_p, *w_p;
	cachedCharSeq *seq;

	if (LENGTH(start) != seqs->nelt || LENGTH(width) != seqs->nelt)
		error("Biostrings internal error in _narrow_RoSeqs(): "
		      "'start' and 'width' must have the same length as 'seqs'");
	for (i = 0, seq = seqs->elts, s_p = INTEGER(start), w_p = INTEGER(width);
	     i < seqs->nelt;
	     i++, seq++, s_p++, w_p++)
	{
		s = *s_p;
		w = *w_p;
		if (s == NA_INTEGER || w == NA_INTEGER)
			error("Biostrings internal error in _narrow_RoSeqs():"
			      "NAs in 'start' or 'width' are not supported");
		s--; // 0-based start (offset)
		if (s < 0 || w < 0 || s + w > seq->length)
			error("Biostrings internal error in _narrow_RoSeqs():"
			      "invalid narrowing");
		seq->seq += s;
		seq->length = w;
	}
	return;
}


/*****************************************************************************
 * From a cachedCharSeq struct to a character string.
 *
 * TODO: Move to the IRanges package.
 */

SEXP _new_CHARSXP_from_cachedCharSeq(const cachedCharSeq *seq, SEXP lkup)
{
	// IMPORTANT: We use user-controlled memory for this private memory
	// pool so it is persistent between calls to .Call().
	// It will last until the end of the R session and can only grow
	// during the session. It is NOT a memory leak!
	static int bufsize = 0;
	static char *buf = NULL;
	int new_bufsize;
	char *new_buf;

	new_bufsize = seq->length + 1;
	if (new_bufsize > bufsize) {
		new_buf = (char *) realloc(buf, new_bufsize);
		if (new_buf == NULL)
			error("_new_CHARSXP_from_cachedCharSeq(): "
			      "call to realloc() failed");
		buf = new_buf;
		bufsize = new_bufsize;
	}
	if (lkup == R_NilValue) {
		Ocopy_byteblocks_to_i1i2(0, seq->length - 1,
			buf, seq->length,
			seq->seq, seq->length, sizeof(char));
	} else {
		Ocopy_bytes_to_i1i2_with_lkup(0, seq->length - 1,
			buf, seq->length,
			seq->seq, seq->length,
			INTEGER(lkup), LENGTH(lkup));
	}
	buf[seq->length] = 0;
	return mkChar(buf);
}


/*****************************************************************************
 * From a character vector to a RoSeqs struct and vice versa.
 */

RoSeqs _new_RoSeqs_from_STRSXP(int nelt, SEXP x)
{
	RoSeqs seqs;
	cachedCharSeq *elt1;
	SEXP elt2;
	int i;

	if (nelt > LENGTH(x))
		error("_new_RoSeqs_from_STRSXP(): "
		      "'nelt' must be <= 'LENGTH(x)'");
	seqs = _alloc_RoSeqs(nelt);
	for (i = 0, elt1 = seqs.elts; i < nelt; i++, elt1++) {
		elt2 = STRING_ELT(x, i);
		if (elt2 == NA_STRING)
			error("input sequence %d is NA", i+1);
		elt1->seq = CHAR(elt2);
		elt1->length = LENGTH(elt2);
	}
	return seqs;
}

SEXP _new_STRSXP_from_RoSeqs(const RoSeqs *seqs, SEXP lkup)
{
	SEXP ans, ans_elt;
	int i;
	const cachedCharSeq *seq;

	PROTECT(ans = NEW_CHARACTER(seqs->nelt));
	for (i = 0, seq = seqs->elts; i < seqs->nelt; i++, seq++) {
		PROTECT(ans_elt = _new_CHARSXP_from_cachedCharSeq(seq, lkup));
		SET_STRING_ELT(ans, i, ans_elt);
		UNPROTECT(1);
	}
	UNPROTECT(1);
	return ans;
}


/*****************************************************************************
 * From a RoSeqs struct to a SharedRaw object.
 */

SEXP _new_SharedRaw_from_RoSeqs(const RoSeqs *seqs, SEXP lkup)
{
        SEXP tag, ans;
        int tag_length, i;
        const cachedCharSeq *seq;
        char *dest;

        tag_length = 0;
        for (i = 0, seq = seqs->elts; i < seqs->nelt; i++, seq++)
                tag_length += seq->length;
        PROTECT(tag = NEW_RAW(tag_length));
        dest = (char *) RAW(tag);
        for (i = 0, seq = seqs->elts; i < seqs->nelt; i++, seq++) {
                if (lkup == R_NilValue) {
                        Ocopy_byteblocks_to_i1i2(0, seq->length - 1,
                                dest, seq->length,
                                seq->seq, seq->length, sizeof(char));
                } else {
                        Ocopy_bytes_to_i1i2_with_lkup(0, seq->length - 1,
                                dest, seq->length,
                                seq->seq, seq->length,
                                INTEGER(lkup), LENGTH(lkup));
                }
                dest += seq->length;
        }
        PROTECT(ans = new_SharedVector("SharedRaw", tag));
        UNPROTECT(2);
        return ans;
}


/*****************************************************************************
 * From a character vector to a SharedRaw object.
 *
 * --- .Call ENTRY POINT ---
 * Arguments:
 *   x: a character vector;
 *   start/width: integer vectors of the same length as 'x' and describing a
 *                valid "narrowing" of 'x';
 *   lkup: lookup table for encoding the letters in 'x';
 *   collapse: not yet supported.
 * TODO: Support the 'collapse' argument
 */

SEXP new_SharedRaw_from_STRSXP(SEXP x, SEXP start, SEXP width,
                SEXP collapse, SEXP lkup)
{
        int nseq;
        RoSeqs seqs;

        nseq = LENGTH(start);
        if (collapse == R_NilValue) {
                if (nseq != 1)
                        error("'collapse' must be specified when the number "
                              "of input sequences is not exactly 1");
        } else {
                if (LENGTH(collapse) != 1
                 || LENGTH(STRING_ELT(collapse, 0)) != 0)
                        error("'collapse' can only be NULL "
                              "or the empty string for now");
        }
        seqs = _new_RoSeqs_from_STRSXP(nseq, x);
        _narrow_RoSeqs(&seqs, start, width);
        return _new_SharedRaw_from_RoSeqs(&seqs, lkup);
}


/*****************************************************************************
 * From a CharAEAE buffer to a RoSeqs struct.
 */

RoSeqs _new_RoSeqs_from_CharAEAE(const CharAEAE *char_aeae)
{
	RoSeqs seqs;
	cachedCharSeq *elt1;
	CharAE *elt2;
	int i;

	seqs = _alloc_RoSeqs(char_aeae->nelt);
	for (i = 0, elt1 = seqs.elts, elt2 = char_aeae->elts;
	     i < char_aeae->nelt;
	     i++, elt1++, elt2++)
	{
		elt1->seq = elt2->elts;
		elt1->length = elt2->nelt;
	}
	return seqs;
}


/*****************************************************************************
 * From a RoSeqs struct to an IRanges object.
 * Only the lengths of the sequences holded by RoSeqs are considered.
 */

SEXP _new_IRanges_from_RoSeqs(const char *classname, const RoSeqs *seqs)
{
	const cachedCharSeq *seq;
	SEXP start, width, ans;
	int *start_elt, *width_elt, *start_prev_elt, i;

#ifdef DEBUG_BIOSTRINGS
	if (debug) {
		Rprintf("[DEBUG] _new_IRanges_from_RoSeqs(): BEGIN\n");
	}
#endif
	seq = seqs->elts;
	PROTECT(start = NEW_INTEGER(seqs->nelt));
	PROTECT(width = NEW_INTEGER(seqs->nelt));
	start_elt = INTEGER(start);
	width_elt = INTEGER(width);
	if (seqs->nelt >= 1) {
		*(start_elt++) = 1;
		*(width_elt++) = seq->length;
	}
	if (seqs->nelt >= 2)
		for (i = 1, start_prev_elt = INTEGER(start); i < seqs->nelt; i++) {
			*(start_elt++) = *(start_prev_elt++) + (seq++)->length;
			*(width_elt++) = seq->length;
		}
	PROTECT(ans = new_IRanges(classname, start, width, R_NilValue));
#ifdef DEBUG_BIOSTRINGS
	if (debug) {
		Rprintf("[DEBUG] _new_IRanges_from_RoSeqs(): END\n");
	}
#endif
	UNPROTECT(3);
	return ans;
}


/*****************************************************************************
 * Getting the order of a RoSeqs struct.
 *
 * The implementation below tries to optimize performance and memory footprint
 * by using a zero-copy approach. This is achieved at the (modest) cost of
 * using the 'base_seq' static variable. Would that be a problem in a
 * multithreading context?
 */

static int cmp_cachedCharSeq(const void *p1, const void *p2)
{
	const cachedCharSeq *seq1, *seq2;
	int min_nelt, ret;

	seq1 = (const cachedCharSeq *) p1;
	seq2 = (const cachedCharSeq *) p2;
	min_nelt = seq1->length <= seq2->length ? seq1->length : seq2->length;
	ret = memcmp(seq1->seq, seq2->seq, min_nelt);
	return ret != 0 ? ret : seq1->length - seq2->length;
}

static const cachedCharSeq *base_seq;
static const int *base_order;

static int cmp_cachedCharSeq_indices_for_ordering(const void *p1, const void *p2)
{
	int i1, i2, ret;

	i1 = *((const int *) p1);
	i2 = *((const int *) p2);
	ret = cmp_cachedCharSeq(base_seq + i1, base_seq + i2);
	// How qsort() will break ties is undefined (the Quicksort algo is
	// not "stable"). By returning i1 - i2 in case of a tie, we ensure that
	// the _get_RoSeqs_order() function below is "stable" and returns
	// the same thing on all platforms. In addition, this coincides with
	// what the order() function does in R.
	return ret != 0 ? ret : i1 - i2;
}

static int cmp_cachedCharSeq_indices_for_matching(const void *key, const void *p)
{
	return cmp_cachedCharSeq(key, base_seq + base_order[*((const int *) p)]);
}

int _get_RoSeqs_is_unsorted(const RoSeqs *seqs, int strictly)
{
	int i1, i2, cmp, ans;

	ans = 0;
	if (strictly) {
		for (i1 = 0, i2 = 1; i2 < seqs->nelt; i1++, i2++) {
			cmp = cmp_cachedCharSeq(seqs->elts + i1, seqs->elts + i2);
			if (cmp >= 0) {
				ans = 1;
				break;
			}
		}
	} else {
		for (i1 = 0, i2 = 1; i2 < seqs->nelt; i1++, i2++) {
			cmp = cmp_cachedCharSeq(seqs->elts + i1, seqs->elts + i2);
			if (cmp > 0) {
				ans = 1;
				break;
			}
		}
	}
	return ans;
}

void _get_RoSeqs_order(const RoSeqs *seqs, int *order, int base1)
{
	int i, *ord;

	if (base1 == 0) {
		base_seq = seqs->elts;
		for (i = 0, ord = order; i < seqs->nelt; i++, ord++)
			*ord = i;
	} else {
		base_seq = seqs->elts - 1; // because we will sort 1-based indices
		for (i = 0, ord = order; i < seqs->nelt; i++, ord++)
			*ord = i + 1; // 1-based indices
	}
	if (_get_RoSeqs_is_unsorted(seqs, 0))
		qsort(order, seqs->nelt, sizeof(int), cmp_cachedCharSeq_indices_for_ordering);
	return;
}

void _get_RoSeqs_rank(const RoSeqs *seqs, const int *order, int *rank)
{
	int i, *ord1, *ord2;

	if (seqs->nelt == 0)
		return;
	rank[*order] = 1;
	for (i = 2, ord1 = (int *) order, ord2 = (int *) (order+1); i <= seqs->nelt;
	     i++, ord1++, ord2++) {
		if (cmp_cachedCharSeq(seqs->elts + *ord1, seqs->elts + *ord2) == 0) {
			rank[*ord2] = rank[*ord1];
		} else {
			rank[*ord2] = i;
		}
	}
	return;
}


/*****************************************************************************
 * Getting duplicated information for a RoSeqs struct.
 */

void _get_RoSeqs_duplicated(const RoSeqs *seqs, const int *order, int *duplicated)
{
	int i, *ord1, *ord2;

	if (seqs->nelt == 0)
		return;
	duplicated[*order] = 0;
	for (i = 2, ord1 = (int *) order, ord2 = (int *) (order+1); i <= seqs->nelt;
	     i++, ord1++, ord2++)
		duplicated[*ord2] =
			cmp_cachedCharSeq(seqs->elts + *ord1, seqs->elts + *ord2) == 0;
	return;
}


/*****************************************************************************
 * Getting identical matching information for a RoSeqs struct.
 */

void _get_RoSeqs_match(const RoSeqs *seqs, const RoSeqs *set, int nomatch,
		               const int *seqs_order, const int *set_order,
		               int *match_buffer, int *match_pos)
{
	int i, n, *curr_found, *prev_found;
	void *curr_seq;

	base_seq = set->elts;
	base_order = set_order;

	n = set->nelt;
	curr_found = match_buffer;
	for (i = 0; i < n; i++)
		curr_found[i] = i;

	prev_found = curr_found;
	for (i = 0; i < seqs->nelt; i++) {
		curr_seq = (void *) (seqs->elts + seqs_order[i]);
		curr_found =
			(int *) bsearch(curr_seq, curr_found, n, sizeof(int),
					        cmp_cachedCharSeq_indices_for_matching);
		if (curr_found == NULL) {
			match_pos[seqs_order[i]] = nomatch;
			curr_found = prev_found;
		} else {
			while ((*curr_found > 0) &&
				   (cmp_cachedCharSeq(curr_seq,
						      set->elts + set_order[*curr_found - 1]) == 0))
				curr_found--;
			match_pos[seqs_order[i]] = set_order[*curr_found] + 1;
			n -= *curr_found - *prev_found;
			prev_found = curr_found;
		}
	}
	return;
}