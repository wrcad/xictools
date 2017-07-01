
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: gtkdvedit.cc,v 5.49 2016/02/21 18:49:42 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "fio.h"
#include "sced.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "fio_library.h"
#include "gtkmain.h"
#include "gtkinlines.h"
#include "events.h"
#include "promptline.h"
#include "errorlog.h"
#include "menu.h"
#include "filestat.h"


//-----------------------------------------------------------------------------
// The Device Parameters pop-up.  This panel allows addition of
// properties to library devices, and initiates saving the devices in
// the device library file or a cell file.
//
// Help system keywords used:
//  devedit

namespace {
    namespace gtkdvedit {
        struct sBstate : public CmdState
        {
            friend struct sDE;

            sBstate(const char*, const char*);
            virtual ~sBstate();

            void setXY(int x, int y)
                {
                    xref = x;
                    yref = y;
                }

            void b1down();
            void b1up();
            void esc();

        private:
            int xref, yref;
        };

        sBstate *Bcmd;

        // Container for text entries.
        //
        struct entries_t
        {
            entries_t()
                {
                    cname = 0;
                    prefix = 0;
                    model = 0;
                    value = 0;
                    param = 0;
                    branch = 0;
                }

            ~entries_t()
                {
                    delete [] cname;
                    delete [] prefix;
                    delete [] model;
                    delete [] value;
                    delete [] param;
                    delete [] branch;
                }

            char *cname;
            char *prefix;
            char *model;
            char *value;
            char *param;
            char *branch;
        };

        struct sDE
        {
            sDE(GRobject);
            ~sDE();

            GtkWidget *shell() { return (de_shell); }

            void set_ref(int, int);

        private:
            void load(entries_t*);
            static void de_cancel_proc(GtkWidget*, void*);
            static void de_help_proc(GtkWidget*, void*);
            static void de_menu_proc(GtkWidget*, void*);
            static void de_devs_proc(GtkWidget*, void*);
            static void de_branch_proc(GtkWidget*, void*);

            GRobject de_caller;
            GtkWidget *de_shell;
            GtkWidget *de_cname;
            GtkWidget *de_prefix;
            GtkWidget *de_model;
            GtkWidget *de_value;
            GtkWidget *de_param;
            GtkWidget *de_toggle;
            GtkWidget *de_branch;
            GtkWidget *de_nophys;
            int de_menustate;
            int de_xref, de_yref;

            static const char *orient_labels[];
        };

        sDE *DE;

        const char *sDE::orient_labels[] =
            { "none", "Left", "Down", "Right", "Up", 0 };
    }
}

using namespace gtkdvedit;


// Pop up the Device Parameters panel.
//
void
cSced::PopUpDevEdit(GRobject caller, ShowMode mode)
{
    if (!GRX || !mainBag())
        return;
    if (mode == MODE_OFF) {
        delete DE;
        return;
    }
    if (DE)
        return;

    CDs *sd = CurCell(Electrical);
    if (!sd) {
        Log()->PopUpErr("No current cell!");
        if (caller)
            Menu()->SetStatus(caller, false);
        return;
    }
    CDm_gen mgen(sd, GEN_MASTERS);
    for (CDm *mdesc = mgen.m_first(); mdesc; mdesc = mgen.m_next()) {
        if (mdesc->hasInstances()) {
            Log()->PopUpErr(
                "Current cell contains instantiations, can't be a device.");
            if (caller)
                Menu()->SetStatus(caller, false);
            return;
        }
    }
    CDs *cursdp = CurCell(Physical);
    if (cursdp && !cursdp->isEmpty()) {
        Log()->PopUpErr(
            "Current cell contains physical data, can't be a device.");
        if (caller)
            Menu()->SetStatus(caller, false);
        return;
    }

    new sDE(caller);
    if (!DE->shell()) {
        delete DE;
        return;
    }
    gtk_window_set_transient_for(GTK_WINDOW(DE->shell()),
        GTK_WINDOW(mainBag()->Shell()));

    GRX->SetPopupLocation(GRloc(), DE->shell(), mainBag()->Viewport());
    gtk_widget_show(DE->shell());
}
// End of cSced functions.


sDE::sDE(GRobject caller)
{
    DE = this;
    de_caller = caller;
    de_shell = 0;
    de_cname = 0;
    de_prefix = 0;
    de_model = 0;
    de_value = 0;
    de_param = 0;
    de_toggle = 0;
    de_branch = 0;
    de_nophys = 0;
    de_menustate = 0;
    de_xref = de_yref = 0;

    CDs *cursde = CurCell(Electrical);

    de_shell = gtk_NewPopup(0, "Device Parameters", de_cancel_proc, 0);
    if (!de_shell)
        return;

    GtkWidget *form = gtk_table_new(1, 1, false);
    gtk_widget_show(form);
    gtk_container_add(GTK_CONTAINER(de_shell), form);

    int row = 0;
    GtkWidget *label = gtk_label_new("Device Name");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 2, row, row+1,
        (GtkAttachOptions)0, (GtkAttachOptions)0, 2, 2);
    de_cname = gtk_entry_new();
    gtk_widget_show(de_cname);
    gtk_table_attach(GTK_TABLE(form), de_cname, 2, 3, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    gtk_entry_set_text(GTK_ENTRY(de_cname), DSP()->CurCellName()->string());

    label = gtk_label_new("SPICE Prefix");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 2, row, row+1,
        (GtkAttachOptions)0, (GtkAttachOptions)0, 2, 2);
    de_prefix = gtk_entry_new();
    gtk_widget_show(de_prefix);
    gtk_table_attach(GTK_TABLE(form), de_prefix, 2, 3, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    CDp_name *pn = (CDp_name*)(cursde ? cursde->prpty(P_NAME) : 0);
    if (pn && pn->name_string())
        gtk_entry_set_text(GTK_ENTRY(de_prefix), pn->name_string()->string());

    GtkWidget *button = gtk_button_new_with_label("Help");
    gtk_widget_set_name(button, "help");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(de_help_proc), 0);
    gtk_table_attach(GTK_TABLE(form), button, 3, 4, row-2, row,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK), 2, 2);

    label = gtk_label_new("Default Model");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 2, row, row+1,
        (GtkAttachOptions)0, (GtkAttachOptions)0, 2, 2);
    de_model = gtk_entry_new();
    gtk_widget_show(de_model);
    gtk_table_attach(GTK_TABLE(form), de_model, 2, 4, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    CDp_user *pu = (CDp_user*)(cursde ? cursde->prpty(P_MODEL) : 0);
    if (pu) {
        char *s = hyList::string(pu->data(), HYcvPlain, false);
        gtk_entry_set_text(GTK_ENTRY(de_model), s);
        delete [] s;
    }

    label = gtk_label_new("Default Value");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 2, row, row+1,
        (GtkAttachOptions)0, (GtkAttachOptions)0, 2, 2);
    de_value = gtk_entry_new();
    gtk_widget_show(de_value);
    gtk_table_attach(GTK_TABLE(form), de_value, 2, 4, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    pu = (CDp_user*)(cursde ? cursde->prpty(P_VALUE) : 0);
    if (pu) {
        char *s = hyList::string(pu->data(), HYcvPlain, false);
        gtk_entry_set_text(GTK_ENTRY(de_value), s);
        delete [] s;
    }

    label = gtk_label_new("Default Parameters");
    gtk_widget_show(label);
    gtk_table_attach(GTK_TABLE(form), label, 0, 2, row, row+1,
        (GtkAttachOptions)0, (GtkAttachOptions)0, 2, 2);
    de_param = gtk_entry_new();
    gtk_widget_show(de_param);
    gtk_table_attach(GTK_TABLE(form), de_param, 2, 4, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    pu = (CDp_user*)(cursde ? cursde->prpty(P_PARAM) : 0);
    if (pu) {
        char *s = hyList::string(pu->data(), HYcvPlain, false);
        gtk_entry_set_text(GTK_ENTRY(de_param), s);
        delete [] s;
    }

    de_toggle = gtk_toggle_button_new_with_label("Hot Spot");
    gtk_widget_set_name(de_toggle, "hotspot");
    gtk_widget_show(de_toggle);
    gtk_signal_connect(GTK_OBJECT(de_toggle), "clicked",
        GTK_SIGNAL_FUNC(de_branch_proc), 0);
    gtk_table_attach(GTK_TABLE(form), de_toggle, 0, 1, row, row+1,
        (GtkAttachOptions)0, (GtkAttachOptions)0, 2, 2);
    GtkWidget *entry = gtk_option_menu_new();
    gtk_widget_set_name(entry, "orient");
    gtk_widget_show(entry);
    GtkWidget *menu = gtk_menu_new();
    gtk_widget_set_name(menu, "orient");
    for (int i = 0; orient_labels[i]; i++) {
        GtkWidget *mi = gtk_menu_item_new_with_label(orient_labels[i]);
        gtk_widget_set_name(mi, orient_labels[i]);
        gtk_widget_show(mi);
        gtk_menu_append(GTK_MENU(menu), mi);
        gtk_signal_connect(GTK_OBJECT(mi), "activate",
            GTK_SIGNAL_FUNC(de_menu_proc), (void*)(long)i);
    }
    gtk_option_menu_set_menu(GTK_OPTION_MENU(entry), menu);
    gtk_table_attach(GTK_TABLE(form), entry, 1, 2, row, row+1,
        (GtkAttachOptions)0, (GtkAttachOptions)0, 2, 2);
    de_branch = gtk_entry_new();
    gtk_widget_show(de_branch);
    gtk_table_attach(GTK_TABLE(form), de_branch, 2, 4, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    CDp_branch *pb = (CDp_branch*)(cursde ? cursde->prpty(P_BRANCH) : 0);
    if (pb) {
        int ix = 0;
        if (!pb->rot_x() && pb->rot_y()) {
            if (pb->rot_y() > 0)
                ix = 4;
            else
                ix = 2;
        }
        else if (pb->rot_x() && !pb->rot_y()) {
            if (pb->rot_x() < 0)
                ix = 1;
            else
                ix = 3;
        }
        de_menustate = ix;
        gtk_option_menu_set_history(GTK_OPTION_MENU(entry), ix);
        if (pb->br_string())
            gtk_entry_set_text(GTK_ENTRY(de_branch), pb->br_string());

        de_xref = pb->pos_x();
        de_yref = pb->pos_y();

        GRX->SetStatus(de_toggle, true);
        de_branch_proc(de_toggle, 0);
    }

    de_nophys = gtk_check_button_new_with_label(
        "No Physical Implementation");
    gtk_widget_set_name(de_nophys, "nophys");
    gtk_widget_show(de_nophys);
    gtk_table_attach(GTK_TABLE(form), de_nophys, 0, 4, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
    row++;

    CDp *pnp = cursde ? cursde->prpty(P_NOPHYS) : 0;
    if (pnp)
        GRX->SetStatus(de_nophys, true);

    button = gtk_button_new_with_label("Save in Library");
    gtk_widget_set_name(button, "save_lib");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(de_devs_proc), 0);
    gtk_table_attach(GTK_TABLE(form), button, 0, 2, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_button_new_with_label("Save as Cell File");
    gtk_widget_set_name(button, "save_nat");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(de_devs_proc), (void*)(long)1);
    gtk_table_attach(GTK_TABLE(form), button, 2, 3, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);

    button = gtk_button_new_with_label("Dismiss");
    gtk_widget_set_name(button, "dismiss");
    gtk_widget_show(button);
    gtk_signal_connect(GTK_OBJECT(button), "clicked",
        GTK_SIGNAL_FUNC(de_cancel_proc), 0);
    gtk_table_attach(GTK_TABLE(form), button, 3, 4, row, row+1,
        (GtkAttachOptions)(GTK_EXPAND | GTK_FILL | GTK_SHRINK),
        (GtkAttachOptions)0, 2, 2);
}


sDE::~sDE()
{
    DE = 0;
    if (de_caller)
        GRX->SetStatus(de_caller, false);
    if (Bcmd)
        Bcmd->esc();
    if (de_shell)
        gtk_widget_destroy(de_shell);
}


void
sDE::set_ref(int x, int y)
{
    de_xref = x;
    de_yref = y;
    GRX->SetStatus(de_branch, false);
}


namespace {
    // Copy, and remove leading/trailing white space.  Null is
    // returned for an empty string.
    //
    char *strip(const char *str)
    {
        if (!str)
            return (0);
        while (isspace(*str))
            str++;
        if (!*str)
            return (0);
        char *nstr = lstring::copy(str);
        char *e = (char*)nstr + strlen(nstr) - 1;
        while (e >= nstr && isspace(*e))
            *e-- = 0;
        return (nstr);
    }
}


// Load the entries from the pop-up.
//
void
sDE::load(entries_t *e)
{
    if (!e)
        return;
    e->cname = strip(gtk_entry_get_text(GTK_ENTRY(de_cname)));
    char *cn = lstring::strip_path(e->cname);
    if (cn != e->cname) {
        char *ctmp = e->cname;
        e->cname = lstring::copy(cn);
        delete [] ctmp;
    }
    e->prefix = strip(gtk_entry_get_text(GTK_ENTRY(de_prefix)));
    e->model = strip(gtk_entry_get_text(GTK_ENTRY(de_model)));
    e->value = strip(gtk_entry_get_text(GTK_ENTRY(de_value)));
    e->param = strip(gtk_entry_get_text(GTK_ENTRY(de_param)));
    e->branch = strip(gtk_entry_get_text(GTK_ENTRY(de_branch)));
}


// Static function.
void
sDE::de_cancel_proc(GtkWidget*, void*)
{
    SCD()->PopUpDevEdit(0, MODE_OFF);
}


// Static function.
void
sDE::de_help_proc(GtkWidget*, void*)
{
    DSPmainWbag(PopUpHelp("devedit"))
}


// Static function.
void
sDE::de_menu_proc(GtkWidget*, void *arg)
{
    if (DE)
        DE->de_menustate = (long)arg;
}


// Static function.
void
sDE::de_devs_proc(GtkWidget*, void *arg)
{
    if (!DE)
        return;
    CDs *cursde = CurCell(Electrical);
    if (!cursde) {
        Log()->PopUpErr("No curent cell!");
        return;
    }

    entries_t ent;
    DE->load(&ent);
    if (!ent.cname) {
        Log()->PopUpErr("No Cell Name given, this is required.");
        return;
    }

    int nodecnt = 0;
    CDp_node *pnd = (CDp_node*)cursde->prpty(P_NODE);
    while (pnd) {
        nodecnt++;
        pnd = pnd->next();
    }

    // If a device is keyed by 'X', it is really a subcircuit macro
    // call to the model library.  The subname field must be 0 for
    // devices in the device library file.  The name of the subcircuit
    // macro is stored in a model property, so that the text will be
    // added along with the model text.  The subckt macros are saved
    // in the model database.
    bool is_sc = ent.prefix && (*ent.prefix == 'X' || *ent.prefix == 'x');

    if (!is_sc && !nodecnt) {
        Log()->PopUpErr(
            "Device has no nodes, use subct button to define nodes.");
        return;
    }

    // For a subcircuit macro reference, the only valid properties are
    // node, name, param, and model (for the macro name).  Note that
    // the user can reset the macro name in instances.

    if (ent.model) {
        CDp_user *pu = (CDp_user*)cursde->prpty(P_MODEL);
        if (pu)
            hyList::destroy(pu->data());
        else {
            pu = new CDp_user(P_MODEL);
            pu->set_next_prp(cursde->prptyList());
            cursde->setPrptyList(pu);
        }
        pu->set_data(new hyList(cursde, ent.model, HYcvPlain));

        pu = (CDp_user*)cursde->prpty(P_VALUE);
        if (pu) {
            cursde->prptyUnlink(pu);
            delete pu;
        }
    }
    else if (ent.value) {
        if (is_sc) {
            Log()->PopUpErr("No Model given, this is mandatory for macro.");
            return;
        }
        CDp_user *pu = (CDp_user*)cursde->prpty(P_VALUE);
        if (pu)
            hyList::destroy(pu->data());
        else {
            pu = new CDp_user(P_VALUE);
            pu->set_next_prp(cursde->prptyList());
            cursde->setPrptyList(pu);
        }
        pu->set_data(new hyList(cursde, ent.value, HYcvPlain));

        pu = (CDp_user*)cursde->prpty(P_MODEL);
        if (pu) {
            cursde->prptyUnlink(pu);
            delete pu;
        }
    }
    else {
        if (is_sc) {
            Log()->PopUpErr("No Model given, this is mandatory for macro.");
            return;
        }
        CDp_user *pu = (CDp_user*)cursde->prpty(P_VALUE);
        if (pu) {
            cursde->prptyUnlink(pu);
            delete pu;
        }
        pu = (CDp_user*)cursde->prpty(P_MODEL);
        if (pu) {
            cursde->prptyUnlink(pu);
            delete pu;
        }
    }

    if (ent.param) {
        CDp_user *pu = (CDp_user*)cursde->prpty(P_PARAM);
        if (pu)
            hyList::destroy(pu->data());
        else {
            pu = new CDp_user(P_PARAM);
            pu->set_next_prp(cursde->prptyList());
            cursde->setPrptyList(pu);
        }
        pu->set_data(new hyList(cursde, ent.param, HYcvPlain));
    }
    else {
        CDp_user *pu = (CDp_user*)cursde->prpty(P_PARAM);
        if (pu) {
            cursde->prptyUnlink(pu);
            delete pu;
        }
    }

    if (GRX->GetStatus(DE->de_toggle) && !is_sc) {
        CDp_branch *pb = (CDp_branch*)cursde->prpty(P_BRANCH);
        if (!pb) {
            pb = new CDp_branch;
            pb->set_next_prp(cursde->prptyList());
            cursde->setPrptyList(pb);
        }
        pb->set_br_string(ent.branch);
        switch (DE->de_menustate) {
        default:
        case 0:
            pb->set_rot_x(0);
            pb->set_rot_y(0);
            break;
        case 1:  // Left
            pb->set_rot_x(-1);
            pb->set_rot_y(0);
            break;
        case 2:  // Down
            pb->set_rot_x(0);
            pb->set_rot_y(-1);
            break;
        case 3:  // Right
            pb->set_rot_x(1);
            pb->set_rot_y(0);
            break;
        case 4:  // Up
            pb->set_rot_x(0);
            pb->set_rot_y(1);
            break;
        }
        pb->set_pos_x(Bcmd->xref);
        pb->set_pos_y(Bcmd->yref);
    }
    else {
        CDp_branch *pb = (CDp_branch*)cursde->prpty(P_BRANCH);
        if (pb) {
            cursde->prptyUnlink(pb);
            delete pb;
        }
    }

    if (GRX->GetStatus(DE->de_nophys)) {
        CDp *p = cursde->prpty(P_NOPHYS);
        if (!p) {
            p = new CDp(P_NOPHYS);
            p->set_string("nophys");
            p->set_next_prp(cursde->prptyList());
            cursde->setPrptyList(p);
        }
    }
    else {
        CDp *p = cursde->prpty(P_NOPHYS);
        if (p) {
            cursde->prptyUnlink(p);
            delete p;
        }
    }

    if (!ent.prefix) {
        // This is only possible for a ground terminal.
        if (nodecnt == 1 && !ent.model && !ent.value && !ent.param &&
                !GRX->GetStatus(DE->de_toggle)) {
            CDp_name *pn = (CDp_name*)cursde->prpty(P_NAME);
            if (pn) {
                cursde->prptyUnlink(pn);
                delete pn;
            }
        }
        else {
            Log()->PopUpErr(
                "No Prefix given, this is required for all except "
                "ground terminal.");
            return;
        }
    }
    else {
        CDp_name *pn = (CDp_name*)cursde->prpty(P_NAME);
        if (!pn) {
            pn = new CDp_name;
            pn->set_next_prp(cursde->prptyList());
            cursde->setPrptyList(pn);
        }
        pn->set_name_string(ent.prefix);
    }
    cursde->setDevice(true);

    if (!SCD()->saveAsDev(ent.cname, (arg != 0))) {
        Log()->ErrorLogV(mh::Processing,
            "Save as device failed:\n%s.", Errs()->get_error());
        return;
    }

    if (arg)
        PL()->ShowPrompt("Device cell saved.");
    else
        PL()->ShowPrompt("Device library updated, in current directory.");

    SCD()->PopUpDevEdit(0, MODE_OFF);
}


// When the Branch button is active, clicking in the drawing moves a
// marker around to set the "hot spot" location for the branch property.

// Static function.
void
sDE::de_branch_proc(GtkWidget *caller, void*)
{
    if (GRX->GetStatus(caller)) {
        if (!Bcmd && DE) {
            Bcmd = new sBstate("brloc", "devedit#hspot");
            Bcmd->setXY(DE->de_xref, DE->de_yref);
            if (!EV()->PushCallback(Bcmd))
                delete Bcmd;
            DSP()->ShowCrossMark(DISPLAY, Bcmd->xref, Bcmd->yref,
                HighlightingColor, 20, DSP()->CurMode());
        }
    }
    else {
        if (Bcmd)
            Bcmd->esc();
    }
}
// End of sDE functions.


sBstate::sBstate(const char *nm, const char *hk) : CmdState(nm, hk)
{
    xref = 0;
    yref = 0;
}


sBstate::~sBstate()
{
    Bcmd = 0;
}


void
sBstate::b1down()
{
    DSP()->ShowCrossMark(ERASE, xref, yref, HighlightingColor,
        20, DSP()->CurMode());
    EV()->Cursor().get_xy(&xref, &yref);
    DSP()->ShowCrossMark(DISPLAY, xref, yref, HighlightingColor,
        20, DSP()->CurMode());
}


void
sBstate::b1up()
{
}


void
sBstate::esc()
{
    if (DE)
        DE->set_ref(xref, yref);
    DSP()->ShowCrossMark(ERASE, xref, yref, HighlightingColor, 20,
        DSP()->CurMode());
    EV()->PopCallback(this);
    delete this;
}

