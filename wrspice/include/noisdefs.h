
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
 $Id: noisdefs.h,v 2.50 2015/08/07 04:03:51 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Gary W. Ng
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef NOISDEFS_H
#define NOISDEFS_H

#include "acdefs.h"


//
// Structure used to carry information between subparts
// of the noise analysis code.
//

struct sNdata
{
    sNdata()
        {
            freq = 0.0;
            lstFreq = 0.0;
            delFreq = 0.0;
            outNoiz = 0.0;
            inNoise = 0.0;
            GainSqInv = 0.0;
            lnGainInv = 0.0;
            lnFreq = 0.0;
            lnLastFreq = 0.0;
            delLnFreq = 0.0;
            outNumber = 0;
            numPlots = 0;

            prtSummary = 0;
            outpVector = 0;
            namelist = 0;
            gainLimit = false;
        }

    int noise(sCKT*, int, int);
    double integrate(double, double, double);

    double freq;
    double lstFreq;
    double delFreq;
    double outNoiz;   // integrated output noise as of last frequency point
    double inNoise;   // integrated input noise as of last frequency point
    double GainSqInv;
    double lnGainInv;
    double lnFreq;
    double lnLastFreq;
    double delLnFreq;
    int outNumber;    // keeps track of the current output variable
    int numPlots;     // keeps track of the number of active plots so we can
                      //  close them in loop
    unsigned int prtSummary;
    double *outpVector;  // pointer to our array of noise outputs
    IFuid *namelist;     // list of plot names
    bool gainLimit;      // Gain limited by N_MINGAIN
};

// Structure used to describe a noise analysis.
//
struct sNOISEAN : public sACAN
{
    sNOISEAN()
        {
            Ioutdata = 0;
            Irun = 0;
            Noutput = 0;
            NoutputRef = 0;
            Ninput = 0;
            NsavOnoise = 0;
            NsavInoise = 0;
            NStpsSm = 0;
            NdataPtr = 0;
            IdataPtr = 0;
            NposOutNode = 0;
            NnegOutNode = 0;
            Nstep = 0;
        }

    ~sNOISEAN()
        {
            if (IdataPtr) {
                delete [] IdataPtr->namelist;
                delete [] IdataPtr->outpVector;
                delete IdataPtr;
            }
            if (NdataPtr) {
                delete [] NdataPtr->namelist;
                delete [] NdataPtr->outpVector;
                delete NdataPtr;
            }
            delete Ioutdata;
        }

    sJOB *dup()  { return (0); } // XXX fixme

    sOUTdata *Ioutdata; // integrated noise output data struct
    sRunDesc *Irun;     // run descriptor for integrated noise
    const char *Noutput;    // name of the noise output summation node
    const char *NoutputRef; // name of the noise output reference node
    const char *Ninput;     // name of AC source used to input ref. the noise
    double NsavOnoise;  // integrated output noise when we left last time
    double NsavInoise;  // integrated input noise when we left last time
    int NStpsSm; // number of steps before we output a noise summary report
    sNdata *NdataPtr;   // substruct for density calculation
    sNdata *IdataPtr;   // substruct for integrated calculation
    int NposOutNode;    // node numbers for output
    int NnegOutNode;
    int Nstep;          // step count, for summary print
};

struct NOISEanalysis : public IFanalysis
{
    // noisetp.cc
    NOISEanalysis();
    int setParm(sJOB*, int, IFdata*);

    // noiaskq.cc
    int askQuest(const sCKT*, const sJOB*, int, IFdata*) const;

    // noiprse.cc
    int parse(sLine*, sCKT*, int, const char**, sTASK*);

    // noian.cc
    int anFunc(sCKT*, int);

    sJOB *newAnal() { return (new sNOISEAN); }

private:
    static int noi_operation(sCKT*, int);
    static int noi_dcoperation(sCKT*, int);
};

extern NOISEanalysis NOISEinfo;

// Codes for saving and retrieving integrated noise data.

#define LNLSTDENS 0 // Array loc that log of the last noise density stored.
#define OUTNOIZ   1 // Array loc that integrated output noise is stored.
#define INNOIZ    2 // Array loc that integrated input noise is stored.

#ifndef NSTATVARS
#define NSTATVARS 3 // Number of "state" variables that must be stored for
                    // each noise generator.
                    // In this case it is three: LNLSTDENS, OUTNOIZ
                    // and INNOIZ.
#endif

// Noise analysis parameters.
//
#define N_OUTPUT     1
#define N_OUTREF     2
#define N_INPUT      3
#define N_PTSPERSUM  4


// Noise routine operations/modes.
//
#define N_DENS       1
#define INT_NOIZ     2
#define N_OPEN       1
#define N_CALC       2
#define N_CLOSE      3
#define SHOTNOISE    1
#define THERMNOISE   2
#define N_GAIN       3


// Tolerances and limits to make numerical analysis more robust.

#define N_MINLOG   1E-38
//    The smallest number we can take the log of.


#define N_MINGAIN  1E-20
//    The smallest input-output gain we can tolerate
//    (to calculate input-referred noise we divide
//    the output noise by the gain).

#define N_INTFTHRESH 1E-10
//    The largest slope (of a log-log noise spectral
//    density vs. freq plot) at which the noise
//    spectum is still considered flat. (no need for
//    log curve fitting).

#define N_INTUSELOG 1E-10
//    Decides which expression to use for the integral of
//    x**k.  If k is -1, then we must use a 'ln' form.
//    Otherwise, we use a 'power' form.  This
//    parameter is the region around (k=) -1 for which
//    use the 'ln' form.

// Maximum length for noise output variables we will generate.
#define N_MXVLNTH  64

// Export for device library.
extern void NevalSrc(double*, double*, sCKT*, int, int, int, double,
    double = 1.0);

#endif // NOISDEFS_H

