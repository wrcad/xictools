
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

 // Under Windows, create the XtLicenseInfo file.  Under Unix/Linux,
 // print the host name and IP address, and the hardware address.

#include "secure.h"
#include "miscutil/miscutil.h"
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
//const char *msw::MSWpkgSuffix = "-4";
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
    miscutil::ifc_t::destroy(ifc);
#endif

    return (0);
}

