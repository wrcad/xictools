
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

#ifndef TECH_KWORDS_H
#define TECH_KWORDS_H


// A repository for the core tech file keywords.
//
struct sTKW
{
    // Global keywords.
    const char *Set()               const { return ("Set"); }
    const char *If()                const { return ("If"); }
    const char *IfDef()             const { return ("IfDef"); }
    const char *IfNdef()            const { return ("IfNdef"); }
    const char *Else()              const { return ("Else"); }
    const char *EndIf()             const { return ("EndIf"); }
    const char *Define()            const { return ("Define"); }
    const char *Comment()           const { return ("Comment"); }

    // Paths.
    const char *Path()              const { return ("Path"); }
    const char *LibPath()           const { return ("LibPath"); }
    const char *HelpPath()          const { return ("HelpPath"); }
    const char *ScriptPath()        const { return ("ScriptPath"); }

    // Global Parameters.
    const char *Technology()        const { return ("Technology"); }
    const char *Vendor()            const { return ("Vendor"); }
    const char *Process()           const { return ("Process"); }
    const char *MapLayer()          const { return ("MapLayer"); }
    const char *DefineLayer()       const { return ("DefineLayer"); }
    const char *DefinePurpose()     const { return ("DefinePurpose"); }
    const char *StandardVia()       const { return ("StandardVia"); }

    // Layer keywords.
    const char *Derived()           const { return ("Derived"); }
    const char *DerivedLayer()      const { return ("DerivedLayer"); }
    const char *Layer()             const { return ("Layer"); }
    const char *LayerName()         const { return ("LayerName"); }
    const char *PhysLayer()         const { return ("PhysLayer"); }
    const char *PhysLayerName()     const { return ("PhysLayerName"); }
    const char *ElecLayer()         const { return ("ElecLayer"); }
    const char *ElecLayerName()     const { return ("ElecLayerName"); }
    const char *LppName()           const { return ("LppName"); }
    const char *Description()       const { return ("Description"); }
    const char *kwRGB()             const { return ("RGB"); }
    const char *AltRGB()            const { return ("AltRGB"); }
    const char *Filled()            const { return ("Filled"); }
    const char *AltFilled()         const { return ("AltFilled"); }
    const char *HpglFilled()        const { return ("HpglFilled"); }
    const char *XfigFilled()        const { return ("XfigFilled"); }
    const char *Invisible()         const { return ("Invisible"); }
    const char *AltInvisible()      const { return ("AltInvisible"); }
    const char *Blink()             const { return ("Blink"); }
    const char *NoSelect()          const { return ("NoSelect"); }
    const char *WireActive()        const { return ("WireActive"); }
    const char *NoInstView()        const { return ("NoInstView"); }
    const char *Invalid()           const { return ("Invalid"); }
    const char *WireWidth()         const { return ("WireWidth"); }
    const char *CrossThick()        const { return ("CrossThick"); }
    const char *Symbolic()          const { return ("Symbolic"); }
    const char *NoMerge()           const { return ("NoMerge"); }
    const char *SpacingTable()      const { return ("SpacingTable"); }

    const char *Background()        const { return ("Background"); }
    const char *ElecBackground()    const { return ("ElecBackground"); }
    const char *PhysBackground()    const { return ("PhysBackground"); }

    // Attributes, can be set in layer blocks, too.
    const char *Axes()              const { return ("Axes"); }
    const char *ShowGrid()          const { return ("ShowGrid"); }
    const char *PhysShowGrid()      const { return ("PhysShowGrid"); }
    const char *ElecShowGrid()      const { return ("ElecShowGrid"); }
    const char *GridOnBottom()      const { return ("GridOnBottom"); }
    const char *PhysGridOnBottom()  const { return ("PhysGridOnBottom"); }
    const char *ElecGridOnBottom()  const { return ("ElecGridOnBottom"); }
    const char *GridStyle()         const { return ("GridStyle"); }
    const char *PhysGridStyle()     const { return ("PhysGridStyle"); }
    const char *ElecGridStyle()     const { return ("ElecGridStyle"); }
    const char *GridCoarseMult()    const { return ("GridCoarseMult"); }
    const char *PhysGridCoarseMult() const { return ("PhysGridCoarseMult"); }
    const char *ElecGridCoarseMult() const { return ("ElecGridCoarseMult"); }
    const char *Expand()            const { return ("Expand"); }
    const char *PhysExpand()        const { return ("PhysExpand"); }
    const char *ElecExpand()        const { return ("ElecExpand"); }
    const char *DisplayAllText()    const { return ("DisplayAllText"); }
    const char *PhysDisplayAllText() const { return ("PhysDisplayAllText"); }
    const char *ElecDisplayAllText() const { return ("ElecDisplayAllText"); }
    const char *ShowPhysProps()     const { return ("ShowPhysProps"); }
    const char *LabelAllInstances() const { return ("LabelAllInstances"); }
    const char *PhysLabelAllInstances() const { return ("PhysLabelAllInstances"); }
    const char *ElecLabelAllInstances() const { return ("ElecLabelAllInstances"); }
    const char *ShowContext()       const { return ("ShowContext"); }
    const char *PhysShowContext()   const { return ("PhysShowContext"); }
    const char *ElecShowContext()   const { return ("ElecShowContext"); }
    const char *ShowTinyBB()        const { return ("ShowTinyBB"); }
    const char *PhysShowTinyBB()    const { return ("PhysShowTinyBB"); }
    const char *ElecShowTinyBB()    const { return ("ElecShowTinyBB"); }

    // Attributes, not allowed in print blocks.
    const char *MfgGrid()           const { return ("MfgGrid"); }
    const char *SnapGridSpacing()   const { return ("SnapGridSpacing"); }
    const char *SnapPerGrid()       const { return ("SnapPerGrid"); }
    const char *GridPerSnap()       const { return ("GridPerSnap"); }
    const char *EdgeSnapping()      const { return ("EdgeSnapping"); }
    const char *RulerEdgeSnapping() const { return ("RulerEdgeSnapping"); }
    const char *RulerSnapToGrid()   const { return ("RulerSnapToGrid"); }

    // Attributes that set a variable, not allowed in print blocks.
    const char *BoxLineStyle()      const { return ("BoxLineStyle"); }
    const char *Constrain45()       const { return ("Constrain45"); }
    const char *LayerReorderMode()  const { return ("LayerReorderMode"); }
    const char *NoPlanarize()       const { return ("NoPlanarize"); }
    const char *RoundFlashSides()   const { return ("RoundFlashSides"); }
    const char *GridNoCoarseOnly()  const { return ("GridNoCoarseOnly"); }
    const char *GridThreshold()     const { return ("GridThreshold"); }

    // Function key macros.
    const char *FKey(int i)
        {
            snprintf(kwbuf, sizeof(kwbuf), "F%dKey", i);
            return (kwbuf);
        }

    // Grid register macros.
    const char *GridReg(int i)
        {
            snprintf(kwbuf, sizeof(kwbuf), "GridReg%d", i);
            return (kwbuf);
        }
    const char *PhysGridReg(int i)
        {
            snprintf(kwbuf, sizeof(kwbuf), "PhysGridReg%d", i);
            return (kwbuf);
        }
    const char *ElecGridReg(int i)
        {
            snprintf(kwbuf, sizeof(kwbuf), "ElecGridReg%d", i);
            return (kwbuf);
        }

    // Layer Palette macros.
    const char *PhysLayerPalette(int i)
        {
            snprintf(kwbuf, sizeof(kwbuf), "PhysLayerPalette%d", i);
            return (kwbuf);
        }
    const char *ElecLayerPalette(int i)
        {
            snprintf(kwbuf, sizeof(kwbuf), "ElecLayerPalette%d", i);
            return (kwbuf);
        }

    // Fonts
    const char *Font1()             const { return ("Font1"); }
    const char *Font2()             const { return ("Font2"); }
    const char *Font3()             const { return ("Font3"); }
    const char *Font4()             const { return ("Font4"); }
    const char *Font5()             const { return ("Font5"); }
    const char *Font6()             const { return ("Font6"); }
    const char *Font1P()            const { return ("Font1P"); }
    const char *Font2P()            const { return ("Font2P"); }
    const char *Font3P()            const { return ("Font3P"); }
    const char *Font4P()            const { return ("Font4P"); }
    const char *Font5P()            const { return ("Font5P"); }
    const char *Font6P()            const { return ("Font6P"); }
    const char *Font1Q()            const { return ("Font1Q"); }
    const char *Font2Q()            const { return ("Font2Q"); }
    const char *Font3Q()            const { return ("Font3Q"); }
    const char *Font4Q()            const { return ("Font4Q"); }
    const char *Font5Q()            const { return ("Font5Q"); }
    const char *Font6Q()            const { return ("Font6Q"); }
    const char *Font1W()            const { return ("Font1W"); }
    const char *Font2W()            const { return ("Font2W"); }
    const char *Font3W()            const { return ("Font3W"); }
    const char *Font4W()            const { return ("Font4W"); }
    const char *Font5W()            const { return ("Font5W"); }
    const char *Font6W()            const { return ("Font6W"); }
    const char *Font1X()            const { return ("Font1X"); }
    const char *Font2X()            const { return ("Font2X"); }
    const char *Font3X()            const { return ("Font3X"); }
    const char *Font4X()            const { return ("Font4X"); }
    const char *Font5X()            const { return ("Font5X"); }
    const char *Font6X()            const { return ("Font6X"); }

    // Conversion.
    const char *MergeOnRead()       const { return ("MergeOnRead"); }
    const char *InToLower()         const { return ("InToLower"); }
    const char *OutToUpper()        const { return ("OutToUpper"); }
    const char *StreamData()        const { return ("StreamData"); }
    const char *StreamIn()          const { return ("StreamIn"); }
    const char *StreamOut()         const { return ("StreamOut"); }
    const char *NoDrcDataType()     const { return ("NoDrcDataType"); }

    const char *LispLogging()       const { return ("LispLogging"); }
    const char *ReadDRF()           const { return ("ReadDRF"); }
    const char *ReadOaTech()        const { return ("ReadOaTech"); }
    const char *ReadCdsTech()       const { return ("ReadCdsTech"); }
    const char *ReadCdsLmap()       const { return ("ReadCdsLmap"); }
    const char *ReadCniTech()       const { return ("ReadCniTech"); }

    // Printing.
    const char *HardCopyDevice()    const { return ("HardCopyDevice"); }
    const char *DefaultDriver()     const { return ("DefaultDriver"); }
    const char *ElecDefaultDriver() const { return ("ElecDefaultDriver"); }
    const char *PhysDefaultDriver() const { return ("PhysDefaultDriver"); }

    // OBSOLETE but still recognized, synonyms for DefaultDriver, etc.
    const char *AltDriver()         const { return ("AltDriver"); }
    const char *AltElecDriver()     const { return ("AltElecDriver"); }
    const char *ElecAltDriver()     const { return ("ElecAltDriver"); }
    const char *AltPhysDriver()     const { return ("AltPhysDriver"); }
    const char *PhysAltDriver()     const { return ("PhysAltDriver"); }
    // Alias for LppName.
    const char *LongName()          const { return ("LongName"); }
    const char *GridSpacing()       const { return ("GridSpacing"); }
    const char *PhysGridSpacing()   const { return ("PhysGridSpacing"); }
    const char *ElecGridSpacing()   const { return ("ElecGridSpacing"); }
    const char *Snapping()          const { return ("Snapping"); }
    const char *PhysSnapping()      const { return ("PhysSnapping"); }
    const char *ElecSnapping()      const { return ("ElecSnapping"); }

    // OBSOLETE, unrecognized
    const char *AltAxes()           const { return ("AltAxes"); }
    const char *AltGridSpacing()    const { return ("AltGridSpacing"); }
    const char *AltPhysGridSpacing() const { return ("AltPhysGridSpacing"); }
    const char *PhysAltGridSpacing() const { return ("PhysAltGridSpacing"); }
    const char *AltElecGridSpacing() const { return ("AltElecGridSpacing"); }
    const char *ElecAltGridSpacing() const { return ("ElecAltGridSpacing"); }
    const char *AltShowGrid()       const { return ("AltShowGrid"); }
    const char *AltElecShowGrid()   const { return ("AltElecShowGrid"); }
    const char *ElecAltShowGrid()   const { return ("ElecAltShowGrid"); }
    const char *AltPhysShowGrid()   const { return ("AltPhysShowGrid"); }
    const char *PhysAltShowGrid()   const { return ("PhysAltShowGrid"); }
    const char *AltGridOnBottom()   const { return ("AltGridOnBottom"); }
    const char *AltElecGridOnBottom() const { return ("AltElecGridOnBottom"); }
    const char *ElecAltGridOnBottom() const { return ("ElecAltGridOnBottom"); }
    const char *AltPhysGridOnBottom() const { return ("AltPhysGridOnBottom"); }
    const char *PhysAltGridOnBottom() const { return ("PhysAltGridOnBottom"); }
    const char *AltGridStyle()      const { return ("AltGridStyle"); }
    const char *AltElecGridStyle()  const { return ("AltElecGridStyle"); }
    const char *ElecAltGridStyle()  const { return ("ElecAltGridStyle"); }
    const char *AltPhysGridStyle()  const { return ("AltPhysGridStyle"); }
    const char *PhysAltGridStyle()  const { return ("PhysAltGridStyle"); }
    const char *AltBackground()     const { return ("AltBackground"); }
    const char *AltElecBackground() const { return ("AltElecBackground"); }
    const char *ElecAltBackground() const { return ("ElecAltBackground"); }
    const char *AltPhysBackground() const { return ("AltPhysBackground"); }
    const char *PhysAltBackground() const { return ("PhysAltBackground"); }

    // Print driver block keywords.
    const char *HardCopyCommand()   const { return ("HardCopyCommand"); }
    const char *HardCopyResol()     const { return ("HardCopyResol"); }
    const char *HardCopyDefResol()  const { return ("HardCopyDefResol"); }
    const char *HardCopyLegend()    const { return ("HardCopyLegend"); }
    const char *HardCopyOrient()    const { return ("HardCopyOrient"); }
    const char *HardCopyDefWidth()  const { return ("HardCopyDefWidth"); }
    const char *HardCopyDefHeight() const { return ("HardCopyDefHeight"); }
    const char *HardCopyDefXoff()   const { return ("HardCopyDefXoff"); }
    const char *HardCopyDefYoff()   const { return ("HardCopyDefYoff"); }
    const char *HardCopyMinWidth()  const { return ("HardCopyMinWidth"); }
    const char *HardCopyMinHeight() const { return ("HardCopyMinHeight"); }
    const char *HardCopyMinXoff()   const { return ("HardCopyMinXoff"); }
    const char *HardCopyMinYoff()   const { return ("HardCopyMinYoff"); }
    const char *HardCopyMaxWidth()  const { return ("HardCopyMaxWidth"); }
    const char *HardCopyMaxHeight() const { return ("HardCopyMaxHeight"); }
    const char *HardCopyMaxXoff()   const { return ("HardCopyMaxXoff"); }
    const char *HardCopyMaxYoff()   const { return ("HardCopyMaxYoff"); }
    const char *HardCopyPixWidth()  const { return ("HardCopyPixWidth"); }

    // OBSOLETE, unrecognized
    const char *NoAlt()             const { return ("NoAlt"); }

private:
    char kwbuf[64];
};

extern sTKW Tkw;

// Base for keyword handling class for keyword editors.
//
struct tcKWstruct
{
    tcKWstruct()
        {
            kw_list = 0;
            kw_undolist = 0;
            kw_newstr = 0;
        }

    ~tcKWstruct()
        {
            stringlist::destroy(kw_list);
            stringlist::destroy(kw_undolist);
        }

    virtual void load_keywords(const CDl*, const char*) = 0;
    virtual char *insert_keyword_text(const char*, const char* = 0,
        const char* = 0) = 0;
    virtual void remove_keyword_text(int, bool = false, const char* = 0,
        const char* = 0) = 0;

    void undo_keyword_change();
    void clear_undo_list();

    stringlist *list()          { return (kw_list); }
    stringlist *undo_list()     { return (kw_undolist); }
    const char *new_string()    { return (kw_newstr); }

protected:
    void remove(stringlist *prev, stringlist *sl)
        {
            if (prev)
                prev->next = sl->next;
            else
                kw_list = sl->next;
            sl->next = kw_undolist;
            kw_undolist = sl;
        }

    stringlist *kw_list;        // list of keyword assignment strings
    stringlist *kw_undolist;    // assignments removed by last operation
    char *kw_newstr;            // last string added
};

#endif

