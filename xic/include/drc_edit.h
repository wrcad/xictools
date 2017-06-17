
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
 $Id: drc_edit.h,v 5.9 2014/12/12 03:56:05 stevew Exp $
 *========================================================================*/

#ifndef DRC_EDIT_H
#define DRC_EDIT_H

struct DRCtestDesc;
struct DRCtest;
struct sLspec;
struct stringlist;

// This is a base class containing the tookit-independent logic for
// the rules editor dialog.

// Argument to user_rule_mod()
//
enum Umode { Uinhibit, Uuninhibit, Udelete };

struct DRCedit
{
    DRCedit();
    ~DRCedit();

protected:
    stringlist *rule_list();
    DRCtestDesc *inhibit_selected();
    DRCtestDesc *remove_selected();
    void user_rule_mod(Umode);

    int ed_rule_selected;           // rule number selected or -1
    DRCtest *ed_usertest;           // removed user test
    DRCtestDesc *ed_last_delete;    // for undo
    DRCtestDesc *ed_last_insert;    // for undo

    static bool ed_text_input;  // true if prompting for text input
};

#endif

