
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
#include <QSpinBox>
#include <QLineEdit>

class QCheckBox;
class QToolButton;
class QTkwent;
namespace qtinterf {
    class QTdoubleSpinBox;
    class QTexpDoubleSpinBox;
}
using namespace qtinterf;


//-----------------------------------------------------------------------------
// QTchoiceSpinBox:  a string Spin Box that is used for keyword choice
// selection.

class QTchoiceSpinBox : public QSpinBox
{
public:
    explicit QTchoiceSpinBox(QWidget *prnt = nullptr) : QSpinBox(prnt)
    {
        lineEdit()->setReadOnly(true);
    }
    ~QTchoiceSpinBox() { }

    // Overrides
    QString textFromValue(int) const;
    int valueFromText(const QString&) const;
    QValidator::State validate(QString&, int&) const;

    QStringList *entries()      { return (&csb_entries); }

private:
    QStringList csb_entries;  // List of words.
};


//-----------------------------------------------------------------------------
// The keyword entry area used in the panels and pages to set
// variables.  Lots of options.

// Wrapper to access KWent::ent cleanly.
//
struct xKWent : public KWent
{
    inline QTkwent *qtent();
};

typedef void(*EntryCallback)(bool, variable*, void*);

enum EntryMode { KW_NORMAL, KW_INT_2, KW_REAL_2, KW_FLOAT, KW_NO_SPIN,
    KW_NO_CB, KW_STR_PICK };

// KW_NORMAL       Normal mode (regular spin button or entry).
// KW_INT_2        Use two integer fields (min/max).
// KW_REAL_2       Use two real fields (min/max).
// KW_FLOAT        Use exponential notation, one field only.
// KW_NO_SPIN      Numeric entry but no adjustment.
// KW_NO_CB        Special callbacks for debug popup.
// KW_STR_PICK     String selection, one field only.


// Class used in the keyword entry dialogs.
//
class QTkwent : public QGroupBox, public userEnt
{
    Q_OBJECT

public:
    QTkwent(EntryMode, EntryCallback, xKWent*, const char*, Kword** = 0);
    virtual ~QTkwent();

    // userEnt virtual override
    void callback(bool, variable*);

    void setup(double, double, double, double, int);
    void set_state(bool);

    // Global callback functions for generic data.
    static void ke_bool_func(bool, variable*, void*);
    static void ke_int_func(bool, variable*, void*);
    static void ke_int2_func(bool, variable*, void*);
    static void ke_real_func(bool, variable*, void*);
    static void ke_real2_func(bool, variable*, void*);
    static void ke_string_func(bool, variable*, void*);
    static void ke_float_hdlr(xKWent*, bool, bool);

    QCheckBox *active()         { return (ke_active); }
    QLineEdit *entry()          { return (ke_entry); }
    QLineEdit *entry2()         { return (ke_entry2); }
    QTchoiceSpinBox *choice()   { return (ke_choice); }
    xKWent *kwstruct()          { return (ke_kwstruct); }

private slots:
    void set_btn_slot(int);
    void def_btn_slot();
    void val_changed_slot(const QString&);

private:
    EntryCallback ke_update;
    xKWent      *ke_kwstruct;

    double      ke_val;                 // spin box numeric values
    double      ke_del;
    double      ke_pgsize;
    double      ke_rate;                // spin button parameters
    int         ke_numd;                // fraction digits shown
    int         ke_thandle;             // timer handle
    EntryMode   ke_mode;                // operating mode
    const char  *ke_defstr;             // default string

    QCheckBox   *ke_active;             // "set" check box
    QToolButton *ke_deflt;              // "def" button
    QLineEdit   *ke_entry;              // entry area
    QLineEdit   *ke_entry2;             // second entry area
    QTdoubleSpinBox *ke_spbox;          // spin box
    QTdoubleSpinBox *ke_spbox2;         // second spin box
    QTchoiceSpinBox *ke_choice;         // for keyword choice
    QTexpDoubleSpinBox *ke_expsb;       // for exponential input spin box
};

inline QTkwent *
xKWent::qtent()     { return (dynamic_cast<QTkwent*>(ent)); }

#endif

