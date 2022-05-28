// the simpicial method

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "qnrutil.h"
#include "param.h"
#include "yield.h"
#include "optimizer.h"

#include "simulator.h"

// Minimum distance out from old plane.
#define STEP (param.accuracy()/200.0)

// Distance out from search boundary.
#define OUT  (param.accuracy()/200.0)

#define DEFINED  1.0e-18
#define INSCRIBE 1.0e-8


int
OPTIMIZER::opt_begin()
{
    // Read input file, create and initialize global arrays.
    readinit();

    for (int x = 1; x <= o_dim; x++)
        o_maxplane *= 2;
    o_ab = matrix(1, o_maxplane, 0, o_dim);
    o_abpnts = imatrix(1, o_maxplane, 0, o_dim);
    o_hullpnts = matrix(1, param.maxiter() + 3*o_dim + 1, 1, o_dim);
    o_aNmatrix = matrix(1, o_dim, 1, o_dim);
    o_temp = vector(0, o_dim);

    // Open the output file.
    char outfile[LINE_LENGTH];
    strcpy(outfile, param.output_dir());
    strcat(outfile, param.circuit_name());
    strcat(outfile, param.opt_ext());
    strcat(outfile, param.opt_num());

    if ((o_fpout = fopen(outfile,"w")) == NULL) {
        printf("Can't write to the '");
        printf(outfile);
        printf("' file.\n");
        exit(255);
    }

    // Check values of MIN_ITER and MAX_ITER.
    if (param.miniter() > param.maxiter()) {
        out_screen_file(
            "MIN_ITER = %d is greater than MAX_ITER = %d.  MAX_ITER will "
            "supercede.\n",
            param.miniter(), param.maxiter());
    }

    // Initialize local arrays.
    o_facecenter = vector(1, o_dim);
    o_radius = vector(0, o_dim+1);
    for (int x = 0; x <= o_dim+1; x++)
        o_radius[x] = 0.0;
    o_direction = vector(1, o_dim);
    for (int x = 1; x <= o_dim; x++)
        o_direction[x] = 0.0;
    o_pntstack = ivector(1, o_dim);
    o_pc = vector(1, o_dim);
    o_po = vector(0, o_dim);

    // Find initial dim*2 points, print margins.
    // Turn the margins into points, print margins.
    out_screen_file("Margins calculated to within +/-%5.2f percent.\n",
        param.accuracy());
    out_screen_file(
        "        Margins:                                 Percent:\n");
    for (int x = 1; x <= o_dim; x++) {
        for (int y = 1; y <= o_dim; y++)
            o_pc[y] = o_po[y] = o_centerpnt[y];
        o_pc[x] = o_centerpnt[x]-STEP/o_scale[x];
        o_po[x] = o_lower[x]-OUT/o_scale[x];
        if (!addpoint())
            nrerror("parameter values failed");
        else {
            out_screen_file("%s\t%9.5f%s...%9.5f ...",
                o_names[x-1], o_hullpnts[o_pntcount][x]*o_scale[x],
                (o_hullpnts[o_pntcount][x] > o_lower[x]) ? "*" : " ",
                o_centerpnt[x]*o_scale[x]);
        }
        o_pc[x] = o_centerpnt[x] + STEP/o_scale[x];
        o_po[x] = o_upper[x] + OUT/o_scale[x];
        if (!addpoint())
            nrerror("parameter values failed");
        else {
            out_screen_file("%9.5f%s     -%5.1f  +%5.1f\n",
                o_hullpnts[o_pntcount][x]*o_scale[x],
                (o_hullpnts[o_pntcount][x] > o_upper[x]) ? "*" : " ",
                100.0*(o_centerpnt[x] - o_hullpnts[o_pntcount-1][x])/
                    o_centerpnt[x],
                100.0*(o_hullpnts[o_pntcount][x] - o_centerpnt[x])/
                    o_centerpnt[x]);
        }
    }

    // Pick combinations of dim points and make the hull.
    intpickpnts(1, o_pntstack);
    out_screen_file("hull planes: %i total\n\n", o_plncount);

    return (opt_loop());
}


namespace {
    bool check_terminate()
    {
        if (Sp.GetFlag(FT_INTERRUPT)) {
            Sp.SetFlag(FT_INTERRUPT, false);
            return (true);
        }
        return (false);
    }
}


int
OPTIMIZER::opt_loop()
{
    o_iterate = 0;
    o_direcount = 0;
    int oldcount = 0, lowcount = 0;
    do {
        // Stick a sphere inside the scaled hull (same as an ellipse).
        out_screen_file("%i\n", ++o_iterate);

        int big = findface(o_facecenter, o_radius);
        if (!big) {
            out_screen_file("Could not expand in any critical direction.\n");
            o_stop = 2;
        }

        if (!o_stop) {
            // Print the results.
            out_screen_file("center    ");
            for (int x = 1; x <= o_dim; x++)
                out_screen_file("%9.4f ", o_centerpnt[x]*o_scale[x]);
            out_screen_file("\naxes      ");
            for (int x = 1; x <= o_dim; x++)
                out_screen_file("%9.3f ", o_radius[0]*o_scale[x]);
            out_screen_file("\ndirection ");
            for (int x = 1; x <= o_dim; x++) {
                double c = -o_ab[big][x];
                out_screen_file("%9.3f ", c);
                o_direction[x] += c*c;
            }
            out_screen_file("\n");
            ++o_direcount;

            // Find the closest boundary and calculate the search points.
            double cbig = 0.0;
            for (int x = 1; x <= o_dim; x++) {
                double bound = ((o_ab[big][x] > 0.0) ? o_lower[x] : o_upper[x]);
                double c = o_ab[big][x]/(o_facecenter[x] - bound);
                if (c > cbig)
                    cbig = c;
            }

            // pc on plane, po on the boundary
            // In 1-D step=STEP/o_scale[x]
            // In 2-D step equals smallest of the above
            double step = STEP/o_scale[o_dim+1];
            for (int x = 2; x <= o_dim; x++) {
                if (STEP/o_scale[x] < step)
                    step = STEP/o_scale[x];
            }

            // Check that we are not against search boundary.
            for (int x = 1; x <= o_dim; x++) {
                double out = o_facecenter[x] - 2*step*o_ab[big][x];
                if ((out >= o_upper[x]) || (out <= o_lower[x]))
                    o_abpnts[big][0] = 1;
            }
            if (o_abpnts[big][0])
                out_screen_file("Could not expand in above direction.\n");
            else {
 
                // pc and po shifted out a little bit
                for (int x = 1; x <= o_dim; x++) {
                    o_pc[x] = o_facecenter[x] - step*o_ab[big][x];
                    o_po[x] = o_facecenter[x] - o_ab[big][x]/cbig -
                        OUT/o_scale[x]*o_ab[big][x];
                }
                lowcount = o_plncount;
                oldcount = o_plncount;

                // Flag plane if not convex.
                if (!addpoint()) {
                    o_abpnts[big][0] = 1;
                    out_screen_file("New point is not outside hull.\n");
                }
                else {
                    // Print the new point.
                    out_screen_file("new point ");
                    for (int x = 1; x <= o_dim; x++) {
                        out_screen_file("%9.3f%s",
                        o_hullpnts[o_pntcount][x]*o_scale[x],
                        (o_hullpnts[o_pntcount][x]<o_lower[x]) ||
                            (o_hullpnts[o_pntcount][x] > o_upper[x]) ?
                            "*" : " ");
                    }
                    out_screen_file("\n");

                    // Flag planes the new point is exterior to.
                    for (int y = 1; y <= oldcount; y++) {
                        double distance = o_ab[y][0];
                        for (int x = 1; x <= o_dim; x++)
                            distance += o_ab[y][x]*o_hullpnts[o_pntcount][x];
                        if (distance < 0.0) {
                            o_abpnts[y][0] = 2;
                            --lowcount;
                        }
                    }

                    // Find new planes, using new point and old plane
                    // combinations.
                    for (int xx = 1; xx <= oldcount; xx++) {
                        if (o_abpnts[xx][0] == 2) {
                            for (int x = 1; x <= o_dim; x++) {
                                for (int y = 1; y <= o_dim; y++)
                                    o_pntstack[y] = o_abpnts[xx][y];
                                o_pntstack[x] = o_pntcount;
                                makeaplane(o_pntstack);
                                // memory
                                if (o_plncount == o_maxplane)
                                    make_more_mem();
                            }
                        }
                    }

                    // Delete old planes.
                    for (int y = 1; y <= oldcount; ) {
                        if (o_abpnts[y][0] == 2) {
                            for (int x = 0; x <= o_dim; x++) {
                                o_ab[y][x] = o_ab[o_plncount][x];
                                o_abpnts[y][x] = o_abpnts[o_plncount][x];
                            }
                            --o_plncount;
                        }
                        else
                            y++;
                    }

                }
            }
        }
        out_screen_file("hull planes: %i deleted, %i added, %i total\n",
            oldcount-lowcount, o_plncount-lowcount, o_plncount);

        // Check if loop should terminate.
        // Maximum number of iterations.
        if (o_iterate == param.maxiter()) {
            o_stop = 2;
            out_screen_file("Number of iterations equals MAX_ITER.\n");
        }

        // Maximum number of planes.
        if (o_stop == 3)
            out_screen_file("Number of hull planes exceeds MAX_PLANE.\n");

        // Radius.
        if (!(o_stop>=2)&&(o_radius[0] - o_radius[o_dim+1])/o_radius[0] <
                STEP*2) {
            o_stop = 1;
            out_screen_file("Axes of inscribed ellipse increasing slowly.\n");
        }
        for (int x = o_dim+1; x >= 1; x--)
            o_radius[x] = o_radius[x-1];

        // Minimum number of iterations.
        if (!(o_stop >= 2) && param.miniter() > o_iterate)
            o_stop = 0;

        // Check user request to terminate.
        if (check_terminate()) {
            o_stop = 2;
            out_screen_file("Termination requested\n");
        }
    } while (!o_stop);

    return (opt_result());
}


int
OPTIMIZER::opt_result()
{
    out_screen_file("\nResults.");

    // Inscribe one more time.
    int *tang = ivector(1, o_dim+1-o_pin);
    postcenter(tang, o_radius);
    out_screen_file("\nCENTER    ");
    for (int x = 1; x <= o_dim; x++)
        out_screen_file("%9.4f ", o_centerpnt[x]*o_scale[x]);
    out_screen_file("\nAXES      ");
    for (int x = 1; x <= o_dim; x++)
        out_screen_file("%9.3f ", o_radius[0]*o_scale[x]);
    out_screen_file("\nAVE DIRECTION ");
    for (int x = 1; x <= o_dim; x++)
        out_screen_file("%5.3f     ", sqrt(o_direction[x]/o_direcount));

    out_screen_file("\n\ncriticalness ");
    for (int x = 1; x <= o_dim; x++) {
        o_direction[x] = 0.0;
        for (int y = 1; y <= o_dim+1-o_pin; y++)
            o_direction[x] += (o_ab[tang[y]][x])*(o_ab[tang[y]][x]);
        out_screen_file("%6.3f    ", sqrt(o_direction[x]/(o_dim+1-o_pin)));
    }

    // Convexity check along critical vectors.
    out_screen_file(
        "\n\ndistance to boundary (normalized) ... critical direction\n");
    for (int y = 1; y <= o_dim+1-o_pin; y++) {
        for (int x = 1; x <= o_dim; x++) {
            // Find the closest boundary and calculate the search points.
            double cbig = 0.0;
            for (int xx = 1; xx <= o_dim; xx++) {
                double bound =
                    (o_ab[tang[y]][xx] > 0.0) ? o_lower[xx] : o_upper[xx];
                double c = o_ab[tang[y]][xx]/(o_centerpnt[xx]-bound);
                if (c > cbig)
                    cbig = c;
            }
            // pc at center, po on boundary
            o_pc[x] = o_centerpnt[x];
            o_po[x] = o_centerpnt[x] - o_ab[tang[y]][x]/cbig -
                OUT/o_scale[x]*o_ab[tang[y]][x];
        }
        // Find boundary.
        if (!addpoint())
            nrerror("parameter values failed");
        else {
            double dist = 0.0;
            for (int x = 1; x <= o_dim; x++)
                dist += (o_hullpnts[o_pntcount][x] -
                    o_centerpnt[x])*(o_hullpnts[o_pntcount][x] - o_centerpnt[x]);
            dist = sqrt(dist);
            out_screen_file("%6.4f  ... ", (dist/o_radius[0]));
            for (int x = 1; x <= o_dim; x++)
                out_screen_file("%7.3f   ",-o_ab[tang[y]][x]);
        }
        out_screen_file("\n");
    }
    out_screen_file("\n");

    // Yield calculations assuming three and one sigma.
    long seed = -1;
    if (o_var > 0) {
        out_screen_file("Yield calculation for parameter(s):\t");
        for (int x = 1; x <= o_dim; x++) {
            if (o_yield[x])
                out_screen_file("%s\t", o_names[x-1]);
        }

        out_screen_file("\nAssuming 1 sigma variations of    :\t");
        for (int x = 1; x <= o_dim; x++) {
            if (o_yield[x])
                out_screen_file("%4.1f\t", 1.0/(3.0*o_centerpnt[x]));
        }
                
        // Ellipse estimate.
        double yest3 = multiarea(300.0*o_radius[0], o_var);
        double yest1 = multiarea(100.0*o_radius[0], o_var);

        // Hull estimate.
        long tick = 0;
        double *ypnt = vector(1, o_dim);
        double total3 = 0.0;
        double total1 = 0.0;
        double ycount3 = 0.0;
        double ycount1 = 0.0;
        for (long z = 1; z <= 10000 && tick <= 1000 && (z-tick) <= 1000; z++) {
            int outside = 0;
            double gpdf3 = 1.0;
            double gpdf1 = 1.0;
            for (int x = 1; x <= o_dim; x++) {
                ypnt[x] = (o_yield[x]) ?
                    o_lower[x] + (o_upper[x] -
                        o_lower[x])*uniform_deviate(&seed) :
                    o_centerpnt[x];
                gpdf3 *= normal(300.0*(ypnt[x]-o_centerpnt[x]));
                gpdf1 *= normal(100.0*(ypnt[x]-o_centerpnt[x]));
            }
            for (int y = 1; y <= o_plncount && !outside; y++) {
                double distance = o_ab[y][0];
                for (int x = 1; x <= o_dim; x++)
                    distance += o_ab[y][x]*ypnt[x];
                if (distance < 0.0)
                    outside = 1;
            }
            if (!outside) {
                ycount3 += gpdf3;
                ycount1 += gpdf1;
                ++tick;
            }
            total3 += gpdf3;
            total1 += gpdf1;
        }
        double gint3 = 1.0;
        double gint1 = 1.0; 
        for (int x = 1; x <= o_dim; x++) {
            gint3 *= area(300.0*(o_upper[x] - o_centerpnt[x]),
                300.0*(o_centerpnt[x] - o_lower[x]));
            gint1 *= area(100.0*(o_upper[x] - o_centerpnt[x]),
                100.0*(o_centerpnt[x] - o_lower[x]));
        }
        out_screen_file(
            "\nYield within ellipse, yield within hull:  %7.3f  %5.1f",
            100.0*yest3, 100.0*gint3*(ycount3/total3));
        out_screen_file("\n\nAssuming 1 sigma variations of    :\t");
        for (int x = 1; x <= o_dim; x++) {
            if (o_yield[x])
                out_screen_file("%4.1f\t",1.0/o_centerpnt[x]);
        }

        out_screen_file(
            "\nYield within ellipse, yield within hull:  %7.3f  %5.1f\n\n",
            100.0*yest1, 100.0*gint1*(ycount1/total1));
    }
    fclose(o_fpout);
    return (0);
}


void
OPTIMIZER::postcenter(int *tang, double *radius)
{
    // Inscribe the sphere.
    // Initialize the input tabeau.
    double **tab = matrix(1, o_dim+3, 1, o_plncount+1+2*o_pin);
    int *left = ivector(1, o_dim+1);
    int *right = ivector(1, o_plncount+2*o_pin);
 
    for (int y = 0; y <= o_dim; y++)
        tab[y+1][1] = 0.0;
    tab[o_dim+2][1] = 1.0;
    for (int x = 1; x <= o_plncount; x++)
        tab[1][x+1] = -o_ab[x][0];
    for (int x = 1; x <= o_pin; x++) {
        tab[1][x*2+o_plncount] = -o_centerpnt[o_xyz[x]]-INSCRIBE;
        tab[1][x*2+1+o_plncount] = o_centerpnt[o_xyz[x]]-INSCRIBE;
    }
 
    for (int x = 1; x <= o_plncount; x++)
        tab[o_dim+2][x+1] = -1.0;
    for (int x = o_plncount+1; x <= o_plncount+2*o_pin; x++)
        tab[o_dim+2][x+1] = 0.0;
 
    for (int y = 1; y <= o_dim; y++) {
        for (int x = 1; x <= o_plncount; x++)
            tab[y+1][x+1] = o_ab[x][y];
    }
    for (int y = 1; y <= o_dim; y++) {
        for (int x = 1; x <= o_pin; x++) {
            tab[y+1][x*2+o_plncount] = (y == o_xyz[x] ? -1.0 : 0.0);
            tab[y+1][x*2+1+o_plncount] = (y == o_xyz[x] ? 1.0 : 0.0);
        }
    }

    // Call the linear program.
    if (simplx(tab, o_dim+1, o_plncount+2*o_pin, right, left) != 0)
        nrerror("inscribing in hull failed");

    radius[0] = -tab[1][1];
    for (int y = 1; y <= o_dim; y++) {
        for (int x = 1; x <= o_plncount+2*o_pin; x++) {
            if (right[x] == y+o_plncount+2*o_pin)
                o_centerpnt[y] = -tab[1][x+1];
        }
    }
    int x = 0;
    for (int y = 1; y <= o_dim+1; y++) {
        if (left[y] <= o_plncount)
            tang[++x]=left[y];
    }

    free_matrix(tab, 1, o_dim+3, 1, o_plncount+1+2*o_pin);
    free_ivector(left, 1, o_dim+1);
    free_ivector(right, 1, o_plncount+2*o_pin);
}


int
OPTIMIZER::findface(double *centface, double *radius)
{
    // Inscribe the sphere.
    // Initialize the input tabeau.
    double **tab = matrix(1, o_dim+3, 1, o_plncount+1+2*o_pin);
    int *left = ivector(1, o_dim+1);
    int *right = ivector(1, o_plncount+2*o_pin);
    double *sine = vector(1, o_dim+1);
    int *tang = ivector(1, o_dim+1-o_pin);
    int *intsect = ivector(1, o_dim+1);

    for (int y = 0; y <= o_dim; y++)
        tab[y+1][1] = 0.0;
    tab[o_dim+2][1] = 1.0;
    for (int x = 1; x <= o_plncount; x++)
        tab[1][x+1] = -o_ab[x][0];
    for (int x = 1; x <= o_pin; x++) {
        tab[1][x*2 + o_plncount] = -o_startpnt[o_xyz[x]] - INSCRIBE;
        tab[1][x*2+1 + o_plncount] = o_startpnt[o_xyz[x]] - INSCRIBE;
    }

    for (int x = 1; x <= o_plncount; x++)
        tab[o_dim+2][x+1] = -1.0;
    for (int x = o_plncount+1; x <= o_plncount+2*o_pin; x++)
        tab[o_dim+2][x+1] = 0.0;

    for (int y = 1; y <= o_dim; y++) {
        for (int x = 1; x <= o_plncount; x++)
            tab[y+1][x+1] = o_ab[x][y];
    }
    for (int y = 1; y <= o_dim; y++) {
        for (int x = 1; x <= o_pin; x++) {
            tab[y+1][x*2 + o_plncount] = (y == o_xyz[x] ? -1.0 : 0.0);
            tab[y+1][x*2 + 1 + o_plncount] = (y == o_xyz[x] ? 1.0 : 0.0);
        }
    }

    int xx = 0;
    int face = 0;
    double facevalue = 0.0;

    // Call the linear program.
    if (simplx(tab, o_dim+1, o_plncount+2*o_pin, right, left) != 0) {
        o_stop = 2;
        out_screen_file("Hull simplex failed.\n");
        goto bailed;
    }

    radius[0] = -tab[1][1];
    for (int y = 1; y <= o_dim; y++) {
        for (int x = 1; x <= o_plncount+2*o_pin; x++) {
            if (right[x] == y+o_plncount+2*o_pin)
                o_centerpnt[y] = -tab[1][x+1];
        }
    }
    for (int y = 1; y <= o_dim+1; y++) {
        if (left[y] <= o_plncount) {
            if (xx+1 > o_dim+1-o_pin)
                out_screen_file("One tangent plane is being ignored\n");
            else
                tang[++xx] = left[y];
        }
    }

    int tnum;
    if ((tnum = xx) != o_dim+1-o_pin)
        out_screen_file("One or more tangent planes are missing.\n");

    // Find largest of the tangetial faces.
    for (int z = 1; z <= tnum; z++) {
        int plane = tang[z];
        if (!o_abpnts[plane][0]) {

            // Find the intersecting planes, which share N-1 points
            // (noshare is 1).
            int count = 0;
            for (int x = 1; x <= o_plncount; x++) {
                int noshare = 0;
                for (int y = 1; noshare < 2 && y <= o_dim; y++) {
                    int share = 0;
                    for (int yy = 1; !share && yy <= o_dim; yy++)
                        share = (o_abpnts[plane][yy] == o_abpnts[x][y]);
                    if (!share)
                        ++noshare;
                }
                if (noshare == 1) {
                    if (++count <= o_dim)
                        intsect[count] = x;
                }
            }
            if (count != o_dim) {
                o_abpnts[plane][0] = 1;
            }
            else {
                intsect[o_dim+1] = plane;

                // Find the angles for the plane.
                sine[o_dim+1] = 0.0;
                for (int y = 1; y <= o_dim; y++) {
                    double cos = 0.0;
                    for (int x = 1; x <= o_dim; x++)
                        cos = cos + o_ab[intsect[y]][x]*o_ab[plane][x];
                    sine[y] = sqrt(1.0 - cos*cos);
                }
                for (int y = 0; y <= o_dim; y++)
                    tab[y+1][1] = 0.0;
                tab[o_dim+2][1] = 1.0;
                for (int x = 1; x <= o_dim+1; x++)
                    tab[1][x+1] = -o_ab[intsect[x]][0];
                for (int x = 1; x <= o_dim+1; x++)
                    tab[o_dim+2][x+1] = -sine[x];
                for (int y = 1; y <= o_dim; y++) {
                    for (int x = 1; x <= o_dim+1; x++)
                        tab[y+1][x+1] = o_ab[intsect[x]][y];
                }
                for (int y = 0; y <= o_dim+1; y++)
                    tab[y+1][o_dim+3] = -tab[y+1][o_dim+2];
                tab[1][o_dim+2] -= INSCRIBE;
                tab[1][o_dim+3] -= INSCRIBE;

                // Call the linear program.
                if (simplx(tab, o_dim+1, o_dim+2, right, left) != 0) {
                    o_abpnts[plane][0] = 1;
                }
                else {
                    // Save face is the biggest so far.
                    if (-tab[1][1] > facevalue) {
                        facevalue = (-tab[1][1]);
                        face = plane;
                        for (int y = 1; y <= o_dim; y++) {
                            for (int x = 1; x <= o_dim+2; x++) {
                                if (right[x] == y+o_dim+2)
                                    centface[y] = -tab[1][x+1];
                            }
                        }
                    }
                }

            }
            if (o_abpnts[plane][0] == 1) {
                out_screen_file("          ");
                for (int x = 1; x <= o_dim; x++)
                    out_screen_file("%9.3f ", -o_ab[plane][x]);
                out_screen_file("\nCould not expand in above direction.\n");
            }

        }
    }
bailed:
    free_matrix(tab, 1, o_dim+3, 1, o_plncount+1+2*o_pin);
    free_ivector(left, 1, o_dim+1);
    free_ivector(right, 1, o_plncount+2*o_pin);
    free_vector(sine, 1, o_dim+1);
    free_ivector(tang, 1, o_dim+1-o_pin);
    free_ivector(intsect, 1, o_dim+1);
    return face;
}


void
OPTIMIZER::intpickpnts(int depth, int *pp)
{
    for (pp[depth] = depth*2-1; depth*2 >= pp[depth]; ++pp[depth]) {
        if (depth == o_dim)
            makeaplane(pp);
        else
            intpickpnts(depth+1, pp);
    }
}


void
OPTIMIZER::makeaplane(int *pp)
{
    // B
    for (int a = 1; a <= o_dim; a++) {
        for (int y = 1; y <= o_dim; y++)
            o_aNmatrix[a][y] = o_hullpnts[pp[a]][y];
    }
    o_temp[0] = det(o_aNmatrix, o_dim);

    // A
    for (int n = 1; n <= o_dim; n++) {
        for (int a = 1; a <= o_dim; a++) {
            for (int y = 1; y <= o_dim; y++)
                o_aNmatrix[a][y] = o_hullpnts[pp[a]][y];
            o_aNmatrix[a][n] = -1.0;
        }
        o_temp[n] = det(o_aNmatrix, o_dim);
    }

    // Check that plane is well defined.
    int def;
    for (int n = 1; !(def = (fabs(o_temp[n]) > DEFINED)) && n <= o_dim; n++);

    if (def) {
        // Check that all points (not in the plane) are on the same
        // side of the plane.
        int posd = 0, negd = 0;
        for (int y = 1; !(posd && negd) && y <= o_pntcount; y++) {
            int pntinpln;
            for (int a = 1; !(pntinpln = (y == pp[a])) && a <= o_dim; a++);
            if (!pntinpln) {
                double distance = o_temp[0];
                for (int n = 1; n <= o_dim; n++) 
                    distance += o_temp[n]*o_hullpnts[y][n];
                if (distance >= 0.0)
                    posd = 1;
                else
                    negd = 1;
            }
        }
        // Save the good planes.
        if (!(posd && negd)) {
            ++o_plncount;

            // Normalize.
            double sumsqr = 0.0;
            for (int n = 1; n <= o_dim; n++)
                sumsqr += o_temp[n]*o_temp[n];
            double norm = 1/(sqrt(sumsqr));
            for (int n = 0; n <= o_dim; n++)
                    o_temp[n] *= norm;
            double distance = o_temp[0];
            for (int n = 1; n <= o_dim; n++)
                distance += o_temp[n]*o_centerpnt[n];
            if (distance < 0.0) {
                for (int n = 0; n <= o_dim; n++)
                    o_temp[n] *= -1.0;
            }

            // Store plane in global array.
            for (int n = 0; n <= o_dim; n++)
                o_ab[o_plncount][n] = o_temp[n];
            o_abpnts[o_plncount][0] = 0;
            for (int n = 1; n <= o_dim; n++)
                o_abpnts[o_plncount][n] = pp[n];
        }
    }
}


void
OPTIMIZER::make_more_mem()
{
    int newmax;
    if (o_maxplane >= param.maxplane()) {
        o_stop = 3;
        newmax = o_maxplane+1000;
    }
    else {
        newmax = o_maxplane*2;
        if (newmax > param.maxplane())
            newmax = param.maxplane();
    }
    o_ab = reallocmatrix(o_ab, 1, o_maxplane, newmax, 0, o_dim);
    o_abpnts = ireallocmatrix(o_abpnts, 1, o_maxplane, newmax, 0, o_dim);
    o_maxplane = newmax;
}


#define TINY 1.0e-20

// Static function.
double
OPTIMIZER::det(double **aa, int n)
{
    double *vv = vector(1,n);
    double d = 1.0;
    for (int i = 1; i <= n; i++) {
        double big = 0.0;
        for (int j = 1; j <= n; j++) {
            double tem = fabs(aa[i][j]);
            if (tem > big)
                big = tem;
        }
        if (big == 0.0)
            nrerror ("Singular matrix in routine dim");
        vv[i] = 1.0/big;
    }
    for (int j = 1; j <= n; j++) {
        for (int i = 1; i < j; i++) {
            double sum = aa[i][j];
            for (int k = 1; k < i; k++)
                sum -= aa[i][k]*aa[k][j];
            aa[i][j] = sum;
        }
        int imax = -1;
        double big = 0.0;
        for (int i = j; i <= n; i++) {
            double sum = aa[i][j];
            for (int k = 1; k < j; k++)
                sum -= aa[i][k]*aa[k][j];
            aa[i][j] = sum;
            double dum = vv[i]*fabs(sum);
            if (dum >= big) {
                big = dum;
                imax = i;
            }
        }
        if (j != imax) {
            for (int k = 1; k <= n; k++) {
                double dum = aa[imax][k];
                aa[imax][k] = aa[j][k];
                aa[j][k] = dum;
            }
            d = -d;
            vv[imax] = vv[j];
        }
        if (aa[j][j] == 0.0)
            aa[j][j] = TINY;
        if (j != n) {
            double dum = 1.0/(aa[j][j]);
            for (int i = j+1; i <= n; i++)
                aa[i][j] *= dum;
        }
    }
    // tack on the determinant determination
    for (int j = 1; j <= n; j++)
        d *= aa[j][j];

    free_vector(vv, 1, n);
    return (d);
}


void
OPTIMIZER::out_screen_file(const char *fmt, ...)
{
    char outline[LONG_LINE_LENGTH];
    va_list args;
    va_start(args, fmt);
    vsnprintf(outline, LONG_LINE_LENGTH, fmt, args);
    va_end(args);

    printf("%s", outline);
    fprintf(o_fpout, "%s", outline);
}

