
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

#ifndef SCEDIF_H
#define SCEDIF_H

// This class satisfies all references to the schematic editing (SCED)
// system in general application code.  It is subclassed by the SCED
// system, but can be instantiated stand-alone to satisfy the SCED
// references if the application does not provide SCED.

class cNodeMap;
struct CDcbin;
struct CDo;
struct hyList;
struct MenuEnt;

// Used in main_txtcmds.cc, part of the Spice Interface group.
#define VA_SpiceHostDisplay   "SpiceHostDisplay"

// Electrical connections display.
enum DotsType { DotsNone, DotsSome, DotsAll };

// Arg for PopUpSim()
enum SpType { SpNil, SpBusy, SpPause, SpDone, SpError };


inline class cScedIf *ScedIf();

class cScedIf
{
    static cScedIf *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cScedIf *ScedIf() { return (cScedIf::ptr()); }

    cScedIf();
    virtual ~cScedIf() { }

    // capability flag
    virtual bool hasSced() { return (false); }

    // scedif.cc
    int whichPrpty(const char*, bool*);

    // sced.cc
    virtual void modelLibraryOpen(const char*) = 0;
    virtual void modelLibraryClose() = 0;
    virtual void closeSpice() = 0;
    virtual bool simulationActive() = 0;
    virtual bool logConnect() = 0;
    virtual void setLogConnect(bool) = 0;

    // sced_check.cc
    virtual void checkElectrical(CDcbin*) = 0;

    // sced_connect.cc
    virtual bool connectAll(bool, CDs* = 0) = 0;
    virtual bool connect(CDs*) = 0;
    virtual void updateNames(CDs*) = 0;

    // sced_dots.cc
    virtual void recomputeDots() = 0;
    virtual void updateDots(CDs*, CDo*) = 0;
    virtual void dotsSetDirty(const CDs*) = 0;
    virtual void dotsUpdateDirty() = 0;
    virtual void clearDots() = 0;
    virtual void updateDotsCellName(CDcellName, CDcellName) = 0;

    // sced_fixup.cc
    virtual void addParentConnection(CDs*, int, int) = 0;
    virtual void install(CDo*, CDs*, bool) = 0;
    virtual void uninstall(CDo*, CDs*) = 0;

    // secd_menu.cc
    virtual bool setupCommand(MenuEnt*, bool*, bool*) = 0;

    // sced_mutual.cc
    virtual bool setMutLabel(CDs*, CDp_nmut*, CDp_nmut*) = 0;
    virtual void mutToNewMut(CDs*) = 0;
    virtual void mutParseName(const char*, char*, char*) = 0;

    // sced_nodemap.cc
    virtual void setModified(cNodeMap*) = 0;
    virtual void destroyNodes(CDs*) = 0;
    virtual void updateNodes(const CDs*) = 0;
    virtual void registerGlobalNetName(const char*) = 0;

    // sced_plot.cc
    virtual void setPlotMarkColors() = 0;
    virtual void clearPlots() = 0;
    virtual char *getPlotCmd(bool) = 0;
    virtual void setPlotCmd(const char*) = 0;
    virtual char *getIplotCmd(bool) = 0;
    virtual void setIplotCmd(const char*) = 0;
    virtual hyList *getPlotList() = 0;
    virtual hyList *getIplotList() = 0;

    // sced_prplabel.cc
    virtual void genDeviceLabels(CDc*, CDc*, bool) = 0;
    virtual CDla *changeLabel(CDla*, CDs*, hyList*) = 0;
    virtual int checkRepositionLabels(const CDc*) = 0;

    // sced_prpty.cc
    virtual bool setDevicePrpty(const char*, const char*) = 0;
    virtual char *getDevicePrpty(const char*) = 0;
    virtual CDp *prptyModify(CDc*, CDp*, int, const char*, hyList*) = 0;

    // sced_shape.cc
    virtual const char *const *shapesList() = 0;
    virtual void addShape(int) = 0;

    // sced_spiceout.cc
    virtual char *getAnalysis(bool) = 0;
    virtual void setAnalysis(const char*) = 0;
    virtual hyList *getAnalysisList() = 0;
    virtual void dumpSpiceDeck(FILE*) = 0;

    // sced_subckt.cc
    virtual void assertSymbolic(bool) = 0;

    // graphics
    virtual void PopUpSpiceIf(GRobject, ShowMode) = 0;
    virtual void PopUpDevs(GRobject, ShowMode) = 0;
    virtual void PopUpDots(GRobject, ShowMode) = 0;
    virtual void DevsEscCallback() = 0;
    virtual void PopUpDevEdit(GRobject, ShowMode) = 0;
    virtual bool PopUpNodeMap(GRobject, ShowMode, int = -1) = 0;
    virtual void PopUpSim(SpType) = 0;

    // sced.h
    virtual DotsType showingDots() = 0;

    virtual void setDoingPlot(bool) = 0;
    virtual bool doingPlot() = 0;
    virtual void setDoingIplot(bool) = 0;
    virtual bool doingIplot() = 0;

private:
    static cScedIf *instancePtr;
};

class cScedIfStubs : public cScedIf
{
    void modelLibraryOpen(const char*) { }
    void modelLibraryClose() { }
    void closeSpice() { }
    bool simulationActive() { return (false); }
    bool logConnect() { return (false); }
    void setLogConnect(bool) { }

    void checkElectrical(CDcbin*) { }

    bool connectAll(bool, CDs* = 0) { return (true); }
    bool connect(CDs*) { return (true); }
    void updateNames(CDs*) { }

    void recomputeDots() { }
    void updateDots(CDs*, CDo*) { }
    void dotsSetDirty(const CDs*) { }
    void dotsUpdateDirty() { }
    void clearDots() { }
    void updateDotsCellName(CDcellName, CDcellName) { }

    void addParentConnection(CDs*, int, int) { }
    void install(CDo*, CDs*, bool) { }
    void uninstall(CDo*, CDs*) { }

    bool setupCommand(MenuEnt*, bool*, bool*) { return (false); }

    bool setMutLabel(CDs*, CDp_nmut*, CDp_nmut*) { return (false); }
    void mutToNewMut(CDs*) { }
    void mutParseName(const char*, char*, char*) { }

    void setModified(cNodeMap*) { }
    void destroyNodes(CDs*) { }
    void updateNodes(const CDs*) { }
    void registerGlobalNetName(const char*) { }

    void setPlotMarkColors() { }
    void clearPlots() { }
    char *getPlotCmd(bool) { return (0); }
    void setPlotCmd(const char*) { }
    char *getIplotCmd(bool) { return (0); }
    void setIplotCmd(const char*) { }
    hyList *getPlotList() { return (0); }
    hyList *getIplotList() { return (0); }

    void genDeviceLabels(CDc*, CDc*, bool) { }
    CDla *changeLabel(CDla*, CDs*, hyList*) { return (0); } 
    int checkRepositionLabels(const CDc*) { return (0); }

    bool setDevicePrpty(const char*, const char*) { return (false); }
    char *getDevicePrpty(const char*) { return (0); }
    CDp *prptyModify(CDc*, CDp*, int, const char*, hyList*) { return (0); }

    const char *const *shapesList() { return (0); }
    void addShape(int) { }

    char *getAnalysis(bool) { return (0); }
    void setAnalysis(const char*) { }
    hyList *getAnalysisList() { return (0); }
    void dumpSpiceDeck(FILE*) { }

    void assertSymbolic(bool) { }

    void PopUpSpiceIf(GRobject, ShowMode) { }
    void PopUpDevs(GRobject, ShowMode) { }
    void PopUpDots(GRobject, ShowMode) { }
    void DevsEscCallback() { }
    void PopUpDevEdit(GRobject, ShowMode) { }
    bool PopUpNodeMap(GRobject, ShowMode, int = -1) { return (false); }
    void PopUpSim(SpType) { }

    DotsType showingDots() { return (DotsNone); }

    void setDoingPlot(bool) { }
    bool doingPlot() { return (false); }
    void setDoingIplot(bool) { }
    bool doingIplot() { return (false); }
};

#endif

