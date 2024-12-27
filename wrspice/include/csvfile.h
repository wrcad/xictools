
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

#ifndef CSVFILE_H
#define CSVFILE_H

#include "datavec.h"


//
// Read and write comma-separated variable (CSV) formats.
//

class cCSVout : public cFileOut
{
public:
    enum CMTLEV { CMTLEV_none, CMTLEV_names, CMTLEV_all };
    cCSVout(sPlot*);
    ~cCSVout();

    static bool is_csv_ext(const char*);

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
    long co_pointPosn;
    int co_prec;
    int co_numdims;
    int co_length;
    int co_dims[MAXDIMS];
    sDvList *co_dlist;
    bool co_realflag;
    bool co_pad;
    bool co_no_close;
    bool co_nmsmpl;
    char co_cmtchar;
    char co_cmtlev;
};

class cCSVin
{
public:
    cCSVin();
    ~cCSVin();

    sPlot *csv_read(const char*);

private:
    FILE *ci_fp;
    char *ci_title;
    char *ci_date;
    sPlot *ci_plots;
    int ci_flags;
    int ci_nvars;
    int ci_npoints;
    int ci_numdims;
    int ci_dims[MAXDIMS];
    bool ci_padded;
};

#endif

