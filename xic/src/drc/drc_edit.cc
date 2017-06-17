
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
 $Id: drc_edit.cc,v 5.23 2017/03/14 01:26:30 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "drc.h"
#include "drc_edit.h"
#include "dsp_layer.h"
#include "layertab.h"
#include "tech_layer.h"
#include "events.h"
#include "promptline.h"
#include "tech.h"
#include "cd_lgen.h"


bool DRCedit::ed_text_input = false;

// This is a base class containing the tookit-independent logic for
// the rules editor dialog.

DRCedit::DRCedit()
{
    ed_rule_selected = -1;
    ed_usertest = 0;
    ed_last_delete = 0;
    ed_last_insert = 0;
    ed_text_input = false;
}

DRCedit::~DRCedit()
{
    user_rule_mod(Udelete);
    delete ed_usertest;
}


// Return a list of design rules for the current layer.
//
stringlist *
DRCedit::rule_list()
{
    char *str = 0;
    if (LT()->CurLayer()) {
        sLstr lstr;
        DRC()->techPrintLayer(0, &lstr, false, LT()->CurLayer(), true, false);
        str = lstr.string_trim();
    }
    if (!str)
        return (0);
    stringlist *s0 = 0, *se = 0;
    char *s = str;
    do {
        char *t = strchr(s, '\n');
        while (t && *(t-1) == '\\')
            t = strchr(t+1, '\n');
        if (t)
            *t++ = 0;
        if (!s0)
            s0 = se = new stringlist(lstring::copy(s), 0);
        else {
            se->next = new stringlist(lstring::copy(s), 0);
            se = se->next;
        }
        s = t;
    } while (s && *s);
    delete [] str;
    return (s0);
}


// Make the highlighted rule inactive or active.
//
DRCtestDesc *
DRCedit::inhibit_selected()
{
    if (!LT()->CurLayer())
        return (0);
    DRCtestDesc *td = *tech_prm(LT()->CurLayer())->rules_addr();
    if (ed_rule_selected == 0) {
        if (!td)
            return (0);
        td->setInhibited(!td->inhibited());
        return (td);
    }
    for (int i = ed_rule_selected - 1; i && td; i--, td = td->next()) ;
    if (!td || !td->next())
        return (0);
    DRCtestDesc *dl = td->next();
    dl->setInhibited(!dl->inhibited());
    return (dl);
}


// Remove the highlighted rule from the list.
//
DRCtestDesc *
DRCedit::remove_selected()
{
    if (!LT()->CurLayer())
        return (0);
    DRCtestDesc *td = *tech_prm(LT()->CurLayer())->rules_addr();
    if (ed_rule_selected == 0) {
        if (!td)
            return (0);
        return (DRC()->unlinkRule(td));
    }
    for (int i = ed_rule_selected - 1; i && td; i--, td = td->next()) ;
    if (!td || !td->next())
        return (0);
    return (DRC()->unlinkRule(td->next()));
}


// When a user defined rule template is "deleted", all instances of the
// rule are inhibited.  When the rule is really deleted (i.e., undelete is
// no longer possible), the inhibited instances are deleted.
//
void
DRCedit::user_rule_mod(Umode mode)
{
    if (!ed_usertest)
        return;
    bool needupd = false;
    CDl *ld;
    CDlgenDrv lgen;
    while ((ld = lgen.next()) != 0) {
        DRCtestDesc *tdn;
        for (DRCtestDesc *td = *tech_prm(ld)->rules_addr(); td; td = tdn) {
            tdn = td->next();
            if (td->matchUserRule(ed_usertest->name())) {
                if (mode == Uinhibit)
                    td->setInhibited(true);
                else if (mode == Uuninhibit)
                    td->setInhibited(false);
                else if (mode == Udelete && td->inhibited()) {
                    DRC()->unlinkRule(td);
                    delete td;
                }
                if (ld == LT()->CurLayer())
                    needupd = true;
                continue;
            }
        }
    }
    if (needupd)
        DRC()->PopUpRules(0, MODE_UPD);
}

