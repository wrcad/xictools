
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

#ifndef SI_SPT_H
#define SI_SPT_H

 // Database of position-dependent user-specified factors.
 //
struct spt_t
{
    spt_t(char *n, int x, unsigned int dx, unsigned int nx,
        int y, unsigned int dy, unsigned int ny)
    {
        matrix = 0;
        data_name = n;
        org_x = x;
        del_x = dx;
        num_x = nx;
        org_y = y;
        del_y = dy;
        num_y = ny;
        if (nx && ny) {
            unsigned long sz = nx*ny;
            matrix = new float[sz];
            for (unsigned long i = 0; i < sz; i++)
                matrix[i] = 0.0;
        }
    }

    ~spt_t() { delete [] matrix; delete [] data_name; }

    bool save_item(int x, int y, float item)
    {
        if (x < org_x || x >= (int)(org_x + del_x*num_x))
            return (false);
        if (y < org_y || y >= (int)(org_y + del_y*num_y))
            return (false);
        unsigned long ix = ((y - org_y)/del_y)*num_x + (x - org_x)/del_x;
        matrix[ix] = item;
        return (true);
    }

    float retrieve_item(int x, int y)
    {
        if (x < org_x || x >= (int)(org_x + del_x*num_x))
            return (0.0);
        if (y < org_y || y >= (int)(org_y + del_y*num_y))
            return (0.0);
        unsigned long ix = ((y - org_y)/del_y)*num_x + (x - org_x)/del_x;
        return (matrix[ix]);
    }

    const char *keyword() { return (data_name); }

    void params(int *ox, unsigned int *dx, unsigned int *nx,
            int *oy, unsigned int *dy, unsigned int *ny)
        {
            if (ox)
                *ox = org_x;
            if (dx)
                *dx = del_x;
            if (nx)
                *nx = num_x;
            if (oy)
                *oy = org_y;
            if (dy)
                *dy = del_y;
            if (ny)
                *ny = num_y;
        }

    // spt.cc
    static SymTab *swapSPtableCx(SymTab*);
    static bool readSpatialParameterTable(const char*);
    static bool newSpatialParameterTable(const char*, double, double, int,
        double, double, int);
    static bool writeSpatialParameterTable(const char*, const char*);
    static int clearSpatialParameterTable(const char*);
    static spt_t *findSpatialParameterTable(const char*);

private:
    float *matrix;
    char *data_name;
    int org_x;
    unsigned int del_x;
    unsigned int num_x;
    int org_y;
    unsigned int del_y;
    unsigned int num_y;
};


// An interface to a table of string tables, useful for keeping track
// of SPT names.
//
namespace nametab {
    SymTab *swapNametabCx(SymTab*);
    SymTab *findNametab(const char*, bool);
    bool removeNametab(const char*);
    stringlist *listNametabs();
    void clearNametabs();
}

#endif

