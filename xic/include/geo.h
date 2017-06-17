
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
 $Id: geo.h,v 5.39 2016/10/10 16:31:51 stevew Exp $
 *========================================================================*/

#ifndef GEO_H
#define GEO_H

#include "geo_if.h"
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <limits.h>


#define VA_RoundFlashSides      "RoundFlashSides"
#define VA_ElecRoundFlashSides  "ElecRoundFlashSides"

// If defined, include test for use of main class before initialized.
//#define GEO_TEST_NULL

// Default sides per 360 degrees for "round" objects.  In physical
// mode, objects can have the range, in electrical mode the value is
// fixed at the default.
//
#define DEF_RoundFlashSides     32
#define MIN_RoundFlashSides     8
#define MAX_RoundFlashSides     256

struct Point;
struct BBox;
struct Zlist;
struct Ylist;
struct CDo;
struct sLspec;
struct CDs;
class cTfmStack;

// The "current transform" parameters.
//
struct sCurTx
{
    sCurTx() { set_identity(); }

    double magn()         const { return (ct_magn); }
    void set_magn(double m)
        {
            if (m >= 0.001 && m <= 1000.0)
                ct_magn = m;
        }
    bool magset()         const { return (fabs(ct_magn - 1.0) > 1e-12); }

    int angle()           const { return (ct_angle); }
    void set_angle(int a)
        {
            while (a < 0)
                a += 360;
            while (a >= 360)
                a -= 360;
            if (a%45 == 0)
                ct_angle = a;
        }

    bool reflectY()       const { return (ct_reflectY); }
    void set_reflectY(bool b)   { ct_reflectY = b; }

    bool reflectX()       const { return (ct_reflectX); }
    void set_reflectX(bool b)   { ct_reflectX = b; }

    bool is_identity() const
        {
            if (ct_reflectX || ct_reflectY || ct_angle != 0 || magset())
                return (false);
            return (true);
        }

    void set_identity()
        {
            ct_magn = 1.0;
            ct_angle = 0;
            ct_reflectY = false;
            ct_reflectX = false;
        }

    // geo.cc
    char *tform_string() const;
    bool parse_tform_string(const char*, bool);

private:
    double ct_magn;
    int ct_angle;
    bool ct_reflectY;
    bool ct_reflectX;
};


inline class cGEO *GEO();

class cGEO : public GEOif
{
#ifdef GEO_TEST_NULL
    static cGEO *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();
#endif

public:
#ifdef GEO_TEST_NULL
    friend inline cGEO *GEO() { return (cGEO::ptr()); }
#else
    friend inline cGEO *GEO() { return (instancePtr); }
#endif

    // geo.cc
    cGEO();
    ~cGEO();

    void clearAll();
    void applyCurTransform(cTfmStack*, int, int, int, int);

    static char *path_string(const Point*, int, int);
    static char *path_diff_string(const Point*, int, int);

    // geo_arcpath.cc
    Point *makeArcPath(int*, bool, int, int, int, int, int=0, int=0,
        double=0.0, double=0.0) const;

    // geo_dist.cc
    static int mindist(const Point*, const Point*, const Point*, Point*);
    static int mindist(const Point*, const CDo*, Point*);
    static int mindist(const Point*, const Point*, const CDo*, Point*, Point*);
    static int mindist(const CDo*, const CDo*, Point*, Point*);
    static int maxdist(const Point*, const CDo*, Point*);
    static int maxdist(const CDo*,  const CDo*, Point*, Point*);

    // geo_edge.cc
    static XIrt edge_zlist(const CDs*, sLspec*, const Point*, const Point*,
        bool, bool, int*, Zlist**);
    static Zlist *merge_edge_zlists(const Zlist*, int, Zlist*);
    static void edge_overlap(const Zlist*, int, const Point*, const Point*,
        int*, int*);

    // geo_line.cc
    static bool lines_cross(const Point&, const Point&, const Point&,
        const Point&, Point*);

    // geo_lineclip.cc
    static bool line_clip(int*, int*, int*, int*, const BBox*);
    static bool check_colinear(int, int, int, int, int, int, int);
    static bool check_colinear(const Point&, const Point&, const Point&, int);
    static bool check_colinear(const Point*, const Point*, int, int, int);
    static bool clip_colinear(Point&, Point&, Point&, Point&);

    // geo_path.cc
    static bool path_box_intersect(const Point*, int, const BBox*, bool);
    static bool path_path_intersect(const Point*, int, const Point*, int,
        bool);

    // geo_tospot.cc
    void setToSpot(Point*, int*) const;
    int to_spot(int c) const
        {
            if (geoSpotSize <= 0)
                return (c);
            if (c > 0)
                c += geoSpotSize/2;
            else if (c < 0)
                c -= geoSpotSize/2;
            else
                return (0);
            return ((c/geoSpotSize)*geoSpotSize);
        }

    void setSpotSize(int s)             { geoSpotSize = s; }
    int spotSize() const                { return (geoSpotSize); }
    void setElecRoundFlashSides(int r)  { geoElecRoundSides = r; }
    void setPhysRoundFlashSides(int r)  { geoPhysRoundSides = r; }
    int roundFlashSides(bool elec) const
        {
            return (elec ? geoElecRoundSides : geoPhysRoundSides);
        }

    // Should be temporary: switch in/out new scanline-based
    // geometry functions.
    bool useSclFuncs()                  { return (geoUseSclFuncs); }
    void setUseSclFuncs(bool b)         { geoUseSclFuncs = b; }

    const sCurTx *curTx()               { return (&geoCurTform); }
    void setCurTx(sCurTx &t)            { geoCurTform = t; }

private:
    int geoSpotSize;        // Implicit grid
    int geoElecRoundSides;  // Sides per 360 degrees for elec round objects
    int geoPhysRoundSides;  // Sides per 360 degrees for phys round objects
    bool geoUseSclFuncs;    // Use new scanline geometry functions.
    sCurTx geoCurTform;     // "Current Transform" parameters

    static cGEO *instancePtr;
};

#endif

