
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

// Apply the mutt binary mime categorization test to the file given as
// an argument (default "LICENSE") and print results.  We want
// "BINARY", otherwise mutt would assign "text/plain" to the
// attachment which corrupts the file.

int main(int argc, char **argv)
{
    const char *fname = "LICENSE";
    if (argc > 1)
        fname = argv[1];
    FILE *fp = fopen(fname, "rb");
    if (!fp) {
        fprintf(stderr, "Failed to open file for reading.\n");
        return (1);
    }

    // From mutt.
    int ch;
    int lobin = 0;
    int hibin = 0;
    int ascii = 0;
    while ((ch = getc(fp)) != EOF) {
        if (ch == '\n')
            continue;
        if (ch == '\r')
            continue;
        if (ch & 0x80)
            hibin++;
        else if (ch == '\t' || ch == '\f')
            ascii++;
        else if (ch < 32 || ch == 127)
            lobin++;
        else
            ascii++;
    }
    if (lobin == 0 || (lobin + hibin + ascii)/lobin >= 10)
        printf("ASCII\n");
    else
        printf("BINARY\n");
    if (lobin == 0)
        printf("lobin\t0\n");
    printf("lobin\t%d\nhibin\t%d\nascii\t%d\ntest\t%d\n",
        lobin, hibin, ascii, (lobin + hibin + ascii)/lobin);
    return (0);
}
