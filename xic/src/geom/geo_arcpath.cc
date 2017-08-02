
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

#include "geo.h"
#include "geo_point.h"


#ifndef M_PI
#define M_PI        3.14159265358979323846  // pi
#endif

#define SPOT_MAX 50

namespace {
    // Radius of an ellipse as a function of the center angle
    // relative to the x-axis.
    //
    inline double rad(double ang, int rx, int ry)
    {
        double drx = rx;
        double dry = ry;
        double T1 = dry*cos(ang);
        double T2 = drx*sin(ang);
        return (drx*dry/sqrt(T1*T1 + T2*T2));
    }
}


// Create an arc path.  If the angles are equal, falls back to a round
// path if wid is zero, or donut otherwise.
//
// If geoSpotSize is positive and elec is false, disks and donuts are
// created using an algorithm which places all vertices at the center
// of a spot, and uses a minimal number of vertices.  The
// geoRoundFlashSides is ignored in this case.
//
Point *
cGEO::makeArcPath(int *numpts, bool elec, int cen_x, int cen_y,
    int rad1x, int rad1y, int rad2x, int rad2y,
    double ang1, double ang2) const
{
    int sptsize = (!elec && geoSpotSize > 0) ? geoSpotSize : 0;
    int rndsides = elec ? geoElecRoundSides : geoPhysRoundSides;

    Point *points = 0;
    if (ang1 == ang2 || fabs(fabs(ang1 - ang2) - 2*M_PI) < 1e-10) {
        if (rad2x == 0 && rad2y == 0) {
            // round flash path

            if (sptsize) {
                rad1x = ((rad1x + sptsize/2)/sptsize)*sptsize;
                rad1y = ((rad1y + sptsize/2)/sptsize)*sptsize;
                int spmax = SPOT_MAX*sptsize;
                if (rad1x == 0 || rad1y == 0 ||
                        (rad1x > spmax && rad1y > spmax))
                    goto normal_round;

                // All vertices will be centered on a spot
                cen_x = (cen_x/sptsize)*sptsize + sptsize/2;
                cen_y = (cen_y/sptsize)*sptsize + sptsize/2;

                double r1x2 = rad1x*(double)rad1x;
                double r1y2 = rad1y*(double)rad1y;
                int ix = mmRnd(sqrt(r1x2*r1x2/(r1x2+r1y2)));
                int iy = mmRnd(sqrt(r1y2*r1y2/(r1y2+r1x2)));
                ix /= sptsize;
                iy /= sptsize;
                if (!ix)
                    ix = 1;
                if (!iy)
                    iy = 1;

                int np = ix + iy + 1;
                *numpts = 4*np + 1;
                points = new Point[*numpts];

                for (int i = 0; i <= iy; i++) {
                    int ny = i*sptsize;
                    int nx = mmRnd(sqrt(r1x2*(1.0 - ny*ny/r1y2)));
                    nx = ((nx + sptsize/2)/sptsize)*sptsize;

                    points[np - i].set(cen_x + nx, cen_y + ny);
                    points[np + i].set(cen_x + nx, cen_y - ny);
                    points[3*np - i].set(cen_x - nx, cen_y - ny);
                    points[3*np + i].set(cen_x - nx, cen_y + ny);
                }
                for (int i = 0; i <= ix; i++) {
                    int nx = i*sptsize;
                    int ny = mmRnd(sqrt(r1y2*(1.0 - nx*nx/r1x2)));
                    ny = ((ny + sptsize/2)/sptsize)*sptsize;

                    points[i].set(cen_x + nx, cen_y + ny);
                    points[2*np - i].set(cen_x + nx, cen_y - ny);
                    points[2*np + i].set(cen_x - nx, cen_y - ny);
                    points[4*np - i].set(cen_x - nx, cen_y + ny);
                }
            }
            else {
normal_round:
                double DPhi = 2*M_PI/rndsides;
                *numpts = rndsides + 1;
                points = new Point[*numpts];
                double ang = (rndsides & 1) ? 0.0 : DPhi/2.0;
                for (int i = 0; i < rndsides; ang += DPhi, i++)
                    points[i].set(mmRnd(cen_x + rad1x*sin(ang)),
                        mmRnd(cen_y + rad1y*cos(ang)));
                points[rndsides] = points[0];
            }
        }
        else {
            // donut path

            if (sptsize) {
                rad1x = ((rad1x + sptsize/2)/sptsize)*sptsize;
                rad1y = ((rad1y + sptsize/2)/sptsize)*sptsize;
                if (rad1x == 0 || rad1y == 0)
                    goto normal_donut;
                rad2x = ((rad2x + sptsize/2)/sptsize)*sptsize;
                rad2y = ((rad2y + sptsize/2)/sptsize)*sptsize;
                if (rad2x == 0 || rad2y == 0)
                    goto normal_donut;
                int spmax = SPOT_MAX*sptsize;
                if (rad1x > spmax && rad1y > spmax &&
                        rad2x > spmax && rad2y > spmax)
                    goto normal_donut;

                // All vertices will be centered on a spot
                cen_x = (cen_x/sptsize)*sptsize + sptsize/2;
                cen_y = (cen_y/sptsize)*sptsize + sptsize/2;

                double r1x2 = rad1x*(double)rad1x;
                double r1y2 = rad1y*(double)rad1y;
                int ix1 = mmRnd(sqrt(r1x2*r1x2/(r1x2+r1y2)));
                int iy1 = mmRnd(sqrt(r1y2*r1y2/(r1y2+r1x2)));
                ix1 /= sptsize;
                iy1 /= sptsize;
                if (!ix1)
                    ix1 = 1;
                if (!iy1)
                    iy1 = 1;
                double r2x2 = rad2x*(double)rad2x;
                double r2y2 = rad2y*(double)rad2y;
                int ix2 = mmRnd(sqrt(r2x2*r2x2/(r2x2+r2y2)));
                int iy2 = mmRnd(sqrt(r2y2*r2y2/(r2y2+r2x2)));
                ix2 /= sptsize;
                iy2 /= sptsize;
                if (!ix2)
                    ix2 = 1;
                if (!iy2)
                    iy2 = 1;

                *numpts = 4*(ix1+iy1+1) + 4*(ix2+iy2+1) + 3;
                points = new Point[*numpts];

                int np = ix1 + iy1 + 1;
                Point *pts = points;
                for (int i = 0; i <= iy1; i++) {
                    int ny = i*sptsize;
                    int nx = mmRnd(sqrt(r1x2*(1.0 - ny*ny/r1y2)));
                    nx = ((nx + sptsize/2)/sptsize)*sptsize;

                    pts[np - i].set(cen_x + nx, cen_y + ny);
                    pts[np + i].set(cen_x + nx, cen_y - ny);
                    pts[3*np - i].set(cen_x - nx, cen_y - ny);
                    pts[3*np + i].set(cen_x - nx, cen_y + ny);
                }
                for (int i = 0; i <= ix1; i++) {
                    int nx = i*sptsize;
                    int ny = mmRnd(sqrt(r1y2*(1.0 - nx*nx/r1x2)));
                    ny = ((ny + sptsize/2)/sptsize)*sptsize;

                    pts[i].set(cen_x + nx, cen_y + ny);
                    pts[2*np - i].set(cen_x + nx, cen_y - ny);
                    pts[2*np + i].set(cen_x - nx, cen_y - ny);
                    pts[4*np - i].set(cen_x - nx, cen_y + ny);
                }

                pts = points + 4*np + 1;
                np = ix2 + iy2 + 1;
                for (int i = 0; i <= iy2; i++) {
                    int ny = i*sptsize;
                    int nx = mmRnd(sqrt(r2x2*(1.0 - ny*ny/r2y2)));
                    nx = ((nx + sptsize/2)/sptsize)*sptsize;
                    nx = -nx;

                    pts[np - i].set(cen_x + nx, cen_y + ny);
                    pts[np + i].set(cen_x + nx, cen_y - ny);
                    pts[3*np - i].set(cen_x - nx, cen_y - ny);
                    pts[3*np + i].set(cen_x - nx, cen_y + ny);
                }
                for (int i = 0; i <= ix2; i++) {
                    int nx = i*sptsize;
                    int ny = mmRnd(sqrt(r2y2*(1.0 - nx*nx/r2x2)));
                    ny = ((ny + sptsize/2)/sptsize)*sptsize;
                    nx = -nx;

                    pts[i].set(cen_x + nx, cen_y + ny);
                    pts[2*np - i].set(cen_x + nx, cen_y - ny);
                    pts[2*np + i].set(cen_x - nx, cen_y - ny);
                    pts[4*np - i].set(cen_x - nx, cen_y + ny);
                }
                pts[4*np + 1] = points[0];
            }
            else {
normal_donut:
                int sides = rndsides;
                double DPhi = 2*M_PI/sides;
                *numpts = 2*sides + 3;
                points = new Point[*numpts];
                double a1 = (rndsides & 1) ? 0.0 : DPhi/2.0;
                double a2 = a1;
                for (int i = 0; i < sides; i++) {
                    points[i].set(mmRnd(cen_x + rad1x*sin(a1)),
                        mmRnd(cen_y + rad1y*cos(a1)));
                    a1 += DPhi;
                    points[i+sides+1].set(mmRnd(cen_x + rad2x*sin(a2)),
                        mmRnd(cen_y + rad2y*cos(a2)));
                    a2 -= DPhi;
                }
                points[sides] = points[0];
                points[sides+sides+1] = points[sides+1];
                points[sides+sides+2] = points[0];
            }
        }
    }
    else {
        // arc path
        if (ang1 < ang2)
            ang1 += 2*M_PI;

        int sides = (int)(((ang1 - ang2)/(2*M_PI))*rndsides);
        if (sides < 3)
            sides = 3;

        // There is a subtlety here in that the angle that we have
        // used to parameterize ellipses above is not the true angle. 
        // For an arc, the true angles should be used, which is a bit
        // more complicated.  The rad function gives the effective
        // radius as a function of true angle.

        if (rad2x == 0 && rad2y == 0) {
            double DPhi = (ang1 - ang2)/sides;
            *numpts = sides + 3;
            points = new Point[*numpts];

            double T = rad(ang2, rad1x, rad1y);
            points[0].set(mmRnd(cen_x + T*cos(ang2)),
                mmRnd(cen_y + T*sin(ang2)));

            T = rad(ang1, rad1x, rad1y);
            points[sides].set(mmRnd(cen_x + T*cos(ang1)),
                mmRnd(cen_y + T*sin(ang1)));
            points[sides+1].set(cen_x, cen_y);
            points[sides+2] = points[0];

            double A1 = ang2;
            double A2 = ang1;
            for (int i = 1; i < sides; i++) {
                A1 += DPhi;
                A2 -= DPhi;
                T = rad(A1, rad1x, rad1y);
                points[i].set(mmRnd(cen_x + T*cos(A1)),
                    mmRnd(cen_y + T*sin(A1)));
            }
        }
        else {
            double DPhi = (ang1 - ang2)/sides;
            *numpts = 2*sides + 3;
            points = new Point[*numpts];

            double T = rad(ang2, rad1x, rad1y);
            points[0].set(mmRnd(cen_x + T*cos(ang2)),
                mmRnd(cen_y + T*sin(ang2)));

            T = rad(ang1, rad1x, rad1y);
            points[sides].set(mmRnd(cen_x + T*cos(ang1)),
                mmRnd(cen_y + T*sin(ang1)));

            T = rad(ang1, rad2x, rad2y);
            points[sides+1].set(mmRnd(cen_x + T*cos(ang1)),
                mmRnd(cen_y + T*sin(ang1)));

            T = rad(ang2, rad2x, rad2y);
            points[sides+sides+1].set(mmRnd(cen_x + T*cos(ang2)),
                mmRnd(cen_y + T*sin(ang2)));
            points[sides+sides+2] = points[0];

            double A1 = ang2;
            double A2 = ang1;
            for (int i = 1; i < sides; i++) {
                A1 += DPhi;
                A2 -= DPhi;
                T = rad(A1, rad1x, rad1y);
                points[i].set(mmRnd(cen_x + T*cos(A1)),
                    mmRnd(cen_y + T*sin(A1)));

                T = rad(A2, rad2x, rad2y);
                points[i+sides+1].set(mmRnd(cen_x + T*cos(A2)),
                    mmRnd(cen_y + T*sin(A2)));
            }
        }
    }
    return (points);
}

