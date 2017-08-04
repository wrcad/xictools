
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

