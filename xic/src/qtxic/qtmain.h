
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

//
// Main Xic application-specific header
//

#ifndef QTMAIN_H
#define QTMAIN_H

#include "main.h"
#include "qtinterf/qtinterf.h"
#include "qtinterf/qtfont.h"
#include "qtinterf/draw_qt_w.h"
#include "dsp_tkif.h"

#include <QDialog>
#include <QFontMetrics>

class coord_w;
class param_w;
class layertab_w;
class keys_w;
class expand_d;
struct idle_proc;
struct sKeyEvent;

class QMenu;
class QMenuBar;


// Length of keypress buffer
#define CBUFMAX 16

inline class QTpkg *qtPkgIf();

class QTpkg : public cGrPkg
{
public:
    QTpkg()
        {
            busy_popup = 0;
            in_main_loop = false;
            not_mapped = false;
            idle_control = 0;
        }

    friend inline QTpkg *qtPkgIf()
        { return (dynamic_cast<QTpkg*>(GRpkgIf())); }

    // cGrPkg virtual overrides
    GRwbag *NewGX();
    int Initialize(GRwbag*);
    void ReinitNoGraphics();
    void Halt();
    void AppLoop();
    bool CheckForInterrupt();
    int Iconify(int);
    bool SubwinInit(int);
    void SubwinDestroy(int);

    bool SetWorking(bool);
    void SetOverrideBusy(bool);
    bool GetMainWinIdentifier(char*);

    bool IsDualPlane();
    bool IsTrueColor();
    void CloseGraphicsConnection();
    bool GetDisplayString(const char*, char*);
    bool CheckScreenAccess(hostent*, const char*, const char*);
    int RegisterIdleProc(int(*)(void*), void*);
    bool RemoveIdleProc(int);
    int RegisterTimeoutProc(int, int(*)(void*), void*);
    bool RemoveTimeoutProc(int);
    int StartTimer(int, bool*);
    void SetFont(const char*, int);
    const char *GetFont(int);
    PTretType PointTo();
    // end of overrides

/*
    void RegisterEventHandler(void(*)(GdkEvent*, void*), void*);
    bool NotMapped() { return (not_mapped); }
    GRobject BusyPopup() { return (busy_popup); }
    void SetBusyPopup(GRobject w) { busy_popup = w; }
*/

private:
    GRpopup *busy_popup;        // busy indicator
    bool in_main_loop;          // gtk_main called
    bool not_mapped;            // true when iconic
    idle_proc *idle_control;
};


// Length of keypress buffer
#define CBUFMAX 16

class keys_w : public draw_qt_w
{
public:
    keys_w(int, QWidget*);

    void show_keys();
    void set_keys(const char*);
    void bsp_keys();
    void check_exec(bool);

    int key_pos()       { return (keypos); }
    int key(int k)      { return (keys[k]); }
    char *key_buf()     { return (keys); }
    void append(char c) { keys[keypos++] = c; }

private:
    int keypos;
    char keys[CBUFMAX + 1];
    int win_number;
};

class subwin_d : public QDialog, virtual public DSPwbag, public qt_bag,
    public qt_draw
{
    Q_OBJECT

public:
    subwin_d(int, QWidget*);
    ~subwin_d();

    // cAppWinFuncs interface
    //
    // pixmap manipulations
    void SwitchToPixmap();
    void SwitchFromPixmap(const BBox*);
    GRobject DrawableReset();
    void CopyPixmap(const BBox*);
    void DestroyPixmap();
    bool DumpWindow(const char*, const BBox*);
    bool PixmapOk();

    // key handling/display interface
    void GetTextBuf(char*);
    void SetTextBuf(const char*);
    void ShowKeys();
    void SetKeys(const char*);
    void BspKeys();
    void CheckExec(bool);
    char *KeyBuf();
    int KeyPos();

    // misc pop-ups
    void PopUpGrid(GRobject, ShowMode);
    void PopUpExpand(GRobject, ShowMode, int, int,
        bool(*)(const char*, void*), void*, const char*, bool);
    void PopUpZoom(GRobject, ShowMode, int, int);
    //
    // End of cAppWinFuncs interface

    QMenuBar *MenuBar() { return (menubar); }
    QWidget *Viewport() { return (viewport->widget()); }

    bool keypress_handler(unsigned int, unsigned int, const char*, bool);

    keys_w *keys() { return (keys_pressed); }

    static DrawType draw_type() { return (drawtype); }

    QSize sizeHint() const { return (QSize(500, 400)); }
    QSize minimumSizeHint() const { return (QSize(250, 200)); }

signals:
    void update_coords(int, int);

protected slots:
    void button_down_slot(QMouseEvent*);
    void button_up_slot(QMouseEvent*);
    void key_down_slot(QKeyEvent*);
    void key_up_slot(QKeyEvent*);
    void motion_slot(QMouseEvent*);
    void enter_slot(QEvent*);
    void leave_slot(QEvent*);
    void resize_slot(QResizeEvent*);

public:
    expand_d *expand;           // expand pop-up

protected:
    QMenuBar *menubar;
    keys_w *keys_pressed;
    int win_number;

private:
    static DrawType drawtype;
};


class mainwin : public subwin_d
{
    Q_OBJECT

public:
    // qtmain.cc
    mainwin();
    void initialize();
    void set_coord_mode(int, int, bool, bool);
    void show_parameters();
    void set_focus(QWidget*);
    void set_indicating(bool);
    void send_key_event(sKeyEvent*);

    keys_w *keys() { return (keys_pressed); }
    draw_if *prompt_line() { return (promptline); }
    layertab_w *layer_table() { return (layertab); }

    QSize sizeHint() const { return (QSize(800, 650)); }
    QSize minimumSizeHint() const { return (QSize(800, 650)); }

    QWidget *PhysButtonBox() { return (phys_button_box); }
    QWidget *ElecButtonBox() { return (elec_button_box); }

signals:
    void side_button_press(MenuEnt*);

private slots:
    void wr_btn_slot();
    void update_coords_slot(int, int);

private:
    QWidget *phys_button_box;
    QWidget *elec_button_box;

    draw_if *promptline;
    coord_w *coords;
    layertab_w *layertab;
    param_w *status;
};


//-----------------------------------------------------------------------------
// Utilities

inline int
char_width()
{
    QFont *f;
    if (FC.getFont(&f, FNT_FIXED)) {
        QFontMetrics fm(*f);
        return (fm.width(QString("X")));
    }
    return (8);
}


inline int
any_string_width(QWidget*, const char *str)
{
    QFont *f;
    if (FC.getFont(&f, FNT_FIXED)) {
        QFontMetrics fm(*f);
        return (fm.width(QString(str)));
    }
    return (8*strlen(str));
}
  

inline int
line_height()
{
    QFont *f;
    if (FC.getFont(&f, FNT_FIXED)) {
        QFontMetrics fm(*f);
        return (fm.height());
    }
    return (16);
}


inline unsigned int
mod_state(int qtstate)
{
    int state = 0;
    if (qtstate & Qt::ShiftModifier)
        state |= GR_SHIFT_MASK;
    if (qtstate & Qt::ControlModifier)
        state |= GR_CONTROL_MASK;
    if (qtstate & Qt::AltModifier)
        state |= GR_ALT_MASK;
    return (state);
}


inline bool
is_modifier_key(int key)
{
    return (
        key == Qt::Key_Shift    ||
        key == Qt::Key_Control  ||
        key == Qt::Key_Meta     ||
        key == Qt::Key_Alt      ||
        key == Qt::Key_CapsLock);
}

#endif

