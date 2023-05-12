
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
#include "output.h"
#include "csdffile.h"
#include "cshell.h"
#include "keywords.h"
#include "circuit.h"
#include "ginterf/graphics.h"
#include <time.h>


// Read and write CSDF (Common Simulation Data File).  This format is
// used by HSPICE and other Synopsys products for plot-viewing,
// developed by ViewLogic (Mentor).

cCSDFout::cCSDFout(sPlot *pl)
{
    co_plot = pl;
    co_fp = 0;
    co_dlist = 0;
    co_length = 0;
    co_numvars = 0;
    co_realflag = false;
    co_no_close = false;
}


cCSDFout::~cCSDFout()
{
    file_close();
}


bool
cCSDFout::file_write(const char *filename, bool app)
{
    if (!file_open(filename, app ? "a" : "w", false))
        return (false);
    if (!file_head())
        return (false);
    if (!file_vars())
        return (false);
    if (!file_points())
        return (false);
    // end plot data
    fprintf(co_fp, "#;\n");
    if (!file_close())
        return (false);
    return (true);
}


// Return true if the file extension is a known CSDF extension.
//
bool
cCSDFout::is_csdf_ext(const char *fname)
{
    if (!fname)
        return (false);
    const char *extn = strrchr(fname, '.');
    if (!extn)
        return (false);
    if (extn == fname)
        return (false);
    extn++;
    if (lstring::cieq(extn, "csdf"))
        return (true);
    if (lstring::ciprefix("tr", extn) || lstring::ciprefix("ac", extn) ||
            lstring::ciprefix("sw", extn)) {
        const char *t = extn + 2;
        if (!*t)
            return (true);
        for ( ; *t; t++) {
            if (!isdigit(*t))
                return (false);
        }
        return (true);
    }
    return (false);
}


// Open the CSDF, return true if successful.
//
bool
cCSDFout::file_open(const char *filename, const char *mode, bool)
{
    file_close();
    FILE *fp = 0;
    if (filename && *filename) {
        if (!(fp = fopen(filename, mode))) {
            GRpkg::self()->Perror(filename);
            return (false);
        }
    }
    co_fp = fp;
    co_length = 0;
    co_numvars = 0;
    co_realflag = true;

    return (true);
}


// Output the header part of the CSDF file.
//
bool
cCSDFout::file_head()
{
    if (!co_plot) {
        GRpkg::self()->ErrPrintf(ET_INTERR,
            "null plot data pointer encountered.\n");
        return (false);
    }

    // Figure out the analysis type, bail if not supported.
    char anal[4];
    memset(anal, 0, 4);
    if (co_plot->type_name()) {
        anal[0] = co_plot->type_name()[0];
        anal[1] = co_plot->type_name()[1];
    }
    else if (co_plot->name()) {
        anal[0] = co_plot->name()[0];
        anal[1] = co_plot->name()[1];
        if (anal[1] == '.')
            anal[1] = co_plot->name()[2];
    }
    if (islower(anal[0]))
        anal[0] = toupper(anal[0]);
    if (islower(anal[1]))
        anal[1] = toupper(anal[1]);

    // There does not seem to be an "OP" analysis in CSDF, HSPICE does
    // not produce a CSDF file for this case.  Here, we'll fake it with
    // "DC".
    if (anal[0] == 'O' && anal[1] == 'P') {
        anal[0] = 'D';
        anal[1] = 'C';
    }
    if (strcmp(anal, "TR") && strcmp(anal, "AC") && strcmp(anal, "DC")) {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "plot data type not supported in CSDF format.\n");
        return (false);
    }

    fprintf(co_fp, "#H\n");
    fprintf(co_fp, "SOURCE=\'%s\' VERSION=\'%s\'\n", Global.Product(),
        Global.OSname());
    fprintf(co_fp, "TITLE=\'%s\'\n", co_plot->title());
    fprintf(co_fp, "SERIALNO=\'%s\'\n", Global.Version());

    time_t tloc;
    time(&tloc);
    struct tm *tm = localtime(&tloc);
    fprintf(co_fp, "TIME=\'%02d:%02d:%02d\' DATE=\'%02d/%02d/%4d\'\n",
        tm->tm_hour, tm->tm_min, tm->tm_sec,
        1 + tm->tm_mon, tm->tm_mday, 1900 + tm->tm_year);

    sDataVec *v = co_plot->find_vec("all");
    v->sort();
    co_dlist = v->link();
    v->set_link(0); // so list isn't freed in VecGc()

    // Make sure that the scale is the first in the list.
    //
    bool found_scale = false;
    sDvList *tl, *dl;
    for (tl = 0, dl = co_dlist; dl; tl = dl, dl = dl->dl_next) {
        if (dl->dl_dvec == co_plot->scale()) {
            if (tl) {
                tl->dl_next = dl->dl_next;
                dl->dl_next = co_dlist;
                co_dlist = dl;
            }
            found_scale = true;
            break;
        }
    }
    if (!found_scale && co_plot->scale()) {
        dl = new sDvList;
        dl->dl_next = co_dlist;
        dl->dl_dvec = co_plot->scale();
        co_dlist = dl;
    }

    double temp = DEF_temp - wrsCONSTCtoK;
    for (variable *vv = co_plot->environment(); vv; vv = vv->next()) {
        if (vv->type() == VTYP_REAL && lstring::cieq(vv->name(), "temp")) {
            temp = vv->real();
            break;
        }
    }
    fprintf(co_fp, "ANALYSIS=\'%s\' TEMPERATURE=\'%.5e\' SWEEPVAR=\'%s\'\n",
        anal, temp, co_dlist->dl_dvec->name());

    co_length = co_dlist->dl_dvec->length();
    int nvars = 0;
    for (dl = co_dlist; dl; dl = dl->dl_next) {
        v = dl->dl_dvec;
        if (v->iscomplex())
            co_realflag = false;
        if (v->length() != co_length)
            continue;
        nvars++;
    }
    co_numvars = nvars - 1;  // don't count scale

    if (co_realflag)
        fprintf(co_fp, "SWEEPMODE=\'%s\' COMPLEXVALUES=\'%s\' FORMAT=\'%s\'\n",
            "VAR_STEP", "NO", "1 VOLTSorAMPS;EFLOAT");
    else
        fprintf(co_fp, "SWEEPMODE=\'%s\' COMPLEXVALUES=\'%s\' FORMAT=\'%s\'\n",
            "LOG", "YES", "1 R_VOLTSorAMPS;EFLOAT/I_VOLTSorAMPS;EFLOAT");

    if (co_length > 0)
        // Is this really needed?  We don't bother with it unless the
        // vector length is known.
        fprintf(co_fp, "XBEGIN=\'%.5e\' XEND=\'%.5e\'\n",
            co_dlist->dl_dvec->realval(0),
            co_dlist->dl_dvec->realval(co_length-1));

    fprintf(co_fp, "NODES=\'%d\'\n", co_numvars);

    return (true);
}


// Output the vector names.
//
bool
cCSDFout::file_vars()
{
    if (!co_dlist || !co_dlist->dl_dvec)
        return (false);
    fprintf(co_fp, "#N");
    int sc_length = co_dlist->dl_dvec->length();
    int cols = 2;
    for (sDvList *dl = co_dlist->dl_next; dl; dl = dl->dl_next) {
        sDataVec *v = dl->dl_dvec;
        if (v->length() != sc_length)
            continue;

        cols += strlen(v->name()) + 3;
        if (cols > 80) {
            fprintf(co_fp, "\n");
            fprintf(co_fp, "\'%s\'", v->name());
            cols = strlen(v->name());
        }
        else
            fprintf(co_fp, " \'%s\'", v->name());
    }
    fprintf(co_fp, "\n");
    return (true);
}


// Output the data set.
//
bool
cCSDFout::file_points(int indx)
{
    if (!co_dlist || !co_dlist->dl_dvec)
        return (false);
    if (co_length == 0 && indx >= 0) {
        // True when called from output function the first time.
        co_length = 1;
    }

    for (int i = 0; i < co_length; i++) {
        sDataVec *v = co_dlist->dl_dvec;
        fprintf(co_fp, "#C %.5e %d", v->realval(i), co_numvars);
        int ncnt = 2;
        for (sDvList *dl = co_dlist->dl_next; dl; dl = dl->dl_next) {
            v = dl->dl_dvec;
            if (v->length() != co_length)
                continue;
            if (v && v->has_data()) {
                if (v->length() == co_length) {
                    if (co_realflag) {
                        fprintf(co_fp, " %12.5e", v->realval(i));
                        ncnt++;
                    }
                    else {
                        fprintf(co_fp, " %12.5e / %12.5e",
                            v->realval(i), v->imagval(i));
                        ncnt += 2;
                    }
                }
            }
            if (ncnt > 5) {
                fprintf(co_fp, "\n");
                ncnt = 0;
            }
        }
        if (ncnt != 0)
            fprintf(co_fp, "\n");
    }

    return (true);
}


bool
cCSDFout::file_update_pcnt(int)
{
    // end plot data
    fprintf(co_fp, "#;\n");
    // Here we could fill in XEND in the header if needed.
    return (true);
}


// Close the file.
//
bool
cCSDFout::file_close()
{
    sDvList::destroy(co_dlist);
    co_dlist = 0;
    if (co_fp && co_fp != stdout && !co_no_close)
        fclose(co_fp);
    co_fp = 0;
    return (true);
}
// End of cCSDFout functions.


cCSDFin::cCSDFin()
{
    ci_fp = 0;
    ci_header = 0;
    ci_names = 0;
    ci_length = 0;
    ci_numvars = 0;
}


cCSDFin::~cCSDFin()
{
    setvar_t::destroy(ci_header);
    setvar_t::destroy(ci_names);
    close_file();
}


// Read a CSDF file. Returns a list of plot structures.
//
sPlot *
cCSDFin::csdf_read(const char *name)
{
    ci_fp = Sp.PathOpen(name, "rb");
    if (!ci_fp) {
        GRpkg::self()->Perror(name);
        return (0);
    }

    OP.pushPlot();
    TTY.ioPush();
    CP.PushControl();

    sPlot *p, *plots = 0, *endpl = 0;
    while ((p = parse_plot()) != 0) {
        if (endpl) {
            endpl->set_next_plot(p);
            endpl = endpl->next_plot();
        }
        else
            plots = endpl = p;
    }
    close_file();

    CP.PopControl();
    TTY.ioPop();
    OP.popPlot();

    // Make the vectors permanent, and fix dimension.
    for (p = plots; p; p = p->next_plot()) {
        sDataVec *v, *nv;
        for (v = p->tempvecs(); v; v = nv) {
            nv = v->next();
            v->set_next(0);
            v->newperm(p);
            if (v->numdims() == 1)
                v->set_dims(0, v->length());
        }
        p->set_tempvecs(0);
    }
    return (plots);
}


namespace {
    int
    nametype(const char *s)
    {
        int t = UU_NOTYPE;
        if (!s)
            return (t);
        char c = islower(*s) ? toupper(*s) : *s;
        if (c == 'V')
            t = UU_VOLTAGE;
        else if (c == 'I')
            t = UU_CURRENT;
        else if (c == 'T')
            t = UU_TIME;
        else if (c == 'H' || c == 'F')
            t = UU_FREQUENCY;
        return (t);
    }
}


sPlot *
cCSDFin::parse_plot()
{
    char *t;
    while ((t = fgets(ci_buffer, CSDF_BSIZE,  ci_fp)) != 0) {
        while (isspace(*t))
            t++;
        if (*t)
            break;
    }
    if (!t || !*t)
        return (0);
    if (t[0] == '#' && t[1] == ';')
        return (0);

    if (t[0] != '#' || t[1] != 'H') {
        GRpkg::self()->ErrPrintf(ET_ERROR, "incorrect file format.\n");
        return (0);
    }

    if (!parse_header())
        return (0);

    char *title = 0;
    for (setvar_t *s = ci_header; s; s = s->sv_next) {
        if (lstring::cieq(s->sv_name, "title")) {
            title = lstring::copy(s->sv_value);
            break;
        }
    }
    if (!title)
        title = lstring::copy("untitled");

    char *time = 0;
    for (setvar_t *s = ci_header; s; s = s->sv_next) {
        if (lstring::cieq(s->sv_name, "time")) {
            time = lstring::copy(s->sv_value);
            break;
        }
    }

    char *date = 0;
    for (setvar_t *s = ci_header; s; s = s->sv_next) {
        if (lstring::cieq(s->sv_name, "date")) {
            date = lstring::copy(s->sv_value);
            break;
        }
    }
    if (date && time) {
        t = new char[strlen(date) + strlen(time) + 2];
        sprintf(t, "%s %s", date, time);
        delete [] date;
        delete [] time;
        date = t;
        time = 0;
    }
    else if (!date) {
        date = lstring::copy(datestring());
        delete [] time;
    }

    char *analysis = 0;
    for (setvar_t *s = ci_header; s; s = s->sv_next) {
        if (lstring::cieq(s->sv_name, "analysis")) {
            analysis = lstring::copy(s->sv_value);
            break;
        }
    }

    char *sweepvar = 0;
    for (setvar_t *s = ci_header; s; s = s->sv_next) {
        if (lstring::cieq(s->sv_name, "sweepvar")) {
            sweepvar = lstring::copy(s->sv_value);
            break;
        }
    }

    bool log_scale = false;
    for (setvar_t *s = ci_header; s; s = s->sv_next) {
        if (lstring::cieq(s->sv_name, "sweepmode")) {
            if (lstring::cieq(s->sv_value, "log"))
                log_scale = true;
            break;
        }
    }

    bool is_complex = false;
    for (setvar_t *s = ci_header; s; s = s->sv_next) {
        if (lstring::cieq(s->sv_name, "complexvalues")) {
            if (lstring::cieq(s->sv_value, "yes"))
                is_complex = true;
            break;
        }
    }

    if (!sweepvar)
        return (0);

    sPlot *p = new sPlot(0);
    p->set_title(title);
    p->set_date(date);
    delete [] title;
    delete [] date;

    if (!analysis)
        p->set_name("CSDF import");
    else {
        if (lstring::cieq(analysis, "tr")) {
            delete [] analysis;
            analysis = lstring::copy("TRAN");
        }
        t = new char[strlen(analysis) + 15];
        sprintf(t, "CSDF %s import", analysis);
        p->set_name(t);
        delete [] t;
        delete [] analysis;
    }

    sDataVec *vsc = new sDataVec(nametype(sweepvar));
    p->set_scale(vsc);
    p->set_tempvecs(vsc);
    if (log_scale)
        vsc->set_gridtype(GRID_XLOG);
    vsc->set_plot(p);
    vsc->set_name(sweepvar);
    delete [] sweepvar;

    sDataVec *vend = vsc;
    for (setvar_t *s = ci_names; s; s = s->sv_next) {
        sDataVec *v = new sDataVec(nametype(s->sv_name));
        v->set_plot(p);
        v->set_name(s->sv_name);
        vend->set_next(v);
        vend = vend->next();
    }

    for (sDataVec *v = p->tempvecs(); v; v = v->next()) {
        v->set_length(ci_length);
        v->set_allocated(ci_length);
        v->set_numdims(1);
        if (is_complex)
            v->set_compvec(new complex[ci_length]);
        else
            v->set_realvec(new double[ci_length]);
    }

    // Read the trace data.
    for (int i = 0; i < ci_length; i++) {
        if (!parse_rec(p, i)) {
            delete p;
            return (0);
        }
    }
    return (p);
}


bool
cCSDFin::parse_header()
{
    setvar_t::destroy(ci_header);
    ci_header = 0;
    setvar_t::destroy(ci_names);
    ci_names = 0;
    ci_length = 0;
    ci_numvars = 0;

    // First parse the header defines, end on name list.
    setvar_t *end = 0;
    char *s;
    while ((s = fgets(ci_buffer, CSDF_BSIZE,  ci_fp)) != 0) {
        while (isspace(*s))
            s++;
        char *t = s + strlen(s) - 1;
        while (t >= s && isspace(*t))
            *t-- = 0;
        if (*s == '#')
            break;
        setvar_t *h;
        while ((h = parse_set(&s)) != 0) {
            if (end) {
                end->sv_next = h;
                end = end->sv_next;
            }
            else
                ci_header = end = h;
        }
    }
    if (!s || s[1] != 'N') {
        GRpkg::self()->ErrPrintf(ET_ERROR,
            "file syntax error, name list not found.\n");
        return (false);
    }

    // Parse the names in the name list.
    s += 2;
    end = 0;
    for (;;) {
        char *tok = lstring::getqtok(&s);
        if (tok) {
            if (tok[0] =='V' && tok[1] == '(') {
                // Lower-case the 'V', as is standard in WRspice and
                // required for proper operation in the plot hash table.
                tok[0] = 'v';
            }
            if ((tok[0] == 'I' || tok[0] == 'i') && tok[1] == '(') {
                // Convert I(xxx) to xxx#branch.
                char *t0 = new char[strlen(tok) + 5];
                char *t = t0, *n = tok+2;
                while (*n && *n != ')')
                    *t++ = *n++;
                strcpy(t, "#branch");
                delete [] tok;
                tok = t0;
            }
            if (end) {
                end->sv_next = new setvar_t(tok, 0);
                end = end->sv_next;
            }
            else
                ci_names = end = new setvar_t(tok, 0);
        }
        else {
            s = fgets(ci_buffer, CSDF_BSIZE,  ci_fp);
            if (!s) {
                GRpkg::self()->ErrPrintf(ET_ERROR, "premature end of file.\n");
                return (false);
            }
            while (isspace(*s))
                s++;
            if (*s == '#')
                break;
        }
    }

    // We have read the first line of the first #C record.

    // Count the number of #C records in the file, which is the length
    // of all vectors.
    //
    char tbuf[CSDF_BSIZE];
    unsigned long fpos = ftell(ci_fp);
    int cnt = 1;
    while ((s = fgets(tbuf, CSDF_BSIZE, ci_fp)) != 0) {
        while (isspace(*s))
            s++;
        if (s[0] == '#') {
            if (s[1] == 'C')
                cnt++;
            else
                break;
        }
    }
    ci_length = cnt;
    fseek(ci_fp, fpos, SEEK_SET);

    return (true);
}


// Parse a name=value construct and advance pointer.
//
setvar_t *
cCSDFin::parse_set(char **strp)
{
    char *tok1 = lstring::gettok(strp, "=");
    if (!tok1)
        return (0);
    char *tok2 = lstring::getqtok(strp);
    setvar_t *h = new setvar_t(tok1, tok2);
    return (h);
}


namespace {
    inline bool get_float(char **strp, double *d, bool cplx = false)
    {
        char *tok = lstring::gettok(strp, cplx ? "/" : 0);
        bool ret =  (tok && sscanf(tok, "%lf", d) == 1);
        delete [] tok;
        return (ret);
    }
}


bool
cCSDFin::parse_rec(sPlot *pl, int ix)
{
    char *s;
    for (;;) {
        s = ci_buffer;
        while (isspace(*s))
            s++;
        if (s[0] != '#' || s[1] != 'C') {
            if (!*s) {
                // empty line
                if (!fgets(ci_buffer, CSDF_BSIZE, ci_fp)) {
                    GRpkg::self()->ErrPrintf(ET_ERROR,
                        "premature end of file.\n");
                    return (false);
                }
                continue;
            }
            GRpkg::self()->ErrPrintf(ET_ERROR,
                "file syntax error, missing data record.\n");
            return (false);
        }
        break;
    }
    s += 3;

    const char *msg = "syntax error in data record.\n";

    // First, the scale value.
    double d;
    if (!get_float(&s, &d)) {
        GRpkg::self()->ErrPrintf(ET_ERROR, msg);
        return (false);
    }
    sDataVec *v = pl->scale();
    if (v->iscomplex()) {
        v->set_realval(ix, d);
        v->set_imagval(ix, 0.0);
    }
    else
        v->set_realval(ix, d);

    // Skip over the number of values.
    lstring::advtok(&s);

    // Get the vector data.
    if (v->iscomplex()) {
        for (v = pl->tempvecs()->next(); v; v = v->next()) {
            if (!get_float(&s, &d, true)) {
                // line ended
                s = fgets(ci_buffer, CSDF_BSIZE, ci_fp);
                if (!s) {
                    GRpkg::self()->ErrPrintf(ET_ERROR, msg);
                    return (false);
                }
                while (isspace(*s))
                    s++;
                if (!get_float(&s, &d, true)) {
                    GRpkg::self()->ErrPrintf(ET_ERROR, msg);
                    return (false);
                }
            }
            v->set_realval(ix, d);
            if (!get_float(&s, &d, true)) {
                // line ended
                s = fgets(ci_buffer, CSDF_BSIZE, ci_fp);
                if (!s) {
                    GRpkg::self()->ErrPrintf(ET_ERROR, msg);
                    return (false);
                }
                while (isspace(*s))
                    s++;
                if (!get_float(&s, &d, true)) {
                    GRpkg::self()->ErrPrintf(ET_ERROR, msg);
                    return (false);
                }
            }
            v->set_imagval(ix, d);
        }
    }
    else {
        for (v = pl->tempvecs()->next(); v; v = v->next()) {
            if (!get_float(&s, &d)) {
                // line ended
                s = fgets(ci_buffer, CSDF_BSIZE, ci_fp);
                if (!s) {
                    GRpkg::self()->ErrPrintf(ET_ERROR, msg);
                    return (false);
                }
                while (isspace(*s))
                    s++;
                if (!get_float(&s, &d)) {
                    GRpkg::self()->ErrPrintf(ET_ERROR, msg);
                    return (false);
                }
            }
            v->set_realval(ix, d);
        }
    }

    // Read the first line of the next record;
    if (!fgets(ci_buffer, CSDF_BSIZE, ci_fp)) {
        GRpkg::self()->ErrPrintf(ET_ERROR, "premature end of file.\n");
        return (false);
    }
    return (true);
}


// Close the file.
//
void
cCSDFin::close_file()
{
    if (ci_fp && ci_fp != stdin)
        fclose(ci_fp);
    ci_fp = 0;
}
// End of cCSDFin functions.

