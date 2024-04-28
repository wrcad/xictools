
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "help_defs.h"
#include "help_context.h"
#include "help_pkgs.h"
#include "miscutil/lstring.h"
#include "miscutil/pathlist.h"
#include "miscutil/miscutil.h"
#include "httpget/transact.h"
#ifdef WIN32
#include "miscutil/msw.h"
#endif
#include <unistd.h>

//
// Implementation of package management for XicTools.
//


#ifndef OSNAME
#define OSNAME "unknown"
#endif
#ifndef ARCH
#define ARCH "unknown"
#endif

namespace {
    void find_pkg(const char *prog, stringlist **l1, stringlist **l2)
    {
        if (l1) {
            while (*l1) {
                // 9 == strlen*("xictools_")
                if (lstring::prefix(prog, (*l1)->string + 9))
                    break;
                *l1 = (*l1)->next;
            }
        }
        if (l2) {
            while (*l2) {
                // 9 == strlen*("xictools_")
                if (lstring::prefix(prog, (*l2)->string + 9))
                    break;
                *l2 = (*l2)->next;
            }
        }
    }

    // Return the next unsigned int found, -1 if none, while skipping
    // non-numeric.  Advance pointer.
    //
    int nextint(const char **s)
    {
        const char *p = *s;
        while (*p && !isdigit(*p))
            p++;
        if (!*p)
            return (-1);  // error return
        int i = atoi(p);
        while (isdigit(*p))
            p++;
        *s = p;
        return (i);
    }

    bool newerpkg(stringlist *loc, stringlist *avl)
    {
        if (!loc || !avl)
            return (false);
        const char *locpkg = loc->string;
        const char *avlpkg = avl->string;

        delete [] lstring::gettok(&locpkg, "-");  // xictools_prog
        delete [] lstring::gettok(&locpkg, "-");  // osname
        int lgen = nextint(&locpkg);
        int lmaj = nextint(&locpkg);
        int lmin = nextint(&locpkg);
        if (lmin < 0)
            return (false);

        delete [] lstring::gettok(&avlpkg, "-");  // xictools_prog
        delete [] lstring::gettok(&avlpkg, "-");  // osname
        int agen = nextint(&avlpkg);
        int amaj = nextint(&avlpkg);
        int amin = nextint(&avlpkg);
        if (amin < 0)
            return (false);
        if (agen > lgen)
            return (true);
        if (agen < lgen)
            return (false);
        if (amaj > lmaj)
            return (true);
        if (amaj < lmaj)
            return (false);
        if (amin > lmin)
            return (true);
        return (false);
    }

    const char *xt_pkgs[] = { "adms", "fastcap", "fasthenry", "mozy",
        "mrouter", "vl", "wrspice", "xic", 0 };
}


// Compose an HTML page listing the installed packages, and packages
// that are available on the distribution site.  The buttons allow
// initiation of downloading/installation of selected packages.
//
char *
pkgs::pkgs_page()
{
    stringlist *local = local_pkgs();
    stringlist *avail = avail_pkgs();

    sLstr lstr;
    lstr.add("<html>\n<head>\n<body bgcolor=#ffffff>\n");
    lstr.add("<h2><i>XicTools</i> Packages</h4>\n");
    lstr.add("<blockquote>\n");

    lstr.add("<form method=get action=\":xt_download\">\n");
    lstr.add("<table cellpadding=4>\n");
    lstr.add("<tr><th>Installed</th><th>Available</th><th>download</th></tr>\n");
    for (const char **pn = xt_pkgs; *pn;  pn++) {
        const char *prog = *pn;
        stringlist *locpkg = local;
        stringlist *avlpkg = avail;
        find_pkg(prog, &locpkg, &avlpkg);
        if (!avlpkg && !locpkg)
            continue;
        bool newer = newerpkg(locpkg, avlpkg);
        if (newer) {
            lstr.add("<tr><td bgcolor=#fff0f0>");
        }
        else {
            lstr.add("<tr><td bgcolor=#f0fff0>");
        }
        if (locpkg)
            lstr.add(locpkg->string);
        else
            lstr.add("not found");
        lstr.add("</td><td>");
        if (avlpkg)
            lstr.add(avlpkg->string);
        else
            lstr.add("not found");
        lstr.add("</td><td>");
        if (avlpkg) {
            lstr.add("<center><input type=checkbox name=d_");
            lstr.add(avlpkg->string);
            lstr.add("></center>");
        }
        lstr.add("</td></tr>\n");
    }
    lstr.add("</table>\n");

    lstr.add("<input type=submit value=\"Download\">\n");
    lstr.add("<input type=submit value=\"Download and Test Install\">\n");
    lstr.add("<input type=submit value=\"Download and Install\">\n");
    lstr.add("</form>\n");
    lstr.add("</body>\n</html>\n");
    return (lstr.string_trim());
}


#ifdef WIN32

namespace {
    char *msw_exepath;

    void msw_install()
    {
        if (!msw_exepath)
            return;
        PROCESS_INFORMATION *info = msw::NewProcess(0, msw_exepath,
            DETACHED_PROCESS | CREATE_NEW_PROCESS_GROUP, false);
        (void)info;
    }
}

#endif


// Fork a terminal and start the installation process.
//
int
pkgs::xt_install(dl_elt_t *dlist, bool dryrun)
{
    if (!dlist)
        return (-1);
    sLstr lstr;
    lstr.add("/bin/bash ");
    for (dl_elt_t *d = dlist; d; d = d->next) {
        if (lstr.string()) {
            lstr.add_c(' ');
            lstr.add(d->filename);
        }
        else {
            lstr.add(d->filename);
            if (dryrun)
                lstr.add(" -t");
        }
    }
    // The lstr contains the command string, since the first element
    // is the wr_install script, and remaining elements are package
    // files.
    
    char *cmd = lstr.string_trim();
#ifdef WIN32
    // Can't overwrite the current executable under Windows, will
    // create a new process to do the install on program exit.

    if (!atexit(msw_install)) {
        msw_exepath = cmd;
        MessageBox(0, "Exit this program to install updates.", "Info", MB_OK);
        return (0);
    }
    MessageBox(0, "Unknown error when scheduling exit process.", "Error",
        MB_OK);
#else
    int pid = miscutil::fork_terminal(cmd);
    delete [] cmd;

    if (pid > 0) {
        int status = 0;
#ifdef HAVE_SYS_WAIT_H
        for (;;) {
            int rpid = waitpid(pid, &status, 0);
            if (rpid == -1 && errno == EINTR)
                continue;
            break;
        }
#endif
        return (status);
    }
#endif
    return (-1);
}


namespace {
    char *get_osname(const char *pkgname)
    {
        const char *t = strchr(pkgname, '-');
        if (!t)
            return (0);
        t++;
        const char *e = strchr(t, '-');
        int len = e ? e-t : strlen(t);
        char *n = new char[len+1];
        strncpy(n, t, len);
        n[len] = 0;
        return (n);
    }


    /* not used
    char *get_version(const char *pkgname)
    {
        const char *t = strchr(pkgname, '-');
        if (!t)
            return (0);
        t++;
        t = strchr(t, '-');
        if (!t)
            return (0);
        t++;
        const char *e = strchr(t, '-');
        int len = e ? e-t : strlen(t);
        char *n = new char[len+1];
        strncpy(n, t, len);
        n[len] = 0;
        return (n);
    }
    */


    char *get_arch(const char *pkgname)
    {
        const char *t = strchr(pkgname, '-');
        if (!t)
            return (0);
        t++;
        t = strchr(t, '-');
        if (!t)
            return (0);
        t++;
        t = strchr(t, '-');
        if (!t)
            return (0);
        t++;
        const char *e = strchr(t, '-');
        int len = e ? e-t : strlen(t);
        char *n = new char[len+1];
        strncpy(n, t, len);
        n[len] = 0;
        return (n);
    }
}


// Construct the url to download the package whose name is passed.
//
char *
pkgs::get_download_url(const char *pkgname)
{
    if (!pkgname)
        return (0);
    if (!strcmp(pkgname, "wr_install")) {
        sLstr lstr;
        lstr.add("http://wrcad.com/xictools/");
        lstr.add("scripts");
        lstr.add_c('/');
        lstr.add(pkgname);
        return (lstr.string_trim());
    }
    char *osn = get_osname(pkgname);
    if (!osn)
        return (0);
    char *arch = get_arch(pkgname);
    if (!arch) {
        delete [] osn;
        return (0);
    }
    const char *sfx = 0;
    if (lstring::prefix("Linux", osn))
        sfx = ".rpm";
    else if (lstring::prefix("Darwin", osn))
        sfx = ".pkg";
    else if (lstring::prefix("Win", osn))
        sfx = ".exe";
    else {
        delete [] osn;
        delete [] arch;
        return (0);
    }
    if (strcmp(arch, "x86_64")) {
        char *t = new char[strlen(osn) + strlen(arch) + 2];
        char *e = lstring::stpcpy(t, osn);
        delete [] osn;
        osn = t;
        *e++ = '.';
        strcpy(e, arch);
    }
    sLstr lstr;
    lstr.add("http://wrcad.com/xictools/");
    lstr.add(osn);
    lstr.add_c('/');
    lstr.add(pkgname);
    lstr.add(sfx);
    return (lstr.string_trim());
}


// Return a raw list pf packages available for download.  The first
// list element is actually the OSNAME.
//
stringlist *
pkgs::avail_pkgs()
{
    Transaction trn;

    // Create the url:
    //    http://wrcad.com/cgi-bin/cur_release.cgi?h=osname
    //
    char buf[256];
    char *e = lstring::stpcpy(buf,
        "http://wrcad.com/cgi-bin/cur_release.cgi?h=");
    e = lstring::stpcpy(e, OSNAME);
    if (!strcmp(OSNAME, "Win32")) {
        if (strcmp(ARCH, "i386")) {
            *e++ = '.';
            strcpy(e, ARCH);
        }
    }
    else {
        if (strcmp(ARCH, "x86_64")) {
            *e++ = '.';
            strcpy(e, ARCH);
        }
    }
    trn.set_url(buf);

    trn.set_timeout(5);
    trn.set_retries(0);
    trn.set_http_silent(true);

    trn.transact();
    char *s = trn.response()->data;
    stringlist *list = 0;
    if (s && *s) {
        const char *ss = s;
        const char *osn = 0;
        const char *arch = 0;
        char *t;
        while ((t = lstring::gettok(&ss)) != 0) {
            if (!osn) {
                osn = t;
                char *a = strchr(t, '.');
                if (a) {
                    *a++ = 0;
                    arch = a;
                }
                else
                    arch = "x86_64";
                continue;
            }
            char *vrs = strchr(t, '-');
            if (!vrs)
                break;
            *vrs++ = 0;
            snprintf(buf, sizeof(buf), "xictools_%s-%s-%s-%s",
                t, osn, vrs, arch);
            list = new stringlist(lstring::copy(buf), list);
            delete [] t;
        }
        if (list && list->next)
            stringlist::sort(list);
    }
    return (list);
}


// Return a raw list of the installed XicTools packages.
//
stringlist *
pkgs::local_pkgs()
{
    stringlist *list = 0;
    char buf[256];
#ifdef WIN32
    for (const char **pp = xt_pkgs; *pp; pp++) {
        const char *prog = *pp;
        char *vrs = msw::GetInstallData(prog, "DisplayVersion");
        if (!vrs)
            continue;
        snprintf(buf, sizeof(buf), "xictools_%s-Win32-%s-i386", prog, vrs);
        delete [] vrs;
        list = new stringlist(lstring::copy(buf), list);
    }
    if (list && list->next)
        stringlist::sort(list);
#else
#ifdef __APPLE__
    // NOTE:  The original 4.3 packages had names like "xictools_xic". 
    // This was changed to "xictools_xic-4.3.x".  On update, we must
    // ensure that the earlier package is removed , i.e., run pkgutil
    // --forget on it, from the preinstall script.

    FILE *fp = popen("pkgutil --pkgs | grep xictools", "r");
    if (fp) {
        char *s;
        while ((s = fgets(buf, 256, fp)) != 0) {
            char *e = s + strlen(s) - 1;
            while (e >= s && isspace(*e))
                *e-- = 0;
            char *vers = strchr(s, '-');
            if (vers) {
                vers++;
                char *v = lstring::copy(vers);
                int len = vers - buf;
                snprintf(vers, sizeof(buf) - len, "Darwin-%s-x86_64", v);
                delete [] v;
            }
            else {
                const char *ss = s + 9;  // strlen("xictools_")
                const char *v = 0;
                if (!strcmp("adms", ss))
                    v = "2.3.60";
                else if (!strcmp("fastcap", ss))
                    v = "2.0.11";
                else if (!strcmp("fasthenry", ss))
                    v = "3.0.12";
                else if (!strcmp("mozy", ss))
                    v = "4.3.1";
                else if (!strcmp("mrouter", ss))
                    v = "1.2.1";
                else if (!strcmp("vl", ss))
                    v = "4.3.1";
                else if (!strcmp("wrspice", ss))
                    v = "4.3.1";
                else if (!strcmp("xic", ss))
                    v = "4.3.1";
                else
                    continue;  // WTF? can't happen
                int len = strlen(s);
                snprintf(s + len, sizeof(buf) - len, "-Darwin-%s-x86_64", v);
            }
            list = new stringlist(lstring::copy(s), list);
        }
        if (list && list->next)
            stringlist::sort(list);
        fclose(fp);
    }
#else
    if (!access("/usr/bin/rpm", X_OK) || !access("/bin/rpm", X_OK)) {
        FILE *fp = popen("rpm -qa | grep xictools_", "r");
        if (fp) {
            char *s;
            while ((s = fgets(buf, 256, fp)) != 0)
                list = new stringlist(lstring::copy(s), list);
            pclose(fp);
        }
    }

    if (!access("/usr/bin/dpkg", X_OK) || !access("/bin/dpkg", X_OK)) {
        FILE *fp = popen("dpkg-query --show | grep xictools-", "r");
        if (fp) {
            char *s;
            while ((s = fgets(buf, 256, fp)) != 0) {
                char *pname = lstring::gettok(&s);
                char *vers = lstring::gettok(&s);
                if (!vers) {
                    delete [] pname;
                    continue;
                }
                // The pname is in a form like "xictools-xic-ubuntu17".
                char *os = strchr(pname, '-');
                *os++ = '_';  // now xictools_xic...
                os = strchr(os+1, '-');
                if (!os) {
                    delete [] pname;
                    delete [] vers;
                    continue;
                }
                *os++ = 0;
                *os = toupper(*os);
                snprintf(buf, sizeof(buf), "%s-Linux%s-%s", pname, os, vers);
                delete [] pname;
                delete [] vers;
                list = new stringlist(lstring::copy(buf), list);
            }
            pclose(fp);
        }
    }
    if (list && list->next)
        stringlist::sort(list);

#endif
#endif
    return (list);
}


// Return a page showing the XicTools packages available for download.
//
char *
pkgs::list_avail_pkgs()
{
    sLstr lstr;
    lstr.add("<html>\n<head>\n<body bgcolor=#ffffff>\n");
    lstr.add("<h4>Available Packages</h4>\n");
    lstr.add("<blockquote>\n");

    stringlist *list = avail_pkgs();
    if (list) {
        for (stringlist *sl = list; sl; sl = sl->next) {
            lstr.add(sl->string);
            lstr.add("<br>\n");
        }
        stringlist::destroy(list);
    }
    else {
        lstr.add("Sorry, prebuilt packages for ");
        lstr.add(OSNAME);
        if (strcmp(ARCH, "x86_64")) {
            lstr.add_c('.');
            lstr.add(ARCH);
        }
        lstr.add(" are not currently available.\n");
    }
    lstr.add("</blockquote>\n");
    lstr.add("</body>\n</html>\n");
    return (lstr.string_trim());
}


// Return a page showing the currently installed XicTools packages.
//
char *
pkgs::list_cur_pkgs()
{
    sLstr lstr;
    lstr.add("<html>\n<head>\n<body bgcolor=#ffffff>\n");
    lstr.add("<h4>Currently Installed Packages</h4>\n");
    lstr.add("<blockquote>\n");

    stringlist *list = local_pkgs();
    if (list) {
        for (stringlist *sl = list; sl; sl = sl->next) {
            lstr.add(sl->string);
            lstr.add("<br>");
        }
        stringlist::destroy(list);
    }
    else
        lstr.add("Error: package database query failed.");

    lstr.add("</blockquote>\n");
    lstr.add("</body>\n</html>\n");
    return (lstr.string_trim());
}

