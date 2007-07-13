/****************************************************************************
                      A BOYER-MOORE-LIKE MATCHING ALGO
		            Author: Herve Pages
 ****************************************************************************/
#include "Biostrings.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* I've turned off the "MWshift feature". Doesn't seem to give very good
 * results :-( To turned it on, set MWSHIFT_NPMAX to a positive integer
 * (typically 128, 256 or 512, doesn't have to be a power of 2).
 * The original idea of this feature was to try to boost the boyermoore()
 * function when the pattern is small.
 */
#define MWSHIFT_NPMAX	0


/****************************************************************************/
static int debug = 0;

SEXP match_boyermoore_debug()
{
#ifdef DEBUG_BIOSTRINGS
	debug = !debug;
	Rprintf("Debug mode turned %s in 'match_boyermoore.c'\n", debug ? "on" : "off");
#else
	Rprintf("Debug mode not available in 'match_boyermoore.c'\n");
#endif
	return R_NilValue;
}


/****************************************************************************
 * P0buffer holds a copy of the current pattern P0.
 * We must always have P0buffer_nP <= P0buffer_length
 */
static char *P0buffer = NULL;
static int P0buffer_length = 0, P0buffer_nP = 0;

/* Status meaning:
 *     -1: init_P0buffer() has changed the value of P0buffer_length.
 *   >= 0: init_P0buffer() didn't change the value of P0buffer_length.
 *         The non-negative integer is the length of the longest common
 *         suffix between old and new P0 (<= min(nP,P0buffer_nP)).
 */
static int P0buffer_init_status;

static void init_P0buffer(const char *P, int nP)
{
	int min_nP_nP0, j;

	if (nP == 0) { /* should never happen but safer anyway... */
		P0buffer_init_status = 0;
		return;
	}
	if (nP > 20000)
		error("pattern is too long");
	if (nP > P0buffer_length) {
		/* We need more memory */
		if (P0buffer != NULL)
			free(P0buffer);
		P0buffer_length = 0;
		P0buffer = (char *) malloc(nP * sizeof(char));
		if (P0buffer == NULL)
			error("can't allocate memory for P0buffer");
		P0buffer_length = nP;
		P0buffer_init_status = -1;
	} else {
		/* We have enough memory */
		if (nP < P0buffer_nP) min_nP_nP0 = nP; else min_nP_nP0 = P0buffer_nP;
		for (j = 0; j < min_nP_nP0; j++)
			if (P[j] != P0buffer[j])
				break;
		P0buffer_init_status = j;
	}
	memcpy(P0buffer, P, nP * sizeof(char));
	P0buffer_nP = nP;
	return;
}

/****************************************************************************
 * j0/shift0
 * =========
 * 
 * For any j1 and j2 such that 0 <= j1 < j2 <= nP (nP being the length of
 * the pattern P), we note P(j1, j2) the subpattern of P starting at
 * position j1 and ending at position j2-1.
 * Note that with this definition: P(0, nP) = P.
 *
 * Definition of j0 and shift0
 * ---------------------------
 *   j0 = start position in P of the smallest suffix of P that is not a
 *        substring of P(1, nP-1)
 * Note that j0 can also be defined by using the MWshift function (see
 * section "The Matching Window shifts" below):
 *   j0 = max{j1 | MWshift(j1, nP) >= j}
 *
 *  shift0 =  MWshift(j0, nP)
 *
 * Properties
 * ----------
 *
 *   (a) If j1 <= j0, then MWshift(j1, nP) = shift0 >= j0
 *   (b) If j1 > j0, then MWshift(j1, nP) < j1
 *   (c) MWshift(j1, j2) <= shift0
 *   (d) If j < j0, VSGSshift(c, j) = shift0
 *   (e) VSGSshift(P[0], 0) = shift0
 */

static int P0buffer_j0, P0buffer_shift0;

static void init_j0shift0()
{
	int j0, shift0, length, j;

	length = 1;
	j0 = P0buffer_nP - 1;
	for (j = j0 - 1; j >= 1; j--) {
		if (memcmp(P0buffer + j, P0buffer + j0, length) == 0) {
			length++;
			j0--;
		}
	}
	for (shift0 = j0 - j; shift0 < P0buffer_nP; shift0++, length--) {
		if (memcmp(P0buffer, P0buffer + shift0, length) == 0)
			break;
	}
	P0buffer_j0 = j0;
	P0buffer_shift0 = shift0;
	/*Rprintf("j0=%d shift0=%d\n", j0, shift0);*/
}


/****************************************************************************
 * The Very Strong Good Suffix shifts
 * ==================================
 */

/* Contains the "Very Strong Good Suffix shifts" for current pattern P0. */
static int *VSGSshift_table = NULL;

/* VSGSshift_table is a 256 x P0buffer_length matrix.
 * Its layout is (only the values marked with an "x" will be potentially
 * used):
 *
 *           0 1 2 3 4 5 j
 *         0 x x x x - -
 *         1 x x x x - - 
 *         2 x x x x - -    P0buffer_nP = 4 <= P0buffer_length = 6
 *         .............
 *       256 x x x x - -
 *         c
 *
 * The "x" region is defined by 0 <= j < P0buffer_nP
 */

#define VSGS_SHIFT(c, j) (VSGSshift_table[P0buffer_length * ((unsigned char) c) + j])

static int get_VSGSshift(char c, int j)
{
	int shift, k, k1, k2, length;

	if (j < P0buffer_j0)
		return P0buffer_shift0;
	shift = VSGS_SHIFT(c, j);
	if (shift != 0)
		return shift;
	for (shift = 1; shift < P0buffer_nP; shift++) {
		if (shift <= j) {
			k = j - shift;
			if (P0buffer[k] != c)
				continue;
			k1 = k + 1;
		} else {
			k1 = 0;
		}
		k2 = P0buffer_nP - shift;
		if (k1 == k2)
			break;
		length = k2 - k1;
		if (memcmp(P0buffer + k1, P0buffer + k1 + shift, length) == 0)
			break;
	}
	/* shift is P0buffer_nP when the "for" loop is not interrupted by "break" */
	/*Rprintf("VSGSshift(c=%c, j=%d) = %d\n", c, j, shift);*/
	return VSGS_SHIFT(c, j) = shift;
}

static void init_VSGSshift_table()
{
	int u, j;
	char c;

	if (P0buffer_init_status == -1 && VSGSshift_table != NULL) {
		free(VSGSshift_table);
		VSGSshift_table = NULL;
	}
	if (P0buffer_length != 0 && VSGSshift_table == NULL) {
		VSGSshift_table = (int *) malloc(256 * P0buffer_length *
						sizeof(int));
		if (VSGSshift_table == NULL)
			error("can't allocate memory for VSGSshift_table");
	}
	for (u = 0; u < 256; u++) {
		for (j = 0; j < P0buffer_nP; j++) {
			c = (char) u;
			VSGS_SHIFT(c, j) = 0;
		}
	}
}


/****************************************************************************
 * The Matching Window shifts
 * ==========================
 *
 * Definition
 * ----------
 *
 * MWshift(j1, j2), the "Matching Window shift" for P(j1, j2), is defined by:
 *   Imagine that, for a given alignement of P with the subject S, all
 *   letters in subpattern P(j1, j2) are matching S (we say that the current
 *   Matching Window is (j1, j2)). Then MWshift(j1, j2) is the smallest
 *   (non-zero) amount of letters that we can shift P to the right without
 *   introducing an immediate mismatch.
 *
 * How to compute the MWshift function
 * -----------------------------------
 * 
 * The MWshift function depends only on the pattern and therefore can be
 * preprocessed.
 * To compute MWshift(j1, j2) we need to solve a matching problem again but
 * within P itself i.e. we must find the smallest (non-zero) amount of letters
 * that we need to shift P(j1, j2) to the _left_ until it matches P again.
 * IMPORTANT: This match must be a "full" match (when we are still within the
 * limits of P) and a "partial" match (when we are out of limits i.e. we've
 * moved to far to the left). For a "partial" match, all letters that are
 * still within P limits must match. When we have moved so far that all
 * letters are out of limits, then we have a "partial" match too.
 * This ensure that MWshift(j1, j2) is always <= j2.
 * 
 * Example: P = acbaba
 *              012345
 * 
 *   MWshift(0,1) = 1
 *   MWshift(1,2) = 2
 *   MWshift(2,3) = 3
 *   MWshift(3.4) = MWshift(2,4) = MWshift(1,4) = MWshift(0,4) = 3
 *   ...
 *   MWshift(4,5) = MWshift(5,6) = MWshift(4,6) = 2
 *   MWshift(0,6) = 5 (MWshift max)
 * 
 * A nice property of the MWshift function is that:
 *     if j1 <= j1' < j2' <= j2, then MWshift(j1', j2') <= MWshift(j1, j2)
 * Therefore the biggest value for MWshift is reached for j1 = 0 and j2 = nP.
 * To summarize:
 *     1 <= MWshift(j1, j2) <= min(j2, MWshift(0, nP))
 * Another property of the MWshift function is that if P1 and P2 are 2
 * patterns such that P1 is a prefix of P2 then MWshift1 and MWshift2 (their
 * respective MWshift functions) are equal on the set of points (j1, j2) that
 * are valid for MWshift1.
 * This last property is used by the init_MWshift_table() C function below.
 * 
 * No need to preprocess the MWshift function
 * --------------------------------------
 *
 * However this preprocessing would be too expensive since it requires the
 * evaluation of nP * (nP + 1) / 2 values, and each evaluation itself is a
 * string matching sub-problem with a cost of its own. So the trick is to
 * delay those evaluations until they are needed, and the fact is that, in
 * practise, very few of them are actually needed compared to the total
 * number of possible MWshift(j1, j2) values.
 */

/* Contains the MWshift values for current pattern P0. */
static int *MWshift_table = NULL;

/* MWshift_table is a 2-dim array with nrow = ncol = P0buffer_length.
 * The layout of MWshift_table is (only the values marked with an "x" will
 * be potentially used):
 *
 *           1 2 3 4 5 6 j2
 *         0 x x x x - -
 *         1 - x x x - - 
 *         2 - - x x - -    P0buffer_nP = 4 <= P0buffer_length = 6
 *         3 - - - x - -
 *         4 - - - - - -
 *         5 - - - - - -
 *        j1
 *
 * The "x" region is defined by 0 <= j1 < j2 <= P0buffer_nP
 */

#define MW_SHIFT(j1, j2) (MWshift_table[P0buffer_length * (j1) + (j2) - 1])

static int get_MWshift(int j1, int j2)
{
	int shift, k1, k2, length;

	shift = MW_SHIFT(j1, j2);
	if (shift != 0)
		return shift;
	for (shift = 1; shift < j2; shift++) {
		if (shift < j1) k1 = j1 - shift; else k1 = 0;
		k2 = j2 - shift;
		length = k2 - k1;
		if (memcmp(P0buffer + k1, P0buffer + k1 + shift, length) == 0)
			break;
	}
	/* shift is j2 when the "for" loop is not interrupted by "break" */
	return MW_SHIFT(j1, j2) = shift;
}

static void init_MWshift_table()
{
	int j1, j2 = 1;

	if (P0buffer_init_status == -1 && MWshift_table != NULL) {
		free(MWshift_table);
		MWshift_table = NULL;
	}
	if (P0buffer_length != 0 && MWshift_table == NULL) {
		MWshift_table = (int *) malloc(P0buffer_length * P0buffer_length *
						sizeof(int));
		if (MWshift_table == NULL)
			error("can't allocate memory for MWshift_table");
	}
	if (P0buffer_init_status != -1)
		j2 = P0buffer_init_status + 1;
	for ( ; j2 <= P0buffer_nP; j2++) {
		for (j1 = 0; j1 < j2; j1++) {
			MW_SHIFT(j1, j2) = 0;
		}
	}
}


/****************************************************************************
 * The boyermoore() function
 * =========================
 * 
 * My poor understanding of the original Boyer-Moore algo as explained in Dan
 * Gusfield book "Algorithms on strings, trees, and sequences", is that it
 * will be inefficent in this situation:
 *   S = "Xbabababababab"
 *   P = "abababababab"
 * because after it has applied the "strong good suffix rule" and shifted P 2
 * letters to the right, it will start comparing P and S suffixes again which
 * is a waste of time.
 * The original idea of the "MWshift feature" introduced in the boyermoore()
 * function below was to never compare twice the letters that are in the
 * current Matching Window (defined by (j1, j2) in P and (i1, i2) in S).
 */

#define ADJUSTMW(i, j, shift) \
{ \
	if ((shift) <= (j)) \
		(j) -= (shift); \
	else { \
		(i) += (shift) - (j); \
		(j) = 0; \
	} \
}

/* Returns the number of matches */
static int boyermoore(const char *P, int nP, const char *S, int nS, int is_count_only)
{
	int count = 0, n, i1, i2, j1, j2, shift, shift1, i, j;
	char Pmrc, c; /* Pmrc is P right-most char */

	init_P0buffer(P, nP);
	init_j0shift0();
	init_VSGSshift_table();
	if (nP <= MWSHIFT_NPMAX)
		init_MWshift_table();
	Pmrc = P[nP-1];
	n = nP - 1;
	j2 = 0;
	while (n < nS) {
		if (j2 == 0) {
			/* No Matching Window yet, we need to find one */
			c = S[n];
			if (c != Pmrc) {
				shift = get_VSGSshift(c, nP-1);
				n += shift;
				continue;
			}
			i1 = n;
			i2 = i1 + 1;
			j2 = nP;
			j1 = j2 - 1;
			/* Now we have a Matching Window (1-letter suffix) */
		}
		/* We try to extend the current Matching Window... */
		if (j1 > 0) {
			/* ... to the left */
			for (i = i1-1, j = j1-1; j >= 0; i--, j--)
				if ((c = S[i]) != P[j])
					break;
			i1 = i + 1;
			j1 = j + 1;
		}
		if (j2 < nP) {
			/* ... to the right */
			for ( ; j2 < nP; i2++, j2++)
				if (S[i2] != P[j2])
					break;
		}
		if (j2 == nP) { /* the Matching Window is a suffix */
			if (j1 == 0) {
				/* we have a full match! */
				if (!is_count_only)
					_Biostrings_reportMatch(i1);
				count++;
				shift = P0buffer_shift0;
			} else {
				shift = get_VSGSshift(c, j1 - 1);
			}
		} else {
			shift = get_MWshift(j1, j2);
			c = S[n];
			if (c != Pmrc) {
				shift1 = get_VSGSshift(c, nP-1);
				if (shift1 > shift)
					shift = shift1;
			}
		}
		n += shift;
		if (nP <= MWSHIFT_NPMAX) {
			ADJUSTMW(i1, j1, shift)
			ADJUSTMW(i2, j2, shift)
		} else {
			j2 = 0; /* forget the current Matching Window */
		}
	}
	return count;
}

SEXP match_boyermoore(SEXP p_xp, SEXP p_offset, SEXP p_length,
		SEXP s_xp, SEXP s_offset, SEXP s_length,
		SEXP count_only)
{
	int pat_offset, pat_length, subj_offset, subj_length,
	    is_count_only, count;
	const Rbyte *pat, *subj;
	SEXP ans;

	pat_offset = INTEGER(p_offset)[0];
	pat_length = INTEGER(p_length)[0];
	pat = RAW(R_ExternalPtrTag(p_xp)) + pat_offset;
	subj_offset = INTEGER(s_offset)[0];
	subj_length = INTEGER(s_length)[0];
	subj = RAW(R_ExternalPtrTag(s_xp)) + subj_offset;
	is_count_only = LOGICAL(count_only)[0];

	if (!is_count_only)
		_Biostrings_resetMatchPosBuffer();
	count = boyermoore((char *) pat, pat_length, (char *) subj, subj_length, is_count_only);
	if (!is_count_only) {
		PROTECT(ans = allocVector(INTSXP, count));
		memcpy(INTEGER(ans), _Biostrings_resetMatchPosBuffer(),
					sizeof(int) * count);
	} else {
		PROTECT(ans = allocVector(INTSXP, 1));
		INTEGER(ans)[0] = count;
	}
	UNPROTECT(1);
	return ans;
}
