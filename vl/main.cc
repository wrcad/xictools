
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
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
 * This software is available for non-commercial use under the terms of   *
 * the GNU General Public License as published by the Free Software       *
 * Foundation; either version 2 of the License, or (at your option) any   *
 * later version.                                                         *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * You should have received a copy of the GNU General Public License      *
 * along with this program; if not, write to the Free Software            *
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.              *
 *========================================================================*
 *                                                                        *
 * Verilog Support Files                                                  *
 *                                                                        *
 *========================================================================*
 $Id: main.cc,v 1.4 2006/02/28 01:39:35 stevew Exp $
 *========================================================================*/

#include "vl_st.h"
#include "vl_list.h"
#include "vl_defs.h"
#include "vl_types.h"

extern FILE *yyin;
extern int YYTrace;


int
main(int argc, char **argv)
{
    cout << '\n' << vl_version() << "\n\n";
    if (argc == 1) {
        cout << "Usage: vl [-v] [-d#] [-p] [-tmin | -tmax] file [files]\n";
        cout << "  -v       be verbose (print more warnings)\n";
        cout << "  -p       print input from internal representation\n";
        cout << "  -dN      N integer, debugging flags:\n";
        cout << "             1    print action before evaluation\n";
        cout << "             2    print variable assignment\n";
        cout << "             4    not used\n";
        cout << "             8    not used\n";
        cout << "             16   print time slot action list before "
                                   "evaluation\n";
        cout << "             32   print some things about the description\n";
        cout << "             64   dump module info\n";
        cout << "             128  print yacc parser trace\n";
        cout << "  -tmin    use minimum delays\n";
        cout << "  -tmax    use maximum delays\n";

        return (0);
    }
    bool print = false;
    int dbg = 0;
    VLdelayType mtm = DLYtyp;
    for (int i = 1; i < argc; i++)
        if (argv[i][0] == '-') {
            if (argv[i][1] == 'p')
                print = true;
            else if (argv[i][1] == 'd') {
                if (argv[i][2] == '0') {
                    if (argv[i][3] == 'x' || argv[i][3] == 'X')
                        sscanf(argv[i]+4, "%x", &dbg);
                    else
                        sscanf(argv[i]+4, "%o", &dbg);
                }
                else
                    dbg = atoi(argv[i]+2);
            }
            else if (argv[i][1] == 't') {
                char *s = argv[i] + 2;
                if (*s == '0' || (*s == 'm' && *(s+1) == 'i'))
                    mtm = DLYmin;
                else if (*s == '2' || (*s == 'm' && *(s+1) == 'a'))
                    mtm = DLYmax;
            }
            else if (argv[i][1] == 'v') {
                // handled in VP.parse()
                ;
            }
        }
    if (dbg & DBG_ptrace)
        YYTrace = 1;
    if (VP.parse(argc, argv))
        return (1);

    if (print)
        VP.print(cout);
    else {
        vl_simulator VS;
        if (VS.initialize(VP.description, mtm, dbg))
            VS.simulate();
        delete VS.description;
        VS.initialize(0);
    }
    return (0);
}


// Needed by vl, let vl handle file name resolution.
//
FILE *
vl_file_open(const char*, const char*)
{
    return (0);
}

