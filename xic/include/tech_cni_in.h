
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2012 Whiteley Research Inc, all rights reserved.        *
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
 $Id: tech_cni_in.h,v 5.2 2012/10/18 20:10:57 stevew Exp $
 *========================================================================*/

#ifndef TECH_CNI_IN_H
#define TECH_CNI_IN_H


//-----------------------------------------------------------------------------
// cTechCdsIn  Cadence Virtuoso ascii tech file reader

inline class cTechCniIn *CniIn();

// Main class for compatibility package
//
class cTechCniIn : public cLispEnv
{
public:
    friend inline cTechCniIn *CniIn() { return (cTechCniIn::instancePtr); }

    cTechCniIn();
    ~cTechCniIn();

    bool read(const char*, char**);

private:
    // Santana display
    static bool dispLayerPurposePairs(lispnode*, lispnode*, char**);

    // Santana tech
    static bool techId(lispnode*, lispnode*, char**);
    static bool viewTypeUnits(lispnode*, lispnode*, char**);
    static bool mfgGridResolution(lispnode*, lispnode*, char**);
    static bool layerMapping(lispnode*, lispnode*, char**);
    static bool maskNumbers(lispnode*, lispnode*, char**);
    static bool layerMaterials(lispnode*, lispnode*, char**);
    static bool purposeMapping(lispnode*, lispnode*, char**);
    static bool derivedLayers(lispnode*, lispnode*, char**);
    static bool connectivity(lispnode*, lispnode*, char**);
    static bool viaLayers(lispnode*, lispnode*, char**);
    static bool physicalRules(lispnode*, lispnode*, char**);
    static bool deviceContext(lispnode*, lispnode*, char**);
    static bool characterizationRules(lispnode*, lispnode*, char**);
    static bool oxideDefinitions(lispnode*, lispnode*, char**);
    static bool mosfetDefinitions(lispnode*, lispnode*, char**);

    static cTechCniIn *instancePtr;
};

#endif

