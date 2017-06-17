
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2014 Whiteley Research Inc, all rights reserved.        *
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
 * LSTUNPACK.CC -- Fast[er]Cap list file unpacker.                        *
 *                                                                        *
 *========================================================================*
 $Id: lstunpack.cc,v 1.1 2014/07/08 04:35:06 stevew Exp $
 *========================================================================*/

#include "lstring.h"


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

