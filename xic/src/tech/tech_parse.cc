
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
#include "si_macro.h"
#include "si_lisp.h"
#include "si_parsenode.h"
#include "si_lspec.h"
#include "errorlog.h"
#include "tech.h"
#include "tech_kwords.h"
#include "tech_drf_in.h"
#include "tech_cds_in.h"
#include "tech_cni_in.h"
#include "tech_attr_cx.h"
#include "tech_via.h"
#include "tech_layer.h"
#include "main_variables.h"
#include "oa_if.h"
#include "miscutil/filestat.h"

#include <unistd.h>

//
// Functions to read a technology file.
//


namespace {
    // These are keywords where the eval(expr) construct is *not* expanded
    // as read.
    //
    const char *SkipEvals[] = {
        "SPICE",
        "CMPUT",
        "MODEL",
        "VALUE",
        "INITC",
        "PARAM",
        0
    };


    inline bool
    do_eval()
    {
        for (const char **s = SkipEvals; *s; s++) {
            if (Tech()->Matching(*s))
                return (false);
        }
        return (true);
    }
}


// Call this just brfore reading the tech file, whether or not a
// techfile is actually read.
//
void
cTech::InitPreParse()
{
    tc_phys_hc_format = tc_hccb.format;
    tc_elec_hc_format = tc_hccb.format;
}


// Call this just after reading the tech file, whether or not a
// techfile is actually read.
//
void
cTech::InitPostParse()
{
    DSPattrib *a = DSP()->MainWdesc()->Attrib();
    tc_default_phys_resol =
        INTERNAL_UNITS(a->grid(Physical)->spacing(Physical));
    tc_default_phys_snap = a->grid(Physical)->snap();

    // Save a backup of the current layer database.
    CDldb()->saveState();

    DSPmainDraw(defineLinestyle(DSP()->BoxLinestyle(),
        DSP()->BoxLinestyle()->mask))

    DSPmainDraw(defineLinestyle(&a->grid(Physical)->linestyle(),
        a->grid(Physical)->linestyle().mask))
    DSPmainDraw(defineLinestyle(&a->grid(Electrical)->linestyle(),
        a->grid(Electrical)->linestyle().mask))

    DSPmainDraw(SetBackground(DSP()->Color(BackgroundColor)))
    DSPmainDraw(SetWindowBackground(DSP()->Color(BackgroundColor)))
    if (DSP()->CurMode() == Physical)
        tc_hccb.format = tc_phys_hc_format;
    else
        tc_hccb.format = tc_elec_hc_format;
    DSPmainDraw(SetGhostColor(DSP()->Color(GhostColor)))
    DSP()->ColorTab()->alloc();

    // Look for active and poly layers.  We need to make sure
    // that the active layer has "Conductor Exclude poly" set,
    // for MOS device handling.

    char buf[128];
    CDl *actv = CDldb()->findLayer("active_layer");
    if (actv && !tech_prm(actv)->exclude()) {
        if (!actv->isConductor())
            actv->setConductor(true);
        CDl *npoly = CDldb()->findLayer("ngate_layer");
        CDl *ppoly = CDldb()->findLayer("pgate_layer");
        if (actv && (npoly || ppoly)) {
            sLspec *excl = new sLspec;
            if (npoly == ppoly)
                strcpy(buf, npoly->name());
            else if (npoly && ppoly) {
                snprintf(buf, sizeof(buf), "%s|%s", npoly->name(),
                    ppoly->name());
            }
            else if (npoly)
                strcpy(buf, npoly->name());
            else
                strcpy(buf, ppoly->name());
            const char *t = buf;
            if (!excl->parseExpr(&t))
                delete excl;
            else
                tech_prm(actv)->set_exclude(excl);
        }
    }
}


// Call this before starting a dispatch loop using GetKeywordLine, if
// the function can be called stand-alone (outside of the Parse
// function).
//
void
cTech::BeginParse()
{
    if (tc_parse_level == 0) {
        tc_comment_cx.clear();

        if (!tc_tech_macros) {
            tc_tech_macros = new SImacroHandler;
            tc_tech_macros->setup_keywords(Tkw.Define(), Tkw.If(),
                Tkw.IfDef(), Tkw.IfNdef(), Tkw.Else(), Tkw.EndIf());
        }

        tc_kwbuf = new char[KEYBUFSIZ];
        tc_inbuf = new char[TECH_BUFSIZE];
        tc_origbuf = new char[TECH_BUFSIZE];
        tc_last_layer = 0;
        SetLineCount(0);
    }
    tc_parse_level++;
}


// Call this after ending a dispatch loop using GetKeywordLine, if
// BeginParse was called.
//
void
cTech::EndParse()
{
    tc_parse_level--;
    if (tc_parse_level == 0) {
        delete [] tc_kwbuf;
        tc_kwbuf = 0;
        delete [] tc_inbuf;
        tc_inbuf = 0;
        delete [] tc_origbuf;
        tc_origbuf = 0;
    }
}


// Open and read the technology file, setting global data structure
// entries as appropriate.
//
void
cTech::Parse(FILE *techfp)
{
    if (tc_techfile_read)
        return;

    BeginParse();

    Errs()->init_error();
    const char *kw;
    while ((kw = GetKeywordLine(techfp)) != 0) {
        TCret tcret = dispatch(techfp);
        if (tcret != TCmatch) {
            Log()->WarningLogV(mh::Techfile, "%s\n", tcret);
            delete [] tcret;
        }
    }
    if (tc_last_layer) {
        TCret tcret = checkLayerFinal(tc_last_layer);
        if (tcret != TCnone) {
            Log()->WarningLogV(mh::Techfile, "%s\n", tcret);
            delete [] tcret;
        }
    }

    // Reset the electrical snap points to a one-micron grid if
    // necessary.  This was not done in older tech files, so there may
    // be be lots of old user files with off-grid connection points. 
    // The grid command can be used to set set a finer grid to repair
    // old files.
    //
    DSPattrib *a = DSP()->MainWdesc()->Attrib();
    double spa = a->grid(Electrical)->spacing(Electrical);
    if (ELEC_INTERNAL_UNITS(spa) % CDelecResolution) {
        a->grid(Electrical)->set_spacing(1.0);
        Log()->WarningLog(mh::Techfile,
            "Electrical snap grid is not a unit multiple, reset to 1.0.\n");
    }

    EndParse();

    tc_techfile_read = true;
}


// Parse and digest a single line directive.  The line has the same
// format as in a tech file, except:
//
//  - There is no macro substitution or other pre-processing.
//  - No line continuation.
//  - Layer block lines must be in the form
//      [elec]layer layername <regular line ...>
//    The layer must already exist, so can't create new layers
//    except cheating with "layer oldlayer layer newlayer".
//
// Most keywords are handled, though print driver block keywords are
// not supported, nor are any blocks that require multiple lines, such
// as Device blocks.
// 
// If the keyword is recognized and the processing went well, true is
// returned.  Otherwise false is returned, with an error message in
// the Errs system.
//
bool
cTech::ParseLine(const char *line)
{
    CDl *layer = 0;
    const char *s = line;
    char *tok = lstring::gettok(&s);
    if (!tok) {
        Errs()->add_error("ParseLine: null or empty string.");
        return (false);
    }
    if (lstring::cieq(tok, "layer") || lstring::cieq(tok, "physlayer")) {
        delete [] tok;
        tok = lstring::gettok(&s);
        if (!tok) {
            Errs()->add_error("ParseLine: missing layer name.");
            return (false);
        }
        layer = CDldb()->findLayer(tok, Physical);
        if (!layer) {
            Errs()->add_error("ParseLine: unknown layer %s.", tok);
            delete [] tok;
            return (false);
        }
        delete [] tok;
        line = s;
    }
    else if (lstring::cieq(tok, "eleclayer")) {
        delete [] tok;
        tok = lstring::gettok(&s);
        if (!tok) {
            Errs()->add_error("ParseLine: missing layer name.");
            return (false);
        }
        layer = CDldb()->findLayer(tok, Electrical);
        if (!layer) {
            Errs()->add_error("ParseLine: unknown layer %s.", tok);
            delete [] tok;
            return (false);
        }
        delete [] tok;
        line = s;
    }
    else
        delete [] tok;

    // We save the state, so that we can be called when Parse is
    // active.  This could happen, e.g., from a techfile script.

    CDl *last_layer_bak = tc_last_layer;
    tc_last_layer = layer;
    int line_count_bak = LineCount();
    SetLineCount(1);
    bool lnum_bak = tc_no_line_num;
    tc_no_line_num = true;

    // Grab and save the keyword.
    s = line;
    char *kw = lstring::gettok(&s);
    // Convert kw to upper case.
    for (char *t = kw; *t; t++) {
        if (islower(*t))
            *t = toupper(*t);
    }

    // Save the rest of the line, strip trailing white space. 
    // Continuation is not supported here.

    char *inbuf = lstring::copy(s);
    char *t = inbuf + strlen(inbuf) - 1;
    while (t >= inbuf && isspace(*t))
        *t-- = 0;

    char *kwbuf_bak = tc_kwbuf;
    tc_kwbuf = kw;
    char *inbuf_bak = tc_inbuf;
    tc_inbuf = inbuf;
    char *origbuf_bak = tc_origbuf;
    tc_origbuf = 0;

    TCret tcret = dispatch(0);
    if (tcret != TCmatch) {
        Errs()->add_error(tcret);
        delete [] tcret;
    }

    delete [] tc_kwbuf;
    tc_kwbuf = kwbuf_bak;
    delete [] tc_inbuf;
    tc_inbuf = inbuf_bak;
    delete [] tc_origbuf;
    tc_origbuf = origbuf_bak;

    tc_last_layer = last_layer_bak;
    SetLineCount(line_count_bak);
    tc_no_line_num = lnum_bak;

    return (tcret == TCmatch);
}


// Read a logical line of input.  In normal reading mode, the keyword
// is placed in tc_kwbuf, and a pointer to this is returned.  The
// rest of the line goes to tc_inbuf.  If the line is changed due to
// a substitution, and tc_origbuf is not 0, the original line is
// copied to tc_origbuf.  If not 0, origbuf contains the empty string
// if no substitution.
//
// If the read mode is tNoKeyword or tVerbatim, the keyword is not
// separated out, and a pointer to an empty string is returned.  All
// text goes to tc_inbuf.  In all cases, backslash continuation is
// performed.  In tNoKeyword mode, macro and variable expansion is
// done.  This is skipped in tVerbatim mode.
//
// Null is returned on EOF.
//
const char *
cTech::GetKeywordLine(FILE *techfp, tReadMode rmode)
{
    *tc_kwbuf = 0;
    *tc_inbuf = 0;
    if (!techfp)
        return (0);

    for (;;) {
        if (!getLineFromFile(techfp, rmode))
            return (0);
        IncLineCount();
        if (!ExpandLine(techfp, rmode))
            break;
    }
    if (rmode == tReadNormal)
        SetCmtKey(tc_kwbuf);
    return (tc_kwbuf);
}


// Look for a trailing negation and return false if found, otherwise
// return true.
//
bool
cTech::GetBoolean(const char *buf)
{
    while (isspace(*buf)) buf++;
    if (*buf == '0')
        return (false);
    if (*buf == 'n' || *buf == 'N')
        return (false);
    if (*buf == 'f' || *buf == 'F')
        return (false);
    if ((buf[0] == 'o' || buf[0] == 'O') && (buf[1] == 'f' || buf[1] == 'F'))
        return (false);
    return (true);
}


// Scan buf for an integer.  Return 0 if non-positive or error.
// Return 1 if 'y'.
//
int
cTech::GetInt(const char *buf)
{
    char *end;
    // Use strtoul instead of strtol so that "0xffffffff" is read as -1,
    // strtol will read this as 0x7fffffff.
    int d = strtoul(buf, &end, 0);
    if (end == buf) {
        while (isspace(*buf))
            buf++;
        if (*buf == 'y')
            d = 1;
    }
    return (d);
}


bool
cTech::GetWord(const char **s, char **tok)
{
    if (tok)
        *tok = 0;
    if (s == 0 || *s == 0)
        return (false);
    while (isspace(**s))
        (*s)++;
    if (!**s)
        return (false);
    const char *st = *s;
    while (**s && !isspace(**s))
        (*s)++;
    if (tok) {
        *tok = new char[*s - st + 1];
        char *c = *tok;
        while (st < *s)
            *c++ = *st++;
        *c = 0;
    }
    while (isspace(**s))
        (*s)++;
    return (true);
}


// Parse a color and return the rgb values.
//
bool
cTech::GetRgb(int *rgb)
{
    int r, g, b;
    if (sscanf(tc_inbuf, "%d %d %d", &r, &g, &b) == 3 && r >= 0 && r <= 255 &&
            g >= 0 && g <= 255 && b >= 0 && b <= 255) {
        rgb[0] = r;
        rgb[1] = g;
        rgb[2] = b;
        return (true);
    }
    char *t = tc_inbuf + strlen(tc_inbuf) - 1;
    while (isspace(*t) && t >= tc_inbuf)
        *t-- = 0;
    if (!DSPpkg::self()->NameToRGB(tc_inbuf, rgb)) {
        rgb[0] = rgb[1] = rgb[2] = 0;
        return (false);
    }
    return (true);
}


// Set the filled status of the layer being defined.  Options are
//   Y
//   N  [OFC]
//   <fillpattern> [YOC]
//
// After 'N' or a fillpattern, the remainder of the line is checked
// for option characters.  Other characters are silently ignored. 
// Everything is case-insensitive.  Options are
//   following 'N'
//     O  Use dotted lines
//     F  Use fat lines for outline
//     C  Draw diagonals across boxes.
//   following <pattern>
//     O  Draw outline
//     Y  Draw outline, same as O
//     F  Use fat lines for outline
//     C  Draw diagonals across boxes.
//
// The template arg is a CDl or sLayerAttr.
//
bool
cTech::GetFilled(CDl *lastlayer)
{
    const char *bp = tc_inbuf;
    while (isspace(*bp))
        bp++;
    lastlayer->setFilled(false);
    lastlayer->setOutlined(false);
    lastlayer->setOutlinedFat(false);
    lastlayer->setCut(false);
    DSPmainDraw(defineFillpattern(dsp_prm(lastlayer)->fill(), 0, 0, 0))
    if (!*bp || *bp == 'y' || *bp == 'Y') {
        lastlayer->setFilled(true);
        return (true);
    }
    if (*bp == 'n' || *bp == 'N') {
        lastlayer->setFilled(false);
        while (isalpha(*bp))
            bp++;
        while (isspace(*bp))
            bp++;
    }
    else {
        lastlayer->setFilled(true);
        int nx, ny;
        unsigned char array[128]; // 32x32 max size
        if (!GetPmap(&bp, array, &nx, &ny)) {
            // There is a message in Errs.
            return (false);
        }
        DSPmainDraw(defineFillpattern(dsp_prm(lastlayer)->fill(), nx, ny,
            array))
    }
    while (*bp) {
        if (*bp == 'y' || *bp == 'Y' || *bp == 'o' || *bp == 'O')
            lastlayer->setOutlined(true);
        else if (*bp == 'f' || *bp == 'F') {
            lastlayer->setOutlined(true);
            lastlayer->setOutlinedFat(true);
        }
        else if (*bp == 'c' || *bp == 'C')
            lastlayer->setCut(true);
        bp++;
    }
    return (true);
}


// As above, but specialized to sLayerAttr for tech_hardcopy.
//
bool
cTech::GetFilled(sLayerAttr *la)
{
    const char *bp = tc_inbuf;
    while (isspace(*bp))
        bp++;
    la->setFilled(false);
    la->setOutlined(false);
    la->setOutlinedFat(false);
    la->setCut(false);
    DSPmainDraw(defineFillpattern(la->fill(), 0, 0, 0))
    if (!*bp || *bp == 'y' || *bp == 'Y') {
        la->setFilled(true);
        return (true);
    }
    if (*bp == 'n' || *bp == 'N') {
        while (isalpha(*bp))
            bp++;
        while (isspace(*bp))
            bp++;
    }
    else {
        la->setFilled(true);
        int nx, ny;
        unsigned char array[128]; // 32x32 max size
        if (!GetPmap(&bp, array, &nx, &ny)) {
            // There is a message in Errs.
            return(false);
        }
        DSPmainDraw(defineFillpattern(la->fill(), nx, ny, array))
    }

    while (*bp) {
        if (*bp == 'y' || *bp == 'Y' || *bp == 'o' || *bp == 'O')
            la->setOutlined(true);
        else if (*bp == 'f' || *bp == 'F') {
            la->setOutlined(true);
            la->setOutlinedFat(true);
        }
        else if (*bp == 'c' || *bp == 'C')
            la->setCut(true);
        bp++;
    }
    return (true);
}


namespace {
    void get_bytes(unsigned char *a, const char *s, int n)
    {
        unsigned int d;
        if (*s == '|') {
            s++;
            d = 0;
            unsigned mask = 1;
            for (int i = 0; i < n; i++) {
                if (!isspace(*s))
                    d |= mask;
                s++;
                mask <<= 1;
            }
        }
        else
            sscanf(s, "%x", &d);

        n = (n+7)/8;
        *a++ = d;
        for (int j = 1; j < n; j++)
            *a++ = d >> 8*j;
    }

    bool ishex(const char *s)
    {
        if (*s == '0' && *(s+1) == 'x')
            s += 2;
        if (!*s)
            return (false);
        while (*s) {
            if (!isxdigit(*s))
                return (false);
            s++;
        }
        return (true);
    }
}


// Function to parse the pixel map specification.  There are several
// possible formats
//
//   N two-digit hex integers   (8xN array)
//   N four-digit hex integers  (16xN array)
//   |...| tokens 
//      Each token has the same number of chars between | |,
//      non-space chars represent a drawn pixel, space chars non-drawn.
//      The map size is the number of chars per token (x) and the
//      number of tokens (y).
//   [x=nx] [y=ny] list of hex numbers
//      The x/y sizes can be explicitly specified.  No space allowed
//      around the '=' charcter.
//   (...)
//      Anything enclosed in parentheses is ignored.
//
// On error, false is returned, with a message in the Errs system.
//
bool
cTech::GetPmap(const char **string, unsigned char *array, int *nx, int *ny)
{
    int setX = 0;   // x=NX was given
    int setY = 0;   // y=NY was given
    int pxlen = 0;  // char count inside |...|
    int pxcnt = 0;  // count of |...| tokens
    int hxlen = 0;  // digits in hex numbers
    int hxcnt = 0;  // count of hex numbers

    unsigned char *a = array;

    char buf[256];
    const char *s = *string;
    while (*s) {
        while (isspace(*s))
            s++;
        const char *lastpos = s;
        char *bp = buf;
        if (*s == '|') {
            *bp++ = *s++;
            while (*s && *s != '|') {
                *bp++ = *s++;
                if (bp - buf > 33) {
                    Errs()->add_error("Too many characters inside | |.");
                    return (false);;
                }
            }
            if (!*s) {
                Errs()->add_error("Missing trailing '|'.");
                return (false);;
            }
            *bp++ = *s++;
            *bp = 0;
        }
        if (*s == '(') {
            s++;
            while (*s && *s != ')')
                s++;
            if (*s == ')')
                s++;
            continue;
        }
        else {
            while (*s && !isspace(*s)) {
                *bp++ = *s++;
                if (bp - buf > 32) {
                    Errs()->add_error("Token length too long.");
                    return (false);
                }
            }
            *bp = 0;
        }

        // Make lower-case.
        bp = buf;
        while (*bp) {
            if (isupper(*bp))
                *bp = tolower(*bp);
            bp++;
        }
        bp = buf;

        if (*bp == 'x') {
            if (pxcnt || hxcnt) {
                Errs()->add_error(
                    "Syntax error, map token found before 'X='.");
                return (false);
            }
            bp++;
            if (*bp == '=')
                bp++;
            int d;
            if (sscanf(bp, "%d", &d) != 1 || d < 2 || d > 32) {
                Errs()->add_error("Syntax or value error following 'X='.");
                return (false);
            }
            if (setX) {
                Errs()->add_error("'X=' already given.");
                return (false);
            }
            setX = d;
        }
        else if (*bp == 'y') {
            if (pxcnt || hxcnt) {
                Errs()->add_error(
                    "Syntax error, map token found before 'Y='.");
                return (false);
            }
            bp++;
            if (*bp == '=')
                bp++;
            int d;
            if (sscanf(bp, "%d", &d) != 1 || d < 2 || d > 32) {
                Errs()->add_error("Syntax or value error following 'Y='.");
                return (false);
            }
            if (setY) {
                Errs()->add_error("'Y=' already given.");
                return (false);
            }
            setY = d;
        }
        else if (*bp == '|') {
            int len = strlen(bp) - 2;
            if (len < 2 || len > 32) {
                Errs()->add_error("Too few or many chars in | |.");
                return (false);
            }
            if (pxlen > 0 && pxlen != len) {
                Errs()->add_error("Inconsistent char count in | |.");
                return (false);
            }
            if (hxcnt) {
                Errs()->add_error("Mixing | | and hex not allowed.");
                return (false);
            }
            if (setX > 0 && setX != len) {
                Errs()->add_error("Inconsistent char count in | | with 'X='.");
                return (false);
            }
            pxlen = len;
            pxcnt++;

            // Save row.
            get_bytes(a, bp, pxlen);
            a += (pxlen+7)/8;

            if (setY > 0 && pxcnt == setY)
                break;
        }
        else if (ishex(bp)) {
            // Hack: avoid misreading "c" which may actually be the
            // cut option specifier that can follow the map.
            if (*bp == 'c' && !*(bp+1)) {
                if (pxcnt || (setX == 0 && (hxcnt == 8 || hxcnt == 16))) {
                    s = lastpos;
                    break;
                }
            }

            if (pxcnt) {
                Errs()->add_error("Mixing | | and hex not allowed.");
                return (false);
            }
            if (*bp == '0' && *(bp+1) == 'x')
                bp += 2;
            if (!setX) {
                int len = strlen(bp);
                if (len & 1)
                    len++;
                if (len > 4) {
                    Errs()->add_error("Too many hex digits.");
                    return (false);
                }
                if (hxlen > 0 && hxlen != len) {
                    Errs()->add_error(
                        "Inconsistent digit count in hex numbers .");
                    return (false);
                }
                hxlen = len;
            }
            hxcnt++;

            // Save row.
            int n = setX ? setX : hxlen*4;
            get_bytes(a, bp, n);
            a += (n+7)/8;

            if (setY > 0 && hxcnt == setY)
                break;
        }
        else {
            s = lastpos;
            break;
        }
    }

    if (setY) {
        if (pxcnt > 0 && pxcnt != setY) {
            Errs()->add_error("Too few | | tokens given.");
            return (false);
        }
        if (hxcnt > 0 && hxcnt != setY) {
            Errs()->add_error("Too few hex digits given.");
            return (false);
        }
    }
    if (setX)
        *nx = setX;
    else if (pxlen)
        *nx = pxlen;
    else
        *nx = hxlen*4;
    if (setY)
        *ny = setY;
    else if (pxcnt)
        *ny = pxcnt;
    else
        *ny = hxcnt;
    *string = s;
    return (true);
}


void
cTech::Comment(const char *t)
{
    sTcomment *c = new sTcomment(new sTcx(tc_comment_cx), lstring::copy(t));
    if (!tc_comments)
        tc_comments = tc_cmts_end = c;
    else {
        tc_cmts_end->next = c;
        tc_cmts_end = tc_cmts_end->next;
    }
}


int
cTech::LineCount() const
{
    if (tc_tech_macros)
        return (tc_tech_macros->line_number());
    return (0);
}


void
cTech::SetLineCount(int c)
{
    if (tc_tech_macros)
        tc_tech_macros->set_line_number(c);
}


void
cTech::IncLineCount()
{
    if (tc_tech_macros)
        tc_tech_macros->inc_line_number();
}


// Create an error message string according to the format and
// arguments, If tc_no_line_num is not set, a valid line number is
// assumed and will be included.
//
char *
cTech::SaveError(const char *fmt, ...) const
{
    va_list args;
    char buf[1024];
    buf[0] = 0;
    int line = tc_no_line_num ? -1 : LineCount();
    if (line > 0)
        snprintf(buf, sizeof(buf), "(Line %d) ", line);
    int ofs = strlen(buf);

    va_start(args, fmt);
    vsnprintf(buf+ofs, 1024-ofs, fmt, args);
    va_end(args);
    return (lstring::copy(buf));
}


// Do the variable substitutions and macro expansion of the text
// buffer.  If techfp, were parsing multi-line input, so include
// multi-line macro handling and line numbers in error messages.
//
bool
cTech::ExpandLine(FILE *techfp, tReadMode rmode)
{
    const char *kw = tc_kwbuf;

    if (rmode != tReadNormal) {
        bool copy_orig = false;
        if (rmode != tVerbatim) {
            // macro expand
            if (tc_tech_macros) {
                char *s = tc_tech_macros->macro_expand(tc_inbuf, &copy_orig);
                if (s) {
                    strcpy(tc_inbuf, s);
                    delete [] s;
                }
                else
                    *tc_inbuf = 0;
            }

            // Do the variable and eval() substitutions.
            int numsub;
            char *emesg;
            if (VarSubst(tc_inbuf, &emesg, &numsub)) {
                if (numsub > 0)
                    copy_orig = true;
            }
            else {
                char *e = SaveError("Variable substitution: %s.", emesg);
                Log()->WarningLogV(mh::Techfile, "%s\n", e);
                delete [] e;
                delete [] emesg;
            }
            if (EvaluateEval(tc_inbuf, &numsub)) {
                if (numsub > 0)
                    copy_orig = true;
            }
            else {
                char *e = SaveError("Expression evaluation error.");
                Log()->WarningLogV(mh::Techfile, "%s\n", e);
                delete [] e;
            }
        }
        if (!copy_orig && tc_origbuf)
            *tc_origbuf = '\0';
    }
    else {
        // Set lines are not macro expanded.
        if (Matching(Tkw.Set())) {
            SetCmtKey(kw);
            if (!vset(tc_inbuf)) {
                char *e = SaveError("%s: syntax error.", Tkw.Set());
                Log()->WarningLogV(mh::Techfile, "%s\n", e);
                delete [] e;
            }
            return (true);
        }

        if (techfp) {
            char *emsg;
            if (tc_tech_macros->handle_keyword(techfp, tc_inbuf, kw, 0,
                    &emsg)) {
                if (emsg) {
                    char *e = SaveError("Macro handling error: %s", emsg);
                    Log()->WarningLogV(mh::Techfile, "%s\n", e);
                    delete [] e;
                    delete [] emsg;
                }
                if (Matching(Tkw.Define()))
                    SetCmtKey(kw);
                return (true);
            }
        }

        bool copy_orig = false;
        // Macro expand.
        if (tc_tech_macros) {
            char *s = tc_tech_macros->macro_expand(tc_inbuf, &copy_orig);
            if (s) {
                strcpy(tc_inbuf, s);
                delete [] s;
            }
            else {
                *tc_inbuf = 0;
                return (true);
            }
        }

        // Do the variable and eval() substitutions.
        int numsub;
        char *emesg;
        if (VarSubst(tc_inbuf, &emesg, &numsub)) {
            if (numsub > 0)
                copy_orig = true;
        }
        else {
            char *e = SaveError("Variable substitution: %s.", emesg);
            Log()->WarningLogV(mh::Techfile, "%s\n", e);
            delete [] e;
            delete [] emesg;
        }
        if (do_eval()) {
            if (EvaluateEval(tc_inbuf, &numsub)) {
                if (numsub > 0)
                    copy_orig = true;
            }
            else {
                char *e = SaveError("Expression evaluation error.");
                Log()->WarningLogV(mh::Techfile, "%s\n", e);
                delete [] e;
            }
        }
        if (!copy_orig && tc_origbuf)
            *tc_origbuf = '\0';

        if (Matching("!Set")) {
            SetCmtKey(kw);
            if (!bangvset(tc_inbuf)) {
                char *e = SaveError("%s: syntax error.", "!Set");
                Log()->WarningLogV(mh::Techfile, "%s\n", e);
                delete [] e;
            }
            else {
                stringlist *s = new stringlist(lstring::copy(tc_inbuf), 0);
                if (!tc_bangset_lines)
                    tc_bangset_lines = s;
                else {
                    stringlist *sl = tc_bangset_lines;
                    while (sl->next)
                        sl = sl->next;
                    sl->next = s;
                }
            }
            return (true);
        }
        if (Matching(Tkw.Comment())) {
            Comment(tc_inbuf);
            SetCmtKey(kw);
            return (true);
        }
    }
    return (false);
}


// This is the top-level dispatch function.  This is called from
// Parse (when reading from a file) as well as ParseLine (when
// reading from a string.  In the latter case, techfp will be null.
//
// Below, and in the sub-functions, whether or not techfp is null
// turns on/off multi-line capability and line numbers in error
// messages.
//
// techfp not null:
//   It is assumed that we are reading a file through techfp.  Multi-
//   line blocks are handled, by use of a local dispatch function
//   looping with GetKeywordLine.  Error messages composed with
//   SaveError will contain the line number.  It is OK to call
//   Log()->warning.
//
// techfp null:
//   It is assumed that we are reading a single line of input, in the
//   manner of ParseLine.  Multi-line constructs are not handled, and
//   will probably cause "keyword not found" errors.  The SaveError
//   function composes error messages without the line number.  All
//   error text is passed up the hierarchy via the TCret returns, there
//   is (or should be) no printed error output until the caller
//   processes the return.  This isn't absolute, for example the
//   ReadDRF and similar keys dump messages to the log.
//
TCret
cTech::dispatch(FILE *techfp)
{
    if (!tc_kwbuf || !tc_inbuf)
        return (lstring::copy("dispatch:  unallocated buffer."));

    // Cadence and Ciranova DRF/Techfile and layer map inclusions.
    //
    if (Matching(Tkw.LispLogging())) {
        cLispEnv::set_logging(GetBoolean(tc_inbuf));
        return (TCmatch);
    }
    if (Matching(Tkw.ReadDRF())) {
        char *err;
        if (!DrfIn()->read(tc_inbuf, &err)) {
            char *e = SaveError("%s: %s", Tkw.ReadDRF(),
                err ? err : "unknown error");
            delete [] err;
            return (e);
        }
        return (TCmatch);
    }
    if (Matching(Tkw.ReadOaTech())) {
        if (OAif()->hasOA()) {
            char *techtemp = filestat::make_temp("xi");
            FILE *fp = fopen(techtemp, "w");
            if (fp) {
                OAif()->print_tech(fp, tc_inbuf, 0, 0);
                char *err;
                bool ret = CdsIn()->read(techtemp, &err);
                fclose(fp);
                unlink(techtemp);
                delete [] techtemp;
                if (!ret) {
                    char *e = SaveError("%s: %s", Tkw.ReadOaTech(),
                        err ? err : "unknown error");
                    delete [] err;
                    return (e);
                }
            }
            else {
                return (SaveError("%s: failed to create temporary file.",
                    Tkw.ReadOaTech()));
            }
        }
        else {
            return (SaveError("%s: OpenAccess is not available.",
                Tkw.ReadOaTech()));
        }
        return (TCmatch);
    }
    if (Matching(Tkw.ReadCdsTech())) {
        char *err;
        if (!CdsIn()->read(tc_inbuf, &err)) {
            char *e = SaveError("%s: %s", Tkw.ReadCdsTech(),
                err ? err : "unknown error");
            delete [] err;
            return (e);
        }
        return (TCmatch);
    }
    if (Matching(Tkw.ReadCdsLmap())) {
        if (!cTechCdsIn::readLayerMap(tc_inbuf)) {
            const char *err = Errs()->get_error();
            return (SaveError("%s: %s", Tkw.ReadCdsLmap(),
                err ? err : "unknown error"));
        }
        return (TCmatch);
    }
    if (Matching(Tkw.ReadCniTech())) {
        char *err;
        if (!CniIn()->read(tc_inbuf, &err)) {
            char *e = SaveError("%s: %s", Tkw.ReadCniTech(),
                err ? err : "unknown error");
            delete [] err;
            return (e);
        }
        return (TCmatch);
    }

    // Keys usually appearing before layer definitions.

    // Set one of the four definable search paths:
    // Path        the symbol search path
    // LibPath     the startup file and library search path
    // HelpPath    path for the help database
    // ScriptPath  path to the user-generated executable scripts
    // These are electrical/physical independent.
    //
    if (Matching(Tkw.Path())) {
        CDvdb()->setVariable(VA_Path, tc_inbuf);
        return (TCmatch);
    }
    if (Matching(Tkw.LibPath())) {
        CDvdb()->setVariable(VA_LibPath, tc_inbuf);
        return (TCmatch);
    }
    if (Matching(Tkw.HelpPath())) {
        CDvdb()->setVariable(VA_HelpPath, tc_inbuf);
        return (TCmatch);
    }
    if (Matching(Tkw.ScriptPath())) {
        CDvdb()->setVariable(VA_ScriptPath, tc_inbuf);
        return (TCmatch);
    }
    if (Matching(Tkw.Technology())) {
        char *tt;
        const char *bp = tc_inbuf;
        if (GetWord(&bp, &tt)) {
            delete [] tc_technology_name;
            tc_technology_name = tt;
        }
        return (TCmatch);
    }
    if (Matching(Tkw.Vendor())) {
        char *tt;
        const char *bp = tc_inbuf;
        if (GetWord(&bp, &tt)) {
            delete [] tc_vendor_name;
            tc_vendor_name = tt;
        }
        return (TCmatch);
    }
    if (Matching(Tkw.Process())) {
        char *tt;
        const char *bp = tc_inbuf;
        if (GetWord(&bp, &tt)) {
            delete [] tc_process_name;
            tc_process_name = tt;
        }
        return (TCmatch);
    }

    // Layer Aliases.
    if (Matching(Tkw.MapLayer())) {
        char *alias;
        const char *bp = tc_inbuf;
        if (GetWord(&bp, &alias)) {
            char *lname;
            if (GetWord(&bp, &lname)) {
                CDldb()->saveAlias(alias, lname);
                delete [] lname;
                delete [] alias;
            }
            else {
                char *e = SaveError("%s: missing layer name for %s.",
                    Tkw.MapLayer(), alias);
                delete [] alias;
                return (e);
            }
        }
        else
            return (SaveError("%s: missing alias name.", Tkw.MapLayer()));
        return (TCmatch);
    }

    // The layer and purpose mappings.
    if (Matching(Tkw.DefineLayer())) {
        char *lname;
        const char *bp = tc_inbuf;;
        if (GetWord(&bp, &lname)) {
            char *end;
            unsigned int lnum = strtoul(bp, &end, 0);
            if (end > bp) {
                if (!CDldb()->saveOAlayer(lname, lnum)) {
                    return (SaveError(
                        "%s: failed to define layer name/number %s/%d, "
                        "name or number clash?", Tkw.DefineLayer(),
                        lname, lnum));
                }
            }
            else {
                return (SaveError("%s: bad or missing layer number for %s.",
                    Tkw.DefineLayer(), lname));
            }
            delete [] lname;
        }
        else
            return (SaveError("%s: layer name missing.", Tkw.DefineLayer()));
        return (TCmatch);
    }
    if (Matching(Tkw.DefinePurpose())) {
        char *pname;
        const char *bp = tc_inbuf;
        if (GetWord(&bp, &pname)) {
            char *end;
            unsigned int pnum = strtoul(bp, &end, 0);
            if (end > bp) {
                if (!CDldb()->saveOApurpose(pname, pnum)) {
                    return (SaveError(
                        "%s: failed to define purpose name/number %s/%d, "
                        "name or number clash?", Tkw.DefinePurpose(),
                        pname, pnum));
                }
            }
            else {
                return (SaveError(
                    "%s: bad or missing purpose number for %s.",
                    Tkw.DefinePurpose(), pname));
            }
            delete [] pname;
        }
        else {
            return (SaveError("%s: purpose name missing.",
                Tkw.DefinePurpose()));
        }
        return (TCmatch);
    }
    // End of preamble keys

    if (tc_parse_user_rule && techfp) {
        TCret tcret = (*tc_parse_user_rule)(techfp);
        if (tcret != TCnone)
            return (tcret);
    }

    // Layer block keywords.
    {
        TCret tcret = dispatchLayerBlock();
        if (tcret != TCnone)
            return (tcret);
    }

    // Device blocks
    if (tc_parse_device && techfp) {
        TCret tcret = (*tc_parse_device)(techfp);
        if (tcret != TCnone)
            return (tcret);
    }

    // Scripts
    if (tc_parse_script && techfp) {
        TCret tcret = (*tc_parse_script)(techfp);
        if (tcret != TCnone)
            return (tcret);
    }

    // Check attributes.
    {
        TCret tcret = ParseAttributes();
        if (tcret != TCnone)
            return (tcret);
    }

    // Standard vias.
    if (Matching(Tkw.StandardVia())) {
        TCret tcret = sStdVia::tech_parse(tc_inbuf);
        if (tcret != TCmatch) {
            char *e = SaveError("%s: %s.", Tkw.StandardVia(), tcret);
            delete [] tcret;
            tcret = e;
        }
        return (tcret);
    }

    // Hardcopy keywords and drivers.
    {
        TCret tcret = ParseHardcopy(techfp);
        if (tcret != TCnone)
            return (tcret);
    }

    return (SaveError("Unknown keyword %s (ignored).", tc_kwbuf));
}


namespace {
    TCret cat_ret(TCret tc1, char *s)
    {
        if (tc1 == TCnone)
            return (s);
        char *e = tc1 + strlen(tc1) - 1;
        char *tn = new char[strlen(tc1) + strlen(s) + 2];
        char *p = lstring::stpcpy(tn, tc1);
        if (*e != '\n')
           *p++ = '\n';
        strcpy(p, s);
        delete [] tc1;
        delete [] s;
        return (tn);
    }
}


// Handle the layer block keywords.  The techfp if nonzero only
// indicates that the line number is valid, all constructs here are
// single-line.
//
TCret
cTech::dispatchLayerBlock()
{
    if (!tc_kwbuf || !tc_inbuf)
        return (SaveError("dispatchLayerBlock: unallocated buffer."));

    const char *lmsg = "Layer %s: Bad %s specification.";

    int rgb[3];  // color buffer
    const char *kw = tc_kwbuf;

    // Keywords for layer definitions
    if (Matching(Tkw.ElecLayerName()) || Matching(Tkw.ElecLayer())) {
        // Start a definition of an electrical layer.  The layer
        // named "SCED" is the active layer.

        // If this comes back dirty, deal with it after processing.
        TCret tcret = TCnone;
        if (tc_last_layer)
            tcret = checkLayerFinal(tc_last_layer);

        tc_last_layer = CDldb()->findLayer(tc_inbuf, Electrical);
        if (!tc_last_layer) {
            tc_last_layer =
                CDldb()->addNewLayer(tc_inbuf, Electrical, CDLnormal, -1);
            if (tc_last_layer && !strcmp(tc_last_layer->name(), "SCED")) {
                // The SCED layer fills polys by default, other layers
                // default to outline.
                tc_last_layer->setFilled(true);
            }
        }
        if (!tc_last_layer) {
            char *e = SaveError(
                "Failed to find or create layer named %s.", tc_inbuf);
            return (cat_ret(tcret, e));
        }
        else {
            SetCmtType(tBlkElyr, tc_last_layer->name());
            if (strcasecmp(tc_last_layer->name(), tc_inbuf)) {
                char *e = SaveError(
                    "Using %s as name for layer %s due to conflicts.",
                    tc_last_layer->name(), tc_inbuf);
                return (cat_ret(tcret, e));
            }
        }
        if (tcret != TCnone)
            return (tcret);
        return (TCmatch);
    }
    if (Matching(Tkw.PhysLayer()) || Matching(Tkw.LayerName()) ||
            Matching(Tkw.PhysLayerName()) || Matching(Tkw.Layer())) {
        // Start a definition of a physical layer.

        // If this comes back dirty, deal with it after processing.
        TCret tcret = TCnone;
        if (tc_last_layer)
            tcret = checkLayerFinal(tc_last_layer);

        tc_last_layer = CDldb()->findLayer(tc_inbuf, Physical);
        if (!tc_last_layer) {
            tc_last_layer =
                CDldb()->addNewLayer(tc_inbuf, Physical, CDLnormal, -1);
        }
        if (!tc_last_layer) {
            char *e = SaveError(
                "Failed to find or create layer named %s.", tc_inbuf);
            return (cat_ret(tcret, e));
        }
        else {
            SetCmtType(tBlkPlyr, tc_last_layer->name());
            if (strcasecmp(tc_last_layer->name(), tc_inbuf)) {
                char *e = SaveError(
                    "Using %s as name for layer %s due to conflicts.",
                    tc_last_layer->name(), tc_inbuf);
                return (cat_ret(tcret, e));
            }
        }
        if (tcret != TCnone)
            return (tcret);
        return (TCmatch);
    }
    if (Matching(Tkw.Derived()) || Matching(Tkw.DerivedLayer())) {
        // Define a derived layer.  This terminates a physical
        // layer definition.

        // If this comes back dirty, deal with it after processing.
        TCret tcret = TCnone;
        if (tc_last_layer)
            tcret = checkLayerFinal(tc_last_layer);

        tc_last_layer = 0;
        const char *ib = tc_inbuf;
        char *lname = lstring::gettok(&ib);
        if (!lname) {
            char *e = SaveError("Unnamed derived layer.");
            return (cat_ret(tcret, e));
        }

        int mode = CLdefault;
        const char *bak = ib; 
        char *tok = lstring::gettok(&ib);
        if (lstring::cieq(tok, "join"))
            mode = CLjoin;
        else if (lstring::cieq(tok, "split") || lstring::cieq(tok, "splith"))
            mode = CLsplitH;
        else if (lstring::cieq(tok, "splitv"))
            mode = CLsplitV;
        else
            ib = bak;
        delete [] tok;

        char *t = tc_inbuf + strlen(tc_inbuf) - 1;
        while (t >= ib && isspace(*t))
            *t-- = 0;
        if (!*ib) {
            char *e = SaveError( "Derived layer %s has no expression.",
                lname);
            return (cat_ret(tcret, e));
        }
        tc_last_layer = CDldb()->addDerivedLayer(lname, -1, mode, ib);
        delete [] lname;
        if (tcret != TCnone)
            return (tcret);
        return (TCmatch);
    }
    if (Matching(Tkw.LppName()) || Matching(Tkw.LongName())) {
        // Set the LPP name of the current layer.
        // Take the obsolete "LongName" keyword as LppName.
        TCret tcret = CheckLD();
        if (tcret != TCnone)
            return (tcret);
        if (!CDldb()->setLPPname(tc_last_layer, tc_inbuf)) {
            return (SaveError(
                "Failed to set LPP name for layer %s, "
                "name already in use?", tc_last_layer->name()));
        }
        return (TCmatch);
    }
    if (Matching(Tkw.Description())) {
        // Set the description of the current layer
        TCret tcret = CheckLD();
        if (tcret != TCnone)
            return (tcret);
        char *t = tc_inbuf + strlen(tc_inbuf) - 1;
        while (t >= tc_inbuf && isspace(*t))
            *t-- = 0;
        if (*tc_inbuf)
            tc_last_layer->setDescription(tc_inbuf);
        return (TCmatch);
    }

    // Look for conversion keywords.
    {
        TCret tcret = ParseCvtLayerBlock();
        if (tcret != TCnone) {
            if (tcret != TCmatch)
                return (tcret);
            return (TCmatch);
        }
    }

    if (Matching(Tkw.kwRGB())) {
        // Set the color of the layer being defined.
        //
        TCret tcret = CheckLD();
        if (tcret != TCnone)
            return (tcret);
        bool ret = GetRgb(rgb);
        dsp_prm(tc_last_layer)->set_red(rgb[0]);
        dsp_prm(tc_last_layer)->set_green(rgb[1]);
        dsp_prm(tc_last_layer)->set_blue(rgb[2]);
        int pix = 0;
        DSPmainDraw(DefineColor(&pix, rgb[0], rgb[1], rgb[2]))
        dsp_prm(tc_last_layer)->set_pixel(pix);
        if (!ret) {
            return (SaveError("%s: unknown color %s.", Tkw.kwRGB(),
                tc_inbuf));
        }
        return (TCmatch);
    }
    if (Matching(Tkw.Filled())) {
        TCret tcret = CheckLD();
        if (tcret != TCnone)
            return (tcret);
        if (!GetFilled(tc_last_layer)) {
            return (SaveError("%s: parse failed.  %s", Tkw.Filled(),
                Errs()->get_error()));
        }
        return (TCmatch);
    }
    if (Matching(Tkw.Invisible())) {
        // Set visibility of layer being defined.
        //
        TCret tcret = CheckLD();
        if (tcret != TCnone)
            return (tcret);
        if (GetBoolean(tc_inbuf)) {
            tc_last_layer->setInvisible(true);
            tc_last_layer->setRstInvisible(true);
        }
        else {
            tc_last_layer->setInvisible(false);
            tc_last_layer->setRstInvisible(false);
        }
        return (TCmatch);
    }
    if (Matching(Tkw.NoSelect())) {
        // Set selectability of layer being defined.
        //
        TCret tcret = CheckLD();
        if (tcret != TCnone)
            return (tcret);
        if (GetBoolean(tc_inbuf)) {
            tc_last_layer->setNoSelect(true);
            tc_last_layer->setRstNoSelect(true);
        }
        else {
            tc_last_layer->setNoSelect(false);
            tc_last_layer->setRstNoSelect(false);
        }
        return (TCmatch);
    }
    if (Matching(Tkw.WireActive())) {
        TCret tcret = CheckLD();
        if (tcret != TCnone)
            return (tcret);
        tc_last_layer->setWireActive(GetBoolean(tc_inbuf));
        return (TCmatch);
    }
    if (Matching(Tkw.NoInstView())) {
        TCret tcret = CheckLD();
        if (tcret != TCnone)
            return (tcret);
        tc_last_layer->setNoInstView(GetBoolean(tc_inbuf));
        return (TCmatch);
    }
    if (Matching(Tkw.Invalid())) {
        TCret tcret = CheckLD();
        if (tcret != TCnone)
            return (tcret);
        tc_last_layer->setInvalid(GetBoolean(tc_inbuf));
        return (TCmatch);
    }
    if (Matching(Tkw.Blink())) {
        // Set blinking status of layer being defined.
        //
        TCret tcret = CheckLD();
        if (tcret != TCnone)
            return (tcret);
        tc_last_layer->setBlink(GetBoolean(tc_inbuf));
        return (TCmatch);
    }
    if (Matching(Tkw.WireWidth())) {
        // Set the default wire width for the layer being defined,
        // physical mode only.
        //
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        double d;
        if (sscanf(tc_inbuf, "%lf", &d) == 1 && d >= 0)
            dsp_prm(tc_last_layer)->set_wire_width(INTERNAL_UNITS(d));
        else
            return (SaveError(lmsg, tc_last_layer->name(), kw));
        return (TCmatch);
    }
    if (Matching(Tkw.CrossThick())) {
        // Thickness for cross-section.
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        double d;
        if (sscanf(tc_inbuf, "%lf", &d) == 1 && d >= 0.01 && d <= 10.0)
            dsp_prm(tc_last_layer)->set_xsect_thickness(INTERNAL_UNITS(d));
        else
            return (SaveError(lmsg, tc_last_layer->name(), kw));
        return (TCmatch);
    }

    // These keywords mostly supply electrical attributes for the
    // physical layers.  These are not generally used outside of the
    // extraction system, though the Thickness and Symbolic values
    // are used in the cross-section display.

    if (Matching(Tkw.Symbolic())) {
        // This suppresses the layer in the cross section view.
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        tc_last_layer->setSymbolic(GetBoolean(tc_inbuf));
        return (TCmatch);
    }
    if (Matching(Tkw.NoMerge())) {
        // This prevents automatic merging in this layer.
        TCret tcret = CheckLD(true);
        if (tcret != TCnone)
            return (tcret);
        tc_last_layer->setNoMerge(GetBoolean(tc_inbuf));
        return (TCmatch);
    }

    // Check extraction support layer block keywords.
    {
        TCret tcret = ParseExtLayerBlock();
        if (tcret != TCnone) {
            if (tcret != TCmatch)
                return (tcret);
            return (TCmatch);
        }
    }

    // Caller's processing.
    if (tc_parse_lyr_blk) {
        TCret tcret = (*tc_parse_lyr_blk)(tc_last_layer);
        if (tcret != TCnone) {
            if (tcret != TCmatch)
                return (tcret);
            return (TCmatch);
        }
    }

    // Obsolete keywords.
    if (Matching(Tkw.AltRGB()) || Matching(Tkw.AltFilled()) ||
            Matching(Tkw.AltInvisible())) {
        const char *obsmsg = "Layer %s: Obsolete %s keyword.";
        return (SaveError(obsmsg, tc_last_layer->name(), kw));
    }

    return (TCnone);
}


// Perform any final checking.
//
TCret
cTech::checkLayerFinal(CDl *ld)
{
    // Call back to the application for layer check, physical only.
    if (ld->physIndex() >= 0) {
        sLstr lstr;
        char *s = ExtCheckLayerKeywords(ld);
        if (s) {
            char *e = s + strlen(s) - 1;
            lstr.add(s);
            if (*e != 'n')
                lstr.add_c('\n');
            delete [] s;
        }
        if (tc_layer_check) {
            s = (*tc_layer_check)(ld);
            if (s) {
                lstr.add(s);
                delete [] s;
            }
        }
        if (lstr.string())
            return (lstr.string_trim());
    }
    return (TCnone);
}


bool
cTech::getLineFromFile(FILE *techfp, tReadMode rmode)
{
    // Assert valid line numbering before every line is read.
    tc_no_line_num = false;

    char *ib = tc_inbuf;
    if (rmode != tReadNormal) {
        bool saving = false;
        int c = getc(techfp);
        if (c == EOF)
            return (false);
        for (;;) {
            while (c != '\n' && c != EOF) {
                if (ib - tc_inbuf < TECH_BUFSIZE-1)
                    *ib++ = c;
                saving = true;
                c = getc(techfp);
            }
            if (saving) {
                if (*(ib-1) == '\r')
                    ib--;  // bah! M$ crap
                if (*(ib-1) == '\\') {
                    // continuation
                    ib--;
                    IncLineCount();
                    c = getc(techfp);
                    continue;
                }
            }
            break;
        }
    }
    else {
        // Look for first keyword, which starts with alpha or '!'.
        int c;
        for (;;) {
            c = getc(techfp);
            if (c == EOF)
                return (false);
            if (isalpha(c) || c == '!')
                break;
            if (c == '#') {
                // comment, skip line
                while (c != '\n' && c != EOF)
                    c = getc(techfp);
                if (c == EOF)
                    return (false);
            }
            if (c == '/') {
                // Also accept '//' as comment start.
                c = getc(techfp);
                if (c == '/') {
                    while (c != '\n' && c != EOF)
                        c = getc(techfp);
                    if (c == EOF)
                        return (false);
                }
            }

            if (c == '\n')
                IncLineCount();
        }

        // Scan to end of keyword and convert to upper case.
        char *cp = tc_kwbuf;
        while (!isspace(c) && c != EOF) {
            if (cp - tc_kwbuf < KEYBUFSIZ-1) {
                if (islower(c))
                    c = toupper(c);
                *cp++ = c;
            }
            c = getc(techfp);
        }
        *cp = 0;

        // Scan to end of line and put args in tc_inbuf, tc_inbuf starts
        // with the first non-space.  Filter control characters, handle
        // continuation.

        bool saving = false;
        for (;;) {
            while (c != '\n' && c != EOF) {
                if (c == '\t')
                    c = ' ';
                if (c > ' ' || saving) {
                    if (ib - tc_inbuf < TECH_BUFSIZE-1 && c >= ' ')
                        *ib++ = c;
                    saving = true;
                }
                c = getc(techfp);
            }
            if (saving && *(ib-1) == '\\') {
                // continuation
                ib--;
                IncLineCount();
                c = getc(techfp);
                continue;
            }
            break;
        }
    }
    *ib = 0;
    if (tc_origbuf)
        strcpy(tc_origbuf, tc_inbuf);
    return (true);
}

