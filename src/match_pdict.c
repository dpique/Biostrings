/****************************************************************************
 *                                                                          *
 *        Inexact matching of a DNA dictionary using a Trusted Band         *
 *                           Author: Herve Pages                            *
 *                                                                          *
 ****************************************************************************/
#include "Biostrings.h"
#include "IRanges_interface.h"

static int debug = 0;

SEXP debug_match_pdict()
{
#ifdef DEBUG_BIOSTRINGS
	debug = !debug;
	Rprintf("Debug mode turned %s in 'match_pdict.c'\n",
		debug ? "on" : "off");
#else
	Rprintf("Debug mode not available in 'match_pdict.c'\n");
#endif
	return R_NilValue;
}

// FIXME: Pass 'const HeadTail *' instead of 'pdict_head'
// and 'pdict_tail'. Also change MatchPDictBuf struct so the
// head_widths and tail_widths members are IntAE buffers.
static MatchPDictBuf new_MatchPDictBuf_from_TB_PDict(SEXP matches_as,
		SEXP pptb, SEXP pdict_head, SEXP pdict_tail)
{
	int tb_length, tb_width;
	const int *head_widths, *tail_widths;

	tb_length = _get_PreprocessedTB_length(pptb);
	tb_width = _get_PreprocessedTB_width(pptb);
	if (pdict_head == R_NilValue)
		head_widths = NULL;
	else
		head_widths = INTEGER(_get_XStringSet_width(pdict_head));
	if (pdict_tail == R_NilValue)
		tail_widths = NULL;
	else
		tail_widths = INTEGER(_get_XStringSet_width(pdict_tail));
	return _new_MatchPDictBuf(matches_as, tb_length, tb_width,
				head_widths, tail_widths);
}

static void match_pdict(SEXP pptb, HeadTail *headtail, const cachedCharSeq *S,
		SEXP max_mismatch, SEXP min_mismatch, SEXP fixed,
		MatchPDictBuf *matchpdict_buf)
{
	int max_nmis, min_nmis, fixedP, fixedS;
	SEXP low2high;
	const char *type;
	TBMatchBuf *tb_matches;

	max_nmis = INTEGER(max_mismatch)[0];
	min_nmis = INTEGER(min_mismatch)[0];
	fixedP = LOGICAL(fixed)[0];
	fixedS = LOGICAL(fixed)[1];
	_select_nmismatch_at_Pshift_fun(fixedP, fixedS);
	type = get_classname(pptb);
/*
	if (strcmp(type, "ACtree2") == 0) {
		_match_pdictACtree2(pptb, headtail, S,
			max_nmis, min_nmis, fixedP, fixedS,
			matchpdict_buf);
		return;
	}
*/
#ifdef DEBUG_BIOSTRINGS
	if (debug)
		Rprintf("[DEBUG] ENTERING match_pdict()\n");
#endif
	low2high = _get_PreprocessedTB_low2high(pptb);
	tb_matches = &(matchpdict_buf->tb_matches);

	if (strcmp(type, "Twobit") == 0)
		_match_Twobit(pptb, S, fixedS, tb_matches);
	else if (strcmp(type, "ACtree") == 0)
		_match_ACtree(pptb, S, fixedS, tb_matches);
	else if (strcmp(type, "ACtree2") == 0)
		_match_tbACtree2(pptb, S, fixedS, tb_matches);
	else
		error("%s: unsupported Trusted Band type in 'pdict'", type);
	/* Call _match_pdict_all_flanks() even if 'headtail' is empty
	 * (i.e. headtail->max_HTwidth == 0) because we need to propagate
	 * the matches to the duplicates anyway */
	_match_pdict_all_flanks(low2high, headtail,
		S, max_nmis, min_nmis, matchpdict_buf);
#ifdef DEBUG_BIOSTRINGS
	if (debug)
		Rprintf("[DEBUG] LEAVING match_pdict()\n");
#endif
	return;
}


/****************************************************************************
 * .Call entry points: XString_match_pdict()
 *                     XString_match_XStringSet()
 *                     XStringViews_match_pdict()
 *                     XStringViews_match_XStringSet()
 *
 * Arguments:
 *   pptb: a PreprocessedTB object;
 *   pdict_head: head(pdict) (XStringSet or NULL);
 *   pdict_tail: tail(pdict) (XStringSet or NULL);
 *   subject: subject;
 *   max_mismatch: max.mismatch (max nb of mismatches out of the TB);
 *   min_mismatch: min.mismatch (min nb of mismatches out of the TB);
 *   fixed: logical vector of length 2;
 *   matches_as: "DEVNULL", "MATCHES_AS_WHICH", "MATCHES_AS_COUNTS"
 *               or "MATCHES_AS_ENDS";
 *   envir: NULL or environment to be populated with the matches.
 *
 ****************************************************************************/

SEXP XString_match_pdict(SEXP pptb, SEXP pdict_head, SEXP pdict_tail,
		SEXP subject,
		SEXP max_mismatch, SEXP min_mismatch, SEXP fixed,
		SEXP matches_as, SEXP envir)
{
	HeadTail headtail;
	cachedCharSeq S;
	MatchPDictBuf matchpdict_buf;

#ifdef DEBUG_BIOSTRINGS
	if (debug)
		Rprintf("[DEBUG] ENTERING XString_match_pdict()\n");
#endif
	headtail = _new_HeadTail(pdict_head, pdict_tail, pptb,
				 max_mismatch, fixed, 1);
	S = cache_XRaw(subject);

	matchpdict_buf = new_MatchPDictBuf_from_TB_PDict(matches_as,
				pptb, pdict_head, pdict_tail);
	match_pdict(pptb, &headtail,
		&S, max_mismatch, min_mismatch, fixed,
		&matchpdict_buf);
#ifdef DEBUG_BIOSTRINGS
	if (debug)
		Rprintf("[DEBUG] LEAVING XString_match_pdict()\n");
#endif
	return _Seq2MatchBuf_as_SEXP(matchpdict_buf.matches_as,
				&(matchpdict_buf.matches), envir);
}

SEXP XString_match_XStringSet(SEXP pattern,
		SEXP subject,
		SEXP max_mismatch, SEXP min_mismatch, SEXP fixed,
		SEXP matches_as, SEXP envir)
{
	error("XString_match_XStringSet(): IMPLEMENT ME!");
	return R_NilValue;
}

SEXP XStringViews_match_pdict(SEXP pptb, SEXP pdict_head, SEXP pdict_tail,
		SEXP subject, SEXP views_start, SEXP views_width,
		SEXP max_mismatch, SEXP min_mismatch, SEXP fixed,
		SEXP matches_as, SEXP envir)
{
	HeadTail headtail;
	int tb_length;
	cachedCharSeq S, S_view;
	int nviews, i, *view_start, *view_width, view_offset;
	MatchPDictBuf matchpdict_buf;
	Seq2MatchBuf global_matchpdict_buf;

#ifdef DEBUG_BIOSTRINGS
	if (debug)
		Rprintf("[DEBUG] ENTERING XStringViews_match_pdict()\n");
#endif
	tb_length = _get_PreprocessedTB_length(pptb);
	headtail = _new_HeadTail(pdict_head, pdict_tail, pptb,
				 max_mismatch, fixed, 1);
	S = cache_XRaw(subject);

	matchpdict_buf = new_MatchPDictBuf_from_TB_PDict(matches_as,
				pptb, pdict_head, pdict_tail);
	global_matchpdict_buf = _new_Seq2MatchBuf(matches_as, tb_length);
	nviews = LENGTH(views_start);
	for (i = 0, view_start = INTEGER(views_start), view_width = INTEGER(views_width);
	     i < nviews;
	     i++, view_start++, view_width++)
	{
		view_offset = *view_start - 1;
		if (view_offset < 0 || view_offset + *view_width > S.length)
			error("'subject' has \"out of limits\" views");
		S_view.seq = S.seq + view_offset;
		S_view.length = *view_width;
		match_pdict(pptb, &headtail, &S_view,
			    max_mismatch, min_mismatch, fixed,
			    &matchpdict_buf);
		_MatchPDictBuf_append_and_flush(&global_matchpdict_buf,
			&matchpdict_buf, view_offset);
	}

#ifdef DEBUG_BIOSTRINGS
	if (debug)
		Rprintf("[DEBUG] LEAVING XStringViews_match_pdict()\n");
#endif
	return _Seq2MatchBuf_as_SEXP(matchpdict_buf.matches_as,
				&global_matchpdict_buf, envir);
}

SEXP XStringViews_match_XStringSet(SEXP pattern,
		SEXP subject, SEXP views_start, SEXP views_width,
		SEXP max_mismatch, SEXP min_mismatch, SEXP fixed,
		SEXP matches_as, SEXP envir)
{
	error("XStringViews_match_XStringSet(): IMPLEMENT ME!");
	return R_NilValue;
}


/****************************************************************************
 * .Call entry points: XStringSet_vmatch_pdict()
 *                     XStringSet_vmatch_XStringSet()
 ****************************************************************************/

static SEXP vwhich_pdict(SEXP pptb, HeadTail *headtail,
		SEXP subject,
		SEXP max_mismatch, SEXP min_mismatch, SEXP fixed,
		MatchPDictBuf *matchpdict_buf)
{
	int S_length, j;
	cachedXStringSet S;
	SEXP ans;
	cachedCharSeq S_elt;

	S = _cache_XStringSet(subject);
	S_length = _get_XStringSet_length(subject);
	PROTECT(ans = NEW_LIST(S_length));
	for (j = 0; j < S_length; j++) {
		S_elt = _get_cachedXStringSet_elt(&S, j);
		match_pdict(pptb, headtail, &S_elt,
			    max_mismatch, min_mismatch, fixed,
			    matchpdict_buf);
		SET_ELEMENT(ans, j, _Seq2MatchBuf_which_asINTEGER(&(matchpdict_buf->matches)));
		_MatchPDictBuf_flush(matchpdict_buf);
	}
	UNPROTECT(1);
	return ans;
}

static SEXP vcount_pdict_notcollapsed(SEXP pptb, HeadTail *headtail,
		SEXP subject,
		SEXP max_mismatch, SEXP min_mismatch, SEXP fixed,
		MatchPDictBuf *matchpdict_buf)
{
	int tb_length, S_length, j, *current_col;
	cachedXStringSet S;
	SEXP ans;
	cachedCharSeq S_elt;
	const IntAE *count_buf;

	tb_length = _get_PreprocessedTB_length(pptb);
	S = _cache_XStringSet(subject);
	S_length = _get_XStringSet_length(subject);
	PROTECT(ans = allocMatrix(INTSXP, tb_length, S_length));
	for (j = 0, current_col = INTEGER(ans);
	     j < S_length;
	     j++, current_col += tb_length)
	{
		S_elt = _get_cachedXStringSet_elt(&S, j);
		match_pdict(pptb, headtail, &S_elt,
			max_mismatch, min_mismatch, fixed,
			matchpdict_buf);
		count_buf = &(matchpdict_buf->matches.match_counts);
		/* count_buf->nelt is tb_length */
		memcpy(current_col, count_buf->elts, sizeof(int) * count_buf->nelt);
		_MatchPDictBuf_flush(matchpdict_buf);
	}
	UNPROTECT(1);
	return ans;
}

static SEXP vcount_pdict_collapsed(SEXP pptb, HeadTail *headtail,
		SEXP subject,
		SEXP max_mismatch, SEXP min_mismatch, SEXP fixed,
		int collapse, SEXP weight,
		MatchPDictBuf *matchpdict_buf)
{
	int tb_length, S_length, ans_length, i, j;
	cachedXStringSet S;
	SEXP ans;
	cachedCharSeq S_elt;
	const IntAE *count_buf;

	tb_length = _get_PreprocessedTB_length(pptb);
	S = _cache_XStringSet(subject);
	S_length = _get_XStringSet_length(subject);
	switch (collapse) {
	    case 1: ans_length = tb_length; break;
	    case 2: ans_length = S_length; break;
	    default: error("'collapse' must be FALSE, 1 or 2");
	}
	if (IS_INTEGER(weight)) {
		PROTECT(ans = NEW_INTEGER(ans_length));
		memset(INTEGER(ans), 0, ans_length * sizeof(int));
	} else {
		PROTECT(ans = NEW_NUMERIC(ans_length));
		for (i = 0; i < ans_length; i++)
			REAL(ans)[i] = 0.00;
	}
	for (j = 0; j < S_length; j++)
	{
		S_elt = _get_cachedXStringSet_elt(&S, j);
		match_pdict(pptb, headtail, &S_elt,
			    max_mismatch, min_mismatch, fixed,
			    matchpdict_buf);
		count_buf = &(matchpdict_buf->matches.match_counts);
		/* count_buf->nelt is tb_length */
		for (i = 0; i < tb_length; i++)
			if (collapse == 1) {
				if (IS_INTEGER(weight))
					INTEGER(ans)[i] += count_buf->elts[i] * INTEGER(weight)[j];
				else
					REAL(ans)[i] += count_buf->elts[i] * REAL(weight)[j];
			} else {
				if (IS_INTEGER(weight))
					INTEGER(ans)[j] += count_buf->elts[i] * INTEGER(weight)[i];
				else
					REAL(ans)[j] += count_buf->elts[i] * REAL(weight)[i];
			}
		_MatchPDictBuf_flush(matchpdict_buf);
	}
	UNPROTECT(1);
	return ans;
}

SEXP XStringSet_vmatch_pdict(SEXP pptb, SEXP pdict_head, SEXP pdict_tail,
		SEXP subject,
		SEXP max_mismatch, SEXP min_mismatch, SEXP fixed,
		SEXP collapse, SEXP weight,
		SEXP matches_as, SEXP envir)
{
	HeadTail headtail;
	int collapse0;
	MatchPDictBuf matchpdict_buf;
	SEXP ans;

#ifdef DEBUG_BIOSTRINGS
	if (debug)
		Rprintf("[DEBUG] ENTERING XStringSet_vmatch_pdict()\n");
#endif
	headtail = _new_HeadTail(pdict_head, pdict_tail, pptb,
				 max_mismatch, fixed, 1);

	matchpdict_buf = new_MatchPDictBuf_from_TB_PDict(matches_as,
				pptb, pdict_head, pdict_tail);
	switch (matchpdict_buf.matches_as) {
	    case MATCHES_AS_NULL:
		error("XStringSet_vmatch_pdict() does not support "
		      "'matches_as=\"%s\"' yet, sorry",
		      matchpdict_buf.matches_as);
	    break;
	    case MATCHES_AS_WHICH:
		PROTECT(ans = vwhich_pdict(pptb, &headtail,
				subject,
				max_mismatch, min_mismatch, fixed,
				&matchpdict_buf));
	    break;
	    case MATCHES_AS_COUNTS:
		collapse0 = INTEGER(collapse)[0];
		if (collapse0 == 0)
			PROTECT(ans = vcount_pdict_notcollapsed(pptb, &headtail,
					subject,
					max_mismatch, min_mismatch, fixed,
					&matchpdict_buf));
		else
			PROTECT(ans = vcount_pdict_collapsed(pptb, &headtail,
					subject,
					max_mismatch, min_mismatch, fixed,
					collapse0, weight,
					&matchpdict_buf));
	    break;
	    case MATCHES_AS_ENDS:
		error("vmatchPDict() is not supported yet, sorry");
	    break;
	}
#ifdef DEBUG_BIOSTRINGS
	if (debug)
		Rprintf("[DEBUG] LEAVING XStringSet_vmatch_pdict()\n");
#endif
	UNPROTECT(1);
	return ans;
}

SEXP XStringSet_vmatch_XStringSet(SEXP pattern,
		SEXP subject,
		SEXP max_mismatch, SEXP min_mismatch, SEXP fixed,
		SEXP collapse, SEXP weight,
		SEXP matches_as, SEXP envir)
{
	error("XStringSet_vmatch_XStringSet(): IMPLEMENT ME!");
	return R_NilValue;
}

