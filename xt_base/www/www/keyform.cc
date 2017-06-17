
/*========================================================================*
 *                                                                        *
 *  Copyright (c) 2016 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
 *  Author:  Stephen R. Whiteley (stevew@wrcad.com)
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Key registration form handling
 *                                                                        *
 *========================================================================*
 $Id: keyform.cc,v 1.1 2016/01/15 19:45:21 stevew Exp $
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

//
// This handles the "Get A Key" form.
//

namespace {
    const char *sedcmdfmt = "sed \'s/@KEY@/%s/;s/@HOSTNAME@/%s/;"
        "s/@MSW@/%s/;s/@LINUX@/%s/;s/@OSX@/%s/;"
        "s/@KEYTEXT@/%s/;s/@EMAIL@/%s/;s%c@INFO@%c%s%c\' < "
        WWWROOT"/www/prices.in";
}


int main(int argc, char **argv)
{
    if (argc < 2)
        return (1);

    sBackEnd::keyval *kval;
    int nkv;
    if (!sBackEnd::get_keyvals(&kval, &nkv, argv[1])) {
        sBackEnd::errordump("Parse failed!");
        return (1);
    }

    // The keys:
    // From "Submit"
    //  hostname
    //  keytext
    //  mtype
    //  email
    //  text
    // From "Find Existing"
    //  key

    OS_type ostype = OS_NONE;
    for (int i = 0; i < nkv; i++) {
        if (!strcmp(kval[i].key, "mtype")) {
            const char *mtype = kval[i].val;
            if (!strcmp(mtype, "msw"))
                ostype = OS_MSW;
            else if (!strcmp(mtype, "linux"))
                ostype = OS_LINUX;
            else if (!strcmp(mtype, "osx"))
                ostype = OS_OSX;
            break;
        }
        if (!strcmp(kval[i].key, "key")) {
            sBackEnd be(ostype);
            if (!be.set_key(kval[i].val)) {
                sBackEnd::errordump(be.error_msg());
                return (1);
            }
            if (!be.recall_hostname()) {
                sBackEnd::errordump(be.error_msg());
                return (1);
            }
            if (!be.recall_mtype()) {
                sBackEnd::errordump(be.error_msg());
                return (1);
            }
            if (!be.recall_keytext()) {
                sBackEnd::errordump(be.error_msg());
                return (1);
            }
            if (!be.recall_email()) {
                sBackEnd::errordump(be.error_msg());
                return (1);
            }
            if (!be.recall_info()) {
                sBackEnd::errordump(be.error_msg());
                return (1);
            }

            // We need an info string where the newlines are backslash
            // quoted.
            int len = 0;
            const char *p = be.info();
            while (*p) {
               if (*p == '\n')
                   len++;
               len++;
               p++;
            }
            char *info = new char[len+1];
            char *t = info;
            p = be.info();
            while (*p) {
               if (*p == '\n')
                   *t++ = '\\';
               *t++ = *p++;
            }
            *t = 0;

            // Find a delimiter for sed that is not used in the info.
            char sp = 0;
            if (strchr(info, '/') == 0)
                sp = '/';
            else if (strchr(info, '%') == 0)
                sp = '%';
            else if (strchr(info, '&') == 0)
                sp = '&';
            else if (strchr(info, '~') == 0)
                sp = '~';
            else if (strchr(info, '+') == 0)
                sp = '+';
            else {
                for (t = info; *t; t++) {
                    if (*t == '+')
                        *t = ' ';
                }
                sp = '+';
            }

            char *sedcmd = new char[strlen(sedcmdfmt) + strlen(info) +
                strlen(be.get_key()) + strlen(be.hostname()) +
                strlen(be.keytext()) + strlen(be.email()) + 20];

            const char *msw = "";
            const char *lnx = "";
            const char *osx = "";
            if (*be.osname() == 'm')
                msw = "selected";
            else if (*be.osname() == 'l')
                lnx = "selected";
            else if (*be.osname() == 'o')
                osx = "selected";

            sprintf(sedcmd, sedcmdfmt, be.get_key(), be.hostname(),
                msw, lnx, osx, be.keytext(), be.email(), sp, sp, info, sp);
            delete [] info;
            system(sedcmd);
            delete [] sedcmd;
            return (0);
        }
    }
    if (ostype == OS_NONE) {
        sBackEnd::errordump("No Operating System selected!");
        return (1);
    }

    sBackEnd be(ostype);

    for (int i = 0; i < nkv; i++) {
        if (!strcmp(kval[i].key, "hostname")) {
            if (!be.set_hostname(kval[i].val)) {
                sBackEnd::errordump(be.error_msg());
                return (1);
            }
        }
        else if (!strcmp(kval[i].key, "keytext")) {
            if (!be.set_keytext(kval[i].val)) {
                sBackEnd::errordump(be.error_msg());
                return (1);
            }
        }
        else if (!strcmp(kval[i].key, "email")) {
            if (!be.set_email(kval[i].val)) {
                sBackEnd::errordump(be.error_msg());
                return (1);
            }
        }
        else if (!strcmp(kval[i].key, "text")) {
            if (!be.set_info(kval[i].val)) {
                sBackEnd::errordump(be.error_msg());
                return (1);
            }
        }
    }

    // Create the user's key.

    const char *key = be.get_key();
    if (!key) {
        sBackEnd::errordump(be.error_msg());
        return (1);
    }

    // Save the key data in a file.

    if (!be.save_key_data()) {
        sBackEnd::errordump(be.error_msg());
        return (1);
    }

    // Email the key data to the user.

    char *df = be.key_datafile();
    FILE *kp = fopen(df, "r");
    if (!kp) {
        sBackEnd::errordump("failed to open data file");
        return (1);
    }
    delete [] df;

    const char *mcmd = "mail -s \"Whiteley Research license key\"";
    char *cmd = new char[strlen(mcmd) + strlen(be.email()) + 2];
    sprintf(cmd, "%s %s", mcmd, be.email());
    FILE *fp = popen(cmd, "w");
    if (!fp) {
        sBackEnd::errordump("failed to start email thread");
        return (1);
    }
    fprintf(fp, "Hello from wrcad.com, your license key is\n%s\n\n",
        be.get_key());
    fprintf(fp, "Keep this in a safe place, you will need this to renew\n"
        "your license.  Contact Whiteley Research if any questions.\n\n");
    fprintf(fp, "Automated message, don't reply!\n\n");
    fprintf(fp, "The data for this key follows, please verify correctness\n"
        "You can re-create the key if necessary to update info.\n\n");
    int c;
    while ((c = fgetc(kp)) != EOF)
        fputc(c, fp);
    fclose(kp);
    fputc('\n', fp);
    pclose(fp);

    // Compose the response page.

    printf("<body background=/images/tmbg.gif text=#000000 link=#9c009e" 
        " vlink=#551a8b alink=#ff0000>\n");
    printf("<br><br><br><br><br><br><br><br><br><br>\n");
    printf("<center><table border=1 cellpadding=12><tr><td bgcolor=white>\n");
    printf("<center>Your license key<br><br> <font size=5><tt>%s</tt></font><br></center>\n",
        be.get_key());
    printf("<br><br>Key data has been emailed to: &nbsp;&nbsp;<tt>%s</tt><br>\n",
        be.email());
    printf("<p><a href=/cgi-bin/prices.cgi?key=%s#getakey><b>Click here</b></a>"
        " to continue.\n", be.get_key());
    printf("</td></tr></table></center>\n");

    return (0);
}

