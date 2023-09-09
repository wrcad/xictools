
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef QTKWENT_H
#define QTKWENT_H

#include "simulator.h"
#include "cshell.h"
#include "variable.h"
#include "keywords.h"

#include <QGroupBox>


class QCheckBox;
class QPushButton;
class QLineEdit;
class QDoubleSpinBox;
class QTkwent;

typedef sKWent<QTkwent> xKWent;
typedef void(*EntryCallback)(bool, variable*, void*);
typedef void(*UpDnCallback)(xKWent*, bool, bool);

enum EntryMode { KW_NORMAL, KW_INT_2, KW_REAL_2, KW_FLOAT, KW_NO_SPIN,
    KW_NO_CB, KW_STR_PICK };

// KW_NORMAL       Normal mode (regular spin button or entry)
// KW_INT_2        Use two integer fields (min/max)
// KW_REAL_2       Use two real fields (min/max)
// KW_FLOAT        Use exponential notation
// KW_NO_SPIN      Numeric entry but no adjustment
// KW_NO_CB        Special callbacks for debug popup
// KW_STR_PICK     String selection - callback arg passed to


// Class used in the keyword entry dialogs.
//
class QTkwent : public QGroupBox, public userEnt
{
    Q_OBJECT

public:
    QTkwent(EntryMode, EntryCallback, xKWent*, const char*, UpDnCallback = 0);
    virtual ~QTkwent();

    void callback(bool isset, variable *v)
    {
        if (ke_update)
            (*ke_update)(isset, v, this);
    }

    void setup(double v, double d, double p, double r, int n)
    {
        ke_val = v;
        ke_del = d;
        ke_pgsize = p;
        ke_rate = r;
        ke_numd = n;
    }

    void set_state(bool);

    // Global callback functions for generic data.
    static void ke_bool_func(bool, variable*, void*);
    static void ke_int_func(bool, variable*, void*);
    static void ke_real_func(bool, variable*, void*);
    static void ke_string_func(bool, variable*, void*);
    static void ke_float_hdlr(xKWent*, bool, bool);

private slots:
    void set_btn_slot(int);
    void def_btn_slot();
    void up_btn_pressed_slot();
    void up_btn_released_slot();
    void down_btn_pressed_slot();
    void down_btn_released_slot();
    void val_changed_slot(const QString&);

private:
    void bump();
    static int repeat_timer(void*);
    static int delay_timer(void*);

    EntryCallback ke_update;
    UpDnCallback ke_udcb;
    xKWent      *ke_kwstruct;

    double      ke_val;                 // spin box numeric values
    double      ke_del;
    double      ke_pgsize;
    double      ke_rate;                // spin button parameters
    int         ke_numd;                // fraction digits shown
    int         ke_thandle;             // timer handle
    bool        ke_down;                // decrement flag
    EntryMode   ke_mode;                // operating mode
    const char  *ke_defstr;             // default string

    QCheckBox   *ke_active;             // "set" check box
    QPushButton *ke_deflt;              // "def" button
    QLineEdit   *ke_entry;              // string entry area
    QLineEdit   *ke_entry2;             // string entry area
    QDoubleSpinBox *ke_spbox;           // spin box
    QDoubleSpinBox *ke_spbox2;          // second spin box
    QPushButton *ke_upbtn;              // choice up button
    QPushButton *ke_dnbtn;              // choice down button
};

#endif

