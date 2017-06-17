
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
 $Id: tech_kwords.cc,v 1.2 2017/03/14 01:26:55 stevew Exp $
 *========================================================================*/

#include "dsp.h"
#include "tech.h"
#include "tech_kwords.h"

//
// Keyword database for the tech package.
//


// Undo the last operation.
//
void
tcKWstruct::undo_keyword_change()
{
    if (kw_newstr) {
        stringlist *lp = 0;
        for (stringlist *l = kw_list; l; l = l->next) {
            if (l->string == kw_newstr) {
                if (lp)
                    lp->next = l->next;
                else
                    kw_list = l->next;
                delete [] l->string;
                delete l;
                break;
            }
            lp = l;
        }
        kw_newstr = 0;
    }
    stringlist *l = kw_list;
    if (l) {
        while (l->next)
            l = l->next;
        l->next = kw_undolist;
    }
    else
        kw_list = kw_undolist;
    kw_undolist = 0;
}


// Free and clear the undo list.
//
void
tcKWstruct::clear_undo_list()
{
    kw_undolist->free();
    kw_undolist = 0;
    kw_newstr = 0;
}

