
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
 $Id: drc_error.cc,v 5.68 2017/03/14 01:26:30 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "drc.h"
#include "editif.h"
#include "cd_celldb.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "fio.h"
#include "fio_chd.h"
#include "layertab.h"
#include "tech.h"
#include "pushpop.h"
#include "filestat.h"

//-----------------------------------------------------------------------------
// Error reporting functions
//-----------------------------------------------------------------------------


//------------------------------------------------------------
// Display functions - these must use the DSP transfrom stack.

// Show the current error zoids and highlighting in the window if
// display is true, called from redisplay().  Otherwise erase the
// error zoids and highlighting.
//
void
cDRC::showCurError(WindowDesc *wdesc, bool display)
{
    if (!wdesc->Wdraw())
        return;
    if (!wdesc->IsSimilar(Physical, DSP()->MainWdesc()))
        return;

    // Hook for alternative error presentation.  This function must be
    // provided by the application.
    //
    if (altShowError(wdesc, display))
        return;

    if (!drc_err_list)
        return;
    if (dspPkgIf()->IsDualPlane())
        wdesc->Wdraw()->SetXOR(display ? GRxHlite : GRxUnhlite);
    else {
        if (display)
            wdesc->Wdraw()->SetColor(
                DSP()->Color(HighlightingColor, Physical));
        else {
            BBox BB(CDnullBB);
            for (DRCerrList *el = drc_err_list; el; el = el->next())
                el->addBB(BB);
            DSP()->TPush();
            DSP()->TLoad(CDtfRegI0);  // Load push/pop transform register.
            DSP()->TBB(&BB, 0);
            DSP()->TPop();
            DRCerrList *etmp = drc_err_list;
            drc_err_list = 0;  // avoid reentrancy
            wdesc->Refresh(&BB);
            drc_err_list = etmp;
            return;
        }
    }

    DSP()->TPush();
    DSP()->TLoad(CDtfRegI0);  // Load push/pop transform register.
    for (DRCerrList *el = drc_err_list; el; el = el->next()) {
        el->showbad(wdesc);
        el->showfailed(wdesc);
    }
    DSP()->TPop();

    if (dspPkgIf()->IsDualPlane())
        wdesc->Wdraw()->SetXOR(GRxNone);
    else if (LT()->CurLayer())
        wdesc->Wdraw()->SetColor(dsp_prm(LT()->CurLayer())->pixel());
}


// Show an error as given in el.
//
void
cDRC::showError(WindowDesc *wdesc, bool display, DRCerrList *el)
{
    static bool skip;  // avoid reentrancy
    if (skip)
        return;

    if (!wdesc->Wdraw())
        return;
    if (!wdesc->IsSimilar(Physical, DSP()->MainWdesc()))
        return;

    if (dspPkgIf()->IsDualPlane())
        wdesc->Wdraw()->SetXOR(display ? GRxHlite : GRxUnhlite);
    else {
        if (display)
            wdesc->Wdraw()->SetColor(
                DSP()->Color(HighlightingColor, Physical));
        else {
            BBox BB(CDnullBB);
            el->addBB(BB);
            DSP()->TBB(&BB, 0);

            skip = true;
            wdesc->Refresh(&BB);
            skip = false;
            return;
        }
    }
    el->showbad(wdesc);
    el->showfailed(wdesc);
    if (dspPkgIf()->IsDualPlane())
        wdesc->Wdraw()->SetXOR(GRxNone);
    else if (LT()->CurLayer())
        wdesc->Wdraw()->SetColor(dsp_prm(LT()->CurLayer())->pixel());
}


// Erase and clear any errors associated with objects that overlap
// AOI.
//
void
cDRC::eraseErrors(const BBox *AOI)
{
    if (!drc_err_list)
        return;
    // Transform AOI to top level coords
    cTfmStack stk;
    stk.TPush();
    stk.TLoad(CDtfRegI0);
    stk.TInverse();
    stk.TLoadInverse();
    BBox tBB = *AOI;
    stk.TBB(&tBB, 0);
    stk.TPop();
    if (dspPkgIf()->IsDualPlane())
        DSPmainDraw(SetXOR(GRxUnhlite))
    else
        DSPmainDraw(SetColor(dsp_prm(CellLayer())->pixel()))
    DRCerrList *ep = 0, *en;
    for (DRCerrList *el = drc_err_list; el; el = en) {
        en = el->next();
        if (!el->pointer() || !el->pointer()->oBB().intersect(&tBB, false)) {
            ep = el;
            continue;
        }

        if (!ep)
            drc_err_list = en;
        else
            ep->set_next(en);
        BBox BB(CDnullBB);
        el->addBB(BB);
        delete el;

        stk.TBB(&BB, 0);

        WindowDesc *wd;
        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
        while ((wd = wgen.next()) != 0) {
            if (wd->IsSimilar(Physical, DSP()->MainWdesc()))
                wd->Refresh(&BB);
        }
    }
    if (dspPkgIf()->IsDualPlane())
        DSPmainDraw(SetXOR(GRxNone))
    else if (LT()->CurLayer())
        DSPmainDraw(SetColor(dsp_prm(LT()->CurLayer())->pixel()))
}


// Erase the error indication for objects about to be committed.  If
// doadd is true, erase the indicators for the added objects, used for
// Undo.
//
void
cDRC::eraseListError(const op_change_t *list, bool doadd)
{
    if (!list || !drc_err_list)
        return;
    if (dspPkgIf()->IsDualPlane())
        DSPmainDraw(SetXOR(GRxUnhlite))
    else
        DSPmainDraw(SetColor(dsp_prm(CellLayer())->pixel()))

    cTfmStack stk;
    stk.TPush();
    stk.TLoad(CDtfRegI0);
    for (const op_change_t *oc = list; oc; oc = EditIf()->ulFindNext(oc)) {
        CDo *odesc = doadd ? oc->oadd() : oc->odel();
        if (!odesc)
            continue;
        if (odesc->type() == CDINSTANCE) {
            DRCerrList *ep = 0, *en;
            int clev = PP()->Level();
            for (DRCerrList *el = drc_err_list; el; el = en) {
                en = el->next();
                // If the stack element at the present context depth is
                // our pointer, delete the entry
                //
                if (el->stack()) {
                    long sz = (long)el->stack()[0];
                    if (sz > clev && odesc == el->stack()[clev+1]) {

                        if (!ep)
                            drc_err_list = en;
                        else
                            ep->set_next(en);
                        BBox BB(CDnullBB);
                        el->addBB(BB);
                        delete el;

                        stk.TBB(&BB, 0);

                        WindowDesc *wd;
                        WDgen wgen(WDgen::MAIN, WDgen::CDDB);
                        while ((wd = wgen.next()) != 0) {
                            if (wd->IsSimilar(Physical, DSP()->MainWdesc()))
                                wd->Refresh(&BB);
                        }
                        continue;
                    }
                }
                ep = el;
            }

        }
        else if (odesc->type() != CDLABEL) {
            DRCerrList *ep = 0, *en;

            for (DRCerrList *el = drc_err_list; el; el = en) {
                en = el->next();

                if (el->pointer() && (odesc == el->pointer()->next_odesc())) {
                    if (!ep)
                        drc_err_list = en;
                    else
                        ep->set_next(en);

                    BBox BB(CDnullBB);
                    el->addBB(BB);
                    delete el;

                    stk.TBB(&BB, 0);

                    WindowDesc *wd;
                    WDgen wgen(WDgen::MAIN, WDgen::CDDB);
                    while ((wd = wgen.next()) != 0) {
                        if (wd->IsSimilar(Physical, DSP()->MainWdesc()))
                            wd->Refresh(&BB);
                    }
                    continue;
                }
                ep = el;
            }
        }
    }
    stk.TPop();

    if (dspPkgIf()->IsDualPlane())
        DSPmainDraw(SetXOR(GRxNone))
    else if (LT()->CurLayer())
        DSPmainDraw(SetColor(dsp_prm(LT()->CurLayer())->pixel()))
}


// Free the list of errors.
//
void
cDRC::clearCurError()
{
    for (DRCerrList *el = drc_err_list; el; el = drc_err_list) {
        drc_err_list = el->next();
        delete el;
    }
}


// Global to push a string into the errors file, used by signal
// handler.
//
void
cDRC::fileMessage(const char *str)
{
    if (drc_err_fp) {
        fprintf(drc_err_fp, "### %s", str);
        fflush(drc_err_fp);
    }
}


// Return the first error list entry found whose return region
// overlaps AOI.
//
DRCerrList *
cDRC::findError(const BBox *AOI)
{
    // So query works in a push.  Don't worry about 45 degree rotation,
    // since AOI is usefully only a point.
    cTfmStack stk;
    stk.TPush();
    stk.TLoad(CDtfRegI0);
    stk.TInverse();
    stk.TLoadInverse();
    BBox BB = *AOI;
    stk.TBB(&BB, 0);
    stk.TPop();

    for (DRCerrList *el = drc_err_list; el; el = el->next()) {
        if (el->intersect(BB))
            return (el);
    }
    return (0);
}


// Dump the current list of errors to a file.  Returns the number of errors
// in the list, no file if 0, or -1 if error opening file.
//
int
cDRC::dumpCurError(const char *fname)
{
    if (!drc_err_list || !DSP()->CurCellName())
        return (0);
    if (!fname || !*fname)
        return (-1);
    if (!filestat::create_bak(fname)) {
        GRpkgIf()->ErrPrintf(ET_ERROR, "%s", filestat::error_msg());
        return (-1);
    }
    FILE *fp = fopen(fname, "w");
    if (!fp)
        return (-1);
    int cnt = 0;
    if (!printFileHeader(fp, Tstring(DSP()->CurCellName()),
            DRC_UNKNOWN_AREA)) {
        Errs()->get_error();
        fclose(fp);
        return (-1);
    }
    for (DRCerrList *el = drc_err_list; el; el = el->next()) {
        if (!el->message())
            continue;
        if (lstring::prefix("In", el->message())) {
            const char *t = el->message();
            lstring::advtok(&t);
            char *cname = lstring::gettok(&t);
            if (!cname)
                continue;
            const char *string = t;
            char *e = cname + strlen(cname) - 1;
            if (*e == ':')
                *e = 0;
            fprintf(fp, "DRC error %d in cell %s:\n", cnt + 1, cname);
            delete [] cname;
            fprintf(fp, "%s\n", string);
        }
        else
            fprintf(fp, "%s\n", el->message());
        if (el->descr())
            fprintf(fp, "%s\n", el->descr());
        cnt++;
    }
    printFileEnd(fp, cnt, false);
    fclose(fp);
    return (cnt);
}


// Print the errors file header.
//
bool
cDRC::printFileHeader(FILE *fp, const char *cname, const BBox *AOI, cCHD *chd)
{
    if (!fp) {
        Errs()->add_error("printFileHeader: null file pointer.");
        return (false);
    }
    if (!cname) {
        if (chd)
            cname = chd->defaultCell(Physical);
        if (!cname) {
            Errs()->add_error("printFileHeader: null cell name pointer.");
            return (false);
        }
    }
    fprintf(fp, "%s %s\n", DRC_EFILE_HEADER, cname);
    fprintf(fp, "# Generated by %s\n", XM()->IdString());
    const char *tech = Tech()->TechnologyName();
    if (!tech || !*tech)
        tech = "default";
    const char *ext = Tech()->TechExtension();
    if (ext)
        fprintf(fp, "# Tech: %s (-T%s)\n", tech, ext);
    else
        fprintf(fp, "# Tech: %s\n", tech);
    fprintf(fp, "# Error level: %d\n", errorLevel());

    if (chd) {
        symref_t *s = chd->findSymref(cname, Physical);
        if (!s) {
            Errs()->add_error("printFileHeader: cell not found in CHD.");
            return (false);
        }
        if (AOI == DRC_UNKNOWN_AREA)
            fprintf(fp, "# Area checked: unknown\n");
        else if (!AOI || *AOI >= *s->get_bb())
            fprintf(fp, "# Area checked: entire cell\n");
        else
            fprintf(fp, "# Area checked: %.3f,%.3f %.3f,%.3f\n",
                MICRONS(AOI->left), MICRONS(AOI->bottom),
                MICRONS(AOI->right), MICRONS(AOI->top));

        const char *srcfile = chd->filename();
        if (!srcfile)
            srcfile = "unknown";
        fprintf(fp, "# Source file: %s\n", srcfile);
    }
    else {
        CDs *sd = CDcdb()->findCell(cname, Physical);
        if (!sd) {
            Errs()->add_error("printFileHeader: cell not found in memory.");
            return (false);
        }
        if (AOI == DRC_UNKNOWN_AREA)
            fprintf(fp, "# Area checked: unknown\n");
        else if (!AOI || *AOI >= *sd->BB())
            fprintf(fp, "# Area checked: entire cell\n");
        else
            fprintf(fp, "# Area checked: %.3f,%.3f %.3f,%.3f\n",
                MICRONS(AOI->left), MICRONS(AOI->bottom),
                MICRONS(AOI->right), MICRONS(AOI->top));

        const char *srcfile = sd->fileName();
        if (!srcfile)
            srcfile = "unknown";
        if (FIO()->IsSupportedArchiveFormat(sd->fileType()))
            fprintf(fp, "# Source file: %s\n", srcfile);
        else if (sd->fileType() == Fnative)
            fprintf(fp, "# Source directory: %s\n", srcfile);
        else if (sd->fileType() == Foa)
            fprintf(fp, "# Source library: %s\n", srcfile);
        fprintf(fp, "# File format: %s\n", FIO()->TypeName(sd->fileType()));
    }
    return (true);
}


// Write the final line to the error log, which contains the error
// count.
//
void
cDRC::printFileEnd(FILE *fp, unsigned int nerrs, bool chkmax)
{
    if (!fp)
        return;
    if (chkmax && maxErrors() > 0 && nerrs >= maxErrors())
        fprintf(fp, "End check, hit maximum error count %d.\n", maxErrors());
    else {
        if (nerrs == 1)
            fprintf(fp, "End check, 1 error found.\n");
        else
            fprintf(fp, "End check, %d errors found.\n", nerrs);
    }

    double secs = (drc_stop_time - drc_start_time)/1000.0;
    int mins = (int)(secs/60);
    if (mins)
        secs -= mins*60;
    int hours = mins/60;
    if (hours)
        mins -= hours*60;
    int days = hours/24;
    if (days)
        hours -= days*24;
    sLstr lstr;
    lstr.add("Elapsed ");
    if (days) {
        lstr.add_i(days);
        lstr.add_c('d');
    }
    if (hours) {
        lstr.add_i(hours);
        lstr.add_c('h');
    }
    if (mins) {
        lstr.add_i(mins);
        lstr.add_c('m');
    }
    lstr.add_g(secs);
    lstr.add_c('s');

    fprintf(fp, "Elapsed %s, %d objects checked.\n", lstr.string(),
        drc_num_checked);
}


// Static function.
// Compose a name for the error log file.
//
char *
cDRC::errFilename(const char *cellname, int pid)
{
    int len = strlen(cellname) + strlen(DRC_EFILE_PREFIX) + 1;
    if (pid > 0) {
        len++;
        int t = pid;
        while (t) {
            len++;
            t /= 10;
        }
    }
    len++;

    char *errfile = new char[len];
    if (pid > 0)
        sprintf(errfile, "%s.%s.%d", DRC_EFILE_PREFIX, cellname, pid);
    else
        sprintf(errfile, "%s.%s", DRC_EFILE_PREFIX, cellname);
    return (errfile);
}

// End of cDRC functions.


// The DRCerrList objects are stored after a DRC run, they record the
// errors for later presentation.

DRCerrList::~DRCerrList()
{
    delete el_pointer;
    if (el_stack) {
        long sz = (long)el_stack[0];
        for (int i = 0; i < sz; i++) {
            if (el_stack[i+1]->is_copy())
                delete el_stack[i+1];
        }
        delete [] el_stack;
    }
    delete [] el_message;
    delete [] el_descr;
}


// Extend the BB to include the "bad" and object areas.
//
void
DRCerrList::addBB(BBox &BB)
{
    BB.add(el_pbad[0].x, el_pbad[0].y);
    BB.add(el_pbad[1].x, el_pbad[1].y);
    BB.add(el_pbad[2].x, el_pbad[2].y);
    BB.add(el_pbad[3].x, el_pbad[3].y);
    if (el_pointer)
        BB.add(&el_pointer->oBB());
}


// Return true if BB overlaps the error indicator.
//
bool
DRCerrList::intersect(const BBox &BB)
{
    Poly po;
    po.numpts = 5;
    po.points = el_pbad;
    return (po.intersect(&BB, false));
}


// Transform the "bad" area.
//
void
DRCerrList::transf(const DRCerrRet *er, const cTfmStack *tstk)
{
    el_pbad[0] = er->pbad(0);
    el_pbad[1] = er->pbad(1);
    el_pbad[2] = er->pbad(2);
    el_pbad[3] = er->pbad(3);
    el_pbad[4] = er->pbad(4);
    tstk->TPath(5, el_pbad);
}


// Draw the error region.
//
void
DRCerrList::showbad(WindowDesc *wdesc)
{
    wdesc->ShowLineW(el_pbad[0].x, el_pbad[0].y, el_pbad[1].x, el_pbad[1].y);
    wdesc->ShowLineW(el_pbad[1].x, el_pbad[1].y, el_pbad[2].x, el_pbad[2].y);
    wdesc->ShowLineW(el_pbad[2].x, el_pbad[2].y, el_pbad[3].x, el_pbad[3].y);
    wdesc->ShowLineW(el_pbad[3].x, el_pbad[3].y, el_pbad[4].x, el_pbad[4].y);
    wdesc->ShowLineW(el_pbad[4].x, el_pbad[4].y, el_pbad[0].x, el_pbad[0].y);
}


// Draw the highlighting of failed objects.
//
void
DRCerrList::showfailed(WindowDesc *wdesc)
{
    if (!el_pointer)
        return;
    if (!wdesc->Wdraw())
        return;
    switch (el_pointer->type()) {
    case CDBOX:
        wdesc->Wdraw()->SetLinestyle(DSP()->BoxLinestyle());
        wdesc->ShowLineBox(&el_pointer->oBB());
        wdesc->Wdraw()->SetLinestyle(0);
        wdesc->ShowLineW(el_pointer->oBB().left, el_pointer->oBB().bottom,
            el_pointer->oBB().right, el_pointer->oBB().top);
        wdesc->ShowLineW(el_pointer->oBB().left, el_pointer->oBB().top,
            el_pointer->oBB().right, el_pointer->oBB().bottom);
        break;

    case CDWIRE:
        wdesc->Wdraw()->SetLinestyle(DSP()->BoxLinestyle());
        Point *polypts;
        int polynum;
        if (((const CDw*)el_pointer)->w_toPoly(&polypts, &polynum)) {
            wdesc->ShowLinePath(polypts, polynum);
            delete [] polypts;
        }
        wdesc->Wdraw()->SetLinestyle(0);
        break;

    case CDPOLYGON:
        wdesc->Wdraw()->SetLinestyle(DSP()->BoxLinestyle());
        wdesc->ShowLinePath(OPOLY(el_pointer)->points(),
            OPOLY(el_pointer)->numpts());
        wdesc->Wdraw()->SetLinestyle(0);
        break;
    }
}


// Obtain the "stack", which is a list of instance descriptors, top to
// bottom, representing the hierarchy from the top cell to the
// instance containing the object being drc'ed.  This includes the
// hierarchy above a "push".
//
void
DRCerrList::newstack(const sPF *gen)
{
    const CDc *st[CDMAXCALLDEPTH];
    int sp = 0;
    PP()->InstanceList(st, &sp);
    // Now have hierarchy above the current cell, if in a push.

    if (gen) {
        // add the descent from the generator, this is relative to
        // the current cell
        sp = gen->drc_stack(st, sp);
    }
    if (sp) {
        el_stack = new const CDc*[sp + 1];
        el_stack[0] = (CDc*)(long)sp;
        for (int i = 0; i < sp; i++)
            el_stack[i + 1] = st[i];
    }
}
// End DRCerrList functions.


//
//  The DRCerrRet list is returned from the test functions.
//

DRCerrRet::DRCerrRet(const DRCtestDesc *td, int vc)
{
    er_rule = td;
    er_errtype = td ? td->result() : 0;
    if (td) {
        for (int i = 0; i < 5; i++)
            er_pbad[i] = td->getPbad(i);
    }
    er_vcount = vc;
    er_next = 0;
}


// Static function.
// Keep only the errors according to the current errLevel.
//
DRCerrRet *
DRCerrRet::filter(DRCerrRet *thiser)
{
    if (!thiser)
        return (0);
    if (DRC()->errorLevel() == 0) {
        // return one error, the first found
        DRCerrRet::destroy(thiser->er_next);
        thiser->er_next = 0;
    }
    else if (DRC()->errorLevel() == 1) {
        char bits[32];
        memset(bits, 0, 32);
        // return one error of each type
        DRCerrRet *ep = 0, *en;
        for (DRCerrRet *e = thiser; e; e = en) {
            en = e->er_next;
            if (bits[e->er_rule->type()]) {
                ep->er_next = en;
                delete e;
                continue;
            }
            bits[e->er_rule->type()] = 1;
            ep = e;
        }
    }
    return (thiser);
}


namespace {
    // Return the type name of the object.
    //
    const char *
    which_obj(int i)
    {
        switch (i) {
        case CDBOX:
            return ("BOX");
        case CDWIRE:
            return ("WIRE");
        case CDPOLYGON:
            return ("POLYGON");
        case CDLABEL:
            return ("LABEL");
        case CDINSTANCE:
            return ("INSTANCE");
        }
        return ("unknown");
    }
}


// Construct an error string reporting the infraction.
//
char *
DRCerrRet::errmsg(const CDo *odesc)
{
    const char *msg1 =
        "%s violation: %s\n  %s on %s bounded by %.4f,%.4f %.4f,%.4f\n";
    const char *msg2 =
        "  Failed %s\n  in poly %.4f,%.4f %.4f,%.4f %.4f,%.4f %.4f,%.4f\n";
    char *name = er_rule->targetString();

    // Just concatenate the info messages.  The separate messages are
    // maintained for Cadence, where they correspond to different
    // constraints.  Here, they all apply to the same rule.
    sLstr istr;
    for (int i = 0; i < 4; i++) {
        const char *m = er_rule->info(i);
        if (m && *m) {
            if (istr.string())
                istr.add_c(' ');
            istr.add(m);
        }
    }
    const char *info = istr.string();
    if (!info)
        info = "";

    sLstr lstr;
    char buf[1024];
    if (odesc)
        snprintf(buf, 1024, msg1, er_rule->ruleName(), info,
            which_obj(odesc->type()), odesc->ldesc()->name(),
            MICRONS(odesc->oBB().left), MICRONS(odesc->oBB().bottom),
            MICRONS(odesc->oBB().right), MICRONS(odesc->oBB().top));
    else
        snprintf(buf, 1024, "%s violation: %s\n", er_rule->ruleName(), info);
    lstr.add(buf);
    buf[0] = 0;

    switch (er_rule->type()) {
    case drConnected:
        delete [] name;
        name = er_rule->sourceString();
        sprintf(buf, "  disconnected feature on %s.\n", name);
        break;

    case drNoHoles:
        delete [] name;
        name = er_rule->sourceString();
        sprintf(buf, "  region surrounded by %s.\n", name);
        break;

    case drExist:
        delete [] name;
        name = er_rule->sourceString();
        sprintf(buf, "  feature on %s.\n", name);
        break;

    case drOverlap:
        sprintf(buf, "  does not entirely overlap %s.\n", name);
        break;

    case drIfOverlap:
        sprintf(buf, "  partially overlaps %s.\n", name);
        break;

    case drNoOverlap:
        sprintf(buf, "  overlaps feature on %s.\n", name);
        break;

    case drAnyOverlap:
        sprintf(buf, "  does not overlap %s.\n", name);
        break;

    case drPartOverlap:
        sprintf(buf, "  does not partially overlap %s.\n", name);
        break;

    case drAnyNoOverlap:
        sprintf(buf, "  completely overlaps %s.\n", name);
        break;

    case drMinArea:
    case drMaxArea:
        if (er_rule->type() == drMinArea)
            strcpy(buf, "  area less than minimum.\n");
        else
            strcpy(buf, "  area greater than maximum.\n");
        break;

    case drMinEdgeLength:
        strcpy(buf, "  less than minimum edge crossing dimension.\n");
        break;

    case drMaxWidth:
        strcpy(buf, "  more than maximum dimension.\n");
        break;

    case drMinWidth:
        strcpy(buf, "  less than minimum dimension.\n");
        break;

    case drMinSpace:
    case drMinSpaceTo:
        sprintf(buf, "  too close to feature on %s.\n",
            er_rule->type() == drMinSpaceTo ? name : "same layer");
        break;

    case drMinSpaceFrom:
        if (er_errtype == TT_OPP)
            sprintf(buf, "   opposite side %s extension violation.\n", name);
        else if (er_errtype == TT_IFOVL)
            sprintf(buf, "   enclosure not fully covered by %s.\n", name);
        else
            sprintf(buf, "   exterior width of %s too narrow.\n", name);
        break;

    case drMinOverlap:
        sprintf(buf, "  less than minimum overlap with %s.\n", name);
        break;

    case drMinNoOverlap:
        sprintf(buf, "  less than minimum non-overlap with %s.\n", name);
        break;

    case drUserDefinedRule:
        break;
    default:
        return (0);
    }
    delete [] name;
    lstr.add(buf);

    char tbuf[128];
    sprintf(buf, msg2, which_test(tbuf),
        MICRONS(er_pbad[0].x), MICRONS(er_pbad[0].y),
        MICRONS(er_pbad[1].x), MICRONS(er_pbad[1].y),
        MICRONS(er_pbad[2].x), MICRONS(er_pbad[2].y),
        MICRONS(er_pbad[3].x), MICRONS(er_pbad[3].y));
    lstr.add(buf);
    return (lstr.string_trim());
}


// Return the name of the test.  Arg buf is some writing room for the
// user defined tests.
//
const char *
DRCerrRet::which_test(char *buf)
{
    switch (er_errtype) {
    case TT_UNKN:
        return ("unknown test");
    case TT_OVL:
        return ("overlap test");
    case TT_IFOVL:
        return ("partial overlap test");
    case TT_NOOVL:
        return ("non-overlap test");
    case TT_ANYOVL:
        return ("any-overlap test");
    case TT_PARTOVL:
        return ("partial-overlap test");
    case TT_ANOOVL:
        return ("notany-overlap test");
    case TT_AMIN:
        return ("min area test");
    case TT_AMAX:
        return ("max area test");
    case TT_ELT:
        sprintf(buf, "edge length test at edge %d", er_vcount);
        return (buf);
    case TT_EWT:
        sprintf(buf, "edge width test at edge %d", er_vcount);
        return (buf);
    case TT_COT:
        sprintf(buf, "corner overlap test at vertex %d", er_vcount);
        return (buf);
    case TT_CWT:
        sprintf(buf, "corner width test at vertex %d", er_vcount);
        return (buf);
    case TT_EST:
        sprintf(buf, "edge space test at edge %d", er_vcount);
        return (buf);
    case TT_CST:
        sprintf(buf, "corner space test at vertex %d", er_vcount);
        return (buf);
    case TT_EX:
        return ("existence test");
    case TT_CON:
        return ("connectivity test");
    case TT_NOH:
        return ("hole test");
    case TT_OPP:
        return ("opposite side extension test");
    }
    // user defined test?
    int fb = er_errtype & 0xff;
    int sz = 0;
    if (fb >= TT_UET && fb < TT_UCT) {
        sz = fb - TT_UET;
        sprintf(buf, "user edge test at edge %d (vec=", er_vcount);
    }
    else if (fb >= TT_UCT && fb < TT_UOT) {
        sz = fb - TT_UCT;
        sprintf(buf, "user corner test at vertex %d (vec=", er_vcount);
    }
    else if (fb >= TT_UOT && fb < TT_UOT + 25) {
        sz = fb - TT_UOT;
        sprintf(buf, "user corner overlap test at vertex %d (vec=", er_vcount);
    }
    if (sz > 0) {
        char *s = buf + strlen(buf);
        unsigned mask = 0x100;
        for (int j = 0; j < sz; j++) {
            if (er_errtype & mask)
                *s++ = '1';
            else
                *s++ = '0';
            mask <<= 1;
        }
        *s++ = ')';
        *s = 0;
        return (buf);
    }
    return ("unknown test");
}

