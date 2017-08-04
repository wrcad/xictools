
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "main.h"
#include "sced.h"
#include "sced_spiceipc.h"
#include "sced_param.h"
#include "edit.h"
#include "extif.h"
#include "dsp_inlines.h"
#include "events.h"
#include "promptline.h"
#include "errorlog.h"
#include "undolist.h"
#ifdef WIN32
#include <winsock2.h>
#else
#include <arpa/inet.h>
#endif


//-----------------------------------------------------------------------------
// SCED 'bang" commands

namespace {
    namespace sced_bangcmds {

        // Electrical
        void calc(const char*);
        void check(const char*);
        void regen(const char*);
        void devkeys(const char*);

        // WRspice Interface
        void spcmd(const char*);
    }
}


void
cSced::setupBangCmds()
{
    // Electrical
    XM()->RegisterBangCmd("calc", &sced_bangcmds::calc);
    XM()->RegisterBangCmd("check", &sced_bangcmds::check);
    XM()->RegisterBangCmd("regen", &sced_bangcmds::regen);
    XM()->RegisterBangCmd("devkeys", &sced_bangcmds::devkeys);

    // WRspice Interface
    XM()->RegisterBangCmd("spcmd", &sced_bangcmds::spcmd);
}


//-----------------------------------------------------------------------------
// Electrical

void
sced_bangcmds::calc(const char *s)
{
    if (!s || !*s) {
        PL()->ShowPrompt("Usage:  !calc expression");
        return;
    }

    CDs *sde = CurCell(Electrical);
    if (!sde) {
        PL()->ShowPrompt("No electrical current cell!");
        return;
    }

    char *str = new char[strlen(s) + 3];
    char *e = str;
    if (*s != '\'')
        *e++ = '\'';
    e = lstring::stpcpy(e, s);
    if (*s != '\'')
        *e++ = '\'';
    *e = 0;

    cParamCx pcx(sde);
    sLstr lstr;
    lstr.add(s);
    lstr.add(" = ");
    pcx.update(&str);
    lstr.add(str);
    delete [] str;
    PL()->ShowPrompt(lstr.string());
}


void
sced_bangcmds::check(const char*)
{
    PL()->ErasePrompt();
    CDs *cursde = CurCell(Electrical);
    if (cursde)
        SCD()->prptyCheck(cursde, stderr, true);
}


void
sced_bangcmds::regen(const char*)
{
    PL()->ErasePrompt();
    CDs *cursde = CurCell(Electrical);
    if (!cursde)
        return;
    EV()->InitCallback();
    Ulist()->ListCheck("regen", cursde, false);
    if (SCD()->prptyRegenCell()) {
        Ulist()->CommitChanges(true);
        PL()->ErasePrompt();
    }
    else
        PL()->ShowPrompt("Done, no changes.");
}


void
sced_bangcmds::devkeys(const char*)
{
    SCD()->dumpDevKeys();
}


//-----------------------------------------------------------------------------
// WRspice Interface

// In binary data returns, integers and doubles are sent in "network
// byte order", which may not be the same as the machine order.  Most
// C libraries have htonl, ntohl, etc.  functions for dealing with
// this, however there does not appear to be support for float/double. 
// The function below does the trick for doubles, both to and from
// network byte order.  It simply reverses byte order if machine order
// does not match network byte order.

// If your application will always run WRspice on the same machine, or
// the same type of machine (e.g., Intel CPU), then you don't need to
// worry about byte order.  In particular, Intel x86 will reorder
// bytes, which has some overhead.  If all machines are Intel, then
// this overhead can be eliminated by skipping the byte ordering
// functions and read/write the values directly.

namespace {
    // Reverse the byte order if the MSB's are at the top address, i.e.,
    // switch to/from "network byte order".  This will reverse bytes on
    // Intel x86, but is a no-op on Sun SPARC (for example).
    // 
    // IEEE floating point is assumed here!
    //
    double net_byte_reorder(double d)
    {
        static double d1p0 = 1.0;

        if ( ((unsigned char*)&d1p0)[7] ) {
            // This means MSB's are at top address, reverse bytes.
            double dr;
            unsigned char *t = (unsigned char*)&dr;
            unsigned char *s = (unsigned char*)&d;
            t[0] = s[7];
            t[1] = s[6];
            t[2] = s[5];
            t[3] = s[4];
            t[4] = s[3];
            t[5] = s[2];
            t[6] = s[1];
            t[7] = s[0];
            return (dr);
        }
        else
            return (d);
    }
}


void
sced_bangcmds::spcmd(const char *s)
{
    // Run the command.
    char *retbuf;           // Message returned.
    char *outbuf;           // Stdout/stderr returned.
    char *errbuf;           // Error message.
    unsigned char *databuf; // Command data.
    if (!SCD()->spif()->DoCmd(s, &retbuf, &outbuf, &errbuf, &databuf)) {
        // No connection to simulator.
        if (retbuf) {
            PL()->ShowPrompt(retbuf);
            delete [] retbuf;
        }
        return;
    }
    if (retbuf) {
        PL()->ShowPrompt(retbuf);
        delete [] retbuf;
    }
    if (outbuf) {
        fputs(outbuf, stdout);
        delete [] outbuf;
    }
    if (errbuf) {
        Log()->ErrorLog("!spcmd", errbuf);
        delete [] errbuf;
    }
    if (databuf) {
        // This is returned only by the eval command at present. 
        // The format is:
        //
        // databuf[0]      'o'
        // databuf[1]      'k'
        // databuf[2]      'd'  (datatype double, other types may be
        //                       added in future)
        // databuf[3]      'r' or 'c' (real or complex)
        // databuf[4-7]    array size, network byte order
        // ...             array of data values, network byte order

        printf("\n");
        if (databuf[0] != 'o' || databuf[1] != 'k' || databuf[2] != 'd') {
            // error (shouldn't happen)
            delete [] databuf;
            return;
        }

        // We'll just print the first 10 values of the return.
        unsigned int size = ntohl(*(int*)(databuf+4));
        double *dp = (double*)(databuf + 8);
        for (unsigned int i = 0; i < size; i++) {
            if (i == 10) {
                printf("...\n");
                break;
            }
            if (databuf[3] == 'r')      // real values
                printf("%d   %g\n", i, net_byte_reorder(dp[i]));
            else if (databuf[3] == 'c') //complex values
                printf("%d   %g,%g\n", i, net_byte_reorder(dp[2*i]),
                    net_byte_reorder(dp[2*i + 1]));
            else
                // wacky error, can't happen
                break;
        }
        delete [] databuf;
    }
}

