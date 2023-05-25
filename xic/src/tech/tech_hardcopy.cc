
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
#include "dsp_window.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "dsp_tkif.h"
#include "cd_lgen.h"
#include "tech.h"
#include "tech_kwords.h"
#include "tech_attr_cx.h"
#include "errorlog.h"


HCcb cTech::tc_hccb =
{
    0,              // hcsetup
    0,              // hcgo
    0,              // hcframe
    0,              // format
    0,              // drvrmask
    HClegOn,        // legend
    HCbest,         // orient
    0,              // resolution
    "",             // command
    false,          // tofile
    "",             // tofilename
    0.0,            // left
    0.0,            // top
    0.0,            // width
    0.0             // height
};


void
cTech::SetHardcopyFuncs(
    bool(*setup)(bool, int, bool, GRdraw*),
    int(*go)(HCorientFlags, HClegType, GRdraw*),
    bool(*frame)(HCframeMode, GRobject, int*, int*, int*, int*, GRdraw*))
{
    tc_hccb.hcsetup = setup;
    tc_hccb.hcgo = go;
    tc_hccb.hcframe = frame;
}


TCret
cTech::ParseHardcopy(FILE *techfp)
{
    const char *kw = tc_kwbuf;
    if (Matching(Tkw.HardCopyDevice())) {
        // Begin the definition of the hardcopy drivers in use.
        //
        while (read_driver(techfp)) ;
        return (TCmatch);
    }
    if (Matching(Tkw.DefaultDriver()) || Matching(Tkw.AltDriver())) {
        // Set the default hardcopy driver, both physical and
        // electrical modes.
        //
        int i = DSPpkg::self()->FindHCindex(tc_inbuf);
        if (i >= 0) {
            tc_hccb.drvrmask &= ~(1 << i);  // enable driver
            tc_phys_hc_format = i;
            tc_elec_hc_format = i;
        }
        return (TCmatch);
    }
    if (Matching(Tkw.ElecDefaultDriver()) || Matching(Tkw.AltElecDriver()) ||
            Matching(Tkw.ElecAltDriver())) {
        // Set the default hardcopy driver used in electrical
        // mode.
        //
        int i = DSPpkg::self()->FindHCindex(tc_inbuf);
        if (i >= 0) {
            tc_hccb.drvrmask &= ~(1 << i);  // enable driver
            tc_elec_hc_format = i;
        }
        return (TCmatch);
    }
    if (Matching(Tkw.PhysDefaultDriver()) || Matching(Tkw.AltPhysDriver()) ||
            Matching(Tkw.PhysAltDriver())) {
        // Set the default hardcopy driver used in physical
        // mode.
        //
        int i = DSPpkg::self()->FindHCindex(tc_inbuf);
        if (i >= 0) {
            tc_hccb.drvrmask &= ~(1 << i);  // enable driver
            tc_phys_hc_format = i;
        }
        return (TCmatch);
    }
    if (
            Matching(Tkw.AltAxes()) ||
            Matching(Tkw.AltGridSpacing()) ||
            Matching(Tkw.AltPhysGridSpacing()) ||
            Matching(Tkw.PhysAltGridSpacing()) ||
            Matching(Tkw.AltElecGridSpacing()) ||
            Matching(Tkw.ElecAltGridSpacing()) ||
            Matching(Tkw.AltShowGrid()) ||
            Matching(Tkw.AltElecShowGrid()) ||
            Matching(Tkw.ElecAltShowGrid()) ||
            Matching(Tkw.AltPhysShowGrid()) ||
            Matching(Tkw.PhysAltShowGrid()) ||
            Matching(Tkw.AltGridOnBottom()) ||
            Matching(Tkw.AltElecGridOnBottom()) ||
            Matching(Tkw.ElecAltGridOnBottom()) ||
            Matching(Tkw.AltPhysGridOnBottom()) ||
            Matching(Tkw.PhysAltGridOnBottom()) ||
            Matching(Tkw.AltGridStyle()) ||
            Matching(Tkw.AltElecGridStyle()) ||
            Matching(Tkw.ElecAltGridStyle()) ||
            Matching(Tkw.AltPhysGridStyle()) ||
            Matching(Tkw.PhysAltGridStyle()) ||
            Matching(Tkw.AltBackground()) ||
            Matching(Tkw.AltElecBackground()) ||
            Matching(Tkw.ElecAltBackground()) ||
            Matching(Tkw.AltPhysBackground()) ||
            Matching(Tkw.PhysAltBackground())) {

        return (SaveError("Obsolete keyword %s, (ignored).", kw));
    }

    return (TCnone);
}


bool
cTech::read_driver(FILE *techfp)
{
    if (!techfp)
        return (false);

    int curdrvr = -1;
    sAttrContext *ac = 0;
    sLayerAttr *la = 0;
    const char *kw = tc_kwbuf;

    if (Matching(Tkw.HardCopyDevice())) {
        char *drvr = 0, *disable = 0;
        const char *cp = tc_inbuf;
        GetWord(&cp, &drvr);
        GetWord(&cp, &disable);
        ac = 0;
        la = 0;

        int i = DSPpkg::self()->FindHCindex(drvr);
        if (i >= 0) {
            curdrvr = i;
            if (disable && (lstring::cieq(disable, "off") ||
                    lstring::cieq(disable, "disable") ||
                    lstring::cieq(disable, "n")))
                tc_hccb.drvrmask |= (1 << i);  // disable driver
            else
                tc_hccb.drvrmask &= ~(1 << i);  // enable driver
            ac = GetAttrContext(curdrvr, true);
        }
        else {
            if (DSPpkg::self()->MainDev() &&
                    DSPpkg::self()->MainDev()->ident != _devNULL_) {
                // Don't whine about drivers in non-graphics mode.
                if (drvr
#ifndef WIN32
                    && strcmp(drvr, "windows_native")
                    // Don't whine about the windows_native driver
                    // when not using Windows.
#endif
                        ) {
                    char *e = SaveError("Unknown hardcopy driver \"%s\".",
                        drvr);
                    Log()->WarningLogV(mh::Techfile, "%s\n", e);
                    delete [] e;
                }
            }
            curdrvr = -1;
        }
        SetCmtType(tBlkHcpy, drvr);
        delete [] drvr;
        delete [] disable;
    }
    else {
        return (false);
    }

    bool ret = false;
    while ((kw = GetKeywordLine(techfp)) != 0) {
        TCret tcret = dispatch_drvr(curdrvr, ac, &la);
        if (tcret != TCnone) {
            if (tcret != TCmatch) {
                Log()->WarningLogV(mh::Techfile, "%s\n", tcret);
                delete [] tcret;
            }
        }
        else {
            if (Matching(Tkw.HardCopyDevice())) {
                ret = true;
                break;
            }
            char *e = SaveError("Unknown keyword \"%s\".", kw);
            Log()->WarningLogV(mh::Techfile, "%s\n", e);
            delete [] e;
        }
    }
    return (ret);
}


namespace {
    // Scan for a floating point constant.  Convert to inches if
    // followed by "cm".  Return false if error.
    //
    bool get_float(const char *buf, double *d)
    {
        const char *s = buf;
        while (isspace(*s)) s++;
        while (isdigit(*s) || *s == '+' || *s == '-' || *s == '.' ||
            *s == 'e' || *s == 'E') s++;
        if (sscanf(buf, "%lf", d) != 1)
            return (false);
        while (isspace(*s)) s++;
        if (*s == 'c' && *(s+1) == 'm')
            (*d) *= 2.54;
        return (true);
    }
}


TCret
cTech::dispatch_drvr(int curdrvr, sAttrContext *ac, sLayerAttr **pla)
{
    const char *nmsg = "Keyword %s not in layer block.";

    if (Matching(Tkw.HardCopyCommand())) {
        char *ip = tc_inbuf;
        while (isspace(*ip))
            ip++;
        if (curdrvr >= 0) {
            HCdesc *hcdesc = DSPpkg::self()->HCof(curdrvr);
            if (hcdesc)
                hcdesc->defaults.command = lstring::copy(ip);
        }
        return (TCmatch);
    }
    if (Matching(Tkw.HardCopyResol())) {
        if (curdrvr >= 0) {
            HCdesc *hcdesc = DSPpkg::self()->HCof(curdrvr);
            if (hcdesc && !(hcdesc->limits.flags & HCfixedResol)) {
                const char *ip = tc_inbuf;
                int i;
                for (i = 0; ; i++) {
                    if (!GetWord(&ip, 0))
                        break;
                }
                if (i) {
                    hcdesc->limits.resols = new const char*[i+1];
                    ip = tc_inbuf;
                    for (i = 0; ; i++) {
                        char *c;
                        if (!GetWord(&ip, &c))
                            break;
                        hcdesc->limits.resols[i] = c;
                    }
                    hcdesc->limits.resols[i] = 0;
                }
            }
        }
        return (TCmatch);
    }
    if (Matching(Tkw.HardCopyDefResol())) {
        if (curdrvr >= 0) {
            HCdesc *hcdesc = DSPpkg::self()->HCof(curdrvr);
            if (hcdesc && !(hcdesc->limits.flags & HCfixedResol)) {
                int i = GetInt(tc_inbuf);
                if (i < 0)
                    i = 0;
                hcdesc->defaults.defresol = i;
            }
        }
        return (TCmatch);
    }
    if (Matching(Tkw.HardCopyLegend())) {
        if (curdrvr >= 0) {
            HCdesc *hcdesc = DSPpkg::self()->HCof(curdrvr);
            if (hcdesc) {
                int i = GetInt(tc_inbuf);
                if (i < 0 || i > 2)
                    i = 0;
                hcdesc->defaults.legend = (HClegType)i;
            }
        }
        return (TCmatch);
    }
    if (Matching(Tkw.HardCopyOrient())) {
        if (curdrvr >= 0) {
            HCdesc *hcdesc = DSPpkg::self()->HCof(curdrvr);
            if (hcdesc) {
                int i = GetInt(tc_inbuf);
                if (i < 0 || i > 2)
                    i = 0;
                hcdesc->defaults.orient = i;
            }
        }
        return (TCmatch);
    }
    if (Matching(Tkw.HardCopyDefWidth())) {
        if (curdrvr >= 0) {
            double d;
            if (get_float(tc_inbuf, &d)) {
                HCdesc *hcdesc = DSPpkg::self()->HCof(curdrvr);
                if (hcdesc)
                    hcdesc->defaults.defwidth = d;
            }
            else {
                return (SaveError("%s: bad floating point format.",
                    Tkw.HardCopyDefWidth()));
            }
        }
        return (TCmatch);
    }
    if (Matching(Tkw.HardCopyDefHeight())) {
        if (curdrvr >= 0) {
            double d;
            if (get_float(tc_inbuf, &d)) {
                HCdesc *hcdesc = DSPpkg::self()->HCof(curdrvr);
                if (hcdesc)
                    hcdesc->defaults.defheight = d;
            }
            else {
                return (SaveError("%s: bad floating point format.",
                    Tkw.HardCopyDefHeight()));
            }
        }
        return (TCmatch);
    }
    if (Matching(Tkw.HardCopyDefXoff())) {
        if (curdrvr >= 0) {
            double d;
            if (get_float(tc_inbuf, &d)) {
                HCdesc *hcdesc = DSPpkg::self()->HCof(curdrvr);
                if (hcdesc)
                    hcdesc->defaults.defxoff = d;
            }
            else {
                return (SaveError("%s: bad floating point format.",
                    Tkw.HardCopyDefXoff()));
            }
        }
        return (TCmatch);
    }
    if (Matching(Tkw.HardCopyDefYoff())) {
        if (curdrvr >= 0) {
            double d;
            if (get_float(tc_inbuf, &d)) {
                HCdesc *hcdesc = DSPpkg::self()->HCof(curdrvr);
                if (hcdesc)
                    hcdesc->defaults.defyoff = d;
            }
            else {
                return (SaveError("%s: bad floating point format.",
                    Tkw.HardCopyDefYoff()));
            }
        }
        return (TCmatch);
    }
    if (Matching(Tkw.HardCopyMinWidth())) {
        if (curdrvr >= 0) {
            double d;
            if (get_float(tc_inbuf, &d)) {
                HCdesc *hcdesc = DSPpkg::self()->HCof(curdrvr);
                if (hcdesc)
                    hcdesc->limits.minwidth = d;
            }
            else {
                return (SaveError("%s: bad floating point format.",
                    Tkw.HardCopyMinWidth()));
            }
        }
        return (TCmatch);
    }
    if (Matching(Tkw.HardCopyMinHeight())) {
        if (curdrvr >= 0) {
            double d;
            if (get_float(tc_inbuf, &d)) {
                HCdesc *hcdesc = DSPpkg::self()->HCof(curdrvr);
                if (hcdesc)
                    hcdesc->limits.minheight = d;
            }
            else {
                return (SaveError("%s: bad floating point format.",
                    Tkw.HardCopyMinHeight()));
            }
        }
        return (TCmatch);
    }
    if (Matching(Tkw.HardCopyMinXoff())) {
        if (curdrvr >= 0) {
            double d;
            if (get_float(tc_inbuf, &d)) {
                HCdesc *hcdesc = DSPpkg::self()->HCof(curdrvr);
                if (hcdesc)
                    hcdesc->limits.minxoff = d;
            }
            else {
                return (SaveError("%s: bad floating point format.",
                    Tkw.HardCopyMinXoff()));
            }
        }
        return (TCmatch);
    }
    if (Matching(Tkw.HardCopyMinYoff())) {
        if (curdrvr >= 0) {
            double d;
            if (get_float(tc_inbuf, &d)) {
                HCdesc *hcdesc = DSPpkg::self()->HCof(curdrvr);
                if (hcdesc)
                    hcdesc->limits.minyoff = d;
            }
            else {
                return (SaveError("%s: bad floating point format.",
                    Tkw.HardCopyMinYoff()));
            }
        }
        return (TCmatch);
    }
    if (Matching(Tkw.HardCopyMaxWidth())) {
        if (curdrvr >= 0) {
            double d;
            if (get_float(tc_inbuf, &d)) {
                HCdesc *hcdesc = DSPpkg::self()->HCof(curdrvr);
                if (hcdesc)
                    hcdesc->limits.maxwidth = d;
            }
            else {
                return (SaveError("%s: bad floating point format.",
                    Tkw.HardCopyMaxWidth()));
            }
        }
        return (TCmatch);
    }
    if (Matching(Tkw.HardCopyMaxHeight())) {
        if (curdrvr >= 0) {
            double d;
            if (get_float(tc_inbuf, &d)) {
                HCdesc *hcdesc = DSPpkg::self()->HCof(curdrvr);
                if (hcdesc)
                    hcdesc->limits.maxheight = d;
            }
            else {
                return (SaveError("%s: bad floating point format.",
                    Tkw.HardCopyMaxHeight()));
            }
        }
        return (TCmatch);
    }
    if (Matching(Tkw.HardCopyMaxXoff())) {
        if (curdrvr >= 0) {
            double d;
            if (get_float(tc_inbuf, &d)) {
                HCdesc *hcdesc = DSPpkg::self()->HCof(curdrvr);
                if (hcdesc)
                    hcdesc->limits.maxxoff = d;
            }
            else {
                return (SaveError("%s: bad floating point format.",
                    Tkw.HardCopyMaxXoff()));
            }
        }
        return (TCmatch);
    }
    if (Matching(Tkw.HardCopyMaxYoff())) {
        if (curdrvr >= 0) {
            double d;
            if (get_float(tc_inbuf, &d)) {
                HCdesc *hcdesc = DSPpkg::self()->HCof(curdrvr);
                if (hcdesc)
                    hcdesc->limits.maxyoff = d;
            }
            else {
                return (SaveError("%s: bad floating point format.",
                    Tkw.HardCopyMaxYoff()));
            }
        }
        return (TCmatch);
    }
    if (Matching(Tkw.HardCopyPixWidth())) {
        // no longer used, but don't complain
        return (TCmatch);
    }

    if (Matching(Tkw.PhysLayer()) || Matching(Tkw.LayerName()) ||
            Matching(Tkw.PhysLayerName()) || Matching(Tkw.Layer()) ||
            Matching(Tkw.ElecLayer()) || Matching(Tkw.ElecLayerName())) {
        *pla = 0;
        if (ac) {
            CDl *ld = CDldb()->findLayer(tc_inbuf, Physical);
            if (ld) {
                SetCmtSubkey(ld->name());
                *pla = ac->get_layer_attributes(ld, true);
            }
            else
                return (SaveError("Unknown layer %s.", tc_inbuf));
        }
        return (TCmatch);
    }
    if (Matching(Tkw.kwRGB())) {
        if (*pla) {
            int rgb[3];
            bool ret = GetRgb(rgb);
            int pix = 0;
            DSPmainDraw(DefineColor(&pix, rgb[0], rgb[1], rgb[2]))
            (*pla)->set_rgbp(rgb[0], rgb[1], rgb[2], pix);
            if (!ret) {
                return (SaveError("%s: unknown color %s.",
                    Tkw.kwRGB(), tc_inbuf));
            }
        }
        else
            return (SaveError(nmsg, Tkw.kwRGB()));
        return (TCmatch);
    }
    if (Matching(Tkw.Filled())) {
        if (*pla) {
            if (!GetFilled(*pla)) {
                return (SaveError("%s: parse failed.  %s",
                    Tkw.Filled(), Errs()->get_error()));
            }
        }
        else
            return (SaveError(nmsg, Tkw.Filled()));
        return (TCmatch);
    }
    if (Matching(Tkw.Invisible())) {
        if (*pla)
            (*pla)->setInvisible(GetBoolean(tc_inbuf));
        else
            return (SaveError(nmsg, Tkw.Invisible()));
        return (TCmatch);
    }
    if (Matching(Tkw.HpglFilled())) {
        // HP-GL fill specification for layer.
        if (curdrvr >= 0) {
            if (curdrvr != DSPpkg::self()->FindHCindex("hpgl")) {
                return (SaveError(
                    "Keyword %s misplaced, applicable only to HPGL driver.",
                     Tkw.HpglFilled()));
            }
            if (*pla) {
                int i1, i2, i3;
                int n = sscanf(tc_inbuf, "%d%d%d", &i1, &i2, &i3);
                if (n >= 1) {
                    if ((i1 >= 1 && i1 <= 4) || i1 == 10 || i1 == 11)
                        (*pla)->set_hpgl_fill(0, i1);
                    else {
                        return (SaveError(
                            "Unsupported %s first value, must be 1-4,10,11.",
                            Tkw.HpglFilled()));
                    }
                }
                if (n >= 2)
                    (*pla)->set_hpgl_fill(1, i2);
                if (n >= 3)
                    (*pla)->set_hpgl_fill(2, i3);
            }
            else
                return (SaveError(nmsg, Tkw.HpglFilled()));
        }
        return (TCmatch);
    }
    if (Matching(Tkw.XfigFilled())) {
        if (curdrvr > 0) {
            if (curdrvr != DSPpkg::self()->FindHCindex("xfig")) {
                return (SaveError(
                    "Keyword %s misplaced, applicable only to Xfig driver.",
                    Tkw.XfigFilled()));
            }
            if (*pla) {
                int i1;
                int n = sscanf(tc_inbuf, "%d", &i1);
                if (n > 0 && i1 > 0) {
                    // Xfig 3.2 valid fills <= 56, we don't support 0
                    if (i1 > 56)
                        i1 = 56;
                    (*pla)->set_misc_fill(0, i1);
                }
            }
            else
                return (SaveError(nmsg, Tkw.XfigFilled()));
        }
        return (TCmatch);
    }

    TCret tcret = ParseAttributes(ac, true);
    if (tcret != TCnone)
        return (tcret);

    return (TCnone);
}


// Print the hardcopy-related entries for the technology file
// This should be called last when creating the tech file.
//
void
cTech::PrintHardcopy(FILE *techfp)
{
    if (tc_phys_hc_format == tc_elec_hc_format) {
        HCdesc *hcdesc = DSPpkg::self()->HCof(tc_phys_hc_format);
        if (hcdesc)
            fprintf(techfp, "%s %s\n", Tkw.DefaultDriver(), hcdesc->keyword);
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.DefaultDriver());
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.ElecDefaultDriver());
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.PhysDefaultDriver());
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.AltDriver());
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.AltElecDriver());
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.ElecAltDriver());
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.AltPhysDriver());
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.PhysAltDriver());
    }
    else {
        HCdesc *hcdesc = DSPpkg::self()->HCof(tc_elec_hc_format);
        if (hcdesc)
            fprintf(techfp, "%s %s\n", Tkw.ElecDefaultDriver(),
                hcdesc->keyword);
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.DefaultDriver());
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.ElecDefaultDriver());
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.AltDriver());
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.AltElecDriver());
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.ElecAltDriver());
        hcdesc = DSPpkg::self()->HCof(tc_phys_hc_format);
        if (hcdesc)
            fprintf(techfp, "%s %s\n", Tkw.PhysDefaultDriver(),
                hcdesc->keyword);
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.PhysDefaultDriver());
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.AltPhysDriver());
        CommentDump(techfp, 0, tBlkNone, 0, Tkw.PhysAltDriver());
    }
    // End of hard copy keywords

    // the hardcopy driver info
    fprintf(techfp, "\n");
    for (int i = 0; DSPpkg::self()->HCof(i); i++) {
        HCdesc *hcdesc = DSPpkg::self()->HCof(i);
        if (tc_hccb.drvrmask & (1 << i))
            fprintf(techfp, "%s %s disable\n\n", Tkw.HardCopyDevice(),
                hcdesc->keyword);
        else {
            const char *drvrkw = hcdesc->keyword;
            fprintf(techfp, "%s %s\n", Tkw.HardCopyDevice(), drvrkw);
            CommentDump(techfp, 0, tBlkHcpy, drvrkw, Tkw.HardCopyDevice());
            if (hcdesc->defaults.command && *hcdesc->defaults.command)
                fprintf(techfp, "%s %s\n", Tkw.HardCopyCommand(),
                    hcdesc->defaults.command);
            CommentDump(techfp, 0, tBlkHcpy, drvrkw, Tkw.HardCopyCommand());
            if (hcdesc->limits.resols) {
                fputs(Tkw.HardCopyResol(), techfp);
                for (int j = 0; hcdesc->limits.resols[j]; j++)
                    fprintf(techfp, " %s", hcdesc->limits.resols[j]);
                fprintf(techfp, "\n");
                fprintf(techfp, "%s %d\n", Tkw.HardCopyDefResol(),
                    hcdesc->defaults.defresol);
            }
            CommentDump(techfp, 0, tBlkHcpy, drvrkw, Tkw.HardCopyResol());

            fprintf(techfp, "%s %d\n", Tkw.HardCopyLegend(),
                hcdesc->defaults.legend);
            CommentDump(techfp, 0, tBlkHcpy, drvrkw, Tkw.HardCopyLegend());
            fprintf(techfp, "%s %d\n", Tkw.HardCopyOrient(),
                hcdesc->defaults.orient);
            CommentDump(techfp, 0, tBlkHcpy, drvrkw, Tkw.HardCopyOrient());

            fprintf(techfp, "%s %g\n", Tkw.HardCopyDefWidth(),
                hcdesc->defaults.defwidth);
            CommentDump(techfp, 0, tBlkHcpy, drvrkw, Tkw.HardCopyDefWidth());
            fprintf(techfp, "%s %g\n", Tkw.HardCopyDefHeight(),
                hcdesc->defaults.defheight);
            CommentDump(techfp, 0, tBlkHcpy, drvrkw, Tkw.HardCopyDefHeight());
            fprintf(techfp, "%s %g\n", Tkw.HardCopyDefXoff(),
                hcdesc->defaults.defxoff);
            CommentDump(techfp, 0, tBlkHcpy, drvrkw, Tkw.HardCopyDefXoff());
            fprintf(techfp, "%s %g\n", Tkw.HardCopyDefYoff(),
                hcdesc->defaults.defyoff);
            CommentDump(techfp, 0, tBlkHcpy, drvrkw, Tkw.HardCopyDefYoff());
            fprintf(techfp, "%s %g\n", Tkw.HardCopyMinWidth(),
                hcdesc->limits.minwidth);
            CommentDump(techfp, 0, tBlkHcpy, drvrkw, Tkw.HardCopyMinWidth());
            fprintf(techfp, "%s %g\n", Tkw.HardCopyMinHeight(),
                hcdesc->limits.minheight);
            CommentDump(techfp, 0, tBlkHcpy, drvrkw, Tkw.HardCopyMinHeight());
            fprintf(techfp, "%s %g\n", Tkw.HardCopyMinXoff(),
                hcdesc->limits.minxoff);
            CommentDump(techfp, 0, tBlkHcpy, drvrkw, Tkw.HardCopyMinXoff());
            fprintf(techfp, "%s %g\n", Tkw.HardCopyMinYoff(),
                hcdesc->limits.minyoff);
            CommentDump(techfp, 0, tBlkHcpy, drvrkw, Tkw.HardCopyMinYoff());
            fprintf(techfp, "%s %g\n", Tkw.HardCopyMaxWidth(),
                hcdesc->limits.maxwidth);
            CommentDump(techfp, 0, tBlkHcpy, drvrkw, Tkw.HardCopyMaxWidth());
            fprintf(techfp, "%s %g\n", Tkw.HardCopyMaxHeight(),
                hcdesc->limits.maxheight);
            CommentDump(techfp, 0, tBlkHcpy, drvrkw, Tkw.HardCopyMaxHeight());
            fprintf(techfp, "%s %g\n", Tkw.HardCopyMaxXoff(),
                hcdesc->limits.maxxoff);
            CommentDump(techfp, 0, tBlkHcpy, drvrkw, Tkw.HardCopyMaxXoff());
            fprintf(techfp, "%s %g\n", Tkw.HardCopyMaxYoff(),
                hcdesc->limits.maxyoff);
            CommentDump(techfp, 0, tBlkHcpy, drvrkw, Tkw.HardCopyMaxYoff());

            sAttrContext *ac = GetAttrContext(i, false);
            if (ac) {
                if (ac)
                    PrintAttributes(techfp, ac, drvrkw);

                CDlgen egen(Electrical);
                CDl *ld;
                while ((ld = egen.next()) != 0) {
                    sLayerAttr *la = ac->get_layer_attributes(ld, false);
                    print_driver_layer_block(techfp, hcdesc, la, Electrical);
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.Layer(), ld->name());
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.LayerName(), ld->name());
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.PhysLayer(), ld->name());
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.PhysLayerName(), ld->name());
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.ElecLayer(), ld->name());
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.ElecLayerName(), ld->name());
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.kwRGB(), ld->name());
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.Filled(), ld->name());
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.Invisible(), ld->name());
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.HpglFilled(), ld->name());
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.XfigFilled(), ld->name());
                }
                CDlgen pgen(Physical);
                while ((ld = pgen.next()) != 0) {
                    sLayerAttr *la = ac->get_layer_attributes(ld, false);
                    print_driver_layer_block(techfp, hcdesc, la, Physical);
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.Layer(), ld->name());
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.LayerName(), ld->name());
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.PhysLayer(), ld->name());
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.PhysLayerName(), ld->name());
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.ElecLayer(), ld->name());
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.ElecLayerName(), ld->name());
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.kwRGB(), ld->name());
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.Filled(), ld->name());
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.Invisible(), ld->name());
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.HpglFilled(), ld->name());
                    CommentDump(techfp, 0, tBlkHcpy, drvrkw,
                        Tkw.XfigFilled(), ld->name());
                }
            }
            fprintf(techfp, "\n");
        }
    }
}


void
cTech::print_driver_layer_block(FILE *techfp, const HCdesc *hcdesc,
    const sLayerAttr *la, DisplayMode mode)
{
    if (!la)
        return;
    const CDl *ld = la->ldesc();

    bool clr_diff = (la->r() != dsp_prm(ld)->red() ||
        la->g() != dsp_prm(ld)->green() || la->b() != dsp_prm(ld)->blue());
    bool vis_diff = (la->isInvisible() != ld->isInvisible());

    bool fill_diff = false;
    if (la->isFilled() != ld->isFilled())
        fill_diff = true;
    else if (la->isFilled()) {
        if (la->cfill()->hasMap() != dsp_prm(ld)->fill()->hasMap())
            fill_diff = true;
        else if (la->cfill()->hasMap()) {
            if (la->cfill()->nX() != dsp_prm(ld)->fill()->nX() ||
                    la->cfill()->nY() != dsp_prm(ld)->fill()->nY())
                fill_diff = true;
            else if (la->isOutlined() != ld->isOutlined())
                fill_diff = true;
            else if (la->isCut() != ld->isCut())
                fill_diff = true;
            else {
                unsigned char *map1 = la->cfill()->newBitmap();
                unsigned char *map2 = dsp_prm(ld)->fill()->newBitmap();
                if (memcmp(map1, map2,
                        la->cfill()->nY()*((la->cfill()->nX() + 7)/8)))
                    fill_diff = true;
                delete [] map1;
                delete [] map2;
            }
        }
    }
    else {
        if (la->isOutlined() != ld->isOutlined())
            fill_diff = true;
        else if (la->isOutlinedFat() != ld->isOutlinedFat())
            fill_diff = true;
        else if (la->isCut() != ld->isCut())
            fill_diff = true;
    }

    bool is_hp = (hcdesc == DSPpkg::self()->FindHCdesc("hpgl"));
    bool hp_diff = (is_hp && la->hpgl_fill(0));
    bool is_xf = (hcdesc == DSPpkg::self()->FindHCdesc("xfig"));
    bool xf_diff = (is_xf && la->misc_fill(0));

    if (!clr_diff && !vis_diff && !fill_diff && !hp_diff && !xf_diff)
        return;

    fprintf(techfp, "%s %s\n",
        mode == Physical ? Tkw.PhysLayer() : Tkw.ElecLayer(), ld->name());

    if (pcheck(techfp, clr_diff))
        fprintf(techfp, "%s %d %d %d\n", Tkw.kwRGB(),
            la->r(), la->g(), la->b());
    if (pcheck(techfp, fill_diff)) {
        fputs(Tkw.Filled(), techfp);
        if (!(la->isFilled())) {
            if (la->isOutlined()) {
                if (la->isOutlinedFat()) {
                    if (la->isCut())
                        fprintf(techfp, " n fat cut\n");
                    else
                        fprintf(techfp, " n fat\n");
                }
                else {
                    if (la->isCut())
                        fprintf(techfp, " n outline cut\n");
                    else
                        fprintf(techfp, " n outline\n");
                }
            }
            else {
                if (la->isCut())
                    fprintf(techfp, " n cut\n");
                else
                    fprintf(techfp, " n\n");
            }
        }
        else {
            if (!la->cfill()->hasMap())
                fprintf(techfp, " y\n");
            else {
                fprintf(techfp, " \\\n");
                unsigned char *map = la->cfill()->newBitmap();
                PrintPmap(techfp, 0, map,
                    la->cfill()->nX(), la->cfill()->nY());
                delete [] map;
                if (la->isOutlined()) {
                    if (la->isCut())
                        fprintf(techfp, " outline cut\n");
                    else
                        fprintf(techfp, " outline\n");
                }
                else {
                    if (la->isCut())
                        fprintf(techfp, " cut\n");
                    else
                        fprintf(techfp, "\n");
                }
            }
        }
    }
    if (is_hp) {
        if (pcheck(techfp, hp_diff))
            fprintf(techfp, "%s %d %d %d\n", Tkw.HpglFilled(),
                la->hpgl_fill(0), la->hpgl_fill(1), la->hpgl_fill(2));
    }
    if (is_xf) {
        if (pcheck(techfp, xf_diff))
            fprintf(techfp, "%s %d\n", Tkw.XfigFilled(), la->misc_fill(0));
    }
    if (pcheck(techfp, vis_diff))
        fprintf(techfp, "%s %c\n", Tkw.Invisible(),
            la->isInvisible() ? 'y' : 'n');
}


// We maintain a sAttrContext struct for each driver.  This is the
// access points for these structs.  If create is true, a new struct
// will be created if it doesn't already exist.
//
sAttrContext *
cTech::GetAttrContext(int drvr, bool create)
{
#define ATTR_SIZE_INCR 16
    if (!DSPpkg::self()->HCof(drvr))
        return (0);
    if (!tc_attr_array) {
        if (!create)
            return (0);
        tc_attr_array_size = ATTR_SIZE_INCR;
        tc_attr_array = new sAttrContext*[tc_attr_array_size];
        memset(tc_attr_array, 0, tc_attr_array_size*sizeof(sAttrContext*));
    }
    if (drvr >= tc_attr_array_size) {
        sAttrContext **tmp = new sAttrContext*[tc_attr_array_size + ATTR_SIZE_INCR];
        memcpy(tmp, tc_attr_array, tc_attr_array_size*sizeof(sAttrContext*));
        memset(tmp + tc_attr_array_size, 0, ATTR_SIZE_INCR*sizeof(sAttrContext*));
        delete [] tc_attr_array;
        tc_attr_array = tmp;
        tc_attr_array_size += ATTR_SIZE_INCR;
    }
    if (create && !tc_attr_array[drvr]) {
        sAttrContext *cx = new sAttrContext;
        tc_attr_array[drvr] = cx;
        *cx->attr() = *DSP()->MainWdesc()->Attrib();
        for (int i = 0; i < AttrColorSize; i++)
            *cx->color(i) = *DSP()->ColorTab()->color_ent(i);
    }
    return (tc_attr_array[drvr]);
}


namespace { sAttrContext hc_bak_attr_cx; }

// Return the sAttrContext used to back up the main window attributes
// while in hard copy mode.
//
sAttrContext *
cTech::GetHCbakAttrContext()
{
    return (&hc_bak_attr_cx);
}
// End of cTech functions.


// Global allocator for sLayerAttr.
eltab_t<sLayerAttr> sAttrContext::ac_allocator;

sLayerAttr *
sAttrContext::get_layer_attributes(CDl *ld, bool create)
{
    if (!ld)
        return (0);
    if (!ac_layer_attrs) {
        if (!create)
            return (0);
        ac_layer_attrs = new itable_t<sLayerAttr>;
    }
    sLayerAttr *la = ac_layer_attrs->find(ld);
    if (!la && create) {
        la = ac_allocator.new_element();
        la->set(ld);
        la->save();
        ac_layer_attrs->link(la);
        ac_layer_attrs = ac_layer_attrs->check_rehash();
    }
    return (la);
}


void
sAttrContext::save_layer_attrs(CDl *ld)
{
    if (!ld)
        return;
    if (!ac_layer_attrs)
        ac_layer_attrs = new itable_t<sLayerAttr>;
    sLayerAttr *la = ac_layer_attrs->find(ld);
    if (!la) {
        la = ac_allocator.new_element();
        la->set(ld);
        la->save();
        ac_layer_attrs->link(la);
        ac_layer_attrs = ac_layer_attrs->check_rehash();
    }
    else
        la->save();
}


void
sAttrContext::restore_layer_attrs(CDl *ld)
{
    if (!ld)
        return;
    if (!ac_layer_attrs)
        return;
    sLayerAttr *la = ac_layer_attrs->find(ld);
    if (la)
        la->restore();
}


// Dump the color information to the tech file.  The second arg sets
// how to treat colors set to default values:  skip these, print
// commented, of print as normal.
//
void
sAttrContext::dump(FILE *techfp, CTPmode pmode)
{
    // Dump the attribute colors.
    for (int i = 0; i < AttrColorSize; i++) {
        sColorTab::sColorTabEnt *c = ac_colors + i;
        switch (i) {
        case BackgroundColor:
        case GhostColor:
        case HighlightingColor:
        case SelectColor1:
        case SelectColor2:
        case MarkerColor:
        case InstanceBBColor:
        case InstanceNameColor:
        case CoarseGridColor:
        case FineGridColor:
            c->check_print(techfp, pmode);
            break;
        case ElecBackgroundColor:
        case PhysBackgroundColor:
        case ElecGhostColor:
        case PhysGhostColor:
        case ElecHighlightingColor:
        case PhysHighlightingColor:
        case ElecSelectColor1:
        case PhysSelectColor1:
        case ElecSelectColor2:
        case PhysSelectColor2:
        case ElecMarkerColor:
        case PhysMarkerColor:
        case ElecInstanceBBColor:
        case PhysInstanceBBColor:
        case ElecInstanceNameColor:
        case PhysInstanceNameColor:
        case ElecCoarseGridColor:
        case PhysCoarseGridColor:
        case PhysFineGridColor:
        case ElecFineGridColor:
            break;
        default:
            c->print(techfp, pmode);
            DSP()->comment_dump(techfp, c->keyword(), c->aliases());
        }
    }
}

