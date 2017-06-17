
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
 $Id: update_itf.cc,v 1.24 2017/04/16 20:27:53 stevew Exp $
 *========================================================================*/

#include "config.h"
#include "update_itf.h"
#include "crypt.h"
#include "httpget/transact.h"
#include "pathlist.h"
#include "filestat.h"
#include "miscutil.h"

#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif


//
// Class for downloading/installing new releases.
//

// NOTES:
// The version string consists of three integers in the form
//   generation.major.minor
//
// The distribution file names for non-Windows have the general form
//   program-osbase-version-arch.suffix
// and for Windows
//   program-version-setup.exe
//
// The osname is the string token used to identify different distribution
// targets.  This has the general form
//   (oper_sys)(identifier)[_(arch)]
// The osbase above is osname with the _(arch) suffix stripped.

#define DST_HOST    "wrcad.com/restricted"
#define DST_PWFILE  ".wrpasswd"
#define DST_PWKEY   "((~ aF&771b;4@wT"
#define DST_PXFILE  ".wrproxy"


// Construct according to the passed version string.  The version
// string is in the form (all ints) generation.major.minor.
//
release_t::release_t(const char *str)
{
    generation = 0;
    major = 0;
    minor = 0;

    char *t = lstring::gettok(&str, ".");
    generation = t ? atoi(t) : 0;
    delete [] t;
    t = lstring::gettok(&str, ".");
    major = t ? atoi(t) : 0;
    delete [] t;
    t = lstring::gettok(&str, ".");
    minor = t ? atoi(t) : 0;
    delete [] t;
}


// Compose a version string.
//
char *
release_t::string() const
{
    char buf[64];
    sprintf(buf, "%d.%d.%d", generation, major, minor);
    return (lstring::copy(buf));
}


bool
release_t::operator==(const release_t &r) const
{
    return (generation == r.generation && major == r.major &&
        minor == r.minor);
}


bool
release_t::operator<(const release_t &r) const
{
    if (generation > r.generation)
        return (false);
    if (generation < r.generation)
        return (true);
    if (major > r.major)
        return (false);
    if (major < r.major)
        return (true);
    return (minor < r.minor);
}
// End of release_t functions


UpdIf::UpdIf(const updif_t &itf)
{
    uif_user = 0;
    uif_password = 0;
    uif_home = lstring::copy(itf.HomeDir());
    uif_product = lstring::copy(itf.Product());
    uif_version = lstring::copy(itf.VersionString());
    uif_osname = lstring::copy(itf.OSname());
    uif_arch = lstring::copy(itf.Arch());
    uif_dist_suffix = lstring::copy(itf.DistSuffix());
    uif_prefix = lstring::copy(itf.Prefix());

    // Set the user/password from the .wrpasswd file in the users
    // home directory.

    char *home = pathlist::get_home("XIC_START_DIR");
    if (!home)
        return;

    char *pwpath = pathlist::mk_path(home, DST_PWFILE);
    delete [] home;

    FILE *fp = fopen(pwpath, "rb");
    if (fp) {

        sCrypt cr;
        cr.getkey(DST_PWKEY);
        cr.initialize();

        char buf[1024];
        if (cr.is_encrypted(fp)) {
            const char *err;
            if (cr.begin_decryption(fp, &err)) {
                int nc = fread(buf, 1, 1024, fp);
                if (nc > 0) {
                    cr.translate((unsigned char*)buf, nc);

                    // Do a sanity test on the decrypted info.
                    bool not_ascii = false;
                    int i, spcnt = 0;
                    for (i = 0; i < 1024 && buf[i]; i++) {
                        if (!isascii(buf[i])) {
                            not_ascii = true;
                            break;
                        }
                        if (isspace(buf[i]))
                            spcnt++;
                    }
                    if (!not_ascii && spcnt == 1 && i < 1024) {
                        char *s = buf;
                        uif_user = lstring::gettok(&s);
                        uif_password = lstring::gettok(&s);
                    }
                    // Else, file is corrupt.
                }
            }
        }
        else {
            char *s;
            while ((s = fgets(buf, 256, fp)) != 0) {
                char *user = lstring::gettok(&s);
                char *pass = lstring::gettok(&s);
                if (pass) {
                    uif_user = user;
                    uif_password = pass;
                    break;
                }
                delete [] user;
                delete [] pass;
            }
        }
        fclose(fp);
    }
    delete [] pwpath;
}


UpdIf::~UpdIf()
{
    delete [] uif_user;
    delete [] uif_password;
    delete [] uif_home;
    delete [] uif_product;
    delete [] uif_version;
    delete [] uif_osname;
    delete [] uif_arch;
    delete [] uif_dist_suffix;
    delete [] uif_prefix;
}


// Write a new encrypted password file in the user's home directory.
// Return 0 on success, otherwise an error message.
//
const char *
UpdIf::update_pwfile(const char *user, const char *pw)
{
    if (!user || !*user || !pw || !*pw)
        return ("user/password can't be null or empty.");

    if (!uif_home)
        return ("could not determine home directory.");
    char *pwpath = pathlist::mk_path(uif_home, DST_PWFILE);

    const char *err = 0;
    FILE *fp = fopen(pwpath, "wb");
    if (fp) {
        sCrypt cr;
        cr.getkey(DST_PWKEY);
        cr.initialize();

        char buf[256];
        strcpy(buf, user);
        strcat(buf, " ");
        strcat(buf, pw);

        if (!cr.begin_encryption(fp, &err, (unsigned char*)buf,
                strlen(buf)+1)) {
            unlink(pwpath);
            if (!err)
                err = "encryption failed, unknown error";
        }
        fclose (fp);
    }
    delete [] pwpath;
    return (err);
}


// Return the program name string for this product.
//
char *
UpdIf::program_name()
{
    char progname[32];
    char *td = progname;
    const char *ts = uif_product;
    if (ts) {
        while (*ts) {
            *td++ = isupper(*ts) ? tolower(*ts) : *ts;
            ts++;
        }
    }
    *td = 0;
    return (lstring::copy(progname));
}


// Return the version of this binary.
//
release_t
UpdIf::my_version()
{
    return (release_t(uif_version));
}


namespace {
    char *getline(char **s)
    {
        if (!s || !*s || !**s)
            return (0);
        char *str = *s;
        char *t = str;
        while (*t && *t != '\n')
            t++;
        int len = t - str;
        char *os = new char[len + 1];
        strncpy(os, str, len);
        os[len] = 0;
        if (*t == '\n')
            t++;
        *s = t;
        return (os);
    }
}


//---------
// The release data file on the server is named prognameRELEASE_SFX, and
// is located in DST_URL.
// The data file contains one or more lines of the form
//   osname version arch suffix
// Each line must contain exactly 4 tokens, or contain only white space.
//---------

#define RELEASE_SFX "_release4"

// Return the current version, from the web.
//
release_t
UpdIf::distrib_version(const char *osname, char **arch, char **suffix,
    char **subdir, char **errmsg)
{
    if (arch)
        *arch = 0;
    if (suffix)
        *suffix = 0;
    if (subdir)
        *subdir = 0;
    if (errmsg)
        *errmsg = 0;
    if (!osname)
        osname = uif_osname;

    release_t rel(0);
    char *progname = program_name();
    if (!progname)
        return (rel);

    if (!uif_user || !uif_password) {
        delete [] progname;
        return (rel);
    }

    Transaction trn;

    // Create the url:
    //    http://user:pass@wrcad.com/restricted/program_release4
    //
    sLstr lstr;
    lstr.add("http://");
    lstr.add(uif_user);
    lstr.add_c(':');
    lstr.add(uif_password);
    lstr.add_c('@');
    lstr.add(DST_HOST);
    lstr.add_c('/');
    lstr.add(progname);
    lstr.add(RELEASE_SFX);
    trn.set_url(lstr.string());
    delete [] progname;

    char *pxy = get_proxy();
    trn.set_proxy(pxy);
    delete [] pxy;

    trn.set_timeout(5);
    trn.set_retries(0);
    trn.set_http_silent(true);

    int error = trn.transact();
    if (error) {
        if (errmsg) {
            if (trn.response()->errorString())
                *errmsg = lstring::copy(trn.response()->errorString());
            else {
                char buf[128];
                sprintf(buf, "transaction failed: error %d.", error);
                *errmsg = lstring::copy(buf);
            }
        }
        return (rel);
    }

    char *s = trn.response()->data;
    char *t;
    while ((t = getline(&s)) != 0) {
        char *t0 = t;
        char *os = lstring::gettok(&t);
        if (!os) {
            delete [] t0;
            continue;
        }
        if (!strcmp(os, osname)) {
            char *vs = lstring::gettok(&t);
            if (arch)
                *arch = lstring::gettok(&t);
            else
                lstring::advtok(&t);
            if (suffix)
                *suffix = lstring::gettok(&t);
            else
                lstring::advtok(&t);
            if (subdir)
                *subdir = lstring::gettok(&t);
            else
                lstring::advtok(&t);

            release_t rt(vs);
            delete [] os;
            delete [] t0;
            delete [] vs;
            return (rt);
        }
        delete [] os;
        delete [] t0;
    }
    return (rel);
}


// Return the name of the program distribution file for the version
// passed.
//
char *
UpdIf::distrib_filename(const release_t &r, const char *osname,
    const char *arch, const char *suffix)
{
    char buf[256];
    char *progname = program_name();
    strcpy(buf, progname);
    delete [] progname;

    if (!osname)
        osname = uif_osname;
    char *vstr = r.string();
    if (lstring::cieq("win", osname))
        sprintf(buf + strlen(buf), "-%s-setup.exe", vstr);
    else {
        // strip off the srch suffix from osname, if any.
        char *osbase = lstring::copy(osname);
        char *t = strrchr(osbase, '_');
        if (t)
            *t = 0;
        if (!arch)
            arch = uif_arch;
        if (!suffix)
            suffix = uif_dist_suffix;
        sprintf(buf + strlen(buf), "-%s-%s-%s.%s", osbase, vstr, arch, suffix);
        delete [] osbase;
    }
    delete [] vstr;

    return (lstring::copy(buf));
}


// Download fname from the distribution repository into a temp
// directory.  If success, the function returns the full path name of
// the downloaded file.
//
char *
UpdIf::download(const char *fname, const char *osname, const char *subdir,
    char **errmsg, bool(*fb_func)(void*, const char*))
{
    if (errmsg)
        *errmsg = 0;

    if (!uif_user || !uif_password)
        return (0);

    // destination directory
    const char *tmpdir = getenv("XIC_TMP_DIR");
    if (!tmpdir || !*tmpdir)
        tmpdir = getenv("TMPDIR");
    if (!tmpdir || !*tmpdir)
        tmpdir = "/tmp";

    if (!osname)
        osname = uif_osname;

    // Create the url:
    //    http://user:pass@wrcad.com/restricted/xictools...
    //
    sLstr lstr;
    lstr.add("http://");
    lstr.add(uif_user);
    lstr.add_c(':');
    lstr.add(uif_password);
    lstr.add_c('@');
    lstr.add(DST_HOST);
    lstr.add("/xictools/");
    lstr.add(osname);
    lstr.add_c('/');
    if (subdir && *subdir) {
        lstr.add(subdir);
        lstr.add_c('/');
    }
    lstr.add(fname);

    Transaction trn;
    trn.set_url(lstr.string());
    char *pxy = get_proxy();
    trn.set_proxy(pxy);
    delete [] pxy;
    if (fb_func)
        trn.set_puts(fb_func);
    else
        trn.set_http_silent(true);
    char *dst = pathlist::mk_path(tmpdir, fname);
    trn.set_destination(dst);
    delete [] dst;

    int error = trn.transact();
    if (error) {
        if (errmsg) {
            if (trn.response()->errorString())
                *errmsg = lstring::copy(trn.response()->errorString());
            else {
                char buf[128];
                sprintf(buf, "transaction failed: error %d.", error);
                *errmsg = lstring::copy(buf);
            }
        }
        unlink(trn.destination());
        return (0);
    }
    return (lstring::copy(trn.destination()));
}


// Install the distfile under prefix.  Since this binary lives on (in
// a backup of the original installation), the user should be able to
// continue, but will use the new release on the next program startup. 
// True is returned on success.
//
int
UpdIf::install(const char *scriptfile, const char *cmdfmt,
    const char *distfile, const char *prefix, const char *osname,
    char **errmsg)
{
    if (errmsg)
        *errmsg = 0;

    if (!scriptfile || !*scriptfile) {
        if (errmsg)
            *errmsg = lstring::copy("Update script filename null or empty.");
        return (-1);
    }

    if (!distfile || !*distfile) {
        if (errmsg)
            *errmsg = lstring::copy("File name is null or empty.");
        return (-1);
    }
    if (!prefix)
        prefix = uif_prefix;
    if (!osname)
        osname = uif_osname;

    if (lstring::ciprefix("win", osname)) {
        if (lstring::ciprefix("win", uif_osname)) {
            int ret = system(distfile);
            return (ret);
        }
        *errmsg = lstring::copy(
            "Can't install Windows release on non-Windows system.");
        return (-1);
    }
    if (lstring::ciprefix("win", uif_osname)) {
        *errmsg = lstring::copy(
            "Can't install non-Windows release on Windows system.");
        return (-1);
    }

    char *cfmt = 0;
    if (!cmdfmt || !*cmdfmt) {
        // We use the sudo command for authorization, so this must be
        // available (requires installation in FreeBSD).  It is also
        // possible to use su:
        //
        //  cfmt = lstring::copy("su root -c \"%%s\"");
        //
        // With su, the password entered is the root password, and it
        // must be typed correctly, only one chance is given.  With
        // sudo, the password is the user's password, and three
        // attempts are allowed.  The user must be listed in
        // /etc/sudoers.

        cfmt = lstring::copy("sudo %s");
        cmdfmt = cfmt;
    }

    char *cmd = new char[strlen(scriptfile) + strlen(distfile) +
        strlen(prefix) + 20];
    sprintf(cmd, "/bin/sh %s -upd %s %s", scriptfile, distfile, prefix);
    char *xcmd = new char[strlen(cmd) + strlen(cmdfmt) + 2];
    if (strstr(cmdfmt, "%s"))
        sprintf(xcmd, cmdfmt, cmd);
    else
        sprintf(xcmd, "%s %s", cmdfmt, cmd);
    delete [] cfmt;
    delete [] cmd;

    int pid = miscutil::fork_terminal(xcmd);
    delete [] xcmd;

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
    return (-1);
}

#define DST_MSGURL  "http://wrcad.com/messages"
#define MSGSFX      "_current_mesg"
#define DGSFX       "_current_mesg_digest"
#define LOCALDIR    ".wr_cache"
//#define DEBUG


namespace {
    void err_handler(Transaction*, const char *msg)
    {
#ifdef DEBUG
        printf("%s\n", msg);
#else
        (void)msg;
#endif
    }
}


// Static function.
// This is somewhat orthogonal to the rest of the class.  Download and
// return a message from the distribution site, if the message has not
// been downloaded before.
//
char *
UpdIf::message(const char *progname)
{
    if (!progname)
        return (0);
    char buf[256];
    Transaction *trn = new Transaction;
    trn->set_timeout(5);
    trn->set_retries(0);
    trn->set_http_silent(true);
    trn->set_err_func(err_handler);

    // Download the message digest.

    sprintf(buf, "%s/%s%s", DST_MSGURL, progname, DGSFX);
    trn->set_url(buf);

    char *pxy = get_proxy();
    trn->set_proxy(pxy);
    delete [] pxy;

    int error = trn->transact();
    if (error) {
#ifdef DEBUG
        printf("Error getting digest:\n%s\n", trn->response()->errorString());
#endif
        delete trn;
        return (0);
    }

    const char *s = trn->response()->data;
    unsigned long h1 = strtoul(s, 0, 0);
    delete trn;
    if (!h1)
        return (0);

    // Check if this digest has alread been seen.  We use the mozy
    // cache directory for local storage.

    // The digest can be any number, we set it to the output from the
    // sum command on the message file.  The second number (block
    // count) will be ignored.

    char *p = pathlist::get_home(0);
    if (!p)
        return (0);
    char *dir = pathlist::mk_path(p, LOCALDIR);
    delete [] p;

    unsigned long h2 = 0;
    sprintf(buf, "%s/%s%s", dir, progname, DGSFX);
    FILE *fp = fopen(buf, "r");
    if (fp) {
        s = fgets(buf, 256, fp);
        fclose(fp);
        h2 = strtoul(s, 0, 0);
    }
    if (h1 == h2) {
        delete [] dir;
        return (0);
    }

    // Message is new, download and save it, and the digest.

    trn = new Transaction;
    trn->set_timeout(5);
    trn->set_retries(0);
    trn->set_http_silent(true);
    trn->set_err_func(err_handler);

    sprintf(buf, "%s/%s%s", DST_MSGURL, progname, MSGSFX);
    trn->set_url(buf);

    pxy = get_proxy();
    trn->set_proxy(pxy);
    delete [] pxy;

    error = trn->transact();
    if (error) {
#ifdef DEBUG
        printf("Error getting message:\n%s\n", trn->response()->errorString());
#endif
        delete [] dir;
        delete trn;
        return (0);
    }
    // Save the message for return.
    char *msg = lstring::copy(trn->response()->data);
    delete trn;

    // Make sure that the local directory exists.
#ifdef WIN32
    mkdir(dir);
#else
    mkdir(dir, 0755);
#endif

    // Save the digest.
    sprintf(buf, "%s/%s%s", dir, progname, DGSFX);
    fp = fopen(buf, "w");
    if (fp) {
        fprintf(fp, "%ld\n", h1);
        fclose(fp);
    }

    // Save the message, and return it.
    sprintf(buf, "%s/%s%s", dir, progname, MSGSFX);
    fp = fopen(buf, "w");
    if (fp) {
        fwrite(msg, strlen(msg), 1, fp);
        fclose(fp);
    }
    delete [] dir;
    return (msg);
}


#define RELSFX      "_current_release"

namespace {
    // The strings are in the form gen.major.minor (all integers). 
    // Return -1, 0, 1 if r1 is earlier, equal to or later than r2.
    // Return -1 if error.
    //
    int relcmp(const char *r1, const char *r2)
    {
        if (!r1)
            return (-1);
        if (!r2)
            return (1);
        int v1[3];
        int n1 = sscanf(r1, "%d.%d.%d", v1, v1+1, v1+2);
        int v2[3];
        int n2 = sscanf(r2, "%d.%d.%d", v2, v2+1, v2+2);

        if (n1 > 0 && n2 > 0) {
            if (v1[0] < v2[0])
                return (-1);
            if (v1[0] > v2[0])
                return (1);
            if (n1 == 1 && n2 == 1)
                return (0);
            if (n1 == 1 || n2 == 1)
                return (-1);
        }
        if (n1 > 1 && n2 > 1) {
            if (v1[1] < v2[1])
                return (-1);
            if (v1[1] > v2[1])
                return (1);
            if (n1 == 2 && n2 == 2)
                return (0);
            if (n1 == 2 || n2 == 2)
                return (-1);
        }
        if (n1 > 2 && n2 > 2) {
            if (v1[2] < v2[2])
                return (-1);
            if (v1[2] > v2[2])
                return (1);
            return (0);
        }
        return (-1);
    }
}


// Static function
// Return true if the current release is not saved in a
// $HOME/.wr_cache/<progname>_current_release file.  In this case,
// create or update the file.  The true return signals the application
// to display a new release message.
//
bool
UpdIf::new_release(const char *progname, const char *release)
{
    if (!progname || !release || !*release)
        return (false);
    char *p = pathlist::get_home(0);
    if (!p)
        return (0);
    char *dir = pathlist::mk_path(p, LOCALDIR);
    delete [] p;

    char *filerel = 0;
    char buf[256];
    sprintf(buf, "%s/%s%s", dir, progname, RELSFX);

    FILE *fp = fopen(buf, "r");
    if (fp) {
        const char *s = fgets(buf, 256, fp);
        fclose(fp);
        filerel = lstring::gettok(&s);
    }
    char *rel = lstring::gettok(&release);
    int val = relcmp(rel, filerel);
    if (val < 1) {
        // Release is earlier or the same as the file release,
        // just clean up and return.
        delete [] dir;
        delete [] filerel;
        delete [] rel;
        return (false);
    }
    delete [] filerel;

    // Release is newer, or the file doesn't exist, update.

    // Make sure that the local directory exists.
#ifdef WIN32
    mkdir(dir);
#else
    mkdir(dir, 0755);
#endif

    // Create or update the file.
    sprintf(buf, "%s/%s%s", dir, progname, RELSFX);
    fp = fopen(buf, "w");
    if (fp) {
        fwrite(rel, strlen(rel), 1, fp);
        fclose(fp);
    }
    delete [] dir;
    delete [] rel;
    return (true);
}


// Static function.
// Return the first line of the $HOME/.wrproxy file if found.
//
char *
UpdIf::get_proxy()
{
    char *home = pathlist::get_home("XIC_START_DIR");
    if (!home)
        return (0);

    char *pwpath = pathlist::mk_path(home, DST_PXFILE);
    delete [] home;

    FILE *fp = fopen(pwpath, "r");
    delete [] pwpath;
    if (fp) {
        char buf[256];
        const char *s = fgets(buf, 256, fp);
        fclose(fp);
        return (lstring::getqtok(&s));
    }
    return (0);
}


// Static function.
// Write a $HOME/.wrproxy file.  The addr may already have a :port
// field, in which case port should be null.  The port will be parsed
// as an integer and if valid added to the addr following a colon.
//
const char *
UpdIf::set_proxy(const char *addr, const char *port)
{
    int p = 0;
    if (port && *port) {
        int tmp;
        if (sscanf(port, "%d", &tmp) == 1 && tmp > 0)
            p = tmp;
        else
            return ("bad port number");
    }
    char *a = lstring::gettok(&addr);
    if (!a)
        return ("bad host address");

    char *home = pathlist::get_home("XIC_START_DIR");
    if (!home) {
        delete [] a;
        return ("can't determine home directory");
    }

    char *pwpath = pathlist::mk_path(home, DST_PXFILE);
    delete [] home;

    FILE *fp = fopen(pwpath, "w");
    delete [] pwpath;
    if (fp) {
        if (p > 0)
            fprintf(fp, "http://%s:%d\n", a, p);
        else
            fprintf(fp, "http://%s\n", a);
        delete [] a;
        fclose (fp);
        return (0);
    }
    delete [] a;
    return ("can't open "DST_PXFILE" file for writing");
}


// Move the .wrproxy file according to the token:
//    "-"     .wrproxy -> .wrproxy.bak
//    "-abc"  .wrprozy -> .wrproxy.abc
//    "+"     .wrproxy.bak -> .wrproxy
//    "+abc"  .wrproxy.abc -> .wrproxy.
//
// Just return 0 if token doesn't start with - or +.
// Return a message on error.
//
const char *
UpdIf::move_proxy(const char *token)
{
    int c = *token++;
    if (c != '-' && c != '+')
        return (0);
    char *home = pathlist::get_home(0);
    if (!home)
        return ("can't determine home directory.");
    char *f1 = pathlist::mk_path(home, DST_PXFILE);
    char *f2;
    if (!*token)
        f2 = pathlist::mk_path(home, DST_PXFILE".bak");
    else {
        f2 = new char[strlen(f1) + strlen(token) + 2];
        sprintf(f2, "%s.%s", f1, token);
    }

    bool ret;
    if (c == '-')
        ret = filestat::move_file_local(f1, f2);
    else
        ret = filestat::move_file_local(f2, f1);
    delete [] home;
    delete [] f1;
    delete [] f2;
    return (ret ? 0 : filestat::error_msg());
}

