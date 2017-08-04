
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

#ifndef DRCIF_H
#define DRCIF_H

#include "tech_ret.h"
#include "tech_spacetab.h"

// This class satisfies all references to the DRC system in general
// application code.  It is subclassed by the DRC system, but can be
// instantiated stand-alone to satisfy the DRC references if the
// application does not provide DRC.

// Design rule keys.
// Order is important, rules on each layer are sorted in this order.
//
// Note: below rules are flagged in an unsigned int bit field, so be
// careful when adding rules
//
enum DRCtype
{
    drNoRule,
    drConnected,
    drNoHoles,
    drExist,
    drOverlap,
    drIfOverlap,
    drNoOverlap,
    drAnyOverlap,
    drPartOverlap,
    drAnyNoOverlap,
    drMinArea,
    drMaxArea,
    drMinEdgeLength,
    drMaxWidth,
    drMinWidth,
    drMinSpace,
    drMinSpaceTo,
    drMinSpaceFrom,
    drMinOverlap,
    drMinNoOverlap,
    drUserDefinedRule
};

struct DRCtestDesc;
struct op_change_t;
struct WindowDesc;
struct BBox;
struct CDl;
struct CDo;
struct sLspec;
struct Point;
struct Zlist;
struct sTspaceTable;


inline class cDrcIf *DrcIf();

class cDrcIf
{
    static cDrcIf *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cDrcIf *DrcIf() { return (cDrcIf::ptr()); }

    cDrcIf();
    virtual ~cDrcIf() { }

    // capability flag
    virtual bool hasDRC() { return (false); }

    // drc.cc
    virtual bool actionHandler(WindowDesc*, int) = 0;
    virtual int minDimension(CDl*) = 0;
    virtual void layerChangeCallback() = 0;

    // drc_error.cc
    virtual void showCurError(WindowDesc*, bool) = 0;
    virtual void eraseListError(const op_change_t*, bool) = 0;
    virtual void clearCurError() = 0;
    virtual void fileMessage(const char*) = 0;

    // drc_eval.cc
    virtual bool intrListTest(const op_change_t*, BBox*) = 0;

    // drc_poly.cc
    virtual bool diskEval(int, int, const CDl*) = 0;
    virtual bool donutEval(int, int, int, int, const CDl*) = 0;
    virtual bool arcEval(int, int, int, int, double, double, const CDl*) = 0;

    // drc_results.cc
    virtual void cancelNext() = 0;
    virtual void windowDestroyed(int) = 0;

    // drc_rule.cc
    virtual void initRules() = 0;
    virtual void deleteRules(DRCtestDesc**) = 0;

    // drc_tech.cc
    virtual bool addMaxWidth(CDl*, double, const char*) = 0;
    virtual bool addMinWidth(CDl*, double, const char*) = 0;
    virtual bool addMinDiagonalWidth(CDl*, double, const char*) = 0;
    virtual bool addMinSpacing(CDl*, CDl*, double, const char*) = 0;
    virtual bool addMinSameNetSpacing(CDl*, CDl*, double, const char*) = 0;
    virtual bool addMinDiagonalSpacing(CDl*, CDl*, double, const char*) = 0;
    virtual bool addMinSpaceTable(CDl*, CDl*, sTspaceTable*) = 0;
    virtual bool addMinArea(CDl*, double, const char*) = 0;
    virtual bool addMinHoleArea(CDl*, double, const char*) = 0;
    virtual bool addMinHoleWidth(CDl*, double, const char*) = 0;
    virtual bool addMinEnclosure(CDl*, CDl*, double, const char*) = 0;
    virtual bool addMinExtension(CDl*, CDl*, double, const char*) = 0;
    virtual bool addMinOppExtension(CDl*, CDl*, double, double,
        const char*) = 0;
    virtual char *spacings(const char*) = 0;
    virtual char *orderedSpacings(const char*) = 0;
    virtual char *minSpaceTables(const char*) = 0;

    // drc.h
    virtual void setInteractive(bool) = 0;
    virtual bool isInteractive() const = 0;
    virtual void setIntrNoErrMsg(bool) = 0;
    virtual bool isIntrNoErrMsg() const = 0;

private:
    static cDrcIf *instancePtr;
};

class cDrcIfStubs : public cDrcIf
{
    bool actionHandler(WindowDesc*, int) { return (false); }
    int minDimension(CDl*) { return (0); }
    void layerChangeCallback() { }

    void showCurError(WindowDesc*, bool) { }
    void eraseListError(const op_change_t*, bool) { }
    void clearCurError() { }
    void fileMessage(const char*) { }

    bool intrListTest(const op_change_t*, BBox*) { return (false); }

    bool diskEval(int, int, const CDl*) { return (true); }
    bool donutEval(int, int, int, int, const CDl*) { return (true); }
    bool arcEval(int, int, int, int, double, double, const CDl*)
        { return (true); }

    void cancelNext() { }
    void windowDestroyed(int) { }

    void initRules() { }
    void deleteRules(DRCtestDesc**) { }

    bool addMaxWidth(CDl*, double, const char*) { return (true); }
    bool addMinWidth(CDl*, double, const char*) { return (true); }
    bool addMinDiagonalWidth(CDl*, double, const char*) { return (true); }
    bool addMinSpacing(CDl*, CDl*, double, const char*) { return (true); }
    bool addMinSameNetSpacing(CDl*, CDl*, double, const char*)
        { return (true); }
    bool addMinDiagonalSpacing(CDl*, CDl*, double, const char*)
        { return (true); }
    bool addMinSpaceTable(CDl*, CDl*, sTspaceTable *t)
        { delete [] t; return (true); }
    bool addMinArea(CDl*, double, const char*) { return (true); }
    bool addMinHoleArea(CDl*, double, const char*) { return (true); }
    bool addMinHoleWidth(CDl*, double, const char*) { return (true); }
    bool addMinEnclosure(CDl*, CDl*, double, const char*) { return (true); }
    bool addMinExtension(CDl*, CDl*, double, const char*) { return (true); }
    bool addMinOppExtension(CDl*, CDl*, double, double, const char*)
        { return (true); }
    char *spacings(const char*) { return (0); }
    char *orderedSpacings(const char*) { return (0); }
    char *minSpaceTables(const char*) { return (0); }

    void setInteractive(bool) { }
    bool isInteractive() const { return (false); }
    void setIntrNoErrMsg(bool) { }
    bool isIntrNoErrMsg() const { return (false); }
};

#endif

