
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
#include <math.h>

#include <QLayout>
#include <QCheckBox>
#include <QPushButton>
#include <QLineEdit>
#include <QDoubleSpinBox>


//----------------------------------------------------------------------------
// A Double Spin Box that uses exponential notation.

void
QTexpDoubleSpinBox::stepBy(int n)
{
    double d = value();
    bool neg = false;
    if (d < 0) {
        neg = true;
        d = -d;
    }
    double logd = log10(d);
    int ex = (int)floor(logd);
    logd -= ex;
    double mant = pow(10.0, logd);

    double del = 1.0;
    if ((n > 0 && !neg) || (n < 0 && neg))
        mant += del;
    else {
        if (mant - del < 1.0)
            mant = 1.0 - (1.0 - mant + del)/10;
        else
            mant -= del;
    }
    d = mant * pow(10.0, ex);
    if (neg)
        d = -d;
    if (d > maximum())
        d = maximum();
    else if (d < minimum())
        d = minimum();
    setValue(d);
}


double
QTexpDoubleSpinBox::valueFromText(const QString & text) const
{
    QByteArray text_ba = text.toLatin1();
    const char *str = text_ba.constData();
    double *d = SPnum.parse(&str, true);
    if (d)
        return (*d);
    return (0.0/0.0);  // NaN, "can't happen"
}

QString
QTexpDoubleSpinBox::textFromValue(double value) const
{
    const char *str = SPnum.printnum(value, (const char*)0, true,
        decimals());
    while (isspace(*str))
        str++;
    return (QString(str));
}

// Change the way we validate user input (if validate => valueFromText)
QValidator::State
QTexpDoubleSpinBox::validate(QString &text, int&) const
{
    QByteArray text_ba = text.toLatin1();
    const char *str = text_ba.constData();
    double *d = SPnum.parse(&str, true);
    return (d ? QValidator::Acceptable : QValidator::Invalid);
}


//----------------------------------------------------------------------------
// A string Spin Box that is used for keyword choice selection.

QString
QTchoiceSpinBox::textFromValue(int i) const
{
    int n = csb_entries.length();
    if (i >= 0 && i < n)
        return (csb_entries[i]);
    return (QString(""));
}


int
QTchoiceSpinBox::valueFromText(const QString &txt) const
{
    return (csb_entries.indexOf(txt));
}


QValidator::State
QTchoiceSpinBox::validate(QString &txt, int&) const
{
    if (csb_entries.indexOf(txt) >= 0)
        return (QValidator::Acceptable);
    return (QValidator::Invalid);
}


//----------------------------------------------------------------------------
// The keyword entry composite widget.

QTkwent::QTkwent(EntryMode m, EntryCallback cb, xKWent *kwstruct,
    const char *defstring, Kword **kw)
{
    ke_update = cb;
    ke_kwstruct = kwstruct;
    ke_val = 0.0;
    ke_del = 0.0;
    ke_pgsize = 0.0;
    ke_rate = 0.0;
    ke_numd = 0;
    ke_thandle = 0;
    ke_mode = m;
    ke_defstr = 0;
    ke_active = 0;
    ke_deflt = 0;
    ke_entry = 0;
    ke_entry2 = 0;
    ke_spbox = 0;
    ke_spbox2 = 0;
    ke_choice = 0;
    ke_expsb = 0;

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
            ke_spbox->setRange(ke_kwstruct->min, ke_kwstruct->max);
            hbox->addWidget(ke_spbox);
            if (ke_mode == KW_INT_2) {
                ke_spbox2 = new QDoubleSpinBox();
                ke_spbox2->setRange(ke_kwstruct->min, ke_kwstruct->max);
                hbox->addWidget(ke_spbox2);
            }
        }
    }
    else if (ke_mode == KW_FLOAT ||
            ke_mode == KW_NO_SPIN || kwstruct->type == VTYP_STRING ||
            kwstruct->type == VTYP_LIST) {
        if (kwstruct->type != VTYP_LIST || ke_mode != KW_NO_CB) {
            if (ke_mode == KW_FLOAT) {
                ke_expsb = new QTexpDoubleSpinBox();
                hbox->addWidget(ke_expsb);
                ke_expsb->setRange(ke_kwstruct->min, ke_kwstruct->max);
                ke_expsb->setSingleStep(0);
            }
            else if (ke_mode == KW_NO_SPIN) {
                ke_entry = new QLineEdit();
                hbox->addWidget(ke_entry);
            }
            else {
                ke_choice = new QTchoiceSpinBox();
                hbox->addWidget(ke_choice);
                if (kw) {
                    int i;
                    for (i = 0; kw[i]->word; i++)
                        ke_choice->entries()->append(kw[i]->word);
                    ke_choice->setMaximum(i-1);
                }
            }
        }
        if (ke_mode != KW_NO_SPIN && ke_mode != KW_NO_CB) {
            if (ke_mode == KW_NORMAL)
                ke_mode = KW_STR_PICK;
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
    if (ke_spbox) {
        if (v && ke_update)
            (*ke_update)(true, v, this);
        connect(ke_spbox, SIGNAL(textChanged(const QString&)),
            this, SLOT(val_changed_slot(const QString&)));
    }
    if (ke_expsb) {
        if (v && ke_update)
            (*ke_update)(true, v, this);
        connect(ke_expsb, SIGNAL(textChanged(const QString&)),
            this, SLOT(val_changed_slot(const QString&)));
    }
    if (ke_choice) {
        if (!v) {
            int val = ke_choice->valueFromText(ke_defstr);
            if (val >= 0)
                ke_choice->setValue(val);
            QString qstr = ke_choice->text();
            if (ke_defstr && ke_active && !QTdev::GetStatus(ke_active)) {
                ke_deflt->setEnabled(qstr != ke_defstr);
            }
        }
        else if (v && ke_update)
            (*ke_update)(true, v, this);
        connect(ke_choice, SIGNAL(textChanged(const QString&)),
            this, SLOT(val_changed_slot(const QString&)));
    }

    if (ke_mode != KW_NO_CB) {
        set_state(v ? true : false);
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


void
QTkwent::callback(bool isset, variable *v)
{
    if (ke_update)
        (*ke_update)(isset, v, this);
}


// Set up spin parameters for numerical entry.
//
void
QTkwent::setup(double v, double d, double p, double r, int n)
{
    ke_val = v;
    ke_del = d;
    ke_pgsize = p;
    ke_rate = r;
    ke_numd = n;
    variable *var = Sp.GetRawVar(ke_kwstruct->word);
    if (ke_spbox) {
        ke_spbox->setDecimals(ke_numd);
        ke_spbox->setSingleStep(pow(10.0, -ke_numd));
        if (!var)
            ke_spbox->setValue(ke_val);
        if (ke_spbox2) {
            ke_spbox2->setDecimals(ke_numd);
            ke_spbox2->setSingleStep(pow(10.0, -ke_numd));
        }
        QString qstr = ke_spbox->text();
        if (ke_defstr && ke_active && !QTdev::GetStatus(ke_active)) {
            ke_deflt->setEnabled(qstr != ke_defstr);
        }
    }
    else if (ke_expsb) {
        ke_expsb->setDecimals(ke_numd);
        if (!var)
            ke_expsb->setValue(ke_val);
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
            QByteArray ba;
            if (ke_entry)
                ba = ke_entry->text().toLatin1();
            else if (ke_spbox)
                ba = ke_spbox->text().toLatin1();
            else if (ke_expsb)
                ba = ke_expsb->text().toLatin1();
            else if (ke_choice)
                ba = ke_choice->text().toLatin1();
            const char *str = ba.constData();
            if (!strcmp(ke_defstr, str))
                isdef = true;
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
    else if (ke_spbox)
        ke_spbox->setEnabled(!state);
    else if (ke_expsb)
        ke_expsb->setEnabled(!state);
    else if (ke_choice)
        ke_choice->setEnabled(!state);
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


/*
    void
    int2_func(bool isset, variable *v, xEnt *ent)
    {
        ent->set_state(isset);
        if (ent->entry) {
            if (isset) {
                int val1=0, val2=0;
                if (v->type() == VTYP_STRING) {
                    const char *string = v->string();
                    double *d = SPnum.parse(&string, false);
                    val1 = (int)*d;
                    while (*string && !isdigit(*string) &&
                        *string != '-' && *string != '+') string++;
                    d = SPnum.parse(&string, true);
                    val2 = (int)*d;
                }
                else if (v->type() == VTYP_LIST) {
                    variable *vx = v->list();
                    val1 = vx->integer();
                    vx = vx->next();
                    val2 = vx->integer();
                }
                char buf[64];
                snprintf(buf, sizeof(buf), "%d", val1);
                gtk_entry_set_text(GTK_ENTRY(ent->entry), buf);

                if (ent->entry2) {
                    snprintf(buf, sizeof(buf), "%d", val2);
                    gtk_entry_set_text(GTK_ENTRY(ent->entry2), buf);
                    gtk_editable_set_editable(GTK_EDITABLE(ent->entry2), false);
                    gtk_widget_set_sensitive(ent->entry2, false);
                }
            }
            else {
                if (ent->entry2) {
                    gtk_editable_set_editable(GTK_EDITABLE(ent->entry2), true);
                    gtk_widget_set_sensitive(ent->entry2, true);
                }
            }
        }
    }
*/

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
            bool isenab = ent->ke_spbox->isEnabled();
            ent->ke_spbox->setEnabled(true);
            ent->ke_spbox->setValue(v->integer());
            ent->ke_spbox->setEnabled(isenab);
        }
        else if (ent->ke_entry) {
            char buf[64];
            snprintf(buf, sizeof(buf), "%d", v->integer());
            ent->ke_entry->setText(buf);
        }
    }
}


/*
    void
    real2_func(bool isset, variable *v, xEnt *ent)
    {
        ent->set_state(isset);
        if (ent->entry) {
            if (isset) {
                double dval1=0.0, dval2=0.0;
                if (v->type() == VTYP_STRING) {
                    const char *string = v->string();
                    double *d = SPnum.parse(&string, false);
                    dval1 = *d;
                    while (*string && !isdigit(*string) &&
                        *string != '-' && *string != '+') string++;
                    d = SPnum.parse(&string, true);
                    dval2 = *d;
                }
                else if (v->type() == VTYP_LIST) {
                    variable *vx = v->list();
                    dval1 = vx->real();
                    vx = vx->next();
                    dval2 = vx->real();
                }
                char buf[64];
                snprintf(buf, sizeof(buf), "%g", dval1);
                gtk_entry_set_text(GTK_ENTRY(ent->entry), buf);

                if (ent->entry2) {
                    snprintf(buf, sizeof(buf), "%g", dval2);
                    gtk_entry_set_text(GTK_ENTRY(ent->entry2), buf);
                    gtk_editable_set_editable(GTK_EDITABLE(ent->entry2), false);
                    gtk_widget_set_sensitive(ent->entry2, false);
                }
            }
            else {
                if (ent->entry2) {
                    gtk_editable_set_editable(GTK_EDITABLE(ent->entry2), true);
                    gtk_widget_set_sensitive(ent->entry2, true);
                }
            }
        }
    }
*/

// Static function.
// Real valued data.
//
void
QTkwent::ke_real_func(bool isset, variable *v, void *entp)
{
    QTkwent *ent = static_cast<QTkwent*>(entp);
    ent->set_state(isset);
    if (isset) {
        if (ent->ke_expsb) {
            ent->ke_expsb->setValue(v->real());
        }
        else if (ent->ke_spbox) {
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
    if (isset) {
        if (ent->ke_entry)
            ent->ke_entry->setText(v->string());
        else if (ent->ke_choice) {
            bool isenab = ent->ke_choice->isEnabled();
            ent->ke_choice->setEnabled(true);
            int val = ent->ke_choice->valueFromText(v->string());
            if (val >= 0)
                ent->ke_choice->setValue(val);
            ent->ke_choice->setEnabled(isenab);
        }
    }
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
        else if (ke_expsb)
            v.set_real(ke_expsb->value());
        ke_kwstruct->callback(state, &v);
    }
    else if (ke_kwstruct->type == VTYP_STRING) {
        if (ke_entry) {
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
        }
        else if (ke_choice) {
            QByteArray ba = ke_choice->text().toLatin1();
            const char *string = ba.constData();
            v.set_string(string);
        }
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
        else if (ke_expsb)
            ke_expsb->setValue(atof(ke_defstr));
    }
    else if (ke_spbox)
        ke_spbox->setValue(ke_val);
    else if (ke_expsb)
        ke_expsb->setValue(ke_val);
    if (ke_entry2)
        ke_entry2->setText(ke_defstr ? ke_defstr : "");
}


void
QTkwent::val_changed_slot(const QString &qstr)
{
    // Set the "Def" button sensitive when the text is different from
    // the default text.
    //
    if (ke_defstr && ke_active && !QTdev::GetStatus(ke_active)) {
        ke_deflt->setEnabled(qstr != ke_defstr);
    }
}

