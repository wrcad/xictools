
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
 * lstunpack -- Fast[er]Cap list file unpacker.                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "miscutil/lstring.h"


// This is a simple program to create a set of FastCap input files from a
// unified list file, The unified format is supported by FasterCap from
// FastFieldSolvers.com, and by the Whiteley Research version of FastCap. 
// It is not supported in the original MIT FastCap, or in FastCap2 from
// FastFieldSolvers.com. 
//
// The argument is a path to a unified list file.  The files are created
// in the current directory.  If the original file is named filename.lst,
// the new list file is named filename_unp.lst.

int main(int argc, char **argv)
{
    if (argc != 2) {
        printf("Usage:  %s filename\n", argv[0]);
        printf(
            "This unpacks a FasterCap unified list file to old-style separate\n"
            "FastCap files, created in the current directory.\n\n");
        return (1);
    }
    FILE *fpin = fopen(argv[1], "r");
    if (!fpin) {
        printf("Unable to open %s.\n\n", argv[1]);
        return (2);
    }

    char buf[256];
    strcpy(buf, lstring::strip_path(argv[1]));
    char *e = strrchr(buf, '.');
    if (e)
        *e = 0;
    strcat(buf, "_unp.lst");

    FILE *fpout =fopen(buf, "w");
    if (!fpout) {
        printf("Unable to open %s.\n\n", buf);
        return (3);
    }

    int lcnt = 0;
    while (fgets(buf, 256, fpin) != 0) {
        lcnt++;
        if (buf[0] == 'E' || buf[0] == 'e') {
            fclose(fpout);
            fpout = 0;
            continue;
        }
        if (buf[0] == 'F' || buf[0] == 'f') {
            if (fpout) {
                fclose(fpout);
                fpout = 0;
            }
            char *s = buf;
            lstring::advtok(&s);
            char *tok = lstring::getqtok(&s);
            if (!tok) {
                printf("Warning: F line without file name on line %d.\n",
                    lcnt);
                continue;
            }
            fpout =fopen(tok, "w");
            if (!fpout) {
                printf("Unable to open %s.\n\n", tok);
                return (3);
            }
            delete [] tok;
            continue;
        }
        if (fpout)
            fputs(buf, fpout);
    }
    return (0);
}

