
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
 $Id: breakpt.cc,v 2.3 2016/07/27 04:41:43 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#include "circuit.h"


//
// Breakpoint/lattice point functions (sCKTlattice).
//

// Delete the first time from the breakpoint table for the given circuit.
//
void
sCKTlattice::clear_break(double ckt_time, double minbrk, double *ckt_breaks)
{
    *ckt_breaks = nextbreak(*ckt_breaks, minbrk);
    *(ckt_breaks+1) = nextbreak(*ckt_breaks, minbrk);
    int i;
    for (i = 0; i < numBreaks; i++)
        if (breaks[i] > ckt_time)
            break;
    if (i) {
        numBreaks -= i;
        for (int j = 0; j < numBreaks; j++)
            breaks[j] = breaks[j+i];
    }
}


// Add the given time to the breakpoint table for the given circuit,
// return true if breakpoint actually added.
//
bool
sCKTlattice::set_break(double time, double ckt_time, double minbrk,
    double *ckt_breaks)
{
    if (lattices) {
        for (int i = 0; i < numLattices; i++) {
            double dt0, dt1;
            if (time < lattices[i].offs) {
                dt0 = lattices[i].offs + lattices[i].per;
                dt1 = lattices[i].offs;;
            }
            else{
                dt0 = lattices[i].per *
                    ceil((time - lattices[i].offs)/lattices[i].per) +
                    lattices[i].offs;
                dt1 = lattices[i].per *
                    floor((time - lattices[i].offs)/lattices[i].per) +
                    lattices[i].offs;
            }
            if (fabs(time - dt0) < minbrk ||
                    fabs(time - dt1) < minbrk) {
                if (time < *ckt_breaks)
                    *ckt_breaks = nextbreak(ckt_time, minbrk);
                *(ckt_breaks+1) = nextbreak(*ckt_breaks, minbrk);
                return (false);
            }
        }
    }
    if (!breaks) {
        breaks = new double[1];
        breaks[0] = time;
        numBreaks = 1;
        return (false);
    }
    for (int i = 0; i < numBreaks; i++) {
        if (breaks[i] > time) { // passed
            if ((breaks[i] - time) <= minbrk) {
                // Very close together - take earlier point.
                breaks[i] = time;
                if (time < *ckt_breaks)
                    *ckt_breaks = nextbreak(ckt_time, minbrk);
                *(ckt_breaks+1) = nextbreak(*ckt_breaks, minbrk);
                return (true);
            }
            if (time - breaks[i-1] <= minbrk) {
                // very close together, but after, so skip
                if (time < *ckt_breaks)
                    *ckt_breaks = nextbreak(ckt_time, minbrk);
                *(ckt_breaks+1) = nextbreak(*ckt_breaks, minbrk);
                return (true);
            }
            // Fits in middle - new array & insert.
            double *tmp = new double[numBreaks + 1];
            int j;
            for (j = 0; j < i; j++)
                *(tmp + j) = breaks[j];
            *(tmp + i) = time;
            for (j = i; j < numBreaks; j++) {
                *(tmp + j+1) = breaks[j];
            }
            delete [] breaks;;
            numBreaks++;
            breaks = tmp;
            if (time < *ckt_breaks)
                *ckt_breaks = nextbreak(ckt_time, minbrk);
            *(ckt_breaks+1) = nextbreak(*ckt_breaks, minbrk);
            return (true);
        }
    }
    // never found it - beyond end of time - extend
    if (time - breaks[numBreaks-1] <= minbrk) {
        // very close together - keep earlier, throw out new point
        if (time < *ckt_breaks)
            *ckt_breaks = nextbreak(ckt_time, minbrk);
        *(ckt_breaks+1) = nextbreak(*ckt_breaks, minbrk);
        return (true);
    }
    // Fits at end - grow array & add.
    Realloc(&breaks, numBreaks+1, numBreaks);
    breaks[numBreaks] = time;
    numBreaks++;
    if (time < *ckt_breaks)
        *ckt_breaks = nextbreak(ckt_time, minbrk);
    *(ckt_breaks+1) = nextbreak(*ckt_breaks, minbrk);
    return (true);
}


// Set up a periodic breakpoint.
//
void
sCKTlattice::set_lattice(double offs, double per)
{
    if (lattices) {
        for (int i = 0; i < numLattices; i++) {
            if (offs == lattices[i].offs && per == lattices[i].per)
                return;
        }
        Realloc(&lattices, numLattices+1, numLattices);
        lattices[numLattices].offs = offs;
        lattices[numLattices].per = per;
        numLattices++;
    }
    else {
        lattices = new lattice[1];
        lattices[0].offs = offs;
        lattices[0].per = per;
        numLattices = 1;
    }
}


// Return the next breakpoint following t.
//
double
sCKTlattice::nextbreak(double t, double minbrk)
{
    double dt0 = 0.0;
    int i;
    if (lattices) {
        for (i = 0; i < numLattices; i++) {
            double dt1 = lattices[i].offs;
            if (t > lattices[i].offs) {
                dt1 += lattices[i].per *
                    ceil((t - lattices[i].offs)/lattices[i].per);
            }
            if (fabs(dt1 - t) < minbrk)
                dt1 += lattices[i].per;
            if (dt0 == 0.0 || dt1 < dt0)
                dt0 = dt1;
        }
    }
    for (i = 0; i < numBreaks; i++) {
        if (breaks[i] > t)
            break;
    }
    while (i < numBreaks && fabs(t - breaks[i]) < minbrk)
        i++;
    if (i == numBreaks)
        return (dt0);
    if (dt0 > 0.0 && dt0 < breaks[i])
        return (dt0);
    return (breaks[i]);
}

