
/*========================================================================*
 *                                                                        *
 *  XicTools Integrated Circuit Design System                             *
 *  Copyright (c) 2016 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * License File Creation                                                  *
 *                                                                        *
 *========================================================================*
 $Id: mkacc.cc,v 1.1 2016/01/18 18:52:34 stevew Exp $
 *========================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
#include "sitedefs.h"

// The stdin is a "New Access" order email.  This will obtain a
// repository password for the email address, and email this to the
// user.


namespace {
    char *gettok(char **s)
    {
        if (s == 0 || *s == 0)
            return (0);
        while (isspace(**s))
            (*s)++;
        if (!**s)
            return (0);
        char *st = *s;
        while (**s && !isspace(**s))
            (*s)++;
        char *cbuf = new char[*s - st + 1];
        char *c = cbuf;
        while (st < *s)
            *c++ = *st++;
        *c = 0;
        while (isspace(**s))
            (*s)++;
        return (cbuf);
    }
}


// Get the access password for key from wrcad.com.
//
char *get_password(const char *key, int months)
{
    char *cmd = new char[strlen(PWCMD) + strlen(key) + 20];
    sprintf(cmd, "%s %s %d", PWCMD, key, months);
    FILE *fp = popen(cmd, "r");
    delete [] cmd;
    if (!fp) {
        fprintf(stderr, "get_password, popen failed.\n");
        return (0);
    }
    char *password = 0;
    char *s, buf[256];
    while ((s = fgets(buf, 256, fp)) != 0) {
        char *u = gettok(&s);
        if (!u)
            continue;
        if (!strcmp(u, "user:")) {
            delete [] u;
            u = gettok(&s);
            if (!u || strcmp(u, key)) {
                delete [] u;
                fprintf(stderr, "get_password, bad return.\n");
                pclose(fp);
                return (0);
            }
            delete [] u;
            delete [] gettok(&s);
            password = gettok(&s);
            break;
        }
        delete [] u;
    }
    pclose(fp);
    if (!password)
        fprintf(stderr, "get_password, failed to obtain password.\n");
    return (password);
}


// Return a static string giving the expiration date (not including
// the grace period).  This assumes that the LICENSE file was just
// created, i.e., we measure time from "now".
//
const char *exptime(int months)
{
    time_t loc = time(0);
    tm *t = localtime(&loc);
    months += t->tm_mon;
    int years = months/12;
    months %= 12;
    t->tm_year += years;
    t->tm_mon = months;
    time_t texp = mktime(t);
    // Add a 3-day grace period.
    // text += 3*24*3600;

    tm *tp = localtime(&texp);
    return (asctime(tp));
}


// Email the license file to the purchaser.
//
bool send_password(const char *email, const char *serial, int months)
{
    char *password = get_password(email, months);
    if (!password)
        return (false);

    char *keydir = new char[strlen(KEYDIR) + strlen(email) + 2];
    sprintf(keydir, "%s/%s", KEYDIR, email);
    const char *mcmd = "cd %s; mail -s \"Repository Access\" -r \""
        LMAILADDR"\" -c "MAILADDR" %s";
    char *cmd = new char[strlen(mcmd) + strlen(keydir) + strlen(email) + 20];
    sprintf(cmd, mcmd, keydir, email);
    FILE *fp = popen(cmd, "w");
    if (!fp) {
        delete [] keydir;
        fprintf(stderr, "send_license, failed to start email thread.\n");
        return (false);
    }

    fprintf(fp, "Key:      %s\n", email);
    fprintf(fp, "Password: %s\n", password);
    fprintf(fp, "Expires:  %s\n", exptime(months));
    fprintf(fp, "Serial number: %s\n\n", serial);

    fprintf(fp, "Save this info in a safe place!\n");
    fprintf(fp,
        "You can reply to this message if there is an error or you have "
        "questions.\n");

    pclose(fp);
    return (true);
}


int main(int, char**)
{
    // Copy the stdin to a temporary file, while grabbing some data.

    char buf[4096];

    char *email = 0;
    char *serial = 0;
    int years = 0;
    bool check = false;
    char *s;
    while ((s = fgets(buf, 4096, stdin)) != 0) {
        char *tok = gettok(&s);
        if (tok) {
            if (!strcmp(tok, "email:")) {
                email = gettok(&s);
                delete [] tok;
                continue;
            }
            if (!strcmp(tok, "serial:")) {
                serial = gettok(&s);
                delete [] tok;
                continue;
            }
            if (!strcmp(tok, "years:")) {
                delete [] tok;
                tok = gettok(&s);
                if (tok) {
                    years = atoi(tok);
                    delete [] tok;
                }
                continue;
            }
            if (!strcmp(tok, "New")) {
                delete [] tok;
                tok = gettok(&s);
                if (tok && !strcmp(tok, "access"))
                    check = true;
            }
            delete [] tok;
        }
    }
    if (!check) {
        fprintf(stderr, "mkacc: unknown order type.\n");
        return (1);
    }
    if (!email) {
        fprintf(stderr, "mkacc: email addr not found.\n");
        return (1);
    }
    if (!serial) {
        fprintf(stderr, "mkacc: serial number not found.\n");
        return (1);
    }
    if (years <= 0) {
        fprintf(stderr, "mkacc: years not found.\n");
        return (1);
    }
    printf("Extracted keyword data.\n");

    if (!send_password(email, serial, years*12))
        return (1);

    printf("Email sent to user.\nDone.\n");
    return (0);
}
