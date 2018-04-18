
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

#include "config.h"
#include "main.h"
#include "editif.h"
#include "scedif.h"
#include "extif.h"
#include "dsp_tkif.h"
#include "dsp_layer.h"
#include "dsp_color.h"
#include "dsp_inlines.h"
#include "si_parsenode.h"
#include "si_parser.h"
#include "si_interp.h"
#include "main_scriptif.h"
#include "fio.h"
#include "layertab.h"
#include "menu.h"
#include "keymap.h"
#include "promptline.h"
#include "errorlog.h"
#include "select.h"
#include "tech.h"
#include "ghost.h"
#include "miscutil/pathlist.h"
#include "ginterf/grfont.h"
#ifdef HAVE_MOZY
#include "help/help_defs.h"
#endif

#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <errno.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_SECURE
#include "miscutil/miscutil.h"
#endif

int BoxFilled;  // part of security system

// Application initialization function.
//
void
cMain::AppInit()
{

#ifdef HAVE_SECURE
    // Below is a booby trap in case the authorizing code is patched
    // over.  This is part of the security system.

    if (!BoxFilled) {
        char *uname = pathlist::get_user_name(false);
        char tbuf[256];

        sprintf(tbuf, "xic: box %s\n", uname);
        miscutil::send_mail(Log()->MailAddress(), "SecurityReport:App2", tbuf);
        delete [] uname;
        raise(SIGTERM);
    }
#endif

#ifdef HAVE_MOZY
    // Set help system defines, do this before init files are read.
#ifdef WIN32
    HLP()->define("Windows");
#endif
    // The symbol "Xic" is always defined.  Also define the virtual
    // program names as appropriate.
    HLP()->define("Xic");
    if (!EditIf()->hasEdit())
        HLP()->define("Xiv");
    else if (!ExtIf()->hasExtract())
        HLP()->define("XicII");
    HLP()->set_no_file_fonts(true);  // Don't use fonts from .mozyrc.
#ifdef HAVE_SECURE
    HLP()->define("xtlserv");
#endif
#endif

    DSP()->SetCurMode(xm_initial_mode);

    // The files read on startup, in order:
    //     .xicinit
    //     tech file
    //     .xicstart
    //     stipple file
    //     keymap
    //     .xicmacros
    //     font files
    //     cell file

    // Create the electrical layers.  The layers and order are set here.
    LT()->InitElecLayers();

    // Read .xicinit file.
    ExecStartupScript(InitScript());

    // Read technology file.
    Tech()->InitPreParse();
    if (!Tech()->TechExtension() || *Tech()->TechExtension()) {
        const char *libpath = CDvdb()->getVariable(VA_LibPath);

        const char *tfbak = mh::Techfile;

        // Read the device_templates file, if one exists.
        if (ExtIf()->hasExtract()) {
            FILE *fp = pathlist::open_path_file(DEVICE_TEMPLATES, libpath,
                "r", 0, true);
            if (fp) {
                mh::Techfile = DEVICE_TEMPLATES" file";
                ExtIf()->parseDeviceTemplates(fp);
                fclose(fp);
            }
        }

        // Now read the technology file.
        char buf[64];
        if (Tech()->TechExtension() && *Tech()->TechExtension())
            sprintf(buf, "%s.%s", TechFileBase(), Tech()->TechExtension());
        else
            strcpy(buf, TechFileBase());

        char *cwd = pathlist::expand_path(".", true, false);
        char *tpath = new char[strlen(cwd) + strlen(buf) + 16];
        strcpy(tpath, cwd);
        delete [] cwd;
        char *e = tpath + strlen(tpath);
        sprintf(e, "/%s", buf);

        // First check in the CWD.
        FILE *fp = fopen(tpath, "r");
        char *realname = 0;
        if (fp) {
            realname = tpath;
            tpath = 0;
        }

        // Next, try a subdirectory named "techfiles".
        if (!fp) {
            sprintf(e, "/techfiles/%s", buf);
            fp = fopen(tpath, "r");
            if (fp) {
                realname = tpath;
                tpath = 0;
            }
        }
        delete [] tpath;

        // Finally, look in the libpath.
        if (!fp) {
            fp = pathlist::open_path_file(buf, libpath, "r", &realname,
                false);
        }
        if (!fp) {
            Log()->WarningLog(mh::Initialization,
                "Can not open technology file.\n");
        }
        else {
            Tech()->SetTechFilename(realname);
            delete [] realname;
            fprintf(stderr, "Reading technology file %s.\n",
                Tech()->TechFilename());
            LT()->FreezeLayerTable(true);
            strcat(buf, " file");
            mh::Techfile = buf;
            Tech()->Parse(fp);
            LT()->FreezeLayerTable(false);
            fclose(fp);
        }
        mh::Techfile = tfbak;
    }
    Tech()->InitPostParse();

    SetCoordMode(CO_REDRAW);

    Menu()->InitAfterModeSwitch(0);
    LT()->InitLayerTable();
    DSPmainDraw(Clear())
    DSP()->MainWdesc()->DefaultWindow();
    ColorTimerInit();

    FIO()->OpenLibrary(CDvdb()->getVariable(VA_LibPath), DeviceLibName());
    ScedIf()->modelLibraryOpen(ModelLibName());
    ExtIf()->initDevs();
    InitSignals(true);

    // Run the scripts defined in the tech file that had a
    // "RunScript" line.  These are not saved.
    XM()->RunTechScripts();

    // Read .xicstart file.
    ExecStartupScript(StartupScript());

    // Read stipple file.
    Tech()->ReadDefaultStipples();

    // Read .xicmacros file.
    ExecStartupScript(MacroFileName());

    Selections.reset();

    // Read vector font file.
    FILE *fp = pathlist::open_path_file(FontFileName(),
        CDvdb()->getVariable(VA_LibPath), "r", 0, true);
    if (fp) {
        FT.parseChars(fp);
        fclose(fp);
    }

    xm_var_bak = CDvdb()->createBackup();
    XM()->SetAppInitDone(true);
    XM()->Rehash();  // Script menus and libraries are read here.
}


void
cMain::ExecStartupScript(const char *filename)
{
    SIfile *sfp = 0;
    char buf[256];
    if (Tech()->TechExtension() && *Tech()->TechExtension()) {
        sprintf(buf, "%s.%s", filename, Tech()->TechExtension());
        sfp = SIfile::create(buf, 0, 0);
        if (!sfp) {
            if (HomeDir()) {
                char *t = pathlist::mk_path(HomeDir(), buf);
                sfp = SIfile::create(t, 0, 0);
                delete [] t;
            }
        }
    }
    if (!sfp) {
        strcpy(buf, filename);
        sfp = SIfile::create(buf, 0, 0);
        if (!sfp) {
            if (HomeDir()) {
                char *t = pathlist::mk_path(HomeDir(), buf);
                sfp = SIfile::create(t, 0, 0);
                delete [] t;
            }
        }
    }
    if (!sfp)
        return;

    SI()->Interpret(sfp, 0, 0, 0);
    EditIf()->ulCommitChanges(true);
    CommitCell();
    delete sfp;
}


// Find the window coordinates of the pointer.  If the pointer is
// not in the window where motion last occurred, return false.
//
bool
cMain::WhereisPointer(int *xp, int *yp)
{
    bool inwin = true;
    int x, y;
    WindowDesc *wdesc = EV()->CurrentWin() ?
        EV()->CurrentWin() : DSP()->MainWdesc();
    wdesc->Wdraw()->QueryPointer(&x, &y, 0);
    if (x < 0 || x >= wdesc->ViewportWidth() ||
            y < 0 || y >= wdesc->ViewportHeight())
        inwin = false;
    wdesc->PToL(x, y, x, y);
    *xp = x;
    *yp = y;
    return (inwin);
}


// Return false and print a message if the current mode is not the
// mode passed.
//
bool
cMain::CheckCurMode(DisplayMode mode)
{
    if (DSP()->CurMode() != mode) {
        PL()->ShowPromptV("This command available in %s mode only!",
            DisplayModeName(mode));
        return (false);
    }
    return (true);
}


// Return false if there is no current layer.
//
bool
cMain::CheckCurLayer()
{
    if (LT()->CurLayer()) {
        if (LT()->CurLayer() == CDldb()->layer(0, DSP()->CurMode()))
            LT()->SetCurLayer(0);
        else if (DSP()->CurMode() == Physical) {
            if (LT()->CurLayer()->physIndex() < 0)
                LT()->SetCurLayer(0);
        }
        else {
            if (LT()->CurLayer()->elecIndex() < 0)
                LT()->SetCurLayer(0);
        }
    }
    if (!LT()->CurLayer()) {
        PL()->ShowPrompt("No current layer!");
        return (false);
    }
    return (true);
}


// If there is no current cell for the mode, try to create one if
// create is true.  Otherwise print an error message and return false.
// If editable is set and the current cell is immutable, print an
// error message and return false.
//
bool
cMain::CheckCurCell(bool editable, bool create, DisplayMode mode)
{
    if (!DSP()->CurCellName()) {
        PL()->ShowPrompt("No current cell!");
        return (false);
    }
    CDs *sd = CurCell(mode);
    if (sd && editable && sd->isImmutable()) {
        PL()->ShowPrompt("Current cell is Read-Only!");
        return (false);
    }
    if (!sd) {
        if (!create) {
            PL()->ShowPromptV("No %s data in current cell!",
                DisplayModeNameLC(mode));
            return (false);
        }
        if (!CD()->ReopenCell(Tstring(DSP()->CurCellName()), mode)) {
            PL()->ShowPrompt(Errs()->get_error());
            return (false);
        }
        PopUpCells(0, MODE_UPD);
    }
    return (true);
}


// Update the user menu and/or read the library files.
//
void
cMain::Rehash()
{
    if (XM()->RunMode() == ModeNormal) {
        if (Menu())
            Menu()->UpdateUserMenu();
    }
    else {
        // Read the library files in the script path
        umenu *u = GetFunctionList();
        umenu::destroy(u);
    }
}


namespace {
    const char *const copyright_msg = 
    "Copyright (C) <b>Whiteley Research Inc.</b>, Sunnyvale, CA %4d,\n"
    "all rights reserved.\n";

    const char *const about_msg[] = {
    "<font size=2>\n",
    "<p>THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND,\n",
    "EXPRESS OR IMPLIED,  INCLUDING BUT NOT LIMITED TO THE  WARRANTIES\n",
    "OF MERCHANTABILITY,  AND FITNESS FOR A PARTICULAR PURPOSE.  IN NO\n",
    "EVENT SHALL WHITELEY RESEARCH INCORPORATED OR STEPHEN R. WHITELEY\n",
    "BE LIABLE FOR ANY CLAIM,  DAMAGES OR OTHER LIABILITY,  WHETHER IN\n",
    "AN ACTION OF CONTRACT,  TORT OR OTHERWISE,  ARISING FROM,  OUT OF\n",
    "OR IN CONNECTION WITH  THE SOFTWARE  OR THE USE OR OTHER DEALINGS\n",
    "IN THE SOFTWARE.<p>\n",
    "</font>\n",
    "Use of the software constitutes formal acceptance of the Terms and\n",
    "Conditions as specified in any Licensing Agreement provided upon\n",
    "purchase of the software.\n<br>\n",
    0 };
}

// This is found with the help files.
#define BANNER_IMG "wrbannermain.gif"


// Pop up the legal message.
//
void
cMain::LegalMsg()
{
    // Add the re-release number to the version number.
    char buf[256];
    strcpy(buf, XM()->VersionString());
    const char *t = XM()->TagString();
    if (t) {
        // tag is xic-x-x-x, or xic-x-x-x-x if re-release.
        t = strchr(t+1, '-');
        if (t)
            t = strchr(t+1, '-');
        if (t)
            t = strchr(t+1, '-');
        if (t)
            t = strchr(t+1, '-');
        if (t)
            strcat(buf, t);
    }

    char *imgfile = 0;
    const char *hp = CDvdb()->getVariable(VA_HelpPath);
    if (hp) {
        pathgen pg(hp);
        char *path;
        while ((path = pg.nextpath(true)) != 0) {
            char *npath = new char[strlen(path) + 30];
            char *e = lstring::stpcpy(npath, path);
            delete [] path;
            *e++ = '/';
            strcpy(e, BANNER_IMG);
            FILE *fp = fopen(npath, "r");
            if (fp) {
                imgfile = npath;
                fclose(fp);
                break;
            }
            delete [] npath;
        }
    }

    const char *prg = "Xic Integrated Circuit Graphical Development Tool";

    sLstr lstr;
    lstr.add("<html><body bgcolor=white text=\"#000000\">\n");
    if (imgfile) {
        lstr.add("<img src=file://");
        lstr.add(imgfile);
        lstr.add(" width=300 align=left valign=top>\n");
        delete [] imgfile;
    }
    else
        lstr.add("<b>Whiteley Research, Inc.</b><br>");
    lstr.add("456 Flora Vista Ave.<br>Sunnyvale CA 94086<br>");
    lstr.add("<tt>wrcad.com</tt>\n");
    lstr.add("<font color=blue>\n");
    lstr.add("<h2>");
    lstr.add(prg);
    lstr.add("</h2>\n");
    lstr.add("<b>Release: ");
    lstr.add(buf);
    lstr.add("</b><br>\n");
    lstr.add("</font>\n");

    sprintf(buf, copyright_msg, BuildYear());
    lstr.add(buf);
    for (int i = 0; about_msg[i]; i++)
        lstr.add(about_msg[i]);
    lstr.add("</body></html>\n");

    if (DSP()->MainWdesc() && DSP()->MainWdesc()->Wbag())
        DSPmainWbag(PopUpHTMLinfo(MODE_ON, lstr.string(), GRloc(LW_CENTER)))
    else
        fprintf(stderr, "%s\n", lstr.string());
}


//
// Shell exec
//

#ifdef __linux
#define sig_t sighandler_t
#endif

// Static function.
// A replacement for the stdlib system() which closes the connection
// to X in the child.  The regular system() sometimes causes problems
// in the parent after the child exits.
//
int
cMain::System(const char *cmd)
{
    if (!cmd)
        return (1);
#ifdef WIN32
    return (system(cmd));
#else
#ifndef HAVE_SYS_WAIT_H
    return (system(cmd));
#else
    static sig_t tmpi = signal(SIGINT, SIG_IGN);
    static sig_t tmpq = signal(SIGQUIT, SIG_IGN);
    // block SIGCHLD
    sigset_t newsigblock, oldsigblock;
    sigemptyset(&newsigblock);
    sigaddset(&newsigblock, SIGCHLD);
    sigprocmask(SIG_BLOCK, &newsigblock, &oldsigblock);

    int pid = fork();
    if (pid == 0) {
        dspPkgIf()->CloseGraphicsConnection();
        signal(SIGINT, tmpi);
        signal(SIGQUIT, tmpq);
        sigprocmask(SIG_SETMASK, &oldsigblock, 0);
        setsid();
        execl("/bin/sh", "sh", "-c", cmd, (char*)0);
        _exit(127);
    }
    int status = 0;
    if (pid != -1) {
        for (;;) {
            int rpid = waitpid(pid, &status, 0);
            if (rpid == -1 && errno == EINTR)
                continue;
            break;
        }
    }
    signal(SIGINT, tmpi);
    signal(SIGQUIT, tmpq);
    sigprocmask(SIG_SETMASK, &oldsigblock, 0);
    return (pid == -1 ? -1 : status);
#endif
#endif
}

