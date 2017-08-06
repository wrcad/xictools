
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
 * License server and authentication, and related utilities               *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <ctype.h>
#ifdef WIN32
#include <winsock2.h>
#include <process.h>
#else
#include <pwd.h>
#include <netdb.h>
#include <netinet/in.h>
#endif
#include <time.h>
#include "config.h"
#include "secure.h"
#include "secure_prv.h"
#include "key.h"
#include "miscutil/encode.h"
#include "miscutil/randval.h"

namespace validate {
    void usage();
    int setup_license(bool);
    void hostaddr(char*, unsigned char*);
    time_t timeset(int);
    int  make_license();
    void setblk(int, int);
    void bogus_block(int);

    randval rnd;

    sTR Tr[NUMBLKS];
    int NumBlocks;

    dblk Blocks[NUMBLKS+1];
    bool  ReadDef;
    int   ProgCode;
    char *Machine;
    char *IpAddr;
}


//
// Options:
//  -r          read XtLicenseInfo file
//  -c code     program code
//  -m machine  host name
//  -i ipaddr   IP address
//

int
main(int argc, char **argv)
{
    using namespace validate;
#ifdef WIN32
    // initialize winsock
    { WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        fprintf(stderr,
    "Warning: WSA initialization failed, no interprocess communication.\n");
        exit(1);
    }
    }
#endif
    int i = 1;
    while (i < argc) {
        if (*argv[i] == '-') {
            if (argv[i][1] == 'r') {
                ReadDef = true;
                i++;
                continue;
            }
            if (argv[i][1] == 'c') {
                i++;
                if (i == argc)
                    usage();
                ProgCode = atoi(argv[i]);
                i++;
                continue;
            }
            if (argv[i][1] == 'm') {
                i++;
                if (i == argc)
                    usage();
                Machine = strdup(argv[i]);
                i++;
                continue;
            }
            if (argv[i][1] == 'i') {
                i++;
                if (i == argc)
                    usage();
                IpAddr = strdup(argv[i]);
                i++;
                continue;
            }
        }
        usage();
    }

    setup_license(ReadDef);
    return(0);
}


void
validate::usage()
{
    fprintf(stderr,
    "Usage: \n\tvalidate [-r] [-c progcode] [-m host] [-i ipaddr]\n\n");
    exit(1);
}


namespace validate {
    struct v_info
    {
        char *hostname;
        char *key;
    };

    struct v_stringlist
    {
        v_stringlist(const char *s) { string = strdup(s); next = 0; }

        char *string;
        v_stringlist *next;
    };

    // Return the program code.
    int getcode(const char *str)
    {
        if (isalpha(*str)) {
            if (!strncasecmp(str, "xiv", 3))
                return (XIV_CODE);
            if (!strncasecmp(str, "xicii", 5))
                return (XICII_CODE);
            if (!strncasecmp(str, "xic", 3))
                return (XIC_CODE);
            if (!strncasecmp(str, "wrs", 3))
                return (WRSPICE_CODE);
            return (-1);
        }
        int d;
        if (sscanf(str, "%d", &d) == 1 && d >= 0 && d <= NUMBLKS)
            return (d);
        return (-1);
    }
}


int
validate::setup_license(bool use_def)
{
    char buf[1024], tbuf[64];
    v_info info[30];
    int infonum = 0;
    if (use_def) {
        printf("Reading XtLicenseInfo...\n");
        printf("Host is hostname:key, addr is 192.168.0.1\n\n");
        FILE *tp = fopen("XtLicenseInfo", "r");
        if (tp) {
            char hname[128], hkey[128];
            while (fgets(buf, 512, tp) && !strncmp(buf, "Succeeded", 9)) {
                char *t = 0;
                if (fgets(buf, 512, tp) && (t = strchr(buf, ':')) != 0) {
                    t++;
                    while (isspace(*t))
                        t++;
                    strcpy(hname, t);
                    t = hname + strlen(hname) - 1;
                    while (t >= hname && isspace(*t))
                        *t-- = 0;
                }
                else {
                    fprintf(stderr, "Format error.\n");
                    exit (1);
                }
                if (fgets(buf, 512, tp) && (t = strchr(buf, ':')) != 0) {
                    t++;
                    while (isspace(*t))
                        t++;
                    strcpy(hkey, t);
                    t = hkey + strlen(hkey) - 1;
                    while (t >= hkey && isspace(*t))
                        *t-- = 0;
                    if (hkey[2] == ':') {
                        // ethernet address, must be lower case!
                        for (char *a = hkey; *a; a++)
                            if (isupper(*a))
                                *a = tolower(*a);
                    }
                }
                else {
                    fprintf(stderr, "Format error.\n");
                    exit (1);
                }
                if (strlen(hname) == 0 || strlen(hkey) == 0 ||
                        strlen(hname) + strlen(hkey) >= 63) {
                    fprintf(stderr, "Format error.\n");
                    exit (1);
                }
                info[infonum].hostname = strdup(hname);
                // The host name should have the domain path stripped.
                char *ds = strchr(info[infonum].hostname, '.');
                if (ds)
                    *ds = 0;
                info[infonum].key = strdup(hkey);
                infonum++;
            }
            fclose (tp);
        }
        else {
            fprintf(stderr, "Can't open XtLicenseInfo.\n");
            exit(1);
        }
    }

    unsigned char addr[4];
    char hostname[64], keystring[64];
    hostaddr(hostname, addr);

    v_stringlist *s0 = 0, *se = 0;
    s0 = se = new v_stringlist("begin");  // dummy first line

    if (infonum) {
        strcpy(hostname, info[0].hostname);
        strcpy(keystring, info[0].key);
        addr[0] = 192;
        addr[1] = 168;
        addr[2] = 0;
        addr[3] = 1;
    }
    else
        sprintf(keystring, "%u.%u.%u.%u", addr[0], addr[1], addr[2], addr[3]);

    int months = 0;
    int count, infocnt = 0;
    for (count = 0; ; count++) {

        printf("Block %d:\n", count);
        printf("Program code? [%d] ",
            count ? Tr[count-1].limits[0] : ProgCode);
        fflush(stdout);
        fgets(buf, 1024, stdin);
        int d = getcode(buf);
        if (d >= 0)
            Tr[count].limits[0] = d;
        else
            Tr[count].limits[0] = count ? Tr[count-1].limits[0] : 0;
        sprintf(tbuf, "%d", Tr[count].limits[0]);
        se->next = new v_stringlist(tbuf);
        se = se->next;

        if (months)
            sprintf(tbuf, "%d", months);
        printf("Months active? [%s] ", months ? tbuf : "unlimited");
        fflush(stdout);
        fgets(buf, 1024, stdin);
        if (sscanf(buf, "%d", &d) != 1)
            d = months;
        months = d;
        int ts;
        if (d)
            ts = timeset(d);
        else
            ts = 0;
        sprintf(tbuf, "%d", d);
        se->next = new v_stringlist(tbuf);
        se = se->next;
        Tr[count].death_date = ts;

        for (;;) {
            printf("Hostname (+/-)? [%s] ", hostname);
            fflush(stdout);
            fgets(buf, 64, stdin);
            if (*buf == '+') {
                if (infocnt < infonum - 1) {
                    infocnt++;
                    addr[0] = 192;
                    addr[1] = 168;
                    addr[2] = 0;
                    addr[3] = 1;
                    strcpy(hostname, info[infocnt].hostname);
                    strcpy(keystring, info[infocnt].key);
                }
                continue;
            }
            if (*buf == '-') {
                if (infocnt > 0) {
                    infocnt--;
                    addr[0] = 192;
                    addr[1] = 168;
                    addr[2] = 0;
                    addr[3] = 1;
                    strcpy(hostname, info[infocnt].hostname);
                    strcpy(keystring, info[infocnt].key);
                }
                continue;
            }
            if (*buf == '\n')
                strcpy(buf, hostname);
            else {
                buf[strlen(buf) - 1] = '\0';

                /*******
                 * Class B/C licenses, not supported.
                if (!strcmp(buf, "any") || !strcmp(buf, "any c")) {
                    printf ("Setting a[3] to 0 for class C site\n");
                    addr[3] = 0;
                    strcpy(buf, ANYHOST);
                }
                else if (!strcmp(buf, "any b")) {
                    printf ("Setting a[2],a[3] to 0 for class B site\n");
                    addr[2] = 0;
                    addr[3] = 0;
                    strcpy(buf, ANYHOSTB);
                }
                *******/

                strcpy(hostname, buf);
            }
            break;
        }
        buf[64] = '\0';
        STRIPHOST(buf);
        se->next = new v_stringlist(buf);
        se = se->next;

        char kbuf[64];
        printf("Key? [%s] ", keystring);
        fflush(stdout);
        fgets(kbuf, 64, stdin);
        if (*kbuf == '\n')
            strcpy(kbuf, keystring);
        char *e = kbuf + strlen(kbuf) - 1;
        while (e >= kbuf && isspace(*e))
            *e-- = 0;
        strcpy(keystring, kbuf);
        int a0, a1, a2, a3;
        if (sscanf(kbuf, "%d.%d.%d.%d", &a0, &a1, &a2, &a3) == 4 &&
                a0 > 0 && a0 < 255 && a1 > 0 && a2 < 255 &&
                a2 >= 0 && a2 <= 255 && a3 >= 0 && a3 <= 255) {
            addr[0] = a0;
            addr[1] = a1;
            addr[2] = a2;
            addr[3] = a3;
            printf("User key: %s:%d.%d.%d.%d\n", buf, a0, a1, a2, a3);
            se->next = new v_stringlist(kbuf);
            se = se->next;
        }
        else {
            addr[0] = 192;
            addr[1] = 168;
            addr[2] = 0;
            addr[3] = 1;

            int dmy;
            if (sscanf(kbuf, "%x:%x:%x:%x:%x:%x", &dmy, &dmy, &dmy,
                    &dmy, &dmy, &dmy) == 6) {
                // Make sure Ethernet address is lower case.
                char *t = kbuf;
                while (*t) {
                    if (isupper(*t))
                        *t = tolower(*t);
                    t++;
                }
            }
            se->next = new v_stringlist(kbuf);
            se = se->next;
            printf("Skip hostname (for Apple)? [n] ");
            fflush(stdout);
            char xbuf[16];
            fgets(xbuf, 16, stdin);
            if (*xbuf == 'y' || *xbuf == 'Y') {
                strcpy(buf, kbuf);
                se->next = new v_stringlist("y");
            }
            else {
                strcat(buf, ":");
                strcat(buf, kbuf);
                se->next = new v_stringlist("n");
            }
            se = se->next;
            buf[64] = '\0';
            printf("User key: %s\n", buf);
        }

        Tr[count].valid_host = strdup(buf);
        Tr[count].valid_addr[0] = addr[0];
        Tr[count].valid_addr[1] = addr[1];
        Tr[count].valid_addr[2] = addr[2];
        Tr[count].valid_addr[3] = addr[3];

        if (Tr[count].limits[0] == SERVER_CODE) {
            printf("Slave server? [n] ");
            fflush(stdout);
            fgets(buf, 1024, stdin);
            if (*buf == 'y') {
                Tr[count].limits[1] = 1;
                se->next = new v_stringlist("y");
            }
            else {
                Tr[count].limits[1] = 0;
                se->next = new v_stringlist("n");
            }
            se = se->next;
        }

        if (count == NUMBLKS-1)
            break;
        printf("Add another block ('r' to repeat last)? [n] ");
        fflush(stdout);
        fgets(buf, 1024, stdin);
        if (*buf == 'r' || *buf == 'b') {
            // repeat last block
            Tr[count].limits[1] = 0;
            count--;
            se->next = new v_stringlist("b");
            se = se->next;
            continue;
        }
        if (*buf != 'y') {
            se->next = new v_stringlist("n");
            se = se->next;
            break;
        }
        se->next = new v_stringlist("y");
        se = se->next;
    }

    // Set the limits.  The server will balk at accepting more than the
    // limit for each type code, for all machines served.
    //
    int limits[20];
    for (int i = 0; i < 20; i++)
        limits[i] = -1;
    for (int i = 0; i <= count; i++) {
        int c = Tr[i].limits[0];
        switch (c) {
        case SERVER_CODE:
        case OA_CODE:
        case XIV_CODE:
        case XICII_CODE:
        case XIC_CODE:
        case XIC_DAEMON_CODE:
        case WRSPICE_CODE:
            if (limits[c] < 0) {
                printf("Code %d user limitation? [0] ", c);
                fflush(stdout);
                fgets(buf, 1024, stdin);
                int lim;
                if (sscanf(buf, "%d", &lim) == 1 && lim > 0) {
                    if (lim > 255)
                        lim = 255;
                    Tr[i].limits[1] = lim;
                    limits[c] = lim;
                }
                else {
                    Tr[i].limits[1] = 0;
                    limits[c] = 0;
                }
                sprintf(tbuf, "%d", Tr[i].limits[1]);
                se->next = new v_stringlist(tbuf);
                se = se->next;
            }
            else
                Tr[i].limits[1] = limits[c];
        default:
            break;
        }
    }

    NumBlocks = count + 1;
    make_license();

    sprintf(buf, "validate.log");
    if (!access(buf, F_OK)) {
        strcpy(buf+512, buf);
        strcat(buf+512, ".bak");
#ifdef WIN32
        rename(buf, buf+512);
#else
        link(buf, buf+512);
        unlink(buf);
#endif
    }
    FILE *fp = fopen(buf, "w");
    if (!fp) {
        fprintf(stderr, "Can't open %s: Permission denied\n", buf);
        exit(1);
    }
    for (v_stringlist *s = s0->next; s; s = s->next)
        fprintf(fp, "%s\n", s->string);
    fclose(fp);

    return (0);
}


void
validate::hostaddr(char *host, unsigned char *addr)
{
    bool hset =false;
    if (Machine) {
        strncpy(host, Machine, 64);
        host[64] = '\0';
        STRIPHOST(host);
        hset = true;
    }
    bool aset = false;
    if (IpAddr) {
        int a[4];
        if (sscanf(IpAddr, "%d.%d.%d.%d", &a[0], &a[1], &a[2], &a[3]) == 4) {
            if (a[0] >= 0 && a[1] >= 0 && a[2] >= 0 && a[3] >= 0 &&
                    a[0] <= 255 && a[1] <= 255 && a[2] <= 255 && a[3] <= 255) {
                addr[0] = a[0];
                addr[1] = a[1];
                addr[2] = a[2];
                addr[3] = a[3];
                aset = true;
            }
        }
    }
    if (hset) {
        if (!aset) {
            addr[0] = 0;
            addr[1] = 0;
            addr[2] = 0;
            addr[3] = 0;
        }
        return;
    }

    host[64] = '\0';
    gethostname(host, 64);
    hostent *he = gethostbyname(host);
    if (!he) {
        fprintf(stderr, "gethostbyname failed for %s.\n", host);
        exit(1);
    }
    STRIPHOST(host);
    addr[0] = he->h_addr_list[0][0];
    addr[1] = he->h_addr_list[0][1];
    addr[2] = he->h_addr_list[0][2];
    addr[3] = he->h_addr_list[0][3];
}


time_t
validate::timeset(int months)
{
    time_t loc;
    time(&loc);
    tm *t = localtime(&loc);
    months += t->tm_mon;
    int years = months/12;
    months %= 12;
    t->tm_year += years;
    t->tm_mon = months;
    // Add a 3-day grace period.
    time_t texp = mktime(t) + 3*24*3600;

    tm *tp = localtime(&texp);
    char *ap = asctime(tp);
    printf("License Expires: %s", ap);
    return (texp);
}


//
// The following routines create the LICENSE file.
//

int
validate::make_license()
{
    if (NumBlocks > NUMBLKS) {
        printf("Too many blocks, %d max.\n", NUMBLKS);
        exit(0);
    }
    rnd.rand_seed(getpid());
    unsigned char done[NUMBLKS];
    for (int i = 0; i < NUMBLKS; i++)
        done[i] = 0;
    for (int i = 0; i < NumBlocks; i++) {
        int j = rnd.rand_value() % NUMBLKS;
        if (done[j]) {
            for ( ; j < NUMBLKS; j++)
                if (!done[j])
                    break;
            if (j == NUMBLKS) {
                for (j = 0; j < NUMBLKS; j++)
                    if (!done[j])
                        break;
            }
        }
        done[j] = 1;
        setblk(j, i);
    }
    for (int i = 0; i < NUMBLKS; i++)
        if (!done[i])
            bogus_block(i);
    bogus_block(NUMBLKS);

    // In the pre 1.8 version, the user limit was established on a
    // per-program basis, which was stored in Blocks[progcode].limits[2].
    // Initialize these values so that an old server won't provide a
    // larger limit.
    Blocks[XIC_CODE].limits[2] = 2;
    Blocks[XIC_DAEMON_CODE].limits[2] = 2;
    Blocks[WRSPICE_CODE].limits[2] = 2;

    // create last block, store maxusers, and zero sum
    for (int i = 0; i < NumBlocks; i++) {
        if (Tr[i].limits[1])
            Blocks[i].limits[2] = Tr[i].limits[1];
    }
    for (int i = 0; i < 16; i++)
        Blocks[NUMBLKS].sum[i] = 0;

    // compute checksum and write to last block
    MD5cx ctx;
    ctx.update((const unsigned char*)Blocks,
        (NUMBLKS+1)*sizeof(dblk));
    ctx.update((const unsigned char*)key, sizeof(key));
    unsigned char final[16];
    ctx.final(final);
    for (int i = 0; i < 16; i++)
        Blocks[NUMBLKS].sum[i] = final[i];

    if (!access(AUTH_FILE, F_OK)) {
        if (!access(AUTH_FILE_BAK, F_OK)) {
            chmod(AUTH_FILE_BAK, 0666);
            unlink(AUTH_FILE_BAK);
        }
#ifdef WIN32
        rename(AUTH_FILE, AUTH_FILE_BAK);
#else
        link(AUTH_FILE, AUTH_FILE_BAK);
        unlink(AUTH_FILE);
#endif
    }
    FILE *fp = fopen(AUTH_FILE, "wb");
    if (!fp) {
        printf("Can't open %s file, permission denied.\n", AUTH_FILE);
        exit(1);
    }
    fwrite(Blocks, sizeof(dblk), NUMBLKS+1, fp);
    fclose(fp);
    chmod(AUTH_FILE, 0444);
    printf("Output written to %s file.\n", AUTH_FILE);
    return (0);
}


void
validate::setblk(int i, int j)
{
    Blocks[i].timelim = htonl(Tr[j].death_date);
    Blocks[i].limits[0] = Tr[j].limits[0];  // prog code
    Blocks[i].limits[1] = Tr[j].limits[1] ? 1 : 0;  // user limit flag
    Blocks[i].limits[2] = key[16];
    Blocks[i].limits[3] = j;                // block where user limit saved
    block blk;
    blk.addr[0] = Tr[j].valid_addr[0];
    blk.addr[1] = Tr[j].valid_addr[1];
    blk.addr[2] = Tr[j].valid_addr[2];
    blk.addr[3] = Tr[j].valid_addr[3];
    blk.code[0] = Tr[j].limits[0];
    blk.code[1] = key[0];
    blk.code[2] = key[16];
    blk.code[3] = key[32];
    int k;
    for (k = 0; Tr[j].valid_host[k] && k < HOSTNAMLEN; k++) {
        // The hostname is stored as lower case.
        // This is new in Generation-3!
        char c = Tr[j].valid_host[k];
        blk.hostname[k] = isupper(c) ? tolower(c) : c;
    }
    unsigned char *s = (unsigned char*)key;
    for ( ; k < HOSTNAMLEN; k++)
        blk.hostname[k] = *s++;
    MD5cx ctx;
    ctx.update((const unsigned char*)&blk, sizeof(block));
    ctx.update((const unsigned char*)key, sizeof(key));
    unsigned char final[16];
    ctx.final(final);
    for (k = 0; k < 16; k++)
        Blocks[i].sum[k] = final[k];
}


void
validate::bogus_block(int i)
{
    time_t tloc;
    time(&tloc);
    Blocks[i].timelim = htonl(tloc + (rnd.rand_value() % 365*24*3600));

    // Make these "lowbin" values (0-31) to increase the probability
    // that the file is recognized as binary in MIME categorization,
    // e.g., when using mutt to email the file,
    // "application/octet-stream" is used if more than 10% of the
    // characters are lowbin.  Otherwise, a text/plain encoding is
    // used, which corrupts the file.
    // 
    for (int k = 0; k < 4; k++)
        Blocks[i].limits[k] = rnd.rand_value() & 0x1f;

    for (int k = 0; k < 16; k++)
        Blocks[i].sum[k] = rnd.rand_value() & 0xff;
}

