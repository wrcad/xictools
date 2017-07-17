
/*========================================================================*
 *                                                                        *
 *  XicTools Integrated Circuit Design System                             *
 *  Copyright (c) 2008 Whiteley Research Inc, all rights reserved.        *
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
 * License File Info                                                      *
 *                                                                        *
 *========================================================================*
 $Id: lfinfo.cc,v 2.8 2010/09/22 03:47:34 stevew Exp $
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
#include "encode.h"
#include "key.h"

namespace lfinfo {
    void usage();
    bool verify(void);
    dblk *open_license(char*);
    bool lookup(dblk*);
    dblk *read_license(char*, int*);
    dblk *find_valid(dblk*);
    bool check_time(dblk*);
    bool check_users(dblk*, dblk*);

    sTR Tr[NUMBLKS];

    char *LicPath;
    int   ProgCode;
    char *Machine;
    char *IpAddr;
}


//
// Options:
//  -l path     path to license file
//  -c code     program code
//  -m machine  host name
//  -i ipaddr   IP address
//

int
main(int argc, char **argv)
{
    using namespace lfinfo;
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
            if (argv[i][1] == 'l') {
                i++;
                if (i == argc)
                    usage();
                LicPath = strdup(argv[i]);
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

    if (verify())
        return (0);
    return (127);
}


void
lfinfo::usage()
{
    fprintf(stderr,
        "Usage: \n\tlfinfo [-l licfile_path] [-c progcode] "
        "[-m host] [-i ipaddr]\n\n");
    exit(1);
}


//
// The following functions are for verification of an existing LICENSE
// file.
//

namespace lfinfo {
    const char *strs0 = "Authorization check failed:";
    const char *strs1 =
        "Can't open license file - see your system administrator.\n";
    const char *strs2 = "Error reading license file.\n";
    const char *strs3 = "Checksum error in license file.\n";
    const char *strs4 = "%s\n  Host %s is not licensed to run this binary.\n";
    const char *strs5 = "%s\n  License for host %s has expired.\n";
    const char *strs6 = "%s\n  Licensed user quota exceeded.\n";
}


bool
lfinfo::verify()
{
    char buf[1024];
    bool oncethru = (LicPath && ProgCode && Machine && IpAddr);
    dblk *blocks = 0;
    for (;;) {
        if (LicPath) {
            strcpy(buf, LicPath);
            LicPath = 0;
        }
        else {
            printf("Enter path to LICENSE file: ");
            fflush(stdout);
            fgets(buf, 1024, stdin);
            char *s = strchr(buf, '\n');
            if (s)
                *s = '\0';
        }
        if (buf[0] == 0)
            strcpy(buf, "LICENSE");
        char *path = new char[strlen(buf) + 1];
        if (!path) {
            fprintf(stderr, "Out of memory!\n");
            exit(1);
        }
        strcpy(path, buf);
        blocks = open_license(path);
        if (!blocks) {
            fprintf(stderr, "Can't open license file.\n");
            delete [] path;
            continue;
        }
        break;
    }

    for (;;) {
        int d;
        if (ProgCode) {
            d = ProgCode;
            ProgCode = 0;
        }
        else {
            printf("Enter program code [%d]: ", Tr[0].limits[0]);
            fflush(stdout);
            fgets(buf, 1024, stdin);
            if (*buf == 'q')
                break;
            if (sscanf(buf, "%d", &d) < 1)
                d = Tr[0].limits[0];
        }
        switch (d) {
        case SERVER_CODE:
        case OA_CODE:
        case XIV_CODE:
        case XICII_CODE:
        case XIC_CODE:
        case XIC_DAEMON_CODE:
        case WRSPICE_CODE:
            Tr[0].limits[0] = d;
            break;
        default:
            continue;
        }

        if (Machine) {
            strcpy(buf, Machine);
            Machine = 0;
        }
        else {
            printf("Enter host [%s]: ",
                Tr[0].valid_host ? Tr[0].valid_host : "none specified");
            fflush(stdout);
            fgets(buf, 1024, stdin);
        }
        if (*buf == 'q' && buf[1] == '\n')
            break;
        if (*buf == '\n' && !Tr[0].valid_host)
            strcpy(buf, "any");
        if (*buf != '\n') {
            char *s = strchr(buf, '\n');
            if (s)
                *s = '\0';
            buf[64] = '\0';
            STRIPHOST(buf);
            if (!Tr[0].valid_host || strcmp(Tr[0].valid_host, buf)) {
                Tr[0].valid_host = new char[strlen(buf) + 1];
                if (!Tr[0].valid_host) {
                    fprintf(stderr, "Out of memory!\n");
                    exit(1);
                }
                strcpy(Tr[0].valid_host, buf);
            }
        }

        if (IpAddr) {
            strcpy(buf, IpAddr);
            IpAddr = 0;
        }
        else {
            printf("Enter IP address [%d.%d.%d.%d]: ", Tr[0].valid_addr[0],
                Tr[0].valid_addr[1], Tr[0].valid_addr[2], Tr[0].valid_addr[3]);
            fflush(stdout);
            fgets(buf, 1024, stdin);
            if (*buf == 'q')
                break;
        }
        int a0, a1, a2, a3;
        if (sscanf(buf, "%d.%d.%d.%d", &d, &d, &d, &d) == 4) {
            sscanf(buf, "%d.%d.%d.%d", &a0, &a1, &a2, &a3);
            Tr[0].valid_addr[0] = a0;
            Tr[0].valid_addr[1] = a1;
            Tr[0].valid_addr[2] = a2;
            Tr[0].valid_addr[3] = a3;
        }
        bool ret = lookup(blocks);
        if (oncethru)
            return (ret);
        printf("Enter 'q' or ^C anytime to quit.\n");
    }
    return (true);
}


dblk *
lfinfo::open_license(char *path)
{
    int error;
    dblk *blocks = read_license(path, &error);
    if (!blocks) {
        switch (error) {
        case 1:
            fputs(strs1, stderr);
            break;
        case 2:
            fprintf(stderr, "Memory allocation failure.\n");
            break;
        case 3:
            fputs(strs2, stderr);
            break;
        case 4:
            fputs(strs3, stderr);
            break;
        }
    }
    return (blocks);
}


bool
lfinfo::lookup(dblk *blocks)
{
    dblk *myblock = find_valid(blocks);
    // host name truncated
    if (!myblock) {
        fprintf(stderr, strs4, strs0, Tr[0].valid_host);
        return (false);
    }
    if (!check_time(myblock)) {
        fprintf(stderr, strs5, strs0, Tr[0].valid_host);
        return (false);
    }
    if (!check_users(myblock, blocks)) {
        fprintf(stderr, strs6, strs0);
        return (false);
    }
    return (true);
}


// Open the authorization file and read it in.  Perform the checksum and
// make sure it matches that in the file.  If error, 0 is returned, and
// error is set: 1 can't open file, 2 malloc failed, 3 read error, 4
// checksum error.
//
dblk *
lfinfo::read_license(char *path, int *error)
{
    FILE *fp = fopen(path, "rb");
    if (!fp) {
        *error = 1;
        return (0);
    }
    dblk *blks = (dblk*)new dblk[NUMBLKS+1];
    if (!blks) {
        fclose(fp);
        *error = 2;
        return (0);
    }
    int size = (NUMBLKS+1)*sizeof(dblk);
    int msz = fread(blks, size, 1, fp);
    fclose(fp);
    if (msz != 1) {
        *error = 3;
        return (0);
    }
    // copy checksum and zero before test
    unsigned char sum[16];
    unsigned char *sump = blks[NUMBLKS].sum;
    for (int i = 0; i < 16; i++) {
        sum[i] = sump[i];
        sump[i] = 0;
    }
    unsigned char final[16];
    MD5cx ctx;
    ctx.update((const unsigned char*)blks, size);
    ctx.update((const unsigned char*)key, sizeof(key));
    ctx.final(final);
    for (int i = 0; i < 16; i++) {
        if (sum[i] != final[i]) {
            *error = 4;
            return (0);
        }
    }
    *error = 0;
    return (blks);
}


// Find the block with matching checksum.
//
dblk*
lfinfo::find_valid(dblk *blocks)
{
    block myblk;
    int i;
    for (i = 0; Tr[0].valid_host[i] && i < HOSTNAMLEN; i++) {
        char c = Tr[0].valid_host[i];
        // Gen3 block hostname always lower case.
        myblk.hostname[i] = isupper(c) ? tolower(c) : c;
    }
    unsigned char *s = (unsigned char*)key;
    for ( ; i < HOSTNAMLEN; i++)
        myblk.hostname[i] = *s++;
    myblk.addr[0] = Tr[0].valid_addr[0];
    myblk.addr[1] = Tr[0].valid_addr[1];
    myblk.addr[2] = Tr[0].valid_addr[2];
    myblk.addr[3] = Tr[0].valid_addr[3];
    myblk.code[0] = Tr[0].limits[0];
    myblk.code[1] = key[0];
    myblk.code[2] = key[16];
    myblk.code[3] = key[32];

    unsigned char final[16];
    MD5cx ctx;
    ctx.update((const unsigned char*)&myblk, sizeof(block));
    ctx.update((const unsigned char*)key, sizeof(key));
    ctx.final(final);
    for (i = 0; i < NUMBLKS; i++)
        if (!memcmp(final, blocks[i].sum, 16))
            break;
    if (i < NUMBLKS)
        return (blocks + i);

    // If the host name contained an upper-case character, try again.
    // The Gen2 licenses do not have hostnames lower-cased.
    bool ucfound = false;
    for (i = 0; Tr[0].valid_host[i] && i < HOSTNAMLEN; i++) {
        char c = Tr[0].valid_host[i];
        if (isupper(c))
           ucfound = true;
        myblk.hostname[i] = c;
    }
    if (ucfound) {
        s = (unsigned char*)key;
        for ( ; i < HOSTNAMLEN; i++)
            myblk.hostname[i] = *s++;
        myblk.addr[0] = Tr[0].valid_addr[0];
        myblk.addr[1] = Tr[0].valid_addr[1];
        myblk.addr[2] = Tr[0].valid_addr[2];
        myblk.addr[3] = Tr[0].valid_addr[3];
        myblk.code[0] = Tr[0].limits[0];
        myblk.code[1] = key[0];
        myblk.code[2] = key[16];
        myblk.code[3] = key[32];

        ctx.reinit();
        ctx.update((const unsigned char*)&myblk, sizeof(block));
        ctx.update((const unsigned char*)key, sizeof(key));
        ctx.final(final);
        for (i = 0; i < NUMBLKS; i++)
            if (!memcmp(final, blocks[i].sum, 16))
                break;
        if (i < NUMBLKS)
            return (blocks + i);
    }

    if (Tr[0].limits[0] != SERVER_CODE && !strchr(Tr[0].valid_host, ':')) {
        // No entry for host.  Look for a site license.
        // Servers and hardware address keys are ineligible.

        // Class C license
        const char *bf = ANYHOST;
        for (i = 0; bf[i] && i < HOSTNAMLEN; i++)
            myblk.hostname[i] = bf[i];
        s = (unsigned char*)key;
        for ( ; i < HOSTNAMLEN; i++)
            myblk.hostname[i] = *s++;
        myblk.addr[0] = Tr[0].valid_addr[0];
        myblk.addr[1] = Tr[0].valid_addr[1];
        myblk.addr[2] = Tr[0].valid_addr[2];
        myblk.addr[3] = 0;
        myblk.code[0] = Tr[0].limits[0];
        myblk.code[1] = key[0];
        myblk.code[2] = key[16];
        myblk.code[3] = key[32];

        ctx.reinit();
        ctx.update((const unsigned char*)&myblk, sizeof(block));
        ctx.update((const unsigned char*)key, sizeof(key));
        ctx.final(final);
        for (i = 0; i < NUMBLKS; i++)
            if (!memcmp(final, blocks[i].sum, 16))
                break;
        if (i < NUMBLKS)
            return (blocks + i);

        // Class B license
        bf = ANYHOSTB;
        for (i = 0; bf[i] && i < HOSTNAMLEN; i++)
            myblk.hostname[i] = bf[i];
        s = (unsigned char*)key;
        for ( ; i < HOSTNAMLEN; i++)
            myblk.hostname[i] = *s++;
        myblk.addr[0] = Tr[0].valid_addr[0];
        myblk.addr[1] = Tr[0].valid_addr[1];
        myblk.addr[2] = 0;
        myblk.addr[3] = 0;
        myblk.code[0] = Tr[0].limits[0];
        myblk.code[1] = key[0];
        myblk.code[2] = key[16];
        myblk.code[3] = key[32];

        ctx.reinit();
        ctx.update((const unsigned char*)&myblk, sizeof(block));
        ctx.update((const unsigned char*)key, sizeof(key));
        ctx.final(final);
        for (i = 0; i < NUMBLKS; i++)
            if (!memcmp(final, blocks[i].sum, 16))
                break;
        if (i < NUMBLKS)
            return (blocks + i);
    }

    return (0);
}


// Print time limit.
//
bool
lfinfo::check_time(dblk *blk)
{
    if (blk->timelim == 0) {
        printf("License expires: Never\n");
        return (true);
    }
    time_t loc;
    time(&loc);
    time_t t = (time_t)ntohl(blk->timelim);
    tm *tp = localtime(&t);
    char *ap = asctime(tp);
    printf("License Expires: %s", ap);
    if ((unsigned)loc < ntohl(blk->timelim))
        return (true);
    return (false);
}


// Print user limit status.
//
bool
lfinfo::check_users(dblk *blk, dblk *blbase)
{
    if (blk->limits[0] != SERVER_CODE && blk->limits[1]) {
        printf("User limit: Applies (max users %d)\n",
            blbase[blk->limits[3]].limits[2]);
    }
    else
        printf("User limit: None\n");
    return (true);
}


#ifndef WIN32
#ifndef HAVE_GETHOSTBYNAME

hostent *
gethostbyname(const char *name)
{
    for (;;) {
        hostent *he = gethostent();
        if (!he)
            return (0);
        if (!strcmp(name, he->h_name))
            return (he);
        if (he->h_aliases) {
            for (int i = 0; he->h_aliases[i]; i++) {
                if (!strcmp(name, he->h_aliases[i]))
                    return (he);
            }
        }
    }
}

#endif
#endif

