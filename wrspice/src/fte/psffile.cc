
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
 $Id: psffile.cc,v 2.8 2016/05/10 20:01:16 stevew Exp $
 *========================================================================*/

#include "spglobal.h"
#include "frontend.h"
#include "psffile.h"
#include "cshell.h"
#include "kwords_fte.h"
#include "errors.h"
#include "hash.h"
#include "graphics.h"

#ifdef WITH_PSFILE
#include "vo.h"
#include "awf.h"

// The Cadence libraries need these.
char VersionIdent[100];
char SubVersion[100];
#endif

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

#ifdef WITH_PSFILE
    sprintf(VersionIdent, "1.0");
    sprintf(SubVersion, "1.0");
#endif
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
        else
            GRpkgIf()->ErrPrintf(ET_WARN, "strange file type %s (ignored).\n",
                vv.get_string());
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

#ifdef WITH_PSFILE
    // Set the output directory and text/binary output format. 
    // Warning, the first argument of awfSetPsfDir is not copied
    // internally so it can't be freed until we're done.

    ps_dirname = lstring::copy(dirname);
    awfSetPsfDir(ps_dirname, !binary);
    return (true);
#else
    (void)dirname;
    GRpkgIf()->ErrPrintf(ET_ERROR,
        "PSF format is not supportred in this binary.");
    return (false);
#endif
}


#ifdef WITH_PSFILE
namespace {
    // Return the appropriate node or branch name.
    //
    char *vec_name(sDataVec *v, bool *is_branch)
    {
        if (is_branch)
            *is_branch = false;
        sUnits Ubr, Uv;
        Ubr.set(UU_CURRENT);
        Uv.set(UU_VOLTAGE);
        if (*v->units() == Ubr) {
            // Keep only names with a '#branch' suffix, strip this.

            char *nm = lstring::copy(v->name());
            char *e = strrchr(nm, '#');
            if (e && lstring::cieq(e+1, "branch")) {
                *e = 0;
                *is_branch = true;
                return (nm);
            }
            delete [] nm;
        }
        else if (*v->units() == Uv) {
            // Keep only names enclosed in v( ), but strip this.

            char *nm = lstring::copy(v->name());
            const char *t = nm;
            if ((*t == 'v' || *t == 'V') && *(t+1) == '(') {
                t += 2;
                char *e = nm + strlen(nm) - 1;
                if (*e == ')')
                    *e = 0;
                e = nm;
                while ((*e++ = *t++) != 0) ;
                return (nm);
            }
            delete [] nm;
        }
        return (0);
    }
}
#endif


// Output the header part.
//
bool
cPSFout::file_head()
{
    if (!ps_plot)
        return (false);
    if (ps_antype == PlAnNONE)
        return (false);

#ifdef WITH_PSFILE
    sDataVec *v = ps_plot->find_vec("all");
    v->sort();
    ps_dlist = v->link();
    v->set_link(0); // so list isn't freed in VecGc()

    // Make sure that the scale is the first in the list.
    //
    bool found_scale = false;
    sDvList *tl, *dl;
    for (tl = 0, dl = ps_dlist; dl; tl = dl, dl = dl->dl_next) {
        if (dl->dl_dvec == ps_plot->scale()) {
            if (tl) {
                tl->dl_next = dl->dl_next;
                dl->dl_next = ps_dlist;
                ps_dlist = dl;
            }
            found_scale = true;
            break;
        }
    }
    if (!found_scale && ps_plot->scale()) {
        dl = new sDvList;
        dl->dl_next = ps_dlist;
        dl->dl_dvec = ps_plot->scale();
        ps_dlist = dl;
    }

    v = ps_dlist->dl_dvec;  // the scale
    if (v->numdims() <= 1) {
        v->set_numdims(1);
        v->set_dims(0, v->length());
    }
    ps_length = v->length();
    ps_numdims = v->numdims();
    for (int j = 0; j < ps_numdims; j++)
        ps_dims[j] = v->dims(j);

    int nvars = 0;
    sDvList *dp = ps_dlist, *dn;
    for (dl = dp->dl_next; dl; dl = dn) {
        dn = dl->dl_next;
        v = dl->dl_dvec;
        bool is_branch;
        char *nm = vec_name(dl->dl_dvec, &is_branch);
        if (!nm) {
            // Oddball vector, such as a measurement result, that
            // ADE won't know about.  Rid it.
            dp->dl_next = dn;
            dl->dl_next = 0;
            delete dl;
            continue;
        }
        delete [] nm;
        dp = dl;
        nvars++;
        if (v->iscomplex())
            ps_complex = true;

        // Be paranoid and assume somewhere we may have
        // forgotten to set the dimensions of 1-D vectors.
        //
        if (v->numdims() <= 1) {
            v->set_numdims(1);
            v->set_dims(0, v->length());
        }
    }
    ps_numvars = nvars;

    if (ps_antype == PlAnTRAN) {
        // Initialize for nvars "nodes".
        awfTrHead(nvars);

        awfTrSetFlushSkipSweep(-1); // pslTranSemantic flushes every 512
        awfTrSetFlushTimePeriod(-1); // don't let awf flush per some timePeriod
        // This would enhance and increase the speed of the binary PSF.
        // Use only if you see that binary PSF is getting slower than textual.

        for (dl = ps_dlist->dl_next; dl; dl = dl->dl_next) {
            bool is_branch;
            char *nm = vec_name(dl->dl_dvec, &is_branch);
            if (is_branch)
                awfTrAddBranch(nm);
            else
                awfTrAddNode(nm);
            delete [] nm;
        }
    }
    else if (ps_antype == PlAnDC) {
        // Initialize for nvars "nodes", and sweep name.  It appears that
        // only one swept value is accepted?

        char *snm = lstring::copy(ps_dlist->dl_dvec->name());
        // Strip the "#sweep".
        char *e = strchr(snm, '#');
        if (e)
            *e = 0;
        awfDcHead(nvars, snm);
        delete [] snm;

        for (dl = ps_dlist->dl_next; dl; dl = dl->dl_next) {
            bool is_branch;
            char *nm = vec_name(dl->dl_dvec, &is_branch);
            if (is_branch)
                awfDcAddBranch(nm);
            else
                awfDcAddNode(nm);
            delete [] nm;
        }
    }
    else if (ps_antype == PlAnAC) {
        // Initialize for nvars "nodes".
        awfAcHead(nvars);

        for (dl = ps_dlist->dl_next; dl; dl = dl->dl_next) {
            bool is_branch;
            char *nm = vec_name(dl->dl_dvec, &is_branch);
            if (is_branch)
                awfAcAddBranch(nm);
            else
                awfAcAddNode(nm);
            delete [] nm;
        }
    }

    return (true);
#else
    return (false);
#endif
}


// Output the vector names and characteristics, nothing to do here,
// but declare that the values are coming next.  We don't want this in
// file_points since file_points may be called more than once.
//
bool
cPSFout::file_vars()
{
#ifdef WITH_PSFILE
    if (ps_antype == PlAnTRAN)
        awfTrValue();
    else if (ps_antype == PlAnDC)
        awfDcValue();
    else if (ps_antype == PlAnAC)
        awfAcValue();
#endif
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

#ifdef WITH_PSFILE
    if (ps_antype == PlAnTRAN) {
        double *vals = new double[ps_numvars];

        for (int i = 0; i < ps_length; i++) {
            double t = ps_dlist->dl_dvec->realval(i);
            awfTrSweep((void*)&t);

            int vnum = 0;
            for (sDvList *dl = ps_dlist->dl_next; dl; dl = dl->dl_next) {
                sDataVec *v = dl->dl_dvec;
                if (i < v->length())
                    vals[vnum++] = v->realval(i);
                else
                    vals[vnum++] = 0.0;
            }
            awfTrTrace(vals, ps_numvars);
            awfTrGroup();
        }
        delete [] vals;
    }
    else if (ps_antype == PlAnDC) {
        double *vals = new double[ps_numvars];

        for (int i = 0; i < ps_length; i++) {
            double t = ps_dlist->dl_dvec->realval(i);
            awfDcSweep((void*)&t);

            int vnum = 0;
            for (sDvList *dl = ps_dlist->dl_next; dl; dl = dl->dl_next) {
                sDataVec *v = dl->dl_dvec;
                if (i < v->length())
                    vals[vnum++] = v->realval(i);
                else
                    vals[vnum++] = 0.0;
            }
            awfDcTrace(vals, ps_numvars);
        }
        delete [] vals;
    }
    else if (ps_antype == PlAnAC) {
        double *vals = new double[2];

        for (int i = 0; i < ps_length; i++) {
            double f = ps_dlist->dl_dvec->realval(i);
            awfAcSweep((void*)&f);

            for (sDvList *dl = ps_dlist->dl_next; dl; dl = dl->dl_next) {
                sDataVec *v = dl->dl_dvec;
                if (i < v->length()) {
                    vals[0] = v->realval(i);
                    vals[1] = v->imagval(i);
                }
                else {
                    vals[0] = 0.0;
                    vals[1] = 0.0;
                }
                awfAcTrace(vals);
            }
        }
        delete [] vals;
    }

    return (true);
#else
    return (false);
#endif
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
#ifdef WITH_PSFILE
    if (ps_antype == PlAnTRAN)
        awfTrTail();
    else if (ps_antype == PlAnDC)
        awfDcTail();
    else if (ps_antype == PlAnAC)
        awfAcTail();
#endif

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
#ifdef WITH_PSFILE
    voInit(argcp, argv);
#else
    (void)argcp;
    (void)argv;
#endif
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

