
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

#include "spglobal.h"
#include "simulator.h"
#include "psffile.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "errors.h"
#include "spnumber/hash.h"
#include "ginterf/graphics.h"


// This is a stripped (WITH_PSFILE blocks removed) version with
// stubs only, and does not provide PSF file support.  This file
// format and its supporting libraries and interface is proprietary
// to Cadence Design Systems Inc.
// ----------------------------------------------------------------

//
// Support for generation of Cadence PSF output.  This requires
// libraries from the Cadence OASIS integration toolkit, and is
// enabled when WITH_PSFILE is defined.
//


cPSFout::cPSFout(sPlot *pl)
{
    ps_plot = pl;
    ps_dlist = 0;
    ps_dirname = 0;
    ps_numvars = 0;
    ps_length = 0;
    ps_numdims = 0;
    for (int i = 0; i < MAXDIMS; i++)
        ps_dims[i] = 0;
    ps_antype = PlAnNONE;
    if (pl) {
        if (lstring::ciprefix("tran", pl->type_name()))
            ps_antype = PlAnTRAN;
        else if (lstring::ciprefix("ac", pl->type_name()))
            ps_antype = PlAnAC;
        else if (lstring::ciprefix("op", pl->type_name()) ||
                lstring::ciprefix("dc", pl->type_name()))
            ps_antype = PlAnDC;
    }
    ps_complex = false;
    ps_binary = false;
}


cPSFout::~cPSFout()
{
    file_close();
}


bool
cPSFout::file_write(const char *dirname, bool append)
{
    (void)append;
    bool ascii = Global.AsciiOut();
    VTvalue vv;
    if (Sp.GetVar(kw_filetype, VTYP_STRING, &vv)) {
        if (lstring::cieq(vv.get_string(), "binary"))
            ascii = false;
        else if (lstring::cieq(vv.get_string(), "ascii"))
            ascii = true;
        else {
            GRpkg::self()->ErrPrintf(ET_WARN,
                "strange file type %s (ignored).\n", vv.get_string());
        }
    }

    if (!file_open(dirname, "w", !ascii))
        return (false);
    if (!file_head())
        return (false);
    if (!file_vars())
        return (false);
    if (!file_points())
        return (false);
    if (!file_close())
        return (false);
    return (true);
}


// Initialize, awf library opens file.
//
bool
cPSFout::file_open(const char *dirname, const char *mode, bool binary)
{
    (void)mode;

    file_close();
    ps_numvars = 0;
    ps_length = 0;
    ps_numdims = 0;
    for (int i = 0; i < MAXDIMS; i++)
        ps_dims[i] = 0;
    ps_complex = false;
    ps_binary = binary;

    (void)dirname;
    GRpkg::self()->ErrPrintf(ET_ERROR,
        "PSF format is not supportred in this binary.");
    return (false);
}


// Output the header part.
//
bool
cPSFout::file_head()
{
    if (!ps_plot)
        return (false);
    if (ps_antype == PlAnNONE)
        return (false);

    return (false);
}


// Output the vector names and characteristics, nothing to do here,
// but declare that the values are coming next.  We don't want this in
// file_points since file_points may be called more than once.
//
bool
cPSFout::file_vars()
{
    return (true);
}


// Output the data.
//
bool
cPSFout::file_points(int)
{
    // For batch mode, this is called for each output set during
    // simulation, the length was never set and may be unknown.
    if (ps_length == 0)
        ps_length = 1;

    return (false);
}


// Fill in the point count field., nothing to do here.
//
bool
cPSFout::file_update_pcnt(int)
{
    return (true);
}


// Close, nothing to do.
//
bool
cPSFout::file_close()
{
    sDvList::destroy(ps_dlist);
    ps_dlist = 0;
    delete [] ps_dirname;
    ps_dirname = 0;
    return (true);
}


// Static function.
// Call from main, performs some kind of init for the Cadence libraries.
//
void
cPSFout::vo_init(int *argcp, char *argv[])
{
    (void)argcp;
    (void)argv;
}


// Return non-null if str indicates to write output as psf.  The
// return is the name of the directory to used for output.  This is
// true for the form
//
// psf[@path]
//
// The "psf" is mandatory (case insensitive).  If just this, then
// output goes into a ./psf directory, created if necessary.  The
// "psf" can be followed by '@' and a directory path, in which case
// output will go to that directory, created if necessary.
//
const char *
cPSFout::is_psf(const char *str)
{
    if (!str)
        return (0);
    if (!lstring::ciprefix("psf", str))
        return (0);
    const char *e = str + 3;
    if (!*e)
        return (str);
    if (*e == '@') {
        e++;
        if (*e)
            return (e);
    }
    return (0);
}
// End of cPSFout functions.

