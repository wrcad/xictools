
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
 $Id: geo_wire.h,v 5.22 2015/06/17 18:36:11 stevew Exp $
 *========================================================================*/

#ifndef GEO_WIRE_H
#define GEO_WIRE_H


struct Point;
struct BBox;
struct Poly;
struct PolyList;
struct Zlist;
namespace geo_wire {
    struct ptList;
}

// Wire end types, following GDSII.
enum WireStyle {CDWIRE_FLUSH=0, CDWIRE_ROUND=1, CDWIRE_EXTEND=2};

// Wire diagnostic flags, from toPoly.
//
// Errors (wire can't be rendered, toPoly fails)
#define CDWIRE_NOPTS        0x1
    // Wire has no points list.
#define CDWIRE_BADWIDTH     0x2
    // Zero width and only one vertex.
#define CDWIRE_BADSTYLE     0x4
    // Flush endstyle and only one vertex.
//
// Info/warnings (toPoly succeeds, but... )
#define CDWIRE_ONEVRT       0x10
    // Only one vertex.
#define CDWIRE_ZEROWIDTH    0x20
    // Wire has zero width.
#define CDWIRE_CLOSEVRTS    0x40
    // Wire has two vertices closer than half-width.
#define CDWIRE_CLIPFIX      0x80
    // The segment clipping fixup was invoked.
#define CDWIRE_BIGPOLY      0x100
    // The wire requires a huge polygon to render.


// wire element
struct Wire
{
    Wire() { points = 0; numpts = 0; attributes = 0; }
    Wire(int w, WireStyle s, int num, Point *pts)
        { points = pts; numpts = num;
        attributes = (((unsigned)w >> 1) << 2) | (s & 0x3); }
    Wire(int num, Point *pts, unsigned int atr)
        { points = pts; numpts = num; attributes = atr; }

    unsigned int wire_width() const { return ((attributes & ~0x3) >> 1); }
    WireStyle wire_style() const { return ((WireStyle)(attributes & 0x3)); }
    void set_wire_width(unsigned int w)
        { attributes = ((w >> 1) << 2) | (attributes & 0x3); }
    void set_wire_style(WireStyle s)
        { attributes = (attributes & ~0x3) | (s & 0x3); }

    static bool chkp(Point *p1, Point *p2, int npoints)
        {
            for (int i = 1; i < npoints; i++) {
                if (p1[i] != p2[i])
                    return (false);
            }
            return (true);
        }

    static bool chkp_r(Point *p1, Point *p2, int npoints)
        {
            int j = npoints-2;
            for (int i = 1; i < npoints; i++, j--) {
                if (p1[i] != p2[j])
                    return (false);
            }
            return (true);
        }

    bool v_compare(const Wire &w2) const
        {
            if (numpts != w2.numpts)
                return (false);
            if (!numpts)
                return (true);
            Point *p = w2.points;
            if (points[0] == p[0])
                return (chkp(points, p, numpts));
            if (points[0] == p[numpts-1])
                return (chkp_r(points, p, numpts));
            return (false);
        }

    // Return true if the wire can be represented by a Manhattan polygon.
    bool is_manhattan() const
        {
            if (wire_style() == CDWIRE_ROUND)
                return (false);
            for (int i = 1; i < numpts; i++) {
                if (points[i].x != points[i-1].x && points[i].y != points[i-1].y)
                    return (false);
            }
            return (true);
        }

    // Return true if an angle is not a multiple of 45 degrees.
    bool has_non45() const
        {
            if (wire_style() == CDWIRE_ROUND)
                return (true);
            for (int i = 1; i < numpts; i++) {
                int dx = points[i].x - points[i-1].x;
                if (!dx)
                    continue;
                int dy = points[i].y - points[i-1].y;
                if (!dy)
                    continue;
                if (abs(dx) != abs(dy))
                    return (true);
            }
            return (false);
        }

    void computeBB(BBox*) const;
    double area() const;
    int perim() const;
    void centroid(double*, double*) const;
    PolyList *clip(const BBox*, bool) const;
    bool intersect(const Point*, bool) const;
    bool intersect(const BBox*, bool) const;
    bool intersect(const Poly*, bool) const;
    bool intersect(const Wire*, bool) const;
    bool toPoly(Point**, int*, unsigned int* = 0) const;
    Zlist *toZlistFancy() const;
    Zlist *toZlist() const;
    Zlist *toZlistR() const;
    bool checkWire();
    void checkWireVerts(Point**, int*);

    static void flagWarnings(char*, unsigned int, const char*);
    static Zlist *cap_zoids(const Point*, const Point*, int, int);
    static void convert_end(int*, int*, int, int, int, bool);

private:
    static bool wire_vertex(const Point*, const Point*, const Point*, int,
        geo_wire::ptList&);
    static bool wire_end(const Point*, const Point*, int, int, int,
        geo_wire::ptList&);

public:
    Point *points;
    int numpts;
    unsigned int attributes;    // bits 0-1   end style type (0, 1, 2)
                                // rest       half width
};

#endif

