
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: pzstr.cc,v 2.29 2016/03/15 00:07:01 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1990 UCB CAD Group
         1993 Stephen R. Whiteley
****************************************************************************/

#include <math.h>
#include "pzdefs.h"
#include "input.h"
#include "spmatrix.h"
#include "outdata.h"
#include "misc.h"


//
//  A variant on the "zeroin" method.  This is a bit convoluted.
//

// These macros used to be in complex.h.  They are used only in this file.

#define DC_ABS(a,b) (fabs(a) + fabs(b))

#define DC_DIVEQ(a,b,c,d) { \
  double r,s,x,y; \
  if(fabs(c)>fabs(d)) { \
    r=(d)/(c); \
    s=(c)+r*(d); \
    x=((*(a))+(*(b))*r)/s; \
    y=((*(b))-(*(a))*r)/s; \
  } else { \
    r=(c)/(d); \
    s=(d)+r*(c); \
    x=((*(a))*r+(*(b)))/s; \
    y=((*(b))*r-(*(a)))/s; \
  } \
  (*(a)) = x; \
  (*(b)) = y; \
}

#define DC_MULT(a,b,c,d,x,y) { \
  *(x) = (a) * (c) - (b) * (d) ; \
  *(y) = (a) * (d) + (b) * (c) ; \
}

#define DC_MINUSEQ(a,b,c,d) { \
  *(a) -= (c) ; \
  *(b) -= (d) ; \
}

#define C_SQRT(A) { \
  double _mag, _a; \
  if ((A).imag == 0.0) { \
    if ((A).real < 0.0) { \
      (A).imag = sqrt(-(A).real); \
      (A).real = 0.0; \
    } else { \
      (A).real = sqrt((A).real); \
      (A).imag = 0.0; \
    } \
  } else { \
    _mag = sqrt((A).real * (A).real + (A).imag * (A).imag); \
    _a = (_mag - (A).real) / 2.0; \
    if (_a <= 0.0) { \
      (A).real = sqrt(_mag); \
      (A).imag /= (2.0 * (A).real); /*XXX*/ \
    } else { \
      _a = sqrt(_a); \
      (A).real = (A).imag / (2.0 * _a); \
      (A).imag = _a; \
    } \
  } \
}

#define C_MAG2(A) (((A).real = (A).real * (A).real + (A).imag * (A).imag), \
  (A).imag = 0.0)

#define C_CONJ(A) ((A).imag *= -1.0)

#define C_CONJEQ(A,B) { \
  (A).real = (B.real); \
  (A).imag = - (B.imag); \
}

#define C_EQ(A,B) { \
  (A).real = (B.real); \
  (A).imag = (B.imag); \
}

#define C_NORM(A,B) { \
  if ((A).real == 0.0 && (A).imag == 0.0) { \
    (B) = 0; \
  } else { \
    while (fabs((A).real) > 1.0 || fabs((A).imag) > 1.0) { \
      (B) += 1; \
      (A).real /= 2.0; \
      (A).imag /= 2.0; \
    } \
    while (fabs((A).real) <= 0.5 && fabs((A).imag) <= 0.5) { \
      (B) -= 1; \
      (A).real *= 2.0; \
      (A).imag *= 2.0; \
    } \
  } \
}

#define C_ABS(A) (sqrt((A).real * (A.real) + (A.imag * A.imag)))

#define C_MUL(A,B) { \
  double TMP1, TMP2; \
  TMP1 = (A.real); \
  TMP2 = (B.real); \
  (A).real = TMP1 * TMP2 - (A.imag) * (B.imag); \
  (A).imag = TMP1 * (B.imag) + (A.imag) * TMP2; \
}

#define C_MULEQ(A,B,C) { \
  (A).real = (B.real) * (C.real) - (B.imag) * (C.imag); \
  (A).imag = (B.real) * (C.imag) + (B.imag) * (C.real); \
}

#define C_DIV(A,B) { \
  double _tmp, _mag; \
  _tmp = (A.real); \
  (A).real = _tmp * (B.real) + (A).imag * (B.imag); \
  (A).imag = - _tmp * (B.imag) + (A.imag) * (B.real); \
  _mag = (B.real) * (B.real) + (B.imag) * (B.imag); \
  (A).real /= _mag; \
  (A).imag /= _mag; \
}

#define C_DIVEQ(A,B,C) { \
  double _mag; \
  (A).real = (B.real) * (C.real) + (B.imag) * (C.imag); \
  (A).imag = (B.imag) * (C.real) - (B.real) * (C.imag); \
  _mag = (C.real) * (C.real) + (C.imag) * (C.imag); \
  (A).real /= _mag; \
  (A).imag /= _mag; \
}

#define C_ADD(A,B) { \
  (A).real += (B.real); \
  (A).imag += (B.imag); \
}

#define C_ADDEQ(A,B,C) { \
  (A).real = (B.real) + (C.real); \
  (A).imag = (B.imag) + (C.imag); \
}

#define C_SUB(A,B) { \
  (A).real -= (B.real); \
  (A).imag -= (B.imag); \
}

#define C_SUBEQ(A,B,C) { \
  (A).real = (B.real) - (C.real); \
  (A).imag = (B.imag) - (C.imag); \
}


#ifdef notdef
#define DEBUG(N)    if (Debug >= (unsigned) (N))
namespace { unsigned int Debug = 1; }
#else
#define DEBUG(N)    if (0)
#endif

#define ERROR(CODE, MESSAGE) {  \
  IP.logError(0, MESSAGE); \
  return (CODE); }

#define R_NORM(A,B) { \
  if ((A) == 0.0) { \
    (B) = 0; \
  } else { \
    while (fabs(A) > 1.0) { \
      (B) += 1; \
      (A) /= 2.0; \
    } \
    while (fabs(A) < 0.5) { \
      (B) -= 1; \
      (A) *= 2.0; \
    } \
  } \
}


#define NITER_LIM    200

#define    SHIFT_LEFT    2
#define    SHIFT_RIGHT   3
#define    SKIP_LEFT     4
#define    SKIP_RIGHT    5
#define    INIT          6

#define    GUESS         7
#define    SPLIT_LEFT    8
#define    SPLIT_RIGHT   9

#define    MULLER        10
#define    SYM           11
#define    SYM2          12
#define    COMPLEX_INIT  13
#define    COMPLEX_GUESS 14
#define    QUIT          15

#define    NEAR_LEFT     4
#define    MID_LEFT      5
#define    FAR_LEFT      6
#define    NEAR_RIGHT    7
#define    FAR_RIGHT     8
#define    MID_RIGHT     9

namespace {
    const char *snames[ ] = {
        "none",
        "none",
        "shift left",
        "shift right",
        "skip left",
        "skip right",
        "init",
        "guess",
        "split left",
        "split right",
        "Muller",
        "sym 1",
        "sym 2",
        "complex_init",
        "complex_guess",
        "quit",
        "none"
    };
}

#define sgn(X) ((X) < 0 ? -1 : (X) == 0 ? 0 : 1)

#define    ISAROOT        2
#define    ISAREPEAT      4
#define    ISANABERRATION 8
#define    ISAMINIMA      16


int
sPZAN::PZfindZeros(sCKT *ckt, PZtrial **rootinfo, int *rootcount)
{
    NIpzK = 0.0;
    NIpzK_mag = 0;
    High_Guess = -1.0;
    Low_Guess = 1.0;
    ZeroTrial = 0;
    Trials = 0;
    NZeros = 0;
    NFlat = 0;
    Max_Zeros = ckt->CKTmatrix->spGetSize(1);
    NIter = 0;
    PZtrapped = 0;
    Aberr_Num = 0;
    NTrials = 0;
    ckt->CKTniState |= NIPZSHOULDREORDER; // Initial for LU fill-ins
    sPZAN *pzan = static_cast<sPZAN*>(ckt->CKTcurJob);

    Seq_Num = 1;

    PZtrial *neighborhood[3];
    PZreset(neighborhood);

    int error = OK;
    do {

        int strat;
        while ((strat = PZstrat(neighborhood)) < GUESS && !PZtrapped)
            if (!PZstep(strat, neighborhood)) {
                strat = GUESS;
                DEBUG(1) fprintf(stderr, "\t\tGuess\n");
                break;
            }

        NIter += 1;
    
        // Evaluate current strategy
        PZtrial *new_trial;
        error = PZeval(strat, neighborhood, &new_trial);
        if (error != OK)
            return (error);

        error = PZrunTrial(ckt, &new_trial, neighborhood);
        if (error != OK)
            return (error);

        if (new_trial->flags & ISAROOT) {
            if (PZverify(neighborhood, new_trial)) {
                NIter = 0;
                PZreset(neighborhood);
            }
            else
                // XXX Verify fails ?!?
                PZupdateSet(neighborhood, new_trial);
        }
        else if (new_trial->flags & ISANABERRATION) {
            PZreset(neighborhood);
            Aberr_Num += 1;
            delete new_trial;
        }
        else if (new_trial->flags & ISAMINIMA) {
            neighborhood[0] = 0;
            neighborhood[1] = new_trial;
            neighborhood[2] = 0;
        }
        else
            PZupdateSet(neighborhood, new_trial);    // Replace a value

        if ((error = OP.pauseTest(pzan->JOBrun)) < 0) {
            OP.error(ERR_WARNING,
                "Pole-Zero analysis interrupted; %d trials, %d roots",
                Seq_Num, NZeros); 
            break;
        }
    }
    while (High_Guess - Low_Guess < 1e40
        && NZeros < Max_Zeros
        && NIter < NITER_LIM && Aberr_Num < 3
        && High_Guess - Low_Guess < 1e35    // XXX Should use mach const
        && (!neighborhood[0] || !neighborhood[2] || PZtrapped
        || neighborhood[2]->s.real - neighborhood[0]->s.real < 1e22));
        // XXX ZZZ

    DEBUG(1) fprintf(stderr,
    "Finished: NFlat %d, NZeros: %d, NTrials %d, Guess %g to %g, aber %d\n",
    NFlat, NZeros, NTrials, Low_Guess, High_Guess, Aberr_Num);

    if (NZeros >= Seq_Num - 1) {
        // Short
        clear_trials(ISAROOT);
        *rootinfo = 0;
        *rootcount = 0;
        ERROR(E_SHORT,
            "The input signal is shorted on the way to the output");
    }
    else
        clear_trials(0);

    *rootinfo = Trials;
    *rootcount = NZeros;

    if (Aberr_Num > 2) {
        OP.error(ERR_WARNING,
    "Pole-zero converging to numerical aberrations; giving up after %d trials",
            Seq_Num);
    }

    if (NIter >= NITER_LIM) {
        OP.error(ERR_WARNING,
            "Pole-zero iteration limit reached; giving up after %d trials",
            Seq_Num);
    }

    return (error);
}


// PZeval: evaluate an estimation function (given by 'strat') for the next
// guess (returned in a PZtrial)

// XXX ZZZ
int
sPZAN::PZeval(int strat, PZtrial **set, PZtrial **new_trial_p)
{
    PZtrial *new_trial = new PZtrial;
    new_trial->multiplicity = 0;
    new_trial->count = 0;
    new_trial->seq_num = Seq_Num++;

    int error;
    switch (strat) {
    case GUESS:
        if (High_Guess < Low_Guess)
            Guess_Param = 0.0;
        else if (Guess_Param > 0.0) {
            if (High_Guess > 0.0)
                Guess_Param = High_Guess * 10.0;
            else
                Guess_Param = 1.0;
        }
        else {
            if (Low_Guess < 0.0)
                Guess_Param = Low_Guess * 10.0;
            else
                Guess_Param = -1.0;
        }
        if (Guess_Param > High_Guess)
            High_Guess = Guess_Param;
        if (Guess_Param < Low_Guess)
            Low_Guess = Guess_Param;
        new_trial->s.real = Guess_Param;
        if (set[1])
            new_trial->s.imag = set[1]->s.imag;
        else
            new_trial->s.imag = 0.0;
        error = OK;
        break;

    case SYM:
    case SYM2:
        error = NIpzSym(set, new_trial);

        if (PZtrapped == 1) {
            if (new_trial->s.real < set[0]->s.real
                    || new_trial->s.real > set[1]->s.real) {
                DEBUG(1) fprintf(stderr,
                    "FIXED UP BAD Strat: %s (%d) was (%.15g,%.15g)\n",
                    snames[strat], PZtrapped,
                    new_trial->s.real, new_trial->s.imag);
                new_trial->s.real =
                    (set[0]->s.real + set[1]->s.real) / 2.0;
            }
        }
        else if (PZtrapped == 2) {
            if (new_trial->s.real < set[1]->s.real
                    || new_trial->s.real > set[2]->s.real) {
                DEBUG(1) fprintf(stderr,
                    "FIXED UP BAD Strat: %s (%d) was (%.15g,%.15g)\n",
                    snames[strat], PZtrapped,
                    new_trial->s.real, new_trial->s.imag);
                new_trial->s.real =
                    (set[1]->s.real + set[2]->s.real) / 2.0;
            }
        }
        else if (PZtrapped == 3) {
            if (new_trial->s.real <= set[0]->s.real
                    || (new_trial->s.real == set[1]->s.real
                    && new_trial->s.imag == set[1]->s.imag)
                    || new_trial->s.real >= set[2]->s.real) {
                DEBUG(1) fprintf(stderr,
                    "FIXED UP BAD Strat: %s (%d), was (%.15g %.15g)\n",
                    snames[strat], PZtrapped,
                    new_trial->s.real, new_trial->s.imag);
                new_trial->s.real =
                    (set[0]->s.real + set[2]->s.real) / 2.0;
                if (new_trial->s.real == set[1]->s.real) {
                    DEBUG(1) fprintf(stderr, "Still off!");
                    if (Last_Move == MID_LEFT ||
                            Last_Move == NEAR_RIGHT)
                        new_trial->s.real =
                            (set[0]->s.real + set[1]->s.real) / 2.0;
                    else
                        new_trial->s.real =
                        (set[1]->s.real + set[2]->s.real) / 2.0;
                }
            }
        }

        break;

    case COMPLEX_INIT:
    // Not automatic
        DEBUG(1) fprintf(stderr, "\tPZ minima at: %-30g %d\n",
            NIpzK, NIpzK_mag);

        new_trial->s.real = set[1]->s.real;

        // NIpzK is a good idea, but the value gets trashed 
        // due to the numerics when zooming in on a minima.
        // The key is to know when to stop taking new values for NIpzK
        // (which I don't).  For now I take the first value indicated
        // by the NIpzSym2 routine.  A "hack".
        //

        if (NIpzK != 0.0 && NIpzK_mag > -10) {
            while (NIpzK_mag > 0) {
                NIpzK *= 2.0;
                NIpzK_mag -= 1;
            }
            while (NIpzK_mag < 0) {
                NIpzK /= 2.0;
                NIpzK_mag += 1;
            }
            new_trial->s.imag = NIpzK;
        }
        else
            new_trial->s.imag = 10000.0;

        // Reset NIpzK so the same value doesn't get used again.

        NIpzK = 0.0;
        NIpzK_mag = 0;
        error = OK;
        break;

    case COMPLEX_GUESS:
        if (!set[2]) {
            new_trial->s.real = set[0]->s.real;
            new_trial->s.imag = 1.0e8;
        }
        else {
            new_trial->s.real = set[0]->s.real;
            new_trial->s.imag = 1.0e12;
        }
        error = OK;
        break;

    case MULLER:
        error = NIpzMuller(set, new_trial);
        break;

    case SPLIT_LEFT:
        new_trial->s.real = (set[0]->s.real + 2 * set[1]->s.real) / 3.0;
        error = OK;
        break;

    case SPLIT_RIGHT:
        new_trial->s.real = (set[2]->s.real + 2 * set[1]->s.real) / 3.0;
        error = OK;
        break;

    default:
        ERROR(E_PANIC, "Step type unkown");
        break;
    }

    *new_trial_p = new_trial;
    return (error);
}


// CKTpzStrat: given three points, determine a good direction or method for
// guessing the next zero

// XXX ZZZ what is a strategy for complex hunting?
int
sPZAN::PZstrat(PZtrial **set)
{
    int new_trap = 0;
    int suggestion;
    if (set[1] && (set[1]->flags & ISAMINIMA))
        suggestion = COMPLEX_INIT;
    else if (set[0] && set[0]->s.imag != 0.0) {
        if (!set[1] || !set[2])
            suggestion = COMPLEX_GUESS;
        else
            suggestion = MULLER;
    }
    else if (!set[0] || !set[1] || !set[2])
        suggestion = INIT;
    else {
        if (sgn(set[0]->f_def.real) != sgn(set[1]->f_def.real)) {
            // Zero crossing between s[0] and s[1]
            new_trap = 1;
            suggestion = SYM2;
        }
        else if (sgn(set[1]->f_def.real) != sgn(set[2]->f_def.real)) {
            // Zero crossing between s[1] and s[2]
            new_trap = 2;
            suggestion = SYM2;
        }
        else {

            int a_mag, b_mag;
            double a, b;
            zaddeq(&a, &a_mag, set[1]->f_def.real, set[1]->mag_def,
                -set[0]->f_def.real, set[0]->mag_def);
            zaddeq(&b, &b_mag, set[2]->f_def.real, set[2]->mag_def,
                -set[1]->f_def.real, set[1]->mag_def);

            if (!PZtrapped) {

                double k1 = set[1]->s.real - set[0]->s.real;
                double k2 = set[2]->s.real - set[1]->s.real;
                if (a_mag + 10 < set[0]->mag_def
                        && a_mag + 10 < set[1]->mag_def
                        && b_mag + 10 < set[1]->mag_def
                        && b_mag + 10 < set[2]->mag_def) {
                    if (k1 > k2)
                        suggestion = SKIP_RIGHT;
                    else
                        suggestion = SKIP_LEFT;
                }
                else if (sgn(a) != -sgn(b)) {
                    if (a == 0.0)
                        suggestion = SKIP_LEFT;
                    else if (b == 0.0)
                        suggestion = SKIP_RIGHT;
                    else if (sgn(a) == sgn(set[1]->f_def.real))
                        suggestion = SHIFT_LEFT;
                    else
                        suggestion = SHIFT_RIGHT;
                }
                else if (sgn(a) == -sgn(set[1]->f_def.real)) {
                    new_trap = 3;
                    // minima in magnitude above the x axis
                    // Search for exact mag. minima, look for complex pair
                    suggestion = SYM;
                }
                else if (k1 > k2)
                    suggestion = SKIP_RIGHT;
                else
                    suggestion = SKIP_LEFT;
            }
            else {
                new_trap = 3; // still
                // XXX ? Are these tests needed or is SYM safe all the time?
                if (sgn(a) != sgn(b)) {
                    // minima in magnitude
                    // Search for exact mag. minima, look for complex pair
                    suggestion = SYM;
                }
                else if (a_mag > b_mag ||
                        (a_mag == b_mag && fabs(a) > fabs(b)))
                    suggestion = SPLIT_LEFT;
                else
                    suggestion = SPLIT_RIGHT;
            }
        }
        if (Consec_Moves >= 3 && PZtrapped == new_trap) {
            new_trap = PZtrapped;
            if (Last_Move == MID_LEFT || Last_Move == NEAR_RIGHT)
                suggestion = SPLIT_LEFT;
            else if (Last_Move == MID_RIGHT || Last_Move == NEAR_LEFT)
                suggestion = SPLIT_RIGHT;
            else
                abort( );    // XXX
            Consec_Moves = 0;
        }
    }

    PZtrapped = new_trap;
    DEBUG(1) {
        if (set[0] && set[1] && set[2])
            fprintf(stderr, "given %.15g %.15g / %.15g %.15g / %.15g %.15g\n",
            set[0]->s.real, set[0]->s.imag, set[1]->s.real, set[1]->s.imag,
            set[2]->s.real, set[2]->s.imag);
        fprintf(stderr, "suggestion(%d/%d/%d | %d): %s\n",
        NFlat, NZeros, Max_Zeros, PZtrapped, snames[suggestion]);
    }
    return (suggestion);
}


// PZrunTrial: eval the function at a given 's', fold in deflation
//
int
sPZAN::PZrunTrial(sCKT *ckt, PZtrial **new_trialp, PZtrial **set)
{
    PZtrial *new_trial = *new_trialp;

    if (new_trial->s.imag < 0.0)
        new_trial->s.imag *= -1.0;

    // Insert the trial into the list of Trials, while calculating
    // the deflation factor from previous zeros

    int pretest = 0;
    int shifted = 0;
    int repeat = 0;
    int error = OK;

    IFcomplex def_frac;
    int def_mag;
    PZtrial *p, *prev, *match;
    do {

        def_mag = 0;
        def_frac.real = 1.0;
        def_frac.imag = 0.0;
        int was_shifted = shifted;
        shifted = 0;

        prev = 0;
        match = 0;

        for (p = Trials; p != 0; p = p->next) {

            IFcomplex diff_frac;
            C_SUBEQ(diff_frac,p->s,new_trial->s);

            if (diff_frac.real < 0.0
                    || (diff_frac.real == 0.0 && diff_frac.imag < 0.0)) {
                prev = p;
            }

            double reltol, abstol;
            if (p->flags & ISAROOT) {
                abstol = 1e-5;
                reltol = 1e-6;
            }
            else {
                abstol = 1e-20;
                reltol = 1e-12;
            }

            if (diff_frac.imag == 0.0 &&
                    fabs(diff_frac.real) / (fabs(p->s.real) + abstol/reltol)
                    < reltol) {

                DEBUG(1) {
                    fprintf(stderr,
                        "diff_frac.real = %10g, p->s = %10g, nt = %10g\n",
                        diff_frac.real, p->s.real, new_trial->s.real);
                    fprintf(stderr, "ab=%g,rel=%g\n", abstol, reltol);
                }
                if (was_shifted || p->count >= 3
                        || !alter(new_trial, set[1], abstol, reltol)) {
                    // assume either a root or minima
                    p->count = 0;
                    pretest = 1;
                    break;
                }
                else
                    p->count += 1;    // try to shift

                shifted = 1;    // Re-calculate deflation
                break;

            }
            else {
                if (!PZtrapped)
                    p->count = 0;
                if (p->flags & ISAROOT) {
                    int diff_mag = 0;
                    C_NORM(diff_frac, diff_mag);
                    if (diff_frac.imag != 0.0) {
                        C_MAG2(diff_frac);
                        diff_mag *= 2;
                    }
                    C_NORM(diff_frac, diff_mag);

                    for (int i = p->multiplicity; i > 0; i--) {
                        C_MUL(def_frac, diff_frac);
                        def_mag += diff_mag;
                        C_NORM(def_frac, def_mag);
                    }
                }
                else if (!match)
                    match = p;
            }
        }

    }
    while (shifted);

    if (pretest) {

        DEBUG(1) fprintf(stderr, "Pre-test taken\n");

        // XXX Should catch the double-zero right off
        // if K is 0.0
        // instead of forcing a re-converge
        //
        DEBUG(1) {
            fprintf(stderr, "NIpzK == %g, mag = %d\n", NIpzK, NIpzK_mag);
            fprintf(stderr, "over at %.30g %.30g (new %.30g %.30g, %x)\n",
            p->s.real, p->s.imag, new_trial->s.real, new_trial->s.imag,
            p->flags);
        }
        if (!(p->flags & ISAROOT) && PZtrapped == 3
                && NIpzK != 0.0 && NIpzK_mag > -10) {
#ifdef notdef
            if (p->flags & ISAROOT) {
                // Ugh! muller doesn't work right
                new_trial->flags = ISAMINIMA;
                new_trial->s.imag = scalb(NIpzK, (int) (NIpzK_mag / 2));
                pretest = 0;
            }
            else {
#endif
            p->flags |= ISAMINIMA;
            delete new_trial;
            *new_trialp = p;
            repeat = 1;
        }
        else if (p->flags & ISAROOT) {
            DEBUG(1) fprintf(stderr, "Repeat at %.30g %.30g\n",
                p->s.real, p->s.imag);
            *new_trialp = p;
            p->flags |= ISAREPEAT;
            p->multiplicity += 1;
            repeat = 1;
        }
        else {
            // Regular zero, as precise as we can get it
            error = E_SINGULAR;
        }
    }

    if (!repeat) {
        if (!pretest) {
            // Run the trial
            ckt->CKTniState |= NIPZSHOULDREORDER;    // XXX
            if (!(ckt->CKTniState & NIPZSHOULDREORDER)) {
                PZload(ckt, &new_trial->s);
                DEBUG(3) {
                    printf("Original:\n");
                    ckt->CKTmatrix->spPrint(0, 1, 1);
                }
                error = ckt->CKTmatrix->spFactor();
                if (error == E_SINGULAR) {
                    DEBUG(1) printf("Needs reordering\n");
                    ckt->CKTniState |= NIPZSHOULDREORDER;
                }
                else if (error != OK)
                    return (error);
            }
            if (ckt->CKTniState & NIPZSHOULDREORDER) {
                PZload(ckt, &new_trial->s);
                error = ckt->CKTmatrix->spOrderAndFactor(0,
                    0.0 /* 0.1 Piv. Rel. */,1.0e-30, 1);
                ((sPZAN *) ckt->CKTcurJob)->PZnumswaps = 1;
            }

            if (error != E_SINGULAR) {
                ckt->CKTniState &= ~NIPZSHOULDREORDER;
                DEBUG(3) {
                    printf("Factored:\n");
                    ckt->CKTmatrix->spPrint(0, 1, 1);
                }
                error = ckt->CKTmatrix->spDProd(&new_trial->f_raw.real,
                    &new_trial->f_raw.imag, &new_trial->mag_raw);
            }
        }

        if (error == E_SINGULAR || (new_trial->f_raw.real == 0.0
                && new_trial->f_raw.imag == 0.0)) {
            new_trial->f_raw.real = 0.0;
            new_trial->f_raw.imag = 0.0;
            new_trial->mag_raw = 0;
            new_trial->f_def.real = 0.0;
            new_trial->f_def.imag = 0.0;
            new_trial->mag_def = 0;
            new_trial->flags = ISAROOT;
        }
        else if (error != OK)
            return (error);
        else {

            // PZnumswaps is either 0 or 1
            new_trial->f_raw.real *= ((sPZAN *) ckt->CKTcurJob)->PZnumswaps;
            new_trial->f_raw.imag *= ((sPZAN *) ckt->CKTcurJob)->PZnumswaps;

            new_trial->f_def.real = new_trial->f_raw.real;
            new_trial->f_def.imag = new_trial->f_raw.imag;
            new_trial->mag_def = new_trial->mag_raw;

            C_DIV(new_trial->f_def, def_frac);
            new_trial->mag_def -= def_mag;
            C_NORM(new_trial->f_def,new_trial->mag_def);
        }

        // Link into the rest of the list
        if (prev) {
            new_trial->next = prev->next;
            if (prev->next)
                prev->next->prev = new_trial;
            prev->next = new_trial;
        }
        else {
            if (Trials)
                Trials->prev = new_trial;
            else
                ZeroTrial = new_trial;
            new_trial->next = Trials;
            Trials = new_trial;
        }
        new_trial->prev = prev;

        NTrials += 1;

        if (!(new_trial->flags & ISAROOT)) {
            if (match)
                check_flat(match, new_trial);
            else
                NFlat = 1;
        }
    }

    show_trial(new_trial, '*');

    return (OK);
}


// Process a zero; inc. zero count, deflate other trials
//
int
sPZAN::PZverify(PZtrial **set, PZtrial *new_trial)
{
    (void)set;
    NZeros += 1;
    if (new_trial->s.imag != 0.0)
        NZeros += 1;
    NFlat = 0;

    if (new_trial->multiplicity == 0) {
        new_trial->flags |= ISAROOT;
        new_trial->multiplicity = 1;
    }

    PZtrial *prev = 0;
    PZtrial *next;
    for (PZtrial *t = Trials; t; t = next) {

        next = t->next;

        if (t->flags & ISAROOT) {
            prev = t;
            // Don't need to bother
            continue;
        }

        IFcomplex diff_frac;
        C_SUBEQ(diff_frac, new_trial->s, t->s);
        if (new_trial->s.imag != 0.0)
            C_MAG2(diff_frac);

        double tdiff = diff_frac.real;
        // Note that Verify is called for each time the root is found, so
        // multiplicity is not significant
        //
        if (diff_frac.real != 0.0) {
            int diff_mag = 0;
            C_NORM(diff_frac, diff_mag);
            diff_mag *= -1;
            C_DIV(t->f_def, diff_frac);
            C_NORM(t->f_def, diff_mag);
            t->mag_def += diff_mag;
        }

        if (t->s.imag != 0.0
                || fabs(tdiff) / (fabs(new_trial->s.real) + 200) < 0.005) {
            if (prev)
                prev->next = t->next;
            if (t->next)
                t->next->prev = prev;
            NTrials -= 1;
            show_trial(t, '-');
            if (t == ZeroTrial) {
                if (t->next)
                    ZeroTrial = t->next;
                else if (t->prev)
                    ZeroTrial = t->prev;
                else
                    ZeroTrial = 0;
            }
            if (t == Trials) {
                Trials = t->next;
            }
            delete [] t;
        }
        else {

            if (prev)
                check_flat(prev, t);
            else
                NFlat = 1;

            if (t->flags & ISAMINIMA)
                t->flags &= ~ISAMINIMA;

            prev = t;
            show_trial(t, '+');
        }
    }

    return (1);    // always ok
}


// pzseek: search the trial list (given a starting point) for the first
//    non-zero entry; direction: -1 for prev, 1 for next, 0 for next
//    -or- first.  Also, sets "Guess_Param" at the next reasonable
//    value to guess at if the search falls of the end of the list
//
PZtrial *
sPZAN::pzseek(PZtrial *t, int dir)
{
    Guess_Param = dir;
    if (t == 0)
        return (0);

    if (dir == 0 && !(t->flags & ISAROOT) && !(t->flags & ISAMINIMA))
        return (t);

    do {
        if (dir >= 0)
            t = t->next;
        else
            t = t->prev;
    }
    while (t && ((t->flags & ISAROOT) || (t->flags & ISAMINIMA)));

    return (t);
}


void
sPZAN::clear_trials(int mode)
{
    PZtrial *t, *next, *prev = 0;
    for (t = Trials; t; t = next) {
        next = t->next;
        if (mode || !(t->flags & ISAROOT)) {
            delete [] t;
        }
        else {
            if (prev)
                prev->next = t;
            else
                Trials = t;
            t->prev = prev;
            prev = t;
        }
    }

    if (prev)
        prev->next = 0;
    else
        Trials = 0;
}


void
sPZAN::PZupdateSet(PZtrial **set, PZtrial *ntrial)
{
    int this_move = 0;
    if (ntrial->s.imag != 0.0) {
        set[2] = set[1];
        set[1] = set[0];
        set[0] = ntrial;
    }
    else if (!set[1])
        set[1] = ntrial;
    else if (!set[2] && ntrial->s.real > set[1]->s.real)
        set[2] = ntrial;
    else if (!set[0])
        set[0] = ntrial;
    else if (ntrial->flags & ISAMINIMA)
        set[1] = ntrial;
    else if (ntrial->s.real < set[0]->s.real) {
        set[2] = set[1];
        set[1] = set[0];
        set[0] = ntrial;
        this_move = FAR_LEFT;
    }
    else if (ntrial->s.real < set[1]->s.real) {
        if (!PZtrapped || ntrial->mag_def < set[1]->mag_def
                || (ntrial->mag_def == set[1]->mag_def
                && fabs(ntrial->f_def.real) < fabs(set[1]->f_def.real))) {
            // Really should check signs, not just compare fabs( )
            set[2] = set[1];    // XXX = set[2]->prev :: possible opt
            set[1] = ntrial;
            this_move = MID_LEFT;
        }
        else {
            set[0] = ntrial;
            this_move = NEAR_LEFT;
        }
    }
    else if (ntrial->s.real < set[2]->s.real) {
        if (!PZtrapped || ntrial->mag_def < set[1]->mag_def
                || (ntrial->mag_def == set[1]->mag_def
                && fabs(ntrial->f_def.real) < fabs(set[1]->f_def.real))) {
            // Really should check signs, not just compare fabs( )
            set[0] = set[1];
            set[1] = ntrial;
            this_move = MID_RIGHT;
        }
        else {
            set[2] = ntrial;
            this_move = NEAR_RIGHT;
        }
    }
    else {
        set[0] = set[1];
        set[1] = set[2];
        set[2] = ntrial;
        this_move = FAR_RIGHT;
    }

    if (PZtrapped && this_move == Last_Move)
        Consec_Moves += 1;
    else
        Consec_Moves = 0;
    Last_Move = this_move;
}


void
sPZAN::zaddeq(double *a, int *amag, double x, int xmag, double y, int ymag)
{
    // Balance magnitudes . . .
    if (xmag > ymag) {
        *amag = xmag;
        if (xmag > 50 + ymag)
            y = 0.0;
        else
            for (xmag -= ymag; xmag > 0; xmag--)
                y /= 2.0;
    }
    else {
        *amag = ymag;
        if (ymag > 50 + xmag)
            x = 0.0;
        else
            for (ymag -= xmag; ymag > 0; ymag--)
                x /= 2.0;
    }

    *a = x + y;
    if (*a == 0.0)
        *amag = 0;
    else {
        while (fabs(*a) > 1.0) {
            *a /= 2.0;
            *amag += 1;
        }
        while (fabs(*a) < 0.5) {
            *a *= 2.0;
            *amag -= 1;
        }
    }
}


void
sPZAN::show_trial(PZtrial *new_trial, char x)
{
    DEBUG(1)
        fprintf(stderr, "%c (%3d/%3d) %.15g %.15g :: %.30g %.30g %d\n", x,
            NIter, new_trial->seq_num, new_trial->s.real, new_trial->s.imag,
            new_trial->f_def.real, new_trial->f_def.imag, new_trial->mag_def);
    DEBUG(1)
        if (new_trial->flags & ISANABERRATION)
            fprintf(stderr, "*** numerical aberration ***\n");
}


void
sPZAN::check_flat(PZtrial *a, PZtrial *b)
{

    int diff_mag = a->mag_def - b->mag_def;
    if (abs(diff_mag) <= 1) {
        double mult;
        if (diff_mag == 1)
            mult = 2.0;
        else if (diff_mag == -1)
            mult = 0.5;
        else
            mult = 1.0;
        IFcomplex diff_frac;
        C_SUBEQ(diff_frac, mult * a->f_def, b->f_def);
        C_MAG2(diff_frac);
        if (diff_frac.real < 1.0e-20)
            NFlat += 1;
    }
    // XXX else NFlat = ??????
}


// XXX ZZZ
int
sPZAN::PZstep(int strat, PZtrial **set)
{
    switch (strat) {
    case INIT:
        if (!set[1]) {
            set[1] = pzseek(ZeroTrial, 0);
        }
        else if (!set[2])
            set[2] = pzseek(set[1], 1);
        else if (!set[0])
            set[0] = pzseek(set[1], -1);
        break;

    case SKIP_LEFT:
        set[0] = pzseek(set[0], -1);
        break;

    case SKIP_RIGHT:
        set[2] = pzseek(set[2], 1);
        break;

    case SHIFT_LEFT:
        set[2] = set[1];
        set[1] = set[0];
        set[0] = pzseek(set[0], -1);
        break;

    case SHIFT_RIGHT:
        set[0] = set[1];
        set[1] = set[2];
        set[2] = pzseek(set[2], 1);
        break;
    }
    if (!set[0] || !set[1] || !set[2])
        return (0);
    else
        return (1);
}


void
sPZAN::PZreset(PZtrial **set)
{
    PZtrapped = 0;
    Consec_Moves = 0;

    set[1] = pzseek(ZeroTrial, 0);
    if (set[1] != 0) {
        set[0] = pzseek(set[1], -1);
        set[2] = pzseek(set[1], 1);
    }
    else {
        set[0] = 0;
        set[2] = 0;
    }
}


int
sPZAN::alter(PZtrial *ntrial, PZtrial *nearto, double abstol, double reltol)
{
    DEBUG(1) fprintf(stderr, "ALTER from: %.30g %.30g\n",
        ntrial->s.real, ntrial->s.imag);
    DEBUG(1) fprintf(stderr, "nt->next %g\n", nearto->prev->s.real);
    DEBUG(1) fprintf(stderr, "nt->next %g\n", nearto->next->s.real);

    double p1, p2;
    if (PZtrapped != 2) {
        DEBUG(1) fprintf(stderr, "not 2\n");
        p1 = nearto->s.real;
        if (nearto->flags & ISAROOT)
            p1 -= 1e-6 * nearto->s.real + 1e-5;
        if (nearto->prev) {
            p1 += nearto->prev->s.real;
            DEBUG(1) fprintf(stderr, "p1 %g\n", p1);
        }
        else
            p1 -= 10.0 * (fabs(p1) + 1.0);

        p1 /= 2.0;
    }
    else
        p1 = nearto->s.real;

    if (PZtrapped != 1) {
        DEBUG(1) fprintf(stderr, "not 1\n");
        p2 = nearto->s.real;
        if (nearto->flags & ISAROOT)
            p2 += 1e-6 * nearto->s.real + 1e-5;
            // XXX Would rather use pow(2)
        if (nearto->next) {
            p2 += nearto->next->s.real;
            DEBUG(1) fprintf(stderr, "p2 %g\n", p2);
        }
        else
            p2 += 10.0 * (fabs(p2)+ 1.0);

        p2 /= 2.0;
    }
    else
        p2 = nearto->s.real;

    if ((nearto->prev && fabs(p1 - nearto->prev->s.real) /
            (fabs(nearto->prev->s.real) + abstol/reltol) < reltol)
            || (nearto->next && fabs(p2 - nearto->next->s.real) /
            (fabs(nearto->next->s.real) + abstol/reltol) < reltol)) {

        DEBUG(1)
            fprintf(stderr, "Bailed out\n");
        return 0;
    }

    if (PZtrapped != 2 && nearto->s.real - p1 > p2 - nearto->s.real) {
        DEBUG(1) fprintf(stderr, "take p1\n");
        ntrial->s.real = p1;
    }
    else {
        DEBUG(1) fprintf(stderr, "take p2\n");
        ntrial->s.real = p2;
    }

    DEBUG(1) fprintf(stderr, "ALTER to  : %.30g %.30g\n",
    ntrial->s.real, ntrial->s.imag);
    return (1);
}


//
// The 'NI' routines.
//

int
sPZAN::NIpzSym(PZtrial *set[3], PZtrial *newt)
{
#ifndef notdef
    return NIpzSym2(set, newt);
#else
    double dx0 = set[1]->s.real - set[0]->s.real;
    double dx1 = set[2]->s.real - set[1]->s.real;

    int a_mag, b_mag, c_mag;
    double a, b, c;
    zaddeq(&a, &a_mag, set[1]->f_def.real, set[1]->mag_def,
        -set[0]->f_def.real, set[0]->mag_def);
    a /= dx0;
    zaddeq(&b, &b_mag, set[2]->f_def.real, set[2]->mag_def,
        -set[1]->f_def.real, set[1]->mag_def);
    b /= dx1;
    zaddeq(&c, &c_mag, b, b_mag, -a, a_mag);

    // XXX What if c == 0.0 ?

    double x0 = (set[0]->s.real + set[1]->s.real) / 2.0;
    double x1 = (set[1]->s.real + set[2]->s.real) / 2.0;

    c /= (x1 - x0);

    newt->s.real = - a / c;
    c_mag -= a_mag;

    newt->s.imag = 0.0;

    while (c_mag > 0) {
        newt->s.real /= 2.0;
        c_mag -= 1;
    }
    while (c_mag < 0) {
        newt->s.real *= 2.0;
        c_mag += 1;
    }
    newt->s.real += set[0]->s.real;
#endif
}


int
sPZAN::NIpzComplex(PZtrial *set[3], PZtrial *newt)
{
    return NIpzSym2(set, newt);
#ifdef notdef
    NIpzMuller(set, newt);
#endif
}


int
sPZAN::NIpzMuller(PZtrial *set[3], PZtrial *newtry)
{
    int min = -999999;
    int i, j = 0;
    int total = 0;
    for (i = 0; i < 3; i++) {
        if (set[i]->f_def.real != 0.0 || set[i]->f_def.imag != 0.0) {
            if (min < set[i]->mag_def - 50)
            min = set[i]->mag_def - 50;
            total += set[i]->mag_def;
            j += 1;
        }
    }

    int magx = total / j;
    if (magx < min)
        magx = min;

    DEBUG(2) fprintf(stderr, "Base scale: %d / %d (0: %d, 1: %d, 2: %d)\n",
    magx, min, set[0]->mag_def, set[1]->mag_def, set[2]->mag_def);

    double scale[3];
    int mag[3];
    for (i = 0; i < 3; i++) {
        mag[i] = set[i]->mag_def - magx;
        scale[i] = 1.0;
        while (mag[i] > 0) {
            scale[i] *= 2.0;
            mag[i] -= 1;
        }
        if (mag[i] < -90)
            scale[i] = 0.0;
        else {
            while (mag[i] < 0) {
            scale[i] /= 2.0;
            mag[i] += 1;
            }
        }
    }

    IFcomplex h0, h1;
    C_SUBEQ(h0, set[0]->s, set[1]->s);
    C_SUBEQ(h1, set[1]->s, set[2]->s);
    IFcomplex lambda_i;
    C_DIVEQ(lambda_i, h0, h1);

    // Quadratic interpolation (Muller's method)

    IFcomplex delta_i;
    C_EQ(delta_i, lambda_i);
    delta_i.real += 1.0;

    // Quadratic coefficients A, B, C (Note: reciprocal form of eqn)

    // A = lambda_i * (f[i-2] * lambda_i - f[i-1] * delta_i + f[i])
    IFcomplex A, B, C, D, E;
    C_MULEQ(A,scale[2] * set[2]->f_def,lambda_i);
    C_MULEQ(C,scale[1] * set[1]->f_def,delta_i);
    C_SUB(A,C);
    C_ADD(A,scale[0] * set[0]->f_def);
    C_MUL(A,lambda_i);

    // B = f[i-2] * lambda_i * lambda_1 - f[i-1] * delta_i * delta_i
    // + f[i] * (lambda_i + delta_i)
    C_MULEQ(B,lambda_i,lambda_i);
    C_MUL(B,scale[2] * set[2]->f_def);
    C_MULEQ(C,delta_i,delta_i);
    C_MUL(C,scale[1] * set[1]->f_def);
    C_SUB(B,C);
    C_ADDEQ(C,lambda_i,delta_i);
    C_MUL(C,scale[0] * set[0]->f_def);
    C_ADD(B,C);

    // C = delta_i * f[i]
    C_MULEQ(C,delta_i,scale[0] * set[0]->f_def);

    while (fabs(A.real) > 1.0 || fabs(A.imag) > 1.0
        || fabs(B.real) > 1.0 || fabs(B.imag) > 1.0
        || fabs(C.real) > 1.0 || fabs(C.imag) > 1.0) {
        A.real /= 2.0;
        B.real /= 2.0;
        C.real /= 2.0;
        A.imag /= 2.0;
        B.imag /= 2.0;
        C.imag /= 2.0;
    }

    // discriminate = B * B - 4 * A * C
    C_MULEQ(D,B,B);
    C_MULEQ(E,4.0 * A,C);
    C_SUB(D,E);

    DEBUG(2) fprintf(stderr, "  Discr: (%g,%g)\n",D.real, D.imag);
    C_SQRT(D);
    DEBUG(1) fprintf(stderr, "  sqrtDiscr: (%g,%g)\n",D.real, D.imag);

#ifndef notdef
    // Maximize denominator

    DEBUG(1) fprintf(stderr, "  B: (%g,%g)\n",B.real, B.imag);
    // Dot product
    double q = B.real * D.real + B.imag * D.imag;
    if (q > 0.0) {
        DEBUG(1) fprintf(stderr, "       add\n");
        C_ADD(B,D);
    } else {
        DEBUG(1) fprintf(stderr, "       sub\n");
        C_SUB(B,D);
    }

#else
    // For trapped zeros, the step should always be positive
    if (C.real >= 0.0) {
        if (B.real < D.real) {
            C_SUB(B,D);
        } else {
            C_ADD(B,D);
        }
    } else {
        if (B.real > D.real) {
            C_SUB(B,D);
        } else {
            C_ADD(B,D);
        }
    }
#endif

    DEBUG(1) fprintf(stderr, "  C: (%g,%g)\n", C.real, C.imag);
    C_DIV(C,-0.5 * B);

    newtry->next = 0;

    DEBUG(1) fprintf(stderr, "  lambda: (%g,%g)\n",C.real, C.imag);
    C_MULEQ(newtry->s,h0,C);

    DEBUG(1) fprintf(stderr, "  h: (%g,%g)\n", newtry->s.real, newtry->s.imag);

    C_ADD(newtry->s,set[0]->s);

    DEBUG(1) fprintf(stderr, "New try: (%g,%g)\n",
    newtry->s.real, newtry->s.imag);

    return OK;
}


int
sPZAN::NIpzSym2(PZtrial *set[3], PZtrial *newt)
{
    /*
    NIpzK = 0.0;
    NIpzK_mag = 0;
    */

    int error = OK;
    // Solve for X = the distance from set[1], where X > 0 => root < set[1]
    double dx0 = set[1]->s.real - set[0]->s.real;
    double dx1 = set[2]->s.real - set[1]->s.real;

    double x0 = (set[0]->s.real + set[1]->s.real) / 2.0;
    // double x1 = (set[1]->s.real + set[2]->s.real) / 2.0;
    // d2x = x1 - x0;
    double d2x = (set[2]->s.real - set[0]->s.real) / 2.0;

    double a;
    int a_mag;
    zaddeq(&a, &a_mag, set[1]->f_def.real, set[1]->mag_def,
        -set[0]->f_def.real, set[0]->mag_def);

    int tmag = 0;
    R_NORM(dx0,tmag);
    a /= dx0;
    a_mag -= tmag;
    R_NORM(a, a_mag);

    double b;
    int b_mag;
    zaddeq(&b, &b_mag, set[2]->f_def.real, set[2]->mag_def,
        -set[1]->f_def.real, set[1]->mag_def);

    tmag = 0;
    R_NORM(dx1,tmag);
    b /= dx1;
    b_mag -= tmag;
    R_NORM(b, b_mag);

    double c;
    int c_mag;
    zaddeq(&c, &c_mag, b, b_mag, -a, a_mag);

    tmag = 0;
    R_NORM(d2x,tmag);
    c /= d2x;    // = f''
    c_mag -= tmag;
    R_NORM(c, c_mag);

    if (c == 0.0 || ((a == 0.0 || c_mag < a_mag - 40)
            && (b = 0.0 || c_mag < b_mag - 40))) {
        //fprintf(stderr, "\t- linear (%g, %d)\n", c, c_mag);
        if (a == 0.0) {
            a = b;
            a_mag = b_mag;
        }
        if (a != 0.0) {
            newt->s.real = - set[1]->f_def.real / a;
            a_mag -= set[1]->mag_def;
            while (a_mag > 0) {
                newt->s.real /= 2.0;
                a_mag -= 1;
            }
            while (a_mag < 0) {
                newt->s.real *= 2.0;
                a_mag += 1;
            }
            newt->s.real += set[1]->s.real;
        }
        else
            newt->s.real = set[1]->s.real;
    }
    else {

        // Quadratic power series about set[1]->s.real
        // c : d2f/dx2 @ s1 (assumed constant for all s), or "2A"

        // a : (df/dx) / (d2f/dx2) @ s1, or "B/2A"
        a /= c;
        R_NORM(a,a_mag);
        a_mag -= c_mag;

        double diff = set[1]->s.real - x0;
        tmag = 0;
        R_NORM(diff,tmag);

        zaddeq(&a, &a_mag, a, a_mag, diff, tmag);

        // b : f(s1) / (1/2 d2f/ds2), or "C / A"
        b = 2.0 * set[1]->f_def.real / c;
        b_mag = set[1]->mag_def - c_mag;
        R_NORM(b,b_mag);

        double disc = a * a;
        int disc_mag = 2 * a_mag;

        // disc = a^2  - b  :: (B/2A)^2 - C/A
        zaddeq(&disc, &disc_mag, disc, disc_mag, - b, b_mag);

        // XXX Spice3 issue here.  In Spice3f5, new_mag is not
        // initialized, and is very likely always nonzero.  "Fixing"
        // the problem by initializing new_mag to 0 causes
        // strangeness:  in the pz2.cir example, the correct values
        // are found, but there is an iteration count overflow
        // message.
        // int new_mag = 0;
        int new_mag = 1;

        if (disc < 0.0) {
            // Look for minima instead, but save radical for later work
            disc *= -1;
            new_mag = 1;
        }

        if (disc_mag % 2 == 0)
            disc = sqrt(disc);
        else {
            disc = sqrt(2.0 * disc);
            disc_mag -= 1;
        }
        disc_mag /= 2;

        if (new_mag != 0) {
            if (NIpzK == 0.0) {
                NIpzK = disc;
                NIpzK_mag = disc_mag;
                DEBUG(1) fprintf(stderr, "New NIpzK: %g*2^%d\n",
                    NIpzK, NIpzK_mag);
            }
            else {
                DEBUG(1) fprintf(stderr,
                "Ignore NIpzK: %g*2^%d for previous value of %g*2^%d\n",
                disc, disc_mag,
                NIpzK, NIpzK_mag);
            }
            disc = 0.0;
            disc_mag = 0;
        }

        // NOTE: c & b get reused here-after

        if (a * disc >= 0.0)
            zaddeq(&c, &c_mag, a, a_mag, disc, disc_mag);
        else
            zaddeq(&c, &c_mag, a, a_mag, -disc, disc_mag);

        // second root = C / (first root)
        if (c != 0.0) {
            b /= c;
            b_mag -= c_mag;
        }
        else {
            // special case
            b = 0.0;
            b = 0;
        }

        zaddeq(&b, &b_mag, set[1]->s.real, 0, -b, b_mag);
        zaddeq(&c, &c_mag, set[1]->s.real, 0, -c, c_mag);

        while (b_mag > 0) {
            b *= 2.0;
            b_mag -= 1;
        }
        while (b_mag < 0) {
            b /= 2.0;
            b_mag += 1;
        }

        while (c_mag > 0) {
            c *= 2.0;
            c_mag -= 1;
        }
        while (c_mag < 0) {
            c /= 2.0;
            c_mag += 1;
        }

        DEBUG(1) fprintf(stderr, "@@@ (%.15g) -vs- (%.15g)\n", b, c);
        // XXXX
        if (b < set[0]->s.real || b > set[2]->s.real) {
            // b not in range
            if (c < set[0]->s.real || c > set[2]->s.real) {
                DEBUG(1) fprintf(stderr, "@@@ both are junk\n");
                if (PZtrapped == 1)
                    newt->s.real = (set[0]->s.real + set[1]->s.real) / 2.0;
                else if (PZtrapped == 2)
                    newt->s.real = (set[1]->s.real + set[2]->s.real) / 2.0;
                else if (PZtrapped == 3) {
                    if (fabs(set[1]->s.real - c) < fabs(set[1]->s.real - b)) {
                        DEBUG(1) fprintf(stderr, "@@@ mix w/second (c)\n");
                        newt->s.real = (set[1]->s.real + c) / 2.0;
                    }
                    else {
                        DEBUG(1) fprintf(stderr, "@@@ mix w/first (b)\n");
                        newt->s.real = (set[1]->s.real + b) / 2.0;
                    }
                }
                else
                    ERROR(E_PANIC,"Lost numerical stability");
            }
            else {
                DEBUG(1) fprintf(stderr, "@@@ take second (c)\n");
                newt->s.real = c;
            }
        }
        else {
            // b in range
            if (c < set[0]->s.real || c > set[2]->s.real) {
                DEBUG(1) fprintf(stderr, "@@@ take first (b)\n");
                newt->s.real = b;
            }
            else {
                // Both in range -- take the smallest mag
                if (a > 0.0) {
                    DEBUG(1) fprintf(stderr, "@@@ push -- first (b)\n");
                    newt->s.real = b;
                }
                else {
                    DEBUG(1) fprintf(stderr, "@@@ push -- first (b)\n");
                    newt->s.real = c;
                }
            }
        }

    }

    newt->s.imag = 0.0;

    return error;
}
