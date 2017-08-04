
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

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Wayne A. Christopher
         1992 David A. Gates ( Xgraph() )
         1993 Stephen R. Whiteley
****************************************************************************/

#include "outplot.h"
#include "frontend.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "commands.h"
#include "filestat.h"


//
// Hard copy functions.
//

const char *kw_hcopyfilename = "hardcopyfilename";

namespace {
    int spool(const char*, const char*, const char*, char*);
    void message(const char*, char*);
    bool get_dim(const char*, double*);
    void mkargv(int*, char**, char*);
    bool wrshcsetup(bool, int, bool, GRdraw*);
    int wrshcgo(HCorientFlags, HClegType, GRdraw*);
}


// Hardcopy parameter defaults
//
HCcb wrsHCcb =
{
    wrshcsetup,     // hcsetup
    wrshcgo,        // hcgo
    0,              // hcframe
    2,              // format  (postscript line draw)
    0,              // drvrmask
    HClegNone,      // legend
    HCbest,         // orient
    0,              // resolution
    0,              // command
    false,          // tofile
    "",             // tofilename
    0.0,            // left
    0.0,            // top
    0.0,            // width
    0.0             // height
};


namespace {
    // This gets called after a format change from the Print panel. 
    // Remember the new format.
    //
    bool wrshcsetup(bool dohc, int fmt, bool, GRdraw*)
    {
        if (dohc && fmt > 0)
            wrsHCcb.format = fmt;
        return (false);
    }


    // Prodecure passed to the hardcopy widget that actually generates
    // the hardcopy.
    //
    int wrshcgo(HCorientFlags, HClegType, GRdraw *grd)
    {
        if (!grd)
            return (true);
        sGraph *graph = static_cast<sGraph*>(grd->UserData());
        if (!graph)
            return (true);
        sGraph *tempgraph = graph->gr_copy();
        GP.PushGraphContext(graph);
        // Context is pushed during New() for ListPixels().
        GRdraw *w = GRpkgIf()->NewDraw();
        GP.PopGraphContext();
        if (!w) {
            GP.DestroyGraph(tempgraph->id());
            return (true);
        }
        w->SetUserData(tempgraph);
        tempgraph->set_dev(w);
        tempgraph->set_fontsize();
        tempgraph->area().set_height(GRpkgIf()->CurDev()->height);
        tempgraph->area().set_width(GRpkgIf()->CurDev()->width);
        tempgraph->area().set_left(GRpkgIf()->CurDev()->xoff);
        tempgraph->area().set_bottom(GRpkgIf()->CurDev()->yoff);
        // accommodate "auto scale"
        if (tempgraph->area().width() == 0) {
            tempgraph->area().set_width(
                (tempgraph->area().height()*graph->area().width())/
                graph->area().height());
            GRpkgIf()->CurDev()->width = tempgraph->area().width();
        }
        else if (tempgraph->area().height() == 0) {
            tempgraph->area().set_height(
                (tempgraph->area().width()*graph->area().height())/
                graph->area().width());
            GRpkgIf()->CurDev()->height = tempgraph->area().height();
        }
        tempgraph->dev()->DefineViewport();
        tempgraph->gr_redraw();
        GP.DestroyGraph(tempgraph->id());
        return (false);
    }
}


// hardcopy setupargs plotargs.
// setupargs: "-d driver -c command -f filename -r resolution -w width
//            -h height -x left_marg -y top_marg -l"
// width, height, left_marg, and top_marg in inches (float format)
// or can be followed with "cm" for centimeters.
// resolution: pixels/inch.
// -l for landscape (otherwise portrait).
//
// defaults: driver defaults, or set variables
// hcopydriver, hcopycommand, hcopyresol,
// hcopywidth, hcopyheight, hcopyxoff, hcopyyoff, hcopylandscape.
//
void
CommandTab::com_hardcopy(wordlist *wl)
{
    char *driver = 0, *command = 0, *filename = 0;
    char *resol = 0, *width = 0, *height = 0;
    char *xoff = 0, *yoff = 0;
    bool lands = false;

    while (wl) {
        if (*wl->wl_word == '-') {
            bool fail = false;
            switch (wl->wl_word[1]) {
            case 'd':
                wl = wl->wl_next;
                if (!wl) {
                    fail = true;
                    break;
                }
                driver = wl->wl_word;
                wl = wl->wl_next;
                break;
            case 'c':
                wl = wl->wl_next;
                if (!wl) {
                    fail = true;
                    break;
                }
                command = wl->wl_word;
                wl = wl->wl_next;
                break;
            case 'f':
                wl = wl->wl_next;
                if (!wl) {
                    fail = true;
                    break;
                }
                filename = wl->wl_word;
                wl = wl->wl_next;
                break;
            case 'w':
                wl = wl->wl_next;
                if (!wl) {
                    fail = true;
                    break;
                }
                width = wl->wl_word;
                wl = wl->wl_next;
                break;
            case 'h':
                wl = wl->wl_next;
                if (!wl) {
                    fail = true;
                    break;
                }
                height = wl->wl_word;
                wl = wl->wl_next;
                break;
            case 'x':
                wl = wl->wl_next;
                if (!wl) {
                    fail = true;
                    break;
                }
                xoff = wl->wl_word;
                wl = wl->wl_next;
                break;
            case 'y':
                wl = wl->wl_next;
                if (!wl) {
                    fail = true;
                    break;
                }
                yoff = wl->wl_word;
                wl = wl->wl_next;
                break;
            case 'r':
                wl = wl->wl_next;
                if (!wl) {
                    fail = true;
                    break;
                }
                resol = wl->wl_word;
                wl = wl->wl_next;
                break;
            case 'l':
                lands = true;
                wl = wl->wl_next;
                break;
            default:
                fail = true;
            }
            if (fail) {
                GRpkgIf()->ErrPrintf(ET_ERROR,
                    "hardcopy: bad option syntax.\n");
                return;
            }
        }
        else
            break;
    }

    VTvalue vv;
    if (!driver && Sp.GetVar(kw_hcopydriver, VTYP_STRING, &vv))
        driver = vv.get_string();

    HCdesc *hcdesc = 0;
    if (!driver) {
        hcdesc = GRpkgIf()->HCof(wrsHCcb.format);
        if (!hcdesc)
            return;
    }
    else {
        hcdesc = GRpkgIf()->FindHCdesc(driver);
        if (!hcdesc) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "hardcopy: no such driver %s.\n", driver);
            return;
        }
    }
    driver = lstring::copy(hcdesc->keyword);

    if (!filename) {
        filename = filestat::make_temp("hc");
        if (!command && Sp.GetVar(kw_hcopycommand, VTYP_STRING, &vv))
            command = lstring::copy(vv.get_string());
        if (!command)
            GRpkgIf()->ErrPrintf(ET_WARN, "hardcopy: no print command.\n");
    }
    else {
        filename = lstring::copy(filename);
        command = 0;
    }

    if (!width && Sp.GetVar(kw_hcopywidth, VTYP_STRING, &vv))
        width = vv.get_string();
    double w;
    if (!width || !get_dim(width, &w) ||
            w < hcdesc->limits.minwidth || w > hcdesc->limits.maxwidth)
        w = hcdesc->defaults.defwidth;
    width = 0;

    if (!height && Sp.GetVar(kw_hcopyheight, VTYP_STRING, &vv))
        height = vv.get_string();
    double h;
    if (!height || !get_dim(height, &h) ||
            h < hcdesc->limits.minheight || h > hcdesc->limits.maxheight)
        h = hcdesc->defaults.defheight;
    height = 0;

    if (!xoff && Sp.GetVar(kw_hcopyxoff, VTYP_STRING, &vv))
        xoff = vv.get_string();
    double x;
    if (!xoff || !get_dim(xoff, &x) ||
            x < hcdesc->limits.minxoff || x > hcdesc->limits.maxxoff)
        x = hcdesc->defaults.defxoff;
    xoff = 0;

    if (!yoff && Sp.GetVar(kw_hcopyyoff, VTYP_STRING, &vv))
        yoff = vv.get_string();
    double y;
    if (!yoff || !get_dim(yoff, &y) ||
            y < hcdesc->limits.minyoff || y > hcdesc->limits.maxyoff)
        y = hcdesc->defaults.defyoff;
    yoff = 0;

    if (!resol && Sp.GetVar(kw_hcopyresol, VTYP_STRING, &vv))
        resol = vv.get_string();
    int r;
    if (resol && sscanf(resol, "%d", &r) == 1) {
        int j;
        for (j = 0; hcdesc->limits.resols[j]; j++)
            if (r == atoi(hcdesc->limits.resols[j]))
                break;
        if (!hcdesc->limits.resols[j])
            r = atoi(hcdesc->limits.resols[hcdesc->defaults.defresol]);
    }
    else
        r = atoi(hcdesc->limits.resols[hcdesc->defaults.defresol]);
    resol = 0;

    char buf[BSIZE_SP];
    sprintf(buf, hcdesc->fmtstring, filename, r, w, h, x, y);
    if (lands)
        strcat(buf, " -l");
    char *argv[20];
    int argc;
    char *cmdstr = lstring::copy(buf);
    mkargv(&argc, argv, cmdstr);
    if (GRpkgIf()->SwitchDev(hcdesc->drname, &argc, argv) == HCSok) {
        if (GP.Plot(wl, 0, 0, 0, GR_PLOT))
            spool(filename, driver, command, 0);
        GRpkgIf()->SwitchDev(0, 0, 0);
    }
    delete [] cmdstr;
    delete [] driver;
    delete [] filename;
}
// End of CommandTab functions.


// Make hardcopy of graph.  This in not used under X, but is called
// if a full-screen graphics context is being used.
//
void
sGraph::gr_hardcopy()
{
    char buf[BSIZE_SP];
    VTvalue vv;
    char *driver = 0;
    if (Sp.GetVar(kw_hcopydriver, VTYP_STRING, &vv))
        driver = vv.get_string();;

    HCdesc *hcdesc = 0;
    if (!driver) {
        hcdesc = GRpkgIf()->HCof(wrsHCcb.format);
        if (!hcdesc)
            return;
    }
    else {
        hcdesc = GRpkgIf()->FindHCdesc(driver);
        if (!hcdesc) {
            GRpkgIf()->ErrPrintf(ET_ERROR,
                "hardcopy: no such driver %s.\n", driver);
            return;
        }
    }
    driver = lstring::copy(hcdesc->keyword);

    double w;
    {
        char *width = 0;
        if (Sp.GetVar(kw_hcopywidth, VTYP_STRING, &vv))
            width = vv.get_string();
        if (!width || !get_dim(width, &w) ||
                w < hcdesc->limits.minwidth || w > hcdesc->limits.maxwidth)
            w = hcdesc->defaults.defwidth;
    }

    double h;
    {
        char *height = 0;
        if (Sp.GetVar(kw_hcopyheight, VTYP_STRING, &vv))
            height = vv.get_string();
        if (!height || !get_dim(height, &h) ||
                h < hcdesc->limits.minheight || h > hcdesc->limits.maxheight)
            h = hcdesc->defaults.defheight;
    }

    double x;
    {
        char *xoff = 0;
        if (Sp.GetVar(kw_hcopyxoff, VTYP_STRING, &vv))
            xoff = vv.get_string();
        if (!xoff || !get_dim(xoff, &x) ||
                x < hcdesc->limits.minxoff || x > hcdesc->limits.maxxoff)
            x = hcdesc->defaults.defxoff;
    }

    double y;
    {
        char *yoff = 0;
        if (Sp.GetVar(kw_hcopyyoff, VTYP_STRING, &vv))
            yoff = vv.get_string();
        if (!yoff || !get_dim(yoff, &y) ||
                y < hcdesc->limits.minyoff || y > hcdesc->limits.maxyoff)
            y = hcdesc->defaults.defyoff;
    }

    int r;
    {
        char *resol = 0;
        if (Sp.GetVar(kw_hcopyresol, VTYP_STRING, &vv))
            resol = vv.get_string();
        bool ok = false;
        if (resol && sscanf(resol, "%d", &r) == 1) {
            int j;
            for (j = 0; hcdesc->limits.resols[j]; j++)
                if (r == atoi(hcdesc->limits.resols[j])) {
                    ok = true;
                    break;
                }
        }
        if (!ok)
            r = atoi(hcdesc->limits.resols[hcdesc->defaults.defresol]);
    }

    bool lands = Sp.GetVar(kw_hcopyresol, VTYP_BOOL, 0);

    char *cmd = 0;
    char *fname = 0;
    if (Sp.GetVar(kw_hcopyfilename, VTYP_STRING, &vv))
        fname = vv.get_string();
    if (fname)
        fname = lstring::copy(fname);
    else {
        if (Sp.GetVar(kw_hcopycommand, VTYP_STRING, &vv))
            cmd = vv.get_string();
        if (cmd)
            cmd = lstring::copy(cmd);
        else {
            GRpkgIf()->ErrPrintf(ET_WARN, "hardcopy: no print command.\n");
            delete [] driver;
            return;
        }
        fname = filestat::make_temp("hc");
    }

    sprintf(buf, hcdesc->fmtstring, fname, r, w, h, x, y);
    if (lands)
        strcat(buf, " -l");
    char *argv[20];
    int argc;
    {
        char *cmdstr = lstring::copy(buf);
        mkargv(&argc, argv, cmdstr);
        delete [] cmdstr;
    }
    if (!GRpkgIf()->SwitchDev(hcdesc->drname, &argc, argv)) {
        if (GP.Plot(gr_command, this, 0, 0, GR_PLOT))
            spool(fname, driver, cmd, 0);
        GRpkgIf()->SwitchDev(0, 0, 0);
    }
    delete [] driver;
    delete [] fname;
    delete [] cmd;
    return;
}


namespace {
    int spool(const char *filename, const char *devtype, const char *command,
        char *mesg)
    {
        char buf[BSIZE_SP];
        if (command && *command) {
            sprintf(buf, "Spooling %s using %s.", filename, devtype);
            message(buf, mesg);

            const char *s;
            if ((s = strchr(command, '%')) != 0 && *(s+1) == 's') {
                strcpy(buf, command);
                buf[s - command] = 0;
                sprintf(buf + strlen(buf), "%s%s", filename, s+2);
            }
            else
                sprintf(buf, "%s %s", command, filename);
            int i;
            if ((i = CP.System(buf)) != 0) {
                sprintf(buf + strlen(buf), ": error status %d returned", i);
                message(buf, mesg);
            }
            else
                message("Done.", mesg);
            if (!i) {
                unlink(filename);
                return (0);
            }

        }
        sprintf(buf, "Data saved in file \"%s\", in %s format.",
            filename, devtype);
        message(buf, mesg);
        return (0);
    }


    void message(const char *instr, char *outstr)
    {
        if (!outstr)
            TTY.out_printf("%s\n", instr);
        else
            strcpy(outstr, instr);
    }


    // Return val in inches and true if ok, return false otherwise.  str
    // is a floating point number possibly followed by "cm".
    //
    bool get_dim(const char *str, double *val)
    {
        char buf[128];
        int i = sscanf(str, "%lf%s", val, buf);
        if (i == 2 && buf[0] == 'c' && buf[1] == 'm')
            *val *= 2.54;
        if (i > 0)
            return (true);
        return (false);
    }


    // Make an argv-type string array from string str.
    //
    void mkargv(int *acp, char **av, char *str)
    {
        char *s = str;
        int j = 0;
        for (;;) {
            while (isspace(*s)) s++;
            if (!*s) {
                *acp = j;
                return;
            }
            char *t = s;
            while (*t && !isspace(*t)) t++;
            if (*t)
                *t++ = '\0';
            av[j++] = s;
            s = t;
        }
    }
}

