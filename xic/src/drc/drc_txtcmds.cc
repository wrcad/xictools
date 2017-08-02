
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "main.h"
#include "drc.h"
#include "dsp_inlines.h"
#include "promptline.h"


//-----------------------------------------------------------------------------
// The DRC 'bang' commands

namespace {
    namespace drc_bangcmds {

        // DRC
        void showz(const char*);
        void errs(const char*);
        void errlayer(const char*);
    }
}


void
cDRC::setupBangCmds()
{
    // DRC
    XM()->RegisterBangCmd("showz", &drc_bangcmds::showz);
    XM()->RegisterBangCmd("errs", &drc_bangcmds::errs);
    XM()->RegisterBangCmd("errlayer", &drc_bangcmds::errlayer);
}


//-----------------------------------------------------------------------------
// DRC

void
drc_bangcmds::showz(const char *s)
{
    if (!*s)
        DRC()->setShowZoids(!DRC()->isShowZoids());
    else if (*s == 'y' || *s == 'Y')
        DRC()->setShowZoids(true);
    else
        DRC()->setShowZoids(false);
    PL()->ErasePrompt();
}


void
drc_bangcmds::errs(const char*)
{
    PL()->ErasePrompt();
    if (DSP()->CurCellName())
        DRC()->setErrlist();
}


void
drc_bangcmds::errlayer(const char *s)
{
    int n = 0;
    char *t = lstring::gettok(&s);
    if (!t) {
        PL()->ShowPrompt(
            "Usage:  !errlayer layer_name [prpty_number]");
        return;
    }
    if (*s && isdigit(*s))
        n = atoi(s);
    bool ret = DRC()->errLayer(t, n);
    delete [] t;
    if (!ret)
        PL()->ShowPrompt("Command failed: internal error");
    else
        PL()->ShowPrompt("Done");
}

