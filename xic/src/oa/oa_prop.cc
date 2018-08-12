
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

#include "main.h"
#include "sced.h"
#include "promptline.h"
#include "cd_propnum.h"
#include "si_lisp.h"
#include "oa_if.h"
#include "pcell_params.h"
#include "oa.h"
#include "oa_prop.h"
#include "oa_nametab.h"
#include "oa_tech_observer.h"
#include "oa_lib_observer.h"
#include "oa_errlog.h"
#include "oa_alib.h"
#include "spnumber/spnumber.h"


namespace {
    // Instantiate this.
    cAlibFixup AlibFixup;

    struct sPrmList
    {
        sPrmList(const char *nm, const char *va)
            {
                pl_next = 0;
                pl_name = lstring::copy(nm);
                pl_value = lstring::copy(va);
            }

        static void destroy(sPrmList *p)
            {
                while (p) {
                    sPrmList *px = p;
                    p = p->pl_next;
                    delete [] px->pl_name;
                    delete [] px->pl_value;
                    delete px;
                }
            }

        void set_next(sPrmList *n)    { pl_next = n; }
        sPrmList *next()        const { return (pl_next); }
        const char *name()      const { return (pl_name); }
        const char *value()     const { return (pl_value); }

        char *ex_name()
            {
                char *n = pl_name;
                pl_name = 0;
                return (n);
            }

        char *ex_value()
            {
                char *v = pl_value;
                pl_value = 0;
                return (v);
            }

    private:
        sPrmList *pl_next;
        char *pl_name;
        char *pl_value;
    };


    // Massage the property string to be reasonable as a string
    // property in Xic.  The string will be double quoted if it
    // contains white space or other quote characters.  Newlines are
    // replaced with "\n", a '\' is added ahead of existing quote
    // chars.
    //
    char *format_property_string(const char *str)
    {
        const char *mathchars = "()<>=/*";
        if (str) {
            while (isspace(*str))
                str++;
        }
        if (!str || !*str)
            return (lstring::copy("\"\""));

        int nl = 0, sp = 0, sq = 0, dq = 0, pm = 0, m = 0;
        for (const char *s = str; *s; s++) {
            if (*s == '\n')
                nl++;
            else if (isspace(*s))
                sp++;
            else if (*s == '\'')
                sq++;
            else if (*s == '"')
                dq++;
            else if (*s == '+' || *s == '-')
                pm++;
            else if (strchr(mathchars, *s))
                m++;
        }

        // Simple name or number, accept as-is.
        if (!nl && !sp && !sq && !dq && !m)
            return (lstring::copy(str));

        // Already double-quoted string, accept as-is.
        if (*str == '"' && *(str + strlen(str) - 1) == '"' && dq == 2)
            return (lstring::copy(str));

        // Already single-quoted, has math chars, accept as-is.
        if ((m || pm) && *str == '\'' && *(str + strlen(str) - 1) == '\'' &&
                sq == 2)
            return (lstring::copy(str));

        if ((m || pm) && !sq && !dq) {
            // Has math chars and no quoting.  If just pm, it could be
            // a number.  If not a number, add single quotes.

            if (!m) {
                const char *s = str;
                if (SPnum.parse(&s, true))
                    m++;
            }
            if (m) {
                sLstr lstr;
                lstr.add_c('\'');
                lstr.add(str);
                lstr.add_c('\'');
                return (lstr.string_trim());
            }
            return (lstring::copy(str));
        }

        // Add double quotes and do some mapping.
        sLstr lstr;
        lstr.add_c('"');
        for (const char *s = str; *s; s++) {
            if (*s == '\n') {
                lstr.add_c('\\');
                lstr.add_c('n');
            }
            else if (*s == '\'' || *s == '"') {
                lstr.add_c('\\');
                lstr.add_c(*s);
            }
            else
                lstr.add_c(*s);

        }
        lstr.add_c('"');
        return (lstr.string_trim());
    }


    // This will create a text string from the lisp node tree, but
    // will convert forms like
    //   pPrm(\"foo\")
    // to just 'foo'.
    //
    // The result appears to be a normal expression, not really lisp,
    // but can be quite long.  The text looks horrid with the space
    // separating each token, but it should be easy to parse, and easy
    // to break into continuation lines in SPICE.
    //
    void print_special(lispnode *p0, sLstr *lstr)
    {
        if (!p0)
            return;
        if (p0->type == LN_NODE) {
            if (p0->is_nil()) {
                lstr->add_c(' ');
                lstr->add("nil");
            }
            else {
                if (p0->string) {
                    if (!strcmp(p0->string, "pPar")) {
                        lstr->add_c(' ');
                        lstr->add(p0->args->string);
                        return;
                    }
                    lstr->add_c(' ');
                    lstr->add(p0->string);
                    lstr->add_c('(');
                }
                else {
                    lstr->add_c(' ');
                    lstr->add_c('(');
                }
                for (lispnode *p = p0->args; p; p = p->next)
                    print_special(p, lstr);
                lstr->add_c(' ');
                lstr->add_c(')');
            }
        }
        else if (p0->type == LN_STRING) {
            lstr->add_c(' ');
            lstr->add(p0->string);
        }
        else if (p0->type == LN_NUMERIC) {
            lstr->add_c(' ');
            if (p0->string)
                lstr->add(p0->string);
            else
                lstr->add_g(p0->value);
        }
        else if (p0->type == LN_OPER) {
            if (p0->lhs) {
                print_special(p0->lhs, lstr);
                lstr->add_c(' ');
            }
            lstr->add(p0->string);
            print_special(p0->args, lstr);
        }
        else if (p0->type == LN_QSTRING) {
            lstr->add_c(' ');
            lstr->add_c('"');
            lstr->add(p0->string);
            lstr->add_c('"');
        }
    }


    // Parameter strings may contain constructs of the form
    //   pPar(\"foo\")
    // where the 'foo' is a parameter name.  This parameter should be
    // listed in the containing cell CDF data, but can be overridden
    // in instance placements.  This should all work in Xic if we just
    // replace the construct with the argument, which is what this
    // function does.  It will copy the string if there are no such
    // constructs.
    // 
    char *promote_param_names(const char *str)
    {
        if (!str)
            return (0);
        while (isspace(*str))
            str++;
        if (strstr(str, "pPar(")) {
            cLispEnv lisp;
            cLispEnv *tenv = lispnode::set_env(&lisp);
            const char *s = str;
            lispnode *p0 = cLispEnv::get_lisp_list(&s);
            sLstr lstr;
            print_special(p0, &lstr);

            lispnode::destroy(p0);
            lispnode::set_env(tenv);
            return (lstr.string_trim());
        }
        return (lstring::copy(str));
    }
}


CDp *
cOAprop::handleProperties(const oaObject *object, DisplayMode mode)
{
    if (!object->hasProp())
        return (0);

    oaType type = object->getType();
    bool ignore_params = false;
    sPrmList *prm_list = 0, *prm_end = 0;
    CDp *p0 = 0;    // General properties, always applied.
    CDp *pxic = 0;  // Properties from Xic.
    CDp *pvrt = 0;  // Xic properties created for Virtuoso.

    const cOAelecInfo *cdf = 0;
    bool is_pcell = false;
    bool is_symbol = false;
    if (mode == Electrical && type == oacDesignType) {
        oaDesign *des = (oaDesign*)object;
        oaScalarName cellName;
        des->getCellName(cellName);
        oaCell *cell = oaCell::find(des->getLib(), cellName);
        if (cell) {
            cdf = getCDFinfo(cell, p_def_symbol, p_def_dev_prop);

            oaScalarName libName;
            des->getLibName(libName);
            oaScalarName viewName(oaNativeNS(), p_def_layout);
            if (oaDesign::exists(libName, cellName, viewName)) {
                oaDesign *pdes =
                    oaDesign::open(libName, cellName, viewName, 'r');
                if (pdes->isSuperMaster())
                    is_pcell = true;
                pdes->close();
            }
        }
        is_symbol =
            (des->getViewType() == oaViewType::get(oacSchematicSymbol));
    }

    char *model_prp = 0;
    oaIter<oaProp> iter(object->getProps());
    while (oaProp *prp = iter.getNext()) {
        oaString name;
        prp->getName(name);

        if (OAerrLog.debug_pcell()) {
            sLstr lstr;
            lstr.add("property ");
            lstr.add(name);
            lstr.add(": ");
            oaString tn = prp->getType().getName();
            lstr.add_c('(');
            lstr.add(tn);
            lstr.add(") ");
            oaString value;
            prp->getValue(value);
            char *tval = format_property_string(value);
            lstr.add(tval);
            delete [] tval;
            OAerrLog.add_log(OAlogPCell, "%s.", lstr.string());
        }

        // These are Xic properties that have been saved in OA.
        if (lstring::prefix(OA_XICP_PFX, name)) {
            const char *np = (const char*)name + strlen(OA_XICP_PFX);
            if (lstring::prefix("version", np)) {
                // This signals that the cell was saved from Xic, so
                // electrical coordinates don't need scaling.  This
                // also signals that data are in "xic mode" rather
                // than "virtuoso mode".  In "xic mode" we keep and
                // apply the Xic properties, and skip generating
                // properties from OA objects, as if done when reading
                // "virtuoso mode".

                p_from_xic = true;
            }
            else if (isdigit(*np)) {
                int num = atoi(np);
                oaString value;
                prp->getValue(value);
                CDp *px = new CDp(value, num);
                px->set_next_prp(pxic);
                pxic = px;
            }
            continue;
        }

        // Label attributes, already handled.
        if (!strcasecmp("XICP_NO_INST_VIEW", name))
            continue;
        if (!strcasecmp("XICP_USE_LINE_LIMIT", name))
            continue;

        // These are Ciranova properties for stretch handles and
        // abutment, that we support.  These are all properties of
        // shapes.
        //
        if (!strcasecmp("pycStretch", name)) {
            oaString value;
            prp->getValue(value);
            CDp *px = new CDp(value, XICP_GRIP);
            px->set_next_prp(p0);
            p0 = px;
            continue;
        }
        if (!strcasecmp("pycAbutClass", name)) {
            oaString value;
            prp->getValue(value);
            CDp *px = new CDp(value, XICP_AB_CLASS);
            px->set_next_prp(p0);
            p0 = px;
            continue;
        }
        if (!strcasecmp("pycAbutRules", name)) {
            oaString value;
            prp->getValue(value);
            CDp *px = new CDp(value, XICP_AB_RULES);
            px->set_next_prp(p0);
            p0 = px;
            continue;
        }
        if (!strcasecmp("pycAbutDirections", name)) {
            oaString value;
            prp->getValue(value);
            CDp *px = new CDp(value, XICP_AB_DIRECTS);
            px->set_next_prp(p0);
            p0 = px;
            continue;
        }
        if (!strcasecmp("pycShapeName", name)) {
            oaString value;
            prp->getValue(value);
            CDp *px = new CDp(value, XICP_AB_SHAPENAME);
            px->set_next_prp(p0);
            p0 = px;
            continue;
        }
        if (!strcasecmp("pycPinSize", name)) {
            oaString value;
            prp->getValue(value);
            CDp *px = new CDp(value, XICP_AB_PINSIZE);
            px->set_next_prp(p0);
            p0 = px;
            continue;
        }

        if (mode == Electrical &&
                (type == oacDesignType || type == oacScalarInstType ||
                type == oacVectorInstBitType || type == oacArrayInstType ||
                type == oacVectorInstType)) {

            // Cadence/Virtuoso property.
            if (!strcasecmp("instNamePrefix", name) && type == oacDesignType) {
                // Electrical cell.
                // Add this as a P_NAME property.
                oaString value;
                prp->getValue(value);
                sLstr lstr;
                if (value.isEqual("pin", true)) {
                    // Special case, the cell is an "ipin" or "opin". 
                    // These are converted to a null device, i.e.,
                    // something that appears in the drawing but is
                    // electrically invisible.  They correspond to
                    // cell terminal boxes.  We will skip adding model
                    // and param properties.

                    lstr.add_c(P_NAME_NULL);
                    lstr.add(" 0");
                    ignore_params = true;

                    CDp *px = new CDp(lstr.string(), P_NAME);
                    px->set_next_prp(pvrt);
                    pvrt = px;
                }
                else {
                    // There are typically two sources for the device
                    // name prefix:  the prefix from the CDF, and the
                    // instNamePrefix property.  We notice that the
                    // CDF prefix is either 'X', or the same as the
                    // property prefix.  The CDF 'X' indicates that
                    // the device should be issued as a subcircuit in
                    // SPICE output, as the "model" name actually
                    // resolves to a .subckt definition in the HSPICE
                    // library.
                    //
                    // The Xic name property will be set from the
                    // instNamePrefix property always.  If the CDF
                    // prefix is 'X', we set a macro property, which
                    // will cause the SPICE output generator to add an
                    // 'X' ahead of the device name.  In this case, we
                    // also add a model property containing the cell
                    // name.


                    bool modprop = false;
                    if (cdf && mode == Electrical && type == oacDesignType) {
                        oaDesign *des = (oaDesign*)object;
                        oaScalarName cellName;
                        des->getCellName(cellName);
                        oaString cn;
                        cellName.get(cn);

                        const char *pf = cdf->prefix();
                        if (pf && (*pf == 'X' || *pf == 'x') &&
                                (*value != 'X' && *value != 'x')) {

                            modprop = true;
                        }
                    }
                    if (modprop) {
                        lstr.add(value);
                        lstr.add(" macro");

                        CDp *px = new CDp(lstr.string(), P_NAME);
                        px->set_next_prp(pvrt);
                        pvrt = px;
                    }
                    else {
                        lstr.add(value);
                    }

                    // Now, in some cases we need to add a model
                    // property with the device name, at other times
                    // not.  This may be tricky.
                    //
                    // If the CDF prefix leads with 'X', then we
                    // always need the model property.  This provides
                    // the macro name.
                    //
                    // Otherwise it really isn't clear, but we will
                    // assume that if the device physical part is a
                    // pcell, then we add the model property,
                    // otherwise not.  It is assumed that a pcell will
                    // use an actual .model or .subckt.  If not a
                    // pcell, then it is assumed that the device is
                    // something like a "pcapacitor", which is taken
                    // as a standard SPICE element.

                    if ((modprop || is_pcell) && !model_prp) {
                        const cOAelecInfo::param *pm = cdf->find_param("model");
                        if (pm)
                            model_prp = lstring::copy(pm->value());
                        else {
                            oaString cellname;
                            ((oaDesign*)object)->getCellName(oaNativeNS(),
                                cellname);
                            model_prp = lstring::copy((const char*)cellname);
                        }
                    }
                }
                continue;
            }

            // Virtuoso stuff, mostly to ignore.
            if (!strcasecmp("portOrder", name))
                continue;
            if (!strcasecmp("interfaceLastChanged", name))
                continue;
            if (!strcasecmp("instancesLastChanged", name))
                continue;
            if (!strcasecmp("vendorName", name))
                continue;
            if (!strcasecmp("partName", name)) {
                // Keep this for the @partName evalText.
                oaString value;
                prp->getValue(value);
                CDp *px = new CDp(value, XICP_PARTNAME);
                px->set_next_prp(pvrt);
                pvrt = px;
                continue;
            }
            if (!strcasecmp("pin#", name))
                continue;
            if (!strcasecmp("instance#", name))
                continue;
            if (!strcasecmp("net#", name))
                continue;
            if (!strcasecmp("lastSchematicExtraction", name))
                continue;
            if (!strcasecmp("connectivityLastUpdated", name))
                continue;
            if (!strcasecmp("schXtrVersion", name))
                continue;
            if (!strcasecmp("schGeometryLastUpdated", name))
                continue;
            if (!strcasecmp("schGeometryVersion", name))
                continue;
            if (!strcasecmp("viewNameList", name))
                continue;
            if (!strcasecmp("hspiceS", name))
                continue;
            if (!strcasecmp("spectre", name))
                continue;
            if (!strcasecmp("lvsIgnore", name))
                continue;
            if (!strcasecmp("model", name)) {
                // If there is a parameter named "model" take it as the
                // model, overriding any existing setting.

                oaString value;
                prp->getValue(value);
                delete [] model_prp;
                model_prp = lstring::copy((const char*)value);
                continue;
            }

            // Electrical cell or instance, save the property in a
            // list for now.

            oaString value;
            prp->getValue(value);
            if (!prm_list)
                prm_list = prm_end = new sPrmList(name, value);
            else {
                prm_end->set_next(new sPrmList(name, value));
                prm_end = prm_end->next();
            }
        }
        else {
            // Keep as individual XICP_OA_UNKN properties.
            sLstr lstr;
            lstr.add(name);
            lstr.add(": ");
            oaString tn = prp->getType().getName();
            lstr.add_c('(');
            lstr.add(tn);
            lstr.add(") ");
            oaString value;
            prp->getValue(value);
            char *tval = format_property_string(value);
            lstr.add(tval);
            delete [] tval;
            CDp *px = new CDp(lstr.string(), XICP_OA_UNKN);
            px->set_next_prp(pvrt);
            pvrt = px;
            continue;
        }
    }
    if (p_from_xic) {
        // The database is saved for use by Xic.  It is NOT intended to
        // be compatible with Virtuoso.

        sPrmList::destroy(prm_list);
        CDp::destroy(pvrt);
        if (pxic) {
            CDp *ptmp = p0;
            p0 = pxic;
            if (ptmp) {
                CDp *p = p0;
                while (p->next_prp())
                    p = p->next_prp();
                p->set_next_prp(ptmp);
            }
        }
        return (p0);
    }

    // Otherwise, we're reading a Virtuoso database.  There shouldn't
    // be any Xic properties.
    CDp::destroy(pxic);

    if (pvrt) {
        CDp *ptmp = p0;
        p0 = pvrt;
        if (ptmp) {
            CDp *p = p0;
            while (p->next_prp())
                p = p->next_prp();
            p->set_next_prp(ptmp);
        }
    }
    if (mode == Electrical && !ignore_params && !is_pcell) {
        // Don't add a param property to the electrical part
        // of a pcell!

        cdf = 0;

        // For an instance, open the CDF for its master.
        if (type == oacScalarInstType || type == oacVectorInstBitType ||
                type == oacArrayInstType || type == oacVectorInstType) {
            oaInst *inst = (oaInst*)object;
            oaScalarName libName, cellName;
            inst->getLibName(libName);
            inst->getCellName(cellName);
            oaLib *lib = oaLib::find(libName);
            if (lib) {
                lib->getAccess(oacReadLibAccess);
                oaCell *cell = oaCell::find(lib, cellName);
                if (cell)
                    cdf = getCDFinfo(cell, p_def_symbol, p_def_dev_prop);
                lib->releaseAccess();
            }
        }

        sLstr lstr;
        if (cdf && cdf->inst_params()) {
            cdf->create_inst_param_string(lstr, prm_list);
        }
        else {
            // No CDF instance parameter info, use what we've collected.
            for (sPrmList *sl = prm_list; sl; sl = sl->next()) {
                if (lstr.string())
                    lstr.add_c('\n');
                lstr.add(sl->name());
                lstr.add_c('=');
                char *val = format_property_string(sl->value());
                lstr.add(val);
                delete [] val;
            }
            if (type == oacDesignType && !is_symbol && cdf &&
                    cdf->all_params()) {
                // A non-pcell schematic master with associated
                // parameters but (of course) no instance parameters. 
                // Grab a copy of the parameter list, the default
                // values are needed for instantiation and are not
                // available elsewhere.

                cdf->create_all_param_string(lstr);
            }
        }
        if (lstr.string()) {
            bool valu = false;
            if (cdf) {
                oaInst *inst = (oaInst*)object;
                oaScalarName libName;
                inst->getLibName(libName);
                oaString libname;
                libName.get(oaNativeNS(), libname);

                // Do some hackery on the analogLib devices.  We don't
                // have access to the Skill function that formats
                // output, but we can fake it, to an extent.

                if (libname == ANALOG_LIB) {
                    oaScalarName cellName;
                    inst->getCellName(cellName);
                    oaString cellname;
                    cellName.get(oaNativeNS(), cellname);
                    valu = AlibFixup.prpty_fix((const char *)cellname, lstr);
                }
            }
            if (valu) {
                CDp *px = new CDp(lstr.string(), P_VALUE);
                px->set_next_prp(p0);
                p0 = px;
            }
            else {
                CDp *px = new CDp(lstr.string(), P_PARAM);
                px->set_next_prp(p0);
                p0 = px;
            }
        }
        sPrmList::destroy(prm_list);
    }
    if (model_prp) {
        // Add a model property.
        CDp *px = new CDp(model_prp, P_MODEL);
        px->set_next_prp(p0);
        p0 = px;
        delete [] model_prp;
    }

    return (p0);
}


// Static function.
// A utility to get the Ciranova pcell parameter constraint properties
// from an oaDesign.  If any are found, a hash table is returned. 
// This is keyed by the parameter name, and payloads are the
// constraint strings.
//
SymTab *
cOAprop::getPropTab(oaDesign *design)
{
    SymTab *pctab = 0;
    if (design->hasProp()) {
        oaIter<oaProp> dp_iter(design->getProps());
        while (oaProp *prp = dp_iter.getNext()) {
            oaString name, val;
            prp->getName(name);
            prp->getValue(val);
            if (name == "cnParamConstraints" && val == "oaHierProp") {
                // This is a Ciranova pcell.
                oaIter<oaProp> pp_iter(prp);
                while ((prp = pp_iter.getNext()) != 0) {
                    prp->getName(name);
                    prp->getValue(val);
                    if (!pctab)
                        pctab = new SymTab(true, true);
                    pctab->add(lstring::copy(name), lstring::copy(val),
                        false);
                }
            }
        }
    }
    return (pctab);
}


// Static function.
// This generates a PCellParam list from the oaParamArray.  The
// optional SymTab, as returned from getPropTab, will provide
// constraint strings.
//
PCellParam *
cOAprop::getPcParameters(const oaParamArray &parray, SymTab *pctab)
{
    PCellParam *p0 = 0, *pe = 0;
    int sz = parray.getSize();
    for (int i = 0; i < sz; i++) {
        oaString name;
        parray[i].getName(name);

        switch (parray[i].getType()) {
        case oacIntParamType:
            {
                const char *c = 0;
                if (pctab) {
                    c = (const char*)SymTab::get(pctab, name);
                    if (c == (const char*)ST_NIL)
                        c = 0;
                }
                PCellParam *p = new PCellParam(PCPint, name, c,
                    (long)parray[i].getIntVal());
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->setNext(p);
                    pe = pe->next();
                }
            }
            break;
        case oacFloatParamType:
            {
                const char *c = 0;
                if (pctab) {
                    c = (const char*)SymTab::get(pctab, name);
                    if (c == (const char*)ST_NIL)
                        c = 0;
                }
                PCellParam *p = new PCellParam(PCPfloat, name, c,
                    parray[i].getFloatVal());
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->setNext(p);
                    pe = pe->next();
                }
            }
            break;
        case oacStringParamType:
            {
                const char *c = 0;
                if (pctab) {
                    c = (const char*)SymTab::get(pctab, name);
                    if (c == (const char*)ST_NIL)
                        c = 0;
                }
                oaString val;
                parray[i].getStringVal(val);
                PCellParam *p = new PCellParam(PCPstring, name, c, val);
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->setNext(p);
                    pe = pe->next();
                }
            }
            break;
        case oacAppParamType:
            {
                // This isn't really handled.
                const char *c = 0;
                if (pctab) {
                    c = (const char*)SymTab::get(pctab, name);
                    if (c == (const char*)ST_NIL)
                        c = 0;
                }
                int sz = parray[i].getNumBytes();
                unsigned char *a = new unsigned char[sz + 1];
                parray[i].getAppVal(a);
                PCellParam *p = new PCellParam(PCPappType, name, c,
                    "foo", a, sz);
                delete [] a;
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->setNext(p);
                    pe = pe->next();
                }
            }
            break;
        case oacDoubleParamType:
            {
                const char *c = 0;
                if (pctab) {
                    c = (const char*)SymTab::get(pctab, name);
                    if (c == (const char*)ST_NIL)
                        c = 0;
                }
                PCellParam *p = new PCellParam(PCPdouble, name, c,
                    parray[i].getDoubleVal());
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->setNext(p);
                    pe = pe->next();
                }
            }
            break;
        case oacBooleanParamType:
            {
                const char *c = 0;
                if (pctab) {
                    c = (const char*)SymTab::get(pctab, name);
                    if (c == (const char*)ST_NIL)
                        c = 0;
                }
                PCellParam *p = new PCellParam(PCPbool, name, c,
                    (bool)parray[i].getBooleanVal());
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->setNext(p);
                    pe = pe->next();
                }
            }
            break;
        case oacTimeParamType:
            {
                const char *c = 0;
                if (pctab) {
                    c = (const char*)SymTab::get(pctab, name);
                    if (c == (const char*)ST_NIL)
                        c = 0;
                }
                PCellParam *p = new PCellParam(PCPtime, name, c,
                    (long)parray[i].getTimeVal());
                if (!p0)
                    p0 = pe = p;
                else {
                    pe->setNext(p);
                    pe = pe->next();
                }
            }
            break;
        }
    }
    return (p0);
}


// Static function.
// Set the array parameters from the PCellParam list.
//
void
cOAprop::savePcParameters(const PCellParam *prms, oaParamArray &parray)
{
    int i = 0;
    for (const PCellParam *p = prms; p; p = p->next())
        i++;
    parray.setSize(i);
    parray.setNumElements(i);
    i = 0;
    for (const PCellParam *p = prms; p; p = p->next()) {
        parray[i].setName(p->name());
        switch (p->type()) {
        case PCPbool:
            parray[i].setBooleanVal(p->boolVal());
            break;
        case PCPint:
            parray[i].setIntVal(p->intVal());
            break;
        case PCPtime:
            parray[i].setTimeVal(p->timeVal());
            break;
        case PCPfloat:
            parray[i].setFloatVal(p->floatVal());
            break;
        case PCPdouble:
            parray[i].setDoubleVal(p->doubleVal());
            break;
        case PCPstring:
            parray[i].setStringVal(p->stringVal());
            break;
        case PCPappType:
            parray[i].setAppVal(p->appValSize(), p->appVal());
            parray[i].setAppType(p->appName());
            break;
        }
        i++;
    }
}


// Static function.
// Add the basic electrical cell properties if missing, add missing
// property labels.
//
void
cOAprop::checkFixProperties(CDc *cdesc)
{
    // If the properties are absent in the instance, call
    // prptyAddStruct to apply the properties from the master.
    //
    if (!cdesc->prpty(P_NAME) && !cdesc->prpty(P_NODE))
        cdesc->prptyAddStruct(true);

    // Add associated label to properties that have missing labels.
    //
    for (CDp *pd = cdesc->prpty_list(); pd; pd = pd->next_prp())
        addPrptyLabel(cdesc, pd);
}


// Static function.
// If pdesc is missing a label, create it.  This does not register any
// action for undo.
//
bool
cOAprop::addPrptyLabel(CDc *cdesc, CDp *pdesc)
{
    if (!cdesc || cdesc->type() != CDINSTANCE)
        return (false);
    if (!pdesc)
        return (false);
    if (pdesc->bound())
        return (true);
    if (pdesc->value() == P_NAME) {
        // The "null" devices don't have a name label.
        CDp_cname *pa = (CDp_cname*)pdesc;
        int key = pa->key();
        if (key != P_NAME_TERM && !isalpha(key))
            return (true);
    }
    CDs *sdesc = cdesc->parent();
    if (!sdesc)
        return (false);
    bool copied = false;
    Label label;
    label.label = hyList::dup(pdesc->label_text(&copied, cdesc));
    if (!label.label)
        return (true);
    if (pdesc->value() == P_VALUE || pdesc->value() == P_PARAM)
        label.xform |= TXTF_LIML;
    DSP()->DefaultLabelSize(label.label, Electrical, &label.width,
        &label.height);
    SCD()->labelPlacement(pdesc->value(), cdesc, &label);
    CDla *nlabel;
    if (sdesc->makeLabel(SCD()->defaultLayer(pdesc), &label, &nlabel,
            false) != CDok)
        return (false);
    if (copied)
        hyList::destroy(label.label);
    if (!nlabel)
        return (false);;
    pdesc->bind(nlabel);
    if (!nlabel->link(sdesc, cdesc, pdesc))
        return (false);
    return (true);
}


// Static function.
// In Virtuoso, there can be an oaCellDMData that provides useful
// things like the device port order and device SPICE parameters. 
// This exists for devices and subcircuits originating from Virtuoso. 
// The info is stored in a data.dm file in the library directory. 
// The data are addressed by opening the oaCellDMData and iterating
// through the properties.  The interesting property is cdfData. 
// This is parsed with the lisp parser to obtain the port ordering
// and other info.
//
// Subcircuits seem not to have parameter info, but do provide
// terminal ordering.  I don't know if this is documented anywhere,
// but it is pretty easy to figure out.  Here we look for this info,
// and if found we store it in a table for later use, and return a
// pointer to it.
//
// If the info is missing, we will look for a portOrder property in
// the schematicSymbol view.  If found, this will be used to create a
// "fake" cOAelecInfo.
//
// The final two arguments are the default symbolic view name and the
// default simulator view name (e.g., "symbol" and "HspiceD").  At least
// one must not be null.
//
const cOAelecInfo *
cOAprop::getCDFinfo(const oaCell *cell, const char *def_symbol,
    const char *def_dev_prop)
{
    oaLib *lib = cell->getLib();
    oaScalarName libName, cellName;
    lib->getName(libName);
    cell->getName(cellName);
    oaString cname;
    cellName.get(cname);

    // Return if we've processed this cell.
    const cOAelecInfo *cdf = cOAelecInfo::find(cname);
    if (cdf)
        return (cdf);

    if (oaCellDMData::exists(libName, cellName)) {

        // This never fails.
        oaCellDMData *cd = oaCellDMData::open(libName, cellName, 'r');

        // If set, dump ascii files of formatted CDF data in the cwd.
        //
        bool write_cdf = CDvdb()->getVariable(VA_OaDumpCdfFiles);

        FILE *fp = 0;
        if (write_cdf) {
            oaString fname = cname + ".cdf";
            fp = fopen(fname, "w");
            if (!fp) {
                // Open failed, silently ignore request to write files.
                write_cdf = false;
            }
        }

        oaIter<oaProp> iter(cd->getProps());
        while (oaProp *prp = iter.getNext()) {
            oaString name;
            prp->getName(name);
            oaString value;
            prp->getValue(value);

            // This property contains a long block of text that
            // provides CDF data for the device.

            if (name == "cdfData") {
                const char *s = value;
                cLispEnv lisp;
                lispnode *p0 = cLispEnv::get_lisp_list(&s);

                if (write_cdf) {
                    for (lispnode *p = p0; p; p = p->next)
                        lispnode::print(p, fp, false, false);
                    fprintf(fp, "\n");
                }

                cOAelecInfo::parse_CDF(p0, cell);
                lispnode::destroy(p0);
            }

            // There are other definitions, too.  So far I don't see
            // anything too useful.  Uncomment this to output these as
            // well.
            // else if (write_cdf)
            //   fprintf(fp, "%s %s\n", (const char*)name, (const char*)value);
        }
        if (fp)
            fclose(fp);
    }

    // Check the portOrder and instNamePrefix properties.

    oaProp *ppo = 0;
    oaProp *pip = 0;

    if (def_dev_prop) {
        oaScalarName viewName(oaNativeNS(), def_dev_prop);
        oaCellView *cv = oaCellView::find(lib, cellName, viewName);
        if (cv && cv->getView()->getViewType() ==
                oaViewType::get(oacSchematicSymbol)) {
            oaDesign *des = oaDesign::open(libName, cellName, viewName, 'r');
            ppo = oaProp::find(des, "portOrder");
            pip = oaProp::find(des, "instNamePrefix");
        }
    }
    if (!ppo && !pip && def_symbol) {
        oaScalarName viewName(oaNativeNS(), def_symbol);
        oaCellView *cv = oaCellView::find(lib, cellName, viewName);
        if (cv && cv->getView()->getViewType() ==
                oaViewType::get(oacSchematicSymbol)) {
            oaDesign *des = oaDesign::open(libName, cellName, viewName, 'r');
            ppo = oaProp::find(des, "portOrder");
            pip = oaProp::find(des, "instNamePrefix");
        }
    }

    cOAelecInfo::set_prp_info(cname, pip, ppo);
    return (cOAelecInfo::find(cname));
}
// End of cOAprop functions.


SymTab *cOAelecInfo::cdf_tab;

cOAelecInfo::cOAelecInfo()
{
    cdf_name = 0;
    cdf_prefix = 0;
    cdf_simulator = 0;
    cdf_inst_params = 0;
    cdf_all_params = 0;
    cdf_terms = 0;
    cdf_ports = 0;
    cdf_map = 0;
    cdf_cell = 0;
}


cOAelecInfo::~cOAelecInfo()
{
    delete [] cdf_name;
    delete [] cdf_prefix;
    delete [] cdf_simulator;
    delete [] cdf_inst_params;
    if (cdf_all_params) {
        // The strings are used in the cdf_inst_params, too.
        for (int i = 0; cdf_all_params[i].p_name; i++) {
            delete [] cdf_all_params[i].p_name;
            delete [] cdf_all_params[i].p_value;
        }
    }
    if (cdf_terms) {
        for (char **p = cdf_terms; *p; p++)
            delete [] *p;
        delete [] cdf_terms;
    }
    if (cdf_ports) {
        for (char **p = cdf_ports; *p; p++)
            delete [] *p;
        delete [] cdf_ports;
    }
    if (cdf_map) {
        for (char **p = cdf_map; *p; p++)
            delete [] *p;
        delete [] cdf_map;
    }
}


// Static function.
// Set the info from the properties.
//
void
cOAelecInfo::set_prp_info(const char *cname, const oaProp *pip,
    const oaProp *ppo)
{
    if (!cname)
        return;
    if (!pip && !ppo)
        return;
    cOAelecInfo *cdf = 0;
    if (cdf_tab) {
        cdf = (cOAelecInfo*)SymTab::get(cdf_tab, cname);
        if (cdf == (cOAelecInfo*)ST_NIL)
            cdf = 0;
    }
    if (!cdf) {
        cdf = new cOAelecInfo;
        cdf->cdf_name = lstring::copy(cname);
        cdf->add();
    }

    // Keep the CDF prefix if we already have one.
    if (!cdf->cdf_prefix && pip) {
        oaString value;
        pip->getValue(value);
        cdf->cdf_prefix = lstring::copy(value);
    }

    // The portOrder and termOrder can be different!
    if (!cdf->cdf_ports && ppo) {
        oaString value;
        ppo->getValue(value);
        const char *s = value;
        cLispEnv lisp;
        lispnode *p0 = cLispEnv::get_lisp_list(&s);
        if (p0 && p0->type == LN_NODE) {
            int cnt = 0;
            for (lispnode *a = p0->args; a; a = a->next) {
                if (a->type != LN_STRING && a->type != LN_QSTRING)
                    continue;
                cnt++;
            }
            char **ary = new char*[cnt+1];
            cnt = 0;
            for (lispnode *a = p0->args; a; a = a->next) {
                if (a->type != LN_STRING && a->type != LN_QSTRING)
                    continue;
                ary[cnt++] = lstring::copy(a->string);
            }
            ary[cnt] = 0;
            cdf->cdf_ports = ary;
        }
        lispnode::destroy(p0);
    }
}


// Static function.
// Parse the top-most node of the cdfData string.
//
void
cOAelecInfo::parse_CDF(lispnode *p, const oaCell *cell)
{
    if (p->type != LN_NODE || !cell)
        return;
    cOAelecInfo *cdf = new cOAelecInfo;
    cdf->cdf_cell = cell;
    for (lispnode *a = p->args; a; a = a->next) {
        if (a->type != LN_STRING)
            continue;
        if (!strcmp(a->string, "parameters"))
            cdf->parse_parameters(a->next);
        else if (!strcmp(a->string, "propList"))
            cdf->parse_propList(a->next);
    }
    cdf->cdf_cell = 0;

    if (!cdf->name()) {
        // Something is nuts.
        delete cdf;
        return;
    }
    if (find(cdf->name())) {
        // Device already in table, this "can't happen".
        delete cdf;
        return;
    }
    cdf->add();
}


// Static function.
// Return the CDF info from the table, if found.
//
const cOAelecInfo *
cOAelecInfo::find(const char *name)
{
    if (!cdf_tab || !name)
        return (0);
    cOAelecInfo *info = (cOAelecInfo*)SymTab::get(cdf_tab, name);
    if (info == (cOAelecInfo*)ST_NIL)
        return (0);
    return (info);
}


// The CDF gives the parameter names to use, in order.  Assemble a
// parameter string based on this.
//
void
cOAelecInfo::create_inst_param_string(sLstr &lstr,
    const sPrmList *prm_list) const
{
    {
        const cOAelecInfo *oit = this;
        if (!oit)
            return;
    }
    if (!cdf_inst_params)
        return;

    for (const cOAelecInfo::param *p = cdf_inst_params; p->name(); p++) {
        const char *pmap = p->name();
        if (cdf_map) {
            const char *const *m = cdf_map;
            while (*m) {
                if (!strcmp(pmap, *m)) {
                    pmap = *(m+1);
                    OAerrLog.add_log(OAlogPCell, "mapping %s to %s",
                        *m, *(m+1));
                    break;
                }
                m++;
                if (!*m)
                    break;
                m++;
            }
        }
        bool found = false;
        for (const sPrmList *sl = prm_list; sl; sl = sl->next()) {
            if (!strcmp(pmap, sl->name())) {
                if (lstr.string())
                    lstr.add_c('\n');
                lstr.add(p->name());
                lstr.add_c('=');
                char *tt = promote_param_names(sl->value());
                char *val = format_property_string(tt);
                delete [] tt;
                lstr.add(val);
                delete [] val;
                found = true;
                break;
            }
        }
        if (!found) {
            // A parameter was not found among the properties,
            // which means that it takes the default value. 
            // If a default is not found, it is skipped.

            // Unfortunately, adding in unmentioned parameters with
            // defaults can introduce names unknown to WRspice.  This
            // is sort-of bad, but do some filtering here.
            //
            if (!strcmp(p->name(), "polyCoef"))
                continue;

            const char *value = p->value();
            if (pmap != p->name()) {
                value = 0;
                const param *a = find_param(pmap);
                if (a)
                    value = a->value();
            }
            if (value && *value) {
                if (lstr.string())
                    lstr.add_c('\n');
                lstr.add(p->name());
                lstr.add_c('=');
                char *v = format_property_string(value);
                lstr.add(v);
                delete [] v;
            }
        }
    }
}


void
cOAelecInfo::create_all_param_string(sLstr &lstr) const
{
    {
        const cOAelecInfo *oit = this;
        if (!oit)
            return;
    }
    if (!cdf_all_params)
        return;

    for (const cOAelecInfo::param *p = cdf_all_params; p->name(); p++) {
        const char *pmap = p->name();
        if (cdf_map) {
            const char *const *m = cdf_map;
            while (*m) {
                if (!strcmp(pmap, *m)) {
                    pmap = *(m+1);
                    OAerrLog.add_log(OAlogPCell, "mapping %s to %s",
                        *m, *(m+1));
                    break;
                }
                m++;
                if (!*m)
                    break;
                m++;
            }
        }

        const char *value = p->value();
        if (pmap != p->name()) {
            value = 0;
            const param *a = find_param(pmap);
            if (a)
                value = a->value();
        }
        if (value) {
            if (lstr.string())
                lstr.add_c('\n');
            lstr.add(p->name());
            lstr.add_c('=');
            char *v = format_property_string(value);
            lstr.add(v);
            delete [] v;
        }
    }
}


// Add a new cOAelecInfo to the table.  We don't check if the name is
// already in use here, the caller should do this.
//
void
cOAelecInfo::add()
{
    {
        cOAelecInfo *oit = this;
        if (!oit)
            return;
    }
    if (!cdf_tab)
        cdf_tab = new SymTab(false, false);
    cdf_tab->add(name(), this, false);
}


// Set the list of instance parameters to be passed on a SPICE device
// line.  The all_params array must have been parsed already, the
// instance parameters must be listed there.
//
void
cOAelecInfo::parse_instParameters(lispnode *p)
{
    if (p->type != LN_NODE)
        return;
    if (!cdf_all_params)
        return;

    int cnt = 0;
    for (lispnode *a = p->args; a; a = a->next) {
        if (a->type != LN_STRING && a->type != LN_QSTRING)
            continue;
        cnt++;
    }
    if (!cnt)
        return;

    cdf_inst_params = new param[cnt + 1];
    cnt = 0;
    for (lispnode *a = p->args; a; a = a->next) {
        if (a->type != LN_STRING && a->type != LN_QSTRING)
            continue;
        for (param *p = cdf_all_params; p->p_name; p++) {
            if (!strcmp(a->string, p->p_name)) {
                cdf_inst_params[cnt].p_name = p->p_name;
                cdf_inst_params[cnt].p_value = p->p_value;
                cnt++;
                break;
            }
        }
    }
    cdf_inst_params[cnt].p_name = 0;
    cdf_inst_params[cnt].p_value = 0;
}


// Set the list of ordered terminal names.
//
void
cOAelecInfo::parse_termOrder(lispnode *p)
{
    if (p->type != LN_NODE)
        return;
    int cnt = 0;
    for (lispnode *a = p->args; a; a = a->next) {
        if (a->type != LN_STRING && a->type != LN_QSTRING)
            continue;
        cnt++;
    }
    if (!cnt)
        return;
    char **ary = new char*[cnt + 1];
    cnt = 0;
    for (lispnode *a = p->args; a; a = a->next) {
        if (a->type != LN_STRING && a->type != LN_QSTRING)
            continue;
        ary[cnt++] = lstring::copy(a->string);
    }
    ary[cnt] = 0;
    cdf_terms = ary;
}


void
cOAelecInfo::parse_propMapping(lispnode *p)
{
    if (p->type != LN_NODE)
        return;
    if (!p->args)
        return;
    int cnt = 0;
    for (lispnode *a = p->args->next; a; a = a->next) {
        if (a->type != LN_STRING && a->type != LN_QSTRING)
            continue;
        cnt++;
    }
    if (!cnt)
        return;
    if (cnt & 1) {
        OAerrLog.add_log(OAlogPCell, "propMapping has odd number elements");
        cnt--;
    }
    char **ary = new char*[cnt + 1];
    cnt = 0;
    for (lispnode *a = p->args->next; a; a = a->next) {
        if (a->type != LN_STRING && a->type != LN_QSTRING)
            continue;
        ary[cnt++] = lstring::copy(a->string);
    }
    ary[cnt] = 0;
    cdf_map = ary;
}


// Fill in the struct fields from the parsed CFD data.
//
void
cOAelecInfo::parse_simulator(const char *simulator, lispnode *p)
{
    if (p && p->type != LN_NODE)
        return;
    cdf_simulator = lstring::copy(simulator);
    if (p) {
        for (lispnode *a = p->args; a; a = a->next) {
            if (a->type != LN_STRING)
                continue;
            if (!strcmp(a->string, "modelName"))
                cdf_name = lstring::copy(a->next->string);
            else if (!strcmp(a->string, "namePrefix"))
                cdf_prefix = lstring::copy(a->next->string);
            else if (!strcmp(a->string, "instParameters"))
                parse_instParameters(a->next);
            else if (!strcmp(a->string, "macroArguments"))
                parse_instParameters(a->next);
            else if (!strcmp(a->string, "termOrder"))
                parse_termOrder(a->next);
            else if (!strcmp(a->string, "propMapping"))
                parse_propMapping(a->next);
        }
    }
    if (!cdf_name) {
        // No modelName was given.  The name can be obtained from the
        // cdf_cell which should have been set earlier.
        // Note:  nothing guarantees that the other fields are filled
        // in, the user should test before use!

        if (cdf_cell) {
            oaString cname;
            cdf_cell->getName(oaNativeNS(), cname);
            cdf_name = lstring::copy(cname);
        }
    }
}


// The parameters list provides info about the device parameters.  We
// will need the default values for the SPICE instance parameters. 
// Other parameters are used in the pcell, and since we've probably
// got a Skill pcell here, they are not of much value.  We will save
// the complete list, then prune it after finding the SPICE
// parameters.
//
void
cOAelecInfo::parse_parameters(lispnode *p)
{

    if (p->type != LN_NODE)
        return;

    int npars = 0;
    sPrmList *prms = 0, *pend = 0;;
    for (lispnode *a = p->args; a; a = a->next) {
        if (a->type != LN_NODE)
            continue;
        const char *name = 0;
        const char *value = 0;
        char buf[64];
        for (lispnode *q = a->args; q; q = q->next) {
            if (q->type != LN_STRING)
                continue;
            if (!strcmp(q->string, "name")) {
                q = q->next;
                if (!q)
                    break;
                if (q->type == LN_STRING || q->type == LN_QSTRING)
                    name = q->string;
            }
            else if (!strcmp(q->string, "defValue")) {
                q = q->next;
                if (!q)
                    break;
                if (q->type == LN_STRING || q->type == LN_QSTRING)
                    value = q->string;
                else if (q->type == LN_NUMERIC) {
                    sprintf(buf, "%g", q->value);
                    value = buf;

                }
            }
            if (name && value)
                break;
        }
        if (name && *name && value) {
            if (!prms)
                prms = pend = new sPrmList(name, value);
            else {
                pend->set_next(new sPrmList(name, value));
                pend = pend->next();
            }
            npars++;
        }
    }
    if (npars) {
        cdf_all_params = new param[npars + 1];
        int cnt = 0;
        while (prms) {
            cdf_all_params[cnt].p_name = prms->ex_name();
            cdf_all_params[cnt].p_value = prms->ex_value();
            sPrmList *px = prms;
            prms = prms->next();
            cnt++;
            delete px;
        }
        cdf_all_params[cnt].p_name = 0;
        cdf_all_params[cnt].p_value = 0;
    }

    if (cdf_all_params) {
        for (param *p = cdf_all_params; p->p_name; p++) {
            const char *val = p->p_value;

            while (lstring::prefix("iPar(", val)) {
                // These show up occasionally, the format is
                //    iPar(\"keyword\")
                // I'm guessing that the keyword can be resolved
                // in the same parameter list, which has been the
                // case.
                const char *t = val + 5;
                while (*t == '\\' || *t == '"')
                    t++;
                char *tval = lstring::copy(t);
                char *e = strrchr(tval, ')');
                if (e) {
                    e--;
                    while (e >= tval && (*e == '\\' || *e == '"'))
                        *e-- = 0;
                }
                const param *pref = find_param(tval);
                delete [] tval;
                if (pref) {
                    val = pref->p_value;
                    continue;
                }
                val = 0;
                break;
            }
            if (val && val != p->p_value) {
                delete [] p->p_value;
                p->p_value = lstring::copy(val);
            }
        }
    }
}


// The propList contains simulator information, including port
// ordering and parameters.
//
void
cOAelecInfo::parse_propList(lispnode *p)
{
    if (p->type != LN_NODE)
        return;
    for (lispnode *a = p->args; a; a = a->next) {
        if (a->type != LN_STRING)
            continue;
        if (!strcmp(a->string, "simInfo"))
            parse_simInfo(a->next);
    }
}


// Look up data for a SPICE-like simulator.  Xic is based on SPICE. 
// The hspice entries are probably always around, other simulators
// follow SPICE conventions, more or less.  If we find data, create a
// CDFinfo struct and place it in the global table.
//
void
cOAelecInfo::parse_simInfo(lispnode *p)
{
    if (p->type != LN_NODE)
        return;

    // Our favored simulator.
    const char *sname = CDvdb()->getVariable(VA_OaDefDevPropView);
    if (!sname || !*sname)
        sname = OA_DEF_DEV_PROP;

    for (lispnode *a = p->args; a; a = a->next) {
        if (a->type != LN_STRING)
            continue;
        if (!strcmp(a->string, sname)) {
            parse_simulator(a->string, a->next);
            return;
        }
    }

    // Common simulators supported by PDKs, in reverse order of
    // favorable matching to WRspice.
    const char *sims[] = {"hspiceD", "hspiceS", "ams", "spectre", 0};

    for (const char **sm = sims; *sm; sm++) {
        if (!strcmp(*sm, sname))
            continue;
        for (lispnode *a = p->args; a; a = a->next) {
            if (a->type != LN_STRING)
                continue;
            if (!strcmp(a->string, *sm)) {
                parse_simulator(a->string, a->next);
                return;
            }
        }
    }

    // Not found.  Add an entry anyway for parameter defaults, and to
    // avoid repeating this process.
    parse_simulator(0, 0);
}
// End of cOAelecInfo functions.

