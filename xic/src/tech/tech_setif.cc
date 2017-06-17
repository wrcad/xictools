
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
 $Id: tech_setif.cc,v 1.5 2017/03/14 01:26:55 stevew Exp $
 *========================================================================*/

#include "dsp.h"
#include "tech.h"


//-----------------------------------------------------------------------------
// DSP configuration

namespace {
    // Hack for Xic, add saved comment lines when generating output for
    // a tech file.
    //
    static void
    comment_dump(FILE *techfp, const char *name, stringlist *alii)
    {
        Tech()->CommentDump(techfp, 0, tBlkNone, 0, name);
        for (stringlist *l = alii; l; l = l->next)
            Tech()->CommentDump(techfp, 0, tBlkNone, 0, l->string);
    }
}


void
cTech::setupInterface()
{
    DSP()->Register_comment_dump(comment_dump);
}

