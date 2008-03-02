#include "Biostrings.h"

static const R_CallMethodDef callMethods[] = {

/* utils.c */
	{"Biostrings_debug_utils", (DL_FUNC) &Biostrings_debug_utils, 0},

/* bufutils.c */
	{"Biostrings_debug_bufutils", (DL_FUNC) &Biostrings_debug_bufutils, 0},

/* views_buffer.c */
	{"Biostrings_debug_views_buffer", (DL_FUNC) &Biostrings_debug_views_buffer, 0},

/* IntIntervals.c */
	{"Biostrings_debug_IntIntervals", (DL_FUNC) &Biostrings_debug_IntIntervals, 0},

	{"narrow_IntIntervals", (DL_FUNC) &narrow_IntIntervals, 4},
	{"int_to_adjacent_intervals", (DL_FUNC) &int_to_adjacent_intervals, 1},
	{"reduce_IntIntervals", (DL_FUNC) &reduce_IntIntervals, 2},

/* XRaw.c */
	{"Biostrings_debug_XRaw", (DL_FUNC) &Biostrings_debug_XRaw, 0},

	{"Biostrings_sexp_address", (DL_FUNC) &Biostrings_sexp_address, 1},
	{"Biostrings_xp_show", (DL_FUNC) &Biostrings_xp_show, 1},
	{"Biostrings_xp_new", (DL_FUNC) &Biostrings_xp_new, 0},
	{"Biostrings_safe_explode", (DL_FUNC) &Biostrings_safe_explode, 1},

	{"Biostrings_XRaw_alloc", (DL_FUNC) &Biostrings_XRaw_alloc, 2},
	{"Biostrings_XRaw_get_show_string", (DL_FUNC) &Biostrings_XRaw_get_show_string, 1},
	{"Biostrings_XRaw_length", (DL_FUNC) &Biostrings_XRaw_length, 1},
	{"Biostrings_XRaw_memcmp", (DL_FUNC) &Biostrings_XRaw_memcmp, 5},

	{"Biostrings_XRaw_copy_from_i1i2", (DL_FUNC) &Biostrings_XRaw_copy_from_i1i2, 4},
	{"Biostrings_XRaw_copy_from_subset", (DL_FUNC) &Biostrings_XRaw_copy_from_subset, 3},

	{"Biostrings_XRaw_read_chars_from_i1i2", (DL_FUNC) &Biostrings_XRaw_read_chars_from_i1i2, 3},
	{"Biostrings_XRaw_read_chars_from_subset", (DL_FUNC) &Biostrings_XRaw_read_chars_from_subset, 2},
	{"Biostrings_XRaw_write_chars_to_i1i2", (DL_FUNC) &Biostrings_XRaw_write_chars_to_i1i2, 4},
	{"Biostrings_XRaw_write_chars_to_subset", (DL_FUNC) &Biostrings_XRaw_write_chars_to_subset, 3},

	{"XRaw_read_ints_from_i1i2", (DL_FUNC) &XRaw_read_ints_from_i1i2, 3},
	{"XRaw_read_ints_from_subset", (DL_FUNC) &XRaw_read_ints_from_subset, 2},
	{"XRaw_write_ints_to_i1i2", (DL_FUNC) &XRaw_write_ints_to_i1i2, 4},
	{"XRaw_write_ints_to_subset", (DL_FUNC) &XRaw_write_ints_to_subset, 3},

	{"XRaw_read_enc_chars_from_i1i2", (DL_FUNC) &XRaw_read_enc_chars_from_i1i2, 4},
	{"XRaw_read_enc_chars_from_subset", (DL_FUNC) &XRaw_read_enc_chars_from_subset, 3},
	{"XRaw_write_enc_chars_to_i1i2", (DL_FUNC) &XRaw_write_enc_chars_to_i1i2, 5},
	{"XRaw_write_enc_chars_to_subset", (DL_FUNC) &XRaw_write_enc_chars_to_subset, 4},

	{"XRaw_read_complexes_from_i1i2", (DL_FUNC) &XRaw_read_complexes_from_i1i2, 4},
	{"XRaw_read_complexes_from_subset", (DL_FUNC) &XRaw_read_complexes_from_subset, 3},

	{"XRaw_loadFASTA", (DL_FUNC) &XRaw_loadFASTA, 4},

/* XInteger.c */
	{"Biostrings_debug_XInteger", (DL_FUNC) &Biostrings_debug_XInteger, 0},

	{"XInteger_alloc", (DL_FUNC) &XInteger_alloc, 2},
	{"XInteger_get_show_string", (DL_FUNC) &XInteger_get_show_string, 1},
	{"XInteger_length", (DL_FUNC) &XInteger_length, 1},
	{"XInteger_memcmp", (DL_FUNC) &XInteger_memcmp, 5},

	{"XInteger_read_ints_from_i1i2", (DL_FUNC) &XInteger_read_ints_from_i1i2, 3},
	{"XInteger_read_ints_from_subset", (DL_FUNC) &XInteger_read_ints_from_subset, 2},
	{"XInteger_write_ints_to_i1i2", (DL_FUNC) &XInteger_write_ints_to_i1i2, 4},
	{"XInteger_write_ints_to_subset", (DL_FUNC) &XInteger_write_ints_to_subset, 3},

/* BString_utils.c */
	{"init_DNAlkups", (DL_FUNC) &init_DNAlkups, 2},
	{"init_RNAlkups", (DL_FUNC) &init_RNAlkups, 2},
	{"BStrings_to_nchars", (DL_FUNC) &BStrings_to_nchars, 1},

/* seqs_to_seqs.c */
	{"Biostrings_debug_seqs_to_seqs", (DL_FUNC) &Biostrings_debug_seqs_to_seqs, 0},

	{"copy_subXRaw", (DL_FUNC) &copy_subXRaw, 4},
	{"STRSXP_to_XRaw", (DL_FUNC) &STRSXP_to_XRaw, 5},
	{"BString_to_XRaw", (DL_FUNC) &BString_to_XRaw, 4},
	{"XRaw_to_BStringList", (DL_FUNC) &XRaw_to_BStringList, 4},
	{"narrow_BStringList", (DL_FUNC) &narrow_BStringList, 4},
        
/* reverseComplement.c */
        {"XRaw_translate_copy_from_i1i2", (DL_FUNC) &XRaw_translate_copy_from_i1i2, 5},
        {"XRaw_translate_copy_from_subset", (DL_FUNC) &XRaw_translate_copy_from_subset, 4},
        {"XRaw_reverse_copy_from_i1i2", (DL_FUNC) &XRaw_reverse_copy_from_i1i2, 4},
        {"XRaw_reverse_translate_copy_from_i1i2", (DL_FUNC) &XRaw_reverse_translate_copy_from_i1i2, 5},

/* char_frequency.c */
	{"char_frequency", (DL_FUNC) &char_frequency, 3},
	{"oligonucleotide_frequency", (DL_FUNC) &oligonucleotide_frequency, 6},

/* normalize_views.c */
	{"Biostrings_normalize_views", (DL_FUNC) &Biostrings_normalize_views, 2},

/* match_naive.c */
	{"match_naive_debug", (DL_FUNC) &match_naive_debug, 0},
	{"is_matching", (DL_FUNC) &is_matching, 5},
	{"match_naive_exact", (DL_FUNC) &match_naive_exact, 3},
	{"match_naive_inexact", (DL_FUNC) &match_naive_inexact, 5},

/* match_boyermoore.c */
	{"match_boyermoore_debug", (DL_FUNC) &match_boyermoore_debug, 0},
	{"match_boyermoore", (DL_FUNC) &match_boyermoore, 7},

/* match_shiftor.c */
	{"match_shiftor_debug", (DL_FUNC) &match_shiftor_debug, 0},
	{"bits_per_long", (DL_FUNC) &bits_per_long, 0},
	{"match_shiftor", (DL_FUNC) &match_shiftor, 9},

/* find_palindromes.c */
	{"find_palindromes_debug", (DL_FUNC) &find_palindromes_debug, 0},
	{"find_palindromes", (DL_FUNC) &find_palindromes, 6},

/* match_BOC.c */
	{"match_BOC_debug", (DL_FUNC) &match_BOC_debug, 0},
	{"match_BOC_preprocess", (DL_FUNC) &match_BOC_preprocess, 12},
	{"match_BOC_exact", (DL_FUNC) &match_BOC_exact, 16},

/* match_BOC2.c */
	{"match_BOC2_debug", (DL_FUNC) &match_BOC2_debug, 0},
	{"match_BOC2_preprocess", (DL_FUNC) &match_BOC2_preprocess, 9},
	{"match_BOC2_exact", (DL_FUNC) &match_BOC2_exact, 13},

/* match_TBdna.c */
	{"match_TBdna_debug", (DL_FUNC) &match_TBdna_debug, 0},
	{"CWdna_free_actree_nodes_buf", (DL_FUNC) &CWdna_free_actree_nodes_buf, 0},
	{"CWdna_pp_charseqs", (DL_FUNC) &CWdna_pp_charseqs, 3},
	{"CWdna_pp_BStringSet", (DL_FUNC) &CWdna_pp_BStringSet, 3},
	{"match_TBdna", (DL_FUNC) &match_TBdna, 10},
	{"shiftListOfInts", (DL_FUNC) &shiftListOfInts, 2},
	{"extract_endIndex", (DL_FUNC) &extract_endIndex, 4},

/* pmatchPattern.c */
	{"lcprefix", (DL_FUNC) &lcprefix, 6},
	{"lcsuffix", (DL_FUNC) &lcsuffix, 6},

/* align_needwunsQS.c */
	{"align_needwunsQS", (DL_FUNC) &align_needwunsQS, 11},

	{NULL, NULL, 0}
};

void R_init_Biostrings(DllInfo *info)
{
	/* Lots of code around assumes that sizeof(Rbyte) == sizeof(char) */
	if (sizeof(Rbyte) != sizeof(char))
		error("sizeof(Rbyte) != sizeof(char)");
	R_registerRoutines(info, NULL, callMethods, NULL, NULL);
	R_RegisterCCallable("Biostrings", "_DNAencode", (DL_FUNC) &_DNAencode);
	R_RegisterCCallable("Biostrings", "_DNAdecode", (DL_FUNC) &_DNAdecode);
	R_RegisterCCallable("Biostrings", "_get_BString_charseq", (DL_FUNC) &_get_BString_charseq);
	R_RegisterCCallable("Biostrings", "_init_match_reporting", (DL_FUNC) &_init_match_reporting);
	R_RegisterCCallable("Biostrings", "_report_match", (DL_FUNC) &_report_match);
	R_RegisterCCallable("Biostrings", "_reported_matches_asSEXP", (DL_FUNC) &_reported_matches_asSEXP);
}

