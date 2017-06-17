
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
 $Id: si_spt.h,v 5.6 2009/02/01 09:27:42 stevew Exp $
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

