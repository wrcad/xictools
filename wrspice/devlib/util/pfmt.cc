
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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
 * WRspice Circuit Simulation and Analysis Tool:  Device Library          *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

// PFMT IFparm formatter.
// This will restructure the hideous IFparm initializers into something
// the looks reasonable.
//
// Usage:  pfmt < device.cc > output.cc
//
// Lines not part of the IFparms should be copied without change.


#include <stdio.h>
#include <ctype.h>
#include <string.h>

int main(int, char**)
{
    char buf[1024], *s;

    // iotok ( ntok , etok , ttok, [\n] dtok )[,]

    while ((s = fgets(buf, 1024, stdin)) != 0) {

        // Skip leading white space.
        while (isspace(*s))
            s++;
        if (!*s) {
            putc('\n', stdout);
            continue;
        }

        // Get the I/O directive token, and skip past the '('.
        char *t = s;
        while (isalpha(*s))
            s++;
        while (isspace(*s))
            s++;
        if (*s != '(' || s == t) {
            fputs(buf, stdout);
            continue;
        }
        *s = 0;
        char *iotok = new char[s-t + 1];
        strcpy(iotok, t);
        t = iotok + strlen(iotok) - 1;
        while (t >= iotok && isspace(*t))
            *t-- = 0;
        *s++ = '(';

        bool tokok = true;
        for (t = iotok; *t; t++) {
            if (!strchr("IOPUA", *t)) {
                tokok = false;
                break;
            }
        }
        if (!tokok) {
            delete [] iotok;
            fputs(buf, stdout);
            continue;
        }
        if (!strcmp(iotok, "IOP"))
            iotok[2] = 0;

        // Get the parameter name token.
        while (isspace(*s))
            s++;
        if (!*s) {
            printf("SYNTAX ERROR: %s", buf);
            delete [] iotok;
            continue;
        }
        t = s;
        while (*s && !isspace(*s) && *s != ',')
            s++;
        if (s == t) {
            printf("SYNTAX ERROR: %s", buf);
            delete [] iotok;
            continue;
        }
        char ctmp = *s;
        *s = 0;
        char *ntok = new char[s-t + 1];
        strcpy(ntok, t);
        *s++ = ctmp;

        // Get the parameter enumerator token.
        while (isspace(*s) || *s == ',')
            s++;
        if (!*s) {
            printf("SYNTAX ERROR: %s", buf);
            delete [] iotok;
            delete [] ntok;
            continue;
        }
        t = s;
        while (*s && !isspace(*s) && *s != ',')
            s++;
        if (s == t) {
            printf("SYNTAX ERROR: %s", buf);
            delete [] iotok;
            delete [] ntok;
            continue;
        }
        ctmp = *s;
        *s = 0;
        char *etok = new char[s-t + 1];
        strcpy(etok, t);
        *s++ = ctmp;

        // Get the datatype token.
        while (isspace(*s) || *s == ',')
            s++;
        if (!*s) {
            printf("SYNTAX ERROR: %s", buf);
            delete [] iotok;
            delete [] ntok;
            delete [] etok;
            continue;
        }
        t = s;
        while (*s && *s != ',')
            s++;
        if (s == t) {
            printf("SYNTAX ERROR: %s", buf);
            delete [] iotok;
            delete [] ntok;
            delete [] etok;
            continue;
        }
        ctmp = *s;
        *s = 0;
        char *ttok = new char[s-t + 1];
        strcpy(ttok, t);
        t = ttok + strlen(ttok) - 1;
        while (t >= ttok && isspace(*t))
            *t-- = 0;
        *s++ = ctmp;

        // Get the description token.
        while (isspace(*s) || *s == ',')
            s++;
        if (!*s) {
            // This might be on the following line.
            s = fgets(buf, 1024, stdin);
            if (!s) {
                printf("FILE ENDED PREMATURELY\n");
                return (0);
            }
            while (isspace(*s))
                s++;
        }
        t = s;
        bool inquote = false;
        while (*s) {
            if (*s == '"') {
                if (inquote && *(s-1) == '\\') {
                    s++;
                    continue;
                }
                inquote = !inquote;
                s++;
                continue;
            }
            if (!inquote && *s == ')')
                break;
            s++;
        }
        if (inquote || *s != ')' || s == t) {
            printf("SYNTAX ERROR: %s", buf);
            delete [] iotok;
            delete [] ntok;
            delete [] etok;
            delete [] ttok;
            continue;
        }
        ctmp = *s;
        char *stmp = s + 1;
        while (isspace(*stmp))
            stmp++;
        char *end = stmp;
        bool has_comma = (*stmp == ',');
        *s = 0;
        char *dtok = new char[s-t + 2];
        dtok[0] = *t++;  // '"'
        while (isspace(*t))
            t++;
        if (islower(*t))
            dtok[1] = toupper(*t);
        else
            dtok[1] = *t;
        strcpy(dtok+2, t+1);
        t = dtok + strlen(dtok) - 1;
        while (t >= dtok && isspace(*t))
            *t-- = 0;
        if (*t == '"') {
            *t-- = 0;
            while (t >= dtok && isspace(*t))
                *t-- = 0;
        }
        t[1] = '"';
        t[2] = 0;
        *s = ctmp;

        if (has_comma)
            end++;
        while (isspace(*end))
            end++;
        char *cmt = 0;
        if (*end) {
            cmt = new char[strlen(end) + 1];
            strcpy(cmt, end);
            end = cmt + strlen(cmt) - 1;
            while (end > cmt && isspace(*end))
                *end-- = 0;
        }

        // Dump the formatted line.
        sprintf(buf, "%s(%s,", iotok, ntok);
        t = buf + strlen(buf);
        while (t-buf < 24)
            *t++ = ' ';
        sprintf(t, "%s,", etok);
        while (*t)
            t++;
        while (t-buf < 44)
            *t++ = ' ';
        sprintf(t, "%s,", ttok);
        while (*t)
            t++;
        *t++ = '\n';
        char *b = t;
        while (t - b < 16)
            *t++ = ' ';
        strcpy(t, dtok);
        while (*t)
            t++;
        *t++ = ')';
        if (has_comma)
            *t++ = ',';

        // Add trailing comment, if any.
        if (cmt) {
            *t++ = ' ';
            end = cmt;
            while (*end)
                *t++ = *end++;
            delete [] cmt;
        }

        *t = 0;
        puts(buf);

        delete [] iotok;
        delete [] ntok;
        delete [] etok;
        delete [] ttok;
        delete [] dtok;
    }
    return (0);
}

