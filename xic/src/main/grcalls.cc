
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
 $Id: grcalls.cc,v 5.28 2017/03/14 01:26:46 stevew Exp $
 *========================================================================*/

#include "main.h"
#include "cd_lgen.h"
#include "fio.h"
#include "editif.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "tech.h"
#include "tech_attr_cx.h"
#include "menu.h"


// These implement the GRappCalls interface to the graphics package.
// These provide info to the graphics package, mostly for hard copy
// support.


const char *
cMain::GetPrintCmd()
{
    return (Tech()->HC().command);
}


namespace {
    // Idle proc to execute a script in callback from the help system.
    //
    static int
    script_idle_proc(void *arg)
    {
        char *name = (char*)arg;
        SIfile *sfp;
        stringlist *wl;
        XM()->OpenScript(name, &sfp, &wl);
        if (sfp || wl) {
            // Cur cell can be null here.
            EditIf()->ulListCheck("script", CurCell(), false);
            SI()->Interpret(sfp, wl, 0, 0);
            if (sfp)
                delete sfp;
            EditIf()->ulCommitChanges(true);
        }
        return (false);
    }
}


namespace {
    // Measure the token length, terminated by non-slphanum.  Assume
    // that the first character is accounted for ('$').
    //
    int toklen(const char *str)
    {
        int cnt = 1;
        str++;  // skip '$'
        while (isalnum(*str)) {
            cnt++;
            str++;
        }
        return (cnt);
    }


    // Return true if kw prefixes str, followed by non-alphsnum.
    //
    bool match(const char *str, const char *kw)
    {
        while (*str && *kw) {
            if (*str != *kw)
                return (false);
            str++;
            kw++;
        }
        if (*kw)
            return (false);
        if (isalnum(*str))
            return (false);
        return (true);
    }


    // Replace href with a new version where toklen chars starting at
    // pos are replaced with newtext.
    //
    void repl(char **href, int pos, int toklen, const char *newtext)
    {
        char *t = new char[strlen(*href) - toklen + strlen(newtext) + 1];
        strncpy(t, *href, pos);
        char *e = lstring::stpcpy(t + pos, newtext);
        strcpy(e, *href + pos + toklen);
        delete [] *href;
        *href = t;
    }
}


// This is the callback that processes the '$' expansion for keywords
// passed to the help system.  The argument is either returned or
// freed.
//
char *
cMain::ExpandHelpInput(char *href)
{
    const char *s = strchr(href, '$');
    int maxrepl = 100;
    while (s) {
        // Make sure that we don't get caught in a loop.
        maxrepl--;
        if (!maxrepl)
            break;

        bool found = true;
        int tlen = toklen(s);
        int pos = s - href;
        if (match(s, "$PROGROOT"))
            repl(&href, pos, tlen, XM()->ProgramRoot());
        else if (match(s+1, "EXAMPLES")) {
            sLstr lstr;
            lstr.add(XM()->ProgramRoot());
            lstr.add("/examples");
            repl(&href, pos, tlen, lstr.string());
        }
        else if (match(s+1, "HELP")) {
            sLstr lstr;
            lstr.add(XM()->ProgramRoot());
            lstr.add("/help");
            repl(&href, pos, tlen, lstr.string());
        }
        else if (match(s+1, "DOCS")) {
            sLstr lstr;
            lstr.add(XM()->ProgramRoot());
            lstr.add("/docs");
            repl(&href, pos, tlen, lstr.string());
        }
        else if (match(s+1, "SCRIPTS")) {
            sLstr lstr;
            lstr.add(XM()->ProgramRoot());
            lstr.add("/scripts");
            repl(&href, pos, tlen, lstr.string());
        }
        else {
            found = false;
            // Check for matches to Xic and environment variables.
            //
            char *nm = new char[tlen];
            strncpy(nm, s+1, tlen-1);
            nm[tlen-1] = 0;
            if (nm[0]) {
                const char *vv = CDvdb()->getVariable(nm);
                if (!vv)
                    vv = getenv(nm);
                if (vv) {
                    found = true;
                    repl(&href, pos, tlen, vv);
                }
            }
            delete [] nm;
        }
        if (found)
            s = strchr(href + pos, '$');
        else
            s = strchr(s+1, '$');
    }
    return (href);
}


// Special input token to indicate source type.
#define HELP_NATIVE_TAG     "@XIC"
#define HELP_CHD_TAG        "@CHD"
#define HELP_LIB_TAG        "@LIB"
#define HELP_OA_TAG         "@OA"

// This callback is from the help system, which allows files to be
// opened for editing by clicking anchor text.  If an input file is
// recognized, open the file and return true.  Return false otherwise.
//
bool
cMain::ApplyHelpInput(const char *name)
{
    // The token may have two words, the first being an archive name,
    // the second being a cell name.

    const char *s = name;
    char *word = lstring::gettok(&s);
    if (!word)
        return (false);
    GCarray<char*> gc_word(word);

    const char *e = strrchr(word, '.');
    if (e) {
        if (!strcmp(e, ".scr")) {
            // reference to a script to execute
            dspPkgIf()->RegisterIdleProc(script_idle_proc,
                lstring::copy(name));
            return (true);
        }
        FileType ftype = FIO()->TypeExt(word);
        if (ftype != Fnone) {
            char *cn = lstring::gettok(&s);
            XM()->EditCell(word, false, 0, cn);
            delete [] cn;
            return (true);
        }
    }
    if (!strcasecmp(word, HELP_NATIVE_TAG)) {
        char *cn = lstring::gettok(&s);
        if (!cn)
            return (false);
        XM()->EditCell(cn, false);
        delete [] cn;
        return (true);
    }
    if (!strcasecmp(word, HELP_CHD_TAG) ||
            !strcasecmp(word, HELP_LIB_TAG) ||
            !strcasecmp(word, HELP_OA_TAG)) {
        char *cn1 = lstring::gettok(&s);
        if (!cn1)
            return (false);
        char *cn2 = lstring::gettok(&s);
        XM()->EditCell(cn1, false, 0, cn2);
        delete [] cn1;
        delete [] cn2;
        return (true);
    }
    return (false);
}


// Return a list of pixels that might be used in a rendering.
//
pix_list *
cMain::ListPixels()
{
    int pix, r, g, b;
    pix_list *p0 = 0, *p = 0;
    DisplayMode mode = DSP()->CurMode();
    if (mode == Electrical ||
            !DSP()->MainWdesc()->Attrib()->grid(mode)->show_on_top()) {
        pix = DSP()->Color(FineGridColor);
        GRpkgIf()->MainDev()->RGBofPixel(pix, &r, &g, &b);
        p = p0 = new pix_list(pix, r, g, b);
        pix = DSP()->Color(CoarseGridColor);
        GRpkgIf()->MainDev()->RGBofPixel(pix, &r, &g, &b);
        p->next = new pix_list(pix, r, g, b);
        p = p->next;
    }
    // The list is in order of the presentation, last pixel is on top
    CDl *ld;
    CDlgen lgen(mode);
    while ((ld = lgen.next()) != 0) {
        if (!p0)
            p = p0 = new pix_list(dsp_prm(ld)->pixel(),
                dsp_prm(ld)->red(), dsp_prm(ld)->green(), dsp_prm(ld)->blue());
        else {
            p->next = new pix_list(dsp_prm(ld)->pixel(),
                dsp_prm(ld)->red(), dsp_prm(ld)->green(), dsp_prm(ld)->blue());
            p = p->next;
        }
    }
    pix = DSP()->Color(InstanceBBColor);
    GRpkgIf()->MainDev()->RGBofPixel(pix, &r, &g, &b);
    if (!p0)
        p = p0 = new pix_list(pix, r, g, b);
    else {
        p->next = new pix_list(pix, r, g, b);
        p = p->next;
    }
    pix = DSP()->Color(InstanceNameColor);
    GRpkgIf()->MainDev()->RGBofPixel(pix, &r, &g, &b);
    p->next = new pix_list(pix, r, g, b);
    p = p->next;
    pix = DSP()->Color(InstanceSizeColor);
    GRpkgIf()->MainDev()->RGBofPixel(pix, &r, &g, &b);
    p->next = new pix_list(pix, r, g, b);
    p = p->next;
    if (!(mode == Electrical ||
            !DSP()->MainWdesc()->Attrib()->grid(mode)->show_on_top())) {
        pix = DSP()->Color(FineGridColor);
        GRpkgIf()->MainDev()->RGBofPixel(pix, &r, &g, &b);
        p->next = new pix_list(pix, r, g, b);
        p = p->next;
        pix = DSP()->Color(CoarseGridColor);
        GRpkgIf()->MainDev()->RGBofPixel(pix, &r, &g, &b);
        p->next = new pix_list(pix, r, g, b);
        p = p->next;
    }
    pix = DSP()->Color(MarkerColor);
    GRpkgIf()->MainDev()->RGBofPixel(pix, &r, &g, &b);
    p->next = new pix_list(pix, r, g, b);
    p = p->next;
    pix = DSP()->Color(HighlightingColor);
    GRpkgIf()->MainDev()->RGBofPixel(pix, &r, &g, &b);
    p->next = new pix_list(pix, r, g, b);
    p = p->next;

    return (p0);
}


// Map a pixel value into a layer (pen) number.
//
int
cMain::PixelIndex(int pixel)
{
    int i = 1;
    CDl *ld;
    CDlgen lgen(DSP()->CurMode());
    while ((ld = lgen.next()) != 0) {
        if (pixel == dsp_prm(ld)->pixel())
            return (i);
        i++;
    }
    return (0);
}


// Return the pixel used in the current background.
//
int
cMain::BackgroundPixel()
{
    return (DSP()->Color(BackgroundColor));
}


// Map a line style into a line style index.
//
int
cMain::LineTypeIndex(const GRlineType*)
{
    return (0);
}


// Map a fill type into a fill pattern index.
//
int
cMain::FillTypeIndex(const GRfillType *f)
{
    if (f) {
        int i = 1;
        CDl *ld;
        CDlgen lgen(DSP()->CurMode());
        while ((ld = lgen.next()) != 0) {
            if (f == dsp_prm(ld)->fill())
                return (i);
            i++;
        }
    }
    return (0);
}


int
cMain::FillStyle(int id, int lnum, int *opt1, int *opt2)
{
    if (id == _devHP_) {
        // Fill style with options for layer index, used by the HP-GL
        // plot driver.

        if (opt1)
            *opt1 = 0;
        if (opt2)
            *opt2 = 0;
        if (lnum > 0) {
            CDl *ld = CDldb()->layer(lnum, DSP()->CurMode());
            if (ld) {
                int d = Tech()->HcopyDriver();
                sAttrContext *ac = Tech()->GetAttrContext(d, false);
                if (ac) {
                    sLayerAttr *la = ac->get_layer_attributes(ld, false);
                    if (la) {
                        if (opt1)
                            *opt1 = la->hpgl_fill(1);
                        if (opt2)
                            *opt2 = la->hpgl_fill(2);
                        return (la->hpgl_fill(0));
                    }
                }
            }
        }
    }
    else if (id == _devXF_) {
        // Fill style for layer index, used by the Xfig plot driver.

        // Use of CDl altfill field:
        //  byte 0:  Xfig
        //  byte 1:  unused
        //  byte 2:  unused
        //  byte 3:  unused

        if (lnum > 0) {
            CDl *ld = CDldb()->layer(lnum, DSP()->CurMode());
            if (ld) {
                int d = Tech()->HcopyDriver();
                sAttrContext *ac = Tech()->GetAttrContext(d, false);
                if (ac) {
                    sLayerAttr *la = ac->get_layer_attributes(ld, false);
                    if (la)
                        return (la->misc_fill(0));
                }
            }
        }
    }
    return (0);
}

// In the graphical application menu configuration code, needed for
// Windows only.
//
// bool cMain::MenuItemLocation(int menu_id, int *x, int *y);

// XDraw interface function.  These are in the graphical application code.
//
// void *cMain::SetupLayers(void *dp, GRdraw *cx, void *ptr)
// bool cMain::DrawCallback(void *dp, GRdraw *cx, int l, int b, int r, int t,
//    int w, int h)

