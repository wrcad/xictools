
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

#include "qtkwent.h"
#include "qttoolb.h"
#include "spnumber/spnumber.h"

#include <QLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QDoubleSpinBox>


//==========================================================================
// The keyword entry composite widget.

QTkwent::QTkwent(EntryMode m, EntryCallback cb, xKWent *kwstruct,
    const char *defstring, UpDnCallback udcb)
{
    ke_update = cb;
    ke_udcb = udcb;
    ke_kwstruct = kwstruct;
    ke_val = 0.0;
    ke_del = 0.0;
    ke_pgsize = 0.0;
    ke_rate = 0.0;
    ke_numd = 0;
    ke_thandle = 0;
    ke_down = false;
    ke_mode = m;
    ke_defstr = 0;
    ke_active = 0;
    ke_deflt = 0;
    ke_entry = 0;
    ke_entry2 = 0;
    ke_spbox = 0;
    ke_spbox2 = 0;
    ke_upbtn = 0;
    ke_dnbtn = 0;

    variable *v = Sp.GetRawVar(kwstruct->word);
    if (!defstring)
        defstring = "";
    ke_defstr = lstring::copy(defstring);

    setTitle(kwstruct->word);

    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setMargin(2);
    hbox->setSpacing(2);

    ke_active = new QCheckBox(tr("Set "));
    ke_active->setChecked(v);
    hbox->addWidget(ke_active);

    if (kwstruct->type != VTYP_BOOL &&
            !(kwstruct->type == VTYP_LIST && ke_mode == KW_NO_CB)) {
        // second term is for "debug" button in debug panel
        ke_deflt = new QPushButton(tr("Def"));
        hbox->addWidget(ke_deflt);
        connect(ke_deflt, SIGNAL(clicked()), this, SLOT(def_btn_slot()));
    }

    if ((ke_mode != KW_FLOAT && ke_mode != KW_NO_SPIN) &&
            (kwstruct->type == VTYP_NUM || kwstruct->type == VTYP_REAL ||
            (kwstruct->type == VTYP_STRING &&
            (ke_mode == KW_INT_2 || ke_mode == KW_REAL_2)))) {
        if (ke_mode == KW_REAL_2) {
            // no spin - may want to add options with and without spin
            ke_entry = new QLineEdit();
            hbox->addWidget(ke_entry);
            ke_entry2 = new QLineEdit();
            hbox->addWidget(ke_entry2);
        }
        else {
            ke_spbox = new QDoubleSpinBox();
            hbox->addWidget(ke_spbox);
            if (ke_mode == KW_INT_2) {
                ke_spbox2 = new QDoubleSpinBox();
                hbox->addWidget(ke_spbox2);
            }
        }
    }
    else if (ke_mode == KW_FLOAT ||
            ke_mode == KW_NO_SPIN || kwstruct->type == VTYP_STRING ||
            kwstruct->type == VTYP_LIST) {
        if (kwstruct->type != VTYP_LIST || ke_mode != KW_NO_CB) {
            ke_entry = new QLineEdit();
            hbox->addWidget(ke_entry);
        }
        if (udcb && ke_mode != KW_NO_SPIN && ke_mode != KW_NO_CB) {
            ke_entry->setReadOnly(true);
            if (ke_mode == KW_NORMAL)
                ke_mode = KW_STR_PICK;
            // up/down buttons
            QVBoxLayout *vb = new QVBoxLayout();
            vb->setMargin(0);
            vb->setSpacing(2);
            hbox->addLayout(vb);

            ke_upbtn = new QPushButton();
            vb->addWidget(ke_upbtn);
            connect(ke_upbtn, SIGNAL(pressed()),
                this, SLOT(up_btn_pressed_slot()));
            connect(ke_upbtn, SIGNAL(released()),
                this, SLOT(up_btn_released_slot()));
            ke_dnbtn = new QPushButton();
            vb->addWidget(ke_dnbtn);
            connect(ke_dnbtn, SIGNAL(pressed()),
                this, SLOT(down_btn_pressed_slot()));
            connect(ke_dnbtn, SIGNAL(released()),
                this, SLOT(down_btn_released_slot()));
       //XXX need arrow icons

            /*
            GtkWidget *arrow = gtk_arrow_new(GTK_ARROW_UP, GTK_SHADOW_OUT);
            gtk_widget_show(arrow);
            GtkWidget *ebox = gtk_event_box_new();
            gtk_widget_show(ebox);
            gtk_container_add(GTK_CONTAINER(ebox), arrow);
            gtk_widget_add_events(ebox, GDK_BUTTON_PRESS_MASK);
            g_signal_connect_after(G_OBJECT(ebox), "button_press_event",
                G_CALLBACK(udcb), kwstruct);
            if (mode == KW_FLOAT) {
                gtk_widget_add_events(ebox, GDK_BUTTON_RELEASE_MASK);
                g_signal_connect_after(G_OBJECT(ebox),
                    "button_release_event", G_CALLBACK(udcb), kwstruct);
            }
            gtk_box_pack_start(GTK_BOX(vbox), ebox, false, false, 0);

            arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_OUT);
            gtk_widget_show(arrow);
            ebox = gtk_event_box_new();
            gtk_widget_show(ebox);
            gtk_container_add(GTK_CONTAINER(ebox), arrow);
            gtk_widget_add_events(ebox, GDK_BUTTON_PRESS_MASK);
            g_signal_connect_after(G_OBJECT(ebox), "button_press_event",
                G_CALLBACK(udcb), kwstruct);
            if (mode == KW_FLOAT) {
                gtk_widget_add_events(ebox, GDK_BUTTON_RELEASE_MASK);
                g_signal_connect_after(G_OBJECT(ebox),
                    "button_release_event", G_CALLBACK(udcb), kwstruct);
            }
            g_object_set_data(G_OBJECT(ebox), "down", (void*)1);
            gtk_box_pack_start(GTK_BOX(vbox), ebox, false, false, 0);

            gtk_box_pack_start(GTK_BOX(hbox), vbox, false, false, 2);
            */
        }
    }
    else if (ke_mode == KW_NO_CB) {
        // KW_NO_CB is used exclusively by the buttons in the debug
        // group.

        VTvalue vv;
        if (Sp.GetVar("debug", VTYP_LIST, &vv)) {
            for (variable *vx = vv.get_list(); vx; vx = vx->next()) {
                if (vx->type() == VTYP_STRING) {
                    if (!strcmp(kwstruct->word, vx->string())) {
                        ke_active->setChecked(true);
                        break;
                    }
                }
            }
        }
        else if (Sp.GetVar("debug", VTYP_STRING, &vv)) {
            if (!strcmp(kwstruct->word, vv.get_string()))
                ke_active->setChecked(true);
        }
        else if (Sp.GetVar("debug", VTYP_BOOL, &vv)) {
            ke_active->setChecked(true);
        }
    }
    if (ke_entry) {
        if (!v) {
            ke_entry->setText(
                kwstruct->lastv1 ? kwstruct->lastv1 : ke_defstr);
            if (ke_entry2)
                ke_entry2->setText(
                    kwstruct->lastv2 ? kwstruct->lastv2 : ke_defstr);
        }
        else if (ke_update)
            (*ke_update)(true, v, this);
        connect(ke_entry, SIGNAL(textChanged(const QString&)),
            this, SLOT(val_changed_slot(const QString&)));
        if (ke_entry2) {
            connect(ke_entry2, SIGNAL(textChanged(const QString&)),
                this, SLOT(val_changed_slot(const QString&)));
        }
    }
    if (ke_mode != KW_NO_CB)
        set_state(v ? true : false);

    if (ke_mode != KW_NO_CB) {
        connect(ke_active, SIGNAL(stateChanged(int)),
            this, SLOT(set_btn_slot(int)));
    }
}


QTkwent::~QTkwent()
{
    delete [] ke_defstr;
    if (ke_kwstruct) {
        if (ke_entry) {
            QByteArray ba = ke_entry->text().toLatin1();
            delete [] ke_kwstruct->lastv1;
            ke_kwstruct->lastv1 = lstring::copy(ba.constData());
        }
        if (ke_entry2) {
            QByteArray ba = ke_entry2->text().toLatin1();
            delete [] ke_kwstruct->lastv2;
            ke_kwstruct->lastv2 = lstring::copy(ba.constData());
        }
        ke_kwstruct->ent = 0;
    }
}


// Set sensitivity status in accord with Set button status.
//
void
QTkwent::set_state(bool state)
{
    if (ke_active)
        QTdev::SetStatus(ke_active, state);
    if (ke_deflt) {
        if (state)
            ke_deflt->setEnabled(false);
        else {
            bool isdef = false;
            if (ke_entry) {
                QByteArray ba = ke_entry->text().toLatin1();
                const char *str = ba.constData();
                if (!strcmp(ke_defstr, str))
                    isdef = true;
            }
            ke_deflt->setEnabled(!isdef);
        }
    }
    if (ke_entry) {
        if (ke_mode != KW_STR_PICK)
            ke_entry->setReadOnly(state);
        else
            ke_entry->setReadOnly(true);
        ke_entry->setEnabled(!state);
    }
}


// Increment or decrement the current value of exponential spin box.
//
void
QTkwent::bump()
{
    if (!ke_entry || !ke_entry->isEnabled())
        return;
    QByteArray ba = ke_entry->text().toLatin1();
    const char *string = ba.constData();
    double d;
    sscanf(string, "%lf", &d);
    bool neg = false;
    if (d < 0) {
        neg = true;
        d = -d;
    }
    double logd = log10(d);
    int ex = (int)floor(logd);
    logd -= ex;
    double mant = pow(10.0, logd);

    if ((!ke_down && !neg) || (ke_down && neg))
        mant += ke_del;
    else {
        if (mant - ke_del < 1.0)
            mant = 1.0 - (1.0 - mant + ke_del)/10;
        else
            mant -= ke_del;
    }
    d = mant * pow(10.0, ex);
    if (neg)
        d = -d;
    if (d > ke_kwstruct->max)
        d = ke_kwstruct->max;
    else if (d < ke_kwstruct->min)
        d = ke_kwstruct->min;
    char buf[128];
    snprintf(buf, sizeof(buf), "%.*e", ke_numd, d);
    ke_entry->setText(buf);
}


// Static function;
// Repetitive timing loop for exponential spin box.
//
int
QTkwent::repeat_timer(void *client_data)
{
    QTkwent *ent = static_cast<QTkwent*>(client_data);
    ent->bump();
    return (true);
}


// Static function;
// Initial delay timer.
//
int
QTkwent::delay_timer(void *client_data)
{
    QTkwent *ent = static_cast<QTkwent*>(client_data);
    ent->ke_thandle = QTdev::self()->AddTimer(50, repeat_timer, client_data);
    return (false);
}


// Static function.
// Boolean data.
//
void
QTkwent::ke_bool_func(bool isset, variable*, void *entp)
{
    QTkwent *ent = static_cast<QTkwent*>(entp);
    ent->set_state(isset);
}


// Static function.
// Integer data.
//
void
QTkwent::ke_int_func(bool isset, variable *v, void *entp)
{
    QTkwent *ent = static_cast<QTkwent*>(entp);
    ent->set_state(isset);
    if (isset) {
        if (ent->ke_spbox) {
            ent->ke_spbox->setValue(v->real());
        }
        else if (ent->ke_entry) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%d", v->integer());
            ent->ke_entry->setText(buf);
        }
    }
}


// Static function.
// Real valued data.
//
void
QTkwent::ke_real_func(bool isset, variable *v, void *entp)
{
    QTkwent *ent = static_cast<QTkwent*>(entp);
    ent->set_state(isset);
    if (isset) {
        if (ent->ke_spbox) {
            ent->ke_spbox->setValue(v->real());
        }
        else if (ent->ke_entry) {
            char buf[64];
            if (ent->ke_mode == KW_NO_SPIN)
                snprintf(buf, sizeof(buf), "%g", v->real());
            else if (ent->ke_mode == KW_FLOAT)
                snprintf(buf, sizeof(buf), "%.*e", ent->ke_numd, v->real());
            else
                snprintf(buf, sizeof(buf), "%.*f", ent->ke_numd, v->real());
            ent->ke_entry->setText(buf);
        }
    }
}


// Static function.
// String data.
//
void
QTkwent::ke_string_func(bool isset, variable *v, void *entp)
{
    QTkwent *ent = static_cast<QTkwent*>(entp);
    ent->set_state(isset);
    if (ent->ke_entry && isset)
        ent->ke_entry->setText(v->string());
}


// Static function.
//
void
QTkwent::ke_float_hdlr(xKWent *entry, bool topbtn, bool pressed)
{
    //XXX
    if (pressed) {
        entry->ent->ke_down = !topbtn;
        entry->ent->bump();
        entry->ent->ke_thandle = QTdev::self()->AddTimer(200, delay_timer,
            entry->ent);
    }
    else 
        QTdev::self()->RemoveTimer(entry->ent->ke_thandle);
}


namespace {
    void error_pr(const char *which, const char *minmax, const char *what)
    {
        GRpkg::self()->ErrPrintf(ET_ERROR, "bad %s%s value, must be %s.\n",
            which, minmax ? minmax : "", what);
    }
}


void
QTkwent::set_btn_slot(int state)
{
    if (!ke_kwstruct)
        return;
    variable v;
    // reset button temporarily, final status is set by callback()
    QTdev::SetStatus(ke_active, !state);

    if (ke_kwstruct->type == VTYP_BOOL) {
        v.set_boolean(state);
        ke_kwstruct->callback(state, &v);
    }
    else if (ke_kwstruct->type == VTYP_NUM) {
        if (ke_entry) {
            int ival;
            QByteArray ba = ke_entry->text().toLatin1();
            const char *string = ba.constData();
            if (sscanf(string, "%d", &ival) != 1) {
                error_pr(ke_kwstruct->word, 0, "an integer");
                return;
            }
            v.set_integer(ival);
        }
        else if (ke_spbox)
            v.set_integer(ke_spbox->value());
        ke_kwstruct->callback(state, &v);
    }
    else if (ke_kwstruct->type == VTYP_REAL) {
        if (ke_entry) {
            QByteArray ba = ke_entry->text().toLatin1();
            const char *string = ba.constData();
            double *d = SPnum.parse(&string, false);
            if (!d) {
                error_pr(ke_kwstruct->word, 0, "a real");
                return;
            }
            v.set_real(*d);
        }
        else if (ke_spbox)
            v.set_real(ke_spbox->value());
        ke_kwstruct->callback(state, &v);
    }
    else if (ke_kwstruct->type == VTYP_STRING) {
        QByteArray ba = ke_entry->text().toLatin1();
        const char *string = ba.constData();
        if (ke_entry2) {
            // hack for two numeric fields
            const char *s = string;
            double *d = SPnum.parse(&s, false);
            if (!d) {
                error_pr(ke_kwstruct->word, " min", "a real");
                return;
            }
            QByteArray ba2 = ke_entry2->text().toLatin1();
            const char *string2 = ba2.constData();
            s = string2;
            d = SPnum.parse(&s, false);
            if (!d) {
                error_pr(ke_kwstruct->word, " max", "a real");
                return;
            }
            char buf[256];
            snprintf(buf, sizeof(buf), "%s %s", string, string2);
            v.set_string(buf);
            ke_kwstruct->callback(state, &v);
            return;
        }
        v.set_string(string);
        ke_kwstruct->callback(state, &v);
    }
    else if (ke_kwstruct->type == VTYP_LIST) {
        QByteArray ba = ke_entry->text().toLatin1();
        const char *string = ba.constData();
        wordlist *wl = CP.LexString(string);
        if (wl) {
            if (!strcmp(wl->wl_word, "(")) {
                // a list
                wordlist *wl0 = wl;
                wl = wl->wl_next;
                variable *v0 = CP.GetList(&wl);
                wordlist::destroy(wl0);
                if (v0) {
                    v.set_list(v0);
                    ke_kwstruct->callback(state, &v);
                    return;
                }
            }
            else {
                char *s = wordlist::flatten(wl);
                v.set_string(s);
                delete [] s;
                ke_kwstruct->callback(state, &v);
                wordlist::destroy(wl);
                return;
            }
        }
        GRpkg::self()->ErrPrintf(ET_ERROR, "parse error in string for %s.\n",
            ke_kwstruct->word);
    }
}


void
QTkwent::def_btn_slot()
{
    // Handler for the "def" button: reset to the default value.
    //
    if (ke_active) {
        if (ke_active->isChecked())
            ke_active->setChecked(false);
    }
    if (ke_defstr) {
        if (ke_entry)
            ke_entry->setText(ke_defstr);
        else if (ke_spbox)
            ke_spbox->setValue(atof(ke_defstr));
    }
    else if (ke_spbox)
        ke_spbox->setValue(ke_val);
    if (ke_entry2)
        ke_entry2->setText(ke_defstr ? ke_defstr : "");
}


void
QTkwent::up_btn_pressed_slot()
{
    if (ke_udcb)
        (*ke_udcb)(ke_kwstruct, true, true);
}


void
QTkwent::up_btn_released_slot()
{
    if (ke_udcb)
        (*ke_udcb)(ke_kwstruct, true, false);
}


void
QTkwent::down_btn_pressed_slot()
{
    if (ke_udcb)
        (*ke_udcb)(ke_kwstruct, false, true);
}


void
QTkwent::down_btn_released_slot()
{
    if (ke_udcb)
        (*ke_udcb)(ke_kwstruct, false, false);
}


void
QTkwent::val_changed_slot(const QString&)
{
    // Set the "Def" button sensitive when the text is different from
    // the default text.
    //
    xKWent *kwent = ke_kwstruct;
    if (ke_defstr && ke_active && !QTdev::GetStatus(ke_active)) {
        QByteArray ba = ke_entry->text().toLatin1();
        const char *str = ba.constData();
        QByteArray ba2;
        const char *str2 = 0;
        if (ke_entry2) {
            ba2 = ke_entry2->text().toLatin1();
            str2 =  ba2.constData();
        }
        bool isdef = true;
        if (strcmp(str, ke_defstr))
            isdef = false;
        else if (str2 && strcmp(str2, ke_defstr))
            isdef = false;
        if (isdef)
            ke_deflt->setEnabled(false);
        else
            ke_deflt->setEnabled(true);
    }
}

