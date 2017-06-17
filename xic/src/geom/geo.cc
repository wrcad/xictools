
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: geo.cc,v 1.27 2016/10/10 16:31:47 stevew Exp $
 *========================================================================*/

#include "cd.h"
#include "geo_memmgr.h"


cGEO *cGEO::instancePtr = 0;

cGEO::cGEO()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cGEO already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    geoSpotSize = -1;   // Defaults to MfgGrid
    geoElecRoundSides = DEF_RoundFlashSides;
    geoPhysRoundSides = DEF_RoundFlashSides;
    geoUseSclFuncs = false;

    new cGEOmmgr;   // Memory manager.
}


#ifdef GEO_TEST_NULL
// Private static error exit.
//
void
cGEO::on_null_ptr()
{
    fprintf(stderr, "Singleton class cGEO used before instantiated.\n");
    exit(1);
}
#endif


cGEO::~cGEO()
{
    clearAll();
}


void
cGEO::clearAll()
{
    geoSpotSize = -1;
    geoElecRoundSides = DEF_RoundFlashSides;
    geoPhysRoundSides = DEF_RoundFlashSides;
    Zlist::reset_join_params();
    geoCurTform.set_identity();
    GEOmmgr()->collectTrash();
}


// Set the transform according to the values of the "current
// transform", and the given reference and translation
// coordinates.
//
void
cGEO::applyCurTransform(cTfmStack *tstk, int ref_x, int ref_y,
    int new_x, int new_y)
{
    tstk->TTranslate(-ref_x, -ref_y);

    if (curTx()->angle() != 0) {
        if (curTx()->angle() == 45)        tstk->TRotate(1, 1);
        else if (curTx()->angle() == 90)   tstk->TRotate(0, 1);
        else if (curTx()->angle() == 135)  tstk->TRotate(-1, 1);
        else if (curTx()->angle() == 180)  tstk->TRotate(-1, 0);
        else if (curTx()->angle() == 225)  tstk->TRotate(-1, -1);
        else if (curTx()->angle() == 270)  tstk->TRotate(0, -1);
        else if (curTx()->angle() == 315)  tstk->TRotate(1, -1);
    }
    // Mirror last, so image will always invert wrt named axis.
    if (curTx()->reflectY())
        tstk->TMY();
    if (curTx()->reflectX())
        tstk->TMX();
    tstk->TTranslate(new_x, new_y);
}


// Static function.
// Return a coordinate list, keeping lines less than 80 chars.  If
// ndgt, print in integer format, otherwise print in float format
// using ndgt precision.
//
char *
cGEO::path_string(const Point *points, int numpts, int ndgt)
{
    if (!points || numpts <= 0)
        return (0);
    char buf[84];
    sLstr lstr;
    int len = 0;
    for (const Point *pair = points; numpts; pair++, numpts--) {
        if (ndgt)
            sprintf(buf, " %.*f,%.*f",
                ndgt, MICRONS(pair->x), ndgt, MICRONS(pair->y));
        else
            sprintf(buf, " %d,%d", pair->x, pair->y);
        int len1 = strlen(buf);
        if (len + len1 < 79) {
            lstr.add(buf);
            len += len1;
        }
        else {
            lstr.add_c('\n');
            lstr.add(buf);
            len = len1;
        }
    }
    return (lstr.string_trim());
}


// Static function.
// As above, but print the differences to the previous values.
//
char *
cGEO::path_diff_string(const Point *points, int numpts, int ndgt)
{
    if (!points || numpts <= 0)
        return (0);
    char buf[84];
    sLstr lstr;
    int len = 0;
    int lx = points->x;
    int ly = points->y;
    for (const Point *pair = points; numpts; pair++, numpts--) {
        if (ndgt)
            sprintf(buf, " %.*f,%.*f",
                ndgt, MICRONS(pair->x - lx), ndgt, MICRONS(pair->y - ly));
        else
            sprintf(buf, " %d,%d", pair->x - lx, pair->y - ly);
        int len1 = strlen(buf);
        if (len + len1 < 79) {
            lstr.add(buf);
            len += len1;
        }
        else {
            lstr.add_c('\n');
            lstr.add(buf);
            len = len1;
        }
        lx = pair->x;
        ly = pair->y;
    }
    return (lstr.string_trim());
}
// End of cGEO functions.


// Return a string describing the current transform, caller must free.
// Returns an empty string for the identity transform.
//
char *
sCurTx::tform_string() const
{
    char buf[64];
    buf[0] = 0;
    if (ct_angle != 0)
        sprintf(buf, "R%d", ct_angle);
    if (ct_reflectY)
        strcat(buf, "MY");
    if (ct_reflectX)
        strcat(buf, "MX");
    if (magset()) {
        sprintf(buf + strlen(buf), "M%.8f", ct_magn);
        char *t = buf + strlen(buf) - 1;
        int i = 0;
        while (*t == '0' && i < 7) {
            *t-- = 0;
            i++;
        }
    }
    return (lstring::copy(buf));
}


// Parse the string and set the transform.  The syntax is as output
// from tform_string.  If error, nothing changes and false is
// returned.
//
bool
sCurTx::parse_tform_string(const char *str, bool no45)
{
    if (str) {
        while (isspace(*str))
            str++;
    }
    if (!str || !*str) {
        set_identity();
        return (true);
    }

    bool ry = false, rx = false;
    int ang = 0;
    double mag = 1.0;
    if (*str == 'R' || *str == 'r') {
        str++;
        char *e;
        ang = strtol(str, &e, 10);
        if (e == str || ang < 0 || ang > 315 || ang%45)
            return (false);
        if (no45 && ang%90)
            return (false);
        str = e;
    }

    // MXMY == MYMX
    if (!strncasecmp(str, "MY", 2)) {
        ry = true;
        str += 2;
    }
    if (!strncasecmp(str, "MX", 2)) {
        rx = true;
        str += 2;
    }
    if (!strncasecmp(str, "MY", 2)) {
        ry = true;
        str += 2;
    }

    if (*str == 'M' || *str == 'm') {
        str++;
        char *e;
        mag = strtod(str, &e);
        if (e == str || mag < 0.001 || mag > 1000.0)
            return (false);
        str = e;
    }
    if (!*str || isspace(*str)) {
        ct_angle = ang;
        ct_reflectY = ry;
        ct_reflectX = rx;
        ct_magn = mag;
        return (true);
    }
    return (false);
}

