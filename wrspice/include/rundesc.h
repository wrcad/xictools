
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1988 Wayne A. Christopher
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef RUNDESC_H
#define RUNDESC_H

//
// sRunDesc functions, low-level output control.
//


// Struct used to store information on a data object.
//
struct dataDesc
{
    // No constructor, bulk allocated.

    ~dataDesc()
        {
//XXX
if (dd_name)
fprintf(stderr, "%s\n", dd_name);
            delete [] dd_name;
        }

    void setup(const char *nm, int ind, int typ, bool rok)
        {
            dd_name = lstring::copy(nm);
            dd_type = typ & IF_VARTYPES;
            dd_gtype = GRID_LIN;
            dd_regular = true;
            dd_outIndex = ind;
            dd_rollover_ok = rok;
        }

    void setup_special(const char *nm, int ind, bool usevec, bool rok)
        {
            dd_name = lstring::copy(nm);
            dd_useVecIndx = usevec;
            dd_outIndex = ind;
            dd_regular = false;
            dd_rollover_ok = rok;
        }

    const char *dname()     const { return (dd_name); }
    sDataVec *vec()         const { return (dd_vec); }
    int type()              const { return (dd_type); }
    int outIndex()          const { return (dd_outIndex); }
    int numbasedims()       const { return (dd_numbasedims); }
    int basedims(int i)     const { return (dd_basedims[i]); }
    IFspecial &sp()         { return (dd_sp); }
    bool useVecIndx()       const { return (dd_useVecIndx); }
    bool regular()          const { return (dd_regular); }

    void set_vec(sDataVec *v)       { dd_vec = v; }
    void set_type(int t)            { dd_type = t; }
    void set_gtype(int t)           { dd_gtype = t; }
    void set_numbasedims(int n)     { dd_numbasedims = n; }
    void set_basedims(int i, int n) { dd_basedims[i] = n; }

    // rundesc.cc
    void addRealValue(double, bool);
    void pushRealValue(double, unsigned int);
    void addComplexValue(IFcomplex, bool);
    void pushComplexValue(IFcomplex, unsigned int);

    static dataDesc *resize(int newsize, int oldsize, dataDesc *oldptr)
    {
        dataDesc *d = new dataDesc[newsize];
        if (oldptr) {
            for (int i = 0; i < oldsize; i++) {
                *(d+i) = *(oldptr+i);
                oldptr[i].dd_name = 0;
            }
            delete [] oldptr;
        }
        for (int i = oldsize; i < newsize; i++)
            *(d+i) = dataDesc();
        return (d);
    }

private:
    const char *dd_name;    // The name of the vector
    sDataVec *dd_vec;       // The data
    int dd_type;            // The type
    int dd_gtype;           // Default plot grid type
    int dd_outIndex;        // Index of regular vector or dependent
    int dd_numbasedims;     // Vector numdims for looping
    int dd_basedims[MAXDIMS]; // Vector dimensions for looping
    IFspecial dd_sp;        // Descriptor for special parameter
    bool dd_useVecIndx;     // Use vector for special array index
    bool dd_regular;        // True if regular vector, false if special
    bool dd_rollover_ok;    // If true, roll over
};

// Struct used to store run status and context.  This is returned from
// beginPlot as an opaque data type to be passed to the other methods.
//
struct sRunDesc
{
    sRunDesc()
        {
            rd_name         = 0;
            rd_type         = 0;
            rd_title        = 0;

            rd_job          = 0;
            rd_ckt          = 0;
            rd_check        = 0;
            rd_sweep        = 0;
            rd_circ         = 0;
            rd_runPlot      = 0;
            rd_rd           = 0;
            rd_data         = 0;

            rd_numData      = 0;
            rd_dataSize     = 0;
            rd_refIndex     = 0;
            rd_pointsSeen   = 0;
            rd_pointCount   = 0;
            rd_numPoints    = 0;
            rd_isComplex    = 0;
            rd_maxPts       = 0;
            rd_cycles       = 0;
            rd_anType       = 0;

            rd_segfilebase  = 0;
            rd_segdelta     = 0.0;
            rd_seglimit     = 0.0;
            rd_segindex     = 0;
            rd_scrolling    = false;
        }

    ~sRunDesc()
        {
//XXX
printf("here %lx\n", this);
            delete [] rd_name;
            delete [] rd_type;
            delete [] rd_title;
            delete [] rd_data;
            delete rd_rd;
            delete rd_segfilebase;
        }

    void plotInit(double, double, double, sPlot*);
    void addPointToPlot(IFvalue*, IFvalue*, bool);
    void pushPointToPlot(sCKT*, IFvalue*, IFvalue*, unsigned int);
    void setupSegments(const char*, double, sOUTdata*);
    void dumpSegment();
    void resetVecs();
    void plotEnd();

    bool datasize();
    void unrollVecs();
    void scalarizeVecs();
    void unscalarizeVecs();
    void segmentizeVecs();
    void unsegmentizeVecs();
    int addDataDesc(const char*, int, int);
    int addSpecialDesc(const char*, int, bool);
    bool getSpecial(sCKT*, int, IFvalue*);
    void setCkt(sCKT*);

    const char *name()              { return (rd_name); }
    void set_name(const char *n)
        {
            char *s = lstring::copy(n);
            delete [] rd_name;
            rd_name = s;
        }

    const char *type()              { return (rd_type); }
    void set_type(const char *t)
        {
            char *s = lstring::copy(t);
            delete [] rd_type;
            rd_type = s;
        }

    const char *title()             { return (rd_title); }
    void set_title(const char *t)
        {
            char *s = lstring::copy(t);
            delete [] rd_title;
            rd_title = s;
        }

    sJOB *job()                     { return (rd_job); }
    void set_job(sJOB *a)           { rd_job = a; }

    sCKT *get_ckt()                 { return (rd_ckt); }
    sFtCirc *circuit()              { return (rd_circ); }

    sPlot *runPlot()                { return (rd_runPlot); }
    void set_runPlot(sPlot *p)      { rd_runPlot = p; }

    sCHECKprms *check()             { return (rd_check); }
    void set_check(sCHECKprms *c)   { rd_check = c; }

    sSWEEPprms *sweep()             { return (rd_sweep); }
    void set_sweep(sSWEEPprms *c)   { rd_sweep = c; }

    cFileOut *rd()                  { return (rd_rd); }
    void set_rd(cFileOut *f)        { rd_rd = f; }

    dataDesc *data(int i)           { return (&rd_data[i]); }

    int numData()                   { return (rd_numData); }

    int refIndex()                  { return (rd_refIndex); }
    void set_refIndex(int i)        { rd_refIndex = i; }

    int pointsSeen()                { return (rd_pointsSeen); }
    void inc_pointsSeen()           { rd_pointsSeen++; }

    int pointCount()                { return (rd_pointCount); }
    void inc_pointCount()           { rd_pointCount++; }

    int numPoints()                 { return (rd_numPoints); }
    void set_numPoints(int n)       { rd_numPoints = n; }

    int maxPts()                    { return (rd_maxPts); }

    int cycles()                    { return (rd_cycles); }
    void set_cycles(int c)          { rd_cycles = c; }

    int anType()                    { return (rd_anType); }
    void set_anType(int t)          { rd_anType = t; }

    void set_dims(int *d)
        {
            d[1] = rd_numPoints;
            d[0] = rd_segindex + 2;
            rd_segindex++;
        }
    const char *segfilebase()       { return (rd_segfilebase); }

    void set_scrolling(bool b)      { rd_scrolling = b; }

    double ref_value()
        {
            if (rd_refIndex >= 0 && rd_data)
                return (rd_data[rd_refIndex].vec()->realval(rd_pointCount-1));
            return (0.0);
        }

private:
    char *rd_name;          // circuit name: e.g., CKT1
    char *rd_type;          // analysis key: e.g., TRAN
    char *rd_title;         // analysis descr: e.g., Transient Analysis

    sJOB *rd_job;           // pointer to analysis job struct
    sCKT *rd_ckt;           // pointer to running circuit
    sCHECKprms *rd_check;   // pointer to range analysis control struct
    sSWEEPprms *rd_sweep;   // pointer to sweep analysis control struct
    sFtCirc *rd_circ;       // pointer to circuit
    sPlot *rd_runPlot;      // associated plot
    cFileOut *rd_rd;        // associated plot file interface
    dataDesc *rd_data;      // the data descriptors

    int rd_numData;         // size of data array
    int rd_dataSize;        // allocated size of data array
    int rd_refIndex;        // index of reference datum in data array
    int rd_pointsSeen;      // running count of output points seen
    int rd_pointCount;      // running count of output points saved
    int rd_numPoints;       // num points expected, -1 if not known
    int rd_isComplex;       // set if any vector is complex
    int rd_maxPts;          // point limit
    int rd_cycles;          // number of data cycles in multi-dim analysis
    int rd_anType;          // analysis type

    // These parameters are for multi-file (segmented) output.
    const char *rd_segfilebase; // base file name, files are "basename.sNN"
    double rd_segdelta;     // range if refValue for segment
    double rd_seglimit;     // end of segment
    int rd_segindex;        // count of segments outputs
    bool rd_scrolling;      // true when scrolling
};

#endif

