
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2014 Whiteley Research Inc, all rights reserved.        *
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
 $Id: geo_3d.h,v 1.9 2014/09/25 04:47:16 stevew Exp $
 *========================================================================*/

#ifndef GEO_3D_H
#define GEO_3D_H

#include "geo_zlist.h"


// A 3-space coordinate.
//
struct xyz3d
{
    xyz3d()
        {
            x = 0;
            y = 0;
            z = 0;
        }

    xyz3d(int xx, int yy, int zz)
        {
            x = xx;
            y = yy;
            z = zz;
        }

    xyz3d(const xyz3d &os, double sc, const xyz3d &t1, const xyz3d &t2)
        {
            x = mmRnd(os.x + sc*(t1.x - t2.x));
            y = mmRnd(os.y + sc*(t1.y - t2.y));
            z = mmRnd(os.z + sc*(t1.z - t2.z));
        }

    // Return the vector length.
    //
    int mag() const
    {
        if (x == 0) {
            if (y == 0)
                return (abs(z));
            if (z == 0)
                return (abs(y));
            return (mmRnd(sqrt(y*(double)y + z*(double)z)));
        }
        if (y == 0) {
            if (z == 0)
                return (abs(x));
            return (mmRnd(sqrt(x*(double)x + z*(double)z)));
        }
        if (z == 0)
            return (mmRnd(sqrt(x*(double)x + y*(double)y)));
        return (mmRnd(sqrt(x*(double)x + y*(double)y + z*(double)z)));
    }


    // Compute os + sc*(t1 - t2).
    //
    void scale(const xyz3d &os, double sc, const xyz3d &t1, const xyz3d &t2)
    {
        x = mmRnd(os.x + sc*(t1.x - t2.x));
        y = mmRnd(os.y + sc*(t1.y - t2.y));
        z = mmRnd(os.z + sc*(t1.z - t2.z));
    }

    int x, y, z;
};

inline bool operator== (const xyz3d &a, const xyz3d &b)
{
    return (a.x == b.x && a.y == b.y && a.z == b.z);
}

inline xyz3d operator+ (const xyz3d &a, const xyz3d &b)
{
    return (xyz3d(a.x + b.x, a.y + b.y, a.z + b.z));
}

inline xyz3d operator- (const xyz3d &a, const xyz3d &b)
{
    return (xyz3d(a.x - b.x, a.y - b.y, a.z - b.z));
}

inline xyz3d operator/ (const xyz3d &a, const int b)
{
    return (xyz3d(a.x/b, a.y/b, a.z/b));
}

struct qflist3d;

// A three-dimensional quadrilateral.  The four points are all in the
// same plane.
//
struct qface3d
{
    qface3d() { }

    qface3d(int x1, int y1, int z1, int x2, int y2, int z2,
            int x3, int y3, int z3, int x4, int y4, int z4) :
        c1(x1, y1, z1), c2(x2, y2, z2), c3(x3, y3, z3), c4(x4, y4, z4) { }

    qface3d(const Point *p1, const Point *p2, int z1, int z2) : 
        c1(p1->x, p1->y, z1), c2(p1->x, p1->y, z2),
        c3(p2->x, p2->y, z2), c4(p2->x, p2->y, z1) { }

    qface3d(const Zoid *Z, int z) :
        c1(Z->xll, Z->yl, z), c2(Z->xul, Z->yu, z),
        c3(Z->xur, Z->yu, z), c4(Z->xlr, Z->yl, z) { }

    qface3d(const xyz3d a, const xyz3d b, const xyz3d c, const xyz3d d) :
        c1(a), c2(b), c3(c), c4(d) { }

    bool intersect(const BBox *BB, BBox *rBB, bool touchok)
        {
            if (c2.z != c1.z || c3.z != c1.z || c4.z != c1.z)
                return (false);
            BBox tBB(mmMin(c1.x, c3.x), mmMin(c1.y, c3.y), mmMax(c1.x, c3.x),
                mmMax(c1.y, c3.y));
            if (rBB)
                *rBB = tBB;
            return (tBB.intersect(BB, touchok));
        }

    qflist3d *split(int) const;
    qflist3d *side_split(int, int, int) const;
    qflist3d *quad() const;
    qflist3d *cut(int, int, bool) const;
    void print(FILE*, int, int, int, double, const char*) const;

    xyz3d c1;
    xyz3d c2;
    xyz3d c3;
    xyz3d c4;
};

struct qflist3d
{
    qflist3d()
        {
            next = 0;
        }

    qflist3d(int x1, int y1, int z1, int x2, int y2, int z2,
            int x3, int y3, int z3, int x4, int y4, int z4, qflist3d *n = 0) :
        Q(x1, y1, z1, x2, y2, z2, x3, y3, z3, x4, y4, z4)
        {
            next = n;
        }

    qflist3d(const Point *p1, const Point *p2, int z1, int z2,
            qflist3d *n = 0) :
        Q(p1, p2, z1, z2)
        {
            next = n;
        }

    qflist3d(const Zoid *Z, int z, qflist3d *n = 0) :
        Q(Z, z)
        {
            next = n;
        }

    qflist3d(const xyz3d a, const xyz3d b, const xyz3d c, const xyz3d d,
            qflist3d *n = 0) :
        Q(a,b,c,d)
        {
            next = n;
        }

    static void destroy(qflist3d *q)
        {
            while (q) {
                qflist3d *x = q;
                q = q->next;
                delete x;
            }
        }

    static qflist3d *copy(const qflist3d *thisqf)
        {
            qflist3d *q0 = 0, *qe = 0;
            for (const qflist3d *q = thisqf; q; q = q->next) {
                if (q0) {
                    qe->next = new qflist3d(*q);
                    qe = qe->next;
                }
                else
                    q0 = qe = new qflist3d(*q);
                qe->next = 0;
            }
            return (q0);
        }

    static void print(const qflist3d *thisqf, FILE *fp, int n,
        int x, int y, double sc, const char *ff)
        {
            for (const qflist3d *q = thisqf; q; q = q->next)
                q->Q.print(fp, n, x, y, sc, ff);
        }

    static qflist3d *split(const qflist3d *thisqf, int t)
        {
            qflist3d *q0 = 0, *qe = 0;
            for (const qflist3d *q = thisqf; q; q = q->next) {
                qflist3d *qs = q->Q.split(t);
                if (qs) {
                    if (!q0)
                        q0 = qe = qs;
                    else {
                        while (qe->next)
                            qe = qe->next;
                        qe->next = qs;
                    }
                }
            }
            return (q0);
        }


    // Apply partitioning to the vertical edge panels.  These are always
    // rectangular.  We cut a thin edge at the top and bottom, and split
    // the middle part with the minimum of partmax and edgemax.
    //
    static qflist3d *side_split(const qflist3d *thisqf, int partmax,
        int edgemax, int thinedge)
        {
            qflist3d *q0 = 0, *qe = 0;
            for (const qflist3d *q = thisqf; q; q = q->next) {
                qflist3d *qs = q->Q.side_split(partmax, edgemax, thinedge);
                if (qs) {
                    if (!q0)
                        q0 = qe = qs;
                    else {
                        while (qe->next)
                            qe = qe->next;
                        qe->next = qs;
                    }
                }
            }
            return (q0);
        }

    static bool save(const qflist3d *thisqf, FILE *fp, const char *qname,
        int indent)
        {
            if (!fp)
                return (false);
            int cnt = 0;
            for (const qflist3d *q = thisqf; q; cnt++, q = q->next) ;
            if (!cnt)
                return (true);

            fprintf(fp, "%*c%s %d\n", indent, ' ', qname ? qname : "unnamed",
                cnt);
            indent++;
            for (const qflist3d *q = thisqf; q; q = q->next) {
                fprintf(fp, "%*c%d,%d,%d %d,%d,%d %d,%d,%d %d,%d,%d\n",
                    indent, ' ',
                    q->Q.c1.x,q->Q.c1.y,q->Q.c1.z,
                    q->Q.c2.x,q->Q.c2.y,q->Q.c2.z,
                    q->Q.c3.x,q->Q.c3.y,q->Q.c3.z,
                    q->Q.c4.x,q->Q.c4.y,q->Q.c4.z);
            }
            return (true);
        }

    qflist3d *next;
    qface3d Q;
};


// A Zoid, but with z-direction coordinate.
//
struct Zoid3d : public Zoid
{
    Zoid3d(Zoid z, int b, int t) : Zoid(z)
        {
            zbot = b;
            ztop = t;
        }

    qflist3d *get_zbottom() const
        {
            return (new qflist3d(
                xlr, yl, zbot, xur, yu, zbot,
                xul, yu, zbot, xll, yl, zbot));
        }

    qflist3d *get_ztop() const
        {
            return (new qflist3d(
                xll, yl, ztop, xul, yu, ztop,
                xur, yu, ztop, xur, yl, ztop));
        }

    qflist3d *get_left() const
        {
            return (new qflist3d(
                xul, yu, zbot, xul, yu, ztop,
                xll, yl, ztop, xll, yl, zbot));
        }

    qflist3d *get_bottom() const
        {
            return (new qflist3d(
                xll, yl, zbot, xll, yl, ztop,
                xlr, yl, ztop, xlr, yl, zbot));
        }

    qflist3d *get_right() const
        {
            return (new qflist3d(
                xlr, yl, zbot, xlr, yl, ztop,
                xur, yu, ztop, xur, yu, zbot));
        }

    qflist3d *get_top() const
        {
            return (new qflist3d(
                xur, yu, zbot, xur, yu, ztop,
                xul, yu, ztop, xul, yu, zbot));
        }

    // Return true if the figures touch or intersect.
    //
    bool intersect(const Zoid3d &Zx) const
        {
            if (Zx.ztop < zbot || ztop < Zx.zbot)
                return (false);
            if (!Zoid::intersect(dynamic_cast<const Zoid*>(&Zx), true))
                return (false);
            return (true);
        }

    double volume() const
        {
            return (area() * MICRONS(ztop - zbot));
        }

    int zbot;
    int ztop;
};

// List element for Zoid3d.
//
struct Zlist3d
{
    Zlist3d(const Zoid &z, int b, int t, Zlist3d *n = 0) : Z(z, b, t)
        {
            next = n;
        }

    static void destroy(Zlist3d *z)
        {
            while (z) {
                Zlist3d *zx = z;
                z = z->next;
                delete zx;
            }
        }

    Zlist3d *next;
    Zoid3d Z;
};

struct glZlist3d;

// A Zoid3d that also carries integer values for group number and
// layer index.  This and the following definitions are used in
// 3d-geometry representations for extraction and layer profile
// display.
//
struct glZoid3d : public Zoid3d
{
    glZoid3d(const Zoid &z, int b, int t, int l, int g) : Zoid3d(z, b, t)
        {
            layer_index = l;
            group = g;
        }

    int zcmp3d(const glZoid3d *zc) const
        {
            int i = Zoid::zcmp(zc);
            if (!i) {
                if (ztop < zc->ztop)
                    return (1);
                if (ztop > zc->ztop)
                    return (-1);
                if (zbot > zc->zbot)
                    return (1);
                if (zbot < zc->zbot)
                    return (-1);
            }
            return (i);
        }

    void print(FILE*, double, const char*, const char *flg = 0) const;
    void rt_triang(glZlist3d**, int);

    int layer_index;
    int group;
};

// A glZoid3d list element.
//
struct glZlist3d
{
    glZlist3d(const Zoid &z, int b, int t, int l, int g) : Z(z, b, t, l, g)
        {
            next = 0;
        }

    glZlist3d(const glZoid3d *z, glZlist3d *n = 0) : Z(*z)
        {
            next = n;
        }

    static void destroy(glZlist3d *z)
        {
            while (z) {
                glZlist3d *zx = z;
                z = z->next;
                delete zx;
            }
        }

    static Zlist *to_zlist(const glZlist3d *thisgl)
        {
            Zlist *z0 = 0, *ze = 0;
            for (const glZlist3d *z = thisgl; z; z = z->next) {
                if (!z0)
                    z0 = ze = new Zlist(&z->Z, 0);
                else {
                    ze->next = new Zlist(&z->Z, 0);
                    ze = ze->next;   
                }
            }
            return (z0);
        }

    static double volume(const glZlist3d *thisgl)
        {
            double v = 0;
            for (const glZlist3d *z = thisgl; z; z = z->next)
                v += z->Z.volume();
            return (v);
        }

    static glZlist3d *sort(glZlist3d*);
    static glZlist3d *manhattanize(glZlist3d*, int);

    glZlist3d *next;
    glZoid3d Z;
};

// Similar to above, but contains a pointer to an glZoid3d.
//
struct glZlistRef3d
{
    glZlistRef3d(const glZoid3d *z, glZlistRef3d *n = 0)
        {
            next = n;
            PZ = z;
        }

    static void destroy(glZlistRef3d *z)
        {
            while (z) {
                glZlistRef3d *zx = z;
                z = z->next;
                delete zx;
            }
        }

    Zlist *to_zlist() const
        {
            Zlist *z0 = 0, *ze = 0;
            for (const glZlistRef3d *z = this; z; z = z->next) {
                if (!z0)
                    z0 = ze = new Zlist(z->PZ, 0);
                else {
                    ze->next = new Zlist(z->PZ, 0);
                    ze = ze->next;   
                }
            }
            return (z0);
        }

    double volume() const
        {
            double v = 0;
            for (const glZlistRef3d *z = this; z; z = z->next)
                v += z->PZ->volume();
            return (v);
        }

    glZlistRef3d *next;
    const glZoid3d *PZ;
};

// A group container for glZlist3d.
//
struct glZgroup3d
{
    glZgroup3d()
        {
            list = 0;
            num = 0;
        }

    ~glZgroup3d()
        {
            while (num--)
                glZlist3d::destroy(list[num]);
            delete [] list;
        }

    glZlist3d **list;
    int num;
};

// A group container for glZlistRef3d.
//
struct glZgroupRef3d
{
    glZgroupRef3d()
        {
            list = 0;
            num = 0;
        }

    glZgroupRef3d(const glZgroup3d*);

    ~glZgroupRef3d()
        {
            while (num--)
                glZlistRef3d::destroy(list[num]);
            delete [] list;
        }

    glZlistRef3d **list;
    int num;
};

// A Ylist for glZlist3d.
//
struct glYlist3d
{
    glYlist3d(glZlist3d*, bool = false);
    ~glYlist3d() { glZlist3d::destroy(y_zlist); }

    static void destroy(glYlist3d *y)
        {
            while (y) {
                glYlist3d *yn = y->next;
                delete y;
                y = yn;
            }
        }

    static double volume(const glYlist3d *thisgl)
        {
            double v = 0;
            for (const glYlist3d *y = thisgl; y; y = y->next)
                v += glZlist3d::volume(y->y_zlist);
            return (v);
        }

    static glZlist3d *to_zl3d(glYlist3d*);
    static glZgroup3d *group(glYlist3d*, int = 0);

private:
    void remove_next(glZlist3d *zp, glZlist3d *z)
        {
            if (!zp)
                y_zlist = z->next;
            else
                zp->next = z->next;
            if (z->Z.yl == y_yl && y_zlist) {
                int y = y_zlist->Z.yl;
                if (y != y_yl) {
                    for (glZlist3d *zl = y_zlist->next; zl; zl = zl->next) {
                        if (zl->Z.yl < y)
                            y = zl->Z.yl;
                        if (y == y_yl)
                            break;
                    }
                    y_yl = y;
                }
            }
        }

    // ZTST ok
    glYlist3d *touching(glZlist3d**, const Zoid3d*);
    glYlist3d *first(glZlist3d**);

public:
    glYlist3d *next;    // next->y_yu, < y_yu
    glZlist3d *y_zlist; // list of zoids, each have Z.yu == y_yu
    int y_yu;           // top of zoids
    int y_yl;           // minimum Z.yl in y_zlist
};

#endif

