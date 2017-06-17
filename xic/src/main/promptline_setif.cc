
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: promptline_setif.cc,v 5.6 2014/10/04 22:18:36 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "promptline.h"
#include "errorlog.h"
#include "tvals.h"


//-----------------------------------------------------------------------------
// DSP configuration

namespace {
    // Show a message.
    //
    void
    show_message(const char *msg, bool popup)
    {
        if (popup)
            Log()->ErrorLog("display", msg);
        else
            PL()->ShowPrompt(msg);
    }


    // Acknowledge an interrupt.
    //
    void
    notify_interrupted()
    {
        PL()->ShowPrompt("Redisplay interrupted.");
    }
}


void
cPromptLine::setupInterface()
{
    DSP()->Register_show_message(show_message);
    DSP()->Register_notify_interrupted(notify_interrupted);
}

