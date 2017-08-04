
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "geo.h"
#include "geo_3d.h"
#include <string.h>
#include <ctype.h>
#include <algorithm>


// Split the panel up into pieces such that no dimension is larger
// than t.
//
qflist3d *
qface3d::split(int t) const
{
    if (!t)
        return (new qflist3d(c1, c2, c3, c4));
    int ts = t/10;
    if (!ts)
        return (new qflist3d(c1, c2, c3, c4));
    qflist3d *q0 = 0;

    int m1 = (c3 - c2).mag();
    int m2 = (c4 - c1).mag();
    if (m2 > m1)
        m1 = m2;
    int n1 = m1/t + (m1 % t ? 1 : 0);

    m1 = (c2 - c1).mag();
    m2 = (c3 - c4).mag();
    if (m2 > m1)
        m1 = m2;
    int n2 = m1/t + (m1 % t ? 1 : 0);

    for (int i = 0; i < n2; i++) {
        xyz3d t1(c1, ((double)i)/n2, c2, c1);
        xyz3d t2(c1, ((double)(i+1))/n2, c2, c1);
        xyz3d t3(c4, ((double)(i+1))/n2, c3, c4);
        xyz3d t4(c4, ((double)i)/n2, c3, c4);

        for (int j = 0; j < n1; j++) {
            xyz3d t141(t1, ((double)j)/n1, t4, t1);
            xyz3d t231(t2, ((double)j)/n1, t3, t2);
            xyz3d t142(t1, ((double)(j+1))/n1, t4, t1);
            xyz3d t232(t2, ((double)(j+1))/n1, t3, t2);
            q0 = new qflist3d(t141, t231, t232, t142, q0);
        }
    }
    return (q0);
}


// Apply partitioning to the vertical edge panels.  These are always
// rectangular.  We cut a thin edge at the top and bottom, and split
// the middle part with the minimum of partmax and edgemax.
//
qflist3d *
qface3d::side_split(int partmax, int edgemax, int thinedge) const
{
    int t = partmax < edgemax ? partmax : edgemax;
    qflist3d *q0 = 0;

    qface3d qt(c1, c2, c3, c4);
    int ht = abs(c1.z - c3.z);
    if (thinedge > 0 && ht >= 3*thinedge) {
        if (c1.z < c3.z) {
            xyz3d t1(c1);
            xyz3d t2(c1);
            t2.z += thinedge;
            xyz3d t3(c3);
            t3.z = t2.z;
            xyz3d t4(c3);
            t4.z = t1.z;
            q0 = new qflist3d(t1, t2, t3, t4, q0);
            t1.z = c3.z - thinedge;
            t2.z = c3.z;
            t3.z = t2.z;
            t4.z = t1.z;
            q0 = new qflist3d(t1, t2, t3, t4, q0);
            t1.z = c1.z + thinedge;
            t2.z = c3.z - thinedge;
            t3.z = t2.z;
            t4.z = t1.z;
            qt.c1 = t1;
            qt.c2 = t2;
            qt.c3 = t3;
            qt.c4 = t4;
        }
        else {
            xyz3d t1(c3);
            xyz3d t2(c3);
            t2.z += thinedge;
            xyz3d t3(c1);
            t3.z = t2.z;
            xyz3d t4(c1);
            t4.z = t1.z;
            q0 = new qflist3d(t1, t2, t3, t4, q0);
            t1.z = c1.z - thinedge;
            t2.z = c1.z;
            t3.z = t2.z;
            t4.z = t1.z;
            q0 = new qflist3d(t1, t2, t3, t4, q0);
            t1.z = c3.z + thinedge;
            t2.z = c1.z - thinedge;
            t3.z = t2.z;
            t4.z = t1.z;
            qt.c1 = t1;
            qt.c2 = t2;
            qt.c3 = t3;
            qt.c4 = t4;
        }
    }

    qflist3d *qs = qt.split(t);
    if (qs) {
        qflist3d *qq = qs;
        while (qq->next)
            qq = qq->next;
        qq->next = q0;
        q0 = qs;
    }
    return (q0);
}


// Split into 3 triangles if triangular, four quads otherwise.
//
qflist3d *
qface3d::quad() const
{
    qflist3d *q0 = 0;
    if (c4 == c1) {
        xyz3d t1 = (c1 + c2 + c3)/3;
        q0 = new qflist3d(c1, c2, t1, t1, q0);
        q0 = new qflist3d(c2, c3, t1, t1, q0);
        q0 = new qflist3d(c3, c1, t1, t1, q0);
    }
    else if (c1 == c2) {
        xyz3d t1 = (c2 + c3 + c4)/3;
        q0 = new qflist3d(c2, c3, t1, t1, q0);
        q0 = new qflist3d(c3, c4, t1, t1, q0);
        q0 = new qflist3d(c4, c2, t1, t1, q0);
    }
    else if (c2 == c3) {
        xyz3d t1 = (c1 + c3 + c4)/3;
        q0 = new qflist3d(c1, c3, t1, t1, q0);
        q0 = new qflist3d(c3, c4, t1, t1, q0);
        q0 = new qflist3d(c4, c1, t1, t1, q0);
    }
    else if (c3 == c4) {
        xyz3d t1 = (c1 + c2 + c3)/3;
        q0 = new qflist3d(c1, c2, t1, t1, q0);
        q0 = new qflist3d(c2, c3, t1, t1, q0);
        q0 = new qflist3d(c3, c1, t1, t1, q0);
    }
    else {
        xyz3d t1(c1, 0.0, c2, c1);
        xyz3d t2(c1, 0.5, c2, c1);
        xyz3d t3(c4, 0.5, c3, c4);
        xyz3d t4(c4, 0.0, c3, c4);

        xyz3d t141(t1, 0.0, t4, t1);
        xyz3d t231(t2, 0.0, t3, t2);
        xyz3d t142(t1, 0.5, t4, t1);
        xyz3d t232(t2, 0.5, t3, t2);
        q0 = new qflist3d(t141, t231, t232, t142, q0);
        t141.scale(t1, 0.5, t4, t1);
        t231.scale(t2, 0.5, t3, t2);
        t142.scale(t1, 1.0, t4, t1);
        t232.scale(t2, 1.0, t3, t2);
        q0 = new qflist3d(t141, t231, t232, t142, q0);

        t1.scale(c1, 0.5, c2, c1);
        t2.scale(c1, 1.0, c2, c1);
        t3.scale(c4, 1.0, c3, c4);
        t4.scale(c4, 0.5, c3, c4);

        t141.scale(t1, 0.0, t4, t1);
        t231.scale(t2, 0.0, t3, t2);
        t142.scale(t1, 0.5, t4, t1);
        t232.scale(t2, 0.5, t3, t2);
        q0 = new qflist3d(t141, t231, t232, t142, q0);
        t141.scale(t1, 0.5, t4, t1);
        t231.scale(t2, 0.5, t3, t2);
        t142.scale(t1, 1.0, t4, t1);
        t232.scale(t2, 1.0, t3, t2);
        q0 = new qflist3d(t141, t231, t232, t142, q0);
    }
    return (q0);
}


// Cut along x or y, orientation depends on vert.
//
qflist3d *
qface3d::cut(int x, int y, bool vert) const
{
    // Require Z-planar.
    if (c2.z != c1.z || c3.z != c1.z || c4.z != c1.z)
        return (0);

    qflist3d *q0 = 0;
    if (vert) {
        if (x > c1.x && x > c2.x && x < c3.x && x < c4.x) {
            xyz3d t1(x, c2.y + ((x - c2.x)*(c3.y - c2.y))/(c3.x - c2.x), c1.z);
            xyz3d t2(x, c1.y + ((x - c1.x)*(c4.y - c1.y))/(c4.x - c1.x), c1.z);
            q0 = new qflist3d(c1, c2, t1, t2, 0);
            q0 = new qflist3d(t2, t1, c3, c4, q0);
        }
    }
    else {
        if (y > c1.y && y > c4.y && y < c2.y && y < c3.y) {
            xyz3d t2(c1.x + ((y - c1.y)*(c2.x - c1.x))/(c2.y - c1.y), y, c1.z);
            xyz3d t3(c4.x + ((y - c4.y)*(c3.x - c4.x))/(c3.y - c4.y), y, c1.z);
            q0 = new qflist3d(c1, t2, t3, c4, 0);
            q0 = new qflist3d(t2, c2, c3, t3, q0);
        }
    }
    return (q0);
}


// Print the fastcap input file line describing the panel.
//
void
qface3d::print(FILE *fp, int n, int x, int y, double sc, const char *ff) const
{
    if (!fp || !ff)
        return;
    char tbuf[32];
    if (c4 == c1) {
        sprintf(tbuf, "T %-3d", n);
        fputs(tbuf, fp);
        char *e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c1.x+x));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c1.y+y));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*c1.z);
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c2.x+x));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c2.y+y));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*c2.z);
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c3.x+x));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c3.y+y));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*c3.z);
        e = tbuf + strlen(tbuf) - 1;
        while (isspace(*e))
            *e-- = 0;
        fputs(tbuf, fp);
    }
    else if (c1 == c2) {
        sprintf(tbuf, "T %-3dd", n);
        fputs(tbuf, fp);
        char *e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c2.x+x));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c2.y+y));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*c2.z);
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c3.x+x));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c3.y+y));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*c3.z);
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c4.x+x));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c4.y+y));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*c4.z);
        e = tbuf + strlen(tbuf) - 1;
        while (isspace(*e))
            *e-- = 0;
        fputs(tbuf, fp);
    }
    else if (c2 == c3) {
        sprintf(tbuf, "T %-3d", n);
        fputs(tbuf, fp);
        char *e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c1.x+x));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c1.y+y));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*c1.z);
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c2.x+x));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c2.y+y));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*c2.z);
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c4.x+x));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c4.y+y));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*c4.z);
        e = tbuf + strlen(tbuf) - 1;
        while (isspace(*e))
            *e-- = 0;
        fputs(tbuf, fp);
    }
    else if (c3 == c4) {
        sprintf(tbuf, "T %-3d", n);
        fputs(tbuf, fp);
        char *e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c1.x+x));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c1.y+y));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*c1.z);
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c2.x+x));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c2.y+y));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*c2.z);
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c3.x+x));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c3.y+y));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*c3.z);
        e = tbuf + strlen(tbuf) - 1;
        while (isspace(*e))
            *e-- = 0;
        fputs(tbuf, fp);
    }
    else {
        sprintf(tbuf, "Q %-3d", n);
        fputs(tbuf, fp);
        char *e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c1.x+x));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c1.y+y));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*c1.z);
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c2.x+x));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c2.y+y));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*c2.z);
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c3.x+x));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c3.y+y));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*c3.z);
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c4.x+x));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*(c4.y+y));
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        sprintf(tbuf, ff, sc*c4.z);
        e = tbuf + strlen(tbuf) - 1;
        while (isspace(*e))
            *e-- = 0;
        fputs(tbuf, fp);
    }
    fprintf(fp, "\n");
}
// End of qface3d functions.


// glYlist3d constructor, z0 is consumed.
//
glYlist3d::glYlist3d(glZlist3d *z0, bool sub)
{
    y_zlist = 0;
    y_yu = y_yl = 0;
    next = 0;

    if (z0) {
        if (sub) {
            y_zlist = z0;
            y_yu = z0->Z.yu;
            y_yl = z0->Z.yl;
        }
        else {
            z0 = glZlist3d::sort(z0);
            y_zlist = z0;
            y_yu = z0->Z.yu;
            y_yl = z0->Z.yl;
            glYlist3d *ylast = this;
            while (z0) {
                if (ylast->y_yl > z0->Z.yl)
                    ylast->y_yl = z0->Z.yl;
                if (z0->next && z0->next->Z.yu != ylast->y_yu) {
                    ylast->next = new glYlist3d(z0->next, true);
                    ylast = ylast->next;
                    glZlist3d *zt = z0->next;
                    z0->next = 0;
                    z0 = zt;
                    continue;
                }
                z0 = z0->next;
            }
        }
    }
}


// Static function.
// Convert this to a glZlist3d, destructive.
//
glZlist3d *
glYlist3d::to_zl3d(glYlist3d *thisgl)
{
    glZlist3d *z0 = 0, *ze = 0;
    glYlist3d *yn;
    for (glYlist3d *y = thisgl; y; y = yn) {
        yn = y->next;
        if (y->y_zlist) {
            if (!z0)
                z0 = ze = y->y_zlist;
            else
                ze->next = y->y_zlist;
            while (ze->next)
                ze = ze->next;
            y->y_zlist = 0;
        }
        delete y;
    }
    return (z0);
}


// Static function.
//
glZgroup3d *
glYlist3d::group(glYlist3d *thisgl, int max_in_grp)
{
    struct gtmp_t { gtmp_t *next; glZlist3d *lst[256]; };
    gtmp_t *g0 = 0, *ge = 0;

    glYlist3d *yl0 = thisgl;

    int gcnt = 0;
    while (yl0) {
        glZlist3d *ze;
        yl0 = yl0->first(&ze);
        if (!ze)
            break;

        int gc = gcnt & 0xff;
        if (!gc) {
            if (!g0)
                g0 = ge = new gtmp_t;
            else {
                ge->next = new gtmp_t;
                ge = ge->next;
            }
            ge->next = 0;
            ge->lst[0] = ze;
        }
        else
            ge->lst[gc] = ze;

        gcnt++;

        int cnt = 1;
        for (glZlist3d *z = ze; z; z = z->next) {
            glZlist3d *zret = 0;
            if (yl0)
                yl0 = yl0->touching(&zret, &z->Z);
            ze->next = zret;
            if (!yl0)
                break;
            while (ze->next) {
                ze = ze->next;
                cnt++;
            }
            if (max_in_grp > 0 && cnt > max_in_grp)
                break;
        }
    }
    glZgroup3d *g = new glZgroup3d;
    g->num = gcnt;
    g->list = new glZlist3d*[gcnt+1];

    glZlist3d **p = g->list;
    for (gtmp_t *gt = g0; gt; gt = ge) {
        ge = gt->next;
        int n = mmMin(gcnt, 256);
        for (int i = 0; i < n; i++)
            *p++ = gt->lst[i];
        gcnt -= n;
        delete gt;
    }
    g->list[g->num] = 0;

    // Set the group number in all of the glZoid3ds.
    for (int i = 0; i < g->num; i++) {
        for (glZlist3d *zl = g->list[i]; zl; zl = zl->next)
            zl->Z.group = i;
    }
    return (g);
}


// Private support function for group().
// Remove elements that touch or overlap Z.
//
glYlist3d *
glYlist3d::touching(glZlist3d **zret, const Zoid3d *Z)
{
    *zret = 0;
    glYlist3d *yl0 = this, *yp = 0, *yn;
    ovlchk_t ovl(Z->minleft() - 1, Z->maxright() + 1, Z->yu + 1);
    for (glYlist3d *y = yl0; y; y = yn) {
        yn = y->next;
        if (y->y_yl > Z->yu) {
            yp = y;
            continue;
        }
        if (y->y_yu < Z->yl)
            break;
        glZlist3d *zp = 0, *zn;
        for (glZlist3d *z = y->y_zlist; z; z = zn) {
            zn = z->next;
            if (ovl.check_break(z->Z))
                break;
            if (ovl.check_continue(z->Z)) {
                zp = z;
                continue;
            }
            if (z->Z.zbot > Z->ztop || z->Z.ztop < Z->zbot) {
                zp = z;
                continue;
            }
            if (Z->touching(&z->Z)) {
                y->remove_next(zp, z);
                z->next = *zret;
                *zret = z;
                continue;
            }
            zp = z;
        }
        if (!y->y_zlist) {
            if (!yp)
                yl0 = yn;
            else
                yp->next = yn;
            delete y;
            continue;
        }
        yp = y;
    }
    return (yl0);
}


// Private support function for merge(), and others.
// Remove and return the top left element.  Empty rows are deleted.
//
glYlist3d *
glYlist3d::first(glZlist3d **zp)
{
    *zp = 0;
    glYlist3d *yl0 = this, *yn;
    for (glYlist3d *y = yl0; y; y = yn) {
        yn = y->next;
        if (y->y_zlist) {
            *zp = y->y_zlist;
            y->remove_next(0, y->y_zlist);
            (*zp)->next = 0;
        }
        if (!y->y_zlist) {
            yl0 = yn;
            delete y;
        }
        if (*zp)
            break;
    }
    return (yl0);
}
// End of glYlist3d functions.


void
glZoid3d::print(FILE *fp, double sc, const char *ff, const char *flg) const
{
    if (!fp)
        fp = stdout;
    char tbuf[32];

    sprintf(tbuf, "* %-3d", layer_index);
    fputs(tbuf, fp);
    char *e = tbuf + strlen(tbuf) - 1;
    if (!isspace(*e))
        putc(' ', fp);

    sprintf(tbuf, ff, sc*zbot);
    fputs(tbuf, fp);
    e = tbuf + strlen(tbuf) - 1;
    if (!isspace(*e))
        putc(' ', fp);

    sprintf(tbuf, ff, sc*ztop);
    fputs(tbuf, fp);
    e = tbuf + strlen(tbuf) - 1;
    if (!isspace(*e))
        putc(' ', fp);

    sprintf(tbuf, ff, sc*yl);
    fputs(tbuf, fp);
    e = tbuf + strlen(tbuf) - 1;
    if (!isspace(*e))
        putc(' ', fp);

    sprintf(tbuf, ff, sc*yu);
    fputs(tbuf, fp);
    e = tbuf + strlen(tbuf) - 1;
    if (!isspace(*e))
        putc(' ', fp);

    sprintf(tbuf, ff, sc*xll);
    fputs(tbuf, fp);
    e = tbuf + strlen(tbuf) - 1;
    if (!isspace(*e))
        putc(' ', fp);

    sprintf(tbuf, ff, sc*xul);
    fputs(tbuf, fp);
    e = tbuf + strlen(tbuf) - 1;
    if (!isspace(*e))
        putc(' ', fp);

    sprintf(tbuf, ff, sc*xlr);
    fputs(tbuf, fp);
    e = tbuf + strlen(tbuf) - 1;
    if (!isspace(*e))
        putc(' ', fp);

    if (flg && *flg) {
        sprintf(tbuf, ff, sc*xur);
        fputs(tbuf, fp);
        e = tbuf + strlen(tbuf) - 1;
        if (!isspace(*e))
            putc(' ', fp);
        fputs(flg, fp);
    }
    else {
        sprintf(tbuf, ff, sc*xur);
        e = tbuf + strlen(tbuf) - 1;
        while (isspace(*e))
            *e-- = 0;
        fputs(tbuf, fp);
    }
    fprintf(fp, "\n");
}


// The zoid must represent a right triangle, which is clipped into
// three new zoids.  The Manhattan part is added to zbox, if w and h
// >= mindim, and the triangular parts call this function recursively
// if big enough.  This effectively Manhattanizes the triangle.
//
void
glZoid3d::rt_triang(glZlist3d **zbox, int mindim)
{
    if (xul == xur) {
        int h = (yu - yl)/2;
        int d = (xlr - xll)/2;
        if (h < mindim || d < mindim)
            return;
        if (xll == xul) {
            glZlist3d *zn = new glZlist3d(this, *zbox);
            zn->Z.xlr = zn->Z.xur = xll + d;
            zn->Z.yu = yl + h;
            *zbox = zn;

            glZoid3d z1(*this);
            z1.xll = z1.xul = z1.xur = xll + d;
            z1.yu = yl + h;
            z1.rt_triang(zbox, mindim);

            glZoid3d z2(*this);
            z2.xlr = xll + d;
            z2.xul = z2.xur = xll;
            z2.yl = yl + h;
            z2.rt_triang(zbox, mindim);
        }
        else if (xlr == xur) {
            glZlist3d *zn = new glZlist3d(this, *zbox);
            zn->Z.xll = zn->Z.xul = xlr - d;
            zn->Z.yu = yl + h;
            *zbox = zn;

            glZoid3d z1(*this);
            z1.xlr = z1.xul = z1.xur = xlr - d;
            z1.yu = yl + h;
            z1.rt_triang(zbox, mindim);

            glZoid3d z2(*this);
            z2.xll = xlr - d;
            z2.xul = z2.xur = xlr;
            z2.yl = yl + h;
            z2.rt_triang(zbox, mindim);
        }
    }
    else if (xll == xlr) {
        int h = (yu - yl)/2;
        int d = (xur - xul)/2;
        if (h < mindim || d < mindim)
            return;
        if (xll == xul) {
            glZlist3d *zn = new glZlist3d(this, *zbox);
            zn->Z.xlr = zn->Z.xur = xll + d;
            zn->Z.yl = yl + h;
            *zbox = zn;

            glZoid3d z1(*this);
            z1.xll = z1.xlr = z1.xul = xll + d;
            z1.yl = yl + h;
            z1.rt_triang(zbox, mindim);

            glZoid3d z2(*this);
            z2.xul = xll;
            z2.xur = xll + d;
            z2.yu = yl + h;
            z2.rt_triang(zbox, mindim);
        }
        else if (xlr == xur) {
            glZlist3d *zn = new glZlist3d(this, *zbox);
            zn->Z.xll = zn->Z.xul = xlr - d;
            zn->Z.yl = yl + h;
            *zbox = zn;

            glZoid3d z1(*this);
            z1.xll = z1.xlr = z1.xur = xlr - d;
            z1.yl = yl + h;
            z1.rt_triang(zbox, mindim);

            glZoid3d z2(*this);
            z2.xll = z2.xur = xlr;
            z2.xul = xlr - d;
            z2.yu = yl + h;
            z2.rt_triang(zbox, mindim);
        }
    }
}
// End of glZoid3d functions.


namespace {
    inline bool zcmp(const glZlist3d *zl1, const glZlist3d *zl2)
    {
        const glZoid3d *z1 = &zl1->Z;
        const glZoid3d *z2 = &zl2->Z;
        return (z1->zcmp3d(z2) < 0);
    }
}


// Static function.
// Sort for glZlist3d.
//
glZlist3d *
glZlist3d::sort(glZlist3d *thisgl)
{
    glZlist3d *z0 = thisgl;
    if (z0 && z0->next) {
        int len = 0;
        for (glZlist3d *z = z0; z; z = z->next, len++) ;
        if (len == 2) {
            if (zcmp(z0, z0->next))
                return (z0);
            glZlist3d *zt = z0->next;
            zt->next = z0;
            z0->next = 0;
            return (zt);
        }
        glZlist3d **tz = new glZlist3d*[len];
        int i = 0;
        for (glZlist3d *z = z0; z; z = z->next, i++)
            tz[i] = z;
        std::sort(tz, tz + len, zcmp);
        len--;
        for (i = 0; i < len; i++)
            tz[i]->next = tz[i+1];
        tz[i]->next = 0;
        z0 = tz[0];
        delete [] tz;
    }
    return (z0);
}


// Static function.
// Convert this (which is destroyed) to a Manhattan representation.
// The returned list consists of rectangles only.  Rectangles added
// from triangular parts have w and h <= mindim.
//
glZlist3d *
glZlist3d::manhattanize(glZlist3d *thisgl, int mindim)
{
   if (mindim < 10)
        mindim = 10;
    glZlist3d *z0 = 0, *z = thisgl;

    // This version decomposes the zoids into right-triangles, then
    // recursively cuts out the Manhattan parts.  The coordinates are
    // not forced to any grid, and the rectangular dimensions vary.

    while (z) {
        int xrmin = mmMin(z->Z.xlr, z->Z.xur);
        int xrmax = mmMax(z->Z.xlr, z->Z.xur);
        int xlmin = mmMin(z->Z.xll, z->Z.xul);
        int xlmax = mmMax(z->Z.xll, z->Z.xul);
        if (xrmin >= xlmax) {
            if (xrmin - xlmax >= mindim) {
                z0 = new glZlist3d(&z->Z, z0);
                z0->Z.xll = z0->Z.xul = xlmax;
                z0->Z.xlr = z0->Z.xur = xrmin;
            }
            else if (((xrmin == xrmax && xlmin == xlmax) ||
                    xrmax - xrmin >= mindim ||
                    xlmax - xlmin >= mindim) && xrmin > xlmax) {
                // keep this if there will be an adjacent zoid, to avoid
                // breaking continuity
                z0 = new glZlist3d(&z->Z, z0);
                z0->Z.xll = z0->Z.xul = xlmax;
                z0->Z.xlr = z0->Z.xur = xrmin;
            }
            if (xrmax - xrmin >= mindim) {
                glZoid3d z1(z->Z);
                z1.xll = z1.xul = xrmin;
                z1.rt_triang(&z0, mindim);
            }
            if (xlmax - xlmin >= mindim) {
                glZoid3d z1(z->Z);
                z1.xlr = z1.xur = xlmax;
                z1.rt_triang(&z0, mindim);
            }
            glZlist3d *zt = z->next;
            delete z;
            z = zt;
        }
        else {
            double sl = z->Z.slope_left();
            double sr = z->Z.slope_right();

            if (z->Z.xur - z->Z.xul < z->Z.xlr - z->Z.xll) {
                int h;
                if (z->Z.xur < z->Z.xll)
                    h = mmRnd((z->Z.xll - z->Z.xlr)/sr);
                else
                    h = mmRnd((z->Z.xlr - z->Z.xll)/sl);
                if (h < mindim) {
                    glZlist3d *zt = z->next;
                    delete z;
                    z = zt;
                    continue;
                }

                int xl = mmRnd(z->Z.xll + h*sl);
                if (xl > z->Z.xlr)
                    xl = z->Z.xlr;
                int xr = mmRnd(z->Z.xlr + h*sr);
                if (xr < z->Z.xll)
                    xr = z->Z.xll;
                glZlist3d *zx = new glZlist3d(&z->Z, 0);
                zx->Z.xul = xl;
                zx->Z.xur = xr;
                zx->Z.yu = z->Z.yl + h;
                z->Z.yl += h;
                z->Z.xll = xl;
                z->Z.xlr = xr;
                zx->next = z;
                z = zx;
            }
            else {
                int h;
                if (z->Z.xul > z->Z.xlr)
                    h = mmRnd((z->Z.xur - z->Z.xul)/sr);
                else
                    h = mmRnd((z->Z.xul - z->Z.xur)/sl);
                if (h < mindim) {
                    glZlist3d *zt = z->next;
                    delete z;
                    z = zt;
                    continue;
                }

                int xl = mmRnd(z->Z.xul - h*sl);
                if (xl > z->Z.xur)
                    xl = z->Z.xur;
                int xr = mmRnd(z->Z.xur - h*sr);
                if (xr < z->Z.xul)
                    xr = z->Z.xul;
                glZlist3d *zx = new glZlist3d(&z->Z, 0);
                zx->Z.xll = xl;
                zx->Z.xlr = xr;
                zx->Z.yl = z->Z.yu - h;
                z->Z.yu -= h;
                z->Z.xul = xl;
                z->Z.xur = xr;
                zx->next = z;
                z = zx;
            }
        }
    }
    return (z0);
}
// End glZlist3d functions.


glZgroupRef3d::glZgroupRef3d(const glZgroup3d *g)
{
    if (!g) {
        num = 0;
        list = 0;
        return;
    }
    num = g->num;
    list = new glZlistRef3d*[num];
    for (int i = 0; i < num; i++) {
        list[i] = 0;
        glZlistRef3d *e = 0;
        for (const glZlist3d *z = g->list[i]; z; z = z->next) {
            glZlistRef3d *zr = new glZlistRef3d(&z->Z);
            if (!e)
                list[i] = e = zr;
            else {
                e->next = zr;
                e = e->next;
            }
        }
    }
}
// End glZgroupRef3d functions.

