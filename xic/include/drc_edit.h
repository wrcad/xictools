
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

