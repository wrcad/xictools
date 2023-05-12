
/*========================================================================*
 *                                                                        *
 *  Distributed by Whiteley Research Inc., Sunnyvale, California, USA     *
 *                       http://wrcad.com                                 *
 *  Copyright (C) 2017 Whiteley Research Inc., all rights reserved.       *
 *  Author: Stephen R. Whiteley, except as indicated.                     *
 *                                                                        *
 *  As fully as possible recognizing licensing terms and conditions       *
 *  imposed by earlier work from which this work was derived, if any,     *
 *  this work is released under the Apache License, Version 2.0 (the      *
 *  "License").  You may not use this file except in compliance with      *
 *  the License, and compliance with inherited licenses which are         *
 *  specified in a sub-header below this one if applicable.  A copy       *
 *  of the License is provided with this distribution, or you may         *
 *  obtain a copy of the License at                                       *
 *                                                                        *
 *        http://www.apache.org/licenses/LICENSE-2.0                      *
 *                                                                        *
 *  See the License for the specific language governing permissions       *
 *  and limitations under the License.                                    *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL WHITELEY RESEARCH INCORPORATED      *
 *   OR STEPHEN R. WHITELEY BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER     *
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,      *
 *   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE       *
 *   USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                        *
 *========================================================================*
 *               XicTools Integrated Circuit Design System                *
 *                                                                        *
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#include "simulator.h"
#include "graph.h"
#include "output.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "parser.h"
#include "spnumber/spnumber.h"

#ifdef WIN32
extern double erfc(double);
#endif

//
// Code to do fourier transforms on data.  Note that we do
// interpolation to get a uniform grid.  Note that if polydegree is 0
// then no interpolation is done.
//


namespace {
// fourier() - perform fourier analysis of an output vector.

// Due to the construction of the program which places all the output
// data in the post-processor, the fourier analysis can not be done
// directly.  This function allows the post processor to hand back
// vectors of time and data values to have the fourier analysis
// performed on them.
//
// fourier(ndata,numFreq,thd,Time,Value,FundFreq,Freq,Mag,Phase,nMag,nPhase)
//         len   10      ?   inp  inp   inp      out  out out   out  out
//
void fourier(
  int ndata,       // number of entries in the Time and Value arrays
  int numFreq,     // number of harmonics to calculate
  double *thd,     // total harmonic distortion (percent) to be returned
  double *Time,    // times at which the voltage/current values were measured
  double *Value,   // voltage or current vector whose transform is desired
  double FundFreq, // the fundamental frequency of the analysis
  double *Freq,    // the frequency value of the various harmonics
  double *Mag,     // the Magnitude of the fourier transform
  double *Phase,   // the Phase of the fourier transform
  double *nMag,    // the normalized magnitude of the transform: nMag(fund)=1
  double *nPhase)  // the normalized phase of the transform: Nphase(fund)=0

  // note we can consider these as a set of arrays:  The sizes are:
  //  Time[ndata], Value[ndata]
  //  Freq[numFreq], Mag[numfreq], Phase[numfreq], nMag[numfreq],
  //  nPhase[numfreq]
  // The arrays must all be allocated by the caller.
  // The Time and Value array must be reasonably distributed over at
  // least one full period of the fundamental Frequency for the
  // fourier transform to be useful.  The function will take the
  // last period of the frequency as data for the transform.

  {
    // we are assuming that the caller has provided exactly one period
    // of the fundamental frequency.
    (void)Time;

    // clear output/computation arrays
    int i;
    for (i = 0; i < numFreq; i++) {
        Mag[i] = 0;
        Phase[i] = 0;
    }
    for (i = 0; i < ndata; i++) {
        for (int j = 0; j < numFreq; j++) {
            Mag[j]   += (Value[i]*sin(j*2.0*M_PI*i/((double)ndata)));
            Phase[j] += (Value[i]*cos(j*2.0*M_PI*i/((double)ndata)));
        }
    }

    Mag[0] = Phase[0]/ndata;
    Phase[0] = nMag[0] = nPhase[0] = Freq[0] = 0;
    *thd = 0;
    for (i = 1; i < numFreq; i++) {
        double tmp = Mag[i]*2.0/ndata;
        Phase[i] *= 2.0/ndata;
        Freq[i] = i * FundFreq;
        Mag[i] = sqrt(tmp*tmp + Phase[i]*Phase[i]);
        Phase[i] = atan2(Phase[i], tmp)*180.0/M_PI;
        nMag[i] = Mag[i]/Mag[1];
        nPhase[i] = Phase[i] - Phase[1];
        if (i > 1)
            *thd += nMag[i]*nMag[i];
    }
    *thd = 100*sqrt(*thd);
  }
}


void
CommandTab::com_fourier(wordlist *wl)
{
    char xbuf[20];
    sprintf(xbuf, "%1.1e", 0.0);
    int shift = strlen(xbuf) - 7;
    if (!OP.curPlot() || !OP.curPlot()->scale()) {
        GRpkg::self()->ErrPrintf(ET_ERROR, "no vectors loaded.\n");
        return;
    }

    VTvalue vv;
    int nfreqs = DEF_nfreqs;
    if (Sp.GetVar(kw_nfreqs, VTYP_NUM, &vv) && vv.get_int() > 0)
        nfreqs = vv.get_int();

    int polydegree = DEF_polydegree;
    if (Sp.GetVar(kw_polydegree, VTYP_NUM, &vv) && vv.get_int() >= 0)
        polydegree = vv.get_int();

    int fourgridsize = DEF_fourgridsize;
    if (Sp.GetVar(kw_fourgridsize, VTYP_NUM, &vv) && vv.get_int() > 0)
        fourgridsize = vv.get_int();

    sDataVec *time = OP.curPlot()->scale();
    if (!time->isreal()) {
        GRpkg::self()->ErrPrintf(ET_ERROR, "fourier needs real scale type.\n");
        return;
    }
    const char *s = wl->wl_word;
    double *ff;
    if (!(ff = SPnum.parse(&s, false)) || (*ff <= 0.0)) {
        GRpkg::self()->ErrPrintf(ET_ERROR, "bad fundamental frequency %s.\n",
            wl->wl_word);
        return;
    }
    double fundfreq = *ff;

    wl = wl->wl_next;
    pnlist *pl0 = Sp.GetPtree(wl, true);
    if (pl0 == 0)
        return;
    sDvList *dl0 = Sp.DvList(pl0);
    if (dl0 == 0)
        return;

    double dp[2];
    if (polydegree) {
        // Build the grid...
        time->minmax(dp, true);

        // Now get the last fund freq...
        double d = 1.0/fundfreq;   // The wavelength...
        if (dp[1] - dp[0] < d) {
            GRpkg::self()->ErrPrintf(ET_ERROR,
                "wavelength longer than time span.\n");
            return;
        }
        else if (dp[1] - dp[0] > d) {
            dp[0] = dp[1] - d;
        }
    }

    double *freq = new double[nfreqs];
    double *mag = new double[nfreqs];
    double *phase = new double[nfreqs];
    double *nmag = new double[nfreqs];
    double *nphase = new double[nfreqs];
    double *grid = 0, *stuff = 0;
    double *timescale;
    if (polydegree) {
        grid =  new double[fourgridsize];
        stuff =  new double[fourgridsize];

        double d = (dp[1] - dp[0])/fourgridsize;
        for (int i = 0; i < fourgridsize; i++)
            grid[i] = dp[0] + i*d;
        timescale = grid;
    }
    else
        timescale = time->realvec();

    sDvList *dl;
    for (dl = dl0; dl; dl = dl->dl_next) {
        sDataVec *vec = dl->dl_dvec;
        if (vec->length() != time->length()) {
            GRpkg::self()->ErrPrintf(ET_ERROR,
                "lengths don't match: %d, %d.\n",
                vec->length(), time->length());
            continue;
        }
        if (!vec->isreal()) {
            GRpkg::self()->ErrPrintf(ET_ERROR,
                "%s isn't real!.\n", vec->name());
            continue;
        }

        if (polydegree) {
            // Now interpolate the stuff...
            sPoly po(polydegree);
            if (!po.interp(vec->realvec(), stuff, time->realvec(),
                    vec->length(), grid, fourgridsize)) {
                GRpkg::self()->ErrPrintf(ET_ERROR, "can't interpolate.\n");
                break;
            }
        }
        else {
            fourgridsize = vec->length();
            stuff = vec->realvec();
        }

        double thd;
        fourier(fourgridsize, nfreqs, &thd, timescale,
                stuff, fundfreq, freq, mag, phase, nmag, nphase);

        TTY.printf("Fourier analysis for %s:\n", vec->name());
        TTY.printf( 
"  No. Harmonics: %d, THD: %g %%, Gridsize: %d, Interpolation Degree: %d\n\n",
            nfreqs,  thd, fourgridsize, polydegree);
        // Each field will have width CP.cp_numdgt + 6 (or 7
        // with HP-UX) + 1 if there is a - sign.
        //
        int fw = ((CP.NumDigits() > 0) ? CP.NumDigits() : 6) + 5 + shift;
        TTY.printf( "Harmonic %-*s   %-*s   %-*s   %-*s   %-*s\n",
                fw, "Frequency", fw, "Magnitude", 
                fw, "Phase", fw, "Norm. Mag",
                fw, "Norm. Phase");
        TTY.printf( "-------- %-*s   %-*s   %-*s   %-*s   %-*s\n",
                fw, "---------", fw, "---------",
                fw, "-----", fw, "---------",
                fw, "-----------");
        for (int i = 0; i < nfreqs; i++) {
            TTY.printf("%-4d    %-*s ", i, fw, SPnum.printnum(freq[i]));
            TTY.printf("%-*s ", fw, SPnum.printnum(mag[i]));
            TTY.printf("%-*s ", fw, SPnum.printnum(phase[i]));
            TTY.printf("%-*s ", fw, SPnum.printnum(nmag[i]));
            TTY.printf("%-*s\n", fw, SPnum.printnum(nphase[i]));
        }
        TTY.send("\n");
    }

    sDvList::destroy(dl0);
    delete [] freq;
    delete [] mag;
    delete [] phase;
    delete [] nmag;
    delete [] nphase;
    if (polydegree) {
        delete [] grid;
        delete [] stuff;
    }
}


/**********
Copyright 1994 Macquarie University, Sydney Australia.  All rights reserved.
Author:   1994 Anthony E. Parker, Department of Electronics, Macquarie Uni.
**********/

#define KEEPWINDOW

namespace {
    // Return an array representing the window function for the time
    // values passed, over the given span.
    //
    double *FFTwindow(int tlen, double *time, double span)
    {
        double *win = new double[tlen];
        double maxt = time[tlen-1];
        const char *window = kw_hanning;
        VTvalue vv;
        if (Sp.GetVar(kw_specwindow, VTYP_STRING, &vv)) 
            window = vv.get_string();

        if (lstring::eq(window, kw_none)) {
            for (int i = 0; i < tlen; i++)
                win[i] = 1.0;
        }
        else if (lstring::eq(window, kw_rectangular)) {
            for (int i = 0; i < tlen; i++) {
                if (maxt - time[i] > span)
                    win[i] = 0.0;
                else
                    win[i] = 1.0;
            }
        }
        else if (lstring::eq(window, kw_hanning) ||
                lstring::eq(window, kw_cosine)) {
            for (int i = 0; i < tlen; i++) {
                if (maxt - time[i] > span)
                    win[i] = 0.0;
                else
                    win[i] = 1.0 - cos(2.0*M_PI*(time[i] - maxt)/span);
            }
        }
        else if (lstring::eq(window, kw_hamming)) {
            for (int i = 0; i < tlen; i++) {
                if (maxt - time[i] > span)
                    win[i] = 0.0;
                else
                    win[i] =
                        1.0 - 0.92/1.08*cos(2.0*M_PI*(time[i] - maxt)/span);
            }
        }
        else if (lstring::eq(window, kw_triangle) ||
                lstring::eq(window, kw_bartlet)) {
            for (int i = 0; i < tlen; i++) {
                if (maxt - time[i] > span)
                    win[i] = 0.0;
                else
                    win[i] = 2.0 - fabs(2.0 + 4.0*(time[i] - maxt)/span);
            }
        }
        else if (lstring::eq(window, kw_blackman)) {
            /* Only order=2 supported.
            int order = DEF_specwindoworder;
            VTvalue vv1;
            if (Sp.GetVar(kw_specwindoworder, VTYP_NUM, &vv1) &&
                    vv1.get_int() >= DEF_specwindoworder_MIN &&
                    vv1.get_int() <= DEF_specwindoworder_MAX)
                order = vv1.get_int();
            */

            for (int i = 0; i < tlen; i++) {
                if (maxt - time[i] > span)
                    win[i] = 0.0;
                else {
                    win[i]  = 1.0;
                    win[i] -= 0.50/0.42*cos(2.0*M_PI*(time[i] - maxt)/span);
                    win[i] += 0.08/0.42*cos(4.0*M_PI*(time[i] - maxt)/span);
                }
            }
        }
        else if (lstring::eq(window, kw_gaussian)) {
            int order = 2;  // only order 2 supported here

            double scale =
                pow(2.0*M_PI/order, 0.5)*(0.5 - erfc(pow(order, 0.5)));
            for (int i = 0; i < tlen; i++) {
                if (maxt - time[i] > span)
                    win[i] = 0.0;
                else
                    win[i] = exp(-0.5*order*(1.0 - 2.0*(maxt - time[i])/span)*
                        (1.0 - 2.0*(maxt - time[i])/span))/scale;
            }
        }
        else {
            GRpkg::self()->ErrPrintf(ET_ERROR, "Unknown window type %s.\n",
                window);
            delete [] win;
            return (0);
        }
        return (win);
    }
}


//
// Code to do fourier transforms on transient analysis data.
//
void
CommandTab::com_spec(wordlist *wl)
{
    if (!OP.curPlot() || !OP.curPlot()->scale()) {
        GRpkg::self()->ErrPrintf(ET_ERROR, "Error: no vectors loaded.\n");
        return;
    }
    if (!OP.curPlot()->scale()->isreal() || 
            !(*OP.curPlot()->scale()->units() == UU_TIME)) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "Error: spec needs real time scale.\n");
        return;
    }

    double *freq;
    const char *s = wl->wl_word;
    int tlen = (OP.curPlot()->scale())->length();
    if (!(freq = SPnum.parse(&s, false)) || (*freq < 0.0)) {
        GRpkg::self()->ErrPrintf(ET_ERROR, "Error: bad start freq %s.\n",
            wl->wl_word);
        return;
    }
    double startf = *freq;

    wl = wl->wl_next;
    s = wl->wl_word;
    if (!(freq = SPnum.parse(&s, false)) || (*freq <= startf)) {
        GRpkg::self()->ErrPrintf(ET_ERROR, "Error: bad stop freq %s.\n",
            wl->wl_word);
        return;
    }
    double stopf = *freq;

    wl = wl->wl_next;
    s = wl->wl_word;
    if (!(freq = SPnum.parse(&s, false)) || !(*freq <= (stopf-startf))) {
        GRpkg::self()->ErrPrintf(ET_ERROR, "Error: bad step freq. %s\n",
            wl->wl_word);
        return;
    }
    double stepf = *freq;

    wl = wl->wl_next;
    double *time = OP.curPlot()->scale()->realvec();
    double span = time[tlen-1] - time[0];
    if (stopf > 0.5*tlen/span) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "Error: nyquist limit exceeded, try stop freq less than %e Hz.\n",
            tlen/2/span);
        return;
    }

    int fpts;
    span = ((int)(span*stepf*1.000000000001))/stepf;
    if (span > 0) {
        startf = (int)(startf/stepf*1.000000000001) * stepf;
        fpts = (int)((stopf - startf)/stepf) + 1;
        if (stopf > startf + (fpts-1)*stepf)
            fpts++;
    }
    else {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "Error: time span limits step freq to %1.1e Hz.\n",
            1/(time[tlen-1] - time[0]));
        return;
    }
    double *win = FFTwindow(tlen, time, span);
    if (!win)
        return;

    int ngood = 0;
    sDvList *dl0 = 0, *dl = 0;
    pnlist *pl0 = Sp.GetPtree(wl, true);
    for (pnlist *pl = pl0; pl; pl = pl->next()) {
        sDataVec *vec = Sp.Evaluate(pl->node());
        if (!vec)
            continue;
        if (vec->link()) {
            for (sDvList *dll = vec->link(); dll; dll = dll->dl_next) {
                vec = dll->dl_dvec;
                if (vec->length() != tlen) {
                    GRpkg::self()->ErrPrintf(ET_ERROR,
                        "Error: lengths don't match: %d, %d\n",
                        vec->length(), tlen);
                    continue;
                }
                if (!vec->isreal()) {
                    GRpkg::self()->ErrPrintf(ET_ERROR,
                        "Error: %s isn't real!\n", vec->name());
                    continue;
                }
                if (*vec->units() == UU_TIME)
                    continue;
                if (!dl0)
                    dl0 = dl = new sDvList;
                else {
                    dl->dl_next = new sDvList;
                    dl = dl->dl_next;
                }
                dl->dl_dvec = vec;
                ngood++;
            }
        }
        else {
            if (vec->length() != tlen) {
                GRpkg::self()->ErrPrintf(ET_ERROR,
                    "Error: lengths don't match: %d, %d\n",
                    vec->length(), tlen);
                continue;
            }
            if (!vec->isreal()) {
                GRpkg::self()->ErrPrintf(ET_ERROR, "Error: %s isn't real!\n", 
                    vec->name());
                continue;
            }
            if (*vec->units() == UU_TIME)
                continue;
            if (!dl0)
                dl0 = dl = new sDvList;
            else {
                dl->dl_next = new sDvList;
                dl = dl->dl_next;
            }
            dl->dl_dvec = vec;
            ngood++;
        }
    }
    pnlist::destroy(pl0);

    if (!ngood) {
       delete [] win;
       return;
    }
 
    sPlot *pl = new sPlot("spectrum");
    pl->new_plot();
    pl->set_title(pl->next_plot()->title());
    OP.setCurPlot(pl);

    freq = new double[fpts];
    sDataVec *f = new sDataVec(UU_FREQUENCY);
    f->set_name("frequency");
    f->set_flags(0);
    f->set_length(fpts);
    f->set_realvec(freq);
    f->newperm();

    double **tdvec = new double*[ngood];
    complex **fdvec = new complex*[ngood];
    {
        int i = 0;
        for (dl = dl0; dl; dl = dl->dl_next, i++) {
            tdvec[i] = dl->dl_dvec->realvec();
            fdvec[i] = new complex[fpts];
            f = new sDataVec(*dl->dl_dvec->units());
            char *tmpname = dl->dl_dvec->basename();
            f->set_name(tmpname);
            delete [] tmpname;
            f->set_flags(VF_COMPLEX);
            f->set_length(fpts);
            f->set_compvec(fdvec[i]);
            f->newperm();
        }
    }
    sDvList::destroy(dl0);

    double *dc = new double[ngood];
    for (int i = 0; i < ngood; i++)
        dc[i] = 0;
    for (int k = 1; k < tlen; k++) {
        double amp = win[k]/(tlen-1);
        for (int i = 0; i < ngood; i++)
            dc[i] += tdvec[i][k]*amp;
    }

    bool trace = Sp.GetVar(kw_spectrace, VTYP_BOOL, 0);

    for (int j = (startf == 0.0 ? 1 : 0); j < fpts; j++) {
        freq[j] = startf + j*stepf;
        if (trace)
            TTY.printf("spec: %e Hz: \r", freq[j]);
        for (int i = 0; i < ngood; i++) {
            fdvec[i][j].real = 0.0; 
            fdvec[i][j].imag = 0.0;
        }
        for (int k = 1; k < tlen; k++) {
            double amp = 2*win[k]/(tlen-1);
            double rad = 2*M_PI*time[k]*freq[j];
            double cosa = amp*cos(rad);
            double sina = amp*sin(rad);
            for (int i = 0; i < ngood; i++) {
                double value = tdvec[i][k] - dc[i];
                fdvec[i][j].real += value*cosa;
                fdvec[i][j].imag += value*sina;
            }
        }
    }
    if (startf == 0.0) {
        freq[0] = 0.0;
        for (int i = 0; i < ngood; i++) {
            fdvec[i][0].real = dc[i]; 
            fdvec[i][0].imag = 0.0;
        }
    }
    if (trace)
        TTY.printf("                           \r");
    delete [] dc;
    delete [] tdvec;
    delete [] fdvec;

#ifdef KEEPWINDOW
    f = new sDataVec(UU_NOTYPE);
    f->set_name("window");
    f->set_flags(0);
    f->set_length(tlen);
    f->set_realvec(win);
    f->newperm();
#else
    delete [] win;
#endif
}

