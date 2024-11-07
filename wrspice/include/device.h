
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
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef DEVICE_H
#define DEVICE_H


//
// Definitions for device library and interface
//

#include <stdlib.h>
#include "circuit.h"
#include "errors.h"

// We don't export config.h to the device library.
#if defined(__GNUC__)
#define WITH_CMP_GOTO
#endif

// This is a base class for the general source device.  This will give
// use efficient access to these fields in WRspice.
//
struct sGENSRCinstance : public sGENinstance
{
    int SRCposNode;
    int SRCnegNode;
    int SRCcontPosNode;
    int SRCcontNegNode;
    int SRCbranch;
    int SRCcontBranch;
    int SRCtype;            // GENSRC_I or GENSRC_V
    int SRCdep;             // 0 or GENSRC_CC or GENSRC_VC
    double SRCdcValue;      // DC and TRANSIENT value of source
};
// Don't change these, used in the src device enum and aski/seti tables.
enum { GENSRC_I = 1, GENSRC_V = 2 };
enum { GENSRC_CC = 1, GENSRC_VC = 2 };


// Number of noise state variables stored for each generator.  Device
// instance subclass needs this.
//
#define NSTATVARS 3


//
// Defines for the flags set for the device parameters.
//

// Mask for the standard flags defined in ifsim.h.
#define IF_STDFLAGS  0xffff

// Used by sensetivity analysis to categorize parameters.
//
#define IF_REDUNDANT 0x00010000
#define IF_PRINCIPAL 0x00020000
#define IF_AC        0x00040000
#define IF_AC_ONLY   0x00080000
#define IF_NONSENSE  0x00100000

#define IF_SETQUERY  0x00200000
#define IF_ORQUERY   0x00400000
#define IF_CHKQUERY  0x00800000

// IF_PRINCIPAL   Parameter which the principle value of a device (used
//                 for naming output variables in sensetivity).
// IF_AC          Parameter significant for time-varying (non-dc) analyses.
// IF_AC_ONLY     Parameter significant for ac analyses only.
// IF_REDUNDANT   Redundant parameter name (e.g. "vto" vs. "vt0").
// IF_NONSENSE    Parameter which is not used by sensetivity in any case.
//
// IF_SETQUERY    This parameter must be non-zero for sensetivity of
//                 following parameter (model params done first).
// IF_CHKQUERY    Prev. parameter must be non-zero for sensetivity.
// IF_ORQUERY     Like above, but or-ed with previous query value.


// These parameters will be added to plots if the "use all" flag is set.
//
#define IF_USEALL    0x01000000

// Macros used in device structure definitions.
//
#define IP(a,b,c,d)     IFparm(a, b, c|IF_SET, d)
#define OP(a,b,c,d)     IFparm(a, b, c|IF_ASK, d)
#define IO(a,b,c,d)     IFparm(a, b, c|IF_IO, d)


// Macro to make elements.
//
#define TSTALLOC(ptr,first,second) \
  if((inst->ptr=ckt->alloc(inst->first,inst->second))==0) { return(E_NOMEM); }

struct sDISTOAN;

typedef void(*NewDevFunc)(IFdevice**, int*);

// Device library interface.  This is logically part of the library,
// and provides some services to the device models.  It also provides
// some sepecialized interface functions to the application.
//
struct sDevLib
{
    // No constructor, call init before use.

    // Initialization.
    void    init();
    int     loadDev(NewDevFunc);
    bool    unloadDev(const IFdevice*);
    int     keyCode(int, int);

    // Functions used in the device library.
    void    parse(sCKT*, sLine*, int, int, int, bool, const char*);
    bool    checkVersion(const char*, const char*);
    double  limvds(double, double);
    double  pnjlim(double, double, double, double, int*);
    double  fetlim(double, double, double);
    double  limexp(double, double, int*);
    double  bcs_egapv(double, double=0.0, double=0.0);

    // Specialized interface functions called by application.
    bool    getMinMax(sCKT*, double*, double*);
    void    distoSet(sCKT*, int, sDISTOAN*);
    void    zeroInductorCurrent(sCKT*);

    double pred(sCKT *ckt, int which)
    {
        return (
            *(ckt->CKTstate1 + which)*ckt->CKTpred[0] +
            *(ckt->CKTstate2 + which)*ckt->CKTpred[1] );
            /*
            *(ckt->CKTstate3 + which)*ckt->CKTpred[2] +
            (ckt->CKTcurTask->TSKmaxOrder > 2 ?
                *(ckt->CKTstate4 + which) :
                *(ckt->CKTstate0 + which))*ckt->CKTpred[3] );
            */
    }

    const char *version()   { return (dl_version); }
    int     numdevs()       { return (dl_numdevs); }

    IFdevice *device(int i)
        {
            if (i >= 0 && i < dl_numdevs)
                return (dl_devices[i]);
            return (0);
        }

    static const char *def_param(const char *nm)
        {
            char key = *nm;
            if (isupper(key))
                key = tolower(key);
            switch (key) {
            case 'v':
            case 'i':
                return ("dc");
            case 'c':
                return ("c");
            case 'l':
                return ("l");
            case 'r':
                return ("r");
            }
            return (0);
        }

private:
    IFdevice **dl_devices;  // Device array.
    int dl_numdevs;         // Number of devices in array.
    int dl_size;            // Array size.

    static const char *dl_version;
};

extern sDevLib DEV;

// Error flag used by sDevOut::textOut, it tells the application the
// nature of the message.  The handling is application dependent.
//
enum OUTerrType
{
    OUT_NONE,
    OUT_INFO,
    OUT_WARNING,
    OUT_FATAL,
    OUT_PANIC
};

// Maximum number of open file descriptors.
#define OUT_MAX_FDS 32

struct wordlist;

// This interface is exported by the application to the device
// library, providing error text reporting services within the
// framework of the application.  The original device model code
// typically uses printf to record errors.  These calls can be
// #defined to functions in this interface.
//
struct sDevOut
{
    // List of strobe output strings.  Output is deferred until
    // strobeDump is called.
    //
    struct sStrobeOut
    {
        sStrobeOut(char *s, int f, int i)
            {
                string = s;
                fd = f;
                id = i;
                next = 0;
            }

        ~sStrobeOut()
            {
                delete [] string;
            }

        char *string;
        int fd;
        int id;
        sStrobeOut *next;
    };

    static void textOut(OUTerrType, const char*, ...);
        // This is a replacement for printf, that sends the text to
        // the application's error message window or log.

    void printModDev(sGENmodel*, sGENinstance*, bool* = 0);
        // This prints an error message header, giving the model and
        // instance name if the instance pointer is nonzero, or just
        // the model name otherwise.  The third argument is an
        // optional pointer to a bool, which is set if the pointer is
        // passed and a header created.  If passed and is set, the
        // function returns without creating the header.  The user can
        // use this to avoid duplicate headers.  It is expected that
        // an error or warning message will follow (using textOut).
        
    void pushMsg(const char*, ...);
        // This pushes a message on a queue.  It is for use with
        // checkMsg below.
        
    void checkMsg(sGENmodel*, sGENinstance*);
        // If there is a message in the queue, a header is created as
        // in printModDev, and the messages follow as from textOut
        // with OUT_WARNING.
        //
        // This and pushMsg can be used to deal with function calls
        // that emit warning messages, but the model/instance pointers
        // are not available in the function.  In the function, the
        // printfs are redefined to pushMsg calls.  The caller (which
        // has model/instance pointers) calls checkMsg when the
        // function returns.

    // The following implement Verilog-A input/output functions.
    void cleanup();
    int fopen(const char*, const char* = 0);
    void fclose(int);
    int fscanf(int, const char*, ...);
    int fseek(int, int, int);
    int ftell(int);
    void display(sGENmodel*, sGENinstance*, const char*, ...);
    void fdisplay(sGENmodel*, sGENinstance*, int, const char*, ...);
    void write(sGENmodel*, sGENinstance*, const char*, ...);
    void fwrite(sGENmodel*, sGENinstance*, int, const char*, ...);
    void monitor(sGENmodel*, sGENinstance*, int, const char*, ...);
    void fmonitor(sGENmodel*, sGENinstance*, int, int, const char*, ...);
    void strobe(sGENmodel*, sGENinstance*, int, const char*, ...);
    void fstrobe(sGENmodel*, sGENinstance*, int, int, const char*, ...);
    void dumpStrobe();
    void warning(sGENmodel*, sGENinstance*, const char*, ...);
    void error(sGENmodel*, sGENinstance*, const char*, ...);
    void fatal(sGENmodel*, sGENinstance*, double, int, const char*, ...);
    int finish(sGENmodel*, sGENinstance*, double, int);
    int stop(sGENmodel*, sGENinstance*, double, int);

private:
    wordlist *dvo_msgs;             // message queue
    sStrobeOut *dvo_strobes;        // strobe message list
    sStrobeOut *dvo_monitors;       // monitor message list
    int dvo_fds[OUT_MAX_FDS];       // open file descriptors
};

extern sDevOut DVO;

#endif // DEVICE_H

