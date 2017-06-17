
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
 *========================================================================*
 *                                                                        *
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: rawfile.h,v 2.4 2015/08/25 21:26:11 stevew Exp $
 *========================================================================*/

#ifndef RAWFILE_H
#define RAWFILE_H

#include "ftedata.h"


//
// Read and write the ascii and binary rawfile formats.
//

class cRawOut : public cFileOut
{
public:
    cRawOut(sPlot*);
    ~cRawOut();


    // virtual overrides
    bool file_write(const char*, bool);
    bool file_open(const char*, const char*, bool);
    void file_set_fp(FILE *fp)
        {
            ro_fp = fp;
            ro_no_close = true;
        }
    bool file_head();
    bool file_vars();
    bool file_points(int = -1);
    bool file_update_pcnt(int);
    bool file_close();

private:
    sPlot *ro_plot;
    FILE *ro_fp;
    long ro_pointPosn;
    int ro_prec;
    int ro_numdims;
    int ro_length;
    int ro_dims[MAXDIMS];
    sDvList *ro_dlist;
    bool ro_realflag;
    bool ro_binary;
    bool ro_pad;
    bool ro_no_close;
};

class cRawIn
{
public:
    cRawIn();
    ~cRawIn();

    sPlot *raw_read(const char*);

private:
    void read_data(bool, sPlot*);

    static void add_point(sDataVec*, double*, double*);
    static void fixdims(sDataVec*, const char*);
    static int atodims(const char*, int*, int*);

    FILE *ri_fp;
};

#endif

