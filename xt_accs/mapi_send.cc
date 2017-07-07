
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2015 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
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
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * MAPI_SEND.CC -- Simple MAPI mail client for Windows                    *
 *                                                                        *
 *========================================================================*
 $Id: mapi_send.cc,v 1.1 2015/05/05 03:32:43 stevew Exp $
 *========================================================================*/

#include "lstring.h"
#include "msw.h"


//
// This is a very skeletal BSD-style "mail" program for Windows, for
// outgoing only, using simple MAPI.
//
// Note:  On Windows 8, MAPI may not be installed.  You can install
// Windows Live Mail from the free Windows Essentials download from
// Microsoft, which provides MAPI.  Once the outgoing server is
// configured, this application should work.
//
// Alas, there is a typical Microsoft issue.  The message is sent
// without a Date header, so it is likely to be rejected.  Perhaps
// installing Thunderbird would avoid this problem, I don't know.
//
// mail [-s subject] [-q file] [-a attachment ...] mailaddr
//
// -s subject
//   Specify the message subject.  The subject should be double
//   quoted if it contains white space.
//
// -q file
//   The contents of the file will be read into the message.
//
// -a attachment
//   The indicated file will be added to the message as an attachment.
//
// Terminate text entry with a single '.' line.


// Tell the msw interface that we're Generation 4.
const char *msw::MSWpkgSuffix = "-4";


int main(int argc, char **argv)
{
    stringlist *attachments = 0;
    const char *subject = 0;
    const char *file = 0;
    const char *address = 0;

    for (int i = 1; i < argc; i++) {
        const char *s = argv[i];
        if (*s == '-') {
            s++;
            if (*s == 's') {
                if (i < argc - 1)
                    subject = argv[++i];
                continue;
            }
            if (*s == 'a') {
                if (i < argc - 1)
                    attachments = new stringlist(lstring::copy(argv[++i]),
                        attachments);
                continue;
            }
            if (*s == 'q')
                if (i < argc - 1)
                    file = argv[++i];
                continue;
            if (*s == 'i' || *s == 'n' || *s == 'v')
                continue;
            fprintf(stderr, "unknown option %s\n", s);
            return (1);
        }
        address = argv[i];
    }
    if (!address) {
        fprintf(stderr, "no destination given\n");
        return (1);
    }
    if (!subject)
        subject = "no subject";
    sLstr lstr;
    if (file) {
        FILE *fp = fopen(file, "r");
        if (!fp) {
            fprintf(stderr, "can't open %s\n", file);
            return (0);
        }
        int c;
        while ((c = getc(fp)) != EOF)
            lstr.add_c(c);
        fclose(fp);
    }
    int c, ccnt = 0;
    bool ready = false;
    while ((c = getchar()) != EOF) {
        if (!ccnt && c == '.') {
            ready = true;
            ccnt++;
            continue;
        }
        ccnt++;
        if (c == '\n') {
            if (ready)
                break;
            ccnt = 0;
        }
        if (ccnt > 1) {
            if (ready)
                lstr.add_c('.');
            ready = false;
        }
        lstr.add_c(c);
    }
    ccnt = 0;
    for (stringlist *a = attachments; a; a = a->next)
        ccnt++;
    char **aary = 0;
    if (ccnt > 0) {
        aary = new char*[ccnt+1];
        ccnt = 0;
        for (stringlist *a = attachments; a; a = a->next) {
            aary[ccnt++] = a->string;
            a->string = 0;
        }
        aary[ccnt] = 0;
        stringlist::destroy(attachments);
    }

    const char *ret = msw::MapiSend(address, subject, lstr.string(),
        ccnt, aary);
    if (ret) {
        fprintf(stderr, "%s\n", ret);
        return (1);
    }
    printf("message sent\n");
    return (0);
}

