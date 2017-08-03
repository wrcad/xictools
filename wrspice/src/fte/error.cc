
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

//
// Print out in more detail what a floating point error was.
//

#include "config.h"
#include "frontend.h"
#include "input.h"
#include "errors.h"
#include "kwords_fte.h"
#include "kwords_analysis.h"
#include <ctype.h>
#ifdef HAVE_SIGNAL
#include <signal.h>
#endif
#include "ttyio.h"

// for cMsgHdlr
#include "misc.h"
#include "toolbar.h"


#define nocurckt     "no current circuit"
#define badmatrix    "ill-formed matrix"
#define singular     "matrix is singular"
#define iterlim      "iteration limit reached"
#define order        "integration order not supported"
#define method       "integration method not supported"
#define timestep     "timestep too small"
#define xmissionline "transmission line in pz analysis"
#define magexceeded  "pole-zero magnitude too large"
#define shorted      "pole-zero input or output shorted"
#define inisout      "pole-zero input is output"
#define askpower     "ac powers cannot be ASKed"
#define nodundef     "node not defined in noise analysis"
#define noacinput    "no ac input source specified for noise"
#define nof2src      "no source at F2 for IM distortion analysis"
#define nodisto      "no distortion analysis - NODISTO defined"
#define nonoise      "no noise analysis - NONOISE defined"
#define mathdbz      "math error, divide by zero"
#define mathinv      "math error, invalid result"
#define mathovf      "math error, overflow"
#define mathunf      "math error, underflow"

namespace {
    const char *errmsg(int);
    const char *errmsg_short(int);
}


// Print the math error that was passed to a signal handller.
//
void
IFsimulator::FPexception(int c)
{
#ifdef WIN32
    (void)c;
    // Win32 exception reporting is done in the signal handler.
#else
    const char *msg;
    switch (c) {
    case FPE_INTOVF:  msg = "integer overflow";
        break;
    case FPE_INTDIV:  msg = "integer divide by zero";
        break;
    case FPE_FLTDIV:  msg = "floating point divide by zero";
        break;
    case FPE_FLTOVF:  msg = "floating point overflow";
        break;
    case FPE_FLTUND:  msg = "floating point underflow";
        break;
    case FPE_FLTRES:  msg = "floating point inexact result";
        break;
    case FPE_FLTINV:  msg = "invalid floating point operation";
        break;
    case FPE_FLTSUB:  msg = "subscript out of range";
        break;
    default:          msg = "unknown error";
    }
    if (ft_fpe_mode == FPEdebug)
        // We may be about to die, so print to the console.
        fprintf(stderr, "Warning: math error, %s.\n", msg);
    else
        GRpkgIf()->ErrPrintf(ET_ERROR, "math error, %s.\n", msg);
#endif
}


// Print a spice error message.
//
void
IFsimulator::Error(int code, const char *mess, const char *badone)
{
    char *s = IP.errMesg(code, badone);
    if (s) {
        if (isupper(*s))
            *s = tolower(*s);
        GRpkgIf()->ErrPrintf(ET_MSG, "%s: %s.\n", mess ? mess : "Error", s);
        delete [] s;
        return;
    }
    GRpkgIf()->ErrPrintf(ET_MSG,
        "%s: %s.\n", mess ? mess : "Error", errmsg(code));
}


const char *
IFsimulator::ErrorShort(int code)
{
    const char *s = IP.errMesgShort(code);
    if (!s)
        s = errmsg_short(code);
    if (!s)
        s = "(unknown)";
    return (s);
}


// Return true if the hspice option is set, in which case we will
// suppress warnings about unhandled HSPICE input, etc.
//
bool
IFsimulator::HspiceFriendly()
{
    return (GetVar(spkw_hspice, VTYP_BOOL, 0, CurCircuit()));

}


namespace {
    const char *errmsg(int code)
    {
        // simulator errors
        switch (code) {
        case E_NOCURCKT:
            return (nocurckt);
        case E_BADMATRIX:
            return (badmatrix);
        case E_SINGULAR:
            return (singular);
        case E_ITERLIM:
            return (iterlim);
        case E_ORDER:
            return (order);
        case E_METHOD:
            return (method);
        case E_TIMESTEP:
            return (timestep);
        case E_XMISSIONLINE:
            return (xmissionline);
        case E_MAGEXCEEDED:
            return (magexceeded);
        case E_SHORT:
            return (shorted);
        case E_INISOUT:
            return (inisout);
        case E_ASKPOWER:
            return (askpower);
        case E_NODUNDEF:
            return (nodundef);
        case E_NOACINPUT:
            return (noacinput);
        case E_NOF2SRC:
            return (nof2src);
        case E_NODISTO:
            return (nodisto);
        case E_NONOISE:
            return (nonoise);
        case E_MATHDBZ:
            return (mathdbz);
        case E_MATHINV:
            return (mathinv);
        case E_MATHOVF:
            return (mathovf);
        case E_MATHUNF:
            return (mathunf);
        }
        return ("unknown error");
    }


    const char *errmsg_short(int code)
    {
        // simulator errors
        switch (code) {
        case E_NOCURCKT:
            return ("(no circuit)");
        case E_BADMATRIX:
            return ("(bad matrix)");
        case E_SINGULAR:
            return ("(singular)");
        case E_ITERLIM:
            return ("(iter limit)");
        case E_ORDER:
            return ("(bad order)");
        case E_METHOD:
            return ("(bad method)");
        case E_TIMESTEP:
            return ("(bad timestep)");
        case E_XMISSIONLINE:
            return ("(transm line)");
        case E_MAGEXCEEDED:
            return ("(mag exceeded)");
        case E_SHORT:
            return ("(shorted)");
        case E_INISOUT:
            return ("(in is out)");
        case E_ASKPOWER:
            return ("(ask power)");
        case E_NODUNDEF:
            return ("(node undef)");
        case E_NOACINPUT:
            return ("(no ac input)");
        case E_NOF2SRC:
            return ("(no f2 src)");
        case E_NODISTO:
            return ("(no disto)");
        case E_NONOISE:
            return ("(no noise)");
        case E_MATHDBZ:
            return ("(divide by zero)");
        case E_MATHINV:
            return ("(invalid result)");
        case E_MATHOVF:
            return ("(overflow)");
        case E_MATHUNF:
            return ("(underflow)");
        }
        return (0);
    }
}


//-----------------------------------------------------------------------------
// Methods for the cMsgHdlr, the base class for the error window
// display.

#define MSG_FMT "%d) %s"


void
cMsgHdlr::first_message(const char *string)
{
    VTvalue vv;
    const char *errorlog = 0;
    if (Sp.GetVar(kw_errorlog, VTYP_STRING, &vv)) {
        errorlog = vv.get_string();
        if (!*errorlog)
            errorlog = ERR_FILENAME;
    }
    else if (Sp.GetVar(kw_errorlog, VTYP_BOOL, &vv))
        errorlog = ERR_FILENAME;

    if (errorlog)
        append_message(string);
}


void
cMsgHdlr::append_message(const char* string)
{
    // Add the message to the list and register an idle proc to
    // display the message (if necessary).  Check if the message
    // duplicates the previous message, and omit it if so, after
    // adding a "duplicate message" line.

    const char *MM_RECD = "...(duplicate messages(s) received)\n";

    if (!mh_list)
        mh_list = mh_list_end = new wordlist(string, 0);
    else {
        if (lstring::eq(string, mh_list_end->wl_word))
            string = MM_RECD;
        else if (lstring::eq(MM_RECD, mh_list_end->wl_word) &&
                mh_list_end->wl_prev &&
                lstring::eq(string, mh_list_end->wl_prev->wl_word))
            return;
        mh_list_end->wl_next = new wordlist(string, mh_list_end);
        mh_list_end = mh_list_end->wl_next;
    }
    if (!mh_proc_id)
        mh_proc_id = ToolBar()->RegisterIdleProc(mh_idle, this);
}


// Static function.
// Idle procedure to handle display of queued messages.
//
int
cMsgHdlr::mh_idle(void *arg)
{
    cMsgHdlr *mh = static_cast<cMsgHdlr*>(arg);

    VTvalue vv;
    const char *errorlog = 0;
    if (Sp.GetVar(kw_errorlog, VTYP_STRING, &vv)) {
        errorlog = vv.get_string();
        if (!*errorlog)
            errorlog = ERR_FILENAME;
    }
    else if (Sp.GetVar(kw_errorlog, VTYP_BOOL, &vv))
        errorlog = ERR_FILENAME;

    if (errorlog) {
        FILE *fp = fopen(errorlog, mh->mh_file_created ? "a" : "w");
        if (fp) {
            mh->mh_file_created = true;

            // Write messages to the file.
            for (wordlist *wl = mh->mh_list; wl; wl = wl->wl_next) {
                mh->mh_count++;
                fprintf(fp, MSG_FMT, mh->mh_count, wl->wl_word);
            }
            fclose(fp);
        }
    }

    // Count messages.
    int cnt = 0;
    for (wordlist *wl = mh->mh_list; wl; wl = wl->wl_next)
        cnt++;

    // Throw away excess messages.
    if (cnt > MAX_ERR_LINES) {
        int mcnt = cnt - MAX_ERR_LINES;
        cnt = 0;
        while (mh->mh_list) {
            wordlist *wl = mh->mh_list;
            mh->mh_list = mh->mh_list->wl_next;
            if (mh->mh_list)
                mh->mh_list->wl_prev = 0;
            delete [] wl->wl_word;
            delete wl;
            cnt++;
            if (cnt >= mcnt)
                break;
        }
    }

    // Display remaining messages.
    while (mh->mh_list) {
        wordlist *wl = mh->mh_list;
        mh->mh_list = mh->mh_list->wl_next;
        if (mh->mh_list)
            mh->mh_list->wl_prev = 0;
        mh->stuff_msg(wl->wl_word);
        delete [] wl->wl_word;
        delete wl;
    }
    mh->mh_list_end = 0;
    mh->mh_proc_id = 0;
    return (0);
}

