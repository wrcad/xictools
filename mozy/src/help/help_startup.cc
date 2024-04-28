
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
 * Help System Files                                                      *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "help_defs.h"
#include "help_startup.h"
#include "help_context.h"
#include "miscutil/pathlist.h"
#include "miscutil/lstring.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>


#define STARTUP_FILE ".mozyrc"

HLPparams::HLPparams(bool nofonts)
{
    LoadMode = LoadProgressive;
    AnchorUnderlineType = AnchorSingleLine;
    AnchorUnderlined = true;
    AnchorButtons = false;
    AnchorHighlight = true;
    FreezeAnimations = false;
    Timeout = 15;
    Retries = 4;
    HTTP_Port = 80;
    FTP_Port = 21;
    CacheSize = 64;
    NoCache = false;
    NoCookies = false;
    DebugMode = false;
    PrintTransact = false;
    BadHTMLwarnings = false;
    LocalImageTest = LInormal;
    SourceFilePath = 0;

    ignore_fonts = nofonts;

    parse_startup_file();

    // some sanity checking
    if (LoadMode != LoadProgressive && LoadMode != LoadDelayed &&
            LoadMode != LoadSync)
        LoadMode = LoadNone;

    if (AnchorUnderlineType)
        AnchorUnderlined = true;
    else {
        AnchorUnderlined = false;
        AnchorUnderlineType = 1;
    }
}


namespace {
    // Try to open the configuration file, if success return the file
    // pointer and set where, if where is non-null.
    //
    FILE *open_cfgfile(char **where, const char *mode)
    {
        if (mode && strchr(mode, 'r')) {
            // Reading: check CWD, then $HOME.

            char *cwd = getcwd(0, 0);
            char *path = pathlist::mk_path(cwd, STARTUP_FILE);
            free(cwd);
            FILE *fp = fopen(path, mode);
            if (fp) {
                if (where)
                    *where = path;
                else
                    delete [] path;
                return (fp);
            }
            delete [] path;
            char *home = pathlist::get_home();
            if (home) {
                path = pathlist::mk_path(home, STARTUP_FILE);
                delete [] home;
                fp = fopen(path, mode);
                if (fp) {
                    if (where)
                        *where = path;
                    else
                        delete [] path;
                    return (fp);
                }
                delete [] path;
            }
        }
        else if (mode && strchr(mode, 'w')) {
            // Writing: trye $HOME, then CWD.

            char *home = pathlist::get_home();
            if (home) {
                char *path = pathlist::mk_path(home, STARTUP_FILE);
                delete [] home;
                FILE *fp = fopen(path, mode);
                if (fp) {
                    if (where)
                        *where = path;
                    else
                        delete [] path;
                    return (fp);
                }
                delete [] path;
            }
            char *cwd = getcwd(0, 0);
            char *path = pathlist::mk_path(cwd, STARTUP_FILE);
            free(cwd);
            FILE *fp = fopen(path, mode);
            if (fp) {
                if (where)
                    *where = path;
                else
                    delete [] path;
                return (fp);
            }
            delete [] path;
        }
        if (where)
            *where = 0;
        return (0);
    }
}


// This reads the startup file $HOME/.mozyrc and sets the values of the
// parameters to those found in the file.
//
void
HLPparams::parse_startup_file()
{
    FILE *fp = open_cfgfile(&SourceFilePath, "r");
    if (!fp)
        return;

    char buf[256];
    while (fgets(buf, 256, fp) != 0) {
        char *s = buf;
        while (isspace(*s))
            s++;
        if (!*s || *s == '#')
            continue;
        char *name = lstring::gettok(&s);
        char *value = lstring::copy(s);
        char *t = value + strlen(value) - 1;
        while (t > value && isspace(*t))
            *t-- = 0;

        if (name && *value) {
            if (lstring::cieq(name, "ImageLoadMode")) {
                int i = atoi(value);
                if (i >= 0 && i <= 3)
                    LoadMode = (LoadType)i;
            }
            else if (lstring::cieq(name, "AnchorUnderline")) {
                int i = atoi(value);
                if (i >= 0 && i <= 4)
                    AnchorUnderlineType = i;
            }
            else if (lstring::cieq(name, "AnchorButtons"))
                AnchorButtons = atoi(value) ? true : false;
            else if (lstring::cieq(name, "AnchorHighlight"))
                AnchorHighlight = atoi(value) ? true : false;
            else if (lstring::cieq(name, "FontFamily")) {
                if (!ignore_fonts)
                    HLP()->context()->setFont(value);
            }
            else if (lstring::cieq(name, "FixedFontFamily")) {
                if (!ignore_fonts)
                    HLP()->context()->setFixedFont(value);
            }
            else if (lstring::cieq(name, "FreezeAnimations"))
                FreezeAnimations = atoi(value) ? true : false;
            else if (lstring::cieq(name, "Timeout")) {
                int i = atoi(value);
                if (i >= 0 && i <= 600)
                    Timeout = i;
            }
            else if (lstring::cieq(name, "Retries")) {
                int i = atoi(value);
                if (i >= 0 && i <= 10)
                    Retries = i;
            }
            else if (lstring::cieq(name, "HTTP_Port")) {
                int i = atoi(value);
                if (i > 0 && i <= 65536)
                    HTTP_Port = i;
            }
            else if (lstring::cieq(name, "FTP_Port")) {
                int i = atoi(value);
                if (i > 0 && i <= 65536)
                    FTP_Port = i;
            }
            else if (lstring::cieq(name, "CacheSize")) {
                int i = atoi(value);
                if (i > 1 && i <= 4096)
                    CacheSize = i;
            }
            else if (lstring::cieq(name, "NoCache"))
                NoCache = atoi(value) ? true : false;
            else if (lstring::cieq(name, "NoCookies"))
                NoCookies = atoi(value) ? true : false;
            else if (lstring::cieq(name, "DebugMode")) {
                DebugMode = atoi(value) ? true : false;
                HLP()->set_debug(DebugMode);
            }
            else if (lstring::cieq(name, "PrintTransact"))
                PrintTransact = atoi(value) ? true : false;
            else if (lstring::cieq(name, "LocalImageTestMode")) {
                int i = atoi(value);
                if (i >= 0 && i <= 2)
                    LocalImageTest = (LImode)i;
            }
            else if (lstring::cieq(name, "BadHTMLwarnings"))
                BadHTMLwarnings = atoi(value) ? true : false;

            else if (lstring::cieq(name, HLP_DefaultBgColor))
                HLP()->context()->setDefaultColor(name, value);
            else if (lstring::cieq(name, HLP_DefaultBgImage))
                HLP()->context()->setDefaultColor(name, value);
            else if (lstring::cieq(name, HLP_DefaultFgText))
                HLP()->context()->setDefaultColor(name, value);
            else if (lstring::cieq(name, HLP_DefaultFgLink))
                HLP()->context()->setDefaultColor(name, value);
            else if (lstring::cieq(name, HLP_DefaultFgVisitedLink))
                HLP()->context()->setDefaultColor(name, value);
            else if (lstring::cieq(name, HLP_DefaultFgActiveLink))
                HLP()->context()->setDefaultColor(name, value);
            else if (lstring::cieq(name, HLP_DefaultBgSelect))
                HLP()->context()->setDefaultColor(name, value);
            else if (lstring::cieq(name, HLP_DefaultFgImagemap))
                HLP()->context()->setDefaultColor(name, value);

            else if (lstring::cieq(name, "Alias"))
                HLP()->context()->setRootAlias(value);
            else
                fprintf(stderr,
                    "Warning: ignoring unknown keyword %s in %s file\n",
                    name, STARTUP_FILE);
        }
        delete [] name;
        delete [] value;
    }
    fclose(fp);
}


// Update the startup file $HOME/.mozyrc with the present parameters. 
// Only parameters defined in an existing .mozyrc file are updated,
// this does not create a new file.  The existing file is saved with a
// .bak extension.
//
void
HLPparams::update()
{
    FILE *fp = 0;
    if (SourceFilePath)
        fp = fopen(SourceFilePath, "r");
    if (!fp) {
        delete [] SourceFilePath;
        SourceFilePath = 0;
        fp = open_cfgfile(&SourceFilePath, "r");
    }
    if (!fp)
        return;
    char *bkfile = new char[strlen(SourceFilePath) + 5];
    char *ee = lstring::stpcpy(bkfile, SourceFilePath);
    strcpy(ee, ".bak");

    FILE *gp = fopen(bkfile, "w");
    if (!gp) {
        fclose(fp);
        delete [] bkfile;
        return;
    }
    int c;
    while ((c = getc(fp)) != EOF)
        putc(c, gp);
    fclose(fp);
    fclose(gp);

    fp = fopen(bkfile, "r");
    delete [] bkfile;
    if (!fp)
        return;
    gp = fopen(SourceFilePath, "w");
    if (!gp) {
        fclose(fp);
        return;
    }

    char buf[256];
    while (fgets(buf, 256, fp) != 0) {
        char *s = buf;
        while (isspace(*s))
            s++;
        if (!*s || *s == '#') {
            fputs(buf, gp);
            continue;
        }
        char *name = lstring::gettok(&s);
        if (lstring::cieq(name, "ImageLoadMode"))
            fprintf(gp, "%s %d\n", name, LoadMode);
        else if (lstring::cieq(name, "AnchorUnderline"))
            fprintf(gp, "%s %d\n", name, AnchorUnderlineType);
        else if (lstring::cieq(name, "AnchorButtons"))
            fprintf(gp, "%s %d\n", name, AnchorButtons);
        else if (lstring::cieq(name, "AnchorHighlight"))
            fprintf(gp, "%s %d\n", name, AnchorHighlight);
        else if (lstring::cieq(name, "FontFamily")) {
            if (ignore_fonts)
                fputs(buf, gp);
            else {
                char *nm = Fnt()->getFamilyName(FNT_MOZY);
                if (nm) {
                    fprintf(gp, "%s %s\n", name, nm);
                    delete [] nm;
                }
            }
        }
        else if (lstring::cieq(name, "FixedFontFamily")) {
            if (ignore_fonts)
                fputs(buf, gp);
            else {
                char *nm = Fnt()->getFamilyName(FNT_MOZY_FIXED);
                if (nm) {
                    fprintf(gp, "%s %s\n", name, nm);
                    delete [] nm;
                }
            }
        }
        else if (lstring::cieq(name, "FreezeAnimations"))
            fprintf(gp, "%s %d\n", name, FreezeAnimations);
        else if (lstring::cieq(name, "Timeout"))
            fprintf(gp, "%s %d\n", name, Timeout);
        else if (lstring::cieq(name, "Retries"))
            fprintf(gp, "%s %d\n", name, Retries);
        else if (lstring::cieq(name, "HTTP_Port"))
            fprintf(gp, "%s %d\n", name, HTTP_Port);
        else if (lstring::cieq(name, "FTP_Port"))
            fprintf(gp, "%s %d\n", name, FTP_Port);
        else if (lstring::cieq(name, "CacheSize"))
            fprintf(gp, "%s %d\n", name, CacheSize);
        else if (lstring::cieq(name, "NoCache"))
            fprintf(gp, "%s %d\n", name, NoCache);
        else if (lstring::cieq(name, "NoCookies"))
            fprintf(gp, "%s %d\n", name, NoCookies);
        else if (lstring::cieq(name, "DebugMode"))
            fprintf(gp, "%s %d\n", name, DebugMode);
        else if (lstring::cieq(name, "PrintTransact"))
            fprintf(gp, "%s %d\n", name, PrintTransact);
        else if (lstring::cieq(name, "LocalImageTestMode"))
            fprintf(gp, "%s %d\n", name, LocalImageTest);
        else if (lstring::cieq(name, "BadHTMLwarnings"))
            fprintf(gp, "%s %d\n", name, BadHTMLwarnings);

        else if (lstring::cieq(name, HLP_DefaultBgColor)) {
            const char *clr = HLP()->context()->getDefaultColor(name);
            if (clr && *clr)
                fprintf(gp, "%s %s\n", name, clr);
        }
        else if (lstring::cieq(name, HLP_DefaultBgImage)) {
            const char *clr = HLP()->context()->getDefaultColor(name);
            if (clr && *clr)
                fprintf(gp, "%s %s\n", name, clr);
        }
        else if (lstring::cieq(name, HLP_DefaultFgText)) {
            const char *clr = HLP()->context()->getDefaultColor(name);
            if (clr && *clr)
                fprintf(gp, "%s %s\n", name, clr);
        }
        else if (lstring::cieq(name, HLP_DefaultFgLink)) {
            const char *clr = HLP()->context()->getDefaultColor(name);
            if (clr && *clr)
                fprintf(gp, "%s %s\n", name, clr);
        }
        else if (lstring::cieq(name, HLP_DefaultFgVisitedLink)) {
            const char *clr = HLP()->context()->getDefaultColor(name);
            if (clr && *clr)
                fprintf(gp, "%s %s\n", name, clr);
        }
        else if (lstring::cieq(name, HLP_DefaultFgActiveLink)) {
            const char *clr = HLP()->context()->getDefaultColor(name);
            if (clr && *clr)
                fprintf(gp, "%s %s\n", name, clr);
        }
        else if (lstring::cieq(name, HLP_DefaultBgSelect)) {
            const char *clr = HLP()->context()->getDefaultColor(name);
            if (clr && *clr)
                fprintf(gp, "%s %s\n", name, clr);
        }
        else if (lstring::cieq(name, HLP_DefaultFgImagemap)) {
            const char *clr = HLP()->context()->getDefaultColor(name);
            if (clr && *clr)
                fprintf(gp, "%s %s\n", name, clr);
        }

        else if (lstring::cieq(name, "Alias"))
            HLP()->context()->dumpRootAliases(gp);
        else
            fprintf(stderr,
                "Warning: ignoring unknown keyword %s in %s file\n",
                name, STARTUP_FILE);

        delete [] name;
    }
    fclose(fp);
    fclose(gp);
}


// Dump a commented config file in SourceFilePath, $HOME, or the CWD,
// whichever works.  If we overwrite, save a back-up.
//
bool
HLPparams::dump()
{
    FILE *fp;
    if (SourceFilePath)
        fp = fopen(SourceFilePath, "r");
    else
        fp = open_cfgfile(&SourceFilePath, "r");
    if (fp) {
        // File exists, back it up.
        char *bkfile = new char[strlen(SourceFilePath) + 5];
        char *ee = lstring::stpcpy(bkfile, SourceFilePath);
        strcpy(ee, ".bak");

        FILE *gp = fopen(bkfile, "w");
        if (!gp) {
            fclose(fp);
            delete [] bkfile;
            return (false);
        }
        int c;
        while ((c = getc(fp)) != EOF)
            putc(c, gp);
        fclose(fp);
        fclose(gp);
    }

    if (SourceFilePath)
        fp = fopen(SourceFilePath, "w");
    else
        fp = open_cfgfile(&SourceFilePath, "w");
    if (!fp)
        return (false);

    time_t now = time(0);;
    struct tm *tgmt = gmtime(&now);
    char *gm = lstring::copy(asctime(tgmt));
    char *s = strchr(gm, '\n');
    if (s)
        *s = 0;

    fprintf(fp, "# MOZY for XicTools-3.3 created %.24s GMT\n", gm);
    delete [] gm;

    fprintf(fp,
"\n"
"# This is the startup file which sets defaults for the mozy HTML\n"
"# viewer and the Xic/WRspice help browser.  It should be installed as\n"   
"# \".mozyrc\" in the user's home directory, if the user wants to save\n"
"# changed settings.  Changes are not saved unless the file is present.\n"
"#\n"
"# This file will be overwritten so don't bother editing.\n"
"\n"
"# --- DISPLAY ATTRIBUTES -------------------------------------------------\n"
"\n"
"# Background color used for HTML pages that don't have a <body> tag,\n"
"# such as help text (default %s).\n", HLP_DEF_BG_COLOR);

    HLPcontext *cx = HLP()->context();
    const char *clr = cx->getDefaultColor(HLP_DefaultBgColor);
    if (!clr || !*clr)
        clr = HLP_DEF_BG_COLOR;
    fprintf(fp, "%s %s\n", HLP_DefaultBgColor, clr);

    fprintf(fp,
"\n"
"# Background image URL to use for pages that don't have a <body> tag\n"
"# (no default).\n");

    clr = cx->getDefaultColor(HLP_DefaultBgImage);
    if (clr && *clr)
        fprintf(fp, "%s %s\n", HLP_DefaultBgImage, clr);
    else
        fprintf(fp, "#%s %s\n", HLP_DefaultBgImage, "path/to/image");

    fprintf(fp,
"\n"
"# Text color to use for pages that don't have a <body> tag\n"
"# (default %s).\n", HLP_DEF_FG_COLOR);

    clr = cx->getDefaultColor(HLP_DefaultFgText);
    if (!clr || !*clr)
        clr = HLP_DEF_FG_COLOR;
    fprintf(fp, "%s %s\n", HLP_DefaultFgText, clr);

    fprintf(fp,
"\n"
"# Color to use for links in pages without a <body> tag\n"
"# (default %s).\n", HLP_DEF_LN_COLOR);

    clr = cx->getDefaultColor(HLP_DefaultFgLink);
    if (!clr || !*clr)
        clr = HLP_DEF_LN_COLOR;
    fprintf(fp, "%s %s\n", HLP_DefaultFgLink, clr);

    fprintf(fp,
"\n"
"# Color to use for visited links, (default %s).\n", HLP_DEF_VL_COLOR);

    clr = cx->getDefaultColor(HLP_DefaultFgVisitedLink);
    if (!clr || !*clr)
        clr = HLP_DEF_VL_COLOR;
    fprintf(fp, "%s %s\n", HLP_DefaultFgVisitedLink, clr);

    fprintf(fp,
"\n"
"# Color to use for activated link foreground, (default %s).\n",
        HLP_DEF_AL_COLOR);

    clr = cx->getDefaultColor(HLP_DefaultFgActiveLink);
    if (!clr || !*clr)
        clr = HLP_DEF_AL_COLOR;
    fprintf(fp, "%s %s\n", HLP_DefaultFgActiveLink, clr);

    fprintf(fp,
"\n"
"# Background color for selected text (default %s).\n", HLP_DEF_SL_COLOR);

    clr = cx->getDefaultColor(HLP_DefaultBgSelect);
    if (!clr || !*clr)
        clr = HLP_DEF_SL_COLOR;
    fprintf(fp, "%s %s\n", HLP_DefaultBgSelect, clr);

    fprintf(fp,
"\n"
"# Color to use for imagemap border, (default %s).\n", HLP_DEF_IM_COLOR);

    clr = cx->getDefaultColor(HLP_DefaultFgImagemap);
    if (!clr || !*clr)
        clr = HLP_DEF_IM_COLOR;
    fprintf(fp, "%s %s\n", HLP_DefaultFgImagemap, clr);

    fprintf(fp,
"\n"
"# How to handle images:\n"
"#  0 Don't display images that require downloading.\n"
"#  1 Download images when encountered in document.\n"
"#  2 Download images after document is displayed.\n"
"#  3 Display images progressively after document is displayed "
"(the default).\n");

    fprintf(fp, "%s %d\n", "ImageLoadMode", LoadMode);

    fprintf(fp,
"\n"
"# How to underline anchors when underlining is enabled:\n"
"#  0 No underline.\n"
"#  1 Single solid underline (default).\n"
"#  2 Double solid underline.\n"
"#  3 Single dashed underline.\n"
"#  4 Double dashed underline.\n");

    fprintf(fp, "%s %d\n", "AnchorUnderline", AnchorUnderlineType);

    fprintf(fp,
"\n"
"# If this is set to one anchors are shown as buttons.  If set to zero\n"
"# (the default), anchors use the underlining style.\n");

    fprintf(fp, "%s %d\n", "AnchorButtons", AnchorButtons);

    fprintf(fp,
"\n"
"# If set to one (the default) anchors will be highlighted when the pointer\n"
"# passes over them.  If zero, there will be no highlighting.\n");

    fprintf(fp, "%s %d\n", "AnchorHighlight", AnchorHighlight);

    if (Fnt()->getType() == GRfontX)
        fprintf(fp,
"\n"
"# The default font families.  This is the XLFD family name with \"-size\"\n"
"# appended.  Defaults:  adobe-times-normal-p-14   misc-fixed-normal-c-14\n");

    else if (Fnt()->getType() == GRfontP)
        fprintf(fp,
"\n"
"# The font families, defaults are Sans 9 and Monospace 9.\n");

    else
        fprintf(fp,
"\n"
"# The font families.\n");

    char *nm = Fnt()->getFamilyName(FNT_MOZY);
    if (nm) {
        fprintf(fp, "%s %s\n", "FontFamily", nm);
        delete [] nm;
    }
    else
        fprintf(fp, "%s %s\n", "#FontFamily", "???");
    nm = Fnt()->getFamilyName(FNT_MOZY_FIXED);
    if (nm) {
        fprintf(fp, "%s %s\n", "FixedFontFamily", nm);
        delete [] nm;
    }
    else
        fprintf(fp, "%s %s\n", "#FixedFontFamily", "???");

    fprintf(fp,
"\n"
"# If set to one, animations are frozen.  If zero (the default) animations\n"
"# will be shown normally.\n");

    fprintf(fp, "%s %d\n", "FreezeAnimations", FreezeAnimations);

    fprintf(fp,
"\n"
"# --- COMMUNICATIONS -----------------------------------------------------\n"
"\n"
"# Time in seconds allowed for a response from a message (0 for no timeout,\n"
"# to 600, default 15).\n");

    fprintf(fp, "%s %d\n", "Timeout", Timeout);

    fprintf(fp,
"\n"
"# Number of times to retry a message after a timeout (0 to 10, default 4).\n");

    fprintf(fp, "%s %d\n", "Retries", Retries);

    fprintf(fp,
"\n"
"# The port number used for HTTP communications (1 to 65536, default 80).\n");

    fprintf(fp, "%s %d\n", "HTTP_Port", HTTP_Port);

    fprintf(fp,
"\n"
"# The port number used for FTP communications (1 to 65536, default 21).\n");

    fprintf(fp, "%s %d\n", "FTP_Port", FTP_Port);

    fprintf(fp,
"\n"
"# --- GENERAL ------------------------------------------------------------\n"
"\n"
"# Number of cache files to save (2 to 4096, default 64).\n");

    fprintf(fp, "%s %d\n", "CacheSize", CacheSize);

    fprintf(fp,
"\n"
"# Set to one to disable disk cache, 0 (the default) enables cache.\n");

    fprintf(fp, "%s %d\n", "NoCache", NoCache);

    fprintf(fp,
"\n"
"# Set to one to disable sending and receiving cookies.\n");

    fprintf(fp, "%s %d\n", "NoCookies", NoCookies);

    fprintf(fp,
"\n"
"# The Alias line(s) implement a directory path aliasing feature for\n"
"# local file paths.  These will be applied to file paths in local URLs\n"
"# and image source paths.  For example, suppose we have\n"
"#   Alias /images /home/joeblow/pics\n"
"# If the source HTML ontains a line like\n"
"#   <img src=/images/mypic.gif ...>\n"
"# then mozy will look for the image /home/joeblow/pics/mypic.gif.  In\n"
"# general, if a local file path is prefixed with the first token of an\n"
"# Alias line, this path is internally mapped to a path where the\n"
"# second token is substituted.\n");

    if (HLP()->context()->haveRootAlias())
        HLP()->context()->dumpRootAliases(fp);
    else
        fprintf(fp, "#Alias /prefix  /some/other/directory\n");

    fprintf(fp,
"\n"
"# --- DEBUGGING ----------------------------------------------------------\n"
"\n"
"# Set this to one to print extended status messages on terminal screen\n"
"# (default 0).\n");

    fprintf(fp, "%s %d\n", "DebugMode", DebugMode);

    fprintf(fp,
"\n"
"# Set this to one to print transaction headers to terminal screen\n"
"# (default 0).\n");

    fprintf(fp, "%s %d\n", "PrintTransact", PrintTransact);

    fprintf(fp,
"\n"
"# Debugging mode for images:\n"
"#  0 Disable debugging mode (the default).\n"
"#  1 Load local images after document is displayed.\n"
"#  2 Display local images progressively after document is displayed.\n");

    fprintf(fp, "%s %d\n", "LocalImageTestMode", LocalImageTest);

    fprintf(fp,
"\n"
"# Issue warnings about bad HTML syntax to terminal (1) or not (0, the "
"default).\n");

    fprintf(fp, "%s %d\n", "BadHTMLwarnings", BadHTMLwarnings);

    fprintf(fp, "\n");

    fclose(fp);
    return (true);
}

