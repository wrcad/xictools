
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

#include "config.h"
#include "outplot.h"
#include "outdata.h"
#include "cshell.h"
#include "commands.h"
#include "frontend.h"
#include "keywords.h"
#include "kwords_fte.h"
#include "kwords_analysis.h"
#include "toolbar.h"
#include "circuit.h"
#include "spnumber/spnumber.h"
#include "miscutil/filestat.h"
#include "ginterf/graphics.h"
#include "help/help_defs.h"

typedef void ParseNode;
#include "spnumber/spparse.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif


typedef sKWent<userEnt> KWent;

// Instantiate.
cKeyWords KW;

#define STR(x) #x
#define STRINGIFY(x) STR(x)


// Print a listing of option keywords and descriptions.
//
void
CommandTab::com_usrset(wordlist *wl)
{
    TTY.init_more();
    if (wl == 0) {
        int i;
        TTY.send("Shell:\n");
        for (i = 0; KW.shell(i)->word; i++)
            KW.shell(i)->print(0);
        TTY.send("Simulator:\n");
        for (i = 0; KW.sim(i)->word; i++)
            KW.sim(i)->print(0);
        TTY.printf("Commands:\n");
        for (i = 0; KW.cmds(i)->word; i++)
            KW.cmds(i)->print(0);
        TTY.printf("Plot:\n");
        for (i = 0; KW.plot(i)->word; i++)
            KW.plot(i)->print(0);
        TTY.printf("Debug:\n");
        for (i = 0; KW.debug(i)->word; i++)
            KW.debug(i)->print(0);
        return;
    }
    for (; wl; wl = wl->wl_next) {
        if (*wl->wl_word == '-') {
            int i;
            switch (wl->wl_word[1]) {
            case 'c':
            case 'C':
                TTY.printf("Commands:\n");
                for (i = 0; KW.cmds(i)->word; i++)
                    KW.cmds(i)->print(0);
                continue;
            case 'd':
            case 'D':
                TTY.printf("Debug:\n");
                for (i = 0; KW.debug(i)->word; i++)
                    KW.debug(i)->print(0);
                continue;
            case 'p':
            case 'P':
                TTY.printf("Plot:\n");
                for (i = 0; KW.plot(i)->word; i++)
                    KW.plot(i)->print(0);
                continue;
            case 's':
            case 'S':
                if (wl->wl_word[2] == 'h' || wl->wl_word[2] == 'H') {
                    TTY.send("Shell:\n");
                    for (i = 0; KW.shell(i)->word; i++)
                        KW.shell(i)->print(0);
                }
                else if (wl->wl_word[2] == 'i' || wl->wl_word[2] == 'I') {
                    TTY.send("Simulator:\n");
                    for (i = 0; KW.sim(i)->word; i++)
                        KW.sim(i)->print(0);
                }
            }
            continue;
        }
        sKW *entry = (sKW*)sHtab::get(Sp.Options(), wl->wl_word);
        if (entry)
            TTY.printf("%-18s %s %s\n", entry->word,
                ((variable*)0)->typeString(entry->type), entry->descr);
        else
            TTY.printf("%-18s      %s\n", wl->wl_word,
                "Not an internal variable.");
    }
}
// End of CommandTab functions.


// Record the keywords in the application's hash table, call on
// startup
//
void
cKeyWords::initDatabase()
{
    KWent **k;
    for (k = (KWent**)KWplot; (*k)->word; k++)
        (*k)->init();
    for (k = (KWent**)KWcolor; (*k)->word; k++)
        (*k)->init();
    for (k = (KWent**)KWdbargs; (*k)->word; k++)
        (*k)->init();
    for (k = (KWent**)KWdebug; (*k)->word; k++)
        (*k)->init();
    for (k = (KWent**)KWcmds; (*k)->word; k++)
        (*k)->init();
    for (k = (KWent**)KWshell; (*k)->word; k++)
        (*k)->init();
    for (k = (KWent**)KWsim; (*k)->word; k++)
        (*k)->init();
}


void
sKW::print(char **rstr)
{
    char buf[256];
    const char *fmt = "%-18s %s %s\n";
    sprintf(buf, fmt, word, ((variable*)0)->typeString(type), descr);
    if (!rstr)
        TTY.send(buf);
    else
        *rstr = lstring::build_str(*rstr, buf);
}


namespace {
    inline void error_pr(const char *which, const char *minmax,
        const char *what)
    {
        GRpkgIf()->ErrPrintf(ET_ERROR, "bad %s%s value, must be %s.\n", which,
            minmax ? minmax : "", what);
    }


    char tmp_buf[64];

    inline char *pr_integer(int min, int max)
    {
        sprintf(tmp_buf, "an integer %d-%d", min, max);
        return (tmp_buf);
    }


    inline char *pr_real(float min, float max)
    {
        sprintf(tmp_buf, "a real %g-%g", min, max);
        return (tmp_buf);
    }


    // format for printing arguments to keywords
    const char *fmt2 = "    %-19s %s\n";
}

/*************************************************************************
 *  Plot Keywords
 *************************************************************************/

// the plotstyle arguments
const char *kw_linplot          = "linplot";
const char *kw_pointplot        = "pointplot";
const char *kw_combplot         = "combplot";

sKW *cKeyWords::KWpstyles[] = {
    new sKW(kw_linplot, "Standard vector plotting."),
    new sKW(kw_pointplot, "Plot data points only."),
    new sKW(kw_combplot, "Produce a comb plot."),
    new sKW(0, 0)
};

// the gridstyle arguments
const char *kw_lingrid          = "lingrid";
const char *kw_xlog             = "xlog";
const char *kw_ylog             = "ylog";
const char *kw_loglog           = "loglog";
const char *kw_polar            = "polar";
const char *kw_smith            = "smith";
const char *kw_smithgrid        = "smithgrid";

sKW *cKeyWords::KWgstyles[] = {
    new sKW(kw_lingrid, "Use linear scales."),
    new sKW(kw_xlog, "Use a log X scale."),
    new sKW(kw_ylog, "Use a log Y scale."),
    new sKW(kw_loglog, "Use log scales for X and Y."),
    new sKW(kw_polar, "Plot on a polar grid."),
    new sKW(kw_smith, "Plot on a Smith grid, transform data."),
    new sKW(kw_smithgrid, "Plot on a Smith grid."),
    new sKW(0, 0)
};

// the scaletype arguments
const char *kw_multi            = "multi";
const char *kw_single           = "single";
const char *kw_group            = "group";

sKW *cKeyWords::KWscale[] = {
    new sKW(kw_multi, "Use individual Y scale for each trace."),
    new sKW(kw_single, "Use a common Y scale for all traces."),
    new sKW(kw_group, "Use common Y scale for V, I, and other."),
    new sKW(0, 0)
};

// plot window geometry
const char *kw_plotgeom         = "plotgeom";

// the plot keywords
const char *kw_curplot          = "curplot";
const char *kw_device           = "device";
const char *kw_gridsize         = "gridsize";
const char *kw_gridstyle        = "gridstyle";
const char *kw_nogrid           = "nogrid";
const char *kw_noplotlogo       = "noplotlogo";
const char *kw_nointerp         = "nointerp";
const char *kw_plotstyle        = "plotstyle";
const char *kw_pointchars       = "pointchars";
const char *kw_polydegree       = "polydegree";
const char *kw_polysteps        = "polysteps";
const char *kw_scaletype        = "scaletype";
const char *kw_ticmarks         = "ticmarks";
const char *kw_title            = "title";
const char *kw_xcompress        = "xcompress";
const char *kw_xdelta           = "xdelta";
const char *kw_xindices         = "xindices";
const char *kw_xlabel           = "xlabel";
const char *kw_xlimit           = "xlimit";
const char *kw_ydelta           = "ydelta";
const char *kw_ylabel           = "ylabel";
const char *kw_ylimit           = "ylimit";
const char *kw_ysep             = "ysep";

// 'no_record' keywords
const char *kw_curanalysis      = "curanalysis";
const char *kw_curplotdate      = "curplotdate";
const char *kw_curplotname      = "curplotname";
const char *kw_curplottitle     = "curplottitle";
const char *kw_plots            = "plots";

// asciiplot keywords
const char *kw_noasciiplotvalue = "noasciiplotvalue";
const char *kw_nobreak          = "nobreak";

// hardcopy keywords
const char *kw_hcopydriver      = "hcopydriver";
const char *kw_hcopycommand     = "hcopycommand";
const char *kw_hcopyresol       = "hcopyresol";
const char *kw_hcopywidth       = "hcopywidth";
const char *kw_hcopyheight      = "hcopyheight";
const char *kw_hcopyxoff        = "hcopyxoff";
const char *kw_hcopyyoff        = "hcopyyoff";
const char *kw_hcopylandscape   = "hcopylandscape";
const char *kw_hcopyrmdelay     = "hcopyrmdelay";

// xgraph keywords
const char *kw_xglinewidth      = "xglinewidth";
const char *kw_xgmarkers        = "xgmarkers";

// dummy entries
namespace {
    const char *colorN          = "colorN";
    const char *plotposnN       = "plotposnN";
}


//
// The plot-related variables
//

struct KWent_colorN : public KWent
{
    KWent_colorN() { set(
        colorN,
        VTYP_STRING, 0.0, 0.0,
        "Plot colors, N is 0-19, color0 is background."); }

    void callback(bool isset, variable*)
    {
        if (isset)
            GRpkgIf()->ErrPrintf(ET_ERROR, "%s is read only.\n", word);
    }
};

struct KWent_curanalysis : public KWent
{
    KWent_curanalysis() { set(
        kw_curanalysis,
        VTYP_STRING, 0.0, 0.0,
        "Latest analysis type (read only)."); }

    void callback(bool isset, variable*)
    {
        if (isset)
            GRpkgIf()->ErrPrintf(ET_ERROR, "%s is read only.\n", word);
    }
};

struct KWent_curplot : public KWent
{
    KWent_curplot() { set(
        kw_curplot,
        VTYP_STRING, 0.0, 0.0,
        "Current plot."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
            Sp.SetCurPlot(v->string());
        }
        KWent::callback(isset, v);
    }
};

struct KWent_curplotdate : public KWent
{
    KWent_curplotdate() { set(
        kw_curplotdate,
        VTYP_STRING, 0.0, 0.0,
        "Current plot date (read only)."); }

    void callback(bool isset, variable*)
    {
        if (isset)
            GRpkgIf()->ErrPrintf(ET_ERROR, "%s is read only.\n", word);
    }
};

struct KWent_curplotname : public KWent
{
    KWent_curplotname() { set(
        kw_curplotname,
        VTYP_STRING, 0.0, 0.0,
        "Current plot name (read only)."); }

    void callback(bool isset, variable*)
    {
        if (isset)
            GRpkgIf()->ErrPrintf(ET_ERROR, "%s is read only.\n", word);
    }
};

struct KWent_curplottitle : public KWent
{
    KWent_curplottitle() { set(
        kw_curplottitle,
        VTYP_STRING, 0.0, 0.0,
        "Current plot title (read only)."); }

    void callback(bool isset, variable*)
    {
        if (isset)
            GRpkgIf()->ErrPrintf(ET_ERROR, "%s is read only.\n", word);
    }
};

struct KWent_device : public KWent
{
    KWent_device() { set(
        kw_device,
        VTYP_STRING, 0.0, 0.0,
        "/dev/tty<?> device for MFB output."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_gridsize : public KWent
{
    KWent_gridsize() { set(
        kw_gridsize,
        VTYP_NUM, 0.0, 10000.0,
        "If > 0, interpolate trace to this many points."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_gridstyle : public KWent
{
    KWent_gridstyle() { set(
        kw_gridstyle,
        VTYP_STRING, 0.0, 0.0,
        "Grid: lin, xlog, ylog, loglog, polar, smith, smithgrid."); }

    void print(char **rstr)
    {
        sKW::print(rstr);
        char buf[256];
        for (int i = 0; KW.gstyles(i)->word; i++) {
            sprintf(buf, fmt2, KW.gstyles(i)->word, KW.gstyles(i)->descr);
            if (!rstr)
                TTY.send(buf);
            else
                *rstr = lstring::build_str(*rstr, buf);
        }
    }

    void callback(bool isset, variable *v)
    {
        int i = 0;
        if (isset) {
            if (v->type() != VTYP_STRING) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad gridstyle value.\n");
                return;
            }
            for (i = 0; KW.gstyles(i)->word; i++)
                if (lstring::cieq(v->string(), KW.gstyles(i)->word))
                    break;
            if (!KW.gstyles(i)->word) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad gridstyle keyword.\n");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_hcopydriver : public KWent
{
    KWent_hcopydriver() { set(
        kw_hcopydriver,
        VTYP_STRING, 0.0, 0.0,
        "Hardcopy driver type."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad hcopydriver value.\n");
                return;
            }
            HCdesc *hcdesc = GRpkgIf()->FindHCdesc(v->string());
            if (!hcdesc) {
                if (GRpkgIf()->HCof(0))
                    // No error if no drivers available (batch mode).
                    GRpkgIf()->ErrPrintf(ET_ERROR,
                        "bad hcopydriver keyword.\n");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_hcopycommand : public KWent
{
    KWent_hcopycommand() { set(
        kw_hcopycommand,
        VTYP_STRING, 0.0, 0.0,
        "Hardcopy spooler command."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_hcopyresol : public KWent
{
    KWent_hcopyresol() { set(
        kw_hcopyresol,
        VTYP_NUM, 0.0, 10000.0,
        "Hardcopy resolution, dpi."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_hcopywidth : public KWent
{
    KWent_hcopywidth() { set(
        kw_hcopywidth,
        VTYP_STRING, 0.0, 0.0,
        "Hardcopy width."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            char buf[64];
            if (v->type() == VTYP_NUM) {
                sprintf(buf, "%d", v->integer());
                v->set_string(buf);
            }
            else if (v->type() == VTYP_REAL) {
                sprintf(buf, "%g", v->real());
                v->set_string(buf);
            }
            else if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_hcopyheight : public KWent
{
    KWent_hcopyheight() { set(
        kw_hcopyheight,
        VTYP_STRING, 0.0, 0.0,
        "Hardcopy height."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            char buf[64];
            if (v->type() == VTYP_NUM) {
                sprintf(buf, "%d", v->integer());
                v->set_string(buf);
            }
            else if (v->type() == VTYP_REAL) {
                sprintf(buf, "%g", v->real());
                v->set_string(buf);
            }
            else if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_hcopyxoff : public KWent
{
    KWent_hcopyxoff() { set(
        kw_hcopyxoff,
        VTYP_STRING, 0.0, 0.0,
        "Hardcopy x-offset."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            char buf[64];
            if (v->type() == VTYP_NUM) {
                sprintf(buf, "%d", v->integer());
                v->set_string(buf);
            }
            else if (v->type() == VTYP_REAL) {
                sprintf(buf, "%g", v->real());
                v->set_string(buf);
            }
            else if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_hcopyyoff : public KWent
{
    KWent_hcopyyoff() { set(
        kw_hcopyyoff,
        VTYP_STRING, 0.0, 0.0,
        "Hardcopy y-offset."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            char buf[64];
            if (v->type() == VTYP_NUM) {
                sprintf(buf, "%d", v->integer());
                v->set_string(buf);
            }
            else if (v->type() == VTYP_REAL) {
                sprintf(buf, "%g", v->real());
                v->set_string(buf);
            }
            else if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_hcopylandscape : public KWent
{
    KWent_hcopylandscape() { set(
        kw_hcopylandscape,
        VTYP_BOOL, 0.0, 0.0,
        "Hardcopy landscape mode."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_hcopyrmdelay : public KWent
{
    KWent_hcopyrmdelay() { set(
        kw_hcopyrmdelay,
        VTYP_NUM, 0.0, 4320.0,
        "Minutes to deletion of plot temp file."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
            filestat::set_rm_minutes(v->integer());
        }
        else
            filestat::set_rm_minutes(0);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_noasciiplotvalue : public KWent
{
    KWent_noasciiplotvalue() { set(
        kw_noasciiplotvalue,
        VTYP_BOOL, 0.0, 0.0,
        "Don't print scale in asciiplot."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_nobreak : public KWent
{
    KWent_nobreak() { set(
        kw_nobreak,
        VTYP_BOOL, 0.0, 0.0,
        "No break between pages in asciiplot."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_nogrid : public KWent
{
    KWent_nogrid() { set(
        kw_nogrid,
        VTYP_BOOL, 0.0, 0.0,
        "No drawing of grid."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_noplotlogo : public KWent
{
    KWent_noplotlogo() { set(
        kw_noplotlogo,
        VTYP_BOOL, 0.0, 0.0,
        "No drawing of WRspice logo."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_nointerp : public KWent
{
    KWent_nointerp() { set(
        kw_nointerp,
        VTYP_BOOL, 0.0, 0.0,
        "Do not use interpolation."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_plotgeom : public KWent
{
    KWent_plotgeom() { set(
        kw_plotgeom,
        VTYP_STRING, 0.0, 0.0,
        "Plot window geometry."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_STRING) {
                const char *string = v->string();
                double *d = SPnum.parse(&string, false);
                if (!d || *d < 100.0 || *d > 2000.0) {
                    error_pr(word, " width", ">= 100 and <= 2000");
                    return;
                }
                while (*string && !isdigit(*string))
                    string++;
                d = SPnum.parse(&string, true);
                if (!d || *d < 100.0 || *d > 2000.0) {
                    error_pr(word, " height", ">= 100 and <= 2000");
                    return;
                }
            }
            else if (v->type() == VTYP_LIST) {
                variable *vx = v->list();
                if (!vx || !vx->next()) {
                    GRpkgIf()->ErrPrintf(ET_ERROR, "bad list for plotgeom.\n");
                    return;
                }
                if (vx->type() == VTYP_REAL && vx->real() >= 100.0 &&
                        vx->real() <= 2000.0) {
                    int val = (int)vx->real();
                    vx->set_integer(val);
                }
                else if (!(vx->type() == VTYP_NUM && vx->integer() >= 100 &&
                        vx->integer() <= 2000)) {
                    error_pr(word, " width", ">= 100 and <= 2000");
                    return;
                }
                vx = vx->next();
                if (vx->type() == VTYP_REAL && vx->real() >= 100.0 &&
                        vx->real() <= 2000.0) {
                    int val = (int)vx->real();
                    vx->set_integer(val);
                }
                else if (!(vx->type() == VTYP_NUM && vx->integer() > 100 &&
                        vx->integer() <= 2000)) {
                    error_pr(word, " height", ">= 100 and <= 2000");
                    return;
                }
            }
            else {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad plotgeom set syntax.\n");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_plotposnN : public KWent
{
    KWent_plotposnN() { set(
        plotposnN,
        VTYP_LIST, 0.0, 15.0,
        "N'th plot screen x-y position, N is 0-15."); }

    void callback(bool isset, variable*)
    {
        if (isset)
            GRpkgIf()->ErrPrintf(ET_ERROR, "%s is read only.\n", word);
    }
};

struct KWent_plots : public KWent
{
    KWent_plots() { set(
        kw_plots,
        VTYP_LIST, 0.0, 0.0,
        "List of plots (read only)."); }

    void callback(bool isset, variable*)
    {
        if (isset)
            GRpkgIf()->ErrPrintf(ET_ERROR, "%s is read only.\n", word);
    }
};


struct KWent_plotstyle : public KWent
{
    KWent_plotstyle() { set(
        kw_plotstyle,
        VTYP_STRING, 0.0, 0.0,
        "Style: linplot, combplot, pointplot."); }

    void print(char **rstr)
    {
        sKW::print(rstr);
        char buf[256];
        for (int i = 0; KW.pstyles(i)->word; i++) {
            sprintf(buf, fmt2, KW.pstyles(i)->word, KW.pstyles(i)->descr);
            if (!rstr)
                TTY.send(buf);
            else
                *rstr = lstring::build_str(*rstr, buf);
        }
    }

    void callback(bool isset, variable *v)
    {
        int i = 0;
        if (isset) {
            if (v->type() != VTYP_STRING) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad plotstyle value.\n");
                return;
            }
            for (i = 0; KW.pstyles(i)->word; i++)
                if (lstring::cieq(v->string(), KW.pstyles(i)->word))
                    break;
            if (!KW.pstyles(i)->word) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad plotstyle keyword.\n");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_pointchars : public KWent
{
    KWent_pointchars() { set(
        kw_pointchars,
        VTYP_STRING, 0.0, 0.0,
        "Characters to use in point plots."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};


struct KWent_polydegree : public KWent
{
    KWent_polydegree() { set(
        kw_polydegree,
        VTYP_NUM, DEF_polydegree_MIN, DEF_polydegree_MAX,
        "Degree of polynomial used for interpolation, default "
            STRINGIFY(DEF_polydegree) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};


struct KWent_polysteps : public KWent
{
    KWent_polysteps() { set(
        kw_polysteps,
        VTYP_NUM, DEF_polysteps_MIN, DEF_polysteps_MAX,
        "Number of interpolating points, default "
            STRINGIFY(DEF_polysteps) "10."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_scaletype : public KWent
{
    KWent_scaletype() { set(
        kw_scaletype,
        VTYP_STRING, 0.0, 0.0,
        "Scale: single, multi, group."); }

    void print(char **rstr)
    {
        sKW::print(rstr);
        char buf[256];
        for (int i = 0; KW.scale(i)->word; i++) {
            sprintf(buf, fmt2, KW.scale(i)->word, KW.scale(i)->descr);
            if (!rstr)
                TTY.send(buf);
            else
                *rstr = lstring::build_str(*rstr, buf);
        }
    }

    void callback(bool isset, variable *v)
    {
        int i = 0;
        if (isset) {
            if (v->type() != VTYP_STRING) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad scaletype value.\n");
                return;
            }
            for (i = 0; KW.scale(i)->word; i++)
                if (lstring::cieq(v->string(), KW.scale(i)->word))
                    break;
            if (!KW.scale(i)->word) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad scaletype keyword.\n");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_ticmarks : public KWent
{
    KWent_ticmarks() { set(
        kw_ticmarks,
        VTYP_NUM, 1.0, 10000.0,
        "Draw a mark every N points."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_title : public KWent
{
    KWent_title() { set(
        kw_title,
        VTYP_STRING, 0.0, 0.0,
        "Title string for plots."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_xcompress : public KWent
{
    KWent_xcompress() { set(
        kw_xcompress,
        VTYP_NUM, 2.0, 10000.0,
        "Number of points to compress into one."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_xdelta : public KWent
{
    KWent_xdelta() { set(
        kw_xdelta,
        VTYP_REAL, 1e-37, 1e38,
        "X spacing of grid lines."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_xglinewidth : public KWent
{
    KWent_xglinewidth() { set(
        kw_xglinewidth,
        VTYP_NUM, 1.0, 16.0,
        "Pixel line width used in xgraph, default 1."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_xgmarkers : public KWent
{
    KWent_xgmarkers() { set(
        kw_xgmarkers,
        VTYP_BOOL, 0.0, 1.0,
        "Use markers in xgraph pointplot, else big pixels."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_xindices : public KWent
{
    KWent_xindices() { set(
        kw_xindices,
        VTYP_STRING, 0.0, 1e7,
        "Vector indices between which compression occurs."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_STRING) {
                const char *string = v->string();
                double *d = SPnum.parse(&string, false);
                if (!d || *d < 0) {
                    error_pr(word, " min", "a positive integer");
                    return;
                }
                while (*string && !isdigit(*string) &&
                    *string != '-' && *string != '+') string++;
                d = SPnum.parse(&string, true);
                if (!d || *d < 0) {
                    error_pr(word, " max", "a positive integer");
                    return;
                }
            }
            else if (v->type() == VTYP_LIST) {
                variable *vx = v->list();
                if (!vx || !vx->next()) {
                    GRpkgIf()->ErrPrintf(ET_ERROR, "bad list for xindices.\n");
                    return;
                }
                if (vx->type() == VTYP_REAL && vx->real() >= 0.0) {
                    int val = (int)vx->real();
                    vx->set_integer(val);
                }
                else if (!(vx->type() == VTYP_NUM && vx->integer() > 0)) {
                    error_pr(word, " min", "a positive integer");
                    return;
                }
                vx = vx->next();
                if (vx->type() == VTYP_REAL && vx->real() >= 0.0) {
                    int val = (int)vx->real();
                    vx->set_integer(val);
                }
                else if (!(vx->type() == VTYP_NUM && vx->integer() > 0)) {
                    error_pr(word, " max", "a positive integer");
                    return;
                }
            }
            else {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad xindices set syntax.\n");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_xlabel : public KWent
{
    KWent_xlabel() { set(
        kw_xlabel,
        VTYP_STRING, 0.0, 0.0,
        "Label for X scale of plot."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_xlimit : public KWent
{
    KWent_xlimit() { set(
        kw_xlimit,
        VTYP_STRING, 0.0, 0.0,
        "X scale limits in plots."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_STRING) {
                const char *string = v->string();
                double *d = SPnum.parse(&string, false);
                if (!d) {
                    error_pr(word, " min", "a real");
                    return;
                }
                while (*string && !isdigit(*string) &&
                        *string != '-' && *string != '+')
                    string++;
                d = SPnum.parse(&string, true);
                if (!d) {
                    error_pr(word, " max", "a real");
                    return;
                }
            }
            else if (v->type() == VTYP_LIST) {
                variable *vx = v->list();
                if (!vx || !vx->next()) {
                    GRpkgIf()->ErrPrintf(ET_ERROR, "bad list for %s.\n", word);
                    return;
                }
                if (vx->type() == VTYP_NUM) {
                    double dval = vx->integer();
                    vx->set_real(dval);
                }
                else if (vx->type() != VTYP_REAL) {
                    error_pr(word, " min", "a real");
                    return;
                }
                vx = vx->next();
                if (vx->type() == VTYP_NUM) {
                    double dval = vx->integer();
                    vx->set_real(dval);
                }
                else if (vx->type() != VTYP_REAL) {
                    error_pr(word, " max", "a real");
                    return;
                }
            }
            else {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad set syntax for %s.\n",
                    word);
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_ydelta : public KWent
{
    KWent_ydelta() { set(
        kw_ydelta,
        VTYP_REAL, 1e-37, 1e38,
        "Y spacing of grid lines."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_ylabel : public KWent
{
    KWent_ylabel() { set(
        kw_ylabel,
        VTYP_STRING, 0.0, 0.0,
        "Label for Y scale of plot."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_ylimit : public KWent
{
    KWent_ylimit() { set(
        kw_ylimit,
        VTYP_STRING, 0.0, 0.0,
        "Y scale limits in plots."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_STRING) {
                const char *string = v->string();
                double *d = SPnum.parse(&string, false);
                if (!d) {
                    error_pr(word, " min", "a real");
                    return;
                }
                while (*string && !isdigit(*string) &&
                    *string != '-' && *string != '+') string++;
                d = SPnum.parse(&string, true);
                if (!d) {
                    error_pr(word, " max", "a real");
                    return;
                }
            }
            else if (v->type() == VTYP_LIST) {
                variable *vx = v->list();
                if (!vx || !vx->next()) {
                    GRpkgIf()->ErrPrintf(ET_ERROR, "bad list for %s.\n", word);
                    return;
                }
                if (vx->type() == VTYP_NUM) {
                    double dval = vx->integer();
                    vx->set_real(dval);
                }
                else if (vx->type() != VTYP_REAL) {
                    error_pr(word, " min", "a real");
                    return;
                }
                vx = vx->next();
                if (vx->type() == VTYP_NUM) {
                    double dval = vx->integer();
                    vx->set_real(dval);
                }
                else if (vx->type() != VTYP_REAL) {
                    error_pr(word, " max", "a real");
                    return;
                }
            }
            else {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad set syntax for %s.\n",
                    word);
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_ysep : public KWent
{
    KWent_ysep() { set(
        kw_ysep,
        VTYP_BOOL, 0.0, 0.0,
        "Separate the traces anong the vertical axis."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

sKW *cKeyWords::KWplot[] = {
    new KWent_colorN(),
    new KWent_curanalysis(),
    new KWent_curplot(),
    new KWent_curplotdate(),
    new KWent_curplotname(),
    new KWent_curplottitle(),
    new KWent_device(),
    new KWent_gridsize(),
    new KWent_gridstyle(),
    new KWent_hcopydriver(),
    new KWent_hcopycommand(),
    new KWent_hcopyresol(),
    new KWent_hcopywidth(),
    new KWent_hcopyheight(),
    new KWent_hcopyxoff(),
    new KWent_hcopyyoff(),
    new KWent_hcopylandscape(),
    new KWent_hcopyrmdelay(),
    new KWent_noasciiplotvalue(),
    new KWent_nobreak(),
    new KWent_nogrid(),
    new KWent_noplotlogo(),
    new KWent_nointerp(),
    new KWent_plotgeom(),
    new KWent_plotposnN(),
    new KWent_plots(),
    new KWent_plotstyle(),
    new KWent_pointchars(),
    new KWent_polydegree(),
    new KWent_polysteps(),
    new KWent_scaletype(),
    new KWent_ticmarks(),
    new KWent_title(),
    new KWent_xcompress(),
    new KWent_xdelta(),
    new KWent_xglinewidth(),
    new KWent_xgmarkers(),
    new KWent_xindices(),
    new KWent_xlabel(),
    new KWent_xlimit(),
    new KWent_ydelta(),
    new KWent_ylabel(),
    new KWent_ylimit(),
    new KWent_ysep(),
    new sKW(0, 0)
};


/*************************************************************************
 *  Color Keywords
 *************************************************************************/

// the colorN keywords
const char *kw_color0           = "color0";
const char *kw_color1           = "color1";
const char *kw_color2           = "color2";
const char *kw_color3           = "color3";
const char *kw_color4           = "color4";
const char *kw_color5           = "color5";
const char *kw_color6           = "color6";
const char *kw_color7           = "color7";
const char *kw_color8           = "color8";
const char *kw_color9           = "color9";
const char *kw_color10          = "color10";
const char *kw_color11          = "color11";
const char *kw_color12          = "color12";
const char *kw_color13          = "color13";
const char *kw_color14          = "color14";
const char *kw_color15          = "color15";
const char *kw_color16          = "color16";
const char *kw_color17          = "color17";
const char *kw_color18          = "color18";
const char *kw_color19          = "color19";

struct KWent_color : public KWent
{
    KWent_color(const char *w, const char *d)
        { set(w, VTYP_STRING, 0.0, 0.0, d); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "bad %s value, must be a string.\n", word);
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

sKW *cKeyWords::KWcolor[] = {
    new KWent_color(kw_color0, "Plot background."),
    new KWent_color(kw_color1, "Plot foreground."),
    new KWent_color(kw_color2, "Plot trace color."),
    new KWent_color(kw_color3, ""),
    new KWent_color(kw_color4, ""),
    new KWent_color(kw_color5, ""),
    new KWent_color(kw_color6, ""),
    new KWent_color(kw_color7, ""),
    new KWent_color(kw_color8, ""),
    new KWent_color(kw_color9, ""),
    new KWent_color(kw_color10, ""),
    new KWent_color(kw_color11, ""),
    new KWent_color(kw_color12, ""),
    new KWent_color(kw_color13, ""),
    new KWent_color(kw_color14, ""),
    new KWent_color(kw_color15, ""),
    new KWent_color(kw_color16, ""),
    new KWent_color(kw_color17, ""),
    new KWent_color(kw_color18, ""),
    new KWent_color(kw_color19, ""),
    new sKW(0,0)
};


/*************************************************************************
 *  Debug Keywords
 *************************************************************************/

// the debug arguments
const char *kw_async            = "async";
const char *kw_control          = "control";
const char *kw_cshpar           = "cshpar";
const char *kw_eval             = "eval";
const char *kw_ginterface       = "ginterface";
const char *kw_helpsys          = "helpsys";
const char *kw_plot             = "plot";
const char *kw_parser           = "parser";
const char *kw_siminterface     = "siminterface";
const char *kw_vecdb            = "vecdb";

struct KWent_dbarg : public KWent
{
    KWent_dbarg(const char *w, const char *d) { set(
        w,
        VTYP_BOOL, 0.0, 0.0,
        d); }

    void callback(bool isset, variable *v)
    {
        if (lstring::cieq(word, kw_async))
            Sp.SetFlag(FT_ASYNCDB, isset);
        else if (lstring::cieq(word, kw_control))
            Sp.SetFlag(FT_CTRLDB, isset);
        else if (lstring::cieq(word, kw_cshpar))
            CP.SetFlag(CP_DEBUG, isset);
        else if (lstring::cieq(word, kw_eval))
            Sp.SetFlag(FT_EVDB, isset);
        else if (lstring::cieq(word, kw_ginterface))
            Sp.SetFlag(FT_GIDB, isset);
        else if (lstring::cieq(word, kw_helpsys))
            HLP()->set_debug(isset);
        else if (lstring::cieq(word, kw_plot))
            Sp.SetFlag(FT_GRDB, isset);
        else if (lstring::cieq(word, kw_parser))
            Parser::Debug = isset;
        else if (lstring::cieq(word, kw_siminterface)) {
            Sp.SetFlag(FT_SIMDB, isset);
            sCKT::CKTstepDebug = isset;
        }
        else if (lstring::cieq(word, kw_vecdb))
            Sp.SetFlag(FT_VECDB, isset);
        else
            return;
        KWent::callback(isset, v);
    }
};

sKW *cKeyWords::KWdbargs[] = {
    new KWent_dbarg(kw_async, "Debug background simulations."),
    new KWent_dbarg(kw_control, "Debug control structures."),
    new KWent_dbarg(kw_cshpar, "Debug command interpreter."),
    new KWent_dbarg(kw_eval, "Debug script evaluation."),
    new KWent_dbarg(kw_ginterface, "Debug graphical interface."),
    new KWent_dbarg(kw_helpsys, "Debug help system."),
    new KWent_dbarg(kw_plot, "Debug plotting."),
    new KWent_dbarg(kw_parser, "Debug input parsing."),
    new KWent_dbarg(kw_siminterface, "Debug simulation interface."),
    new KWent_dbarg(kw_vecdb, "Debug vector database."),
    new sKW(0, 0)
};


// the debug keywords
const char *kw_debug            = "debug";
const char *kw_display          = "display";
const char *kw_dontplot         = "dontplot";
const char *kw_nosubckt         = "nosubckt";
const char *kw_program          = "program";
const char *kw_strictnumparse   = "strictnumparse";
const char *kw_units_catchar    = "units_catchar";
const char *kw_subc_catchar     = "subc_catchar";
const char *kw_subc_catmode     = "subc_catmode";
const char *kw_plot_catchar     = "plot_catchar";
const char *kw_spec_catchar     = "spec_catchar";
const char *kw_var_catchar      = "var_catchar";
const char *kw_term             = "term";
const char *kw_trantrace        = "trantrace";
const char *kw_fpemode          = "fpemode";


inline  int
dbarg(const char *word)
{
    for (int i = 0; KW.dbargs(i)->word; i++)
        if (lstring::cieq(word, KW.dbargs(i)->word))
            return (i);
    return (-1);
}


struct KWent_debug : public KWent
{
    KWent_debug() { set(
        kw_debug,
        VTYP_LIST, 0.0, 0.0,
        "Enable debugging messages."); }

    void print(char **rstr)
    {
        sKW::print(rstr);
        char buf[256];
        for (int i = 0; KW.dbargs(i)->word; i++) {
            sprintf(buf, fmt2, KW.dbargs(i)->word, KW.dbargs(i)->descr);
            if (!rstr)
                TTY.send(buf);
            else
                *rstr = lstring::build_str(*rstr, buf);
        }
    }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_BOOL) {
                for (int i = 0; KW.dbargs(i)->word; i++)
                    static_cast<KWent_dbarg*>(KW.dbargs(i))->callback(true, v);
                CP.RawVarSet(word, true, v);
            }
            else if (v->type() == VTYP_STRING) {
                int i = dbarg(v->string());
                if (i >= 0) {
                    static_cast<KWent_dbarg*>(KW.dbargs(i))->callback(true, v);
                    CP.RawVarSet(word, true, v);
                }
                else {
                    GRpkgIf()->ErrPrintf(ET_ERROR,
                        "unknown debug keyword %s.\n", v->string());
                    return;
                }
            }
            else if (v->type() == VTYP_LIST) {
                // first check the list for bad entries
                int i;
                variable *lv = 0, *tnext, *tv;
                for (tv = v->list(); tv; lv = tv, tv = tnext) {
                    tnext = tv->next();
                    if (tv->type() == VTYP_STRING) {
                        i = dbarg(tv->string());
                        if (i >= 0)
                            continue;
                    }
                    // delete the bad entry
                    GRpkgIf()->ErrPrintf(ET_WARN,
                        "debug list contains bad entry %s, ignored.\n",
                            tv->string());
                    if (lv)
                        lv->set_next(tnext);
                    else {
                        v->zero_list();
                        v->set_list(tnext);
                    }
                    delete tv;
                    tv = lv;
                }
                if (!v->list())
                    return;

                // list is ok
                for (tv = v->list(); tv; tv = tv->next()) {
                    i = dbarg(tv->string());
                    static_cast<KWent_dbarg*>(KW.dbargs(i))->callback(true, v);
                }
                CP.RawVarSet(word, true, v);
            }
            else {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad debug keyword.\n");
                return;
            }
        }
        else {
            for (int i = 0; KW.dbargs(i)->word; i++)
                static_cast<KWent_dbarg*>(KW.dbargs(i))->callback(false, v);
            CP.RawVarSet(word, false, v);
        }
        KWent::callback(isset, v);
    }
};

struct KWent_display : public KWent
{
    KWent_display() { set(
        kw_display,
        VTYP_STRING, 0.0, 0.0,
        "X display used, read only."); }

    void callback(bool isset, variable*)
    {
        if (isset)
            GRpkgIf()->ErrPrintf(ET_ERROR, "display is read only.\n");
    }
};

struct KWent_dontplot : public KWent
{
    KWent_dontplot() { set(
        kw_dontplot,
        VTYP_BOOL, 0.0, 0.0,
        "Don't send graphics code to output device."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_nosubckt : public KWent
{
    KWent_nosubckt() { set(
        kw_nosubckt,
        VTYP_BOOL, 0.0, 0.0,
        "No subcircuit expansion."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_program : public KWent
{
    KWent_program() { set(
        kw_program,
        VTYP_STRING, 0.0, 0.0,
        "Program name."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "bad %s value, must be a string.\n", word);
                return;
            }
            CP.SetProgram(v->string());
        }
        else
            CP.SetProgram(Sp.Simulator());
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_strictnumparse : public KWent
{
    KWent_strictnumparse() { set(
        kw_strictnumparse,
        VTYP_BOOL, 0.0, 0.0,
        "Don't allow trailing characters after number."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        Sp.SetFlag(FT_STRICTNUM, isset);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};


namespace {
    // Replace %c with c in new string.
    const char *cpystr(const char *s, char c)
    {
        char *n = new char[strlen(s)];
        sprintf(n, s, c);
        return (n);
    }
}
        

struct KWent_units_catchar : public KWent
{
    KWent_units_catchar() { set(
        kw_units_catchar,
        VTYP_STRING, 0.0, 0.0,
        cpystr("Separation char, units string from number, default \'%c\'.",
            DEF_UNITS_CATCHAR)); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
            if (strlen(v->string()) > 1 || !ispunct(*v->string())) {
                error_pr(word, 0, "one punctuation character");
                return;
            }
        }
        if (isset)
            Sp.SetUnitsCatchar(*v->string());
        else
            Sp.SetUnitsCatchar(DEF_UNITS_CATCHAR);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_subc_catchar : public KWent
{
    KWent_subc_catchar() { set(
        kw_subc_catchar,
        VTYP_STRING, 0.0, 0.0,
        cpystr("Subckt name expansion field separator char, default \'%c\'.",
            DEF_SUBC_CATCHAR)); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
            if (strlen(v->string()) > 1 || !ispunct(*v->string())) {
                error_pr(word, 0, "one punctuation character");
                return;
            }
        }
        if (isset)
            Sp.SetSubcCatchar(*v->string());
        else
            Sp.SetSubcCatchar(DEF_SUBC_CATCHAR);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_subc_catmode : public KWent
{
    KWent_subc_catmode() { set(
        kw_subc_catmode,
        VTYP_STRING, 0.0, 0.0,
        "Subckt name mapping method, \"wrspice\" or \"spice3\"."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
            if (lstring::cieq(v->string(), "wrspice")) {
                Sp.SetSubcCatmode(SUBC_CATMODE_WR);
            }
            else if (lstring::cieq(v->string(), "spice3")) {
                Sp.SetSubcCatmode(SUBC_CATMODE_SPICE3);
            }
            else {
                error_pr(word, 0, "\"wrspice\" or \"spice3\"");
                return;
            }
        }
        else
            Sp.SetSubcCatmode(SUBC_CATMODE_WR);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_plot_catchar : public KWent
{
    KWent_plot_catchar() { set(
        kw_plot_catchar,
        VTYP_STRING, 0.0, 0.0,
        cpystr(
            "Separation char, plot and vector names, default \'%c\'.",
            DEF_PLOT_CATCHAR)); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
            if (strlen(v->string()) > 1 || !ispunct(*v->string())) {
                error_pr(word, 0, "one punctuation character");
                return;
            }
        }
        if (isset)
            Sp.SetPlotCatchar(*v->string());
        else
            Sp.SetPlotCatchar(DEF_PLOT_CATCHAR);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_spec_catchar : public KWent
{
    KWent_spec_catchar() { set(
        kw_spec_catchar,
        VTYP_STRING, 0.0, 0.0,
        cpystr("Char that signifies a \"special\" vector, default \'%c\'.",
            DEF_SPEC_CATCHAR)); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
            if (strlen(v->string()) > 1 || !ispunct(*v->string())) {
                error_pr(word, 0, "one punctuation character");
                return;
            }
        }
        if (isset)
            Sp.SetSpecCatchar(*v->string());
        else
            Sp.SetSpecCatchar(DEF_SPEC_CATCHAR);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_var_catchar : public KWent
{
    KWent_var_catchar() { set(
        kw_var_catchar,
        VTYP_STRING, 0.0, 0.0,
        cpystr("Shell variable concatenation character, default \'%c\'.",
            DEF_VAR_CATCHAR)); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
            if (strlen(v->string()) > 1 || !ispunct(*v->string())) {
                error_pr(word, 0, "one punctuation character");
                return;
            }
        }
        if (isset)
            CP.SetVarCatchar(*v->string());
        else
            CP.SetVarCatchar(DEF_VAR_CATCHAR);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_term : public KWent
{
    KWent_term() { set(
        kw_term,
        VTYP_STRING, 0.0, 0.0,
        "Terminal name."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "bad %s value, must be a string.\n", word);
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_trantrace : public KWent
{
    KWent_trantrace() { set(
        kw_trantrace,
        VTYP_NUM, 0.0, 2.0,
        "Transient analysis trace messages."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
            Sp.SetTranTrace(v->integer());
        }
        else
            Sp.SetTranTrace(0);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_fpemode : public KWent
{
    KWent_fpemode() { set(
        kw_fpemode,
        VTYP_NUM, 0.0, 2.0,
        "Floating-point exception handling."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
#ifdef WIN32
            if (v->integer() > 1) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "FPE signals are not available under Windows.\n");
                return;
            }
#endif
            Sp.SetFPEmode((FPEmode)v->integer());
        }
        else
            Sp.SetFPEmode(FPEdefault);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

sKW *cKeyWords::KWdebug[] = {
    new KWent_debug(),
    new KWent_display(),
    new KWent_dontplot(),
    new KWent_nosubckt(),
    new KWent_program(),
    new KWent_strictnumparse(),
    new KWent_units_catchar(),
    new KWent_subc_catchar(),
    new KWent_subc_catmode(),
    new KWent_plot_catchar(),
    new KWent_spec_catchar(),
    new KWent_var_catchar(),
    new KWent_term(),
    new KWent_trantrace(),
    new KWent_fpemode(),
    new sKW(0, 0)
};


/*************************************************************************
 *  Command Keywords
 *************************************************************************/

// filetype arguments
const char *kw_ascii            = "ascii";
const char *kw_binary           = "binary";

sKW *cKeyWords::KWft[] = {
    new sKW(kw_ascii, "Ascii rawfile."),
    new sKW(kw_binary, "Binary rawfile."),
    new sKW(0, 0)
};

// level arguments
const char *kw_i                = "i";
const char *kw_b                = "b";
const char *kw_a                = "a";

sKW *cKeyWords::KWlevel[] = {
    new sKW(kw_i, "Intermediate level for help."),
    new sKW(kw_b, "Beginner level for help."),
    new sKW(kw_a, "Advanced level for help."),
    new sKW(0, 0)
};

// specwindow arguments
const char *kw_bartlet          = "bartlet";
const char *kw_blackman         = "blackman";
const char *kw_cosine           = "cosine";
const char *kw_gaussian         = "gaussian";
const char *kw_hamming          = "hamming";
const char *kw_hanning          = "hanning";
const char *kw_none             = "none";
const char *kw_rectangular      = "rectangular";
const char *kw_triangle         = "triangle";

sKW *cKeyWords::KWspec[] = {
    new sKW(kw_bartlet,   "Bartlet (triangle) window."),
    new sKW(kw_blackman,  "Blackman order 2 window."),
    new sKW(kw_cosine,    "Hanning (cosine) window."),
    new sKW(kw_gaussian,  "Gaussian window."),
    new sKW(kw_hamming,   "Hamming window."),
    new sKW(kw_hanning,   "Hanning (cosine) window."),
    new sKW(kw_none,      "No windowing."),
    new sKW(kw_rectangular,"Rectangular window."),
    new sKW(kw_triangle,  "Bartlet (triangle) window."),
    new sKW(0, 0)
};

// units arguments
const char *kw_radians          = "radians";
const char *kw_degrees          = "degrees";

sKW *cKeyWords::KWunits[] = {
    new sKW(kw_radians, "Trig functions use radians."),
    new sKW(kw_degrees, "Trig functions use degrees."),
    new sKW(0, 0)
};

// the command configuration keywords
const char *kw_appendwrite      = "appendwrite";
const char *kw_checkiterate     = "checkiterate";
const char *kw_diff_abstol      = "diff_abstol";
const char *kw_diff_reltol      = "diff_reltol";
const char *kw_diff_vntol       = "diff_vntol";
const char *kw_dollarcmt        = "dollarcmt";
const char *kw_dpolydegree      = "dpolydegree";
const char *kw_editor           = "editor";
const char *kw_errorlog         = "errorlog";
const char *kw_filetype         = "filetype";
const char *kw_fourgridsize     = "fourgridsize";
const char *kw_helpinitxpos     = "helpinitxpos";
const char *kw_helpinitypos     = "helpinitypos";
const char *kw_helppath         = "helppath";
const char *kw_installcmdfmt    = "installcmdfmt";
const char *kw_level            = "level";
const char *kw_modpath          = "modpath";
const char *kw_mplot_cur        = "mplot_cur";
const char *kw_nfreqs           = "nfreqs";
const char *kw_nocheckupdate    = "nocheckupdate";
const char *kw_nomodload        = "nomodload";
const char *kw_nopadding        = "nopadding";
const char *kw_nopage           = "nopage";
const char *kw_noprtitle        = "noprtitle";
const char *kw_numdgt           = "numdgt";
const char *kw_printautowidth   = "printautowidth";
const char *kw_printnoheader    = "printnoheader";
const char *kw_printnoindex     = "printnoindex";
const char *kw_printnopageheader= "printnopageheader";
const char *kw_printnoscale     = "printnoscale";
const char *kw_random           = "random";
const char *kw_rawfile          = "rawfile";
const char *kw_rawfileprec      = "rawfileprec";
const char *kw_rhost            = "rhost";
const char *kw_rprogram         = "rprogram";
const char *kw_spectrace        = "spectrace";
const char *kw_specwindow       = "specwindow";
const char *kw_specwindoworder  = "specwindoworder";
const char *kw_spicepath        = "spicepath";
const char *kw_units            = "units";
const char *kw_xicpath          = "xicpath";


struct KWent_appendwrite : public KWent
{
    KWent_appendwrite() { set(
        kw_appendwrite,
        VTYP_BOOL, 0.0, 0.0,
        "Append to file with write command, no overwrite."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_checkiterate : public KWent
{
    KWent_checkiterate() { set(
        kw_checkiterate,
        VTYP_NUM, 0.0, 10.0,
        "Binary search iterations in range detect."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_diff_abstol : public KWent
{
    KWent_diff_abstol() { set(
        kw_diff_abstol,
        VTYP_REAL, DEF_diff_abstol_MIN, DEF_diff_abstol_MAX,
        "Absolute error tolerance, default " STRINGIFY(DEF_diff_abstol) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_diff_reltol : public KWent
{
    KWent_diff_reltol() { set(
        kw_diff_reltol,
        VTYP_REAL, DEF_diff_reltol_MIN, DEF_diff_reltol_MAX,
        "Relative error tolerance, default " STRINGIFY(DEF_diff_reltol) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_diff_vntol : public KWent
{
    KWent_diff_vntol() { set(
        kw_diff_vntol,
        VTYP_REAL, DEF_diff_vntol_MIN, DEF_diff_vntol_MAX,
        "Voltage error tolerance, default " STRINGIFY(DEF_diff_vntol) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_dollarcmt : public KWent
{
    KWent_dollarcmt() { set(
        kw_dollarcmt,
        VTYP_BOOL, 0.0, 0.0,
        "Forms like \" $xxx\" treated as comment start in file."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_dpolydegree : public KWent
{
    KWent_dpolydegree() { set(
        kw_dpolydegree,
        VTYP_NUM, DEF_dpolydegree_MIN, DEF_dpolydegree_MAX,
        "Poly degree for deriv function, default "
            STRINGIFY(DEF_dpolydegree) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_editor : public KWent
{
    KWent_editor() { set(
        kw_editor,
        VTYP_STRING, 0.0, 0.0,
        "Editor invocation string."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_errorlog : public KWent
{
    KWent_errorlog() { set(
        kw_errorlog,
        VTYP_STRING, 0.0, 0.0,
        "Error log file."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING && v->type() != VTYP_BOOL) {
                error_pr(word, 0, "a string or a boolean");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_filetype : public KWent
{
    KWent_filetype() { set(
        kw_filetype,
        VTYP_STRING, 0.0, 0.0,
        "Rawfile type: ascii or binary."); }

    void print(char **rstr)
    {
        sKW::print(rstr);
        char buf[256];
        for (int i = 0; KW.ft(i)->word; i++) {
            sprintf(buf, fmt2, KW.ft(i)->word, KW.ft(i)->descr);
            if (!rstr)
                TTY.send(buf);
            else
                *rstr = lstring::build_str(*rstr, buf);
        }
    }

    void callback(bool isset, variable *v)
    {
        int i = 0;
        if (isset) {
            if (v->type() != VTYP_STRING) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad filetype value.\n");
                return;
            }
            for (i = 0; KW.ft(i)->word; i++)
                if (lstring::cieq(v->string(), KW.ft(i)->word))
                    break;
            if (!KW.ft(i)->word) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad filetype keyword.\n");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_fourgridsize : public KWent
{
    KWent_fourgridsize() { set(
        kw_fourgridsize,
        VTYP_NUM, DEF_fourgridsize_MIN, DEF_fourgridsize_MAX,
        "Number of interpolation points in fourier analysis, default "
            STRINGIFY(DEF_fourgridsize) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_helpinitxpos : public KWent
{
    KWent_helpinitxpos() { set(
        kw_helpinitxpos,
        VTYP_NUM, 0.0, 2000.0,
        "Initial X position for help window."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_helpinitypos : public KWent
{
    KWent_helpinitypos() { set(
        kw_helpinitypos,
        VTYP_NUM, 0.0, 2000.0,
        "Initial Y position for help window."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_installcmdfmt : public KWent
{
    KWent_installcmdfmt() { set(
        kw_installcmdfmt,
        VTYP_STRING, 0.0, 0.0,
        "Installation command format."); }

    void callback(bool isset, variable *v)
    {
        CP.RawVarSet(word, isset, v);
        if (isset) {
            VTvalue vv;
            if (Sp.GetVar(word, VTYP_STRING, &vv))
                HLP()->set_path(vv.get_string(), false);
        }
        KWent::callback(isset, v);
    }
};

struct KWent_helppath : public KWent
{
    KWent_helppath() { set(
        kw_helppath,
        VTYP_STRING, 0.0, 0.0,
        "Path containing help files."); }

    void callback(bool isset, variable *v)
    {
        CP.RawVarSet(word, isset, v);
        if (isset) {
            VTvalue vv;
            if (Sp.GetVar(word, VTYP_STRING, &vv))
                HLP()->set_path(vv.get_string(), false);
        }
        KWent::callback(isset, v);
    }
};

struct KWent_level : public KWent
{
    KWent_level() { set(
        kw_level,
        VTYP_STRING, 0.0, 0.0,
        "Level for help (b, i, or a)."); }

    void print(char **rstr)
    {
        sKW::print(rstr);
        char buf[256];
        for (int i = 0; KW.level(i)->word; i++) {
            sprintf(buf, fmt2, KW.level(i)->word, KW.level(i)->descr);
            if (!rstr)
                TTY.send(buf);
            else
                *rstr = lstring::build_str(*rstr, buf);
        }
    }

    void callback(bool isset, variable *v)
    {
        int i = 0;
        if (isset) {
            if (v->type() != VTYP_STRING) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad level value.\n");
                return;
            }
            for (i = 0; KW.level(i)->word; i++)
                if (lstring::cieq(v->string(), KW.level(i)->word))
                    break;
            if (!KW.level(i)->word) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad level keyword.\n");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_modpath : public KWent
{
    KWent_modpath() { set(
        kw_modpath,
        VTYP_STRING, 0.0, 0.0,
        "Search path for loadable device model files."); }

    void callback(bool isset, variable *v)
    {
        CP.RawVarSet(word, isset, v);
        ToolBar()->UpdateFiles();
        KWent::callback(isset, v);
    }
};

struct KWent_mplot_cur : public KWent
{
    KWent_mplot_cur() { set(
        kw_mplot_cur,
        VTYP_STRING, 0.0, 0.0,
        "Name of current output file for check command."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_nfreqs : public KWent
{
    KWent_nfreqs() { set(
        kw_nfreqs,
        VTYP_NUM, DEF_nfreqs_MIN, DEF_nfreqs_MAX,
        "Number of frequencies in fourier command, default "
            STRINGIFY(DEF_nfreqs) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_nocheckupdate : public KWent
{
    KWent_nocheckupdate() { set(
        kw_nocheckupdate,
        VTYP_BOOL, 0.0, 0.0,
        "Don't check for updates on program start."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_nomodload : public KWent
{
    KWent_nomodload() { set(
        kw_nomodload,
        VTYP_BOOL, 0.0, 0.0,
        "Don't auto-load device model modules on WRspice startup."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_nopadding : public KWent
{
    KWent_nopadding() { set(
        kw_nopadding,
        VTYP_BOOL, 0.0, 0.0,
        "Binary rawfile is not padded."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_nopage : public KWent
{
    KWent_nopage() { set(
        kw_nopage,
        VTYP_BOOL, 0.0, 0.0,
        "Suppress page ejects."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        Sp.SetFlag(FT_NOPAGE, isset);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_noprtitle : public KWent
{
    KWent_noprtitle() { set(
        kw_noprtitle,
        VTYP_BOOL, 0.0, 0.0,
        "Don't print circuit title line when sourced."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_numdgt : public KWent
{
    KWent_numdgt() { set(
        kw_numdgt,
        VTYP_NUM, DEF_numdgt_MIN, DEF_numdgt_MAX,
        "Number of significant digits to print, default "
            STRINGIFY(DEF_numdgt) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
            CP.SetNumDigits(v->integer());
        }
        else
            CP.SetNumDigits(-1);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_printautowidth : public KWent
{
    KWent_printautowidth() { set(
        kw_printautowidth,
        VTYP_BOOL, 0.0, 0.0,
        "Scale width to fit columns in print command."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_printnoheader : public KWent
{
    KWent_printnoheader() { set(
        kw_printnoheader,
        VTYP_BOOL, 0.0, 0.0,
        "Skip top header in column mode of print command."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_printnoindex : public KWent
{
    KWent_printnoindex() { set(
        kw_printnoindex,
        VTYP_BOOL, 0.0, 0.0,
        "Skip index column in print command."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_printnopageheader : public KWent
{
    KWent_printnopageheader() { set(
        kw_printnopageheader,
        VTYP_BOOL, 0.0, 0.0,
        "Skip page header in column mode of print command."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_printnoscale : public KWent
{
    KWent_printnoscale() { set(
        kw_printnoscale,
        VTYP_BOOL, 0.0, 0.0,
        "Don't print scale in print command."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_random : public KWent
{
    KWent_random() { set(
        kw_random,
        VTYP_BOOL, 0.0, 0.0,
        "Gauss, etc. enabled to return random values."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_rawfile : public KWent
{
    KWent_rawfile() { set(
        kw_rawfile,
        VTYP_STRING, 0.0, 0.0,
        "Rawfile for plot output."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
            Sp.GetOutDesc()->set_outFile(v->string());
        }
        else
            Sp.GetOutDesc()->set_outFile(0);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_rawfileprec : public KWent
{
    KWent_rawfileprec() { set(
        kw_rawfileprec,
        VTYP_NUM, DEF_rawfileprec_MIN, DEF_rawfileprec_MAX,
        "Significant digits in rawfile, default "
            STRINGIFY(DEF_rawfileprec) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
            Sp.GetOutDesc()->set_outNdgts(v->integer());
        }
        else
            Sp.GetOutDesc()->set_outNdgts(0);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_rhost : public KWent
{
    KWent_rhost() { set(
        kw_rhost,
        VTYP_STRING, 0.0, 0.0,
        "Host name for remote simulations."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_rprogram : public KWent
{
    KWent_rprogram() { set(
        kw_rprogram,
        VTYP_STRING, 0.0, 0.0,
        "Remote simulator name."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_spectrace : public KWent
{
    KWent_spectrace() { set(
        kw_spectrace,
        VTYP_BOOL, 0.0, 0.0,
        "Spec command, print trace in FFT."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_specwindow : public KWent
{
    KWent_specwindow() { set(
        kw_specwindow,
        VTYP_STRING, 0.0, 0.0,
        "Spec command, Fourier analysis window type."); }

    void print(char **rstr)
    {
        sKW::print(rstr);
        char buf[256];
        for (int i = 0; KW.spec(i)->word; i++) {
            sprintf(buf, fmt2, KW.spec(i)->word, KW.spec(i)->descr);
            if (!rstr)
                TTY.send(buf);
            else
                *rstr = lstring::build_str(*rstr, buf);
        }
    }

    void callback(bool isset, variable *v)
    {
        int i = 0;
        if (isset) {
            if (v->type() != VTYP_STRING) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad specwindow value.\n");
                return;
            }
            for (i = 0; KW.spec(i)->word; i++)
                if (lstring::cieq(v->string(), KW.spec(i)->word))
                    break;
            if (!KW.spec(i)->word) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad specwindow keyword.\n");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_specwindoworder : public KWent
{
    KWent_specwindoworder() { set(
        kw_specwindoworder,
        VTYP_NUM, 2.0, 8.0,
        "Spec command, gaussian window order, default "
            STRINGIFY(DEF_specwindoworder) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_spicepath : public KWent
{
    KWent_spicepath() { set(
        kw_spicepath,
        VTYP_STRING, 0.0, 0.0,
        "Path to use in aspice command."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_units : public KWent
{
    KWent_units() { set(
        kw_units,
        VTYP_STRING, 0.0, 0.0,
        "if 'degrees' trig functions don't use radians."); }

    void print(char **rstr)
    {
        sKW::print(rstr);
        char buf[256];
        for (int i = 0; KW.units(i)->word; i++) {
            sprintf(buf, fmt2, KW.units(i)->word, KW.units(i)->descr);
            if (!rstr)
                TTY.send(buf);
            else
                *rstr = lstring::build_str(*rstr, buf);
        }
    }

    void callback(bool isset, variable *v)
    {
        int i = 0;
        if (isset) {
            if (v->type() != VTYP_STRING) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad units value.\n");
                return;
            }
            for (i = 0; KW.units(i)->word; i++)
                if (lstring::cieq(v->string(), KW.units(i)->word))
                    break;
            if (!KW.units(i)->word) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad units keyword.\n");
                return;
            }
            if (i == 1)
                cx_degrees = true;
            else
                cx_degrees = false;
        }
        else
            cx_degrees = false;
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_xicpath : public KWent
{
    KWent_xicpath() { set(
        kw_xicpath,
        VTYP_STRING, 0.0, 0.0,
        "Path to Xic graphical editor."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

sKW *cKeyWords::KWcmds[] = {
    new KWent_appendwrite(),
    new KWent_checkiterate(),
    new KWent_diff_abstol(),
    new KWent_diff_reltol(),
    new KWent_diff_vntol(),
    new KWent_dollarcmt(),
    new KWent_dpolydegree(),
    new KWent_editor(),
    new KWent_errorlog(),
    new KWent_filetype(),
    new KWent_fourgridsize(),
    new KWent_helpinitxpos(),
    new KWent_helpinitypos(),
    new KWent_helppath(),
    new KWent_installcmdfmt(),
    new KWent_level(),
    new KWent_modpath(),
    new KWent_mplot_cur(),
    new KWent_nfreqs(),
    new KWent_nocheckupdate(),
    new KWent_nomodload(),
    new KWent_nopadding(),
    new KWent_nopage(),
    new KWent_noprtitle(),
    new KWent_numdgt(),
    new KWent_printautowidth(),
    new KWent_printnoheader(),
    new KWent_printnoindex(),
    new KWent_printnopageheader(),
    new KWent_printnoscale(),
    new KWent_random(),
    new KWent_rawfile(),
    new KWent_rawfileprec(),
    new KWent_rhost(),
    new KWent_rprogram(),
    new KWent_spectrace(),
    new KWent_specwindow(),
    new KWent_specwindoworder(),
    new KWent_spicepath(),
    new KWent_units(),
    new KWent_xicpath(),
    new sKW(0, 0)
};


/*************************************************************************
 *  Shell Keywords
 *************************************************************************/

// the shell keywords
const char *kw_cktvars          = "cktvars";
const char *kw_height           = "height";
const char *kw_history          = "history";
const char *kw_ignoreeof        = "ignoreeof";
const char *kw_noaskquit        = "noaskquit";
const char *kw_nocc             = "nocc";
const char *kw_noclobber        = "noclobber";
const char *kw_noedit           = "noedit";
const char *kw_noerrwin         = "noerrwin";
const char *kw_noglob           = "noglob";
const char *kw_nomoremode       = "nomoremode";
const char *kw_nonomatch        = "nonomatch";
const char *kw_nosort           = "nosort";
const char *kw_prompt           = "prompt";
const char *kw_sourcepath       = "sourcepath";
const char *kw_unixcom          = "unixcom";
const char *kw_width            = "width";
const char *kw_wmfocusfix       = "wmfocusfix";


struct KWent_cktvars : public KWent
{
    KWent_cktvars() { set(
        kw_cktvars,
        VTYP_BOOL, 0.0, 0.0,
        "Commands recognize variables set in SPICE .options line."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_height : public KWent
{
    KWent_height() { set(
        kw_height,
        VTYP_NUM, 2.0, 1000.0,
        "Screen height in characters."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                TTY.setheight((int)v->real());
                v->set_integer(TTY.getheight());
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
            TTY.setheight(v->integer());
        }
        else
            TTY.setheight(0);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_history : public KWent
{
    KWent_history() { set(
        kw_history,
        VTYP_NUM, 0.0, 1000.0,
        "Number of saved commands."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                CP.SetMaxHistLength((int)v->real());
                v->set_integer(CP.MaxHistLength());
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
            CP.SetMaxHistLength(v->integer());
        }
        else
            CP.SetMaxHistLength(CP_DefHistLen);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_ignoreeof : public KWent
{
    KWent_ignoreeof() { set(
        kw_ignoreeof,
        VTYP_BOOL, 0.0, 0.0,
        "Ignore end of file (^D)."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.SetFlag(CP_IGNOREEOF, isset);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_noaskquit : public KWent
{
    KWent_noaskquit() { set(
        kw_noaskquit,
        VTYP_BOOL, 0.0, 0.0,
        "Don't verify before termination."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_nocc : public KWent
{
    KWent_nocc() { set(
        kw_nocc,
        VTYP_BOOL, 0.0, 0.0,
        "No command completion."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.SetFlag(CP_NOCC, isset);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_noclobber : public KWent
{
    KWent_noclobber() { set(
        kw_noclobber,
        VTYP_BOOL, 0.0, 0.0,
        "Don't overwrite files when redirecting output."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.SetFlag(CP_NOCLOBBER, isset);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_noedit : public KWent
{
    KWent_noedit() { set(
        kw_noedit,
        VTYP_BOOL, 0.0, 0.0,
        "Don't use the command line editing facility."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        if (TTY.infile() && isatty(TTY.infileno()))
            CP.SetupTty(TTY.infileno(), true);
        if (!CP.GetFlag(CP_LOCK_NOEDIT))
            CP.SetFlag(CP_NOEDIT, isset);
        if (TTY.infile() && isatty(TTY.infileno()))
            CP.SetupTty(TTY.infileno(), false);
#ifdef TIOCSTI
        if (TTY.infile() && isatty(TTY.infileno())) {
            if (!CP.GetFlag(CP_NOEDIT))
                TTY.out_printf("\n");
            char end[2];
            end[0] = '\n';
            end[1] = 0;
            ioctl(TTY.infileno(), TIOCSTI, &end);
        }
#endif
        KWent::callback(isset, v);
    }
};

struct KWent_noerrwin : public KWent
{
    KWent_noerrwin() { set(
        kw_noerrwin,
        VTYP_BOOL, 0.0, 0.0,
        "Don't use separate window for error messages."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        Sp.SetFlag(FT_NOERRWIN, isset);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_noglob : public KWent
{
    KWent_noglob() { set(
        kw_noglob,
        VTYP_BOOL, 0.0, 0.0,
        "Don't expand wildcard characters."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.SetFlag(CP_NOGLOB, isset);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_nomoremode : public KWent
{
    KWent_nomoremode() { set(
        kw_nomoremode,
        VTYP_BOOL, 0.0, 0.0,
        "No 'more' mode."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        TTY.setmore(!isset);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_nonomatch : public KWent
{
    KWent_nonomatch() { set(
        kw_nonomatch,
        VTYP_BOOL, 0.0, 0.0,
        "Use wildcard characters literally if no match."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.SetFlag(CP_NONOMATCH, isset);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_nosort : public KWent
{
    KWent_nosort() { set(
        kw_nosort,
        VTYP_BOOL, 0.0, 0.0,
        "Suppress sorting of output listings."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_prompt : public KWent
{
    KWent_prompt() { set(
        kw_prompt,
        VTYP_STRING, 0.0, 0.0,
        "Prompt string."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
            CP.SetPromptString(v->string());
        }
        else
            CP.SetPromptString(0);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_sourcepath : public KWent
{
    KWent_sourcepath() { set(
        kw_sourcepath,
        VTYP_STRING, 0.0, 0.0,
        "Search path for input files."); }

    void callback(bool isset, variable *v)
    {
        CP.RawVarSet(word, isset, v);
        ToolBar()->UpdateFiles();
        KWent::callback(isset, v);
    }
};

struct KWent_unixcom : public KWent
{
    KWent_unixcom() { set(
        kw_unixcom,
        VTYP_BOOL, 0.0, 0.0,
        "Execute operating system commands."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.SetFlag(CP_DOUNIXCOM, isset);
        if (isset) {
            char *s = getenv("PATH");
            if (s)
                CP.Rehash(s, !CP.GetFlag(CP_NOCC));
            else
                GRpkgIf()->ErrPrintf(ET_WARN, "no PATH in environment.\n");
        }
        else
            Cmds.CcSetup();
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_width : public KWent
{
    KWent_width() { set(
        kw_width,
        VTYP_NUM, 20.0, 1000.0,
        "Screen width in characters."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                TTY.setwidth((int)v->real());
                v->set_integer(TTY.getwidth());
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
            TTY.setwidth(v->integer());
        }
        else
            TTY.setwidth(0);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_wmfocusfix : public KWent
{
    KWent_wmfocusfix() { set(
        kw_wmfocusfix,
        VTYP_BOOL, 0.0, 0.0,
        "Tell old window manager to revert focus to console after pop-up."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

sKW *cKeyWords::KWshell[] = {
    new KWent_cktvars(),
    new KWent_height(),
    new KWent_history(),
    new KWent_ignoreeof(),
    new KWent_noaskquit(),
    new KWent_nocc(),
    new KWent_noclobber(),
    new KWent_noedit(),
    new KWent_noerrwin(),
    new KWent_noglob(),
    new KWent_nomoremode(),
    new KWent_nonomatch(),
    new KWent_nosort(),
    new KWent_prompt(),
    new KWent_sourcepath(),
    new KWent_unixcom(),
    new KWent_width(),
    new KWent_wmfocusfix(),
    new sKW(0, 0)
};


/*************************************************************************
 *  Simulator Keywords
 *************************************************************************/

// the method arguments

sKW *cKeyWords::KWmethod[] = {
    new sKW(spkw_trap, "Trapezoidal integration."),
    new sKW(spkw_gear, "Gear integration."),
    new sKW(0, 0)
};

// the optmerge arguments

sKW *cKeyWords::KWoptmerge[] = {
    new sKW(spkw_global, "Shell variables override .options."),
    new sKW(spkw_local, "Shell variables are overridden by .options."),
    new sKW(spkw_noshell, "Shell variables are ignored."),
    new sKW(0, 0)
};

sKW *cKeyWords::KWparhier[] = {
    new sKW(spkw_global, "Global parameter definitions override local."),
    new sKW(spkw_local, "Local parameter definitions override global."),
    new sKW(0, 0)
};

// the steptype arguments

sKW *cKeyWords::KWstep[] = {
    new sKW(spkw_interpolate,
        "Interpolate user time steps during transient analysis."),
    new sKW(spkw_hitusertp,
        "Perform transient analysis at user timepoints."),
    new sKW(spkw_nousertp,
        "Output raw time points in transient analysis."),
    new sKW(spkw_fixedstep,
        "Force internal time delta = transient analysis step."),
    new sKW(0, 0)
};

// Spice option keywords

// parser keywords
const char *kw_modelcard        = "modelcard";
const char *kw_pexnodes         = "pexnodes";
const char *kw_nobjthack        = "nobjthack";
const char *kw_subend           = "subend";
const char *kw_subinvoke        = "subinvoke";
const char *kw_substart         = "substart";


namespace {
    // Function to check/set interface-specific variables.  If true is
    // returned, the variable is read only.  Otherwise, the variable
    // has been accepted.
    // Note: interface-specific variables are those that appear in
    // the Spice options string.  See analysis/optsetp.c for list.
    //
    bool checknset(const char *name, bool isset, variable *v)
    {
        if (Sp.CurPlot()) {
            for (variable *tv = Sp.CurPlot()->environment(); tv;
                    tv = tv->next()) {
                if (lstring::cieq(tv->name(), name)) {
                    GRpkgIf()->ErrPrintf(ET_ERROR,
            "can't set %s, it is in the current plot environment.\n", name);
                    return (true);
                }
            }
        }

        IFdata data;
        data.type = IF_FLAG;
        data.v.rValue = 0.0;
        if (isset && v) {
            switch (v->type()) {
            case VTYP_BOOL:
                data.v.iValue = v->boolean();
                break;
            case VTYP_NUM:
                data.v.iValue = v->integer();
                data.type = IF_INTEGER;
                break;
            case VTYP_REAL:
                data.v.rValue = v->real();
                data.type = IF_REAL;
                break;
            case VTYP_STRING:
                data.v.sValue = v->string();
                data.type = IF_STRING;
                break;
            case VTYP_LIST:
            default:
                // SetOption() can't handle lists
                return (true);
            }
        }
        Sp.SetOption(isset, name, &data);
        CP.RawVarSet(name, isset, v);
        return (false);
    }
}


struct KWent_abstol : public KWent
{
    KWent_abstol() { set(
        spkw_abstol,
        VTYP_REAL, DEF_abstol_MIN, DEF_abstol_MAX,
        "Absolute error tolerance, default " STRINGIFY(DEF_abstol) ".") ;}

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_chgtol : public KWent
{
    KWent_chgtol() { set(
        spkw_chgtol,
        VTYP_REAL, DEF_chgtol_MIN, DEF_chgtol_MAX,
        "Charge tolerance, default " STRINGIFY(DEF_chgtol) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_dcmu : public KWent
{
    KWent_dcmu() { set(
        spkw_dcmu,
        VTYP_REAL, DEF_dcMu_MIN, DEF_dcMu_MAX,
        "DC last iteration mixing, default " STRINGIFY(DEF_dcMu) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_defad : public KWent
{
    KWent_defad() { set(
        spkw_defad,
        VTYP_REAL, DEF_defaultMosAD_MIN, DEF_defaultMosAD_MAX,
        "MOS drain diffusion area, default 0."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_defas : public KWent
{
    KWent_defas() { set(
        spkw_defas,
        VTYP_REAL, DEF_defaultMosAS_MIN, DEF_defaultMosAS_MAX,
        "MOS source diffusion area, default 0."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_defl : public KWent
{
    KWent_defl() { set(
        spkw_defl,
        VTYP_REAL, DEF_defaultMosL_MIN, DEF_defaultMosL_MAX,
        "MOS channel length, default set by model."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_defw : public KWent
{
    KWent_defw() { set(
        spkw_defw,
        VTYP_REAL, DEF_defaultMosW_MIN, DEF_defaultMosW_MAX,
        "MOS channel width, default set by model."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_delmin : public KWent
{
    KWent_delmin() { set(
        spkw_delmin,
        VTYP_REAL, DEF_delMin_MIN, DEF_delMin_MAX,
        "Minimum time step."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_dphimax : public KWent
{
    KWent_dphimax() { set(
        spkw_dphimax,
        VTYP_REAL, DEF_dphiMax_MIN, DEF_dphiMax_MAX,
        "Max phase change per time step, default "
            STRINGIFY(DEF_dphiMax) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_gmax : public KWent
{
    KWent_gmax() { set(
        spkw_gmax,
        VTYP_REAL, DEF_gmax_MIN, DEF_gmax_MAX,
        "Maximum conductance allowed, default " STRINGIFY(DEF_gmax) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_gmin : public KWent
{
    KWent_gmin() { set(
        spkw_gmin,
        VTYP_REAL, DEF_gmin_MIN, DEF_gmin_MAX,
        "Minimum conductance allowed, default " STRINGIFY(DEF_gmin) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_maxdata : public KWent
{
    KWent_maxdata() { set(
        spkw_maxdata,
        VTYP_REAL, DEF_maxData_MIN, DEF_maxData_MAX,
        "Max KB size of internal data per plot, default "
        STRINGIFY(DEF_maxData) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_minbreak : public KWent
{
    KWent_minbreak() { set(
        spkw_minbreak,
        VTYP_REAL, DEF_minBreak_MIN, DEF_minBreak_MAX,
        "Minimum interval for break."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_pivrel : public KWent
{
    KWent_pivrel() { set(
        spkw_pivrel,
        VTYP_REAL, DEF_pivotRelTol_MIN, DEF_pivotRelTol_MAX,
        "Minimum relative pivot value, default "
            STRINGIFY(DEF_pivotRelTol) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_pivtol : public KWent
{
    KWent_pivtol() { set(
        spkw_pivtol,
        VTYP_REAL, DEF_pivotAbsTol_MIN, DEF_pivotAbsTol_MAX,
        "Minimum pivot value, default " STRINGIFY(DEF_pivotAbsTol) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_rampup : public KWent
{
    KWent_rampup() { set(
        spkw_rampup,
        VTYP_REAL, DEF_rampup_MIN, DEF_rampup_MAX,
        "Transient source ramp-up time, default " STRINGIFY(DEF_rampup) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_reltol : public KWent
{
    KWent_reltol() { set(
        spkw_reltol,
        VTYP_REAL, DEF_reltol_MIN, DEF_reltol_MAX,
        "Relative error tolerance, default " STRINGIFY(DEF_reltol) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_temp : public KWent
{
    KWent_temp() { set(
        spkw_temp,
        VTYP_REAL, DEF_temp_MIN - wrsCONSTCtoK, DEF_temp_MAX - wrsCONSTCtoK,
        "Circuit operating temperature, default 25C."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_tnom : public KWent
{
    KWent_tnom() { set(
        spkw_tnom,
        VTYP_REAL, DEF_nomTemp_MIN - wrsCONSTCtoK,
            DEF_nomTemp_MAX - wrsCONSTCtoK,
        "Nominal parameter temperature, default 25C."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_trapratio : public KWent
{
    KWent_trapratio() { set(
        spkw_trapratio,
        VTYP_REAL, DEF_trapRatio_MIN, DEF_trapRatio_MAX,
        "Trapezoid integration convergence test ratio, default "
            STRINGIFY(DEF_trapRatio) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_trtol : public KWent
{
    KWent_trtol() { set(
        spkw_trtol,
        VTYP_REAL, DEF_trtol_MIN, DEF_trtol_MAX,
        "Truncation error factor, default " STRINGIFY(DEF_trtol) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_vntol : public KWent
{
    KWent_vntol() { set(
        spkw_vntol,
        VTYP_REAL, DEF_voltTol_MIN, DEF_voltTol_MAX,
        "Voltage error tolerance, default " STRINGIFY(DEF_voltTol) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_xmu : public KWent
{
    KWent_xmu() { set(
        spkw_xmu,
        VTYP_REAL, DEF_xmu_MIN, DEF_xmu_MAX,
        "Trapezoid/Euler mixing, default " STRINGIFY(DEF_xmu) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max) {
                double dval = v->integer();
                v->set_real(dval);
            }
            else if (!(v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max)) {
                error_pr(word, 0, pr_real(min, max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_bypass : public KWent
{
    KWent_bypass() { set(
        spkw_bypass,
        VTYP_NUM, DEF_bypass_MIN, DEF_bypass_MAX,
        "Bypass computation if values unchanged."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL)
                v->set_integer(v->real() != 0.0);
            else if (v->type() == VTYP_NUM)
                v->set_integer(v->integer() != 0);
            else if (v->type() == VTYP_BOOL)
                v->set_integer(1);
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_gminsteps : public KWent
{
    KWent_gminsteps() { set(
        spkw_gminsteps,
        VTYP_NUM, DEF_numGminSteps_MIN, DEF_numGminSteps_MAX,
        "Number of gmin steps, default " STRINGIFY(DEF_numGminSteps) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_interplev : public KWent
{
    KWent_interplev() { set(
        spkw_interplev,
        VTYP_NUM, DEF_polydegree_MIN, DEF_polydegree_MAX,
        "Output interpolation polynomial degree, default "
            STRINGIFY(DEF_polydegree) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_itl1 : public KWent
{
    KWent_itl1() { set(
        spkw_itl1,
        VTYP_NUM, DEF_dcMaxIter_MIN, DEF_dcMaxIter_MAX,
        "DC iteration limit, default " STRINGIFY(DEF_itl1) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_itl2 : public KWent
{
    KWent_itl2() { set(
        spkw_itl2,
        VTYP_NUM, DEF_dcTrcvMaxIter_MIN, DEF_dcTrcvMaxIter_MAX,
        "DC transfer curve iteration limit, default "
            STRINGIFY(DEF_itl2) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_itl2gmin : public KWent
{
    KWent_itl2gmin() { set(
        spkw_itl2gmin,
        VTYP_NUM, DEF_dcOpGminMaxIter_MIN, DEF_dcOpGminMaxIter_MAX,
        "DCOP dynamic gmin stepping iteration limit, default "
            STRINGIFY(DEF_itl2gmin) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_itl2src : public KWent
{
    KWent_itl2src() { set(
        spkw_itl2src,
        VTYP_NUM, DEF_dcOpSrcMaxIter_MIN, DEF_dcOpSrcMaxIter_MAX,
        "DCOP dynamic source stepping iteration limit, default "
            STRINGIFY(DEF_itl2src) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_itl4 : public KWent
{
    KWent_itl4() { set(
        spkw_itl4,
        VTYP_NUM, DEF_tranMaxIter_MIN, DEF_tranMaxIter_MAX,
        "Transient iteration limit, default " STRINGIFY(DEF_itl4) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

#ifdef WITH_THREADS
struct KWent_loadthrds : public KWent
{
    KWent_loadthrds() { set(
        spkw_loadthrds,
        VTYP_NUM, DEF_loadThreads_MIN, DEF_loadThreads_MAX,
        "Number of loading threads, default " STRINGIFY(DEF_loadThreads) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_loopthrds : public KWent
{
    KWent_loopthrds() { set(
        spkw_loopthrds,
        VTYP_NUM, DEF_loopThreads_MIN, DEF_loopThreads_MAX,
        "Number of looping threads, default " STRINGIFY(DEF_loopThreads) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};
#endif

struct KWent_maxord : public KWent
{
    KWent_maxord() { set(
        spkw_maxord,
        VTYP_NUM, DEF_maxOrder_MIN, DEF_maxOrder_MAX,
        "Maximum order of integration method, default "
            STRINGIFY(DEF_maxOrder) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_srcsteps : public KWent
{
    KWent_srcsteps() { set(
        spkw_srcsteps,
        VTYP_NUM, DEF_numSrcSteps_MIN, DEF_numSrcSteps_MAX,
        "Number of source steps, default " STRINGIFY(DEF_numSrcSteps) "."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() == VTYP_REAL && v->real() >= min &&
                    v->real() <= max) {
                int val = (int)v->real();
                v->set_integer(val);
            }
            else if (!(v->type() == VTYP_NUM && v->integer() >= min &&
                    v->integer() <= max)) {
                error_pr(word, 0, pr_integer((int)min, (int)max));
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_dcoddstep : public KWent
{
    KWent_dcoddstep() { set(
        spkw_dcoddstep,
        VTYP_BOOL, 0.0, 0.0,
        "DC sweep will include off-step end value."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_extprec : public KWent
{
    KWent_extprec() { set(
        spkw_extprec,
        VTYP_BOOL, 0.0, 0.0,
        "Extra precision used when solving equations."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_forcegmin : public KWent
{
    KWent_forcegmin() { set(
        spkw_forcegmin,
        VTYP_BOOL, 0.0, 0.0,
        "Enforce min gmin conductivity to ground on all nodes, always."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_gminfirst : public KWent
{
    KWent_gminfirst() { set(
        spkw_gminfirst,
        VTYP_BOOL, 0.0, 0.0,
        "Try gmin stepping before source stepping."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_hspice : public KWent
{
    KWent_hspice() { set(
        spkw_hspice,
        VTYP_BOOL, 0.0, 0.0,
        "Silence unhandled HSPICE parameter warnings."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_jjaccel : public KWent
{
    KWent_jjaccel() { set(
        spkw_jjaccel,
        VTYP_BOOL, 0.0, 0.0,
        "Accelerate Josephson-only simulations."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_noiter : public KWent
{
    KWent_noiter() { set(
        spkw_noiter,
        VTYP_BOOL, 0.0, 0.0,
        "No transient iterations past predistor."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_nojjtp : public KWent
{
    KWent_nojjtp() { set(
        spkw_nojjtp,
        VTYP_BOOL, 0.0, 0.0,
        "No Josephson phase change timestep, use trunc error."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_noopiter : public KWent
{
    KWent_noopiter() { set(
        spkw_noopiter,
        VTYP_BOOL, 0.0, 0.0,
        "Go directly to gmin stepping."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_noshellopts : public KWent
{
    KWent_noshellopts() { set(
        spkw_noshellopts,
        VTYP_BOOL, 0.0, 0.0,
        "No use of shell-set spice options in analysis."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_noklu : public KWent
{
    KWent_noklu() { set(
        spkw_noklu,
        VTYP_BOOL, 0.0, 0.0,
        "Don't use KLU for matrix factor and solve."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_nomatsort : public KWent
{
    KWent_nomatsort() { set(
        spkw_nomatsort,
        VTYP_BOOL, 0.0, 0.0,
        "Don't sort sparse matrix before solve."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_oldlimit : public KWent
{
    KWent_oldlimit() { set(
        spkw_oldlimit,
        VTYP_BOOL, 0.0, 0.0,
        "Use Spice2 type limiting for MOS."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_renumber : public KWent
{
    KWent_renumber() { set(
        spkw_renumber,
        VTYP_BOOL, 0.0, 0.0,
        "Renumber source lines after subcircuit expansion."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_savecurrent : public KWent
{
    KWent_savecurrent() { set(
        spkw_savecurrent,
        VTYP_BOOL, 0.0, 0.0,
        "Save all device currents during analysis."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_spice3 : public KWent
{
    KWent_spice3() { set(
        spkw_spice3,
        VTYP_BOOL, 0.0, 0.0,
        "Use Spice3 timestep control."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_trapcheck : public KWent
{
    KWent_trapcheck() { set(
        spkw_trapcheck,
        VTYP_BOOL, 0.0, 0.0,
        "Perform trapezoid integration convergence test."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_trytocompact : public KWent
{
    KWent_trytocompact() { set(
        spkw_trytocompact,
        VTYP_BOOL, 0.0, 0.0,
        "Compress LTRA history list."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_useadjoint : public KWent
{
    KWent_useadjoint() { set(
        spkw_useadjoint,
        VTYP_BOOL, 0.0, 0.0,
        "Compute adjoint matrix and solve for current in some devices."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_method : public KWent
{
    KWent_method() { set(
        spkw_method,
        VTYP_STRING, 0.0, 0.0,
        "Type of integration, trapezoid or gear."); }

    void print(char **rstr)
    {
        sKW::print(rstr);
        char buf[256];
        for (int i = 0; KW.method(i)->word; i++) {
            sprintf(buf, fmt2, KW.method(i)->word, KW.method(i)->descr);
            if (!rstr)
                TTY.send(buf);
            else
                *rstr = lstring::build_str(*rstr, buf);
        }
    }

    void callback(bool isset, variable *v)
    {
        int i = 0;
        if (isset) {
            if (v->type() != VTYP_STRING) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad method value.\n");
                return;
            }
            for (i = 0; KW.method(i)->word; i++)
                if (lstring::cieq(v->string(), KW.method(i)->word))
                    break;
            if (!KW.method(i)->word) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad method keyword.\n");
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_optmerge : public KWent
{
    KWent_optmerge() { set(
        spkw_optmerge,
        VTYP_STRING, 0.0, 0.0,
        "How shell variables merge with .options."); }

    void print(char **rstr)
    {
        sKW::print(rstr);
        char buf[256];
        for (int i = 0; KW.optmerge(i)->word; i++) {
            sprintf(buf, fmt2, KW.optmerge(i)->word, KW.optmerge(i)->descr);
            if (!rstr)
                TTY.send(buf);
            else
                *rstr = lstring::build_str(*rstr, buf);
        }
    }

    void callback(bool isset, variable *v)
    {
        int i = 0;
        if (isset) {
            if (v->type() != VTYP_STRING) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad optmerge value.\n");
                return;
            }
            for (i = 0; KW.optmerge(i)->word; i++)
                if (lstring::cieq(v->string(), KW.optmerge(i)->word))
                    break;
            if (!KW.optmerge(i)->word) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad optmerge keyword.\n");
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_parhier : public KWent
{
    KWent_parhier() { set(
        spkw_parhier,
        VTYP_STRING, 0.0, 0.0,
        "Subcircuit parameter scope, local (default) or global."); }

    void print(char **rstr)
    {
        sKW::print(rstr);
        char buf[256];
        for (int i = 0; KW.parhier(i)->word; i++) {
            sprintf(buf, fmt2, KW.parhier(i)->word, KW.parhier(i)->descr);
            if (!rstr)
                TTY.send(buf);
            else
                *rstr = lstring::build_str(*rstr, buf);
        }
    }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
            int i;
            for (i = 0; KW.parhier(i)->word; i++) {
                if (lstring::cieq(v->string(), KW.parhier(i)->word))
                    break;
            }
            if (!KW.parhier(i)->word) {
                error_pr(word, 0, "\"local\" or \"global\"");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_steptype : public KWent
{
    KWent_steptype() { set(
        spkw_steptype,
        VTYP_STRING, 0.0, 0.0,
        "Transient output: interpolate, hitusertp, nousertp."); }

    void print(char **rstr)
    {
        sKW::print(rstr);
        char buf[256];
        for (int i = 0; KW.step(i)->word; i++) {
            sprintf(buf, fmt2, KW.step(i)->word, KW.step(i)->descr);
            if (!rstr)
                TTY.send(buf);
            else
                *rstr = lstring::build_str(*rstr, buf);
        }
    }

    void callback(bool isset, variable *v)
    {
        int i = 0;
        if (isset) {
            if (v->type() != VTYP_STRING) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad steptype value.\n");
                return;
            }
            for (i = 0; KW.step(i)->word; i++)
                if (lstring::cieq(v->string(), KW.step(i)->word))
                    break;
            if (!KW.step(i)->word) {
                GRpkgIf()->ErrPrintf(ET_ERROR, "bad steptype keyword.\n");
                return;
            }
        }
        if (checknset(word, isset, v))
            return;
        KWent::callback(isset, v);
    }
};

struct KWent_modelcard : public KWent
{
    KWent_modelcard() { set(
        kw_modelcard,
        VTYP_STRING, 0.0, 0.0,
        "Name of model card, default .model."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_pexnodes : public KWent
{
    KWent_pexnodes() { set(
        kw_pexnodes,
        VTYP_BOOL, 0.0, 0.0,
        "Parameter expand node names."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_nobjthack : public KWent
{
    KWent_nobjthack() { set(
        kw_nobjthack,
        VTYP_BOOL, 0.0, 0.0,
        "Assume BJT's have four nodes."); }

    void callback(bool isset, variable *v)
    {
        if (isset)
            v->set_boolean(true);
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_subend : public KWent
{
    KWent_subend() { set(
        kw_subend,
        VTYP_STRING, 0.0, 0.0,
        "End subcircuits, default .ends."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_subinvoke : public KWent
{
    KWent_subinvoke() { set(
        kw_subinvoke,
        VTYP_STRING, 0.0, 0.0,
        "Prefix to invoke subcircuits, default 'x'."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

struct KWent_substart : public KWent
{
    KWent_substart() { set(
        kw_substart,
        VTYP_STRING, 0.0, 0.0,
        "Start subcircuit definition, default .subckt."); }

    void callback(bool isset, variable *v)
    {
        if (isset) {
            if (v->type() != VTYP_STRING) {
                error_pr(word, 0, "a string");
                return;
            }
        }
        CP.RawVarSet(word, isset, v);
        KWent::callback(isset, v);
    }
};

sKW *cKeyWords::KWsim[] = {
    new KWent_abstol(),
    new KWent_bypass(),
    new KWent_chgtol(),
    new KWent_dcmu(),
    new KWent_dcoddstep(),
    new KWent_defad(),
    new KWent_defas(),
    new KWent_defl(),
    new KWent_defw(),
    new KWent_delmin(),
    new KWent_dphimax(),
    new KWent_extprec(),
    new KWent_forcegmin(),
    new KWent_gmax(),
    new KWent_gmin(),
    new KWent_gminfirst(),
    new KWent_gminsteps(),
    new KWent_hspice(),
    new KWent_interplev(),
    new KWent_itl1(),
    new KWent_itl2(),
    new KWent_itl2gmin(),
    new KWent_itl2src(),
    new KWent_itl4(),
    new KWent_jjaccel(),
#ifdef WITH_THREADS
    new KWent_loadthrds(),
    new KWent_loopthrds(),
#endif
    new KWent_maxdata(),
    new KWent_maxord(),
    new KWent_method(),
    new KWent_minbreak(),
    new KWent_modelcard(),
    new KWent_pexnodes(),
    new KWent_nobjthack(),
    new KWent_noiter(),
    new KWent_nojjtp(),
    new KWent_noopiter(),
    new KWent_noshellopts(),
    new KWent_noklu(),
    new KWent_nomatsort(),
    new KWent_oldlimit(),
    new KWent_optmerge(),
    new KWent_parhier(),
    new KWent_pivrel(),
    new KWent_pivtol(),
    new KWent_rampup(),
    new KWent_reltol(),
    new KWent_renumber(),
    new KWent_savecurrent(),
    new KWent_spice3(),
    new KWent_srcsteps(),
    new KWent_steptype(),
    new KWent_subend(),
    new KWent_subinvoke(),
    new KWent_substart(),
    new KWent_temp(),
    new KWent_tnom(),
    new KWent_trapcheck(),
    new KWent_trapratio(),
    new KWent_trtol(),
    new KWent_trytocompact(),
    new KWent_useadjoint(),
    new KWent_vntol(),
    new KWent_xmu(),

/* com_usrset needs these alphabetized 
    new KWent_abstol(),
    new KWent_chgtol(),
    new KWent_dcmu(),
    new KWent_defad(),
    new KWent_defas(),
    new KWent_defl(),
    new KWent_defw(),
    new KWent_delmin(),
    new KWent_dphimax(),
    new KWent_gmax(),
    new KWent_gmin(),
    new KWent_maxdata(),
    new KWent_minbreak(),
    new KWent_pivrel(),
    new KWent_pivtol(),
    new KWent_reltol(),
    new KWent_temp(),
    new KWent_tnom(),
    new KWent_trapratio(),
    new KWent_trtol(),
    new KWent_vntol(),
    new KWent_xmu(),

    new KWent_bypass(),
    new KWent_gminsteps(),
    new KWent_interplev(),
    new KWent_itl1(),
    new KWent_itl2(),
    new KWent_itl2gmin(),
    new KWent_itl2src(),
    new KWent_itl4(),
#ifdef WITH_THREADS
    new KWent_loadthrds(),
    new KWent_loopthrds(),
#endif
    new KWent_maxord(),
    new KWent_srcsteps(),

    new KWent_dcoddstep(),
    new KWent_extprec(),
    new KWent_forcegmin(),
    new KWent_gminfirst(),
    new KWent_hspice(),
    new KWent_jjaccel(),
    new KWent_noiter(),
    new KWent_nojjtp(),
    new KWent_noopiter(),
    new KWent_noshellopts(),
    new KWent_noklu(),
    new KWent_nomatsort(),
    new KWent_oldlimit(),
    new KWent_renumber(),
    new KWent_savecurrent(),
    new KWent_spice3(),
    new KWent_trapcheck(),
    new KWent_trytocompact(),
    new KWent_useadjoint(),

    new KWent_method(),
    new KWent_optmerge(),
    new KWent_parhier(),
    new KWent_steptype(),

    new KWent_modelcard(),
    new KWent_pexnodes(),
    new KWent_nobjthack(),
    new KWent_subend(),
    new KWent_subinvoke(),
    new KWent_substart(),
*/
    new sKW(0, 0)
};

