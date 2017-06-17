
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
 * FCPP.CC -- Fast[er]Cap output post-processor.                          *
 *                                                                        *
 *========================================================================*
 $Id: fcpp.cc,v 1.3 2014/07/19 03:44:24 stevew Exp $
 *========================================================================*/

#include "lstring.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

//
// Stand-alone post-processor for Fast[er]Cap output files.
//
// This extracts the capacitance and mutual capacitance values from
// the capacitance matrix, printing to standard output.
//
// Usage:  fcpp fastcap_outfile
//

namespace {
    // The matrix lines in the fastcap output (at least) can be
    // arbitrarily long.  This function reads and returns an
    // arbitrarily long line.
    //
    char *getline(FILE *fp)
    {
        sLstr lstr;
        int c;
        while ((c = fgetc(fp)) != EOF) {
            lstr.add_c(c);
            if (c == '\n')
                break;
        }
        return (lstr.string_trim());
    }


    // Return the scale factor given a word with a standard prefix.
    //
    double get_scale(const char *str, double *aux_scale)
    {
        *aux_scale = 1.0;
        if (lstring::ciprefix("atto", str))
            return (1e-18);
        if (lstring::ciprefix("femto", str))
            return (1e-15);
        if (lstring::ciprefix("pico", str))
            return (1e-12);
        if (lstring::ciprefix("nano", str))
            return (1e-9);
        if (lstring::ciprefix("micro", str))
            return (1e-6);
        if (lstring::ciprefix("milli", str))
            return (1e-3);
        if (lstring::ciprefix("kilo", str))
            return (1e+3);
        if (lstring::ciprefix("mega", str))
            return (1e+6);
        if (lstring::ciprefix("giga", str))
            return (1e+9);
        if (lstring::ciprefix("tera", str))
            return (1e+12);
        if (lstring::ciprefix("peta", str))
            return (1e+15);
        if (lstring::ciprefix("exa", str))
            return (1e+18);
        if (lstring::ciprefix("every", str)) {
            // FastCap can produce strange "out of range" units.
            lstring::advtok(&str);  // "every"
            lstring::advtok(&str);  // "unit"
            lstring::advtok(&str);  // "is"
            double d = atof(str);
            *aux_scale = d/1e-18;
            return (1e-18);
        }
        return (1.0);
    }


    // Return the appropriate scale factor prefix given the scale
    // factor.
    //
    const char *scale_str(double sc)
    {
        if (sc == 1e-18)
            return ("atto");
        if (sc == 1e-15)
            return ("femto");
        if (sc == 1e-12)
            return ("pico");
        if (sc == 1e-9)
            return ("nano");
        if (sc == 1e-6)
            return ("micro");
        if (sc == 1e-3)
            return ("milli");
        if (sc == 1e+3)
            return ("kilo");
        if (sc == 1e+6)
            return ("mega");
        if (sc == 1e+9)
            return ("giga");
        if (sc == 1e+12)
            return ("tera");
        if (sc == 1e+15)
            return ("peta");
        if (sc == 1e+18)
            return ("exa");
        return ("");
    }


    // Return the self-capacitance of t in mat.  This is the diagonal
    // term from the fastcap matrix, minus the mutual capacitance to
    // each other element.
    //
    double selfcap(int t, float **mat, int size)
    {
        float *row = mat[t];
        double cap = 0.0;
        for (int i = 0; i < size; i++)
            cap += row[i];
        return (cap);
    }


    // Return the mutual capacatance for Cpq (p != q) from mat.  This
    // is simply the negative of the matrix element.
    //
    double mutcap(int p, int q, float **mat)
    {
        return ((double)-mat[p][q]);
    }
}


// Parse the fastcap output file (fname) and return the
// capacitance matrix and size.  On error, an error string
// (malloc'ed) is returned instead.
//
char *
fc_get_matrix(const char *fname, int *size, float ***mret, double *units,
    char ***nameret)
{
    *size = 0;
    *mret = 0;
    *units = 0.0;
    *nameret = 0;
    char buf[256];
    FILE *fp = fopen(fname, "r");
    if (!fp) {
        sprintf(buf, "can't open file %s.", fname);
        return (lstring::copy(buf));
    }

    // We need to parse both forms.
    //
    // FastCap format:
    // CAPACITANCE MATRIX, femtofarads
    //                           1          2
    // 0%default_group 1      2.553     -2.393
    // 1%default_group 2     -2.393      2.551

    // FasterCap format:
    // Capacitance matrix is:
    // Demo evaluation version
    // Dimension 2 x 2
    // g1_0  3.76182e-15 -3.66421e-15
    // g2_1  -3.58443e-15 3.77836e-15
    //
    // This is repeated for each iteration, we need to grab the
    // last instance in the file.

    off_t posn = 0;
    char *str = 0;
    for (;;) {
        char *tstr;
        while ((tstr = getline(fp)) != 0) {
            if (lstring::ciprefix("CAPACITANCE MATRIX", tstr))
                break;
            delete [] tstr;
        }
        if (tstr) {
            delete [] str;
            str = tstr;
            posn = ftell(fp);
            continue;
        }
        break;
    }
    if (!str) {
        fclose(fp);
        sprintf(buf, "can't find matrix data in file %s.", fname);
        return (lstring::copy(buf));
    }
    fseek(fp, posn, SEEK_SET);

    char *s = str;
    lstring::advtok(&s);
    lstring::advtok(&s);
    double aux_scale;
    *units = get_scale(s, &aux_scale);
    delete [] str;

    int matsz = 0;
    for (;;) {
        if ((str = getline(fp)) == 0) {
            fclose(fp);
            sprintf(buf, "read error or premature EOF, file %s.", fname);
            return (lstring::copy(buf));
        }
        if (lstring::ciprefix("demo", str)) {
            // "Demo evaluation version"
            delete [] str;
            continue;
        }
        if (lstring::ciprefix("dimen", str)) {
            // "Dimension N x N"
            s = str;
            lstring::advtok(&s);
            matsz = atoi(s);
            delete [] str;
            break;
        }
        int cnt = 0;
        s = str;
        for (;;) {
            while (isspace(*s))
                s++;
            if (isdigit(*s)) {
                cnt++;
                while (isdigit(*s))
                    s++;
                continue;
            }
            break;
        }
        matsz = cnt;
        delete [] str;
        break;
    }

    if (!matsz) {
        fclose(fp);
        sprintf(buf, "matrix data has no size, file %s.", fname);
        return (lstring::copy(buf));
    }
    float **matrix = new float*[matsz];
    char **names = new char*[matsz];

    for (int i = 0; i < matsz; i++) {
        if ((str = getline(fp)) == 0) {
            fclose(fp);
            for (int k = 0; k < i; i++)
                delete [] matrix[k];
            delete [] matrix;
            sprintf(buf, "read error or premature EOF, file %s.", fname);
            return (lstring::copy(buf));
        }
        matrix[i] = new float[matsz];

        s = str;
        char *tok = lstring::gettok(&s);
        names[i] = tok;
        char *t = strchr(tok, '%');
        if (t) {
            // FastCap
            *t = 0;
            lstring::advtok(&s);
        }
        else {
            t = strchr(tok, '_');
            if (t) {
                // FasterCap
                names[i] = lstring::copy(t+1);
                delete [] tok;
            }
        }
        for (int j = 0; j < matsz; j++) {
            tok = lstring::gettok(&s);
            if (!tok) {
                fclose(fp);
                for (int k = 0; k <= i; i++)
                    delete [] matrix[k];
                delete [] matrix;
                sprintf(buf,
                    "read error or premature EOF, file %s.", fname);
                return (lstring::copy(buf));
            }
            matrix[i][j] = aux_scale*atof(tok);
            delete [] tok;
        }
        delete [] str;
    }
    fclose(fp);
    *size = matsz;
    *mret = matrix;
    *nameret = names;
    return (0);
}


// Fastcap post-processor, find the capacitance matrix and print the
// result.
//
void
fc_post_process(const char *fc_outfile)
{
    char buf[256];
    float **mat;
    int size;
    double units;
    char **names;
    char *err = fc_get_matrix(fc_outfile, &size, &mat, &units, &names);
    if (err) {
        fprintf(stderr,"Error: %s\n", err);
        delete [] err;
        return;
    }
    FILE *fp = stdout;
    fprintf(fp, "Output file: %s\n", fc_outfile);

    fprintf(fp, "\nSelf Capacitance (%sfarads):\n", scale_str(units));
    for (int i = 0; i < size; i++) {
        sprintf(buf, "C.%s", names[i]);
        fprintf(fp, " %-12s%.3g\n", buf, selfcap(i, mat, size));
    }
    if (size > 1) {
        fprintf(fp, "\nMutual Capacitance (%sfarads):\n", scale_str(units));
        for (int i = 0; i < size; i++) {
            for (int j = i+1; j < size; j++) {
                sprintf(buf, "C.%s.%s", names[i], names[j]);
                fprintf(fp, " %-12s%.3g\n", buf, mutcap(i, j, mat));
            }
        }
    }

    for (int i = 0; i < size; i++) {
        delete [] mat[i];
        delete [] names[i];
    }
    delete [] mat;
    delete [] names;
}


int
main(int argc, char **argv)
{
    if (argc != 2) {
        fprintf(stderr, "Usage: fcpp fastcap_outfile\n");
        return (1);
    }
    fc_post_process(argv[1]);
    return (0);
}

