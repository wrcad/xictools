
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
 * License Data Extraction Program                                        *
 *                                                                        *
 *========================================================================*
 $Id: licinfo.cc,v 2.8 2016/01/12 19:09:39 stevew Exp $
 *========================================================================*/

 // Under Windows, create the XtLicenseInfo file.  Under Unix/Linux,
 // print the host name and IP address, and the hardware address.

#include "secure.h"
#include "miscutil.h"
#include <string.h>
#include <stdlib.h>
#ifdef WIN32
#include "msw.h"
#else
#include <unistd.h>
#include <netdb.h>
#endif

// Name of output file
#define OUTFILE "XtLicenseInfo"

#ifdef WIN32
// Tell the msw interface that we're Generation 4.
const char *msw::MSWpkgSuffix = "-4";
#endif


int
main()
{
#ifdef WIN32
    FreeConsole();
    WSADATA wsadata;
    if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
        MessageBox(0,
            "Windows Socket Architecture failed to initialize",
            "ERROR", MB_ICONSTOP);
        return (1);
    }

    FILE *fp = fopen(OUTFILE, "w");

    char buf[1024];
    bool success = true;
    if (!msw::GetProductID(buf, fp)) {
        strcpy(buf, "key not available");
        success = false;
    }
    char *key = strdup(buf);

    if (gethostname(buf, 256)) {
        strcpy(buf, "hostname not found");
        success = false;
    }
    char *hostname = strdup(buf);

    sprintf(buf, "%s\nHost Name: %s\nKey: %s\n",
        success ? "Succeeded" : "FAILED", hostname, key);
    if (fp) {
        fputs(buf, fp);
        fclose(fp);
    }
    sprintf(buf + strlen(buf), "%s \"%s\"\n",
        fp ? "Output in file" : "Failed opening output file", OUTFILE);
    MessageBox(0, buf, "XicTools License Info",
        success ? MB_ICONINFORMATION : MB_ICONSTOP);

#else

    char hostname[256];
    if (gethostname(hostname, 256) != 0) {
        fprintf(stderr, "Error: system call failed: gethostname\n");
        fprintf(stderr, "I can't determine the host name!\n");
        exit(1);
    }
    hostent *he = gethostbyname(hostname);
    if (!he) {
        fprintf(stderr, "Error: system call failed: gethostbyname\n");
        fprintf(stderr, "I can't resolve IP address for %s!\n", hostname);
        exit (1);
    }

    printf("Host name:\t%s\n", hostname);
    for (char **al = he->h_addr_list; al[0]; al++) {
        unsigned char *a = (unsigned char*)al[0];
        printf("ARP IP address:\t%d.%d.%d.%d\n", a[0], a[1], a[2], a[3]);
    }

    const char *key = 0;
    miscutil::ifc_t *ifc = miscutil::net_if_list();
    for (miscutil::ifc_t *i = ifc; i; i = i->next()) {
        printf("%s\n", i->name());
        if (i->ip())
            printf("  IP %s\n", i->ip());
        if (i->hw()) {
            printf("  HW %s\n", i->hw());
            if (!key && i->ip())
                key = i->hw();
        }
    }
    if (key) {
        FILE *fp = fopen(OUTFILE, "w");
        if (fp) {
            fprintf(fp, "%s\nHost Name: %s\nKey: %s\n",
                "Succeeded", hostname, key);
            fclose(fp);
            printf("%s file created in current directory.\n", OUTFILE);
        }
        else
            printf("Error: unable to write %s file.\n", OUTFILE);
    }
    ifc->free();
#endif

    return (0);
}

