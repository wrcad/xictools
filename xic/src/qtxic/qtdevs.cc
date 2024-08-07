
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

#include "qtdevs.h"
#include "sced.h"
#include "edit.h"
#include "cd_lgen.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_library.h"
#include "events.h"
#include "menu.h"
#include "ebtn_menu.h"
#include "errorlog.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/qtcanvas.h"

#include <QLayout>
#include <QMenuBar>
#include <QToolBar>
#include <QMenu>
#include <QAction>
#include <QToolButton>
#include <QGroupBox>
#include <QIcon>
#include <QPixmap>
#include <QMouseEvent>


//-----------------------------------------------------------------------------
// QTdevMenuDlg::This implements a menu of devices from the device
// library, in three styles.  The drop-down menu styles take less
// screen space and the categories variation (vs.  alphabetic) is the
// default.  The graphical palette might be more useful to beginners.
// Called from the Electrical side menu devs button.
//
// Help system keywords used:
//  dev:xxxxx   (device name)

#ifdef Q_OS_MACOS
#define USE_QTOOLBAR
#endif

// The Device Menu popup control function.
//
void
cSced::PopUpDevs(GRobject caller, ShowMode mode)
{
    if (!QTdev::exists() || !QTmainwin::exists())
        return;
    if (mode == MODE_ON) {
        if (QTdevMenuDlg::self()) {
            QTdevMenuDlg::self()->activate(true);
            return;
        }
    }
    else if (mode == MODE_OFF) {
        delete QTdevMenuDlg::self();
        SCD()->setShowDevs(false);
        return;
    }
    else if (mode == MODE_UPD) {
        if (!QTdevMenuDlg::self())
            return;
        bool wasactive = QTdevMenuDlg::self()->is_active();
        caller = QTdevMenuDlg::self()->get_caller();
        delete QTdevMenuDlg::self();
        if (!wasactive)
            return;
    }

    stringlist *wl = FIO()->GetLibNamelist(XM()->DeviceLibName(), LIBdevice);
    if (!wl) {
        Log()->ErrorLog(mh::Initialization,
            "No device library found.\n"
            "You must exit and properly install the device.lib file,\n"
            "or set XIC_LIB_PATH in the environment.");
        return;
    }

    new QTdevMenuDlg(caller, wl);
    stringlist::destroy(wl);

    QTdevMenuDlg::self()->set_transient_for(QTmainwin::self());
    QTdev::self()->SetPopupLocation(GRloc(LW_UL), QTdevMenuDlg::self(),
        QTmainwin::self()->Viewport());
    QTdevMenuDlg::self()->show();
}


// Reset the currently selected device colors in the toolbar.
// Called on exit from XM()->LPlace().
//
void
cSced::DevsEscCallback()
{
    if (QTdevMenuDlg::self())
        QTdevMenuDlg::self()->esc();
}
// End of cSced functions.


// This is the name of a dummy cell that if found in the device library
// that will enable mutual inductors.
#define MUT_DUMMY "mut"

namespace {
    // XPM
    const char * more_xpm[] = {
    "32 25 3 1",
    " 	c none",
    ".	c black",
    "x	c blue",
    "                                ",
    "               .                ",
    "               ..               ",
    "               .x.              ",
    "               .xx.             ",
    "               .xxx.            ",
    "               .xxxx.           ",
    "               .xxxxx.          ",
    "      ..........xxxxxx.         ",
    "      .xxxxxxxxxxxxxxxx.        ",
    "      .xxxxxxxxxxxxxxxxx.       ",
    "      .xxxxxxxxxxxxxxxxxx.      ",
    "      .xxxxxxxxxxxxxxxxxxx.     ",
    "      .xxxxxxxxxxxxxxxxxx.      ",
    "      .xxxxxxxxxxxxxxxxx.       ",
    "      .xxxxxxxxxxxxxxxx.        ",
    "      ..........xxxxxx.         ",
    "               .xxxxx.          ",
    "               .xxxx.           ",
    "               .xxx.            ",
    "               .xx.             ",
    "               .x.              ",
    "               ..               ",
    "               .                ",
    "                                "};

    // XPM
    const char * dd_xpm[] = {
    "32 25 3 1",
    " 	c none",
    ".	c black",
    "x	c sienna",
    "                                ",
    "   ..........................   ",
    "   .          .         .   .   ",
    "   .          . ....... .   .   ",
    "   ............         .....   ",
    "              .         .       ",
    "              .  xx  xx .       ",
    "              . x xx  x .       ",
    "              .         .       ",
    "              .         .       ",
    "              . x xx xx .       ",
    "              .  xx xx  .       ",
    "              .         .       ",
    "              .         .       ",
    "              .  xx x x .       ",
    "              . xx x x  .       ",
    "              .         .       ",
    "              .         .       ",
    "              . x xx x  .       ",
    "              .  xx x x .       ",
    "              .         .       ",
    "              ...........       ",
    "                                ",
    "                                ",
    "                                "};

    // XPM
    const char * dda_xpm[] = {
    "32 25 3 1",
    " 	c none",
    ".	c black",
    "x	c sienna",
    "                                ",
    "   ..........................   ",
    "   .          .         .   .   ",
    "   .          . ....... .   .   ",
    "   ............         .....   ",
    "              .         .       ",
    "              .  xx  xx .       ",
    "              . x xx  x .       ",
    "              .         .       ",
    "              .         .       ",
    "              . x xx xx .       ",
    "      .       .  xx xx  .       ",
    "     ...      .         .       ",
    "     . .      .         .       ",
    "    .. ..     .  xx x x .       ",
    "    .....     . xx x x  .       ",
    "   ..   ..    .         .       ",
    "   ..   ..    .         .       ",
    "  ...   ...   . x xx x  .       ",
    "              .  xx x x .       ",
    "              .         .       ",
    "              ...........       ",
    "                                ",
    "                                ",
    "                                "};

    // XPM
    const char * pict_xpm[] = {
    "32 24 3 1",
    " 	c none",
    ".	c black",
    "x	c sienna",
    "                                ",
    "                                ",
    "   ..........................   ",
    "   xxxxxxxxxxxxxxxxxxxxxxxxxx   ",
    "       xx             xx        ",
    "       xx          .  xx        ",
    "       xx          .  xx        ",
    "       xx          .  xx        ",
    "       xx      .....  xx        ",
    "       xx    . ..     xx        ",
    "       xx    . ..     xx        ",
    "       xx .... ..     xx        ",
    "       xx    . ..     xx        ",
    "       xx    . ..     xx        ",
    "       xx      .....  xx        ",
    "       xx          .  xx        ",
    "       xx          .  xx        ",
    "       xx          .  xx        ",
    "       xx             xx        ",
    "   xxxxxxxxxxxxxxxxxxxxxxxxxx   ",
    "   ..........................   ",
    "                                ",
    "                                ",
    "                                "};
}

// Characteristic device size (window space).
#define DEVSIZE (11*CDelecResolution)

// Positional parameters (viewport space).
#define CELL_SIZE 60
#define SPA 6


namespace {
    // Comparison function for device names.
    //
    bool comp_func(const char *p, const char *q)
    {
        while (*p) {
            char c = islower(*p) ? toupper(*p) : *p;
            char d = islower(*q) ? toupper(*q) : *q;
            if (c != d)
                return (c < d);
            p++;
            q++;
        }
        return (q != 0);
    }
}


QTdevMenuDlg *QTdevMenuDlg::instPtr;

QTdevMenuDlg::QTdevMenuDlg(GRobject caller, stringlist *wl) :
    QTbag(this), QTdraw(XW_DEFAULT)
{
    instPtr = this;
    dv_caller = caller;
    dv_morebtn = 0;
    dv_entries = 0;
    dv_numdevs = 0;
    dv_leftindx = 0;
    dv_leftofst = 0;
    dv_rightindx = 0;
    dv_curdev = 0;
    dv_pressed = 0;
    dv_px = 0;
    dv_py = 0;
    dv_width = 0;
    dv_foreg = 0;
    dv_backg = 0;
    dv_hlite = 0;
    dv_selec = 0;
    dv_active = false;

    stringlist::sort(wl, comp_func);

    setWindowTitle(tr("Device Palette"));
    setAttribute(Qt::WA_DeleteOnClose);

    const char *type = CDvdb()->getVariable(VA_DevMenuStyle);

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    if (type && *type == '0' + dvMenuPict) {
        dv_type = dvMenuPict;
        gd_viewport = new QTcanvas();

        QFont *fnt;
        if (Fnt()->getFont(&fnt, FNT_SCREEN))
            gd_viewport->set_font(fnt);
        connect(QTfont::self(), &QTfont::fontChanged,
            this, &QTdevMenuDlg::font_changed_slot, Qt::QueuedConnection);

        connect(gd_viewport, &QTcanvas::resize_event,
            this, &QTdevMenuDlg::resize_slot);
        connect(gd_viewport, &QTcanvas::press_event,
            this, &QTdevMenuDlg::button_down_slot);
        connect(gd_viewport, &QTcanvas::release_event,
            this, &QTdevMenuDlg::button_up_slot);
        connect(gd_viewport, &QTcanvas::motion_event,
            this, &QTdevMenuDlg::motion_slot);
        connect(gd_viewport, &QTcanvas::enter_event,
            this, &QTdevMenuDlg::enter_slot);
        connect(gd_viewport, &QTcanvas::leave_event,
            this, &QTdevMenuDlg::leave_slot);

        int i = stringlist::length(wl);
        dv_entries = new sEnt[i];
        dv_numdevs = i;
        dv_leftindx = 0;
        i = 0;
        for (stringlist *ww = wl; ww; i++, ww = ww->next) {
            if (OIfailed(CD()->OpenExisting(ww->string, 0)) &&
                    OIfailed(FIO()->OpenLibCell(XM()->DeviceLibName(),
                    ww->string, LIBdevice, 0))) {
                continue;
            }
            dv_entries[i].name = lstring::copy(ww->string);
        }
        dv_width = init_sizes();

        QHBoxLayout *hbox = new QHBoxLayout(this);
        hbox->setContentsMargins(qmtop);
        hbox->setSpacing(2);
        hbox->addWidget(gd_viewport);

        QVBoxLayout *vbox = new QVBoxLayout();
        hbox->addLayout(vbox);
        vbox->setContentsMargins(qm);
        vbox->setSpacing(2);

        dv_morebtn = new QToolButton();
        dv_morebtn->setIcon(QIcon(QPixmap(more_xpm)));
        dv_morebtn->setMaximumWidth(80);
        vbox->addWidget(dv_morebtn);
        connect(dv_morebtn, &QAbstractButton::clicked,
            this, &QTdevMenuDlg::more_btn_slot);

        QToolButton *btn = new QToolButton();
        btn->setIcon(QIcon(QPixmap(dd_xpm)));
        btn->setMaximumWidth(80);
        vbox->addWidget(btn);
        connect(btn, &QAbstractButton::clicked,
            this, &QTdevMenuDlg::style_btn_slot);

        dv_backg = QTbag::PopupColor(GRattrColorDvBg).rgb();
        dv_foreg = QTbag::PopupColor(GRattrColorDvFg).rgb();
        dv_hlite = QTbag::PopupColor(GRattrColorDvHl).rgb();
        dv_selec = QTbag::PopupColor(GRattrColorDvSl).rgb();
        dv_pressed = -1;
        dv_curdev = -1;

        dv_active = true;
        return;
    }

//    gtk_window_set_resizable(GTK_WINDOW(wb_shell), false);

    if (type && *type == '0' + dvMenuAlpha) {
        dv_type = dvMenuAlpha;

        QVBoxLayout *vbox = new QVBoxLayout(this);
        vbox->setContentsMargins(qmtop);
        vbox->setSpacing(2);

        QGroupBox *gb = new QGroupBox();
        vbox->addWidget(gb);
        QHBoxLayout *hbox = new QHBoxLayout(gb);
        hbox->setContentsMargins(qm);
        hbox->setSpacing(2);

#ifdef USE_QTOOLBAR
        QToolBar *menubar = new QToolBar();
#else
        QMenuBar *menubar = new QMenuBar();
#endif
        hbox->addWidget(menubar);

        char lastc = 0;
        QMenu *menu = 0;
        for (stringlist *ww = wl; ww; ww = ww->next) {
            if (OIfailed(CD()->OpenExisting(ww->string, 0)) &&
                    OIfailed(FIO()->OpenLibCell(XM()->DeviceLibName(),
                    ww->string, LIBdevice, 0)))
                continue;
            char c = *ww->string;
            if (islower(c))
                c = toupper(c);
            if (c != lastc) {
                char bf[4];
                bf[0] = c;
                bf[1] = '\0';

#ifdef USE_QTOOLBAR
                QAction *a = menubar->addAction(bf);
                menu = new QMenu();
                a->setMenu(menu);
                QToolButton *tb = dynamic_cast<QToolButton*>(
                    menubar->widgetForAction(a));
                if (tb)
                    tb->setPopupMode(QToolButton::InstantPopup);
#else
                menu = menubar->addMenu(bf);
#endif
                connect(menu, &QMenu::triggered,
                    this, &QTdevMenuDlg::menu_slot);
            }
            menu->addAction(ww->string);
            lastc = c;
        }

        QToolButton *btn = new QToolButton();
        btn->setIcon(QIcon(QPixmap(pict_xpm)));
        hbox->addWidget(btn);
        connect(btn, &QAbstractButton::clicked,
            this, &QTdevMenuDlg::style_btn_slot);
    }
    else {
        dv_type = dvMenuCateg;

        QVBoxLayout *vbox = new QVBoxLayout(this);
        vbox->setContentsMargins(qmtop);
        vbox->setSpacing(2);

        QHBoxLayout *hbox = new QHBoxLayout();
        vbox->addLayout(hbox);
        hbox->setContentsMargins(qm);
        hbox->setSpacing(2);

#ifdef USE_QTOOLBAR
        QToolBar *menubar = new QToolBar();
        QAction *a = menubar->addAction(tr("Devices"));
        QMenu *menu_d = new QMenu();
        a->setMenu(menu_d);
        QToolButton *tb =
            dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
        if (tb)
            tb->setPopupMode(QToolButton::InstantPopup);
        connect(menu_d, &QMenu::triggered,
            this, &QTdevMenuDlg::menu_slot);

        a = menubar->addAction(tr("Sources"));
        QMenu *menu_s = new QMenu();
        a->setMenu(menu_s);
        tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
        if (tb)
            tb->setPopupMode(QToolButton::InstantPopup);
        connect(menu_s, &QMenu::triggered,
            this, &QTdevMenuDlg::menu_slot);

        a = menubar->addAction(tr("Macros"));
        QMenu *menu_m = new QMenu();
        a->setMenu(menu_m);
        tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
        if (tb)
            tb->setPopupMode(QToolButton::InstantPopup);
        connect(menu_m, &QMenu::triggered,
            this, &QTdevMenuDlg::menu_slot);

        a = menubar->addAction(tr("Terminals"));
        QMenu *menu_t = new QMenu();
        a->setMenu(menu_t);
        tb = dynamic_cast<QToolButton*>(menubar->widgetForAction(a));
        if (tb)
            tb->setPopupMode(QToolButton::InstantPopup);
        connect(menu_t, &QMenu::triggered,
            this, &QTdevMenuDlg::menu_slot);
#else
        QMenuBar *menubar = new QMenuBar();

        QMenu *menu_d = menubar->addMenu(tr("Devices"));
        connect(menu_d, &QMenu::triggered,
            this, &QTdevMenuDlg::menu_slot);
        QMenu *menu_s = menubar->addMenu(tr("Sources"));
        connect(menu_s, &QMenu::triggered,
            this, &QTdevMenuDlg::menu_slot);
        QMenu *menu_m = menubar->addMenu(tr("Macros"));
        connect(menu_m, &QMenu::triggered,
            this, &QTdevMenuDlg::menu_slot);
        QMenu *menu_t = menubar->addMenu(tr("Terminals"));
        connect(menu_t, &QMenu::triggered,
            this, &QTdevMenuDlg::menu_slot);
#endif
        hbox->addWidget(menubar);

        for (stringlist *ww = wl; ww; ww = ww->next) {
            CDcbin cbin;
            if (OIfailed(CD()->OpenExisting(ww->string, &cbin)) &&
                    OIfailed(FIO()->OpenLibCell(XM()->DeviceLibName(),
                    ww->string, LIBdevice, &cbin)))
                continue;
            if (!cbin.elec())
                continue;

            QMenu *menu = 0;
            CDpfxName namestr;
            CDelecCellType et = cbin.elec()->elecCellType(&namestr);
            switch (et) {
            case CDelecBad:
                break;
            case CDelecNull:
                // This should include the "mut" pseudo-device.
                menu = menu_d;
                break;
            case CDelecGnd:
            case CDelecTerm:
                menu = menu_t;
                break;
            case CDelecDev:
                {
                    char c = namestr ? *Tstring(namestr) : 0;
                    if (isupper(c))
                        c = tolower(c);
                    switch (c) {
                    case 'e':
                    case 'f':
                    case 'g':
                    case 'h':
                    case 'i':
                    case 'v':
                        menu = menu_s;
                        break;
                    case 'x':
                        // shouldn't get here
                        menu = menu_m;
                        break;
                    default:
                        menu = menu_d;
                        break;
                    }
                }
                break;
            case CDelecMacro:
            case CDelecSubc:
                menu = menu_m;
                break;
            }
            if (menu)
                menu->addAction(ww->string);
        }

        QToolButton *btn = new QToolButton();
        btn->setIcon(QIcon(QPixmap(dda_xpm)));
        hbox->addWidget(btn);
        connect(btn, &QAbstractButton::clicked,
            this, &QTdevMenuDlg::style_btn_slot);
    }
    dv_active = true;
}


QTdevMenuDlg::~QTdevMenuDlg()
{
    instPtr = 0;
    if (dv_caller)
        QTdev::SetStatus(dv_caller, false);
    delete [] dv_entries;
}


QSize
QTdevMenuDlg::sizeHint() const
{
    if (dv_type == dvMenuPict)
        return (QSize(dv_width, CELL_SIZE + 4));
    if (dv_type == dvMenuCateg)
        return (QSize(300, 24));
    if (dv_type == dvMenuAlpha)
        return (QSize(500, 24));
    return (QSize(-1, -1));
}


void
QTdevMenuDlg::activate(bool active)
{
    // Keep the saved position relative to the main window, absolute
    // coordinates can cause trouble with multi-monitors.

    if (active) {
        if (!dv_active) {
            QPoint pmain = QTmainwin::self()->pos();
            move(dv_px + pmain.x(), dv_py + pmain.y());
            show();
            dv_active = true;
        }
    }
    else {
        if (dv_active) {
            QPoint pmain = QTmainwin::self()->pos();
            QPoint px = pos();
            dv_px = px.x() - pmain.x();
            dv_py = px.y() - pmain.y();
            hide();
            dv_active = false;
            if (dv_caller)
                QTdev::SetStatus(dv_caller, false);
        }
    }
}


void
QTdevMenuDlg::esc()
{
    if (dv_type == dvMenuPict) {
        if (dv_pressed >= 0)
            render_cell(dv_pressed, false);
        dv_pressed = -1;
    }
}


// These functions are used only in the pictorial mode.
//

// Initialize the widths of the display areas for each device, return the
// total width.
//
int
QTdevMenuDlg::init_sizes()
{
    double ratio = (double)(CELL_SIZE - 2*SPA)/DEVSIZE;
    int left = 2*SPA;

    for (int i = 0; i < dv_numdevs; i++) {
        CDcbin cbin;
        if (OIfailed(CD()->OpenExisting(dv_entries[i].name, &cbin)))
            continue;
        CDs *sdesc = cbin.elec();
        if (!sdesc)
            continue;
        CDs *syr = sdesc->symbolicRep(0);
        if (syr)
            sdesc = syr;
        sdesc->computeBB();
        BBox BB = *sdesc->BB();
        dv_entries[i].x = left - SPA;
        dv_entries[i].width = (int)(BB.width()*ratio) + 2*SPA;
        int sw = QTfont::stringWidth(dv_entries[i].name, FNT_SCREEN);
        if (dv_entries[i].width < sw)
            dv_entries[i].width = sw;
        left += dv_entries[i].width + 2*SPA;
    }
    if (!QTmainwin::self())
        return (0);

    int wid = QTmainwin::self()->Viewport()->width();
    left += 40 + SPA;  // Button width is approx 40.
    if (left < wid)
        wid = left;
    if (wid <= 0)
        // all lib cells are empty, shouldn't happen
        wid = 100;
    return (wid);
}


// Render the library device in the toolbar.
//
void
QTdevMenuDlg::render_cell(int which, bool selected)
{
    SetColor(selected ? dv_selec : dv_backg);
    CDcbin cbin;
    if (OIfailed(CD()->OpenExisting(dv_entries[which].name, &cbin)))
        return;
    CDs *sdesc = cbin.elec();
    if (!sdesc)
        return;
    CDs *syr = sdesc->symbolicRep(0);
    if (syr)
        sdesc = syr;
    sdesc->computeBB();
    BBox BB = *sdesc->BB();
    int xx = (BB.left + BB.right)/2;
    int yy = (BB.bottom + BB.top)/2;

    int vp_height = CELL_SIZE - 2*SPA;
    int vp_width = dv_entries[which].width + 2;

    WindowDesc wd;
    wd.Attrib()->set_display_labels(Electrical, SLupright);
    wd.SetWbag(DSP()->MainWdesc()->Wbag());
    wd.SetWdraw(this);

    wd.SetRatio(((double)vp_height)/DEVSIZE);
    int hei = (int)(vp_height/wd.Ratio());
    wd.Window()->top = yy + hei/2;
    wd.Window()->bottom = yy - hei/2;
    // Shift a little for text placement compensation.
    wd.Window()->left = BB.left - (int)(SPA/wd.Ratio());
    wd.Window()->right = BB.right - (int)(SPA/wd.Ratio());

    wd.InitViewport(vp_width, vp_height);
    *wd.ClipRect() = wd.Viewport();

    gd_viewport->switch_to_pixmap2();

    SetColor(selected ? dv_selec : dv_backg);
    SetBackground(selected ? dv_selec : dv_backg);
    SetFillpattern(0);
    Box(0, 0, vp_width - 1, vp_height - 1);

    DSP()->TPush();
    SetColor(dv_foreg);
    CDl *ld;
    CDlgen lgen(DSP()->CurMode());
    while ((ld = lgen.next()) != 0) {
        if (ld->isInvisible() || ld->isNoInstView())
            continue;
        CDg gdesc;
        gdesc.init_gen(sdesc, ld);
        CDo *pointer;
        while ((pointer = gdesc.next()) != 0) {
            if (pointer->type() == CDBOX)
                wd.ShowBox(&pointer->oBB(), ld->getAttrFlags(),
                    dsp_prm(ld)->fill());
            else if (pointer->type() == CDPOLYGON) {
                const Poly po(((const CDpo*)pointer)->po_poly());
                wd.ShowPolygon(&po, ld->getAttrFlags(), dsp_prm(ld)->fill(),
                    &pointer->oBB());
            }
            else if (pointer->type() == CDWIRE) {
                const Wire w(((const CDw*)pointer)->w_wire());
                wd.ShowWire(&w, 0, 0);
            }
            else if (pointer->type() == CDLABEL) {
                const Label la(((const CDla*)pointer)->la_label());
                wd.ShowLabel(&la);
            }
        }
    }
    int fwid, fhei;
    TextExtent(dv_entries[which].name, &fwid, &fhei);
    xx = 0;
    yy = vp_height - fhei;
    SetColor(selected ? dv_selec : dv_backg);
    BBox tBB(xx, yy+fhei, xx+fwid, yy);
    Box(tBB.left, tBB.bottom, tBB.right,
        tBB.bottom - (abs(tBB.height())*8)/10);
    SetColor(dv_hlite);
    Text(dv_entries[which].name, tBB.left, tBB.bottom, 0,
        tBB.width(), abs(tBB.height()));
    DSP()->TPop();

    int xoff = dv_entries[which].x - dv_leftofst - 2;
    int yoff = SPA;
    gd_viewport->switch_from_pixmap2(xoff, yoff, 0, 0, vp_width, vp_height);
}


// Function to show another box full of devices, called in 'more'
// mode when there are too many devices to fit at once.
//
void
QTdevMenuDlg::cyclemore()
{
    Clear();
    if (dv_rightindx == dv_numdevs)
        dv_leftindx = 0;
    else
        dv_leftindx = dv_rightindx;
    redraw();
}


// Return the device index of the device under the pointer, or
// -1 if the pointer is not over a device.
//
int
QTdevMenuDlg::whichent(int xx)
{
    xx += dv_leftofst;
    for (int i = dv_leftindx; i < dv_rightindx; i++) {
        if (dv_entries[i].x <= xx &&
                xx <= dv_entries[i].x + dv_entries[i].width)
            return (i);
    }
    return (-1);
}


// Outline the device as selected.  This happens when the pointer is
// over the device.
//
void
QTdevMenuDlg::show_selected(int which)
{
    if (which < 0)
        return;
    int w = dv_entries[which].width + SPA;
    int h = CELL_SIZE - SPA;

    SetLinestyle(0);
    SetFillpattern(0);

    GRmultiPt xp(3);
    xp.assign(0, dv_entries[which].x - SPA/2 - dv_leftofst, SPA/2 + h);
    xp.assign(1, xp.at(0).x, xp.at(0).y - h);
    xp.assign(2, xp.at(1).x + w, xp.at(1).y);
    SetColor(0xcccccc);
    PolyLine(&xp, 3);

    xp.assign(0, dv_entries[which].x - SPA/2 - dv_leftofst, SPA/2 + h);
    xp.assign(1, xp.at(0).x + w, xp.at(0).y);
    xp.assign(2, xp.at(1).x, xp.at(1).y - h);
    SetColor(0x888888);
    PolyLine(&xp, 3);
    gd_viewport->repaint(dv_entries[which].x - SPA/2 - dv_leftofst, SPA/2, w+1, h+1);
}


// Outline the device as unselected, the normal state.
//
void
QTdevMenuDlg::show_unselected(int which)
{
    if (which < 0)
        return;
    int w = dv_entries[which].width + SPA;
    int h = CELL_SIZE - SPA;

    SetLinestyle(0);
    SetFillpattern(0);

    GRmultiPt xp(3);
    xp.assign(0, dv_entries[which].x - SPA/2 - dv_leftofst, SPA/2 + h);
    xp.assign(1, xp.at(0).x, xp.at(0).y - h);
    xp.assign(2, xp.at(1).x + w, xp.at(1).y);
    SetColor(0x888888);
    PolyLine(&xp, 3);

    xp.assign(0, dv_entries[which].x - SPA/2 - dv_leftofst, SPA/2 + h);
    xp.assign(1, xp.at(0).x + w, xp.at(0).y);
    xp.assign(2, xp.at(1).x, xp.at(1).y - h);
    SetColor(0xcccccc);
    PolyLine(&xp, 3);
    gd_viewport->repaint(dv_entries[which].x - SPA/2 - dv_leftofst, SPA/2, w+1, h+1);
}


void
QTdevMenuDlg::redraw()
{
    dv_leftofst = dv_entries[dv_leftindx].x - SPA;
    int wid = gd_viewport->width();
    int i;
    for (i = dv_leftindx; i < dv_numdevs; i++) {
        if (dv_entries[i].x - dv_leftofst +
                dv_entries[i].width + SPA > wid)
            break;
    }
    if (i == dv_numdevs && dv_leftindx == 0) {
        if (dv_morebtn->isVisible())
            dv_morebtn->hide();
    }
    else {
        if (!dv_morebtn->isVisible())
            dv_morebtn->show();
    }
    dv_rightindx = i;

    for (i = dv_leftindx; i < dv_rightindx; i++)
        render_cell(i, i == dv_pressed);
    gd_viewport->update();
    for (i = dv_leftindx; i < dv_rightindx; i++)
        show_unselected(i);
}


void
QTdevMenuDlg::menu_slot(QAction *a)
{
    // Menu handler (MenuCateg and MenuAlpha only).
    QByteArray name_ba = a->text().toLatin1();
    const char *string = name_ba.constData();
    if (XM()->IsDoingHelp()) {
        char tbuf[128];
        snprintf(tbuf, sizeof(tbuf), "dev:%s", string);
        DSPmainWbag(PopUpHelp(tbuf))
        return;
    }
    // Give focus to main window.
    QTmainwin::self()->activateWindow();
    EV()->InitCallback();
    if (!strcmp(string, MUT_DUMMY)) {
        CmdDesc cmd;
        cmd.caller = a;
        cmd.wdesc = DSP()->MainWdesc();
        SCD()->showMutualExec(&cmd);
    }
    else
        ED()->placeDev(0, string, false);
}


void
QTdevMenuDlg::style_btn_slot()
{
    // Switch to the alternate presentation style.
    switch (dv_type) {
    case dvMenuCateg:
        CDvdb()->setVariable(VA_DevMenuStyle, "1");
        break;
    case dvMenuAlpha:
        CDvdb()->setVariable(VA_DevMenuStyle, "2");
        break;
    case dvMenuPict:
        CDvdb()->clearVariable(VA_DevMenuStyle);
        break;
    }
}


void
QTdevMenuDlg::more_btn_slot()
{
    // Show the next page of entries (Pictorial only).
    cyclemore();
}


void
QTdevMenuDlg::font_changed_slot(int fnum)
{
    if (fnum == FNT_SCREEN) {
        QFont *fnt;
        if (Fnt()->getFont(&fnt, FNT_SCREEN))
            gd_viewport->set_font(fnt);
        // Compute new field widths on font change.
        init_sizes();
    }
}


void
QTdevMenuDlg::button_down_slot(QMouseEvent *ev)
{
    // Draw/redraw the toolbar.
    // Select the device clicked on, and call the function to allow
    // device placement.
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int n = whichent((int)ev->position().x());
#else
    int n = whichent(ev->x());
#endif
    if (n < 0)
        return;
    dv_curdev = n;
    if (XM()->IsDoingHelp()) {
        char tbuf[128];
        snprintf(tbuf, sizeof(tbuf), "dev:%s", dv_entries[n].name);
        DSPmainWbag(PopUpHelp(tbuf))
        return;
    }
    if (dv_pressed >= dv_leftindx && dv_pressed < dv_rightindx)
        render_cell(dv_pressed, false);
    int lp = dv_pressed;
    EV()->InitCallback();  // sets pressed to -1
    if (lp != n) {
        // Give focus to main window.
        QTmainwin::self()->activateWindow();
        render_cell(n, true);
        dv_pressed = n;
        if (!strcmp(dv_entries[n].name, MUT_DUMMY)) {
            CmdDesc cmd;
            cmd.caller = 0;
            cmd.wdesc = DSP()->MainWdesc();
            SCD()->showMutualExec(&cmd);
        }
        else
            ED()->placeDev(0, dv_entries[n].name, false);
    }
}


void
QTdevMenuDlg::button_up_slot(QMouseEvent*)
{
}


void
QTdevMenuDlg::motion_slot(QMouseEvent *ev)
{
    // Change the border surrounding the device under the pointer.
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    int n = whichent((int)ev->position().x());
#else
    int n = whichent(ev->x());
#endif
    if (n != dv_curdev)
        show_unselected(dv_curdev);
    dv_curdev = n;
    show_selected(dv_curdev);
}


void
QTdevMenuDlg::enter_slot(QEnterEvent *ev)
{
    // On entering the toolbar, change the border of the device.
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    dv_curdev = whichent((int)ev->position().x());
#else
    dv_curdev = whichent(ev->x());
#endif
    show_selected(dv_curdev);
}


void
QTdevMenuDlg::leave_slot(QEvent*)
{
    // On leaving the toolbar, return the border to normal.
    show_unselected(dv_curdev);
}


void
QTdevMenuDlg::resize_slot(QResizeEvent*)
{
    redraw();
}

