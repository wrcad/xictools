
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

#ifndef EXTIF_H
#define EXTIF_H

#include "si_parsenode.h"
#include "si_lspec.h"
#include "tech_ret.h"

// This class satisfies all references to the extraction system in
// general application code.  It is subclassed by the extraction
// system, but can be instantiated stand-alone to satisfy the
// extraction references if the application does not provide
// extraction.


struct ParseNode;
struct sVia;
struct MenuEnt;

// Name of device templates file read on program startup.
#define DEVICE_TEMPLATES "device_templates"

// Used to pass values to addMOS, matches arguments to Virtuoso
// extractMOS techfile node.
//
struct sMOSdev
{
    sMOSdev()
        {
            name = 0;
            base = 0;
            poly = 0;
            actv = 0;
            well = 0;
        }

    ~sMOSdev()
        {
            delete [] name;
            delete [] base;
            delete [] poly;
            delete [] actv;
            delete [] well;
        }

    char *name;
    char *base;
    char *poly;
    char *actv;
    char *well;
};

// Used to pass values to addRES, matches arguments to Virtuoso
// extractRES techfile node.
//
struct sRESdev
{
    sRESdev()
        {
            name = 0;
            base = 0;
            matl = 0;
        }

    ~sRESdev()
        {
            delete [] name;
            delete [] base;
            delete [] matl;
        }

    char *name;
    char *base;
    char *matl;
};

inline class cExtIf *ExtIf();

class cExtIf
{
    static cExtIf *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cExtIf *ExtIf() { return (cExtIf::ptr()); }

    cExtIf();
    virtual ~cExtIf() { }

    // capability flag
    virtual bool hasExtract() { return (false); }

    // ext.cc
    virtual CDs *cellDesc(cGroupDesc*) = 0;
    virtual void windowShowHighlighting(WindowDesc*) = 0;
    virtual void windowShowBlinking(WindowDesc*) = 0;
    virtual void layerChangeCallback() = 0;
    virtual void preCurCellChangeCallback() = 0;
    virtual void postCurCellChangeCallback() = 0;
    virtual void deselect() = 0;
    virtual void postModeSwitchCallback() = 0;
    virtual siVariable *createFormatVars() = 0;
    virtual bool rlsolverMsgs() = 0;
    virtual void setRLsolverMsgs(bool) = 0;
    virtual bool logRLsolver() = 0;
    virtual void setLogRLsolver(bool) = 0;
    virtual bool logGrouping() = 0;
    virtual void setLogGrouping(bool) = 0;
    virtual bool logExtracting() = 0;
    virtual void setLogExtracting(bool) = 0;
    virtual bool logAssociating() = 0;
    virtual void setLogAssociating(bool) = 0;
    virtual bool logVerbose() = 0;
    virtual void setLogVerbose(bool) = 0;

    // ext_device.cc
    virtual void initDevs() = 0;

    // ext_duality.cc
    virtual bool associate(CDs*) = 0;
    virtual int groupOfNode(CDs*, int) = 0;
    virtual int nodeOfGroup(CDs*, int) = 0;
    virtual void clearFormalTerms(CDs*) = 0;

    // ext_extract.cc
    virtual bool skipExtract(CDs*) = 0;

    // ext_gnsel.cc
    virtual bool selectShowNode(int) = 0;
    virtual int netSelB1Up() = 0;
    virtual int netSelB1Up_altw() = 0;

    // ext_gplane.cc
    virtual bool setInvGroundPlaneImmutable(bool) = 0;

    // ext_group.cc
    virtual void invalidateGroups(bool = false) = 0;
    virtual void destroyGroups(CDs*) = 0;
    virtual void clearGroups(CDs*) = 0;

    // ext_menu.cc
    virtual bool setupCommand(MenuEnt*, bool*, bool*) = 0;

    // ext_nets.cc
    virtual void arrangeTerms(CDcbin*, bool) = 0;
    virtual void placePhysSubcTerminals(const CDc*, int, const CDc*,
        unsigned int, unsigned int) = 0;

    // ext_tech.cc
    virtual void parseDeviceTemplates(FILE*) = 0;
    virtual void addMOS(const sMOSdev*) = 0;
    virtual void addRES(const sRESdev*) = 0;

    // ext_view.cc
    virtual CDol *selectItems(const char*, const BBox*, int, int, int) = 0;

    // graphical
    virtual void PopUpExtSetup(GRobject, ShowMode) = 0;

    // ext.h
    virtual bool isExtractionView() = 0;
    virtual bool isExtractionSelect() = 0;

private:
    static cExtIf *instancePtr;
};

class cExtIfStubs : public cExtIf
{
    CDs *cellDesc(cGroupDesc*) { return (0); }
    void windowShowHighlighting(WindowDesc*) { }
    void windowShowBlinking(WindowDesc*) { }
    void layerChangeCallback() { }
    void preCurCellChangeCallback() { }
    void postCurCellChangeCallback() { }
    void deselect() { }
    void postModeSwitchCallback() { }
    siVariable *createFormatVars() { return (0); }
    bool rlsolverMsgs() { return (false); }
    void setRLsolverMsgs(bool) { }
    bool logRLsolver() { return (false); }
    void setLogRLsolver(bool) { }
    bool logGrouping() { return (false); }
    void setLogGrouping(bool) { }
    bool logExtracting() { return (false); }
    void setLogExtracting(bool) { }
    bool logAssociating() { return (false); }
    void setLogAssociating(bool) { }
    bool logVerbose() { return (false); }
    void setLogVerbose(bool) { }

    void initDevs() { }

    bool associate(CDs*) { return (false); }
    int groupOfNode(CDs*, int) { return (-1); }
    int nodeOfGroup(CDs*, int) { return (-1); }
    void clearFormalTerms(CDs*) { }

    bool skipExtract(CDs*) { return (false); }

    bool selectShowNode(int) { return (false); }
    int netSelB1Up() { return (-1); }
    int netSelB1Up_altw() { return (-1); }

    bool setInvGroundPlaneImmutable(bool) { return (false); }

    void invalidateGroups(bool) { }
    void destroyGroups(CDs*) { }
    void clearGroups(CDs*) { }

    bool setupCommand(MenuEnt*, bool*, bool*) { return (false); }

    void arrangeTerms(CDcbin*, bool) { }
    void placePhysSubcTerminals(const CDc*, int, const CDc*,
        unsigned int, unsigned int) { }

    void parseDeviceTemplates(FILE*) { };
    void addMOS(const sMOSdev*) { }
    void addRES(const sRESdev*) { }

    CDol *selectItems(const char*, const BBox*, int, int, int) { return (0); }

    void PopUpExtSetup(GRobject, ShowMode) { }

    bool isExtractionView() { return (false); }
    bool isExtractionSelect() { return (false); }
};

#endif

