
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
 $Id: mklic.cc,v 1.3 2016/01/18 18:52:12 stevew Exp $
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


// The stdin is a "New Order" or "New Demo" email.  This will
// 1.  Create a corresponding LICENSE file
// 2.  Save the license file, and the email, in the key directory.
// 3.  Create and email the notification email to the purchaser,
//     attaching the LICENSE file.


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


// Save the order info in tfn into a permanent file.
//
bool save_order(const char *key, const char *tfn, const char *serial)
{
    char *fn = new char[strlen(KEYDIR) + strlen(key) + strlen(serial) + 10];
    sprintf(fn, "%s/%s/order-%s", KEYDIR, key, serial);
    FILE *fp = fopen(tfn, "r");
    if (!fp) {
        fprintf(stderr, "save_order, open of temp file failed.\n");
        return (false);
    }
    FILE *op = fopen(fn, "w");
    if (!op) {
        fclose(fp);
        fprintf(stderr, "save_order, open of order file failed.\n");
        return (false);
    }
    char c;
    while ((c = getc(fp)) != EOF) {
        if (putc(c, op) < 0) {
            fclose(fp);
            fclose(op);
            fprintf(stderr, "save_order: write of order file failed.\n");
            return (false);
        }
    }
    fclose(fp);
    fclose(op);
    return (true);
}


// Copy the license generation script into the temp file.
//
bool read_script(const char *key, const char *tfn, const char *serial)
{
    char *ofile = new char[strlen(KEYDIR) + strlen(key) + strlen(serial) + 20];
    sprintf(ofile, "%s/%s/order-%s", KEYDIR, key, serial);
    FILE *fp = fopen(ofile, "r");
    if (!fp) {
        delete [] ofile;
        fprintf(stderr, "read_script, open of order file failed.\n");
        return (false);
    }
    delete [] ofile;

    FILE *op = fopen(tfn, "w");
    if (!op) {
        delete [] ofile;
        fclose(fp);
        fprintf(stderr, "read_script, open of temp file failed.\n");
        return (false);
    }

    bool bflg = false;
    char *buf = new char[4096];
    char *s;
    while ((s = fgets(buf, 4096, fp)) != 0) {
        if (bflg) {
            if (fputs(s, op) < 0) {
                fclose(fp);
                fclose(op);
                fprintf(stderr, "read_script: write to temp file failed.\n");
                return (false);
            }
            continue;
        }
        char *tok = gettok(&s);
        if (tok && !strcmp(tok, "BEGIN"))
            bflg = true;
        delete [] tok;
    }
    fclose(fp);
    fclose(op);
    return (true);
}


// Create the LICENSE file.
//
bool create_license(const char *key, const char *infile, const char *serial)
{
    // Delete any existing LICENSE file.

    char *keydir = new char[strlen(KEYDIR) + strlen(key) + 2];
    sprintf(keydir, "%s/%s", KEYDIR, key);
    char *lfile = new char[strlen(keydir) + 20];
    sprintf(lfile, "%s/LICENSE", keydir);
    unlink(lfile);

    // Create a new LICENSE file, also copied to LICENSE-serial.

    const char *cmdfmt =
        "cd %s; %s < %s > /dev/null; cp LICENSE LICENSE-%s;";
    char *cmd = new char[strlen(cmdfmt) + strlen(VALIDATE) + strlen(keydir) +
        strlen(infile) + strlen(serial) + 20];
    sprintf(cmd, cmdfmt, keydir, VALIDATE, infile, serial);
    system(cmd);
    delete [] cmd;

    if (access(lfile, F_OK) < 0) {
        delete [] keydir;
        delete [] lfile;
        fprintf(stderr, "create_license, failed to create license file.\n");
        return (false);
    }

    // Move the validate.log file to validate.log-serial.

    char *logfile = new char[strlen(keydir) + 20];
    sprintf(logfile, "%s/%s", keydir, "validate.log");
    char *nlog = new char[strlen(logfile) + strlen(serial) + 2];
    sprintf(nlog, "%s-%s", logfile, serial);
    link(logfile, nlog);
    unlink(logfile);

    delete [] nlog;
    delete [] logfile;
    delete [] lfile;
    delete [] keydir;

    return (true);
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
bool send_license(const char *key, const char *email, const char *serial,
    int months, bool demo)
{
    char *password = get_password(key, months);
    if (!password)
        return (false);

    char *keydir = new char[strlen(KEYDIR) + strlen(key) + 2];
    sprintf(keydir, "%s/%s", KEYDIR, key);
    const char *mcmd = "cd %s; mail -s \"License File\" -r \""
        LMAILADDR"\" -c "MAILADDR" -a LICENSE %s";
    char *cmd = new char[strlen(mcmd) + strlen(keydir) + strlen(email) + 20];
    sprintf(cmd, mcmd, keydir, email);
    FILE *fp = popen(cmd, "w");
    if (!fp) {
        delete [] keydir;
        fprintf(stderr, "send_license, failed to start email thread.\n");
        return (false);
    }

    fprintf(fp, "Key:      %s\n", key);
    fprintf(fp, "Password: %s\n", password);
    fprintf(fp, "Expires:  %s\n", exptime(months));
    fprintf(fp, "Serial number: %s\n\n", serial);

    FILE *ip = fopen(demo ? DEMONOTE : LICNOTE, "r");
    if (!ip) {
        fprintf(stderr, "send_license: failed to open licnote file.\n");
        return (false);
    }
    int c;
    while ((c = fgetc(ip)) != EOF)
        fputc(c, fp);
    fclose(ip);
    fputc('\n', fp);
    fputc('\n', fp);
    fprintf(fp,
        "Reply to this message if there is an error or you have questions.\n");

    pclose(fp);
    return (true);
}


int main(int, char**)
{
    // Copy the stdin to a temporary file, while grabbing some data.

    char buf[4096];
    char tfn[20];
    strcpy(tfn, "/tmp/mklicXXXXXX");
    int fd = mkstemp(tfn);
    if (fd < 0) {
        fprintf(stderr, "mklic: mkstemp failed.\n");
        return (1);
    }

    char *key = 0;
    char *email = 0;
    char *serial = 0;
    int months = 0;
    bool demo = false;
    bool check = false;
    char *s;
    while ((s = fgets(buf, 4096, stdin)) != 0) {
        int len = strlen(s);
        if (write(fd, s, len) != len) {
            close(fd);
            unlink(tfn);
            fprintf(stderr, "mklic: write failed.\n");
            return (1);
        }
        char *tok = gettok(&s);
        if (tok) {
            if (!strcmp(tok, "key:")) {
                key = gettok(&s);
                delete [] tok;
                continue;
            }
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
            if (!strcmp(tok, "months:")) {
                delete [] tok;
                tok = gettok(&s);
                if (tok) {
                    months = atoi(tok);
                    delete [] tok;
                }
                continue;
            }
            if (!strcmp(tok, "New")) {
                delete [] tok;
                tok = gettok(&s);
                if (tok && !strcmp(tok, "demo")) {
                    demo = true;
                    check = true;
                }
                if (tok && !strcmp(tok, "order")) {
                    check = true;
                }
            }
            delete [] tok;
        }
    }
    close(fd);
    if (!check) {
        unlink(tfn);
        fprintf(stderr, "mklic: unknown order type.\n");
        return (1);
    }
    if (!key) {
        unlink(tfn);
        fprintf(stderr, "mklic: key not found.\n");
        return (1);
    }
    if (!email) {
        unlink(tfn);
        fprintf(stderr, "mklic: email addr not found.\n");
        return (1);
    }
    if (!serial) {
        unlink(tfn);
        fprintf(stderr, "mklic: serial number not found.\n");
        return (1);
    }
    if (months <= 0) {
        unlink(tfn);
        fprintf(stderr, "mklic: months not found.\n");
        return (1);
    }
    printf("Created temporary file, extracted keyword data.\n");

    // Create the key directory if necessary.

    sprintf(buf, "%s/%s", KEYDIR, key);
    if (access(buf, F_OK) < 0) {
        if (mkdir(buf, 0755) < 0) {
            unlink(tfn);
            fprintf(stderr, "mklic: failed to create key dir.\n");
            return (1);
        }
        printf("Created key directory %s.\n", buf);
    }

    // Save the order email.

    if (!save_order(key, tfn, serial)) {
        unlink(tfn);
        return (1);
    }
    printf("Wrote order file in key directory.\n");

    // Replace the temp file with the text for the license generation
    // program input.  This is the text that follows "BEGIN".

    if (!read_script(key, tfn, serial)) {
        unlink(tfn);
        return (1);
    }
    printf("Extracted license info to temporary file.\n");

    // Create the LICENSE file.

    if (!create_license(key, tfn, serial)) {
        unlink(tfn);
        return (1);
    }
    printf("Created license file, saved in key directory.\n");
    unlink(tfn);  // Done with this.

    // Email the license file.

    if (!send_license(key, email, serial, months, demo)) {
        return (1);
    }
    printf("Sent email.\nDone.\n\n");

    // Done with this.
    sprintf(buf, "%s/%s/LICENSE", KEYDIR, key);
    unlink(buf);

    return (0);
}

