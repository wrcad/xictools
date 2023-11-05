
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

#include "qtcells.h"
#include "editif.h"
#include "cfilter.h"
#include "cd_celldb.h"
#include "cd_digest.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_chd.h"
#include "events.h"
#include "menu.h"
#include "cell_menu.h"
#include "promptline.h"
#include "errorlog.h"
#include "select.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qttextw.h"
#include "qtinterf/qtmsg.h"
#include "qtinterf/qtinput.h"
#include "miscutil/filestat.h"

#include <QLayout>
#include <QPushButton>
#include <QGroupBox>
#include <QLabel>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QMimeData>
#include <QComboBox>
#include <QDrag>


//----------------------------------------------------------------------
//  Cells Listing Panel
//
// Help system keywords used:
//  cellspanel


// Static function.
//
char *
QTmainwin::get_cell_selection()
{
    if (QTcellsDlg::self())
        return (QTcellsDlg::self()->get_selection());
    return (0);
}


// Static function.
// Called on crash to prevent updates.
//
void
QTmainwin::cells_panic()
{
    delete QTcellsDlg::self();
}


void
cMain::PopUpCells(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_OFF) {
        if (QTcellsDlg::self())
            QTcellsDlg::self()->deleteLater();
        return;
    }
    if (mode == MODE_UPD) {
        if (QTcellsDlg::self())
            QTcellsDlg::self()->update();
        return;
    }
    if (QTcellsDlg::self())
        return;

    new QTcellsDlg(caller);

    QTcellsDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(), QTcellsDlg::self(),
        QTmainwin::self()->Viewport());
    QTcellsDlg::self()->show();
}
// End of cMain functions.


namespace {
    // Command state for the Search function.
    //
    struct ListState : public CmdState
    {
        ListState(const char *nm, const char *hk) : CmdState(nm, hk)
        {
            lsAOI = CDinfiniteBB;
        }
        virtual ~ListState();

        static void show_search(bool);
        char *label_text();
        stringlist *cell_list(bool);

        void b1down() { cEventHdlr::sel_b1down(); }
        void b1up();
        void esc();
        void undo() { cEventHdlr::sel_undo(); }
        void redo() { cEventHdlr::sel_redo(); }
        void message() { PL()->ShowPrompt(info_msg); }

    private:
        BBox lsAOI;

        static const char *info_msg;
    };

    const char *ListState::info_msg =
        "Use pointer to define area for subcell list.";
    ListState *ListCmd;
}


QTcellsDlg *QTcellsDlg::instPtr;

QTcellsDlg::QTcellsDlg(GRobject c)
{
    instPtr = this;
    c_caller = c;
    c_clear_pop = 0;
    c_repl_pop = 0;
    c_copy_pop = 0;
    c_rename_pop = 0;
    c_save_pop = 0;
    c_msg_pop = 0;
    c_label = 0;
    c_clearbtn = 0;
    c_treebtn = 0;
    c_openbtn = 0;
    c_placebtn = 0;
    c_copybtn = 0;
    c_replbtn = 0;
    c_renamebtn = 0;
    c_searchbtn = 0;
    c_flagbtn = 0;
    c_infobtn = 0;
    c_showbtn = 0;
    c_fltrbtn = 0;
    c_page_combo = 0;
    c_mode_combo = 0;
    c_pfilter = cfilter_t::parse(0, Physical, 0);
    c_efilter = cfilter_t::parse(0, Electrical, 0);
    c_copyname = 0;
    c_replname = 0;
    c_newname = 0;
    c_no_select = false;
    c_no_update = false;
    c_dragging = false;
    c_drag_x = c_drag_y = 0;
    c_cols = 0;
    c_page = 0;
    c_mode = DSP()->CurMode();
    c_start = 0;
    c_end = 0;

    setWindowTitle(tr("Cells Listing"));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout();
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);
    vbox->addLayout(hbox);

    QVBoxLayout *col1 = new QVBoxLayout();
    col1->setContentsMargins(qm);
    col1->setSpacing(2);
    hbox->addLayout(col1);

    // button column
    //
    c_clearbtn = new QPushButton(tr("Clear"));
    col1->addWidget(c_clearbtn);
    c_clearbtn->setAutoDefault(false);
    connect(c_clearbtn, SIGNAL(clicked()), this, SLOT(clear_btn_slot()));
    if (!EditIf()->hasEdit())
        c_clearbtn->hide();

    c_treebtn = new QPushButton(tr("Tree"));
    col1->addWidget(c_treebtn);
    c_treebtn->setAutoDefault(false);
    connect(c_treebtn, SIGNAL(clicked()), this, SLOT(tree_btn_slot()));

    c_openbtn = new QPushButton(tr("Open"));
    col1->addWidget(c_openbtn);
    c_openbtn->setAutoDefault(false);
    connect(c_openbtn, SIGNAL(clicked()), this, SLOT(open_btn_slot()));

    c_placebtn = new QPushButton(tr("Place"));
    col1->addWidget(c_placebtn);
    c_placebtn->setAutoDefault(false);
    connect(c_placebtn, SIGNAL(clicked()), this, SLOT(place_btn_slot()));
    if (!EditIf()->hasEdit())
        c_placebtn->hide();

    c_copybtn = new QPushButton(tr("Copy"));
    col1->addWidget(c_copybtn);
    c_copybtn->setAutoDefault(false);
    connect(c_copybtn, SIGNAL(clicked()), this, SLOT(copy_btn_slot()));
    if (!EditIf()->hasEdit())
        c_copybtn->hide();

    c_replbtn = new QPushButton(tr("Replace"));;
    col1->addWidget(c_replbtn);
    c_replbtn->setAutoDefault(false);
    connect(c_replbtn, SIGNAL(clicked()), this, SLOT(repl_btn_slot()));
    if (!EditIf()->hasEdit())
        c_replbtn->hide();

    c_renamebtn = new QPushButton(tr("Rename"));;
    col1->addWidget(c_renamebtn);
    c_renamebtn->setAutoDefault(false);
    connect(c_renamebtn, SIGNAL(clicked()), this, SLOT(rename_btn_slot()));
    if (!EditIf()->hasEdit())
        c_renamebtn->hide();

    c_searchbtn = new QPushButton(tr("Search"));;
    col1->addWidget(c_searchbtn);
    c_searchbtn->setCheckable(true);
    c_searchbtn->setAutoDefault(false);
    connect(c_searchbtn, SIGNAL(toggled(bool)),
        this, SLOT(search_btn_slot(bool)));

    c_flagbtn = new QPushButton(tr("Flags"));
    col1->addWidget(c_flagbtn);
    c_flagbtn->setAutoDefault(false);
    connect(c_flagbtn, SIGNAL(clicked()), this, SLOT(flag_btn_slot()));

    c_infobtn = new QPushButton(tr("Info"));;
    col1->addWidget(c_infobtn);
    c_infobtn->setAutoDefault(false);
    connect(c_infobtn, SIGNAL(clicked()), this, SLOT(info_btn_slot()));

    c_showbtn = new QPushButton(tr("Show"));;
    col1->addWidget(c_showbtn);
    c_showbtn->setAutoDefault(false);
    connect(c_showbtn, SIGNAL(toggled(bool)), this, SLOT(show_btn_slot(bool)));

    c_fltrbtn = new QPushButton(tr("Filter"));
    col1->addWidget(c_fltrbtn);
    c_fltrbtn->setCheckable(true);
    c_fltrbtn->setAutoDefault(false);
    connect(c_fltrbtn, SIGNAL(toggled(bool)), this, SLOT(fltr_btn_slot(bool)));

    QPushButton *btn = new QPushButton(tr("Help"));
    col1->addWidget(btn);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(clicked()), this, SLOT(help_btn_slot()));

    // title label
    //
    QVBoxLayout *col2 = new QVBoxLayout();
    hbox->addLayout(col2);
    col2->setContentsMargins(qm);
    col2->setSpacing(2);

    QGroupBox *gb = new QGroupBox();
    col2->addWidget(gb);
    QHBoxLayout *hb = new QHBoxLayout(gb);
    hb->setContentsMargins(qmtop);
    hb->setSpacing(2);

    c_label = new QLabel("");
    hb->addWidget(c_label);

    // scrolled text area
    //
    wb_textarea = new QTtextEdit();
    wb_textarea->setReadOnly(true);
    wb_textarea->setMouseTracking(true);
    wb_textarea->setAcceptDrops(false);
    col2->addWidget(wb_textarea);
    connect(wb_textarea, SIGNAL(resize_event(QResizeEvent*)),
        this, SLOT(resize_slot(QResizeEvent*)));
    connect(wb_textarea, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(mouse_press_slot(QMouseEvent*)));
    connect(wb_textarea, SIGNAL(motion_event(QMouseEvent*)),
        this, SLOT(mouse_motion_slot(QMouseEvent*)));

    QFont *fnt;
    if (FC.getFont(&fnt, FNT_FIXED))
        wb_textarea->setFont(*fnt);
    connect(QTfont::self(), SIGNAL(fontChanged(int)),
        this, SLOT(font_changed_slot(int)), Qt::QueuedConnection);

    // bottom row buttons
    //
    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    btn = new QPushButton(tr("Save Text"));
    hbox->addWidget(btn);
    btn->setCheckable(true);
    btn->setAutoDefault(false);
    connect(btn, SIGNAL(toggled(bool)), this, SLOT(save_btn_slot(bool)));

    c_page_combo = new QComboBox();
    hbox->addWidget(c_page_combo);
    connect(c_page_combo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(page_menu_slot(int)));

    // dismiss button
    //
    btn = new QPushButton(tr("Dismiss"));
    hbox->addWidget(btn);
    connect(btn, SIGNAL(clicked()), this, SLOT(dismiss_btn_slot()));

    // mode menu
    //
    c_mode_combo = new QComboBox();
    hbox->addWidget(c_mode_combo);
    c_mode_combo->addItem(tr("Phys Cells"));
    c_mode_combo->addItem(tr("Elec Cells"));
    c_mode_combo->setCurrentIndex(c_mode);
    connect(c_mode_combo, SIGNAL(currentIndexChanged(int)),
        this, SLOT(mode_changed_slot(int)));

    update();
}


QTcellsDlg::~QTcellsDlg()
{
    instPtr = 0;
    XM()->SetTreeCaptive(false);
    if (c_caller)
        QTdev::Deselect(c_caller);
    if (ListCmd)
        ListCmd->esc();
    if (QTdev::GetStatus(c_showbtn))
        // erase highlighting
        DSP()->ShowCells(0);
    XM()->PopUpCellFlags(0, MODE_OFF, 0, 0);
    XM()->PopUpCellFilt(0, MODE_OFF, Physical, 0, 0);
    if (c_clear_pop)
        c_clear_pop->popdown();
    if (c_copy_pop)
        c_copy_pop->popdown();
    if (c_repl_pop)
        c_repl_pop->popdown();
    if (c_rename_pop)
        c_rename_pop->popdown();
    if (c_save_pop)
        c_save_pop->popdown();
    if (c_msg_pop)
        c_msg_pop->popdown();
    delete c_pfilter;
    delete c_efilter;
    delete [] c_copyname;
    delete [] c_replname;
    delete [] c_newname;
}


void
QTcellsDlg::update()
{
    select_range(0, 0);
    if (QTdev::GetStatus(c_showbtn))
        DSP()->ShowCells(0);
    XM()->PopUpCellFlags(0, MODE_OFF, 0, 0);
    if (DSP()->MainWdesc()->DbType() == WDchd) {
        if (c_clear_pop)
            c_clear_pop->popdown();
        if (c_copy_pop)
            c_copy_pop->popdown();
        if (c_repl_pop)
            c_repl_pop->popdown();
        if (c_rename_pop)
            c_rename_pop->popdown();

        QTdev::Deselect(c_showbtn);

        c_clearbtn->setEnabled(false);
        c_openbtn->setEnabled(false);
        c_placebtn->setEnabled(false);
        c_copybtn->setEnabled(false);
        c_replbtn->setEnabled(false);
        c_renamebtn->setEnabled(false);
        c_flagbtn->setEnabled(false);

        c_clearbtn->hide();
        c_openbtn->hide();
        c_placebtn->hide();
        c_copybtn->hide();
        c_replbtn->hide();
        c_renamebtn->hide();
        c_flagbtn->hide();
        c_fltrbtn->hide();

        if (ActiveInput())
            ActiveInput()->popdown();
        XM()->PopUpCellFilt(0, MODE_OFF, Physical, 0, 0);
    }
    else {
        c_clearbtn->setEnabled(true);
        c_openbtn->setEnabled(true);
        if (EditIf()->hasEdit()) {
            c_placebtn->setEnabled(true);
            c_copybtn->setEnabled(true);
            c_replbtn->setEnabled(true);
            c_renamebtn->setEnabled(true);
        }
        c_flagbtn->setEnabled(true);

        c_clearbtn->show();
        c_openbtn->show();
        if (EditIf()->hasEdit()) {
            c_placebtn->show();
            c_copybtn->show();
            c_replbtn->show();
            c_renamebtn->show();
        }
        c_flagbtn->show();
        c_fltrbtn->show();
    }
    if (!c_no_update) {
        int wid = wb_textarea->width();
        c_cols = (wid-4)/QTfont::stringWidth(0, wb_textarea);
        char *s = cell_list(c_cols);
        update_text(s);
        delete [] s;
    }
    if (QTdev::GetStatus(c_searchbtn) && ListCmd) {
        char *s = ListCmd->label_text();
        c_label->setText(tr(s));
        delete [] s;
    }
    else if (DSP()->MainWdesc()->DbType() == WDchd) {
        const char *chd_msg = "Cells in displayed cell hierarchy";
        c_label->setText(tr(chd_msg));
    }
    else {
        const char *pcells_msg = "Physical cells (+ modified, * top level)";
        const char *ecells_msg = "Electrical cells (+ modified, * top level)";
        if (c_mode == Physical)
            c_label->setText(tr(pcells_msg));
        else
            c_label->setText(tr(ecells_msg));
    }

    if (QTdev::GetStatus(c_searchbtn)) {
        DisplayMode oldm = c_mode;
        c_mode = DSP()->CurMode();
        if (oldm != c_mode)
            XM()->PopUpCellFilt(0, MODE_UPD, c_mode, 0, 0);
        c_mode_combo->setCurrentIndex(c_mode);
        c_mode_combo->setEnabled(false);
    }
    else
        c_mode_combo->setEnabled(true);
    check_sens();
}


char *
QTcellsDlg::get_selection()
{
    return (wb_textarea->get_selection());
}


// Function to clear cells from the database, or clear the entire
// database.
//
void
QTcellsDlg::clear(const char *name)
{
    if (!name || !*name) {
        EV()->InitCallback();
        if (c_clear_pop)
            c_clear_pop->popdown();
        c_clear_pop = PopUpAffirm(c_clearbtn, GRloc(),
            "This will clear the database.\nContinue?", c_clear_cb, 0);
        if (c_clear_pop)
            c_clear_pop->register_usrptr((void**)&c_clear_pop);
    }
    else {
        CDcbin cbin;
        if (!CDcdb()->findSymbol(name, &cbin))
            // can't happen
            return;

        // Clear top-level empty parts.
        bool p_cleared = false;
        bool e_cleared = false;
        if (cbin.phys() && cbin.phys()->isEmpty() &&
                !cbin.phys()->isSubcell()) {
            delete cbin.phys();
            cbin.setPhys(0);
            p_cleared = true;
        }
        if (cbin.elec() && cbin.elec()->isEmpty() &&
                !cbin.elec()->isSubcell()) {
            delete cbin.elec();
            cbin.setElec(0);
            e_cleared = true;
        }
        if (p_cleared || e_cleared) {
            XM()->PopUpCells(0, MODE_UPD);
            XM()->PopUpTree(0, MODE_UPD, 0, TU_CUR);
        }
        if (!cbin.phys() && !cbin.elec()) {
            // Cell and top cell will either be gone, or both ok.
            if (!DSP()->TopCellName()) {
                FIOreadPrms prms;
                XM()->EditCell(XM()->DefaultEditName(), false, &prms);
            }
            else {
                EditIf()->ulListBegin(false, false);
                for (int i = 1; i < DSP_NUMWINS; i++) {
                    if (DSP()->Window(i) && !DSP()->Window(i)->CurCellName())
                        DSP()->Window(i)->SetCurCellName(DSP()->CurCellName());
                }
            }
            return;
        }

        if (!cbin.isSubcell()) {
            EV()->InitCallback();
            char buf[256];
            snprintf(buf, sizeof(buf),
                "This will clear %s and all its\n"
                "subcells from the database.  Continue?", name);
            if (c_clear_pop)
                c_clear_pop->popdown();
            c_clear_pop = PopUpAffirm(c_clearbtn, GRloc(), buf,
                c_clear_cb, lstring::copy(name));
            if (c_clear_pop)
                c_clear_pop->register_usrptr((void**)&c_clear_pop);
        }
        else {
            PopUpMessage(
                "Can't clear, cell is not top level in both modes.",
                true, false, false);
        }
    }
}


// Function to copy a selected cell to a new name.
//
void
QTcellsDlg::copy_cell(const char *name)
{
    if (!name)
        return;
    delete [] c_copyname;
    c_copyname = lstring::copy(name);
    if (c_copy_pop)
        c_copy_pop->popdown();
    sLstr lstr;
    lstr.add("Copy cell ");
    lstr.add(c_copyname);
    lstr.add("\nEnter new name to create copy ");

    c_copy_pop = PopUpEditString((GRobject)c_copybtn, GRloc(),
        lstr.string(), c_copyname, c_copy_cb, c_copyname, 214, 0);
    if (c_copy_pop)
        c_copy_pop->register_usrptr((void**)&c_copy_pop);
}


// Function to replace selected instances with listing selection.
//
void
QTcellsDlg::replace_instances(const char *name)
{
    if (!name)
        return;
    delete [] c_replname;
    c_replname = lstring::copy(name);
    if (c_repl_pop)
        c_repl_pop->popdown();
    sLstr lstr;
    lstr.add("This will replace selected cells\nwith ");
    lstr.add(c_replname);
    lstr.add(".  Continue? ");

    c_repl_pop = PopUpAffirm(c_replbtn, GRloc(), lstr.string(),
        c_repl_cb, c_replname);
    if (c_repl_pop)
        c_repl_pop->register_usrptr((void**)&c_repl_pop);
}


// Function to rename all instances of a cell in the database.
//
void
QTcellsDlg::rename_cell(const char *name)
{
    if (!name)
        return;
    delete [] c_newname;
    c_newname = lstring::copy(name);
    if (c_rename_pop)
        c_rename_pop->popdown();
    sLstr lstr;
    lstr.add("Rename cell ");
    lstr.add(c_newname);
    lstr.add("\nEnter new name for cell ");
    c_rename_pop = PopUpEditString((GRobject)c_renamebtn,
        GRloc(), lstr.string(), c_newname, c_rename_cb, c_newname, 214, 0);
    if (c_rename_pop)
        c_rename_pop->register_usrptr((void**)&c_rename_pop);
}


// Select the chars in the range, start=end deselects existing.
//
void
QTcellsDlg::select_range(int start, int end)
{
    wb_textarea->select_range(start, end);
    c_start = start;
    c_end = end;
    check_sens();
}


namespace {
    // Sort comparison, strip junk prepended to cell names.
    bool
    cl_comp(const char *s1, const char *s2)
    {
        while (isspace(*s1) || *s1 == '*' || *s1 == '+')
            s1++;
        while (isspace(*s2) || *s2 == '*' || *s2 == '+')
            s2++;
        return (strcmp(s1, s2) < 0);
    }
}


stringlist *
QTcellsDlg::raw_cell_list(int *pcnt, int *ppgs, bool nomark)
{
    if (pcnt)
        *pcnt = 0;
    if (ppgs)
        *ppgs = 0;
    stringlist *s0 = 0;
    if (QTdev::GetStatus(c_searchbtn) && ListCmd)
        s0 = ListCmd->cell_list(nomark);
    else {
        if (DSP()->MainWdesc()->DbType() == WDchd) {
            cCHD *chd = CDchd()->chdRecall(DSP()->MainWdesc()->DbName(), false);
            if (chd)
                s0 = chd->listCellnames(c_mode, true);
        }
        else {
            CDgenTab_cbin sgen;
            CDcbin cbin;
            while (sgen.next(&cbin) != false) {
                if (c_mode == Physical) {
                    if (!cbin.phys())
                        continue;
                    if (c_pfilter && !c_pfilter->inlist(&cbin))
                        continue;
                }
                if (c_mode == Electrical) {
                    if (!cbin.elec())
                        continue;
                    if (c_efilter && !c_efilter->inlist(&cbin))
                        continue;
                }
                if (nomark) {
                    s0 = new stringlist(lstring::copy(
                        Tstring(cbin.cellname())), s0);
                }
                else {
                    int j = 0;
                    char buf[128];
                    if (c_mode == Physical) {
                        if (cbin.phys()->isModified())
                            buf[j++] = '+';
                        if (!cbin.phys()->isSubcell())
                            buf[j++] = '*';
                    }
                    else {
                        if (cbin.elec()->isModified())
                            buf[j++] = '+';
                        if (!cbin.elec()->isSubcell())
                            buf[j++] = '*';
                    }
                    if (!j)
                        buf[j++] = ' ';
                    strcpy(buf+j, Tstring(cbin.cellname()));
                    s0 = new stringlist(lstring::copy(buf), s0);
                }
            }
            stringlist::sort(s0, &cl_comp);
        }
    }

    int pagesz = 0;
    const char *s = CDvdb()->getVariable(VA_ListPageEntries);
    if (s) {
        pagesz = atoi(s);
        if (pagesz < 100 || pagesz > 50000)
            pagesz = 0;
    }
    if (pagesz == 0)
        pagesz = DEF_LIST_MAX_PER_PAGE;

    int cnt = 0;
    for (stringlist *sl = s0; sl; sl = sl->next)
        cnt++;
    int min = c_page * pagesz;
    while (min >= cnt && min > 0) {
        c_page--;
        min = c_page * pagesz;
    }
    int max = min + pagesz;

    cnt = 0;
    stringlist *slprev = 0;
    for (stringlist *sl = s0; sl; slprev = sl, cnt++, sl = sl->next) {
        if (cnt < min)
            continue;
        if (cnt == min) {
            if (slprev) {
                slprev->next = 0;
                stringlist::destroy(s0);
                s0 = sl;
            }
            continue;
        }
        if (cnt >= max) {
            for (stringlist *st = sl; st; st = st->next)
                cnt++;
            stringlist::destroy(sl->next);
            sl->next = 0;
            break;
        }
    }
    if (pcnt)
        *pcnt = cnt;
    if (ppgs)
        *ppgs = pagesz;
    return (s0);
}


// Return a formatted list of cells found in the database.
//
char *
QTcellsDlg::cell_list(int cols)
{
    int cnt, pagesz;
    stringlist *s0 = raw_cell_list(&cnt, &pagesz, false);
    if (cnt <= pagesz)
        c_page_combo->hide();
    else {
        char buf[128];
        c_page_combo->clear();
        for (int i = 0; i*pagesz < cnt; i++) {
            int tmpmax = (i+1)*pagesz;
            if (tmpmax > cnt)
                tmpmax = cnt;
            snprintf(buf, sizeof(buf), "%d - %d", i*pagesz, tmpmax);
            c_page_combo->addItem(buf);
        }
        c_page_combo->setCurrentIndex(c_page);
        c_page_combo->show();
    }

    char *t = stringlist::col_format(s0, cols);
    stringlist::destroy(s0);
    return (t);
}


// Refresh the text while keeping current top location.
//
void
QTcellsDlg::update_text(char *newtext)
{
    if (wb_textarea == 0 || newtext == 0)
        return;
    double val = wb_textarea->get_scroll_value();
    wb_textarea->set_chars(newtext);
    wb_textarea->set_scroll_value(val);
}


// Set button sensitivity according to mode.
//
void
QTcellsDlg::check_sens()
{
    bool has_sel = wb_textarea->has_selection();
    if (!has_sel) {
        if (QTdev::GetStatus(c_showbtn))
            DSP()->ShowCells(0);
        c_treebtn->setEnabled(false);
        c_openbtn->setEnabled(false);
        c_placebtn->setEnabled(false);
        c_copybtn->setEnabled(false);
        c_replbtn->setEnabled(false);
        c_renamebtn->setEnabled(false);
    }
    else {
        bool mode_ok = (c_mode == DSP()->CurMode());
        c_treebtn->setEnabled(true);
        if (DSP()->MainWdesc()->DbType() == WDcddb) {
            c_openbtn->setEnabled(mode_ok);
            if (EditIf()->hasEdit()) {
                bool ed_ok = !CurCell() || !CurCell()->isImmutable();
                c_placebtn->setEnabled(mode_ok && ed_ok);
                c_copybtn->setEnabled(true);
                c_replbtn->setEnabled(mode_ok && ed_ok &&
                    (Selections.firstObject(CurCell(), "c")) != 0);
                c_renamebtn->setEnabled(ed_ok);
            }
        }
        if (QTdev::GetStatus(c_showbtn)) {
            char *cn = get_selection();
            DSP()->ShowCells(cn);
            delete [] cn;
        }
    }
}


// Static function.
// Callback for the clear button dialog.
//
void
QTcellsDlg::c_clear_cb(bool state, void *arg)
{
    char *name = (char*)arg;
    if (state)
        XM()->Clear(name);
    delete [] name;
}


// Static function.
// Callback for the copy button dialog.
//
ESret
QTcellsDlg::c_copy_cb(const char *newname, void *arg)
{
    char *name = (char*)arg;
    if (EditIf()->copySymbol(name, newname)) {
        CDcbin cbin;
        if (CDcdb()->findSymbol(newname, &cbin)) {
            // select new cell
            QTpkg::self()->RegisterIdleProc(c_highlight_idle,
                (void*)cbin.cellname());
        }
        return (ESTR_DN);
    }
    sLstr lstr;
    lstr.add("Copy cell ");
    lstr.add(name);
    lstr.add("\nName invalid or not unique, try again ");
    instPtr->c_copy_pop->update(lstr.string(), 0);
    return (ESTR_IGN);
}


// Static function.
// Callback for the replace dialog.
//
void
QTcellsDlg::c_repl_cb(bool state, void *arg)
{
    if (state) {
        Errs()->init_error();
        EV()->InitCallback();
        CDs *cursd = CurCell();
        if (!cursd)
            return;
        EditIf()->ulListCheck("replace", cursd, false);
        sSelGen sg(Selections, cursd, "c");
        CDc *cd;
        while ((cd = (CDc*)sg.next()) != 0) {
            CDcbin cbin;
            CD()->OpenExisting((const char*)arg, &cbin);
            if (!EditIf()->replaceInstance(cd, &cbin, false, false)) {
                Errs()->add_error("ReplaceCell failed");
                Log()->ErrorLog(mh::EditOperation, Errs()->get_error());
            }
        }
        EditIf()->ulCommitChanges(true);
        XM()->PopUpCells(0, MODE_UPD);
        XM()->PopUpTree(0, MODE_UPD, 0, TU_CUR);
    }
}


// Static function.
// Callback for the rename dialog.
//
ESret
QTcellsDlg::c_rename_cb(const char *newname, void *arg)
{
    if (!instPtr)
        return (ESTR_DN);
    char *name = (char*)arg;
    if (EditIf()->renameSymbol(name, newname)) {
        EV()->InitCallback();
        XM()->ShowParameters();
        XM()->PopUpCells(0, MODE_UPD);
        XM()->PopUpTree(0, MODE_UPD, 0, TU_CUR);

        CDcbin cbin;
        if (CDcdb()->findSymbol(newname, &cbin))
            // select renamed cell
            QTpkg::self()->RegisterIdleProc(c_highlight_idle,
                (void*)cbin.cellname());

        WindowDesc *wd;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wd = wgen.next()) != 0) {
            if (wd->Attrib()->expand_level(wd->Mode()) != -1)
                wd->Redisplay(0);
        }
        const char *nn = Tstring(cbin.cellname());
        for (const char *n = nn; *n; n++) {
            if (isalnum(*n) || *n == '_' || *n == '$' || *n == '?')
                continue;
            Log()->WarningLogV("rename",
                "New cell name \"%s\" contains non-portable character,\n"
                "recommend cell names use alphanum and $_? only.", nn);
            break;
        }
        return (ESTR_DN);
    }
    sLstr lstr;
    lstr.add("Rename cell ");
    lstr.add(name);
    lstr.add(Errs()->get_error());
    instPtr->c_rename_pop->update(lstr.string(), 0);
    return (ESTR_IGN);
}


// Called from the filter panel Apply button.  The flt is a copy that
// we need to own.
//
void
QTcellsDlg::c_filter_cb(cfilter_t *flt, void*)
{
    if (!instPtr) {
        delete flt;
        return;
    }
    if (!flt)
        return;
    if (flt->mode() == Physical) {
        delete instPtr->c_pfilter;
        instPtr->c_pfilter = flt;
    }
    else {
        delete instPtr->c_efilter;
        instPtr->c_efilter = flt;
    }
    instPtr->update();
}


// Static function.
// Idle proc to highlight the listing for name.
//
int
QTcellsDlg::c_highlight_idle(void *arg)
{
    if (instPtr && arg) {
        const char *name = (const char*)arg;
        int n = strlen(name);
        if (!n)
            return (0);
        char *s = instPtr->wb_textarea->get_chars(0, -1);
        char *t = s;
        while (*t) {
            while (isspace(*t))
                t++;
            if (*t == '*' || *t == '+')
                t++;
            if (*t == '*' || *t == '+')
                t++;
            if (!*t)
                break;
            if (!strncmp(t, name, n) && (isspace(t[n]) || !t[n])) {
                int top = t - s;
                instPtr->select_range(top, top + n);
                break;
            }
            while (*t && !isspace(*t))
                t++;
        }
        delete [] s;
    }
    return (0);
}


// Private static handler.
// Callback for the save file name pop-up.
//
ESret
QTcellsDlg::c_save_cb(const char *string, void *arg)
{
    QTcellsDlg *cp = (QTcellsDlg*)arg;
    if (string) {
        if (!filestat::create_bak(string)) {
            cp->c_save_pop->update(
                "Error backing up existing file, try again", 0);
            return (ESTR_IGN);
        }
        FILE *fp = fopen(string, "w");
        if (!fp) {
            cp->c_save_pop->update("Error opening file, try again", 0);
            return (ESTR_IGN);
        }
        char *txt = cp->wb_textarea->get_chars(0, -1);
        if (txt) {
            unsigned int len = strlen(txt);
            if (len) {
                if (fwrite(txt, 1, len, fp) != len) {
                    cp->c_save_pop->update("Write failed, try again", 0);
                    delete [] txt;
                    fclose(fp);
                    return (ESTR_IGN);
                }
            }
            delete [] txt;
        }
        fclose(fp);

        if (cp->c_msg_pop)
            cp->c_msg_pop->popdown();
        cp->c_msg_pop = new QTmsgDlg(0, "Text saved in file.", false);
        cp->c_msg_pop->register_usrptr((void**)&cp->c_msg_pop);
        QTdev::self()->SetPopupLocation(GRloc(), cp->c_msg_pop,
            cp->wb_shell);
        cp->c_msg_pop->set_visible(true);
        QTpkg::self()->RegisterTimeoutProc(2000, c_timeout, cp);
    }
    return (ESTR_DN);
}


int
QTcellsDlg::c_timeout(void *arg)
{
    QTcellsDlg *cp = (QTcellsDlg*)arg;
    if (cp->c_msg_pop)
        cp->c_msg_pop->popdown();
    return (0);
}


void
QTcellsDlg::clear_btn_slot()
{
    if (c_clear_pop)
        c_clear_pop->popdown();
    if (c_copy_pop)
        c_copy_pop->popdown();
    if (c_repl_pop)
        c_repl_pop->popdown();
    if (c_rename_pop)
        c_rename_pop->popdown();

    c_no_select = true;
    char *cname = get_selection();
    clear(cname);
    c_no_select = false;
    delete [] cname;
}


void
QTcellsDlg::tree_btn_slot()
{
    if (c_clear_pop)
        c_clear_pop->popdown();
    if (c_copy_pop)
        c_copy_pop->popdown();
    if (c_repl_pop)
        c_repl_pop->popdown();
    if (c_rename_pop)
        c_rename_pop->popdown();

    c_no_select = true;
    char *cname = get_selection();

    if (cname) {
        // find the main menu button for the tree popup
        MenuEnt *m = MainMenu()->FindEntry(MMcell, MenuTREE);
        GRobject widg = 0;
        if (m)
            widg = m->cmd.caller;
        XM()->PopUpTree(widg, MODE_ON, cname,
            c_mode == Physical ? TU_PHYS : TU_ELEC);
        // If tree is "captive" don't update after main window
        // display mode switch.
        XM()->SetTreeCaptive(true);

        if (widg)
            QTdev::Select(widg);
    }
    c_no_select = false;
    delete [] cname;
}


void
QTcellsDlg::open_btn_slot()
{
    if (c_clear_pop)
        c_clear_pop->popdown();
    if (c_copy_pop)
        c_copy_pop->popdown();
    if (c_repl_pop)
        c_repl_pop->popdown();
    if (c_rename_pop)
        c_rename_pop->popdown();

    c_no_select = true;
    char *cname = get_selection();

    if (cname) {
        c_no_update = true;
        EV()->InitCallback();
        XM()->EditCell(cname, false);
        c_no_update = false;
    }
    c_no_select = false;
    delete [] cname;
}


void
QTcellsDlg::place_btn_slot()
{
    if (c_clear_pop)
        c_clear_pop->popdown();
    if (c_copy_pop)
        c_copy_pop->popdown();
    if (c_repl_pop)
        c_repl_pop->popdown();
    if (c_rename_pop)
        c_rename_pop->popdown();

    c_no_select = true;
    char *cname = get_selection();

    if (cname) {
        EV()->InitCallback();
        EditIf()->addMaster(cname, 0);
    }
    c_no_select = false;
    delete [] cname;
}


void
QTcellsDlg::copy_btn_slot()
{
    if (c_clear_pop)
        c_clear_pop->popdown();
    if (c_copy_pop)
        c_copy_pop->popdown();
    if (c_repl_pop)
        c_repl_pop->popdown();
    if (c_rename_pop)
        c_rename_pop->popdown();

    c_no_select = true;
    char *cname = get_selection();

    copy_cell(cname);
    c_no_select = false;
    delete [] cname;
}


void
QTcellsDlg::repl_btn_slot()
{
    if (c_clear_pop)
        c_clear_pop->popdown();
    if (c_copy_pop)
        c_copy_pop->popdown();
    if (c_repl_pop)
        c_repl_pop->popdown();
    if (c_rename_pop)
        c_rename_pop->popdown();

    c_no_select = true;
    char *cname = get_selection();

    replace_instances(cname);
    c_no_select = false;
    delete [] cname;
}


void
QTcellsDlg::rename_btn_slot()
{
    if (c_clear_pop)
        c_clear_pop->popdown();
    if (c_copy_pop)
        c_copy_pop->popdown();
    if (c_repl_pop)
        c_repl_pop->popdown();
    if (c_rename_pop)
        c_rename_pop->popdown();

    c_no_select = true;
    char *cname = get_selection();

    rename_cell(cname);
    c_no_select = false;
    delete [] cname;
}


void
QTcellsDlg::search_btn_slot(bool state)
{
    if (c_clear_pop)
        c_clear_pop->popdown();
    if (c_copy_pop)
        c_copy_pop->popdown();
    if (c_repl_pop)
        c_repl_pop->popdown();
    if (c_rename_pop)
        c_rename_pop->popdown();

    c_no_select = true;
    char *cname = get_selection();

    if (state)
        EV()->InitCallback();
    ListState::show_search(state);
    c_no_select = false;
    delete [] cname;
}


void
QTcellsDlg::flag_btn_slot()
{
    if (c_clear_pop)
        c_clear_pop->popdown();
    if (c_copy_pop)
        c_copy_pop->popdown();
    if (c_repl_pop)
        c_repl_pop->popdown();
    if (c_rename_pop)
        c_rename_pop->popdown();

    c_no_select = true;
    char *cname = get_selection();

    stringlist *s0;
    if (cname)
        s0 = new stringlist(lstring::copy(cname), 0);
    else
        s0 = raw_cell_list(0, 0, true);
    XM()->PopUpCellFlags(c_flagbtn, MODE_ON, s0, c_mode);
    stringlist::destroy(s0);
    c_no_select = false;
    delete [] cname;
}


void
QTcellsDlg::info_btn_slot()
{
    if (c_clear_pop)
        c_clear_pop->popdown();
    if (c_copy_pop)
        c_copy_pop->popdown();
    if (c_repl_pop)
        c_repl_pop->popdown();
    if (c_rename_pop)
        c_rename_pop->popdown();

    c_no_select = true;
    char *cname = get_selection();

    if (DSP()->MainWdesc()->DbType() == WDchd) {
        cCHD *chd = CDchd()->chdRecall(DSP()->MainWdesc()->DbName(), false);
        if (chd) {
            symref_t *p = chd->findSymref(cname, DSP()->CurMode(), true);
            if (p) {
                stringlist *sl = new stringlist(
                    lstring::copy(Tstring(p->get_name())), 0);
                int flgs = FIO_INFO_OFFSET | FIO_INFO_INSTANCES |
                    FIO_INFO_BBS | FIO_INFO_FLAGS;
                char *str = chd->prCells(0, DSP()->CurMode(), flgs, sl);
                stringlist::destroy(sl);
                PopUpInfo(MODE_ON, str, STY_FIXED);
                delete [] str;
            }
        }
    }
    else
        XM()->ShowCellInfo(cname, true, c_mode);
    c_no_select = false;
    delete [] cname;
}


void
QTcellsDlg::show_btn_slot(bool state)
{
    if (c_clear_pop)
        c_clear_pop->popdown();
    if (c_copy_pop)
        c_copy_pop->popdown();
    if (c_repl_pop)
        c_repl_pop->popdown();
    if (c_rename_pop)
        c_rename_pop->popdown();

    c_no_select = true;
    char *cname = get_selection();

    if (state)
        DSP()->ShowCells(cname);
    else
        DSP()->ShowCells(0);
    c_no_select = false;
    delete [] cname;
}


void
QTcellsDlg::fltr_btn_slot(bool state)
{
    if (c_clear_pop)
        c_clear_pop->popdown();
    if (c_copy_pop)
        c_copy_pop->popdown();
    if (c_repl_pop)
        c_repl_pop->popdown();
    if (c_rename_pop)
        c_rename_pop->popdown();

    c_no_select = true;
    char *cname = get_selection();

    if (state)
        XM()->PopUpCellFilt(c_fltrbtn, MODE_ON, c_mode, c_filter_cb, 0);
    else
        XM()->PopUpCellFilt(0, MODE_OFF, Physical, 0, 0);
    c_no_select = false;
    delete [] cname;
}


void
QTcellsDlg::help_btn_slot()
{
    DSPmainWbag(PopUpHelp("cellspanel"))
}


void
QTcellsDlg::resize_slot(QResizeEvent *ev)
{
    int cols = (ev->size().width()-4)/QTfont::stringWidth(0, wb_textarea) - 2;
    if (cols == c_cols)
        return;
    c_cols = cols;
    char *s = cell_list(cols);
    update_text(s);
    delete [] s;
    select_range(0, 0);
}


void
QTcellsDlg::mouse_press_slot(QMouseEvent *ev)
{
    if (ev->type() == QEvent::MouseButtonRelease) {
        c_dragging = false;
        ev->accept();
        return;
    }
    if (ev->type() != QEvent::MouseButtonPress) {
        ev->ignore();
        return;
    }
    ev->accept();

    if (c_no_select)
        return;

    select_range(0, 0);

    char *str =
        lstring::copy(wb_textarea->toPlainText().toLatin1().constData());
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int xx = ev->position().x();
    int yy = ev->position().y();
#else
    int xx = ev->x();
    int yy = ev->y();
#endif
    QTextCursor cur = wb_textarea->cursorForPosition(QPoint(xx, yy));
    int posn = cur.position();

    if (isspace(str[posn])) {
        // Clicked on white space.
        delete [] str;
        return;
    }

    char *start = str + posn;
    char *end = start;
    while (!isspace(*start) && start > str)
        start--;
    if (isspace(*start))
        start++;
    while (!isspace(*end) && *end)
        end++;
    *end = 0;

    // The top level cells are listed with an '*'.
    // Modified cells are listed  with a '+'.
    while ((*start == '+' || *start == '*') && !CDcdb()->findSymbol(start))
        start++;

    if (start == end) {
        delete [] str;
        return;
    }
    select_range(start - str, end - str);
    delete [] str;

    c_dragging = true;
    c_drag_x = xx;
    c_drag_y = yy;
}


void
QTcellsDlg::mouse_motion_slot(QMouseEvent *ev)
{
    if (ev->type() != QEvent::MouseMove) {
        ev->ignore();
        return;
    }
    ev->accept();

    if (!c_dragging)
        return;
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    if (abs(ev->position().x() - c_drag_x) < 5 &&
            abs(ev->position().y() - c_drag_y) < 5)
#else
    if (abs(ev->x() - c_drag_x) < 5 && abs(ev->y() - c_drag_y) < 5)
#endif
        return;

    char *sel = wb_textarea->get_selection();
    if (!sel)
        return;

    c_dragging = false;
    QDrag *drag = new QDrag(wb_textarea);
    QMimeData *mimedata = new QMimeData();
    QByteArray qdata((const char*)sel, strlen(sel)+1);
    mimedata->setData("text/plain", qdata);
    delete [] sel;
    drag->setMimeData(mimedata);
    drag->exec(Qt::CopyAction);
    delete drag;
}


void
QTcellsDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_FIXED) {
        QFont *fnt;
        if (FC.getFont(&fnt, FNT_FIXED))
            wb_textarea->setFont(*fnt);
        int cols = (wb_textarea->width()-4)/
            QTfont::stringWidth(0, wb_textarea) - 2;
        if (cols == c_cols)
            return;
        c_cols = cols;
        char *s = cell_list(cols);
        update_text(s);
        delete [] s;
        select_range(0, 0);
    }
}


void
QTcellsDlg::save_btn_slot(bool state)
{
    if (!state)
        return;
    if (c_save_pop)
        return;
    c_save_pop = new QTledDlg(0,
        "Enter path to file for saved text:", "", "Save", false);
    c_save_pop->register_caller(sender(), false, true);
    c_save_pop->register_callback(
        (GRledPopup::GRledCallback)&c_save_cb);
    c_save_pop->set_callback_arg(this);
    c_save_pop->register_usrptr((void**)&c_save_pop);

    QTdev::self()->SetPopupLocation(GRloc(), c_save_pop, this);
    c_save_pop->set_visible(true);
}


void
QTcellsDlg::page_menu_slot(int i)
{
    if (c_page != i) {
        c_page = i;
        update();
    }
}


void
QTcellsDlg::dismiss_btn_slot()
{
    XM()->PopUpCells(0, MODE_OFF);
}


void
QTcellsDlg::mode_changed_slot(int i)
{
    DisplayMode m = i ? Electrical : Physical;
    if (c_mode != m) {
        c_mode = m;
        XM()->PopUpCellFilt(0, MODE_UPD, c_mode, 0, 0);
        update();
        XM()->PopUpTree(0, MODE_UPD, 0,
            c_mode == Physical ? TU_PHYS : TU_ELEC);
    }
}
// End of QTcellsDlg functions and slots.


//-----------------------
// ListState functions

ListState::~ListState()
{
    ListCmd = 0;
}


// Static function.
// The search function, print a listing of the cells found in an area
// of the screen that the user defines.
//
void
ListState::show_search(bool on)
{
    if (on) {
        if (ListCmd)
            return;
        ListCmd = new ListState("SEARCH", "xic:cells#search");
        if (!EV()->PushCallback(ListCmd)) {
            delete ListCmd;
            ListCmd = 0;
            return;
        }
        ListCmd->message();
    }
    else {
        if (!ListCmd)
            return;
        ListCmd->esc();
    }
    if (QTcellsDlg::self())
        QTcellsDlg::self()->update();
}


char *
ListState::label_text()
{
    char buf[256];
    if (lsAOI == CDinfiniteBB) {
        buf[0] = 0;
        if (DSP()->MainWdesc()->DbType() == WDchd) {
            cCHD *chd = CDchd()->chdRecall(DSP()->MainWdesc()->DbName(), false);
            if (chd) {
                symref_t *p = chd->findSymref(DSP()->MainWdesc()->DbCellName(),
                    DSP()->CurMode(), true);
                snprintf(buf, sizeof(buf), "Cells under %s",
                    p ? Tstring(p->get_name()) : "<unknown>");
            }
        }
        else {
            snprintf(buf, sizeof(buf), "Cells under %s",
                Tstring(DSP()->CurCellName()));
        }
    }
    else {
        if (CDvdb()->getVariable(VA_InfoInternal))
            snprintf(buf, sizeof(buf), "Cells intersecting (%d,%d %d,%d)",
                lsAOI.left, lsAOI.bottom, lsAOI.right, lsAOI.top);
        else if (DSP()->CurMode() == Physical) {
            int ndgt = CD()->numDigits();
            snprintf(buf, sizeof(buf),
                "Cells intersecting (%.*f,%.*f %.*f,%.*f)",
                ndgt, MICRONS(lsAOI.left), ndgt, MICRONS(lsAOI.bottom),
                ndgt, MICRONS(lsAOI.right), ndgt, MICRONS(lsAOI.top));
        }
        else {
            snprintf(buf, sizeof(buf),
                "Cells intersecting (%.3f,%.3f %.3f,%.3f)",
                ELEC_MICRONS(lsAOI.left), ELEC_MICRONS(lsAOI.bottom),
                ELEC_MICRONS(lsAOI.right), ELEC_MICRONS(lsAOI.top));
        }
    }
    return (lstring::copy(buf));
}


stringlist *
ListState::cell_list(bool nomark)
{
    stringlist *s0 = 0;
    if (DSP()->MainWdesc()->DbType() == WDchd) {
        cCHD *chd = CDchd()->chdRecall(DSP()->MainWdesc()->DbName(), false);
        if (chd) {
            const char *cname = DSP()->MainWdesc()->DbCellName();
            syrlist_t *sy0 = chd->listing(DSP()->CurMode(), cname, false,
                &lsAOI);
            for (syrlist_t *s = sy0; s; s = s->next) {
                s0 = new stringlist(
                    lstring::copy(Tstring(s->symref->get_name())), s0);
            }
            syrlist_t::destroy(sy0);
        }
        stringlist::destroy(s0);
    }
    else {
        if (!DSP()->CurCellName())
            return (0);
        s0 = CurCell()->listSubcells(-1, true, !nomark, &lsAOI);
    }
    return (s0);
}


// Finish selection.  If success, do the list.
//
void
ListState::b1up()
{
    BBox BB;
    if (cEventHdlr::sel_b1up(&BB, 0, B1UP_NOSEL)) {
        lsAOI = BB;
        if (QTcellsDlg::self())
            QTcellsDlg::self()->update();
    }
}


// Esc entered, clean up and exit.
//
void
ListState::esc()
{
    cEventHdlr::sel_esc();
    if (QTcellsDlg::self())
        QTcellsDlg::self()->end_search();
    PL()->ErasePrompt();
    EV()->PopCallback(this);
    delete this;
}

