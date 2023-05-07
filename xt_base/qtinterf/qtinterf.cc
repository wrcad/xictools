
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
#include "qtnumer.h"
#include "qthcopy.h"
#include "qtedit.h"
#include "interval_timer.h"

#include "help/help_defs.h"

#include <QApplication>


// Device-dependent setup.
//
void
GRpkg::DevDepInit(unsigned int cfg)
{
    if (cfg & _devQT_)
        GRpkgIf()->RegisterDevice(new QTdev);
}


//-----------------------------------------------------------------------------
// QTdev methods

QTdev * QTdev::instancePtr = 0;

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
        reuse = new QTbag(0);
    return (reuse);
}


// Install a timer, return its ID.
//
int
QTdev::AddTimer(int ms, int(*cb)(void*), void *arg)
{
//XXX
printf("new timer\n");
    dv_timers = new interval_timer(cb, arg, dv_timers, 0);
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
    for (interval_timer *t = dv_timers; t; t = t->nextTimer()) {
        if (t->id() == id) {
            delete t;
            return;
        }
    }
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
    if (GRpkgIf()->MainWbag()) {
        QTbag *w = dynamic_cast<QTbag*>(GRpkgIf()->MainWbag());
        if (w && w->hc) {
            if (GRpkgIf()->CheckForEvents()) {
                GRpkgIf()->HCabort("User aborted");
                str = "ABORTED";
            }
            w->hc->set_message(str);
        }
    }
}
// End of virtual overrides.


// Remaining functions are unique to class.
//

// Return the connection file descriptor.
//
int
QTdev::ConnectFd()
{
    return (-1);
}


// Set the state of btn to unselected, suppress the signal.
//
void
QTdev::Deselect(GRobject btn)
{
    if (!btn)
        return;
    /*
    if (GTK_IS_TOGGLE_BUTTON(btn)) {
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn))) {
            g_signal_connect(G_OBJECT(btn), "toggled",
                G_CALLBACK(toggle_btn_hdlr), (gpointer)0);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn), 0);
        }
    }
    else if (GTK_IS_CHECK_MENU_ITEM(btn)) {
        if (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(btn))) {
            g_signal_connect(G_OBJECT(btn), "toggled",
                G_CALLBACK(toggle_menu_hdlr), (gpointer)0);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(btn), 0);
        }
    }
    */
}


// Set the state of btn to selected, suppress the signal.
//
void
QTdev::Select(GRobject btn)
{
    if (!btn)
        return;
    /*
    if (GTK_IS_TOGGLE_BUTTON(btn)) {
        if (!gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn))) {
            g_signal_connect(G_OBJECT(btn), "toggled",
                G_CALLBACK(toggle_btn_hdlr), (gpointer)0);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn), 1);
        }
    }
    else if (GTK_IS_CHECK_MENU_ITEM(btn)) {
        if (!gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(btn))) {
            g_signal_connect(G_OBJECT(btn), "toggled",
                G_CALLBACK(toggle_menu_hdlr), (gpointer)0);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(btn), 1);
        }
    }
    */
}


// Return the status of btn.
//
bool
QTdev::GetStatus(GRobject btn)
{
    /*
    if (!btn)
        return (false);
    if (GTK_IS_TOGGLE_BUTTON(btn))
        return (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn)));
    if (GTK_IS_CHECK_MENU_ITEM(btn))
        return (gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(btn)));
    */
    return (true);
}


// Set the status of btn, suppress the signal.
//
void
QTdev::SetStatus(GRobject btn, bool state)
{
    if (!btn)
        return;
    /*
    if (GTK_IS_TOGGLE_BUTTON(btn)) {
        bool cur = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn));
        if (cur != state) {
            g_signal_connect(G_OBJECT(btn), "toggled",
                G_CALLBACK(toggle_btn_hdlr), (gpointer)0);
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn), state);
        }
    }
    else if (GTK_IS_CHECK_MENU_ITEM(btn)) {
        bool cur = gtk_check_menu_item_get_active(GTK_CHECK_MENU_ITEM(btn));
        if (cur != state) {
            g_signal_connect(G_OBJECT(btn), "toggled",
                G_CALLBACK(toggle_menu_hdlr), (gpointer)0);
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(btn), state);
        }
    }
    */
}


void
QTdev::CallCallback(GRobject obj)
{
    /*
    if (!obj)
        return;
    if (!IsSensitive(obj))
        return;
    if (GTK_IS_BUTTON(obj) || GTK_IS_TOGGLE_BUTTON(obj) ||
            GTK_IS_RADIO_BUTTON(obj))
        gtk_button_clicked(GTK_BUTTON(obj));
    else if (GTK_IS_MENU_ITEM(obj) || GTK_IS_CHECK_MENU_ITEM(obj) ||
            GTK_IS_RADIO_MENU_ITEM(obj))
        gtk_menu_item_activate(GTK_MENU_ITEM(obj));
    */
}


// Return the root window coordinates x+width, y of obj.
//
void
QTdev::Location(GRobject obj, int *x, int *y)
{
    *x = 0;
    *y = 0;
    /*
    if (obj && gtk_widget_get_window(GTK_WIDGET(obj))) {
        GdkWindow *wnd = gtk_widget_get_parent_window(GTK_WIDGET(obj));
        int rx, ry;
        gdk_window_get_origin(wnd, &rx, &ry);
        GtkAllocation a;
        gtk_widget_get_allocation(GTK_WIDGET(obj), &a);
        *x = rx + a.x + a.width;
        *y = ry + a.y;
        return;
    }
    GtkWidget *w = GTK_WIDGET(obj);
    while (w && gtk_widget_get_parent(w))
        w = gtk_widget_get_parent(w);
    if (gtk_widget_get_window(w)) {
        int rx, ry;
        gdk_window_get_origin(gtk_widget_get_window(w), &rx, &ry);
        if (x)
            *x = rx + 50;
        if (y)
            *y = ry + 50;
        return;
    }
    if (dv_default_focus_win) {
        int rx, ry;
        gdk_window_get_origin(dv_default_focus_win, &rx, &ry);
        if (x)
            *x = rx + 100;
        if (y)
            *y = ry + 300;
    }
    */
}


// Return the pointer position in root window coordinates
//
void
QTdev::PointerRootLoc(int *x, int *y)
{
    /*
    GdkModifierType state;
    GdkWindow *window = gdk_get_default_root_window();
    gdk_window_get_pointer(window, x, y, &state);
    */
}


// Return the label string of the button, accelerators stripped.  Do not
// free the return.
//
const char *
QTdev::GetLabel(GRobject btn)
{
    /*
    if (!btn)
        return (0);
    const char *string = 0;
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(btn));
    if (!child)
        return (0);
    if (GTK_IS_LABEL(child))
        string = gtk_label_get_label(GTK_LABEL(child));
    else if (GTK_IS_CONTAINER(child)) {
        GList *stuff = gtk_container_get_children(GTK_CONTAINER(child));
        for (GList *a = stuff; a; a = a->next) {
            GtkWidget *item = (GtkWidget*)a->data;
            if (GTK_IS_LABEL(item)) {
                string = gtk_label_get_label(GTK_LABEL(item));
                break;
            }
        }
        g_list_free(stuff);
    }
    return (string);
    */
    return (0);
}


// Set the label of the button
//
void
QTdev::SetLabel(GRobject btn, const char *text)
{
    /*
    if (!btn)
        return;
    GtkWidget *child = gtk_bin_get_child(GTK_BIN(btn));
    if (!child)
        return;
    if (GTK_IS_LABEL(child))
        gtk_label_set_text(GTK_LABEL(child), text);
    else if (GTK_IS_CONTAINER(child)) {
        GList *stuff = gtk_container_get_children(GTK_CONTAINER(child));
        for (GList *a = stuff; a; a = a->next) {
            GtkWidget *item = (GtkWidget*)a->data;
            if (GTK_IS_LABEL(item)) {
                gtk_label_set_text(GTK_LABEL(item), text);
                break;
            }
        }
        g_list_free(stuff);
    }
    */
}


void
QTdev::SetSensitive(GRobject obj, bool sens_state)
{
    /*
    if (obj)
        gtk_widget_set_sensitive((GtkWidget*)obj, sens_state);
    */
}


bool
QTdev::IsSensitive(GRobject obj)
{
    /*
    if (!obj)
        return (false);
    GtkWidget *w = GTK_WIDGET(obj);
    while (w) {
        if (!gtk_widget_get_sensitive(w))
            return (false);
        w = gtk_widget_get_parent(w);
    }
    */
    return (true);
}


void
QTdev::SetVisible(GRobject obj, bool vis_state)
{
    /*
    if (obj) {
        if (vis_state)
            gtk_widget_show((GtkWidget*)obj);
        else
            gtk_widget_hide((GtkWidget*)obj);
    }
    */
}


bool
QTdev::IsVisible(GRobject obj)
{
    /*
    if (!obj)
        return (false);
    GtkWidget *w = GTK_WIDGET(obj);
    while (w) {
        if (!gtk_widget_get_mapped(w))
            return (false);
        w = gtk_widget_get_parent(w);
    }
    */
    return (true);
}


void
QTdev::DestroyButton(GRobject obj)
{
    /*
    if (obj)
        gtk_widget_destroy(GTK_WIDGET(obj));
    */
}


// Move widget into postions according to loc.  MUST be caled on
// visible widget.
//
void
QTdev::SetPopupLocation(GRloc loc, QWidget *widget, QWidget *shell)
{
    if (!widget || !shell)
        return;
    int x, y;
    ComputePopupLocation(loc, widget, shell, &x, &y);
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
// End of QTdev functions.


//-----------------------------------------------------------------------------
// QTbag methods

QTbag::QTbag(QWidget *w)
{
    shell = w;
    input = 0;
    message = 0;
    info = 0;
    info2 = 0;
    htinfo = 0;
    error = 0;
    fontsel = 0;
    hc = 0;
    call_data = 0;
    sens_set = 0;
    err_cnt = 1;
    info_cnt = 1;
    info2_cnt = 1;
    htinfo_cnt = 1;
}

QTbag::~QTbag()
{
    ClearPopups();
    HcopyDisableMsgs();
    delete hc;
}

// NOTE:
// In QT, widgets that require QTbag features inherit QTbag, so that
// the shell field is always a pointer-to-self.  It is crucial that
// widgets that subclass QTbag set the shell pointer in the
// constructor.


void
QTbag::Title(const char *title, const char *icontitle)
{
    if (title && shell)
        shell->setWindowTitle(QString(title));
    if (icontitle && shell)
        shell->setWindowIconText(QString(icontitle));
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
    QTeditPopup *text = new QTeditPopup(this, QTeditPopup::Editor, fname,
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
            QTeditPopup *tw = dynamic_cast<QTeditPopup*>(w);
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
    QTeditPopup *text = new QTeditPopup(this, QTeditPopup::Browser, fname,
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

    QTeditPopup *text = new QTeditPopup(this, QTeditPopup::StringEditor,
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
    QTeditPopup *text = new QTeditPopup(this, QTeditPopup::Mailer, 0,
        false, 0);
    if (subject && *subject)
        text->set_mailsubj(subject);
    if (mailaddr && *mailaddr)
        text->set_mailaddr(mailaddr);
    text->register_quit_callback(downproc);
    text->set_visible(true);
    QTdev::self()->SetPopupLocation(loc, text, shell);
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
        if (fontsel)
            return;
        fontsel = new QTfontPopup(this, indx, arg);
        fontsel->register_caller(caller, false, true);
        fontsel->show();
        fontsel->raise();
        fontsel->activateWindow();
        QTdev::self()->SetPopupLocation(loc, fontsel, shell);
    }
    else {
        if (!fontsel)
            return;
        delete fontsel;
    }
}


// Printing support.  Caller is the initiating button, if any.
//
void
QTbag::PopUpPrint(GRobject, HCcb *cb, HCmode mode, GRdraw*)
{
    if (hc) {
        bool active = !hc->is_active();
        hc->set_active(active);
        if (hc->callbacks() && hc->callbacks()->hcsetup)
            (*hc->callbacks()->hcsetup)(active, hc->format_index(), false, 0);
        return;
    }

    // This will set this->hc if successful,
    QTprintPopup *pd = new QTprintPopup(cb, mode, this);

    if (!hc)
        delete pd;
    else
        hc->show();
}


// Function to query values for command text, resolution, etc.
// The HCcb struct is filled in with the present values.
//
void
QTbag::HCupdate(HCcb *cb, GRobject)
{
    if (hc)
        hc->update(cb);
}


void
QTbag::HCsetFormat(int)
{
    //XXX fixme
}


void
QTbag::HcopyDisableMsgs()
{
    if (hc)
        hc->disable_progress();
}


// This function is called if the main application window is
// reconfigured.  If the popup is active, true is returned, otherwise
// false.  The x, y entries set the location, and the wid and hei
// entries are filled in upon return.
//
bool
QTbag::HcopyLocate(int x, int y, int *wid, int *hei)
{
    *wid = 0;
    *hei = 0;
    if (hc && hc->is_active()) {
        *wid = hc->width();
        *hei = hc->height();
        hc->move(x, y);
        return (true);
    }
    return (false);
}


GRfilePopup *
QTbag::PopUpFileSelector(FsMode mode, GRloc loc,
    void(*cb)(const char*, void*), void(*down_cb)(GRfilePopup*, void*),
    void *arg, const char *root_or_fname)
{
    QTfilePopup *fsel = new QTfilePopup(this, mode, arg, root_or_fname);
    fsel->register_callback(cb);
    fsel->register_quit_callback(down_cb);
    fsel->set_visible(true);
    QTdev::self()->SetPopupLocation(loc, fsel, shell);
    return (fsel);
}


//
// Utilities
//

void
QTbag::ClearPopups()
{
    if (message)
        message->popdown();
    if (input)
        input->popdown();
    if (error)
        error->popdown();
    if (info)
        info->popdown();
    if (info2)
        info2->popdown();
    if (htinfo)
        htinfo->popdown();
    GRpopup *p;
    while ((p = monitor.first_object()) != 0)
        p->popdown();
}


GRaffirmPopup *
QTbag::PopUpAffirm(GRobject caller, GRloc loc, const char *question_str,
    void(*action_callback)(bool, void*), void *action_arg)
{
    QTaffirmPopup *affirm = new QTaffirmPopup(this, question_str, action_arg);
    affirm->register_caller(caller, false, true);
    affirm->register_callback(action_callback);
    affirm->set_visible(true);
    QTdev::self()->SetPopupLocation(loc, affirm, shell);
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
    QTnumPopup *numer = new QTnumPopup(this, prompt_str, initd, mind, maxd,
        del, numd, action_arg);
    numer->register_caller(caller, false, true);
    numer->register_callback(action_callback);
    numer->set_visible(true);
    QTdev::self()->SetPopupLocation(loc, numer, shell);
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
    QTledPopup *inp = new QTledPopup(this, prompt_string, init_str, btnstr,
        action_arg, multiline);
    inp->register_caller(caller, false, true);
    inp->register_callback((GRledPopup::GRledCallback)action_callback);
    inp->register_quit_callback(downproc);
    if (textwidth < 150)
        textwidth = 150;
    inp->setMinimumWidth(textwidth);
    inp->set_visible(true);
    QTdev::self()->SetPopupLocation(loc, inp, shell);
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
    if (input)
        delete input;
    input = new QTledPopup(this, label_str, initial_str, action_str, arg,
        false);
    input->register_callback((GRledPopup::GRledCallback)action_callback);
    input->set_ignore_return(true);
    if (textwidth < 150)
        textwidth = 150;
    input->setMinimumWidth(textwidth);
    if (sens_set)
        // Handle desentizing widgets while pop-up is active.
        (*sens_set)(this, false);
    input->set_visible(true);
}


// Pop up a message box If err is true, the popup will get the
// resources of an error popup.  If multi is true, the widget will not
// use the QTbag::message field, so that there can be arbitrarily
// many of these, but the user must keep track of them.  If desens is
// true, then any popup pointed to by QTbag::input is desensitized
// while the message is active.  This is done only if multi is false.
// If code is LW_XY, the location will be at x, y otherwise the
// location is set by code using QTbag::shell.  The popup widget is
// returned as a GRobject (void*).
//
GRmsgPopup *
QTbag::PopUpMessage(const char *string, bool err, bool desens,
    bool multi, GRloc loc)
{
    (void)desens;
    // XXX NOT HANDLED: desens

    if (!multi && message)
        message->popdown();

    QTmsgPopup *mesg = new QTmsgPopup(this, string, STY_NORM, 300, 76);
    if (!multi)
        message = mesg;
    mesg->setTitle(err ? "Error" : "Message");
    mesg->set_visible(true);
    QTdev::self()->SetPopupLocation(loc, message, shell);
    return (multi ? mesg : 0);
}


//XXX
int
QTbag::PopUpWarn(ShowMode mode, const char *message_str, STYtype style,
    GRloc loc)
{
    if (mode == MODE_OFF) {
        delete error;
        return (0);
    }
    if (mode == MODE_UPD) {
        if (error)
            return (err_cnt);
        return (0);
    }
    if (error) {
        error->setText(message_str);
        error->set_visible(true);
    }
    else {
        error = new QTtextPopup(this, message_str, style, 400, 100);
        error->setTitle("Error");
        error->set_visible(true);
        QTdev::self()->SetPopupLocation(loc, error, shell);
    }
    return (err_cnt);
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
        delete error;
        return (0);
    }
    if (mode == MODE_UPD) {
        if (error)
            return (err_cnt);
        return (0);
    }
    if (error) {
        error->setText(message_str);
        error->set_visible(true);
    }
    else {
        error = new QTtextPopup(this, message_str, style, 400, 100);
        error->setTitle("Error");
        error->set_visible(true);
        QTdev::self()->SetPopupLocation(loc, error, shell);
    }
    return (err_cnt);
}


GRtextPopup *
QTbag::PopUpErrText(const char *message_str, STYtype style, GRloc loc)
{
    QTtextPopup *mesg = new QTtextPopup(this, message_str, style, 400, 100);
    mesg->setTitle("Error");
    mesg->set_visible(true);
    QTdev::self()->SetPopupLocation(loc, mesg, shell);
    return (mesg);
}


int
QTbag::PopUpInfo(ShowMode mode, const char *msg, STYtype style, GRloc loc)
{
    if (mode == MODE_OFF) {
        delete info;
        return (0);
    }
    if (mode == MODE_UPD) {
        if (info)
            return (info_cnt);
        return (0);
    }
    if (info) {
        info->setText(msg);
        info->set_visible(true);
    }
    else {
        info = new QTtextPopup(this, msg, style, 400, 200);
        info->setTitle("Info");
        info->set_visible(true);
        QTdev::self()->SetPopupLocation(loc, info, shell);
    }
    return (info_cnt);
}


int
QTbag::PopUpInfo2(ShowMode, const char*, bool(*)(bool, void*), void*,
    STYtype, GRloc)
{
    return (0);
}


int
QTbag::PopUpHTMLinfo(ShowMode mode, const char *msg, GRloc loc)
{
    if (mode == MODE_OFF) {
        delete htinfo;
        return (0);
    }
    if (mode == MODE_UPD) {
        if (htinfo)
            return (htinfo_cnt);
        return (0);
    }
    if (htinfo) {
        htinfo->setText(msg);
        htinfo->set_visible(true);
    }
    else {
        htinfo = new QTtextPopup(this, msg, STY_HTML, 400, 200);
        htinfo->setTitle("Info");
        htinfo->set_visible(true);
        QTdev::self()->SetPopupLocation(loc, info, shell);
    }
    return (htinfo_cnt);
}

GRledPopup *QTbag::ActiveInput()       { return (input); }
GRmsgPopup *QTbag::ActiveMessage()     { return (message); }
GRtextPopup *QTbag::ActiveInfo()       { return (info); }
GRtextPopup *QTbag::ActiveInfo2()      { return (info2); }
GRtextPopup *QTbag::ActiveHtinfo()     { return (htinfo); }
GRtextPopup *QTbag::ActiveError()      { return (error); }
GRfontPopup *QTbag::ActiveFontsel()    { return (fontsel); }

void
QTbag::SetErrorLogName(const char *fname)
{
//XXX    GTKtextPopup::set_error_log(fname);
}

