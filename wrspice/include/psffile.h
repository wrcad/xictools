
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
 *========================================================================*
 *                                                                        *
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: psffile.h,v 2.2 2015/08/26 04:13:03 stevew Exp $
 *========================================================================*/

#ifndef PSFILE_H
#define PSFILE_H

#include "ftedata.h"


//
// Write Cadence PSF format simulation output.
//

class cPSFout : public cFileOut
{
public:
    enum PlAnType { PlAnNONE, PlAnDC, PlAnAC, PlAnTRAN };

    cPSFout(sPlot*);
    ~cPSFout();

    // virtual overrides
    bool file_write(const char*, bool);
    bool file_open(const char*, const char*, bool);
    void file_set_fp(FILE*) { }
    bool file_head();
    bool file_vars();
    bool file_points(int = -1);
    bool file_update_pcnt(int);
    bool file_close();

    static void vo_init(int*, char*[]);
    static const char *is_psf(const char*);

private:
    sPlot *ps_plot;
    sDvList *ps_dlist;
    char *ps_dirname;
    int ps_numvars;
    int ps_length;
    int ps_numdims;
    int ps_dims[MAXDIMS];
    PlAnType ps_antype;
    bool ps_complex;
    bool ps_binary;
};

#endif

