#include "qtkwent.h"

//==========================================================================
//
// The keyword entry composite functions

namespace {
    // Handler for the "set" button:  set the value, freeze the entry
    // area.
    //
    void action_proc(GtkWidget*, void *client_data)
    {
        xKWent *entry = static_cast<xKWent*>(client_data);
        entry->ent->handler(entry);
    }


    // Handler for the "def" button: reset to the default value.
    //
    void def_proc(GtkWidget*, void *client_data)
    {
        QTent *ent = (QTent*)client_data;
        if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(ent->active)))
            gtk_toggle_button_toggled(GTK_TOGGLE_BUTTON(ent->active));
        if (ent->defstr)
            gtk_entry_set_text(GTK_ENTRY(ent->entry), ent->defstr);
        else if (GTK_IS_SPIN_BUTTON(ent->entry))
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(ent->entry), ent->val);
        if (ent->entry2)
            gtk_entry_set_text(GTK_ENTRY(ent->entry2),
                ent->defstr ? ent->defstr : "");
    }


    // Set the "Def" button sensitive when the text is different from
    // the default text.
    //
    void value_changed(GtkWidget*, void *client_data)
    {
        xKWent *kwent = (xKWent*)client_data;
        QTent *ent = kwent->ent;
        if (ent->defstr && ent->active &&
                !GTKdev::GetStatus(ent->active)) {
            const char *str = gtk_entry_get_text(GTK_ENTRY(ent->entry));
            const char *str2 = 0;
            if (ent->entry2)
                str2 = gtk_entry_get_text(GTK_ENTRY(ent->entry2));
            bool isdef = true;
            if (strcmp(str, ent->defstr))
                isdef = false;
            else if (str2 && strcmp(str2, ent->defstr))
                isdef = false;
            if (isdef)
                gtk_widget_set_sensitive(ent->deflt, false);
            else
                gtk_widget_set_sensitive(ent->deflt, true);
        }
    }
}


// Function to create the keyword entry composite
// char *defstring           Default test (Def button sets this)
// int(*cb)()                Callback function for item list.
//
void
QTent::create_widgets(xKWent *kwstruct, const char *defstring,
    int(*cb)(GtkWidget*, GdkEvent*, void*))
{
    variable *v = Sp.GetRawVar(kwstruct->word);
    if (!defstring)
        defstring = "";
    defstr = lstring::copy(defstring);

    QHBoxLayout *hbox = new QHBoxLayout(this);
    hbox->setMargin(2);
    hbox->setSpacing(2);

    active = new QPushButton(tr("Set"));
    active->setCheckable(true);
    active->setChecked(v);
    hbox->addWidget(active);

    if (kwstruct->type != VTYP_BOOL &&
            !(kwstruct->type == VTYP_LIST && mode == KW_NO_CB)) {
        // second term is for "debug" button in debug panel
        deflt = new QPushButton(tr("Def"));
        hbox->addWidget(deflt);
        connect(drflt, SIGNAL(clicked()), this, SLOT(def_btn_slot()));
//        g_signal_connect(G_OBJECT(deflt), "clicked",
//            G_CALLBACK(def_proc), this);
    }

    if ((mode != KW_FLOAT && mode != KW_NO_SPIN) &&
            (kwstruct->type == VTYP_NUM || kwstruct->type == VTYP_REAL ||
            (kwstruct->type == VTYP_STRING &&
            (mode == KW_INT_2 || mode == KW_REAL_2)))) {
        if (mode == KW_REAL_2) {
            // no spin - may want to add options with and without spin
            entry = new QLineEdit();
            hbox->addWidget(entry);
            entry2 = new QLineEdit();
            hbox->addWidget(entry2);
        }
        else {
            /*
            GtkAdjustment *adj = (GtkAdjustment*)gtk_adjustment_new(val,
                kwstruct->min, kwstruct->max, del, pgsize, 0);
            entry = gtk_spin_button_new(adj, rate, numd);
            gtk_widget_show(entry);
            gtk_widget_set_size_request(entry, 80, -1);
            gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(entry), true);
            gtk_box_pack_start(GTK_BOX(hbox), entry, false, false, 2);
            if (mode == KW_INT_2) {
                adj = (GtkAdjustment*)gtk_adjustment_new(val, kwstruct->min,
                    kwstruct->max, del, pgsize, 0);
                entry2 = gtk_spin_button_new(adj, rate, numd);
                gtk_widget_show(entry2);
                gtk_widget_set_size_request(entry2, 80, -1);
                gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(entry2), true);
                gtk_box_pack_start(GTK_BOX(hbox), entry2, false, false, 2);
            }
            */
        }
    }
    else if (mode == KW_FLOAT ||
            mode == KW_NO_SPIN || kwstruct->type == VTYP_STRING ||
            kwstruct->type == VTYP_LIST) {
        if (kwstruct->type != VTYP_LIST || mode != KW_NO_CB) {
            entry = gtk_entry_new();
            gtk_widget_show(entry);
            gtk_widget_set_size_request(entry, 80, -1);
            gtk_box_pack_start(GTK_BOX(hbox), entry, true, true, 2);
        }
        if (cb && mode != KW_NO_SPIN && mode != KW_NO_CB) {
            gtk_editable_set_editable(GTK_EDITABLE(entry), false);
            if (mode == KW_NORMAL)
                mode = KW_STR_PICK;
            GtkWidget *vbox = gtk_vbox_new(false, 0);
            gtk_widget_show(vbox);

            GtkWidget *arrow = gtk_arrow_new(GTK_ARROW_UP, GTK_SHADOW_OUT);
            gtk_widget_show(arrow);
            GtkWidget *ebox = gtk_event_box_new();
            gtk_widget_show(ebox);
            gtk_container_add(GTK_CONTAINER(ebox), arrow);
            gtk_widget_add_events(ebox, GDK_BUTTON_PRESS_MASK);
            g_signal_connect_after(G_OBJECT(ebox), "button_press_event",
                G_CALLBACK(cb), kwstruct);
            if (mode == KW_FLOAT) {
                gtk_widget_add_events(ebox, GDK_BUTTON_RELEASE_MASK);
                g_signal_connect_after(G_OBJECT(ebox),
                    "button_release_event", G_CALLBACK(cb), kwstruct);
            }
            gtk_box_pack_start(GTK_BOX(vbox), ebox, false, false, 0);

            arrow = gtk_arrow_new(GTK_ARROW_DOWN, GTK_SHADOW_OUT);
            gtk_widget_show(arrow);
            ebox = gtk_event_box_new();
            gtk_widget_show(ebox);
            gtk_container_add(GTK_CONTAINER(ebox), arrow);
            gtk_widget_add_events(ebox, GDK_BUTTON_PRESS_MASK);
            g_signal_connect_after(G_OBJECT(ebox), "button_press_event",
                G_CALLBACK(cb), kwstruct);
            if (mode == KW_FLOAT) {
                gtk_widget_add_events(ebox, GDK_BUTTON_RELEASE_MASK);
                g_signal_connect_after(G_OBJECT(ebox),
                    "button_release_event", G_CALLBACK(cb), kwstruct);
            }
            g_object_set_data(G_OBJECT(ebox), "down", (void*)1);
            gtk_box_pack_start(GTK_BOX(vbox), ebox, false, false, 0);

            gtk_box_pack_start(GTK_BOX(hbox), vbox, false, false, 2);
        }
    }
    else if (mode == KW_NO_CB) {
        // KW_NO_CB is used exclusively by the buttons in the debug
        // group.

        VTvalue vv;
        if (Sp.GetVar("debug", VTYP_LIST, &vv)) {
            for (variable *vx = vv.get_list(); vx; vx = vx->next()) {
                if (vx->type() == VTYP_STRING) {
                    if (!strcmp(kwstruct->word, vx->string())) {
                        gtk_toggle_button_set_active(
                            GTK_TOGGLE_BUTTON(active), true);
                        break;
                    }
                }
            }
        }
        else if (Sp.GetVar("debug", VTYP_STRING, &vv)) {
            if (!strcmp(kwstruct->word, vv.get_string()))
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(active), true);
        }
        else if (Sp.GetVar("debug", VTYP_BOOL, &vv)) {
            gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(active), true);
        }
    }
    if (entry) {
        if (!v) {
            gtk_entry_set_text(GTK_ENTRY(entry),
                kwstruct->lastv1 ? kwstruct->lastv1 : defstr);
            if (entry2)
                gtk_entry_set_text(GTK_ENTRY(entry2),
                    kwstruct->lastv2 ? kwstruct->lastv2 : defstr);
        }
        else if (update)
            (*update)(true, v, this);
        g_signal_connect(G_OBJECT(entry), "changed",
            G_CALLBACK(value_changed), kwstruct);
        if (entry2)
            g_signal_connect(G_OBJECT(entry2), "changed",
                G_CALLBACK(value_changed), kwstruct);
    }
    if (mode != KW_NO_CB)
        set_state(v ? true : false);

    frame = gtk_frame_new(kwstruct->word);
    gtk_widget_show(frame);
    gtk_container_add(GTK_CONTAINER(frame), hbox);

    if (mode != KW_NO_CB)
        g_signal_connect(G_OBJECT(active), "clicked",
            G_CALLBACK(action_proc), kwstruct);
}


// Set sensitivity status in accord with Set button status
//
void
QTent::set_state(bool state)
{
    if (active)
        QTdev::SetStatus(active, state);
    if (deflt) {
        if (state)
            deflt->setEnabled(false);
        else {
            bool isdef = false;
            if (entry) {
                QByteArray ba = entry->text().toLatin1();
                const char *str = ba.constData();
                if (!strcmp(defstr, str))
                    isdef = true;
            }
            deflt->setEnabled(!isdef);
        }
    }
    if (entry) {
        if (mode != KW_STR_PICK)
            entry->setReadOnly(state);
        else
            entry->setReadOnly(true);
        entry->setEnabled(!state);
    }
}


namespace {
    void error_pr(const char *which, const char *minmax, const char *what)
    {
        GRpkg::self()->ErrPrintf(ET_ERROR, "bad %s%s value, must be %s.\n",
            which, minmax ? minmax : "", what);
    }
}


// Processing for the "set" button
//
void
QTent::handler(void *data)
{
    xKWent *kwstruct = (xKWent*)data;
    variable v;
    bool state = QTdev::GetStatus(active);
    // reset button temporarily, final status is set by callback()
    QTdev::SetStatus(active, !state);

    if (kwstruct->type == VTYP_BOOL) {
        v.set_boolean(state);
        kwstruct->callback(state, &v);
    }
    else if (kwstruct->type == VTYP_NUM) {
        QByteArray ba = entry->text().toLatin1();
        const char *string = ba.constData();
        int ival;
        if (sscanf(string, "%d", &ival) != 1) {
            error_pr(kwstruct->word, 0, "an integer");
            return;
        }
        v.set_integer(ival);
        kwstruct->callback(state, &v);
    }
    else if (kwstruct->type == VTYP_REAL) {
        QByteArray ba = entry->text().toLatin1();
        const char *string = ba.constData();
        double *d = SPnum.parse(&string, false);
        if (!d) {
            error_pr(kwstruct->word, 0, "a real");
            return;
        }
        v.set_real(*d);
        kwstruct->callback(state, &v);
    }
    else if (kwstruct->type == VTYP_STRING) {
        QByteArray ba = entry->text().toLatin1();
        const char *string = ba.constData();
        if (entry2) {
            // hack for two numeric fields
            const char *s = string;
            double *d = SPnum.parse(&s, false);
            if (!d) {
                error_pr(kwstruct->word, " min", "a real");
                return;
            }
            QByteArray ba2 = entry2->text().toLatin1();
            const char *string2 = ba2.constData();
            s = string2;
            d = SPnum.parse(&s, false);
            if (!d) {
                error_pr(kwstruct->word, " max", "a real");
                return;
            }
            char buf[256];
            snprintf(buf, sizeof(buf), "%s %s", string, string2);
            v.set_string(buf);
            kwstruct->callback(state, &v);
            return;
        }
        v.set_string(string);
        kwstruct->callback(state, &v);
    }
    else if (kwstruct->type == VTYP_LIST) {
        QByteArray ba = entry->text().toLatin1();
        const char *string = ba->constData();
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
                    kwstruct->callback(state, &v);
                    return;
                }
            }
            else {
                char *s = wordlist::flatten(wl);
                v.set_string(s);
                delete [] s;
                kwstruct->callback(state, &v);
                wordlist::destroy(wl);
                return;
            }
        }
        GRpkg::self()->ErrPrintf(ET_ERROR, "parse error in string for %s.\n",
            kwstruct->word);
    }
}


//
// Some global callback functions for use as an argument to
// QTent::create_widgets(), for generic data
//

// Boolean data
//
void
kw_bool_func(bool isset, variable*, QTent *ent)
{
    ent->set_state(isset);
}


// Integer data
//
void
kw_int_func(bool isset, variable *v, QTent *ent)
{
    ent->set_state(isset);
    if (ent->entry && isset) {
        if (GTK_IS_SPIN_BUTTON(ent->entry)) {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(ent->entry),
                v->integer());
        }
        else {
            char buf[64];
            snprintf(buf, sizeof(buf), "%d", v->integer());
            gtk_entry_set_text(GTK_ENTRY(ent->entry), buf);
        }
    }
}


// Real valued data
//
void
kw_real_func(bool isset, variable *v, QTent *ent)
{
    ent->set_state(isset);
    if (ent->entry && isset) {
        if (GTK_IS_SPIN_BUTTON(ent->entry)) {
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(ent->entry),
                v->real());
        }
        else {
            char buf[64];
            if (ent->mode == KW_NO_SPIN)
                snprintf(buf, sizeof(buf), "%g", v->real());
            else if (ent->mode == KW_FLOAT)
                snprintf(buf, sizeof(buf), "%.*e", ent->numd, v->real());
            else
                snprintf(buf, sizeof(buf), "%.*f", ent->numd, v->real());
            gtk_entry_set_text(GTK_ENTRY(ent->entry), buf);
        }
    }
}


// String data.
//
void
kw_string_func(bool isset, variable *v, QTent *ent)
{
    ent->set_state(isset);
    if (ent->entry && isset)
        gtk_entry_set_text(GTK_ENTRY(ent->entry), v->string());
}


//
// The following functions implement an exponential notation spin button.
//

namespace {
    // Increment or decrement the current value.
    //
    void bump(xKWent *entry)
    {
        if (!gtk_widget_is_sensitive(entry->ent->entry))
            return;
        char *string =
            gtk_editable_get_chars(GTK_EDITABLE(entry->ent->entry), 0, -1);
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

        if ((!entry->ent->down && !neg) || (entry->ent->down && neg))
            mant += entry->ent->del;
        else {
            if (mant - entry->ent->del < 1.0)
                mant = 1.0 - (1.0 - mant + entry->ent->del)/10;
            else
                mant -= entry->ent->del;
        }
        d = mant * pow(10.0, ex);
        if (neg)
            d = -d;
        if (d > entry->max)
            d = entry->max;
        else if (d < entry->min)
            d = entry->min;
        char buf[128];
        snprintf(buf, sizeof(buf), "%.*e", entry->ent->numd, d);
        gtk_entry_set_text(GTK_ENTRY(entry->ent->entry), buf);
    }


    // Repetitive timing loop.
    //
    int repeat_timer(void *client_data)
    {
        bump((xKWent*)client_data);
        return (true);
    }


    // Initial delay timer.
    //
    int delay_timer(void *client_data)
    {
        xKWent *entry = static_cast<xKWent*>(client_data);
        entry->ent->thandle = g_timeout_add(50, repeat_timer, client_data);
        return (false);
    }
}


// This handler is passed in the callback arg of QTent::create_widgets()
//
int
kw_float_hdlr(GtkWidget *caller, GdkEvent *event, void *client_data)
{
    xKWent *entry = static_cast<xKWent*>(client_data);
    if (event->type == GDK_BUTTON_PRESS) {
        entry->ent->down =
            g_object_get_data(G_OBJECT(caller), "down") ? true : false;
        bump(entry);
        entry->ent->thandle = g_timeout_add(200, delay_timer, entry);
    }
    else if (event->type == GDK_BUTTON_RELEASE)
        g_source_remove(entry->ent->thandle);
    return (true);
}

