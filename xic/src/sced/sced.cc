
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
 $Id: sced.cc,v 5.37 2016/05/31 06:23:29 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "sced.h"
#include "sced_modlib.h"
#include "sced_spiceipc.h"
#include "sced_errlog.h"
#include "cd_terminal.h"
#include "cd_netname.h"


// Instantiate error logger.
cScedErrLog ScedErrLog;

cSced::cSced()
{
    sc_model_library            = 0;
    sc_spice_interface          = 0;
    sc_global_tab               = 0;

    sc_analysis_cmd             = 0;
    sc_plot_hpr_list            = 0;
    sc_iplot_hpr_list           = 0;

    sc_show_dots                = DotsNone;
    sc_doing_plot               = false;
    sc_doing_iplot              = false;
    sc_iplot_status_changed     = false;
    sc_show_devs                = false;
    sc_include_nophys           = false;

    sc_ghost                    = new cScedGhost;

    setupInterface();

    sc_model_library            = new cModLib;
    sc_spice_interface          = new cSpiceIPC;

    setupBangCmds();
    setupVariables();
    loadScriptFuncs();
}


void
cSced::modelLibraryOpen(const char *name)
{
    sc_model_library->Open(name);
}


void
cSced::modelLibraryClose()
{
    sc_model_library->Close();
}


sp_line_t *
cSced::modelText(const char *model)
{
    return (sc_model_library->ModelText(model));
}


bool
cSced::isModel(const char *name)
{
    return (sc_model_library->IsModel(name));
}


void
cSced::closeSpice()
{
    sc_spice_interface->CloseSpice();
}


bool
cSced::simulationActive()
{
    return (sc_spice_interface->SimulationActive());
}


//
// Tokenizer for SPICE text.
//

#define SEPAR "=,)("

namespace {
    inline bool
    issep(char c)
    {
        return (c && (isspace(c) || strchr(SEPAR, c)));
    }
}


// Static function.
// Return a "spice" token and advance the pointer.  If nv is false,
// tokens are returned individually, and '=' is taken as a separator.
// If nv is true, if the first non-space char following a token is
// '=', the '=' and the following token are appended, returning a
// name=value token.  which may include white space.
//
char *
cSced::sp_gettok(const char **s, bool nv)
{
    if (s == 0 || *s == 0)
        return (0);
    while (issep(**s))
        (*s)++;
    if (!**s)
        return (0);
    const char *st = *s;
    while (**s && !issep(**s))
        (*s)++;

    if (nv) {
        const char *t = *s;
        while (isspace(*t))
            t++;
        if (*t == '=') {
            t++;
            while (isspace(*t))
                t++;
            if (*t && !issep(*t)) {
                while (*t && !issep(*t))
                    t++;
                *s = t;
            }
        }
    }

    char *cbuf = new char[*s - st + 1];
    char *c = cbuf;
    while (st < *s)
        *c++ = *st++;
    *c = 0;

    // Gobble trailing space or commas, but break on other sep chars.
    while (isspace(**s) || **s == ',')
        (*s)++;
    return (cbuf);
}


bool
cSced::logConnect()
{
    return (ScedErrLog.log_connect());
}


void
cSced::setLogConnect(bool b)
{
    ScedErrLog.set_log_connect(b);
}
// End of cSced functions.


 // Constructor.
te_info_t::te_info_t(const CDp_snode *pn, int indx)
{
    ti_name = 0;
    ti_netex = 0;
    ti_layer_name = 0;
    ti_flags = 0;
    ti_index = indx;
    ti_beg = 0;
    ti_end = 0;
    ti_bterm = false;
    ti_has_phys = false;
    if (pn) {
        ti_name = Tstring(pn->get_term_name());
        ti_flags = pn->term_flags();
        CDsterm *term = pn->cell_terminal();
        if (term) {
            ti_has_phys = true;
            ti_layer_name = term->layer() ? term->layer()->name() : 0;
        }
    }
}


 // Constructor.
te_info_t::te_info_t(const CDsterm *term)
{
    ti_name = 0;
    ti_netex = 0;
    ti_layer_name = 0;
    ti_flags = 0;
    ti_index = 0;
    ti_beg = 0;
    ti_end = 0;
    ti_bterm = false;
    ti_has_phys = false;
    if (term) {
        ti_has_phys = true;
        ti_layer_name = term->layer() ? term->layer()->name() : 0;
        CDp_snode *pn = term->node_prpty();
        if (pn) {
            ti_name = Tstring(pn->get_term_name());
            ti_flags = pn->term_flags();
        }
    }
}


 // Constructor.
te_info_t::te_info_t(const CDp_bsnode *pb)
{
    ti_name = 0;
    ti_netex = 0;
    ti_layer_name = 0;
    ti_flags = 0;
    ti_index = 0;
    ti_beg = 0;
    ti_end = 0;
    ti_bterm = true;
    ti_has_phys = false;
    if (pb) {
        ti_name = Tstring(pb->get_term_name());
        sLstr lstr;
        pb->add_label_text(&lstr);
        ti_netex = lstr.string_trim();
        ti_index = pb->index();
        ti_beg = pb->beg_range();
        ti_end = pb->end_range();
    }
}
// End of te_info_t functions.

