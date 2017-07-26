
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
 $Id: tech_cds_in.h,v 5.11 2016/05/14 20:29:46 stevew Exp $
 *========================================================================*/

#ifndef TECH_CDS_IN_H
#define TECH_CDS_IN_H


//-----------------------------------------------------------------------------
// cTechCdsIn  Cadence Virtuoso ascii tech file reader

inline class cTechCdsIn *CdsIn();

// Main class for compatibility package
//
class cTechCdsIn : public cLispEnv
{
public:
    friend inline cTechCdsIn *CdsIn()
        {
            if (!cTechCdsIn::instancePtr)
                new cTechCdsIn;
            return (cTechCdsIn::instancePtr);
        }

    cTechCdsIn();
    ~cTechCdsIn();

    bool read(const char*, char**);

    static bool readLayerMap(const char*);

private:
    static void setupCdsReserved();
    void register_drc();

    static bool comment(lispnode*, lispnode*, char**);;
    static bool include_node(lispnode*, lispnode*, char**);;
    static bool class_dispatch(lispnode*, lispnode*, char**);;

    // controls
    static bool techParams(lispnode*, lispnode*, char**);
    static bool techPermissions(lispnode*, lispnode*, char**);
    static bool viewTypeUnits(lispnode*, lispnode*, char**);
    static bool mfgGridResolution(lispnode*, lispnode*, char**);

    // layerDefinitions
    static bool techLayers(lispnode*, lispnode*, char**);
    static bool techPurposes(lispnode*, lispnode*, char**);
    static bool techLayerPurposePriorities(lispnode*, lispnode*, char**);
    static bool techDisplays(lispnode*, lispnode*, char**);
    static bool techLayerProperties(lispnode*, lispnode*, char**);
    static bool techDerivedLayers(lispnode*, lispnode*, char**);

    // layerRules
    static bool functions(lispnode*, lispnode*, char**);
    static bool routingDirections(lispnode*, lispnode*, char**);
    static bool stampLabelLayers(lispnode*, lispnode*, char**);
    static bool currentDensityTables(lispnode*, lispnode*, char**);
    static bool viaLayers(lispnode*, lispnode*, char**);
    static bool equivalentLayers(lispnode*, lispnode*, char**);
    static bool streamLayers(lispnode*, lispnode*, char**);

    // viaDefs
    static bool standardViaDefs(lispnode*, lispnode*, char**);
    static bool customViaDefs(lispnode*, lispnode*, char**);

    // constraintGroups
    static bool constraintGroups(lispnode*, lispnode*, char**);
    static bool foundry(lispnode*, lispnode*, char**);
    static bool spacings(lispnode*, lispnode*, char**);
    static bool viaStackingLimits(lispnode*, lispnode*, char**);
    static bool spacingTables(lispnode*, lispnode*, char**);
    static bool orderedSpacings(lispnode*, lispnode*, char**);
    static bool antennaModels(lispnode*, lispnode*, char**);
    static bool electrical(lispnode*, lispnode*, char**);
    static bool memberConstraintGroups(lispnode*, lispnode*, char**);
    static bool LEFDefaultRouteSpec(lispnode*, lispnode*, char**);
    static bool interconnect(lispnode*, lispnode*, char**);
    static bool routingGrids(lispnode*, lispnode*, char**);

    // devices
    static bool tcCreateCDSDeviceClass(lispnode*, lispnode*, char**);
    static bool multipartPathTemplates(lispnode*, lispnode*, char**);
    static bool extractMOS(lispnode*, lispnode*, char**);
    static bool extractRES(lispnode*, lispnode*, char**);
    static bool symContactDevice(lispnode*, lispnode*, char**);
    static bool ruleContactDevice(lispnode*, lispnode*, char**);
    static bool symEnhancementDevice(lispnode*, lispnode*, char**);
    static bool symDepletionDevice(lispnode*, lispnode*, char**);
    static bool symPinDevice(lispnode*, lispnode*, char**);
    static bool symRectPinDevice(lispnode*, lispnode*, char**);
    static bool tcCreateDeviceClass(lispnode*, lispnode*, char**);
    static bool tcDeclareDevice(lispnode*, lispnode*, char**);
    static bool tfcDefineDeviceClassProp(lispnode*, lispnode*, char**);

    // viaSpecs
    static bool viaSpecs(lispnode*, lispnode*, char**);

    //
    // Below are V5 nodes, mostly not supported.
    //

    // physicalRules
    static bool orderedSpacingRules(lispnode*, lispnode*, char**);
    static bool spacingRules(lispnode*, lispnode*, char**);

    // electricalRules
    static bool characterizationRules(lispnode*, lispnode*, char**);
    static bool orderedCharacterizationRules(lispnode*, lispnode*, char**);

    // leRules
    static bool leLswLayers(lispnode*, lispnode*, char**);

    // lxRules
    static bool lxExtractLayers(lispnode*, lispnode*, char**);
    static bool lxNoOverlapLayers(lispnode*, lispnode*, char**);
    static bool lxMPPTemplates(lispnode*, lispnode*, char**);

    // compactorRules
    static bool compactorLayers(lispnode*, lispnode*, char**);
    static bool symWires(lispnode*, lispnode*, char**);
    static bool symRules(lispnode*, lispnode*, char**);

    // lasRules
    static bool lasLayers(lispnode*, lispnode*, char**);
    static bool lasDevices(lispnode*, lispnode*, char**);
    static bool lasWires(lispnode*, lispnode*, char**);
    static bool lasProperties(lispnode*, lispnode*, char**);

    // prRules
    static bool prRoutingLayers(lispnode*, lispnode*, char**);
    static bool prViaTypes(lispnode*, lispnode*, char**);
    static bool prStackVias(lispnode*, lispnode*, char**);
    static bool prMastersliceLayers(lispnode*, lispnode*, char**);
    static bool prViaRules(lispnode*, lispnode*, char**);
    static bool prGenViaRules(lispnode*, lispnode*, char**);
    static bool prTurnViaRules(lispnode*, lispnode*, char**);
    static bool prNonDefaultRules(lispnode*, lispnode*, char**);
    static bool prRoutingPitch(lispnode*, lispnode*, char**);
    static bool prRoutingOffset(lispnode*, lispnode*, char**);
    static bool prOverlapLayer(lispnode*, lispnode*, char**);

    static cTechCdsIn *instancePtr;
};

#endif

