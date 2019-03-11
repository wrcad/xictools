
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef CSDFFILE_H
#define CSDFFILE_H

#include "datavec.h"

// Read and write CSDF (Common Simulation Data File).  This format is
// used by HSPICE and other Synopsys products for plot-viewing,
// developed by ViewLogic (Mentor).

// CSDF file writer.
//
class cCSDFout : public cFileOut
{
public:
    cCSDFout(sPlot*);
    ~cCSDFout();

    static bool is_csdf_ext(const char*);

    // virtual overrides
    bool file_write(const char*, bool);
    bool file_open(const char*, const char*, bool);
    void file_set_fp(FILE *fp)
        {
            co_fp = fp;
            co_no_close = true;
        }
    bool file_head();
    bool file_vars();
    bool file_points(int = -1);
    bool file_update_pcnt(int);
    bool file_close();

private:
    sPlot *co_plot;
    FILE *co_fp;
    sDvList *co_dlist;
    int co_length;
    int co_numvars;
    bool co_realflag;
    bool co_no_close;
};

// A local name/value string list.
//
struct setvar_t
{
    setvar_t(char *n, char *v)
        {
            sv_name = n;
            sv_value = v;
            sv_next = 0;
        }

    ~setvar_t()
        {
            delete [] sv_name;
            delete [] sv_value;
        }

    static void destroy(setvar_t *s)
        {
            while (s) {
                setvar_t *x = s;
                s = s->sv_next;
                delete x;
            }
        }

    char *sv_name;
    char *sv_value;
    setvar_t *sv_next;
};

#define CSDF_BSIZE 256

// CSDF file reader.
//
class cCSDFin
{
public:
    cCSDFin();
    ~cCSDFin();

    sPlot *csdf_read(const char*);

private:
    sPlot *parse_plot();
    bool parse_header();
    setvar_t *parse_set(char**);
    bool parse_rec(sPlot*, int);
    void close_file();

    FILE *ci_fp;
    setvar_t *ci_header;
    setvar_t *ci_names;
    int ci_length;
    int ci_numvars;
    char ci_buffer[CSDF_BSIZE];
};

#endif

