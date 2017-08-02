
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "fio.h"
#include "fio_gdsii.h"
#include "cd_propnum.h"
#include <ctype.h>


//--------------------------------------------------------------------------
// Functions for Text to GDS conversion
//--------------------------------------------------------------------------

namespace {
    // Struct to save GDSII record values.
    //
    struct gds_rec
    {
        gds_rec() { reset(); }

        void reset()
            {
                magn = 1.0;
                angle = 0.0;
                string = 0;
                width = 0;
                cols = rows = 0;
                hj = vj = 0;
                font = 0;
                pathtype = 1;
                plexnum = 0;
                numpts = 0;
                bgnextn = 0;
                endextn = 0;
                reflection = false;
                abs_angle = false;
                abs_mag = false;
                hadplex = false;
            }

        double magn;
        double angle;
        char *string;
        int width;
        int cols, rows;
        int hj, vj;
        int font;
        int pathtype;
        int plexnum;
        int numpts;
        int bgnextn;
        int endextn;
        bool reflection;
        bool abs_angle;
        bool abs_mag;
        bool hadplex;
    };

    // The converter, basically just a GDSII back-end with some input
    // methods.
    //
    struct gdstx_in : public gds_out
    {
        gdstx_in(FILE *tp)
            {
                out_textfp = tp;
                out_linecnt = 0;
            }

        bool read_lib(DisplayMode);
        bool read_structure(const char*);
        bool read_record(gds_rec*);
        bool getline(char**, char**);

        FILE        *out_textfp;                 // text input file pointer
        int         out_linecnt;                 // lines read
        char        out_linebuf[1024];           // buffer for reading
    };
}


// Convert the ascii representation in txtname to a stream file.
// Return false on error, with a diagnostic message in Errs.
//
bool
cFIO::GdsFromText(const char *txtname, const char *gds_fname)
{
    char buf[1024];
    FILE *txtfp = POpen(txtname, "rb");
    if (txtfp == 0) {
        Errs()->sys_error("open");
        Errs()->add_error("Can't open text file %s.", txtname);
        return (false);
    }

    gdstx_in *gds = new gdstx_in(txtfp);
    if (!gds->set_destination(gds_fname)) {
        fclose(txtfp);
        return (false);
    }

    if (!gds->read_lib(Physical)) {
        fclose(txtfp);
        unlink(gds_fname);
        delete gds;
        return (false);
    }
    if (fgets(buf, 1024, txtfp) != 0) {
        // see if there are electrical records
        if (!strncmp(buf, ">> Begin", 8)) {
            if (!gds->read_lib(Electrical)) {
                fclose(txtfp);
                unlink(gds_fname);
                delete gds;
                return (false);
            }
        }
    }
    fclose(txtfp);
    delete gds;
    return (true);
}
// End of cCD functions.


bool
gdstx_in::read_lib(DisplayMode mode)
{
    out_mode = mode;
    double munit, uunit;
    if (out_mode == Physical) {
        munit = 1e-6/CDphysResolution;
        uunit = 1.0/CDphysResolution;
    }
    else {
        munit = 1e-6/CDelecResolution;
        uunit = 1.0/CDelecResolution;
    }
    char strname[256];
    bool lib_started = false;
    char *token, *line;
    while (getline(&token, &line)) {

        if (!strcmp(token, "LIBRARY")) {
            if (sscanf(line, "%s", out_libname) != 1)
                goto bad;
            if (!getline(&token, &line))   // MODIFICATION
                break;
            if (!getline(&token, &line))   // ACCESS
                break;
        }
        else if (!strcmp(token, "U-UNIT")) {
            if (sscanf(line, "%lf", &uunit) != 1)
                goto bad;
            if (!getline(&token, &line))
                break;
            if (sscanf(line, "%lf", &munit) != 1)
                goto bad;
        }
        else if (!strcmp(token, "REFLIB")) {
            if (sscanf(line, "%s", out_lib1) != 1)
                goto bad;
            if (!getline(&token, &line))
                break;
            if (sscanf(line, "%s", out_lib2) != 1)
                goto bad;
        }
        else if (!strcmp(token, "FONT0")) {
            if (*line && sscanf(line, "%s", out_font0) != 1)
                goto bad;
            if (!getline(&token, &line))
                break;
            if (*line && sscanf(line, "%s", out_font1) != 1)
                goto bad;
            if (!getline(&token, &line))
                break;
            if (*line && sscanf(line, "%s", out_font2) != 1)
                goto bad;
            if (!getline(&token, &line))
                break;
            if (*line && sscanf(line, "%s", out_font3) != 1)
                goto bad;
        }
        else if (!strcmp(token, "GENERATIONS")) {
            if (sscanf(line, "%d", &out_generation) != 1)
                goto bad;
        }
        else if (!strcmp(token, "ATTRIBUTE-FILE")) {
            if (sscanf(line, "%s", out_attr) != 1)
                goto bad;
        }
        else if (!strcmp(token, "STRUCTURE")) {
            if (sscanf(line, "%s", strname) != 1)
                goto bad;
            if (!lib_started) {
                if (!write_library(out_version, munit, uunit, &out_date,
                        &out_date, 0))
                    return (false);
                lib_started = true;
            }
            if (!read_structure(strname))
                return (false);
        }
        else if (!strcmp(token, "ATTRIBUTES")) {
            // non-standard - structure property
            int val;
            if (sscanf(line, "%d", &val) != 1)
                goto bad;
            while (isdigit(*line))
                line++;
            line++;  // should be '='

            // have to handle multiple lines, up to ';'
            sLstr lstr;
            for (;;) {
                lstr.add(line);
                const char *e = lstr.string() + lstr.length() - 1;
                while (isspace(*e) && e > lstr.string())
                    e--;
                if (e >= lstr.string() && *e == ';') {
                    lstr.truncate(e - lstr.string(), 0);
                    break;
                }
                if ((line = fgets(out_linebuf, 1024, out_textfp)) == 0)
                    break;
            }
            if (!queue_property(val, lstr.string()))
                return (false);
        }
        else if (!strcmp(token, "END")) {
            if (!lib_started) {
                if (!write_library(out_version, munit, uunit, &out_date,
                        &out_date, 0))
                    return (false);
                lib_started = true;

            }
            return (write_endlib(0));
        }
    }
    Errs()->add_error("premature EOF.");
    return (false);
bad:
    Errs()->add_error("syntax error, line %d.", out_linecnt);
    return (false);
}


namespace {
    // Convert a pathtype 4 endpoint to pathtype 0.
    // int *xe, *ye;     coordinate of endpoint
    // int xb, yb;       coordinate of previous or next point in path
    // int extn;         specified extension
    //
    void
    convert_4to0(int *xe, int *ye, int xb, int yb, int extn)
    {
        if (!extn)
            return;
        if (*xe == xb) {
            if (*ye > yb)
                *ye += extn;
            else
                *ye -= extn;
        }
        else if (*ye == yb) {
            if (*xe > xb)
                *xe += extn;
            else
                *xe -= extn;
        }
        else {
            double dx = (double)(*xe - xb);
            double dy = (double)(*ye - yb);
            double d = sqrt(dx*dx + dy*dy);
            d = extn/d;
            *ye += mmRnd(dy*d);
            *xe += mmRnd(dx*d);
        }
    }
}


bool
gdstx_in::read_structure(const char *strname)
{
    bool ret = write_struct(strname, &out_date, &out_date);
    clear_property_queue();
    if (!ret)
        return (false);

    char instname[256];
    char *token, *line;
    gds_rec r;
    while (getline(&token, &line)) {

        r.reset();
        if (!strcmp(token, "BOUNDARY")) {
            if (!read_record(&r))
                return (false);
            Poly po;
            po.points = out_xy;
            po.numpts = r.numpts;
            ret = write_poly(&po);
            clear_property_queue();
            if (!ret)
                return (false);
        }
        else if (!strcmp(token, "PATH")) {
            if (!read_record(&r))
                return (false);
            if (r.pathtype == 4) {
                convert_4to0(&out_xy[0].x, &out_xy[0].y, out_xy[1].x,
                    out_xy[1].y, r.bgnextn);
                convert_4to0(&out_xy[r.numpts-1].x, &out_xy[r.numpts-1].y,
                    out_xy[r.numpts-2].x, out_xy[r.numpts-2].y, r.endextn);
                r.pathtype = 0;
            }
            Wire wire(r.width, (WireStyle)r.pathtype, r.numpts, out_xy);
            ret = write_wire(&wire);
            clear_property_queue();
            if (!ret)
                return (false);
        }
        else if (!strcmp(token, "SREF")) {
            if (sscanf(line, "%s", instname) != 1)
                goto bad;
            char *s = instname;
            s += strlen(s) - 1;
            while (s >= instname && (isspace(*s) || *s == ';'))
                *s-- = '\0';
            if (!read_record(&r))
                return (false);

            // Just enough info for gds_out::write_sref.
            Instance inst;
            inst.magn = r.magn;
            inst.angle = r.angle;
            inst.origin.set(out_xy[0]);
            inst.reflection = r.reflection;
            inst.name = instname;

            ret = write_sref(&inst);
            clear_property_queue();
            if (!ret)
                return (false);
        }
        else if (!strcmp(token, "AREF")) {
            if (sscanf(line, "%s", instname) != 1)
                goto bad;
            char *s = instname;
            s += strlen(s) - 1;
            while (s >= instname && (isspace(*s) || *s == ';'))
                *s-- = '\0';
            if (!read_record(&r))
                return (false);

            // Just enough info for gds_out::write_sref.
            Instance inst;
            inst.magn = r.magn;
            inst.angle = r.angle;
            inst.nx = r.cols;
            inst.ny = r.rows;
            inst.origin.set(out_xy[0]);
            inst.apts[0].set(out_xy[1]);
            inst.apts[1].set(out_xy[2]);
            inst.gds_text = true;
            inst.reflection = r.reflection;
            inst.name = instname;

            ret = write_sref(&inst);
            clear_property_queue();
            if (!ret)
                return (false);
        }
        else if (!strcmp(token, "TEXT")) {
            if (!read_record(&r))
                return (false);

            Text text;
            text.text = r.string;
            text.x = out_xy[0].x;
            text.y = out_xy[0].y;
            text.magn = r.magn;
            text.angle = r.angle;
            text.font = r.font;
            text.hj = r.hj;
            text.vj = r.vj;
            text.pwidth = r.width;
            text.ptype = r.pathtype;
            text.reflection = r.reflection;
            text.abs_mag = r.abs_mag;
            text.abs_ang = r.abs_angle;
            text.gds_valid = true;

            ret = write_text(&text);
            delete [] r.string;
            clear_property_queue();
            if (!ret)
                return (false);
        }
        else if (!strcmp(token, "BOX")) {
            if (!read_record(&r))
                return (false);

            Poly po;
            po.points = out_xy;
            po.numpts = r.numpts;
            ret = write_poly(&po);
            clear_property_queue();
            if (!ret)
                return (false);
        }
        else if (!strcmp(token, "SNAPNODE")) {
            if (!read_record(&r))
                return (false);
            begin_record(4, II_SNAPNODE, 0);
            if (r.hadplex) {
                begin_record(8, II_PLEX, 3);
                long_copy(r.plexnum);
            }
            begin_record(6, II_LAYER, 2);
            short_copy(out_layer.layer);
            begin_record(6, II_NODETYPE, 2);
            short_copy(out_layer.datatype);
            begin_record(4 + 8*r.numpts, II_XY, 3);
            if (!flush_buf())
                return (false);
            int end = GDS_BUFSIZ - 12;  // leave room for II_ENDEL
            for (int i = 0; i < r.numpts; i++) {
                if (out_bufcnt >= end) {
                    if (!flush_buf())
                        return (false);
                }
                long_copy(2*i);
                long_copy(2*i + 1);
            }
            for (CDp *pd = out_prpty; pd; pd = pd->next_prp()) {
                if (prpty_gdsii(pd->value()))
                    continue;
                if (!write_property_rec(pd->value(), pd->string()))
                    return (false);
            }
            begin_record(4, II_ENDEL, 0);
            if (!flush_buf())
                return (false);
        }
        else if (!strcmp(token, "END"))
            return (write_end_struct());
    }
    Errs()->add_error( "premature EOF.");
    return (false);
bad:
    Errs()->add_error( "syntax error, line %d.", out_linecnt);
    return (false);
}


bool
gdstx_in::read_record(gds_rec *r)
{
    char *token, *line;
    while (getline(&token, &line)) {
        if (!strcmp(token, "LAYER")) {
            if (sscanf(line, "%d", &out_layer.layer) != 1)
                goto bad;
        }
        else if (!strcmp(token, "WIDTH")) {
            if (sscanf(line, "%d", &r->width) != 1)
                goto bad;
        }
        else if (!strcmp(token, "TEXTTYPE")) {
            if (sscanf(line, "%d", &out_layer.datatype) != 1)
                goto bad;
        }
        else if (!strcmp(token, "DATATYPE")) {
            if (sscanf(line, "%d", &out_layer.datatype) != 1)
                goto bad;
        }
        else if (!strcmp(token, "NODETYPE")) {
            if (sscanf(line, "%d", &out_layer.datatype) != 1)
                goto bad;
        }
        else if (!strcmp(token, "COLUMNS")) {
            if (sscanf(line, "%d", &r->cols) != 1)
                goto bad;
            if (!getline(&token, &line))
                break;
            if (sscanf(line, "%d", &r->rows) != 1)
                goto bad;
        }
        else if (!strcmp(token, "COORDINATE")) {
            if (sscanf(line, "%d, %d", &out_xy[0].x, &out_xy[0].y) != 2)
                goto bad;
            r->numpts = 1;
        }
        else if (!strcmp(token, "COORDINATES")) {
            if (sscanf(line, "%d %d, %d", &r->numpts,
                    &out_xy[0].x, &out_xy[0].y) != 3)
                goto bad;
            for (int i = 1; i < r->numpts; i++) {
                if (!getline(0, &line))
                    goto bad;
                if (sscanf(line, "%d, %d", &out_xy[i].x, &out_xy[i].y) != 2)
                    goto bad;
            }
        }
        else if (!strcmp(token, "PREF")) {
            if (sscanf(line, "%d, %d", &out_xy[0].x, &out_xy[0].y) != 2)
                goto bad;
            if (!getline(&token, &line))
                break;
            if (sscanf(line, "%d, %d", &out_xy[1].x, &out_xy[1].y) != 2)
                goto bad;
            if (!getline(&token, &line))
                break;
            if (sscanf(line, "%d, %d", &out_xy[2].x, &out_xy[2].y) != 2)
                goto bad;
            r->numpts = 3;
        }
        else if (!strcmp(token, "HJUSTIFICATION")) {
            if (sscanf(line, "%d", &r->hj) != 1)
                goto bad;
            if (!getline(&token, &line))
                break;
            if (sscanf(line, "%d", &r->vj) != 1)
                goto bad;
            if (!getline(&token, &line))
                break;
            if (sscanf(line, "%d", &r->font) != 1)
                goto bad;
        }
        else if (!strcmp(token, "STRING")) {
            // have to handle multiple lines, up to ';'
            sLstr lstr;
            for (;;) {
                lstr.add(line);
                const char *e = lstr.string() + lstr.length() - 1;
                while (isspace(*e) && e > lstr.string())
                    e--;
                if (e >= lstr.string() && *e == ';') {
                    lstr.truncate(e - lstr.string(), 0);
                    break;
                }
                if ((line = fgets(out_linebuf, 1024, out_textfp)) == 0)
                    break;
            }
            r->string = lstr.string_trim();
        }
        else if (!strcmp(token, "ABSOLUTE")) {
            if (*line == 'A')
                r->abs_angle = true;
            else
                r->abs_mag = true;
        }
        else if (!strcmp(token, "REFLECTION"))
            r->reflection = true;
        else if (!strcmp(token, "MAGNIFICATION")) {
            if (sscanf(line, "%lf", &r->magn) != 1)
                goto bad;
        }
        else if (!strcmp(token, "ANGLE")) {
            if (sscanf(line, "%lf", &r->angle) != 1)
                goto bad;
        }
        else if (!strcmp(token, "PATHTYPE")) {
            if (sscanf(line, "%d", &r->pathtype) != 1)
                goto bad;
        }
        else if (!strcmp(token, "ATTRIBUTES")) {
            int val;
            if (sscanf(line, "%d", &val) != 1)
                goto bad;
            while (isdigit(*line))
                line++;
            line++;  // should be '='

            // have to handle multiple lines, up to ';'
            sLstr lstr;
            for (;;) {
                lstr.add(line);
                const char *e = lstr.string() + lstr.length() - 1;
                while (isspace(*e) && e > lstr.string())
                    e--;
                if (e >= lstr.string() && *e == ';') {
                    lstr.truncate(e - lstr.string(), 0);
                    break;
                }
                if ((line = fgets(out_linebuf, 1024, out_textfp)) == 0)
                    break;
            }
            if (!queue_property(val, lstr.string()))
                return (false);
        }
        else if (!strcmp(token, "BOXTYPE")) {
            if (sscanf(line, "%d", &out_layer.datatype) != 1)
                goto bad;
        }
        else if (!strcmp(token, "PLEX")) {
            if (sscanf(line, "%d", &r->plexnum) != 1)
                goto bad;
            r->hadplex = true;
        }
        else if (!strcmp(token, "BGNEXTN")) {
            if (sscanf(line, "%d", &r->bgnextn) != 1)
                goto bad;
        }
        else if (!strcmp(token, "ENDEXTN")) {
            if (sscanf(line, "%d", &r->endextn) != 1)
                goto bad;
        }
        else if (!strcmp(token, "END"))
            return (true);
    }
    Errs()->add_error( "premature EOF.");
    return (false);
bad:
    Errs()->add_error( "syntax error, line %d.", out_linecnt);
    return (false);
}


bool
gdstx_in::getline(char **token, char **string)
{
    char *line;
    while ((line = fgets(out_linebuf, 1024, out_textfp)) != 0) {
        out_linecnt++;
        if (line[0] == '>' && line[1] == '>')
            continue;
        break;
    }
    if (!line)
        return (false);
    if (token) {
        while (isdigit(*line))
            line++;
        while (isspace(*line))
            line++;
        char *t = line;
        while (*line && !isspace(*line) && *line != ';')
            line++;
        *token = t;
        if (*line)
            *line++ = '\0';
    }
    while (isspace(*line) || *line == ';')
        line++;
    *string = line;
    return (true);
}

