
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

#include "dsp.h"
#include "dsp_layer.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_lspec.h"
#include "tech.h"
#include "tech_layer.h"
#include "tech_via.h"
#include "tech_extract.h"
#include "errorlog.h"
#include "cd_strmdata.h"
#include "cd_lgen.h"
#include "spnumber/spnumber.h"


// Instantiate kwyword repository.
sExtKW Ekw;

// We handle all of the extraction keywords here, with the exception
// of Device blocks.

// Parser for the per-layer Conductor, Via, etc.  descriptions.  Note
// that a non-null techfp signals inclusion of line number
// information in error messages only.
//
TCret
cTech::ParseExtLayerBlock()
{

    if (Matching(Ekw.Conductor())) {
        // Conductor [ exclude _expression_ ]
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        const char *et = 0;
        if (tc_last_layer->isVia())
            et = Ekw.Via();
        else if (tc_last_layer->isDielectric())
            et = Ekw.Dielectric();
        if (et) {
            const char *kw = tc_kwbuf;
            return (SaveError("%s: attempt to redefine %s layer %s as %s.",
                kw, et, tc_last_layer->name(), kw));
        }
        tc_last_layer->setConductor(true);
        const char *inptr = tc_inbuf;
        char *tok = lstring::gettok(&inptr);
        if (tok) {
            if (lstring::cieq(tok, Ekw.Exclude())) {
                delete [] tok;
                sLspec *exclude = new sLspec;
                const char *t = inptr;
                if (!exclude->parseExpr(&t)) {
                    delete exclude;
                    return (SaveError("%s: parse error after %s,\n%s.",
                        Ekw.Conductor(), Ekw.Exclude(), Errs()->get_error()));
                }
                tech_prm(tc_last_layer)->set_exclude(exclude);
            }
            else {
                char *e = SaveError("%s: unknown keyword %s.",
                    Ekw.Conductor(), tok);
                delete [] tok;
                return (e);
            }
        }
        return (TCmatch);
    }
    if (Matching(Ekw.Routing())) {
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        const char *et = 0;
        if (tc_last_layer->isInContact())
            et = Ekw.Contact();
        else if (tc_last_layer->isVia())
            et = Ekw.Via();
        else if (tc_last_layer->isDielectric())
            et = Ekw.Dielectric();
        if (et) {
            const char *kw = tc_kwbuf;
            return (SaveError(
                "%s: attempt to redefine %s layer %s as %s.",
                kw, et, tc_last_layer->name(), kw));
        }
        tc_last_layer->setConductor(true);
        tc_last_layer->setRouting(true);
        if (!ParseRouting(tc_last_layer, tc_inbuf)) {
            return (SaveError("%s: layer %s, parse error: %s",
                Ekw.Routing(), tc_last_layer->name(),  Errs()->get_error()));
        }
        return (TCmatch);
    }
    if (Matching(Ekw.GroundPlane()) || Matching(Ekw.GroundPlaneDark())) {
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        const char *et = 0;
        if (tc_last_layer->isInContact())
            et = Ekw.Contact();
        else if (tc_last_layer->isVia())
            et = Ekw.Via();
        else if (tc_last_layer->isDielectric())
            et = Ekw.Dielectric();
        if (et) {
            const char *kw = tc_kwbuf;
            return (SaveError("%s: attempt to redefine %s layer %s as %s.",
                kw, et, tc_last_layer->name(), kw));
        }
        if (tc_ext_ground_plane && tc_ext_ground_plane != tc_last_layer) {
            return (SaveError("%s: attempt to set layer %s as ground plane, "
                "already set to %s", Ekw.GroundPlane(), tc_last_layer->name(),
                tc_ext_ground_plane->name()));
        }
        tc_last_layer->setConductor(true);
        tc_last_layer->setGroundPlane(true);
        tc_ext_ground_plane = tc_last_layer;

        const char *inptr = tc_inbuf;
        char *global = lstring::gettok(&inptr);
        if (global) {
            if (lstring::cieq(global, Ekw.Global())) {
                delete [] global;
                CDvdb()->setVariable(VA_GroundPlaneGlobal, 0);
            }
            else {
                char *e = SaveError("%s: layer %s, unknown keyword %s.",
                    Ekw.GroundPlane(), tc_last_layer->name(), global);
                delete [] global;
                return (e);
            }
        }
        return (TCmatch);
    }
    if (Matching(Ekw.GroundPlaneClear()) || Matching(Ekw.TermDefault())) {
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        const char *et = 0;
        if (tc_last_layer->isInContact())
            et = Ekw.Contact();
        else if (tc_last_layer->isVia())
            et = Ekw.Via();
        else if (tc_last_layer->isDielectric())
            et = Ekw.Dielectric();
        if (et) {
            const char *kw = tc_kwbuf;
            return (SaveError("%s: attempt to redefine %s layer %s as %s.",
                kw, et, tc_last_layer->name(), kw));
        }
        if (tc_ext_ground_plane && tc_ext_ground_plane != tc_last_layer) {
            return (SaveError("%s: attempt to set layer %s as ground plane, "
                "already set to %s", Ekw.GroundPlaneClear(),
                tc_last_layer->name(), tc_ext_ground_plane->name()));
        }
        tc_last_layer->setConductor(true);
        tc_last_layer->setGroundPlane(true);
        tc_last_layer->setDarkField(true);
        tc_ext_ground_plane = tc_last_layer;
        const char *inptr = tc_inbuf;
        char *multi = lstring::gettok(&inptr);
        if (multi) {
            if (lstring::cieq(multi, Ekw.MultiNet())) {
                delete [] multi;
                CDvdb()->setVariable(VA_GroundPlaneMulti, 0);
                char *gpmode = lstring::gettok(&inptr);
                if (gpmode &&
                        (*gpmode == '0' || *gpmode == '1' || *gpmode == '2')) {
                    gpmode[1] = 0;
                    CDvdb()->setVariable(VA_GroundPlaneMethod, gpmode);
                    delete [] gpmode;
                }
                else if (gpmode) {
                    char *e = SaveError(
                        "%s: layer %s, unknown value %s after %s.",
                        Ekw.GroundPlaneClear(), tc_last_layer->name(),
                        gpmode, Ekw.MultiNet());
                    delete [] gpmode;
                    return (e);
                }
            }
            else {
                char *e = SaveError(
                    "%s: layer %s, unknown keyword %s after %s.",
                    Ekw.GroundPlaneClear(), tc_last_layer->name(), multi,
                    Ekw.GroundPlaneClear());
                delete [] multi;
                return (e);
            }
        }
        return (TCmatch);
    }
    if (Matching(Ekw.Contact())) {
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        const char *et = 0;
        if (tc_last_layer->isRouting())
            et = Ekw.Routing();
        else if (tc_last_layer->isGroundPlane())
            et = Ekw.GroundPlane();
        else if (tc_last_layer->isDielectric())
            et = Ekw.Dielectric();
        else if (tc_last_layer->isVia())
            et = Ekw.Via();
        if (et) {
            const char *kw = tc_kwbuf;
            return (SaveError("%s: attempt to redefine %s layer %s as %s.",
                kw, et, tc_last_layer->name(), kw));
        }
        const char *inptr = tc_inbuf;
        char *vl1 = lstring::gettok(&inptr);
        if (vl1) {
            char *t = strmdata::dec_to_hex(vl1);  // handle "layer,datatype"
            if (t) {
                delete [] vl1;
                vl1 = t;
            }
            if (strlen(vl1) > 4)
                vl1[4] = '\0';
        }
        else {
            return (SaveError("%s: layer %s, parse error.",
                Ekw.Contact(), tc_last_layer->name()));
        }
        while (isspace(*inptr))
            inptr++;
        ParseNode *tree = 0;
        if (*inptr) {
            const char *t = inptr;
            tree = SIparse()->getLexprTree(&t);
            if (!tree) {
                char *er = SIparse()->errMessage();
                delete [] vl1;
                if (er) {
                    char *e = SaveError(
                        "%s: layer experession parse failed:\n%s",
                        Ekw.Contact(), er);
                    delete [] er;
                    return (e);
                }
                return (SaveError("%s: layer %s, parse error.",
                    Ekw.Contact(), tc_last_layer->name()));
            }
        }
        AddContact(tc_last_layer, vl1, tree);
        delete [] vl1;
        return (TCmatch);
    }
    if (Matching(Ekw.Via())) {
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        const char *et = 0;
        if (tc_last_layer->isConductor())
            et = Ekw.Conductor();
        else if (tc_last_layer->isRouting())
            et = Ekw.Routing();
        else if (tc_last_layer->isGroundPlane())
            et = "Ground Plane";
        else if (tc_last_layer->isInContact())
            et = Ekw.Contact();
        else if (tc_last_layer->isDielectric())
            et = Ekw.Dielectric();
        if (et) {
            const char *kw = tc_kwbuf;
            return (SaveError("%s: attempt to redefine %s layer %s as %s.",
                kw, et, tc_last_layer->name(), kw));
        }
        const char *inptr = tc_inbuf;
        char *vl1 = lstring::gettok(&inptr);
        if (!vl1) {
            return (SaveError("%s: layer %s, first layer missing.",
                Ekw.Via(), tc_last_layer->name()));
        }
        char *vl2 = lstring::gettok(&inptr);
        if (!vl2) {
            delete [] vl1;
            return (SaveError("%s: layer %s, second layer missing.",
                Ekw.Via(), tc_last_layer->name()));
        }
        while (isspace(*inptr))
            inptr++;
        ParseNode *tree = 0;
        if (*inptr) {
            const char *t = inptr;
            tree = SIparse()->getLexprTree(&t);
            if (!tree) {
                char *er = SIparse()->errMessage();
                delete [] vl1;
                delete [] vl2;

                if (er) {
                    char *e = SaveError(
                        "%s: layer experession parse failed:\n%s",
                        Ekw.Via(), er);
                    delete [] er;
                    return (e);
                }
                return (SaveError("%s: layer %s, parse error.",
                    Ekw.Via(), tc_last_layer->name()));
            }
        }
        AddVia(tc_last_layer, vl1, vl2, tree);
        delete [] vl1;
        delete [] vl2;
        return (TCmatch);
    }
    if (Matching(Ekw.ViaCut())) {
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        const char *inptr = tc_inbuf;
        while (isspace(*inptr))
            inptr++;
        ParseNode *tree = 0;
        if (*inptr) {
            const char *t = inptr;
            tree = SIparse()->getLexprTree(&t);
            if (!tree) {
                char *er = SIparse()->errMessage();
                if (er) {
                    char *e = SaveError(
                        "%s: layer experession parse failed:\n%s",
                        Ekw.Via(), er);
                    delete [] er;
                    return (e);
                }
                return (SaveError("%s: layer %s, parse error.",
                    Ekw.Via(), tc_last_layer->name()));
            }
        }
        if (!tree) {
            return (SaveError(
                "%s: layer %s, missing or unknown cut expression.",
                Ekw.Via(), tc_last_layer->name()));
        }
        AddVia(tc_last_layer, 0, 0, tree);
        return (TCmatch);
    }
    if (Matching(Ekw.Dielectric())) {
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        const char *et = 0;
        if (tc_last_layer->isConductor())
            et = Ekw.Conductor();
        else if (tc_last_layer->isRouting())
            et = Ekw.Routing();
        else if (tc_last_layer->isGroundPlane())
            et = "Ground Plane";
        else if (tc_last_layer->isInContact())
            et = Ekw.Contact();
        else if (tc_last_layer->isVia())
            et = Ekw.Via();
        if (et) {
            const char *kw = tc_kwbuf;
            return (SaveError("%s: attempt to redefine %s layer %s as %s.",
                kw, et, tc_last_layer->name(), kw));
        }
        if (tc_last_layer->isDielectric()) {
            return (SaveError(
                "%s: layer %s, ignoring multiple specification.",
                Ekw.Dielectric(), tc_last_layer->name()));
        }
        tc_last_layer->setDielectric(true);
        return (TCmatch);
    }
    if (Matching(Ekw.DarkField())) {
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        tc_last_layer->setDarkField(true);
        return (TCmatch);
    }
    if (Matching(Ekw.Planarize())) {
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        bool plz = GetBoolean(tc_inbuf);
        tc_last_layer->setPlanarizing(true, plz);
        return (TCmatch);
    }

    //
    // Electrical parameters
    //

    if (Matching(Ekw.Thickness())) {
        // film thickness, in microns
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        double d;
        if (sscanf(tc_inbuf, "%lf", &d) == 1 && d >= 0.0) {
            dsp_prm(tc_last_layer)->set_thickness(d);
            return (TCmatch);
        }
        return (SaveError("%s: layer %s, bad specification.",
            Ekw.Thickness(), tc_last_layer->name()));
    }
    if (Matching(Ekw.Rho())) {
        // resistivity, ohm-meter
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        double d;
        if (sscanf(tc_inbuf, "%lf", &d) == 1 && d >= 0.0) {
            tech_prm(tc_last_layer)->set_rho(d);
            return (TCmatch);
        }
        return (SaveError("%s: layer %s, bad specification.",
            Ekw.Rho(), tc_last_layer->name()));
    }
    if (Matching(Ekw.Sigma())) {
        // conductivity, 1/(ohm-meter)
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        double d;
        if (sscanf(tc_inbuf, "%lf", &d) == 1 && d > 0.0) {
            tech_prm(tc_last_layer)->set_rho(1.0/d);
            return (TCmatch);
        }
        return (SaveError("%s: layer %s, bad specification.",
            Ekw.Sigma(), tc_last_layer->name()));
    }
    if (Matching(Ekw.Rsh())) {
        // Set the electrical resistance in ohms per square of the
        // layer being defined, for resistance calculation.
        //
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        double d;
        if (sscanf(tc_inbuf, "%lf", &d) == 1 && d >= 0.0) {
            tech_prm(tc_last_layer)->set_ohms_per_sq(d);
            return (TCmatch);
        }
        return (SaveError("%s: layer %s, bad specification.",
            Ekw.Rsh(), tc_last_layer->name()));
    }
    if (Matching(Ekw.Tau())) {
        // Drude relaxation time, sec.
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        double d;
        if (sscanf(tc_inbuf, "%lf", &d) == 1 && d > 0.0) {
            tech_prm(tc_last_layer)->set_tau(d);
            return (TCmatch);
        }
        return (SaveError("%s: layer %s, bad specification.",
            Ekw.Tau(), tc_last_layer->name()));
    }
    if (Matching(Ekw.FH_nhinc())) {
        // The FastHenry nhinc number for the layer.  The FastHenry
        // interface uses this to split conductors in a direction
        // parallel to the substrate to account for penetration/skin
        // depth.
        //
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        int n;
        if (sscanf(tc_inbuf, "%d", &n) == 1 && n > 0) {
            tech_prm(tc_last_layer)->set_fh_nhinc(n);
            return (TCmatch);
        }
        return (SaveError("%s: layer %s, bad value, must be positive integer.",
            Ekw.FH_nhinc(), tc_last_layer->name()));
    }
    if (Matching(Ekw.FH_rh())) {
        // When using nhinc, this is the thickness ratio between
        // adjacent filaments, default is 2.0.
        //
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        double d;
        if (sscanf(tc_inbuf, "%lf", &d) == 1 && d > 0.0) {
            tech_prm(tc_last_layer)->set_fh_rh(d);
            return (TCmatch);
        }
        return (SaveError("%s: layer %s, bad value, must be positive.",
            Ekw.FH_rh(), tc_last_layer->name()));
    }
    if (Matching(Ekw.EpsRel())) {
        // relative dielectric constant
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        double d;
        if (sscanf(tc_inbuf, "%lf", &d) == 1 && d >= 1.0) {
            tech_prm(tc_last_layer)->set_epsrel(d);
            return (TCmatch);
        }
        return (SaveError("%s: layer %s, bad specification.",
            Ekw.EpsRel(), tc_last_layer->name()));
    }
    if (Matching(Ekw.Cap()) || Matching(Ekw.Capacitance())) {
        // Set the electrical capacitance per unit area of the layer
        // being defined, for capacitance calculation.
        //
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        double d1, d2;
        int i = sscanf(tc_inbuf, "%lf %lf", &d1, &d2);
        if ((i == 2 && d1 >= 0.0 && d2 >= 0.0) || (i == 1 && d1 >= 0.0)) {
            tech_prm(tc_last_layer)->set_cap_per_area(d1);
            if (i == 2)
                tech_prm(tc_last_layer)->set_cap_per_perim(d2);
            return (TCmatch);
        }
        return (SaveError("%s: layer %s, bad specification.",
            Ekw.Capacitance(), tc_last_layer->name()));
    }
    if (Matching(Ekw.Lambda())) {
        // superconducting penetration depth, microns
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        double d;
        if (sscanf(tc_inbuf, "%lf", &d) == 1 && d >= 0.0) {
            tech_prm(tc_last_layer)->set_lambda(d);
            return (TCmatch);
        }
        return (SaveError("%s: layer %s, bad specification.",
            Ekw.Lambda(), tc_last_layer->name()));
    }
    if (Matching(Ekw.Tline())) {
        // Set the transmission line parameters of the layer being
        // defined, for calculation of transmission line impedance and
        // other parameters.  The floating point values are optional,
        // they will be obtained from the corresponding via layer if
        // not given or given as 0.
        //
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        const char *inptr = tc_inbuf;
        char *lname = lstring::gettok(&inptr);
        if (lname) {
            delete [] tech_prm(tc_last_layer)->gp_lname();
            tech_prm(tc_last_layer)->set_gp_lname(lname);

            double d1, d2;
            int n = sscanf(inptr, "%lf %lf", &d1, &d2);
            if (n >= 1) {
                if (d1 > 0.0 && d1 < 10.0)
                    tech_prm(tc_last_layer)->set_diel_thick(d1);
                else if (d1 != 0.0) {
                    return (SaveError(
                        "%s: layer %s, bad dielectric thickness.",
                        Ekw.Tline(), tc_last_layer->name()));
                }
                if (n == 2) {
                    if (d2 >= 1.0 && d2 < 100.0)
                        tech_prm(tc_last_layer)->set_diel_const(d2);
                    else if (d1 != 0.0) {
                        return (SaveError(
                            "%s: layer %s, bad dielectric constant.",
                            Ekw.Tline(), tc_last_layer->name()));
                    }
                }
            }
        }
        else {
            return (SaveError("%s: layer %s, bad specification.",
                Ekw.Tline(), tc_last_layer->name()));
        }
        return (TCmatch);
    }
    if (Matching(Ekw.Antenna())) {
        // Antenna ratio (e.g., for MOS gates)
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        float d;
        if (sscanf(tc_inbuf, "%f", &d) == 1 && d >= 0.0) {
            tech_prm(tc_last_layer)->set_ant_ratio(d);
            return (TCmatch);
        }
        return (SaveError("%s: layer %s, bad specification.",
            Ekw.Antenna(), tc_last_layer->name()));
    }

    return (TCnone);
}


// This prints the layer-specific extraction parameters to fp or lstr.
// If cmts, print associated comments.
//
void
cTech::PrintExtLayerBlock(FILE *fp, sLstr *lstr, bool cmts, const CDl *ld)
{
    if (ld->isVia()) {
        for (sVia *v = tech_prm(ld)->via_list(); v; v = v->next()) {
            if (v->layer1() && v->layer2()) {
                PutStr(fp, lstr, Ekw.Via());
                PutChr(fp, lstr, ' ');
                PutStr(fp, lstr, v->layername1());
                PutChr(fp, lstr, ' ');
                PutStr(fp, lstr, v->layername2());
                if (v->tree()) {
                    sLstr tstr;
                    v->tree()->string(tstr);
                    if (tstr.string()) {
                        PutChr(fp, lstr, ' ');
                        PutStr(fp, lstr, tstr.string());
                    }
                }
                PutChr(fp, lstr, '\n');
            }
            if (cmts)
                CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.Via());
        }
    }
    else if (ld->isViaCut()) {
        sVia *v = tech_prm(ld)->via_list();
        if (v && v->tree()) {
            sLstr tstr;
            v->tree()->string(tstr);
            if (tstr.string()) {
                PutStr(fp, lstr, Ekw.ViaCut());
                PutChr(fp, lstr, ' ');
                PutStr(fp, lstr, tstr.string());
                PutChr(fp, lstr, '\n');
            }
        }
        if (cmts)
            CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.ViaCut());
    }
    else if (ld->isDielectric()) {
        PutStr(fp, lstr, Ekw.Dielectric());
        PutChr(fp, lstr, '\n');
        if (cmts)
            CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.Dielectric());
    }
    else if (ld->isGroundPlane()) {
        if (ld->isDarkField()) {
            if (tc_ext_invert_ground_plane) {
                PutStr(fp, lstr, Ekw.GroundPlaneClear());
                PutChr(fp, lstr, ' ');
                PutStr(fp, lstr, Ekw.MultiNet());
                if (tc_ext_gp_mode == GPI_TOP) {
                    PutChr(fp, lstr, ' ');
                    PutStr(fp, lstr, "1");
                }
                else if (tc_ext_gp_mode == GPI_ALL) {
                    PutChr(fp, lstr, ' ');
                    PutStr(fp, lstr, "2");
                }
            }
            else
                PutStr(fp, lstr, Ekw.GroundPlaneClear());
            PutChr(fp, lstr, '\n');
            if (cmts) {
                CommentDump(fp, lstr, tBlkPlyr, ld->name(),
                    Ekw.GroundPlaneClear());
                CommentDump(fp, lstr, tBlkPlyr, ld->name(),
                    Ekw.TermDefault());
                CommentDump(fp, lstr, tBlkPlyr, ld->name(),
                    Ekw.DarkField());
            }
        }
        else {
            if (tc_ext_ground_plane_global) {
                PutStr(fp, lstr, Ekw.GroundPlane());
                PutChr(fp, lstr, ' ');
                PutStr(fp, lstr, Ekw.Global());
            }
            else
                PutStr(fp, lstr, Ekw.GroundPlane());
            PutChr(fp, lstr, '\n');
            if (cmts) {
                CommentDump(fp, lstr, tBlkPlyr, ld->name(),
                    Ekw.GroundPlane());
                CommentDump(fp, lstr, tBlkPlyr, ld->name(),
                    Ekw.GroundPlaneDark());
            }
        }
    }
    else if (ld->isRouting()) {
        if (tech_prm(ld)->exclude()) {
            PutStr(fp, lstr, Ekw.Conductor());
            char *s = tech_prm(ld)->exclude()->string();
            if (s) {
                PutChr(fp, lstr, ' ');
                PutStr(fp, lstr, Ekw.Exclude());
                PutChr(fp, lstr, ' ');
                PutStr(fp, lstr, s);
                delete [] s;
            }
            PutChr(fp, lstr, '\n');
            if (cmts)
                CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.Conductor());
        }
        WriteRouting(ld, fp, lstr);
        PutChr(fp, lstr, '\n');
        if (cmts)
            CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.Routing());
    }
    else if (ld->isInContact()) {
        for (sVia *v = tech_prm(ld)->via_list(); v; v = v->next()) {
            if (v->layer1()) {
                PutStr(fp, lstr, Ekw.Contact());
                PutChr(fp, lstr, ' ');
                PutStr(fp, lstr, v->layername1());
                if (v->tree()) {
                    sLstr tstr;
                    v->tree()->string(tstr);
                    if (tstr.string()) {
                        PutChr(fp, lstr, ' ');
                        PutStr(fp, lstr, tstr.string());
                    }
                }
                PutChr(fp, lstr, '\n');
            }
            if (cmts)
                CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.Contact());
        }
    }
    else if (ld->isConductor()) {
        PutStr(fp, lstr, Ekw.Conductor());
        if (tech_prm(ld)->exclude()) {
            char *s = tech_prm(ld)->exclude()->string();
            if (s) {
                PutChr(fp, lstr, ' ');
                PutStr(fp, lstr, Ekw.Exclude());
                PutChr(fp, lstr, ' ');
                PutStr(fp, lstr, s);
                delete [] s;
            }
        }
        PutChr(fp, lstr, '\n');
        if (cmts)
            CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.Conductor());
    }
    if (ld->isDarkField() && !ld->isGroundPlane() && !ld->isVia()) {
        PutStr(fp, lstr, Ekw.DarkField());
        PutChr(fp, lstr, '\n');
        if (cmts)
            CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.DarkField());
    }
    if (ld->isPlanarizingSet()) {
        PutStr(fp, lstr, Ekw.Planarize());
        PutChr(fp, lstr, ' ');
        PutStr(fp, lstr, ld->isPlanarizing() ? "yes" : "no");
        PutChr(fp, lstr, '\n');
        if (cmts)
            CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.Planarize());
    }

    //
    // Electrical parameters
    //
    TechLayerParams *lp = tech_prm(ld);
    DspLayerParams *dp = dsp_prm(ld);

    char buf[256];

    if (dp->thickness() > 0.0) {
        snprintf(buf, sizeof(buf), "%s %.4f\n", Ekw.Thickness(),
            dp->thickness());
        PutStr(fp, lstr, buf);
    }
    if (cmts)
        CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.Thickness());

    if (lp->fh_nhinc() > 1) {
        snprintf(buf, sizeof(buf), "%s %d\n", Ekw.FH_nhinc(), lp->fh_nhinc());
        PutStr(fp, lstr, buf);
    }
    if (cmts)
        CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.FH_nhinc());

    if (lp->fh_rh() > 0.0 && lp->fh_rh() != DEF_FH_RH) {
        snprintf(buf, sizeof(buf), "%s %g\n", Ekw.FH_rh(), lp->fh_rh());
        PutStr(fp, lstr, buf);
    }
    if (cmts)
        CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.FH_rh());

    if (lp->rho() > 0.0) {
        snprintf(buf, sizeof(buf), "%s %g\n", Ekw.Rho(), lp->rho());
        PutStr(fp, lstr, buf);
    }
    if (cmts) {
        CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.Rho());
        CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.Sigma());
    }

    if (lp->tau() > 0.0) {
        snprintf(buf, sizeof(buf), "%s %g\n", Ekw.Tau(), lp->tau());
        PutStr(fp, lstr, buf);
    }
    if (cmts)
        CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.Tau());

    if (lp->ohms_per_sq() > 0.0) {
        snprintf(buf, sizeof(buf), "%s %g\n", Ekw.Rsh(), lp->ohms_per_sq());
        PutStr(fp, lstr, buf);
    }
    if (cmts)
        CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.Rsh());

    if (lp->epsrel() > 1.0) {
        snprintf(buf, sizeof(buf), "%s %g\n", Ekw.EpsRel(), lp->epsrel());
        PutStr(fp, lstr, buf);
    }
    if (cmts)
        CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.EpsRel());

    if (lp->cap_per_area() > 0.0 || lp->cap_per_perim() > 0.0) {
        snprintf(buf, sizeof(buf), "%s %g %g\n", Ekw.Capacitance(),
            lp->cap_per_area(), lp->cap_per_perim());
        PutStr(fp, lstr, buf);
    }
    if (cmts)
        CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.Capacitance());

    if (lp->lambda() > 0.0) {
        snprintf(buf, sizeof(buf), "%s %g\n", Ekw.Lambda(), lp->lambda());
        PutStr(fp, lstr, buf);
    }
    if (cmts)
        CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.Lambda());

    if (lp->gp_lname() && *lp->gp_lname()) {
        snprintf(buf, sizeof(buf), "%s %s", Ekw.Tline(), lp->gp_lname());
        if (lp->diel_thick() > 0.0 || lp->diel_const() > 0.0) {
            int len = strlen(buf);
            snprintf(buf + len, sizeof(buf) - len, " %g", lp->diel_thick());
        }
        if (lp->diel_const() > 0.0) {
            int len = strlen(buf);
            snprintf(buf + len, sizeof(buf) - len, " %g", lp->diel_const());
        }
        strcat(buf, "\n");
        PutStr(fp, lstr, buf);
    }
    if (cmts)
        CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.Tline());

    if (lp->ant_ratio() > 0.0) {
        snprintf(buf, sizeof(buf), "%s %g\n", Ekw.Antenna(), lp->ant_ratio());
        PutStr(fp, lstr, buf);
    }
    if (cmts)
        CommentDump(fp, lstr, tBlkPlyr, ld->name(), Ekw.Antenna());
}


// Static function.
// Add a via description to a layer, overwrites an existing similar
// specification.
//
sVia *
cTech::AddVia(CDl *ld, const char *ln1, const char *ln2, ParseNode *tree)
{
    if (!ld)
        return (0);

    if (!ln1 && !ln2 && tree) {
        // ViaCut layer.  This is a dielectric that takes its pattern
        // from a layer expression.

        sVia *vl0 = tech_prm(ld)->via_list();
        sVia::destroy(vl0);
        vl0 = new sVia(0, 0, tree);
        tech_prm(ld)->set_via_list(vl0);
        ld->setViaCut(true);
        ld->setDarkField(true);
        return (vl0);
    }

    if (!ln1 || !*ln1)
        return (0);
    if (!ln2 || !*ln2)
        return (0);

    sVia *vl0 = tech_prm(ld)->via_list();
    if (vl0) {
        // Remove any bad or matching vias.
        sVia *vp = 0, *vn;
        for (sVia *v = vl0; v; v = vn) {
            vn = v->next();
            if (!v->layername1() || !v->layername2() ||
                    (!strcmp(ln1, v->layername1()) &&
                        !strcmp(ln2, v->layername2())) ||
                    (!strcmp(ln1, v->layername2()) &&
                        !strcmp(ln2, v->layername1()))) {
                if (!vp)
                    vl0 = vn;
                else
                    vp->setNext(vn);
                delete v;
                continue;
            }
            vp = v;
        }
    }

    sVia *via = new sVia(lstring::copy(ln1), lstring::copy(ln2), tree);
    if (!vl0)
        vl0 = via;
    else {
        sVia *vl = vl0;
        while (vl->next())
            vl = vl->next();
        vl->setNext(via);
    }
    tech_prm(ld)->set_via_list(vl0);
    ld->setVia(true);
    ld->setDarkField(true);
    return (via);
}


// Static function.
// Add a contact description to a layer, overwrites an existing similar
// specification.
//
bool
cTech::AddContact(CDl *ld, const char *ln1, ParseNode *tree)
{
    if (!ld)
        return (false);
    if (!ln1 || !*ln1)
        return (false);

    sVia *vl0 = tech_prm(ld)->via_list();
    if (vl0) {
        // Remove any bad or matching descs.
        sVia *vp = 0, *vn;
        for (sVia *v = vl0; v; v = vn) {
            vn = v->next();
            if (!v->layername1() || !strcmp(ln1, v->layername1())) {
                if (!vp)
                    vl0 = vn;
                else
                    vp->setNext(vn);
                delete v;
                continue;
            }
            vp = v;
        }
    }

    sVia *via = new sVia(lstring::copy(ln1), 0, tree);
    if (!vl0)
        vl0 = via;
    else {
        sVia *vl = vl0;
        while (vl->next())
            vl = vl->next();
        vl->setNext(via);
    }
    tech_prm(ld)->set_via_list(vl0);
    ld->setConductor(true);
    ld->setInContact(true);
    return (true);
}


// Static function.
// Called when all extraction keywords have been parsed, returns a
// string containing compatibility warnings generated by clashing
// keywords.  These are generally not a problem as they will likely be
// ignored.
//
char *
cTech::ExtCheckLayerKeywords(CDl *ld)
{
    sLstr lstr;
    char buf[256], tbuf[32];
    const char *msg = "Layer %s: %s defined but not applicable to %s layer.\n";

    if (!ld)
        return (0);

    TechLayerParams *lp = tech_prm(ld);
    if (ld->isConductor()) {
        if (ld->isVia()) {
            // Via and Conductor
            snprintf(buf, sizeof(buf),
                "ERROR on layer %s: both %s and %s defined.\n",
                ld->name(), Ekw.Via(), Ekw.Conductor());
            lstr.add(buf);
            return (lstr.string_trim());
        }
        if (ld->isDielectric()) {
            // Dielectric and Conductor
            snprintf(buf, sizeof(buf),
                "ERROR on layer %s: both %s and %s defined.\n",
                ld->name(), Ekw.Dielectric(), Ekw.Conductor());
            lstr.add(buf);
            return (lstr.string_trim());
        }
        if (lp->epsrel() > 0.0) {
            // EpsRel on Conductor
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.EpsRel(),
                Ekw.Conductor());
            lstr.add(buf);
        }
        if ((lp->cap_per_area() > 0.0 || lp->cap_per_perim() > 0.0) &&
                ld->isGroundPlane()) {
            // Capacitance on GroundPlane
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Capacitance(),
                Ekw.GroundPlane());
            lstr.add(buf);
        }
        if (lp->gp_lname() != 0 && ld->isGroundPlane()) {
            // Tline on GroundPlane
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Tline(),
                Ekw.GroundPlane());
            lstr.add(buf);
        }
    }
    else if (ld->isVia()) {
        if (lp->rho() > 0.0) {
            // Rho/Sigma on Via
            snprintf(tbuf, sizeof(tbuf), "%s or %s", Ekw.Rho(), Ekw.Sigma());
            snprintf(buf, sizeof(buf), msg, ld->name(), tbuf, Ekw.Via());
            lstr.add(buf);
        }
        if (lp->tau() > 0.0) {
            // Tau on Via
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Tau(), Ekw.Via());
            lstr.add(buf);
        }
        if (lp->ohms_per_sq() > 0.0) {
            // Rsh on Via
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Rsh(), Ekw.Via());
            lstr.add(buf);
        }
        if (lp->cap_per_area() > 0.0 || lp->cap_per_perim() > 0.0) {
            // Capacitance on Via
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Capacitance(),
                Ekw.Via());
            lstr.add(buf);
        }
        if (lp->lambda() > 0.0) {
            // Lambda on Via
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Lambda(),
                Ekw.Via());
            lstr.add(buf);
        }
        if (lp->gp_lname() != 0) {
            // Tline on Via
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Tline(),
                Ekw.Via());
            lstr.add(buf);
        }
        if (lp->ant_ratio() > 0.0) {
            // Antenna on Via
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Antenna(),
                Ekw.Via());
            lstr.add(buf);
        }
    }
    else if (ld->isViaCut()) {
        if (lp->rho() > 0.0) {
            // Rho/Sigma on ViaCut
            snprintf(tbuf, sizeof(tbuf), "%s or %s", Ekw.Rho(), Ekw.Sigma());
            snprintf(buf, sizeof(buf), msg, ld->name(), tbuf, Ekw.Via());
            lstr.add(buf);
        }
        if (lp->tau() > 0.0) {
            // Tau on ViaCut
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Tau(), Ekw.Via());
            lstr.add(buf);
        }
        if (lp->ohms_per_sq() > 0.0) {
            // Rsh on ViaCut
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Rsh(), Ekw.Via());
            lstr.add(buf);
        }
        if (lp->cap_per_area() > 0.0 || lp->cap_per_perim() > 0.0) {
            // Capacitance on ViaCut
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Capacitance(),
                Ekw.Via());
            lstr.add(buf);
        }
        if (lp->lambda() > 0.0) {
            // Lambda on ViaCut
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Lambda(),
                Ekw.Via());
            lstr.add(buf);
        }
        if (lp->gp_lname() != 0) {
            // Tline on ViaCut
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Tline(),
                Ekw.Via());
            lstr.add(buf);
        }
        if (lp->ant_ratio() > 0.0) {
            // Antenna on ViaCut
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Antenna(),
                Ekw.Via());
            lstr.add(buf);
        }
    }
    else if (ld->isDielectric()) {
        if (lp->rho() > 0.0) {
            // Rho/Sigma on Dielectric
            snprintf(tbuf, sizeof(tbuf), "%s or %s", Ekw.Rho(), Ekw.Sigma());
            snprintf(buf, sizeof(buf), msg, ld->name(), tbuf,
                Ekw.Dielectric());
            lstr.add(buf);
        }
        if (lp->tau() > 0.0) {
            // Tau on Via
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Tau(), Ekw.Via());
            lstr.add(buf);
        }
        if (lp->ohms_per_sq() > 0.0) {
            // Rsh on Dielectric
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Rsh(),
                Ekw.Dielectric());
            lstr.add(buf);
        }
        if (lp->cap_per_area() > 0.0 || lp->cap_per_perim() > 0.0) {
            // Capacitance on Dielectric
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Capacitance(),
                Ekw.Dielectric());
            lstr.add(buf);
        }
        if (lp->lambda() > 0.0) {
            // Lambda on Dielectric
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Lambda(),
                Ekw.Dielectric());
            lstr.add(buf);
        }
        if (lp->gp_lname() != 0) {
            // Tline on Dielectric
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Tline(),
                Ekw.Dielectric());
            lstr.add(buf);
        }
        if (lp->ant_ratio() > 0.0) {
            // Antenna on Dielectric
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Antenna(),
                Ekw.Dielectric());
            lstr.add(buf);
        }
    }
    else if (lp->epsrel() > 0.0) {
        const char *msg1 =
            "Layer %s: %s inappropriate on layer with %s defined.\n";
        if (lp->rho() > 0.0) {
            // Rho/Sigma with EpsRel
            snprintf(tbuf, sizeof(tbuf), "%s or %s", Ekw.Rho(), Ekw.Sigma());
            snprintf(buf, sizeof(buf), msg1, ld->name(), tbuf, Ekw.EpsRel());
            lstr.add(buf);
        }
        if (lp->tau() > 0.0) {
            // Tau on Via
            snprintf(buf, sizeof(buf), msg, ld->name(), Ekw.Tau(), Ekw.Via());
            lstr.add(buf);
        }
        if (lp->ohms_per_sq() > 0.0) {
            // Rsh with EpsRel
            snprintf(buf, sizeof(buf), msg1, ld->name(), Ekw.Rsh(),
                Ekw.EpsRel());
            lstr.add(buf);
        }
        if (lp->cap_per_area() > 0.0 || lp->cap_per_perim() > 0.0) {
            // Capacitance with EpsRel
            snprintf(buf, sizeof(buf), msg1, ld->name(), Ekw.Capacitance(),
                Ekw.EpsRel());
            lstr.add(buf);
        }
        if (lp->lambda() > 0.0) {
            // Lambda with EpsRel
            snprintf(buf, sizeof(buf), msg1, ld->name(), Ekw.Lambda(),
                Ekw.EpsRel());
            lstr.add(buf);
        }
        if (lp->gp_lname() != 0) {
            // Tline with EpsRel
            snprintf(buf, sizeof(buf), msg1, ld->name(), Ekw.Tline(),
                Ekw.EpsRel());
            lstr.add(buf);
        }
        if (lp->ant_ratio() > 0.0) {
            // Antenna with EpsRel
            snprintf(buf, sizeof(buf), msg1, ld->name(), Ekw.Antenna(),
                Ekw.EpsRel());
            lstr.add(buf);
        }
    }
    return (lstr.string_trim());
}


// Static function
// Return the sheet resistance (ohms per square) of the layer passed,
// or 0.0 if unavailable.
//
double
cTech::GetLayerRsh(const CDl *ld)
{
    double rsh = tech_prm(ld)->ohms_per_sq();
    if (rsh > 0.0)
        return (rsh);
    double rho = tech_prm(ld)->rho();  // ohm-meter
    double thk = dsp_prm(ld)->thickness();  // microns
    if (rho > 0.0 && thk > 0.0)
        return (1e6*rho/thk);
    return (0.0);
}


// Static function
// If microstrip parameters are available for the passed layer, return
// true and fill the parameters into the array.  The order is:
//   params[0] = linethick();
//   params[1] = linepenet();
//   params[2] = gndthick();
//   params[3] = gndpenet();
//   params[4] = dielthick();
//   params[5] = dielconst();
//
bool
cTech::GetLayerTline(const CDl *ld, double *params)
{
    const char *lname = tech_prm(ld)->gp_lname();
    if (!lname || !*lname)
        return (false);
    params[0] = dsp_prm(ld)->thickness();
    if (params[0] <= 0.0)
        return (false);
    params[1] = tech_prm(ld)->lambda();
    if (params[1] < 0.0)
        return (false);

    CDl *ldgp = CDldb()->findLayer(lname, Physical);
    if (!ldgp)
        return (false);
    params[2] = dsp_prm(ldgp)->thickness();
    if (params[2] <= 0)
        return (false);
    params[3] = tech_prm(ldgp)->lambda();
    if (params[3] < 0.0)
        return (false);

    // If the dielectric params are 0, try to obtain info by searching
    // for a corresponding via layer.  A valid number saved in the
    // metal layer overrides the via layer.

    double dthick = tech_prm(ld)->diel_thick();
    double dconst = tech_prm(ld)->diel_const();

    if (dthick <= 0.0 || dconst <= 0.0) {
        bool found = false;
        CDl *ldv;
        CDextLgen gen(CDL_VIA);
        while ((ldv = gen.next()) != 0) {
            for (sVia *v = tech_prm(ldv)->via_list(); v; v = v->next()) {
                if (v->layer1() && v->layer2()) {
                    CDl *ld1 = v->layer1();
                    CDl *ld2 = v->layer2();
                    if ((ld1 == ld && ld2 == ldgp) ||
                            (ld2 == ld && ld1 == ldgp)) {
                        if (dthick <= 0.0)
                            dthick = tech_prm(ldv)->diel_thick();
                        if (dconst <= 0.0)
                            dconst = tech_prm(ldv)->diel_const();
                        found = true;
                        break;
                    }
                }
            }
            if (found)
                break;
        }
    }
    if (dthick <= 0.0)
        return (false);
    if (dconst <= 1.0)
        return (false);
    params[4] = dthick;
    params[5] = dconst;
    return (true);
}


// Parse the Routing spec.  The Routing keyword has alread been
// recognized and stripped.
//
// Routing dir=H|V|X|Y pitch=px,py offset=ox,oy width=w maxdist=d
//
bool
cTech::ParseRouting(const CDl *ld, const char *line)
{
    if (!ld) {
        Errs()->add_error("no active layer");
        return (false);
    }
    TechLayerParams *tp = tech_prm(ld);
    if (!tp)
        return (true);
    const char *s = line;
    char *tok;
    while ((tok = lstring::gettok(&s, "=")) != 0) {
        if (lstring::ciprefix("dir", tok)) {
            delete [] tok;
            tok = lstring::gettok(&s);
            if (!tok) {
                Errs()->add_error("missing routing direction");
                return (false);
            }
            if (strchr("hHxH", *tok))
                tp->set_route_dir(tDirHoriz);
            else if (strchr("vVyY", *tok))
                tp->set_route_dir(tDirVert);
            else {
                Errs()->add_error("unknown routing direction");
                return (false);
            }
        }
        else if (lstring::ciprefix("p", tok)) {
            double *d = SPnum.parse(&s, false);
            if (!d) {
                Errs()->add_error("missing pitch");
                return (false);
            }
            if (*d > 0.0)
                tp->set_route_h_pitch(INTERNAL_UNITS(*d));
            if (*s == ',') {
                s++;
                d = SPnum.parse(&s, false);
                if (d && *d > 0.0)
                    tp->set_route_v_pitch(INTERNAL_UNITS(*d));
            }
        }
        else if (lstring::ciprefix("o", tok)) {
            double *d = SPnum.parse(&s, false);
            if (!d) {
                Errs()->add_error("missing offset");
                return (false);
            }
            if (*d > 0.0)
                tp->set_route_h_offset(INTERNAL_UNITS(*d));
            if (*s == ',') {
                s++;
                d = SPnum.parse(&s, false);
                if (d && *d > 0.0)
                    tp->set_route_v_offset(INTERNAL_UNITS(*d));
            }
        }
        else if (lstring::ciprefix("w", tok)) {
            double *d = SPnum.parse(&s, false);
            if (!d) {
                Errs()->add_error("missing width");
                return (false);
            }
            if (*d > 0.0)
                tp->set_route_width(INTERNAL_UNITS(*d));
        }
        else if (lstring::ciprefix("maxd", tok)) {
            double *d = SPnum.parse(&s, false);
            if (!d) {
                Errs()->add_error("missing maxdist");
                return (false);
            }
            if (*d > 0.0)
                tp->set_route_max_dist(INTERNAL_UNITS(*d));
        }
        else {
            Errs()->add_error("unknown token");
            return (false);
        }
    }
    return (true);
}


// Write a Routing spec, sarting with the Routing keyword.  If the
// layer is not a routing lyer, return null.
//
// Routing dir=H|V|X|Y pitch=px,py offset=ox,oy width=w maxdist=d
//
void
cTech::WriteRouting(const CDl *ld, FILE *fp, sLstr *lstr)
{
    char buf[256];
    if (ld->flags() & CDL_ROUTING) {
        PutStr(fp, lstr, Ekw.Routing());
        TechLayerParams *lp = tech_prm(ld);

        tRouteDir dir = lp->route_dir();
        if (dir == tDirHoriz)
            PutStr(fp, lstr, " dir=HORIZ");
        if (dir == tDirVert)
            PutStr(fp, lstr, " dir=VERT");

        if (lp->route_h_pitch() > 0) {
            snprintf(buf, sizeof(buf), " pitch=%g",
                MICRONS(lp->route_h_pitch()));
            if (lp->route_v_pitch() > 0 &&
                    lp->route_v_pitch() != lp->route_h_pitch()) {
                int len = strlen(buf);
                char *e = buf + len;
                *e++ = ',';
                len++;
                snprintf(e, sizeof(buf) - len, "%g",
                    MICRONS(lp->route_v_pitch()));
            }
            PutStr(fp, lstr, buf);
        }

        if (lp->route_h_offset() > 0) {
            snprintf(buf, sizeof(buf), " offset=%g",
                MICRONS(lp->route_h_offset()));
            if (lp->route_v_offset() > 0 &&
                    lp->route_v_offset() != lp->route_h_offset()) {
                int len = strlen(buf);
                char *e = buf + len;
                *e++ = ',';
                len++;
                snprintf(e, sizeof(buf) - len, "%g",
                    MICRONS(lp->route_v_offset()));
            }
            PutStr(fp, lstr, buf);
        }

        if (lp->route_width() > 0) {
            snprintf(buf, sizeof(buf), " width=%g",
                MICRONS(lp->route_width()));
            PutStr(fp, lstr, buf);
        }

        if (lp->route_max_dist() > 0) {
            snprintf(buf, sizeof(buf), " maxdist=%g",
                MICRONS(lp->route_max_dist()));
            PutStr(fp, lstr, buf);
        }
    }
}

