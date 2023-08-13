
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
 * QtInterf Graphical Interface Library                                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "qtinterf.h"
#include "qtfile.h"
#include "qtfont.h"
#include "qtaffirm.h"
#include "qtinput.h"
#include "qtlist.h"
#include "qtfont.h"
#include "qtmsg.h"
#include "qttext.h"
#include "qtnumer.h"
#include "qthcopy.h"
#include "qtedit.h"
#include "qttimer.h"
#include "qtidleproc.h"

#include "help/help_defs.h"

#include <QApplication>
#include <QAction>
#include <QPushButton>
#include <QCheckBox>
#include <QRadioButton>
#include <QMenu>
#include <QScreen>
#include <QWindow>


// Device-dependent setup.
//
void
GRpkg::DevDepInit(unsigned int cfg)
{
    if (cfg & _devQT_)
        GRpkg::self()->RegisterDevice(new QTdev);
}


//-----------------------------------------------------------------------------
// QTdev methods

QTdev *QTdev::instancePtr = 0;

QTdev::QTdev()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class QTdev is already instantiated.\n");
        exit (1);
    }
    instancePtr     = this;
    name            = "QT";
    ident           = _devQT_;
    devtype         = GRmultiWindow;

    dv_loop         = 0;
    dv_main_bag     = 0;
    dv_timers       = 0;
    dv_idle_ctrl    = 0;
    dv_minx         = 0;
    dv_miny         = 0;
    dv_loop_level   = 0;
}


QTdev::~QTdev()
{
    instancePtr = 0;
}


// Private static error exit.
//
void
QTdev::on_null_ptr()
{
    fprintf(stderr, "Singleton class QTdev used before insgtantiated.\n");
    exit(1);
}


bool
QTdev::Init(int *argc, char **argv)
{
    if (!QApplication::instance()) {
        static int ac = *argc;
        // QApplications takes a reference as first arg, must not be on stack
        new QApplication(ac, argv);
//XXX
printf("new QApp\n");
        *argc = ac;
    }

    FC.initFonts();

    // set correct information
    width = 1;
    height = 1;
    // application specific code must reset these
    numcolors = 256;
    numlinestyles = 0;
    return (false);
}


bool
QTdev::InitColormap(int, int, bool)
{
    return (false);
}


void
QTdev::RGBofPixel(int pixel, int *r, int *g, int *b)
{
    QColor c(pixel);
    *r = c.red();
    *g = c.green();
    *b = c.blue();
}


int
QTdev::AllocateColor(int *pp, int r, int g, int b)
{
    QColor c(r, g, b);
    *pp = c.rgb();
    return (0);
}


// Return the pixel that is the closest match to colorname.  Also handle
// decimal triples.
//
int
QTdev::NameColor(const char *colorname)
{
    if (colorname && *colorname) {
        int r, g, b;
        if (sscanf(colorname, "%d %d %d", &r, &g, &b) == 3) {
            if (r >= 0 && r <= 255 && g >= 0 && g <= 255 &&
                    b >= 0 && b <= 255) {
                QColor c(r, g, b);
                return (c.rgb());
            }
            else if (r >= 0 && r <= 65535 && g >= 0 && g <= 65535 &&
                    b >= 0 && b <= 65535) {
                QColor c(r/256, g/256, b/256);
                return (c.rgb());
            }
            else
                return (0);
        }
        QColor c(colorname);
        if (c.isValid())
            return (c.rgb());
        if (GRcolorList::lookupColor(colorname, &r, &g, &b)) {
            // My list is much more complete.
            QColor nc(r, g, b);
            return (nc.rgb());
        }
    }
    return (0);
}


// Set indices[3] to the rgb of the named color.
//
bool
QTdev::NameToRGB(const char *colorname, int *indices)
{
    indices[0] = 0;
    indices[1] = 0;
    indices[2] = 0;
    if (colorname && *colorname) {
        int r, g, b;
        if (sscanf(colorname, "%d %d %d", &r, &g, &b) == 3) {
            if (r >= 0 && r <= 255 && g >= 0 && g <= 255 &&
                    b >= 0 && b <= 255) {
                indices[0] = r;
                indices[1] = g;
                indices[2] = b;
                return (true);
            }
            else if (r >= 0 && r <= 65535 && g >= 0 && g <= 65535 &&
                    b >= 0 && b <= 65535) {
                indices[0] = r/256;
                indices[1] = g/256;
                indices[2] = b/256;
                return (true);
            }
            else
                return (false);
        }
        QColor c(colorname);
        if (c.isValid()) {
            indices[0] = c.red();
            indices[1] = c.green();
            indices[2] = c.blue();
            return (true);
        }
        if (GRcolorList::lookupColor(colorname, &r, &g, &b)) {
            // My list is much more complete.
            indices[0] = r;
            indices[1] = g;
            indices[2] = b;
            return (true);
        }
        fprintf(stderr, "Color %s unknown, setting to black.\n", colorname);
    }
    return (false);
}


GRdraw *
QTdev::NewDraw(int apptype)
{
    return (new QTdraw(apptype));
}


GRwbag *
QTdev::NewWbag(const char*, GRwbag *reuse)
{
    if (!reuse)
        reuse = new QTbag();
    return (reuse);
}


// Install a timer, return its ID.
//
int
QTdev::AddTimer(int ms, int(*cb)(void*), void *arg)
{
    dv_timers = new QTtimer(cb, arg, dv_timers, 0);
    dv_timers->set_use_return(true);
    dv_timers->register_list(&dv_timers);
    dv_timers->start(ms);
    return (dv_timers->id());
}


// Remove installed timer.
//
void
QTdev::RemoveTimer(int id)
{
    for (QTtimer *t = dv_timers; t; t = t->nextTimer()) {
        if (t->id() == id) {
            delete t;
            return;
        }
    }
}


// Install an idle procedure, return its ID.  The calback will be called
// repeatedly until it returns 0.
//
int
QTdev::AddIdleProc(int(*cb)(void*), void *arg)
{
    if (!dv_idle_ctrl)
        dv_idle_ctrl = new QTidleproc();
    return (dv_idle_ctrl->add(cb, arg));
}


// Remove an existing idle procedure by id.
//
void
QTdev::RemoveIdleProc(int id)
{
    if (dv_idle_ctrl)
        dv_idle_ctrl->remove(id);
}


// Interrupt handling.
//
void(*
QTdev::RegisterSigintHdlr(void(*)()) )()
{
    return (0);
}


// This function dispatches any pending events, and is called periodically
// by the hardcopy drivers.  If the return value is true, a ^C was typed,
// otherwise false is returned.
//
bool
QTdev::CheckForEvents()
{
    QApplication::instance()->sendPostedEvents();
    QApplication::instance()->processEvents();
    return (false);
}


// Grab a char.  If fd1 or fd2 >= 0, expect input from the stream(s)
// as well.  Return the fd which supplied the char.
//
int
QTdev::Input(int, int, int*)
{
    return (0);
}


// Start a signal handling loop.
//
void
QTdev::MainLoop(bool)
{
    if (!dv_loop_level) {
        dv_loop_level = 1;
        QApplication::instance()->exec();
        return;
    }
    dv_loop_level++;
    dv_loop = new event_loop(dv_loop);
    dv_loop->exec();
    event_loop *tloop = dv_loop;
    dv_loop = dv_loop->next;
    delete tloop;
    dv_loop_level--;
}


void
QTdev::BreakLoop()
{
    if (dv_loop)
        dv_loop->exit();
}


// Write a status/error message on the go button popup, called from
// the graphics drivers.
//
void
QTdev::HCmessage(const char *str)
{
    if (GRpkg::self()->MainWbag()) {
        QTbag *w = dynamic_cast<QTbag*>(GRpkg::self()->MainWbag());
        if (w && w->HC()) {
            if (GRpkg::self()->CheckForEvents()) {
                GRpkg::self()->HCabort("User aborted");
                str = "ABORTED";
            }
            w->HC()->set_message(str);
        }
    }
}
// End of virtual overrides.


// Remaining functions are unique to class.
//


// Return the root window coordinates x+width, y of obj.
//
void
QTdev::Location(GRobject obj, int *xx, int *yy)
{
    if (obj) {
        QObject *o = (QObject*)obj;
        if (o->isWidgetType()) {
            QWidget *widget = dynamic_cast<QWidget*>(o);
            if (widget) {
                QPoint pt = widget->mapToGlobal(QPoint(0, 0));
                *xx = pt.x() + widget->width();
                *yy = pt.y();
                return;
            }
        }
        else {
            QAction *a = dynamic_cast<QAction*>(o);
            QMenu *menu = a ? a->menu() : 0;
            if (menu) {
                QPoint pt = menu->mapToGlobal(QPoint(0, 0));
                *xx = pt.x() + menu->width();
                *yy = pt.y();
                return;
            }
        }
    }
    *xx = 0;
    *yy = 0;
}


// Move widget into postions according to loc.  MUST be called on
// visible widget.
//
void
QTdev::SetPopupLocation(GRloc loc, QWidget *widget, QWidget *shell)
{
    if (!widget || !shell)
        return;
    int x, y;
    ComputePopupLocation(loc, widget, shell, &x, &y);
    QScreen *sc = shell->screen();
    QSize scsz = sc->availableSize();
    QSize wsz = widget->size();
    if (x + wsz.width() > scsz.width())
        x = scsz.width() - wsz.width();
    if (y + wsz.height() > scsz.height())
        y = scsz.height() - wsz.height();
    widget->move(x, y);
}


// Find the position of widget according to loc.  MUST be called on
// visible widget.
//
void
QTdev::ComputePopupLocation(GRloc loc, QWidget *widget, QWidget *shell,
    int *px, int *py)
{
    *px = 0;
    *py = 0;
    if (!widget || !shell)
        return;

    // If the widget was just created and not shown yet, the size may
    // be off unless this is called.
    widget->adjustSize();

    if (shell->x() < 0) {
        // Seems to be a QT bug - sometimes shell->x()/y() return -1 until
        // the window is moved.  This seems to fix the problem.
        QPoint pt = shell->mapToGlobal(QPoint(0, 0));
        shell->move(pt);
    }

    int xo = 0, yo = 0;
    int dx = 0, dy = 0;
    if (!shell->isAncestorOf(widget)) {
        QWidget *parent = shell->parentWidget();
        if (parent) {
                QPoint pt = parent->mapToGlobal(QPoint(0, 0));
                xo = pt.x();
                yo = pt.y();
            while (parent->parentWidget())
                parent = parent->parentWidget();

            // Have to add the window manager frame to the widget
            // dimensions.  Can't call frameGeometry yet since the
            // window manager probably has not added decorations yet,
            // so infer the size change from the parent.

            dy = parent->frameGeometry().height() - parent->height();
            dx = parent->frameGeometry().width() - parent->width();
        }
    }
    if (loc.code == LW_LL) {
        *px = xo + shell->x();
        *py = yo + shell->y() + shell->height() - (widget->height() + dy);
    }
    else if (loc.code == LW_LR) {
        *px = xo + shell->x() + shell->width() - (widget->width() + dx);
        *py = yo + shell->y() + shell->height() - (widget->height() + dy);
    }
    else if (loc.code == LW_UL) {
        *px = xo + shell->x();
        *py = yo + shell->y();
    }
    else if (loc.code == LW_UR) {
        *px = xo + shell->x() + shell->width() - (widget->width() + dx);
        *py = yo + shell->y();
    }
    else if (loc.code == LW_CENTER) {
        *px = xo + shell->x() + (shell->width() - (widget->width() + dx))/2;
        *py = yo + shell->y() + (shell->height() - (widget->height() + dy))/2;
    }
    else if (loc.code == LW_XYR) {
        *px = xo + shell->x() + loc.xpos;
        *py = yo + shell->y() + loc.ypos;
    }
    else if (loc.code == LW_XYA) {
        *px = loc.xpos;
        *py = loc.ypos;
    }
}


// Static function.
// Set the state of obj to unselected, suppress the signal.
//
void
QTdev::Deselect(GRobject obj)
{
    QObject *o = (QObject*)obj;
    if (!o)
        return;
    if (o->isWidgetType()) {
        QPushButton *btn = dynamic_cast<QPushButton*>(o);
        if (btn && btn->isCheckable()) {
            btn->setChecked(false);
            return;
        }
        QCheckBox *cb = dynamic_cast<QCheckBox*>(o);
        if (cb) {
            cb->setChecked(false);
            return;
        }
        QRadioButton *rb = dynamic_cast<QRadioButton*>(o);
        if (rb) {
            rb->setChecked(false);
            return;
        }
    }
    else {
        QAction *a = dynamic_cast<QAction*>(o);
        if (a && a->isCheckable())
            a->setChecked(false);
    }
}


// Static function.
// Set the state of obj to selected, suppress the signal.
//
void
QTdev::Select(GRobject obj)
{
    QObject *o = (QObject*)obj;
    if (!o)
        return;
    if (o->isWidgetType()) {
        QPushButton *btn = dynamic_cast<QPushButton*>(o);
        if (btn && btn->isCheckable()) {
            btn->setChecked(true);
            return;
        }
        QCheckBox *cb = dynamic_cast<QCheckBox*>(o);
        if (cb) {
            cb->setChecked(true);
            return;
        }
        QRadioButton *rb = dynamic_cast<QRadioButton*>(o);
        if (rb) {
            rb->setChecked(true);
            return;
        }
    }
    else {
        QAction *a = dynamic_cast<QAction*>(o);
        if (a && a->isCheckable())
            a->setChecked(true);
    }
}


// Static function.
// Return the status of obj.
//
bool
QTdev::GetStatus(GRobject obj)
{
    QObject *o = (QObject*)obj;
    if (o) {
        if (o->isWidgetType()) {
            QPushButton *btn = dynamic_cast<QPushButton*>(o);
            if (btn && btn->isCheckable())
                return (btn->isChecked());
            QCheckBox *cb = dynamic_cast<QCheckBox*>(o);
            if (cb)
                return (cb->isChecked());
            QRadioButton *rb = dynamic_cast<QRadioButton*>(o);
            if (rb)
                return (rb->isChecked());
        }
        else {
            QAction *a = dynamic_cast<QAction*>(o);
            if (a && a->isCheckable())
                return (a->isChecked());
        }
    }
    // Return true if not a toggle.
    return (true);
}


// Static function.
// Set the status of obj, suppress the signal.
//
void
QTdev::SetStatus(GRobject obj, bool state)
{
    QObject *o = (QObject*)obj;
    if (!o)
        return;
    if (o->isWidgetType()) {
        QPushButton *btn = dynamic_cast<QPushButton*>(o);
        if (btn && btn->isCheckable()) {
            btn->setChecked(state);
            return;
        }
        QCheckBox *cb = dynamic_cast<QCheckBox*>(o);
        if (cb) {
            cb->setChecked(state);
            return;
        }
        QRadioButton *rb = dynamic_cast<QRadioButton*>(o);
        if (rb) {
            rb->setChecked(state);
            return;
        }
    }
    else {
        QAction *a = dynamic_cast<QAction*>(o);
        if (a && a->isCheckable())
            a->setChecked(state);
    }
}


// Static function.
void
QTdev::CallCallback(GRobject obj)
{
    QObject *o = (QObject*)obj;
    if (!o)
        return;
    if (!IsSensitive(obj))
        return;
    if (o->isWidgetType()) {
        QPushButton *btn = dynamic_cast<QPushButton*>(o);
        if (btn)
            btn->click();
    }
    else {
        QAction *a = dynamic_cast<QAction*>(o);
        if (a)
            a->trigger();
    }
}


// Static function.
// Return the label string of the button, accelerators stripped.  Do not
// free the return.
//
const char *
QTdev::GetLabel(GRobject obj)
{
    static char buf[32];
    if (obj) {
        QObject *o = (QObject*)obj;
        QString qs;
        if (o->isWidgetType()) {
            QPushButton *btn = dynamic_cast<QPushButton*>(o);
            if (btn)
                qs = btn->text();
            else {
                QCheckBox *cb = dynamic_cast<QCheckBox*>(o);
                if (cb)
                    qs = cb->text();
                else {
                    QRadioButton *rb = dynamic_cast<QRadioButton*>(o);
                    if (rb)
                        qs = rb->text();
                }
            }
        }
        else {
            QAction *a = dynamic_cast<QAction*>(o);
            if (a)
                qs = a->text();
        }
        int n = qs.size();
        if (n > 0) {
            // Need to strip the '&' if present.
            QByteArray b = qs.toLatin1();
            int i = 0;
            for (int j = 0; j < n; j++) {
                if (b[j] != '&') {
                    buf[i++] = b[j];
                    if (i == sizeof(buf)-1)
                        break;
                }
            }
            buf[i] = 0;
            return (buf);
        }
    }
    return (0);
}


// Static function.
// Set the label of the button
//
void
QTdev::SetLabel(GRobject obj, const char *text)
{
    QObject *o = (QObject*)obj;
    if (!o)
        return;
    if (o->isWidgetType()) {
        QPushButton *btn = dynamic_cast<QPushButton*>(o);
        if (btn) {
            btn->setText(text);
            return;
        }
        QCheckBox *cb = dynamic_cast<QCheckBox*>(o);
        if (cb) {
            cb->setText(text);
            return;
        }
        QRadioButton *rb = dynamic_cast<QRadioButton*>(o);
        if (rb) {
            rb->setText(text);
            return;
        }
    }
    else {
        QAction *a = dynamic_cast<QAction*>(o);
        if (a)
            a->setText(text);
    }
}


// Static function.
void
QTdev::SetSensitive(GRobject obj, bool sens_state)
{
    QObject *o = (QObject*)obj;
    if (!o)
        return;
    if (o->isWidgetType()) {
        QPushButton *btn = dynamic_cast<QPushButton*>(o);
        if (btn) {
            btn->setEnabled(sens_state);
            return;
        }
        QCheckBox *cb = dynamic_cast<QCheckBox*>(o);
        if (cb) {
            cb->setEnabled(sens_state);
            return;
        }
        QRadioButton *rb = dynamic_cast<QRadioButton*>(o);
        if (rb) {
            rb->setEnabled(sens_state);
            return;
        }
        QMenu *menu = dynamic_cast<QMenu*>(o);
        if (menu)
            menu->setEnabled(sens_state);
    }
    else {
        QAction *a = dynamic_cast<QAction*>(o);
        if (a)
            a->setEnabled(sens_state);
    }
}


// Static function.
bool
QTdev::IsSensitive(GRobject obj)
{
    QObject *o = (QObject*)obj;
    if (!o)
        return (false);
    if (o->isWidgetType()) {
        QPushButton *btn = dynamic_cast<QPushButton*>(o);
        if (btn)
            return (btn->isEnabled());
        QCheckBox *cb = dynamic_cast<QCheckBox*>(o);
        if (cb)
            return (cb->isEnabled());
        QRadioButton *rb = dynamic_cast<QRadioButton*>(o);
        if (rb)
            return (rb->isEnabled());
        QMenu *menu = dynamic_cast<QMenu*>(o);
        if (menu)
            return (menu->isEnabled());
    }
    else {
        QAction *a = dynamic_cast<QAction*>(o);
        if (a)
            return (a->isEnabled());
    }
    return (false);
}


// Static function.
void
QTdev::SetVisible(GRobject obj, bool vis_state)
{
    QObject *o = (QObject*)obj;
    if (!o)
        return;
    if (o->isWidgetType()) {
        QPushButton *btn = dynamic_cast<QPushButton*>(o);
        if (btn) {
            if (vis_state)
                btn->show();
            else
                btn->hide();
            return;
        }
        QCheckBox *cb = dynamic_cast<QCheckBox*>(o);
        if (cb) {
            if (vis_state)
                cb->show();
            else
                cb->hide();
            return;
        }
        QRadioButton *rb = dynamic_cast<QRadioButton*>(o);
        if (rb) {
            if (vis_state)
                rb->show();
            else
                rb->hide();
            return;
        }
        QMenu *menu = dynamic_cast<QMenu*>(o);
        if (menu) {
            if (vis_state)
                menu->show();
            else
                menu->hide();
        }
    }
    else {
        QAction *a = dynamic_cast<QAction*>(o);
        if (a)
            a->setVisible(vis_state);
    }
}


// Static function.
bool
QTdev::IsVisible(GRobject obj)
{
    QObject *o = (QObject*)obj;
    if (!o)
        return (false);
    if (o->isWidgetType()) {
        QPushButton *btn = dynamic_cast<QPushButton*>(o);
        if (btn)
            return (btn->isVisible());
        QCheckBox *cb = dynamic_cast<QCheckBox*>(o);
        if (cb)
            return (cb->isVisible());
        QRadioButton *rb = dynamic_cast<QRadioButton*>(o);
        if (rb)
            return (rb->isVisible());
        QMenu *menu = dynamic_cast<QMenu*>(o);
        if (menu)
            return (menu->isVisible());
    }
    else {
        QAction *a = dynamic_cast<QAction*>(o);
        if (a)
            return (a->isVisible());
    }
    return (false);
}


// Static function.
void
QTdev::DestroyButton(GRobject obj)
{
    QObject *o = (QObject*)obj;
    if (!o)
        return;
    if (o->isWidgetType()) {
        QPushButton *btn = dynamic_cast<QPushButton*>(o);
        if (btn) {
            btn->deleteLater();
            return;
        }
        QCheckBox *cb = dynamic_cast<QCheckBox*>(o);
        if (cb) {
            cb->deleteLater();
            return;
        }
        QRadioButton *rb = dynamic_cast<QRadioButton*>(o);
        if (rb) {
            rb->deleteLater();
            return;
        }
        QMenu *menu = dynamic_cast<QMenu*>(o);
        if (menu) {
            menu->deleteLater();
            return;
        }
    }
    else {
        QAction *a = dynamic_cast<QAction*>(o);
        if (a)
            delete a;
    }
}


// Static function.
// Return the connection file descriptor.
//
int
QTdev::ConnectFd()
{
    return (-1);
}


// Static function.
// Return the pointer position in root window coordinates
//
void
QTdev::PointerRootLoc(int *xx, int *yy)
{
    QPoint ptg(QCursor::pos());
    if (xx)
        *xx = ptg.x();
    if (yy)
        *yy = ptg.y();
}
// End of QTdev functions.


//-----------------------------------------------------------------------------
// QTbag methods

const char *QTbag::wb_open_folder_xpm[] = {
    "16 16 12 1",
    "   c None",
    ".  c #808080",
    "+  c #E0E0D0",
    "@  c #4F484F",
    "#  c #909000",
    "$  c #FFF8EF",
    "%  c #CFC860",
    "&  c #003090",
    "*  c #7F7800",
    "=  c #FFC890",
    "-  c #FFF890",
    ";  c #2F3000",
    "        .       ",
    "       .+@      ",
    "   ###.$$+@     ",
    "  #%%.$$$$+@    ",
    "  #%.$$$&$$+@** ",
    "  #.+++&+&+++@* ",
    "############++@ ",
    "#$$$$$$$$$=%#++@",
    "#$-------=-=#+; ",
    " #---=--=-==%#; ",
    " #-----=-=-==#; ",
    " #-=--=-=-=-=#; ",
    "  #=-=-=-=-==#; ",
    "  ############; ",
    "   ;;;;;;;;;;;  ",
    "                "
};

const char *QTbag::wb_closed_folder_xpm[] = {
    "16 16 8 1",
    "   c None",
    ".  c #909000",
    "+  c #000000",
    "@  c #EFE8EF",
    "#  c #FFF8CF",
    "$  c #FFF890",
    "%  c #CFC860",
    "&  c #FFC890",
    "                ",
    "  .....+        ",
    " .@##$$.+       ",
    ".%%%%%%%......  ",
    ".###########$%+ ",
    ".#$$$$$$$$$$&%+ ",
    ".#$$$$$$$&$&$%+ ",
    ".#$$$$$$$$&$&%+ ",
    ".#$$$$$&$&$&$%+ ",
    ".#$$$$$$&$&$&%+ ",
    ".#$$$&$&$&$&&%+ ",
    ".#&$&$&$&$&&&%+ ",
    ".%%%%%%%%%%%%%+ ",
    " ++++++++++++++ ",
    "                ",
    "                "
};

QTbag::QTbag()
{
    wb_shell = 0;
    wb_input = 0;
    wb_message = 0;
    wb_info = 0;
    wb_info2 = 0;
    wb_htinfo = 0;
    wb_warning = 0;
    wb_error = 0;
    wb_fontsel = 0;
    wb_hc = 0;
    wb_call_data = 0;
    wb_sens_set = 0;
    wb_warn_cnt = 1;
    wb_err_cnt = 1;
    wb_info_cnt = 1;
    wb_info2_cnt = 1;
    wb_htinfo_cnt = 1;
}

QTbag::~QTbag()
{
    ClearPopups();
    HcopyDisableMsgs();
    delete wb_hc;
}

// NOTE:
// In QT, widgets that require QTbag features inherit QTbag, so that
// the shell field is always a pointer-to-self.  It is crucial that
// widgets that subclass QTbag set the shell pointer in the
// constructor.


void
QTbag::Title(const char *title, const char *icontitle)
{
    if (title && wb_shell)
        wb_shell->setWindowTitle(QString(title));
    if (icontitle && wb_shell)
        wb_shell->setWindowIconText(QString(icontitle));
}


// Pop up the editor.  If the resuse_ret arg is given, it should be a
// pointer to a QTbag returned from QTdev::NewWbag(), which will be
// used to set up a top-level window.  Otherwise, the editor is set
// up normally, i.e., within an existing hierarchy.
//
GReditPopup *
QTbag::PopUpTextEditor(const char *fname,
    bool (*editsave)(const char*, void*, XEtype), void *arg, bool source)
{
    QTeditDlg *text = new QTeditDlg(this, QTeditDlg::Editor, fname,
        source, arg);
    text->register_callback(editsave);
    text->set_visible(true);
    return (text);
}


// Pop up the file for browsing (read only, no load or source) under an
// existing widget hierarchy.
//
GReditPopup *
QTbag::PopUpFileBrowser(const char *fname)
{
/*
    // If we happen to already have this file open, reread it.
    // Called after something was appended to the file.
    for (interface::svptr *s = widgets; s; s = s->nextItem()) {
        if (s->itemType() == interface::svText) {
            QWidget *w = ItemFromTicket(s->itemTicket());
            QTeditDlg *tw = dynamic_cast<QTeditDlg*>(w);
            if (tw && tw->get_widget_type() == QTeditPopup::Browser) {
                const char *file = tw->get_file();
                if (fname && file && !strcmp(fname, file)) {
                    tw->load_file(fname);
                    return (tw);
                }
            }
        }
    }
*/
    QTeditDlg *text = new QTeditDlg(this, QTeditDlg::Browser, fname,
        false, 0);
    text->set_visible(true);
    return (text);
}


// Edit the string, rather than a file.  The string arg is the initial
// value of the string to edit.  When done, callback is called with a
// copy of the new string and arg.  If callback returns true, the
// widget is destroyed.  Callback is also called on quit with a 0
// string argument.
//
GReditPopup *
QTbag::PopUpStringEditor(const char *string,
    bool (*callback)(const char*, void*, XEtype), void *arg)
{
/*
    if (!callback) {
        // pop down and destroy all string editor windows
        for (interface::svptr *s = widgets; s; s = s->nextItem()) {
            if (s->itemType() == interface::svText) {
                QWidget *w = ItemFromTicket(s->itemTicket());
                QTeditPopup *tw = dynamic_cast<QTeditPopup*>(w);
                if (tw && tw->get_widget_type() == QTeditPopup::StringEditor)
                    delete tw;
            }
        }
        return;
    }
*/

    QTeditDlg *text = new QTeditDlg(this, QTeditDlg::StringEditor,
        string, false, arg);
    text->register_callback(callback);
    text->set_visible(true);
    return (text);
}


// This is a simple mail editor, calls mail_proc with the xed_bag
// when done.  The downproc, if given, is called before destruction.
// See the description of PopUpTextEditor for interpretation of the
// reuse_ret arg.  When given, only the mailaddr and subject args are
// recognized.
//
GReditPopup *
QTbag::PopUpMail(const char *subject, const char *mailaddr,
    void(*downproc)(GReditPopup*), GRloc loc)
{
    QTeditDlg *text = new QTeditDlg(this, QTeditDlg::Mailer, 0,
        false, 0);
    if (subject && *subject)
        text->set_mailsubj(subject);
    if (mailaddr && *mailaddr)
        text->set_mailaddr(mailaddr);
    text->register_quit_callback(downproc);
    text->set_visible(true);
    QTdev::self()->SetPopupLocation(loc, text, wb_shell);
    return (text);
}


// Pop up the font selector.
//  caller      initiating button
//  loc         positioning
//  mode        MODE_ON or MODE_OFF
//  btns        0-terminated list of button names, may be 0
//  cb          Callback function for btns
//  arg         Callback function user arg
//  indx        If >= 1, update the GTKfont for this index
//  labeltext   Alternative text for label
//
void
QTbag::PopUpFontSel(GRobject caller, GRloc loc, ShowMode mode,
    void(*)(const char*, const char*, void*), void *arg, int indx,
    const char**, const char*)
{
    if (mode == MODE_ON) {
        if (wb_fontsel)
            return;
        wb_fontsel = new QTfontDlg(this, indx, arg);
        wb_fontsel->register_caller(caller, false, true);
        wb_fontsel->show();
        wb_fontsel->raise();
        wb_fontsel->activateWindow();
        QTdev::self()->SetPopupLocation(loc, wb_fontsel, wb_shell);
    }
    else {
        if (!wb_fontsel)
            return;
        delete wb_fontsel;
    }
}


// Printing support.  Caller is the initiating button, if any.
//
void
QTbag::PopUpPrint(GRobject, HCcb *cb, HCmode mode, GRdraw*)
{
// gtk impl has this one line
//XXX    GTKprintPopup::hc_hcpopup(caller, this, cb, mode, context);

    if (wb_hc) {
        bool active = !wb_hc->is_active();
        wb_hc->set_active(active);
        if (wb_hc->callbacks() && wb_hc->callbacks()->hcsetup) {
            (*wb_hc->callbacks()->hcsetup)(active, wb_hc->format_index(),
                false, 0);
        }
        return;
    }

    // This will set this->hc if successful,
    QTprintDlg *pd = new QTprintDlg(cb, mode, this);

    if (!wb_hc)
        delete pd;
    else
        wb_hc->show();
}


// Function to query values for command text, resolution, etc.
// The HCcb struct is filled in with the present values.
//
void
QTbag::HCupdate(HCcb *cb, GRobject)
{
    if (wb_hc)
        wb_hc->update(cb);
//XXX gtk impl has long block of code, equiv?
}


// Change the format selection of the Print panel.
//
void
QTbag::HCsetFormat(int)
{
    //XXX fixme
// gtk impl has one line
//XXX    GTKprintPopup::hc_set_format(this, fmt, true);
}


// Remove from the message dispatch list.  Messages to this window
// will not be shown.
//
void
QTbag::HcopyDisableMsgs()
{
    /* gtk impl equiv?
    Mlist.remove(this);
    */
    if (wb_hc)
        wb_hc->disable_progress();
}


GRfilePopup *
QTbag::PopUpFileSelector(FsMode mode, GRloc loc,
    void(*cb)(const char*, void*), void(*down_cb)(GRfilePopup*, void*),
    void *arg, const char *root_or_fname)
{
    QTfileDlg *fsel = new QTfileDlg(this, mode, arg, root_or_fname);
    fsel->register_callback(cb);
    fsel->register_quit_callback(down_cb);
    fsel->set_visible(true);
    QTdev::self()->SetPopupLocation(loc, fsel, wb_shell);
    return (fsel);
}


//
// Utilities
//

void
QTbag::ClearPopups()
{
    if (wb_message)
        wb_message->popdown();
    if (wb_input)
        wb_input->popdown();
    if (wb_warning)
        wb_warning->popdown();
    if (wb_error)
        wb_error->popdown();
    if (wb_info)
        wb_info->popdown();
    if (wb_info2)
        wb_info2->popdown();
    if (wb_htinfo)
        wb_htinfo->popdown();
    GRpopup *p;
    while ((p = wb_monitor.first_object()) != 0)
        p->popdown();
}


// Clean up after a pop-up is destroyed, called from destructors.
//
void
QTbag::ClearPopup(GRpopup *popup)
{
    MonitorRemove(popup);
    if (popup == wb_input) {
        wb_input = 0;
        if (wb_sens_set)
            (*wb_sens_set)(this, true, 0);
    }
    else if (popup == wb_message) {
        if (wb_message->is_desens() && wb_input)
            wb_input->setEnabled(true);
        wb_message = 0;
    }
    else if (popup == wb_info) {
        wb_info = 0;
        wb_info_cnt++;
    }
    else if (popup == wb_info2) {
        wb_info2 = 0;
        wb_info2_cnt++;
    }
    else if (popup == wb_htinfo) {
        wb_htinfo = 0;
        wb_htinfo_cnt++;
    }
    else if (popup == wb_warning) {
        wb_warning = 0;
        wb_warn_cnt++;
    }
    else if (popup == wb_error) {
        wb_error = 0;
        wb_err_cnt++;
    }
    else if (popup == wb_fontsel) {
        wb_fontsel = 0;
    }
}


GRaffirmPopup *
QTbag::PopUpAffirm(GRobject caller, GRloc loc, const char *question_str,
    void(*action_callback)(bool, void*), void *action_arg)
{
    QTaffirmDlg *affirm = new QTaffirmDlg(this, question_str);
    affirm->register_caller(caller, false, true);
    affirm->register_callback(action_callback);
    affirm->set_callback_arg(action_arg);
    affirm->set_visible(true);
    QTdev::self()->SetPopupLocation(loc, affirm, wb_shell);
    return (affirm);
}


// Popup to solicit a numeric value, consisting of a label and a spin
// button.
//
GRnumPopup *
QTbag::PopUpNumeric(GRobject caller, GRloc loc, const char *prompt_str,
    double initd, double mind, double maxd, double del, int numd,
    void(*action_callback)(double, bool, void*), void *action_arg)
{
    QTnumDlg *numer = new QTnumDlg(this, prompt_str, initd, mind, maxd,
        del, numd);
    numer->register_caller(caller, false, true);
    numer->register_callback(action_callback);
    numer->set_callback_arg(action_arg);
    numer->set_visible(true);
    QTdev::self()->SetPopupLocation(loc, numer, wb_shell);
    return (numer);
}


// Simple popup to solicit a text string.  Arg caller, if given, is the
// initiating button, and will destroy the popup if pressed while the
// popup is active.  Args x,y are shell coordinates for the popup.
// When finished, action_callback will be called with the string
// (malloced) and action_arg.  Arg field is the width of the input field.
// Arg widgetp, if given, is a location to hold the cancel widget.  If
// given and it holds a value, the existing value is used to pop down
// the old widget before creating the new one.  Arg downproc is a
// function executed when the widget is destroyed, its arg is true
// if popping down from OK button, false otherwise.
//
// If x,y are both < 0, the popup will be centered on the calling shell.

GRledPopup *
QTbag::PopUpEditString(GRobject caller, GRloc loc, const char *prompt_string,
    const char *init_str, ESret(*action_callback)(const char*, void*),
    void *action_arg, int textwidth, void(*downproc)(bool),
    bool multiline, const char *btnstr)
{
    QTledDlg *inp = new QTledDlg(this, prompt_string, init_str, btnstr,
        multiline);
    inp->register_caller(caller, false, true);
    inp->register_callback((GRledPopup::GRledCallback)action_callback);
    inp->set_callback_arg(action_arg);
    inp->register_quit_callback(downproc);
    if (textwidth < 150)
        textwidth = 150;
    inp->setMinimumWidth(textwidth);
    inp->set_visible(true);
    QTdev::self()->SetPopupLocation(loc, inp, wb_shell);
    return (inp);
}


// PopUpInput and PopUpMessage implement an input solicitation widget
// with error reporting.  Only one of each widget is possible per
// parent QTbag as they set fields in the QTbag struct.

void
QTbag::PopUpInput(const char *label_str, const char *initial_str,
    const char *action_str, void(*action_callback)(const char*, void*),
    void *arg, int textwidth)
{
    if (wb_input)
        delete wb_input;
    wb_input = new QTledDlg(this, label_str, initial_str, action_str, false);
    wb_input->register_callback((GRledPopup::GRledCallback)action_callback);
    wb_input->set_callback_arg(arg);
    wb_input->set_ignore_return(true);
    if (textwidth < 150)
        textwidth = 150;
    wb_input->setMinimumWidth(textwidth);
    if (wb_sens_set)
        // Handle desentizing widgets while pop-up is active.
        (*wb_sens_set)(this, false, 0);
    wb_input->set_visible(true);
}


// Pop up a message box. If err is true, the popup will get the
// resources of an error popup.  If multi is true, the widget will not
// use the QTbag::message field, so that there can be arbitrarily
// many of these, but the user must keep track of them.  If desens is
// true, then any popup pointed to by QTbag::input is desensitized
// while the message is active.  This is done only if multi is false.
//
GRmsgPopup *
QTbag::PopUpMessage(const char *string, bool err, bool desens,
    bool multi, GRloc loc)
{
    if (!multi && wb_message)
        wb_message->popdown();

    QTmsgDlg *mesg = new QTmsgDlg(this, string, err, STY_NORM);
    if (!multi)
        wb_message = mesg;
    QTdev::self()->SetPopupLocation(loc, wb_message, wb_shell);

    if (desens && !multi && wb_input) {
        mesg->set_desens();
        wb_input->setEnabled(false);
    }
    mesg->set_visible(true);
    return (multi ? mesg : 0);
}


//XXX
int
QTbag::PopUpWarn(ShowMode mode, const char *message_str, STYtype style,
    GRloc loc)
{
    if (mode == MODE_OFF) {
        delete wb_error;
        return (0);
    }
    if (mode == MODE_UPD) {
        if (wb_error)
            return (wb_err_cnt);
        return (0);
    }
    if (wb_error) {
        wb_error->setText(message_str);
        wb_error->set_visible(true);
    }
    else {
        wb_error = new QTtextDlg(this, message_str, QTtextDlg::PuWarn, style);
        wb_error->set_visible(true);
        QTdev::self()->SetPopupLocation(loc, wb_error, wb_shell);
    }
    return (wb_err_cnt);
}


// The next few functions pop up and down error and info popups.  The
// shell and text widgets are stored in the QTbag struct, so there
// can be only one of each type per QTbag.  Additional calls to
// PopUpxxx() simply update the text.
//
// The functions return in integer index, which is incremented when
// the window is explicitly popped down.  Calling with mode ==
// MODE_UPD also returns the index, and can be used to determine if
// the "same" window is still visible.

int
QTbag::PopUpErr(ShowMode mode, const char *message_str, STYtype style,
    GRloc loc)
{
    if (mode == MODE_OFF) {
        delete wb_error;
        return (0);
    }
    if (mode == MODE_UPD) {
        if (wb_error)
            return (wb_err_cnt);
        return (0);
    }
    if (wb_error) {
        wb_error->setText(message_str);
        wb_error->set_visible(true);
    }
    else {
        wb_error = new QTtextDlg(this, message_str, QTtextDlg::PuErr, style);
        wb_error->set_visible(true);
        QTdev::self()->SetPopupLocation(loc, wb_error, wb_shell);
    }
    return (wb_err_cnt);
}


GRtextPopup *
QTbag::PopUpErrText(const char *message_str, STYtype style, GRloc loc)
{
    QTtextDlg *mesg = new QTtextDlg(this, message_str, QTtextDlg::PuErrAlso, style);
    mesg->set_visible(true);
    QTdev::self()->SetPopupLocation(loc, mesg, wb_shell);
    return (mesg);
}


int
QTbag::PopUpInfo(ShowMode mode, const char *msg, STYtype style, GRloc loc)
{
    if (mode == MODE_OFF) {
        delete wb_info;
        return (0);
    }
    if (mode == MODE_UPD) {
        if (wb_info)
            return (wb_info_cnt);
        return (0);
    }
    if (wb_info) {
        wb_info->setText(msg);
        wb_info->set_visible(true);
    }
    else {
        wb_info = new QTtextDlg(this, msg, QTtextDlg::PuInfo, style);
        wb_info->setTitle("Info");
        wb_info->set_visible(true);
        QTdev::self()->SetPopupLocation(loc, wb_info, wb_shell);
    }
    return (wb_info_cnt);
}


int
QTbag::PopUpInfo2(ShowMode mode, const char *msg, bool(*cb)(bool, void*),
    void *arg, STYtype style, GRloc loc)
{
    if (mode == MODE_OFF) {
        if (wb_info2)
            wb_info2->popdown();
        return (0);
    }
    if (mode == MODE_UPD) {
        if (wb_info2)
            return (wb_info2_cnt);
        return (0);
    }
    if (wb_info2) {
        wb_info2->setText(msg);
        wb_info2->set_visible(true);
    }
    else {
        wb_info2 = new QTtextDlg(this, msg, QTtextDlg::PuInfo2, style);
        wb_info2->set_visible(true);
        wb_info2->register_callback(cb);
        wb_info2->set_callback_arg(arg);
        QTdev::self()->SetPopupLocation(loc, wb_info2, wb_shell);
    }
    return (wb_info2_cnt);
}


int
QTbag::PopUpHTMLinfo(ShowMode mode, const char *msg, GRloc loc)
{
    if (mode == MODE_OFF) {
        delete wb_htinfo;
        return (0);
    }
    if (mode == MODE_UPD) {
        if (wb_htinfo)
            return (wb_htinfo_cnt);
        return (0);
    }
    if (wb_htinfo) {
        wb_htinfo->setText(msg);
        wb_htinfo->set_visible(true);
    }
    else {
        wb_htinfo = new QTtextDlg(this, msg, QTtextDlg::PuHTML, STY_HTML);
        wb_htinfo->set_visible(true);
        QTdev::self()->SetPopupLocation(loc, wb_htinfo, wb_shell);
    }
    return (wb_htinfo_cnt);
}


// Can probably put this in the header file, but then have to globally
// include a lot of stuff.
GRledPopup      *QTbag::ActiveInput()       { return (wb_input); }
GRmsgPopup      *QTbag::ActiveMessage()     { return (wb_message); }
GRtextPopup     *QTbag::ActiveInfo()        { return (wb_info); }
GRtextPopup     *QTbag::ActiveInfo2()       { return (wb_info2); }
GRtextPopup     *QTbag::ActiveHtinfo()      { return (wb_htinfo); }
GRtextPopup     *QTbag::ActiveWarn()        { return (wb_warning); }
GRtextPopup     *QTbag::ActiveError()       { return (wb_error); }
GRfontPopup     *QTbag::ActiveFontsel()     { return (wb_fontsel); }


void
QTbag::SetErrorLogName(const char *fname)
{
    QTtextDlg::set_error_log(fname);
}


// Static function.
// Return a QColor, initialized with the given attr color.  Used
// with the QTbag popups, main color functions are in QTdev.
//
QColor
QTbag::PopupColor(GRattrColor c)
{
    const char *colorname = GRpkg::self()->GetAttrColor(c);
    if (colorname && *colorname) {
        int r, g, b;
        if (sscanf(colorname, "%d %d %d", &r, &g, &b) == 3) {
            if (r >= 0 && r <= 255 && g >= 0 && g <= 255 &&
                    b >= 0 && b <= 255) {
                return (QColor(r, g, b));
            }
            else if (r >= 0 && r <= 65535 && g >= 0 && g <= 65535 &&
                    b >= 0 && b <= 65535) {
                return (QColor(r/256, g/256, b/256));
            }
            else
                return (QColor("black"));
        }
        QColor qc(colorname);
        if (qc.isValid())
            return (qc);
        if (GRcolorList::lookupColor(colorname, &r, &g, &b)) {
            // My list is much more complete.
            QColor nc(r, g, b);
            return (nc);
        }
    }
    return (QColor("black"));
}

