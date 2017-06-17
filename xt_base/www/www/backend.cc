
/*========================================================================*
 *                                                                        *
 *  Copyright (c) 2016 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
 *  Author:  Stephen R. Whiteley (stevew@wrcad.com)
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Back end for forms processing.
 *                                                                        *
 *========================================================================*
 $Id: backend.cc,v 1.3 2016/01/18 18:52:12 stevew Exp $
 *========================================================================*/

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>
#include "backend.h"


namespace {
    // Substitute in place %xx forms (x is a hex digit) with the actual
    // character value.
    //
    void subpct(char *str)
    {
        if (!str)
            return;
        for (char *s = str; *s; s++) {
            if (*s == '%' && isxdigit(s[1]) && isxdigit(s[2])) {
                char tbf[4];
                tbf[0] = s[1];
                tbf[1] = s[2];
                tbf[2] = 0;
                int d;
                if (sscanf(tbf, "%x", &d) == 1) {
                    if (d == '\r') {
                        // strip these
                        for (char *t = s; (t[0] = t[3]) != 0; t++) ;
                    }
                    else {
                        *s++ = d;
                        for (char *t = s; (t[0] = t[2]) != 0; t++) ;
                    }
                    s--;
                }
            }
        }
    }


    // Strip leading and trailing white space.
    //
    char *gettok(const char *str)
    {
        if (!str)
            return (0);
        while (isspace(*str))
            str++;
        char *s = strdup(str);
        char *e = s + strlen(s) - 1;
        while (e >= s && isspace(*e))
            *e-- = 0;
        return (s);
    }
}


// Static function.
// The str contains the query string.  Parse this and fill in the
// key/value pairs.  Return false if there are too many keys.
//
bool
sBackEnd::get_keyvals(sBackEnd::keyval **kvalp, int *nkeys, const char *str)
{
    char *qstr = strdup(str);
#define MAXKEYS 50
    keyval *kval = new keyval[MAXKEYS];

    // set '+' to space
    for (char *s = qstr; *s; s++) {
        if (*s == '+')
            *s = ' ';
    }

    // split key=value pairs at '&'
    int i = 0;
    char *s = qstr;
    while (s) {
        char *e = strchr(s, '&');
        if (e)
            *e++ = 0;
        kval[i++].key = s;
        if (i == MAXKEYS) {
            // Too many keys, shouldn't happen.
            return (false);
        }
        s = e;
    }
    int nkv = i;

    // split pairs at '='
    for (i = 0; i < nkv; i++) {
        char *e = strchr(kval[i].key, '=');
        if (e)
            *e++ = 0;
        kval[i].val = e && *e ? e : 0;
    }

    // substitute %dd
    for (i = 0; i < nkv; i++) {
        subpct(kval[i].key);
        subpct(kval[i].val);
    }
    *kvalp = kval;
    *nkeys = nkv;
    return (true);
}


// Static function.
// Spew message and die.
//
void
sBackEnd::errordump(const char *msg)
{
    printf("<b>ERROR:</b> %s.\n", msg);
    printf("<p>Use your browser's <b>Back</b> button to return to the "
        "previous page.\n");
    exit(1);
}


// Set the host name field.
//
bool
sBackEnd::set_hostname(const char *str)
{
    if (!str || !*str) {
        be_errmsg = "null or empty host name";
        return (false);
    }

    int len = strlen(str);
    if (len > 64) {
        be_errmsg = "host name string too long";
        return (false);
    }
    be_hostname = gettok(str);

    // strip the domain
    char *s = strchr(be_hostname, '.');
    if (s)
        *s = 0;
    if (!*be_hostname) {
        be_errmsg = "bad host name";
        return (false);
    }

    // convert to lower case
    for (s = be_hostname; *s; s++) {
        if (isupper(*s))
            *s = tolower(*s);
    }
    if (!strcmp(be_hostname, "localhost")) {
        be_errmsg = "the generic host name \"localhost\" is not allowed";
        return (false);
    }
    return (true);
}


// Set the key text field.
//
bool
sBackEnd::set_keytext(const char *str)
{
    if (!str || !*str) {
        be_errmsg = "null or empty key text";
        return (false);
    }

    int len = strlen(str);
    if (len > 64) {
        be_errmsg = "key text string too long";
        return (false);
    }
    be_keytext = gettok(str);

    if (be_ostype == OS_LINUX) {
        int d;
        if (sscanf(be_keytext, "%x:%x:%x:%x:%x:%x",
                &d, &d, &d, &d, &d, &d) != 6) {
            be_errmsg = "key Text should be HWaddr, six colon-separated "
                "2-digit hex numbers.";
            return (false);
        }

        // Lower-case the HWaddr.
        for (char *s = be_keytext; *s; s++) {
            if (isupper(*s))
                *s = tolower(*s);
        }
    }
    return (true);
}


// Set the email address to be associated with the key.
//
bool
sBackEnd::set_email(const char *str)
{
    if (!str || !*str) {
        be_errmsg = "null or empty email address";
        return (false);
    }

    int len = strlen(str);
    if (len > 80) {
        be_errmsg = "email address string too long";
        return (false);
    }
    be_email = gettok(str);

    if (!strchr(be_email, '@') || strchr(be_email, ',') ||
            strchr(be_email, '/') || strchr(be_email, ':') ||
            strchr(be_email, ';')) {
        be_errmsg = "bad email address";
        return (false);
    }
    return (true);
}


// Set the contact info to be associated with the key.
//
bool
sBackEnd::set_info(const char *str)
{
    if (!str || !*str) {
        be_errmsg = "null or empty contact info";
        return (false);
    }

    int len = strlen(str);
    if (len > 2048) {
        be_errmsg = "contact info string too long";
        return (false);
    }
    be_info = strdup(str);
    return (true);
}


// Create and return the user's license key.
//
const char *
sBackEnd::get_key()
{
    if (be_key)
        return (be_key);

    if (!be_keytext) {
        be_errmsg = "no key text given";
        return (0);
    }
    if (!be_email) {
        be_errmsg = "no email address given";
        return (0);
    }
    if (!be_info) {
        be_errmsg = "no contact info given";
        return (0);
    }
    if (be_ostype != OS_OSX) {
        // The hostname isn't used for Apple. 
        if (!be_hostname) {
            be_errmsg = "no host name given";
            return (0);
        }
    }

    // We want to use the key as a username for htpasswd, so it can't
    // have colons.  Strip these out.

    char *ktxt = new char[strlen(be_keytext) + 1];
    char *s = ktxt;
    char *t = be_keytext;
    while (*t) {
        if (*t != ':')
            *s++ = *t;
        t++;
    }
    *s = 0;

    if (be_ostype == OS_OSX) {
        be_key = new char[strlen(ktxt) + 7];
        sprintf(be_key, "%s-%s", "apple", ktxt);
    }
    else {
        be_key = new char[strlen(be_hostname) + strlen(ktxt) + 2];
        sprintf(be_key, "%s-%s", be_hostname, ktxt);
    }
    delete [] ktxt;
    return (be_key);
}


// Set the key field.
//
bool
sBackEnd::set_key(const char *str)
{
    be_key = gettok(str);
    if (!be_key || !*be_key) {
        be_errmsg = "no key given";
        return (false);
    }
    if (!key_exists(be_key)) {
        be_errmsg = "key not found in database";
        return (false);
    }
    return (true);
}


// Save the data associated with the key in a file.
//
bool
sBackEnd::save_key_data()
{
    if (!get_key())
        return (false);
    char *dpath = new char[strlen(KEYDIR) + strlen(be_key) + 2];
    sprintf(dpath, "%s/%s", KEYDIR, be_key);
    char *path = new char[strlen(dpath) + strlen(KEYDATA) + 2];
    sprintf(path, "%s/%s", dpath, KEYDATA);

    // Backup datafile if key exists.
    if (key_exists(be_key)) {
        if (access(path, F_OK) == 0) {
            time_t t = time(0);
            char *npath = new char[strlen(path) + 64];
            sprintf(npath, "%s-%u", path, t);

            if (link(path, npath) == 0)
                unlink(path);
            else {
                delete [] dpath;
                delete [] path;
                delete [] npath;
                be_errmsg = "backup existing data failed";
                return (false);
            }
            delete [] npath;
        }
    }
    else {
        if (mkdir(dpath, 0755) < 0) {
            delete [] dpath;
            delete [] path;
            be_errmsg = "directory creation failed";
            return (false);
        }
    }
    delete [] dpath;

    FILE *fp = fopen(path, "w");
    if (!fp) {
        delete [] path;
        be_errmsg = "can't open data file";
        return (false);
    }
    fprintf(fp, "hostname=%s\n", be_hostname);
    // mtype must go before keytext
    fprintf(fp, "mtype=%s\n", osname());
    fprintf(fp, "keytext=%s\n", be_keytext);
    fprintf(fp, "email=%s\n", be_email);
    if (fprintf(fp, "info=%s\n", be_info) < 0) {
        be_errmsg = "write to data file failed";
        fclose(fp);
        delete [] path;
        return (false);
    }
    fclose(fp);
    chmod(path, 0640);
    delete [] path;
    return (true);
}


// Set the hostname field from the data file.
//
bool
sBackEnd::recall_hostname()
{
    char *df = key_datafile();
    FILE *fp = fopen(df, "r");
    delete [] df;
    if (!fp) {
        be_errmsg = "can't open key data file";
        return (false);
    }
    char buf[256];

    for (;;) {
        char *s = fgets(buf, 256, fp);
        if (!s)
            break;
        char *t = strchr(s, '=');
        if (!t)
            break;
        *t++ = 0;

        char *k = gettok(s);
        if (!k)
            break;

        if (!strcmp("hostname", k)) {
            delete [] k;
            fclose(fp);
            return (set_hostname(t));
        }
        delete [] k;
    }
    fclose(fp);
    be_errmsg = "parse error";
    return (false);
}


// Set the mtype field from the data file.
//
bool
sBackEnd::recall_mtype()
{
    char *df = key_datafile();
    FILE *fp = fopen(df, "r");
    delete [] df;
    if (!fp) {
        be_errmsg = "can't open key data file";
        return (false);
    }
    char buf[256];

    for (;;) {
        char *s = fgets(buf, 256, fp);
        if (!s)
            break;
        char *t = strchr(s, '=');
        if (!t)
            break;
        *t++ = 0;

        char *k = gettok(s);
        if (!k)
            break;

        if (!strcmp("mtype", k)) {
            delete [] k;
            char *v = gettok(t);
            if (!v) {
                be_errmsg = "no data for item";
                fclose(fp);
                return (false);
            }
            if (!strcmp("msw", v)) {
                delete [] v;
                be_ostype = OS_MSW;
                fclose(fp);
                return (true);
            }
            if (!strcmp("linux", v)) {
                delete [] v;
                be_ostype = OS_LINUX;
                fclose(fp);
                return (true);
            }
            if (!strcmp("osx", v)) {
                delete [] v;
                be_ostype = OS_OSX;
                fclose(fp);
                return (true);
            }
            delete [] v;
            be_errmsg = "bad data for item";
            fclose(fp);
            return (false);
        }
        delete [] k;
    }
    fclose(fp);
    be_errmsg = "parse error";
    return (false);
}


// Set the keytext field from the data file.
//
bool
sBackEnd::recall_keytext()
{
    char *df = key_datafile();
    FILE *fp = fopen(df, "r");
    delete [] df;
    if (!fp) {
        be_errmsg = "can't open key data file";
        return (false);
    }
    char buf[256];

    for (;;) {
        char *s = fgets(buf, 256, fp);
        if (!s)
            break;
        char *t = strchr(s, '=');
        if (!t)
            break;
        *t++ = 0;

        char *k = gettok(s);
        if (!k)
            break;

        if (!strcmp("keytext", k)) {
            delete [] k;
            fclose(fp);
            return (set_keytext(t));
        }
        delete [] k;
    }
    fclose(fp);
    be_errmsg = "parse error";
    return (false);
}


// Set the email field from the data file.
//
bool
sBackEnd::recall_email()
{
    char *df = key_datafile();
    FILE *fp = fopen(df, "r");
    delete [] df;
    if (!fp) {
        be_errmsg = "can't open key data file";
        return (false);
    }
    char buf[256];

    for (;;) {
        char *s = fgets(buf, 256, fp);
        if (!s)
            break;
        char *t = strchr(s, '=');
        if (!t)
            break;
        *t++ = 0;

        char *k = gettok(s);
        if (!k)
            break;

        if (!strcmp("email", k)) {
            delete [] k;
            fclose(fp);
            return (set_email(t));
        }
        delete [] k;
    }
    fclose(fp);
    be_errmsg = "parse error";
    return (false);
}


// Set the info field from the data file.
//
bool
sBackEnd::recall_info()
{
    char *df = key_datafile();
    FILE *fp = fopen(df, "r");
    delete [] df;
    if (!fp) {
        be_errmsg = "can't open key data file";
        return (false);
    }
    char buf[4096];

    for (;;) {
        char *s = fgets(buf, 4096, fp);
        if (!s)
            break;
        char *t = strchr(s, '=');
        if (!t)
            break;
        *t++ = 0;

        char *k = gettok(s);
        if (!k)
            break;

        if (!strcmp("info", k)) {
            delete [] k;

            while (s) {
                char *e = t + strlen(t);
                s = fgets(e, 4096, fp);
            }
            fclose(fp);
            return (set_info(t));
        }
        delete [] k;
    }
    fclose(fp);
    be_errmsg = "parse error";
    return (false);
}


// Return the full pathname to the key data file.
//
char *
sBackEnd::key_datafile()
{
    char *path = new char[strlen(KEYDIR) + strlen(be_key) +
        strlen(KEYDATA) + 3];
    sprintf(path, "%s/%s/%s", KEYDIR, be_key, KEYDATA);
    return (path);
}


// Static function.
// Return true if a directory named in key exists.
//
bool
sBackEnd::key_exists(const char *key)
{
    char *path = new char[strlen(KEYDIR) + strlen(key) + 2];
    sprintf(path, "%s/%s", KEYDIR, key);
    struct stat st;
    int i = stat(path, &st);
    if (i == 0)
        return (S_ISDIR(st.st_mode));
    return (false);
}


bool
sBackEnd::set_demousers(const char *str)
{
    if (!str || !*str) {
        be_demousers = -1;
        return (true);
    }
    int d;
    if (sscanf(str, "%d", &d) == 1 && d >= 0) {
        be_demousers = (d != 0);
        return (true);
    }
}


// Set the xivusers field.
//
bool
sBackEnd::set_xivusers(const char *str)
{
    if (!str || !*str) {
        be_xivusers = -1;
        return (true);
    }
    int d;
    if (sscanf(str, "%d", &d) == 1 && d >= 0) {
        be_xivusers = d;
        return (true);
    }
    be_errmsg = "bad XIVUSERS value, must be non-negative integer";
    return (false);
}


// Set the xiciiusers field.
//
bool
sBackEnd::set_xiciiusers(const char *str)
{
    if (!str || !*str) {
        be_xiciiusers = -1;
        return (true);
    }
    int d;
    if (sscanf(str, "%d", &d) == 1 && d >= 0) {
        be_xiciiusers = d;
        return (true);
    }
    be_errmsg = "bad XICIIUSERS value, must be non-negative integer";
    return (false);
}


// Set the xicusers field.
//
bool
sBackEnd::set_xicusers(const char *str)
{
    if (!str || !*str) {
        be_xicusers = -1;
        return (true);
    }
    int d;
    if (sscanf(str, "%d", &d) == 1 && d >= 0) {
        be_xicusers = d;
        return (true);
    }
    be_errmsg = "bad XICUSERS value, must be non-negative integer";
    return (false);
}


// Set the wrsusers field.
//
bool
sBackEnd::set_wrsusers(const char *str)
{
    if (!str || !*str) {
        be_wrsusers = -1;
        return (true);
    }
    int d;
    if (sscanf(str, "%d", &d) == 1 && d >= 0) {
        be_wrsusers = d;
        return (true);
    }
    be_errmsg = "bad WRSUSERS value, must be non-negative integer";
    return (false);
}


// Set the xtusers field.
//
bool
sBackEnd::set_xtusers(const char *str)
{
    if (!str || !*str) {
        be_xtusers = -1;
        return (true);
    }
    int d;
    if (sscanf(str, "%d", &d) == 1 && d >= 0) {
        be_xtusers = d;
        return (true);
    }
    be_errmsg = "bad XTUSERS value, must be non-negative integer";
    return (false);
}


// Set the months field.
//
bool
sBackEnd::set_months(const char *str)
{
    int d;
    if (str && sscanf(str, "%d", &d) == 1 && d > 0) {
        be_months = d;
        return (true);
    }
    be_errmsg = "bad MONTHS value, must be positive integer";
    return (false);
}


// If the user has requested a demo, check if the key allows this.
//
bool
sBackEnd::check_order()
{
    if (!be_key) {
        be_errmsg = "check_order, no key given";
        return (false);
    }
    if (be_demousers >= 0) {
        char *path = new char[strlen(KEYDIR) + strlen(be_key) + 8];
        sprintf(path, "%s/%s/%s", KEYDIR, be_key, "demo");
        struct stat st;
        int i = stat(path, &st);
        delete [] path;
        if (i == 0) {
            be_errmsg = 
                "A demo license has already been provided for your key.\n"
                "Only one demo is allowed per key, contact Whiteley Research\n"
                "to request an additional demo license.\n";
            return (false);
        }
        return (true);
    }
    if (be_xtusers == 0) {
        // We can't handle both fixed and floating licenses for the same
        // program in the same license file.
        if (be_xicusers == 0) {
            be_errmsg = 
                "The XT and XIC fixed-host licenses are redundant.\n";
            return (false);
        }
        if (be_wrsusers == 0) {
            be_errmsg = 
                "The XT and WRS fixed-host licenses are redundant.\n";
            return (false);
        }
        if (be_xicusers > 0) {
            be_errmsg = 
                "The XT is fixed-host and XIC is floating, mix not allowed.\n";
            return (false);
        }
        if (be_wrsusers > 0) {
            be_errmsg = 
                "The XT is fixed-host and WRS is floating, mix not allowed.\n";
            return (false);
        }
        return (true);
    }
    if (be_xtusers > 0) {
        if (be_xicusers == 0) {
            be_errmsg = 
                "The XT is floating and XIC is fixed-host, mix not allowed.\n";
            return (false);
        }
        if (be_wrsusers == 0) {
            be_errmsg = 
                "The XT is floating and WRS is fixed-host, mix not allowed.\n";
            return (false);
        }
    }
    return (true);
}


// Record the order in a file placed in the key directory.  The file
// name has a time stamp for uniqueness.
//
bool
sBackEnd::record_order(int total)
{
    if (!be_key) {
        be_errmsg = "record_order, no key given";
        return (false);
    }

    char *dir = new char[strlen(KEYDIR) + strlen(be_key) + 2];
    sprintf(dir, "%s/%s", KEYDIR, be_key);
    char fbf[64];
    time_t t = time(0);
    sprintf(fbf, "order-%u", t);
    char *path = new char[strlen(dir) + strlen(fbf) + 2];
    sprintf(path, "%s/%s", dir, fbf);
    delete [] dir;

    FILE *fp = fopen(path, "w");
    if (!fp) {
        be_errmsg = "record_order, open failure";
        delete [] path;
        return (false);
    }
    if (xivusers() >= 0) {
        fprintf(fp, "XIV base=$%d months=%d users=%d sub=$%d\n", XIVPM,
            months(), xivusers(),
            XIVPM*months()*(xivusers() > 0 ? xivusers() : 1));
    }
    if (xiciiusers() >= 0) {
        fprintf(fp, "XICII base=$%d months=%d users=%d sub=$%d\n", XICIIPM,
            months(), xiciiusers(),
            XICIIPM*months()*(xiciiusers() > 0 ? xiciiusers() : 1));
    }
    if (xicusers() >= 0) {
        fprintf(fp, "XIC base=$%d months=%d users=%d sub=$%d\n", XICPM,
            months(), xicusers(),
            XICPM*months()*(xicusers() > 0 ? xicusers() : 1));
    }
    if (wrsusers() >= 0) {
        fprintf(fp, "WRS base=$%d months=%d users=%d sub=$%d\n", WRSPM,
            months(), wrsusers(),
            WRSPM*months()*(wrsusers() > 0 ? wrsusers() : 1));
    }
    if (xtusers() >= 0) {
        fprintf(fp, "XT base=$%d months=%d users=%d sub=$%d\n", XTPM,
            months(), xtusers(),
            XTPM*months()*(xtusers() > 0 ? xtusers() : 1));
    }
    fprintf(fp, "TOTAL $%d\n", total);

    if (!be_hostname) {
        if (!recall_hostname()) {
            delete [] path;
            return (false);
        }
    }
    if (be_ostype == OS_NONE) {
        if (!recall_mtype()) {
            delete [] path;
            return (false);
        }
    }
    if (!be_keytext) {
        if (!recall_keytext()) {
            delete [] path;
            return (false);
        }
    }

    fprintf(fp, "BEGIN\n");
    bool didblk = false;
    int xicusrs = 0; 
    if (xicusers() > 0)
        xicusrs += xicusers();
    if (xtusers() > 0)
        xicusrs += xtusers();
    if (xicusrs == 0) {
        if (xicusers() < 0 && xtusers() < 0)
            xicusrs = -1;
    }
    int wrsusrs = 0; 
    if (wrsusers() > 0)
        wrsusrs += wrsusers();
    if (xtusers() > 0)
        wrsusrs += xtusers();
    if (wrsusrs == 0) {
        if (wrsusers() < 0 && xtusers() < 0)
            wrsusrs = -1;
    }
    if (xivusers() >= 0) {
        fprintf(fp, "XIV\n");
        fprintf(fp, "%d\n", be_months);
        fprintf(fp, "%s\n", be_hostname);
        fprintf(fp, "%s\n", be_keytext);
        fprintf(fp, be_ostype == OS_OSX ? "y\n" : "n\n");
        didblk = true;
    }
    if (xiciiusers() >= 0) {
        if (didblk)
            fprintf(fp, "y\n");
        fprintf(fp, "XICII\n");
        fprintf(fp, "%d\n", be_months);
        fprintf(fp, "%s\n", be_hostname);
        fprintf(fp, "%s\n", be_keytext);
        fprintf(fp, be_ostype == OS_OSX ? "y\n" : "n\n");
        didblk = true;
    }
    if (xicusrs >= 0) {
        if (didblk)
            fprintf(fp, "y\n");
        fprintf(fp, "XIC\n");
        fprintf(fp, "%d\n", be_months);
        fprintf(fp, "%s\n", be_hostname);
        fprintf(fp, "%s\n", be_keytext);
        fprintf(fp, be_ostype == OS_OSX ? "y\n" : "n\n");
        didblk = true;
    }
    if (wrsusrs >= 0) {
        if (didblk)
            fprintf(fp, "y\n");
        fprintf(fp, "WRS\n");
        fprintf(fp, "%d\n", be_months);
        fprintf(fp, "%s\n", be_hostname);
        fprintf(fp, "%s\n", be_keytext);
        fprintf(fp, be_ostype == OS_OSX ? "y\n" : "n\n");
        didblk = true;
    }
    fprintf(fp, "n\n");
    if (xivusers() > 0)
        fprintf(fp, "%d\n", xivusers());
    if (xiciiusers() > 0)
        fprintf(fp, "%d\n", xiciiusers());
    if (xicusrs > 0)
        fprintf(fp, "%d\n", xicusrs);
    if (wrsusrs > 0)
        fprintf(fp, "%d\n", wrsusrs);

    fclose(fp);
    chmod(path, 0640);
    delete [] path;
    be_time = t;
    return (true);
}


// Send a copy of the order file to Whiteley Research.  This will actually
// place the order - an email containing the license file will be returned
// to the purchaser.
//
bool
sBackEnd::send_email_order_wr()
{
    if (!be_key) {
        be_errmsg = "send_email, no key";
        return (false);
    }
    if (!be_time) {
        be_errmsg = "send_email, no order number";
        return (false);
    }
    if (be_months <= 0) {
        be_errmsg = "send_email, no months";
        return (false);
    }
    if (!be_email) {
        if (!recall_email())
            return (false);
    }

    char *dir = new char[strlen(KEYDIR) + strlen(be_key) + 2];
    sprintf(dir, "%s/%s", KEYDIR, be_key);
    char fbf[64];
    sprintf(fbf, "order-%u", be_time);
    char *path = new char[strlen(dir) + strlen(fbf) + 2];
    sprintf(path, "%s/%s", dir, fbf);

    FILE *kp = fopen(path, "r");
    delete [] path;
    if (!kp) {
        be_errmsg = "send_email, failed to open order file";
        return (false);
    }

    const char *mcmd = "mail -s \"New Order\"";
    char *cmd = new char[strlen(mcmd) + strlen(MAILADDR) + 2];
    sprintf(cmd, "%s %s", mcmd, MAILADDR);
    FILE *fp = popen(cmd, "w");
    if (!fp) {
        be_errmsg = "send_email, failed to start email thread";
        return (false);
    }
    fprintf(fp,
        "New order from\nkey: %s\nemail: %s\nserial: %u\nmonths: %d\n\n",
        be_key, be_email, be_time, be_months);
    int c;
    while ((c = fgetc(kp)) != EOF)
        fputc(c, fp);
    fclose(kp);
    fputc('\n', fp);
    pclose(fp);
    return (true);
}


// Send a copy of the order to the purchaser as confirmation.
//
bool
sBackEnd::send_email_order_user()
{
    if (!be_key) {
        be_errmsg = "send_email, no key";
        return (false);
    }
    if (!be_time) {
        be_errmsg = "send_email, no order number";
        return (false);
    }
    if (!be_email) {
        if (!recall_email())
            return (false);
    }
    if (!be_info) {
        if (!recall_info())
            return (false);
    }

    char buf[256];

    char *dir = new char[strlen(KEYDIR) + strlen(be_key) + 2];
    sprintf(dir, "%s/%s", KEYDIR, be_key);
    sprintf(buf, "order-%u", be_time);
    char *path = new char[strlen(dir) + strlen(buf) + 2];
    sprintf(path, "%s/%s", dir, buf);

    FILE *kp = fopen(path, "r");
    delete [] path;
    if (!kp) {
        be_errmsg = "send_email, failed to open order file";
        return (false);
    }

    const char *mcmd = "mail -s \"Your Order\"";
    char *cmd = new char[strlen(mcmd) + strlen(be_email) + 2];
    sprintf(cmd, "%s %s", mcmd, be_email);
    FILE *fp = popen(cmd, "w");
    if (!fp) {
        be_errmsg = "send_email, failed to start email thread";
        return (false);
    }
    fprintf(fp, "Hello %s\n\n", be_info);
    fprintf(fp, "This confirms your order from Whiteley Research Inc., "
        "thank you.\n\n");
    fprintf(fp, "Your key: %s\n\n", be_key);

    // Strip off the license file generator input.
    while (fgets(buf, 256, kp) != 0) {
        if (!strncmp(buf, "BEGIN", 5))
            break;
        fputs(buf, fp);
    }
    fclose(kp);
    fputc('\n', fp);
    fputc('\n', fp);
    fprintf(fp, "Your license file will arrive by email within 24 hours.\n\n");
    fprintf(fp, "Automated message, don't reply!\n"
        "Contact %s if there is an error.\n", MAILADDR);

    pclose(fp);
    return (true);
}


// Record the issuance of a demo license.
//
bool
sBackEnd::record_demo()
{
    if (!be_key) {
        be_errmsg = "record_demo, no key given";
        return (false);
    }

    char *path = new char[strlen(KEYDIR) + strlen(be_key) + 8];
    sprintf(path, "%s/%s/%s", KEYDIR, be_key, "demo");

    FILE *fp = fopen(path, "w");
    if (!fp) {
        be_errmsg = "record_demo, open failure";
        delete [] path;
        return (false);
    }
    time_t t = time(0);
    fprintf(fp, "DATE=%u\n", t);
    fprintf(fp, "%s\n", be_demousers == 0 ? "FIXED" : "FLOAT");

    if (!be_hostname) {
        if (!recall_hostname()) {
            delete [] path;
            return (false);
        }
    }
    if (be_ostype == OS_NONE) {
        if (!recall_mtype()) {
            delete [] path;
            return (false);
        }
    }
    if (!be_keytext) {
        if (!recall_keytext()) {
            delete [] path;
            return (false);
        }
    }

    fprintf(fp, "BEGIN\n");
    fprintf(fp, "XIC\n");
    fprintf(fp, "%d\n", 1);
    fprintf(fp, "%s\n", be_hostname);
    fprintf(fp, "%s\n", be_keytext);
    fprintf(fp, be_ostype == OS_OSX ? "y\n" : "n\n");
    fprintf(fp, "y\n");
    fprintf(fp, "WRS\n");
    fprintf(fp, "%d\n", 1);
    fprintf(fp, "%s\n", be_hostname);
    fprintf(fp, "%s\n", be_keytext);
    fprintf(fp, be_ostype == OS_OSX ? "y\n" : "n\n");
    fprintf(fp, "n\n");
    fprintf(fp, "%d\n", be_demousers);
    fprintf(fp, "%d\n", be_demousers);

    fclose(fp);
    chmod(path, 0640);
    delete [] path;
    be_time = t;
    return (true);
}


bool
sBackEnd::send_email_demo_wr()
{
    if (!be_key) {
        be_errmsg = "send_email, no key";
        return (false);
    }
    if (!be_time) {
        be_errmsg = "send_email, no order number";
        return (false);
    }
    if (be_months <= 0) {
        be_errmsg = "send_email, no months";
        return (false);
    }
    if (!be_email) {
        if (!recall_email())
            return (false);
    }

    char *path = new char[strlen(KEYDIR) + strlen(be_key) + 8];
    sprintf(path, "%s/%s/%s", KEYDIR, be_key, "demo");

    FILE *kp = fopen(path, "r");
    delete [] path;
    if (!kp) {
        be_errmsg = "send_email, failed to open demo file";
        return (false);
    }

    const char *mcmd = "mail -s \"New Demo\"";
    char *cmd = new char[strlen(mcmd) + strlen(MAILADDR) + 2];
    sprintf(cmd, "%s %s", mcmd, MAILADDR);
    FILE *fp = popen(cmd, "w");
    if (!fp) {
        be_errmsg = "send_email, failed to start email thread";
        return (false);
    }
    fprintf(fp,
        "New demo request from\nkey: %s\nemail: %s\nserial: %u\nmonths: %d\n\n",
        be_key, be_email, be_time, be_months);
    int c;
    while ((c = fgetc(kp)) != EOF)
        fputc(c, fp);
    fclose(kp);
    fputc('\n', fp);
    pclose(fp);
    return (true);
}


bool
sBackEnd::send_email_demo_user()
{
    if (!be_key) {
        be_errmsg = "send_email, no key";
        return (false);
    }
    if (!be_email) {
        if (!recall_email())
            return (false);
    }
    if (!be_info) {
        if (!recall_info())
            return (false);
    }

    const char *mcmd = "mail -s \"Your demo request\"";
    char *cmd = new char[strlen(mcmd) + strlen(be_email) + 2];
    sprintf(cmd, "%s %s", mcmd, be_email);
    FILE *fp = popen(cmd, "w");
    if (!fp) {
        be_errmsg = "send_email, failed to start email thread";
        return (false);
    }
    fprintf(fp, "Hello %s\n\n", be_info);
    fprintf(fp, "This confirms your demo request from Whiteley Research Inc., "
        "thank you.\n\n");
    fprintf(fp, "Your key: %s\n\n", be_key);
    fprintf(fp, "Requested license type: %s\n\n",
        be_demousers == 0 ? "FIXED" : "FLOAT");
    fprintf(fp, "Your license file will arrive by email within 24 hours.\n\n");
    fprintf(fp, "Automated message, don't reply!\n"
        "Contact %s if there is an error.\n", MAILADDR);

    pclose(fp);
    return (true);
}


// Record the access order in a file placed in the key directory.  The
// file name has a time stamp for uniqueness.  The key is the email
// address.
//
bool
sBackEnd::record_acc_order(int total)
{
    if (!be_key)
        be_key = strdup(be_email);

    char *dir = new char[strlen(KEYDIR) + strlen(be_key) + 2];
    sprintf(dir, "%s/%s", KEYDIR, be_key);
    if (!key_exists(be_key)) {
        if (mkdir(dir, 0755) < 0) {
            delete [] dir;
            be_errmsg = "record_a_cc_order, directory creation failed";
            return (false);
        }
    }

    char fbf[64];
    time_t t = time(0);
    sprintf(fbf, "access-%u", t);
    char *path = new char[strlen(dir) + strlen(fbf) + 2];
    sprintf(path, "%s/%s", dir, fbf);
    delete [] dir;

    FILE *fp = fopen(path, "w");
    if (!fp) {
        be_errmsg = "record_acc_order, open failure";
        delete [] path;
        return (false);
    }
    if (xivusers() > 0) {
        fprintf(fp, "XIV base=$%d years=%d users=%d sub=$%d\n", 2*XIVPM,
            months(), xivusers(), 2*XIVPM*months()*xivusers());
    }
    if (xiciiusers() > 0) {
        fprintf(fp, "XICII base=$%d years=%d users=%d sub=$%d\n", 2*XICIIPM,
            months(), xiciiusers(), 2*XICIIPM*months()*xiciiusers());
    }
    if (xicusers() > 0) {
        fprintf(fp, "XIC base=$%d years=%d users=%d sub=$%d\n", 2*XICPM,
            months(), xicusers(), 2*XICPM*months()*xicusers());
    }
    if (wrsusers() > 0) {
        fprintf(fp, "WRS base=$%d years=%d users=%d sub=$%d\n", 2*WRSPM,
            months(), wrsusers(), 2*WRSPM*months()*wrsusers());
    }
    if (xtusers() > 0) {
        fprintf(fp, "XT base=$%d years=%d users=%d sub=$%d\n", 2*XTPM,
            months(), xtusers(), 2*XTPM*months()*xtusers());
    }
    fprintf(fp, "TOTAL $%d\n\n", total);

    fprintf(fp, "info: %s\n", be_info ? be_info : "");

    fclose(fp);
    chmod(path, 0640);
    delete [] path;
    be_time = t;
    return (true);
}


// Send a copy of the access order file to Whiteley Research.
//
bool
sBackEnd::send_email_acc_order_wr()
{
    if (!be_email) {
        be_errmsg = "send_email, no email address";
        return (false);
    }
    if (!be_time) {
        be_errmsg = "send_email, no order number";
        return (false);
    }
    if (be_months <= 0) {
        be_errmsg = "send_email, no months";
        return (false);
    }

    char *dir = new char[strlen(KEYDIR) + strlen(be_key) + 2];
    sprintf(dir, "%s/%s", KEYDIR, be_key);
    char fbf[64];
    sprintf(fbf, "access-%u", be_time);
    char *path = new char[strlen(dir) + strlen(fbf) + 2];
    sprintf(path, "%s/%s", dir, fbf);

    FILE *kp = fopen(path, "r");
    delete [] path;
    if (!kp) {
        be_errmsg = "send_email, failed to open order file";
        return (false);
    }

    const char *mcmd = "mail -s \"New Access Order\"";
    char *cmd = new char[strlen(mcmd) + strlen(MAILADDR) + 2];
    sprintf(cmd, "%s %s", mcmd, MAILADDR);
    FILE *fp = popen(cmd, "w");
    if (!fp) {
        be_errmsg = "send_email, failed to start email thread";
        return (false);
    }
    fprintf(fp, "New access order from\nemail: %s\nserial: %d\nyears: %d\n\n",
        be_email, be_time, be_months);
    int c;
    while ((c = fgetc(kp)) != EOF)
        fputc(c, fp);
    fclose(kp);
    fputc('\n', fp);
    pclose(fp);
    return (true);
}


// Send a copy of the access order to the purchaser as confirmation.
//
bool
sBackEnd::send_email_acc_order_user()
{
    if (!be_time) {
        be_errmsg = "send_email, no order number";
        return (false);
    }

    char buf[256];

    char *dir = new char[strlen(KEYDIR) + strlen(be_key) + 2];
    sprintf(dir, "%s/%s", KEYDIR, be_key);
    sprintf(buf, "access-%u", be_time);
    char *path = new char[strlen(dir) + strlen(buf) + 2];
    sprintf(path, "%s/%s", dir, buf);

    FILE *kp = fopen(path, "r");
    delete [] path;
    if (!kp) {
        be_errmsg = "send_email, failed to open order file";
        return (false);
    }

    const char *mcmd = "mail -s \"Your Order\"";
    char *cmd = new char[strlen(mcmd) + strlen(be_email) + 2];
    sprintf(cmd, "%s %s", mcmd, be_email);
    FILE *fp = popen(cmd, "w");
    if (!fp) {
        be_errmsg = "send_email, failed to start email thread";
        return (false);
    }
    fprintf(fp, "Hello %s\n\n", be_info);
    fprintf(fp, "This confirms your order from Whiteley Research Inc., "
        "thank you.\n\n");
    fprintf(fp, "Your key: %s\n\n", be_key);

    while (fgets(buf, 256, kp) != 0) {
        fputs(buf, fp);
        if (!strncmp(buf, "TOTAL", 5))
            break;
    }
    fclose(kp);
    fputc('\n', fp);
    fputc('\n', fp);
    fprintf(fp, "Your password will arrive by email within 24 hours.\n\n");
    fprintf(fp, "Automated message, don't reply!\n"
        "Contact %s if there is an error.\n", MAILADDR);

    pclose(fp);
    return (true);
}

