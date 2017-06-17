
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2010 Whiteley Research Inc, all rights reserved.        *
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
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * Example WRspice IPC Application: spclient                              *
 *                                                                        *
 *========================================================================*
 $Id: spclient.h,v 1.4 2014/10/05 04:02:42 stevew Exp $
 *========================================================================*/

#ifndef SPCLIENT_H
#define SPCLIENT_H

#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lstring.h"
#include "sced_spiceipc.h"


// This file contains basic stuff that should be included in your custom
// application.

//-----------------------------------------------------------------------------
// The following are simplified Xic data classes and structs required
// by the spclient demo code.  Each is instantiated globally in the
// application and accessed via an access function.

struct sPL *PL();

// This manages the prompt line in Xic.
struct sPL
{
    friend sPL *PL() { return (_pl); }

    sPL() { _pl = this; }

    void ShowPrompt(const char *fmt, ...)
        {
            va_list args;
            va_start(args, fmt);
            vfprintf(stdout, fmt, args);
            putc('\n', stdout);
        }
    void ErasePrompt() { }

private:
    static sPL *_pl;
};

class cErrLog *Log();

class cErrLog
{
public:
    friend cErrLog *Log() { return (cErrLog::_el); }

    void ErrorLog(const char*, const char *s)   { fprintf(stderr, "%s\n", s); }
    void WarningLog(const char*, const char *s) { fprintf(stderr, "%s\n", s); }

private:
    static cErrLog *_el;
};

struct sSCD *SCD();

// This class contains electrical-mode interfaces in Xic, including
// the SpiceIPC interface.
struct sSCD
{
    friend sSCD *SCD() { return (_scd); }

    sSCD() { _scd = this; _ipc = new cSpiceIPC; }

#define SpBusy 0
#define SpError 0
#define SpDone 0
#define SpPause 0
#define SpNil 0
    void PopUpSim(int) { }
    cSpiceIPC *spif() { return (_ipc); }
    bool iplotStatusChanged() { return (false); }
    void setIplotStatusChanged(bool) { }
    bool doingIplot() { return (false); }
    char *getIplotCmd(bool) { return (0); }

private:
    static sSCD *_scd;
    static cSpiceIPC *_ipc;
};


struct DspPkgIf *dspPkgIf();

// This contains more utilities to support the graphical system.
struct DspPkgIf
{
    friend DspPkgIf *dspPkgIf() { return(_dsp_if); }

    DspPkgIf() { _dsp_if = this; }

    bool GetMainWinIdentifier(char*) { return (false); }
    bool GetDisplayString(const char *hostname, char *buf)
        {
            // Return a display string, making sure that it includes
            // the host name.
            const char *t = getenv("DISPLAY");
            if (t && *t) {
                if (*t == ':') {
                    if (!hostname)
                        return (false);
                    sprintf(buf, "%s%s", hostname, t);
                }
                else
                    strcpy(buf, t);
            }
            else {
                if (!hostname)
                    return (false);
                sprintf(buf, "%s:0", hostname);
            }
            return (true);
        }
    bool CheckScreenAccess(hostent*, const char*, const char*)
        {
            // User can write a function to check access permission of
            // local screen from remote machine.  Here we assume
            // permission, but this may fail.

            return (true);
        }
    void CloseGraphicsConnection() { }
    void SetWorking(bool) { }
    bool CheckForInterrupt() { return (false); }

private:
    static DspPkgIf *_dsp_if;
};


struct sMenu *Menu();

// This manages the Xic menus.
struct sMenu
{
    friend sMenu *Menu() { return (_menu); }

    sMenu() { _menu = this; }

#define MenuRUN 0
    void MenuButtonSet(const char*, const char*, bool) { }

private:
    static sMenu *_menu;
};


// This hack resolves some graphics calls.
#define DSPmainWbag(x)

#endif

