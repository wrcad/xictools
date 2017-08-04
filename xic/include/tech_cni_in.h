
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
    friend inline cTechCniIn *CniIn()
        {
            if (!cTechCniIn::instancePtr)
                new cTechCniIn;
            return (cTechCniIn::instancePtr);
        }

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

