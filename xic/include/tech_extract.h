
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

#ifndef TECH_EXTRACT_H
#define TECH_EXTRACT_H

// Misc.  definitions for techfile support for the extraction system. 
// This is moved from the extraction system to the main techfile code
// for two reasons:  1) Better commonality of techfiles with
// derivative programs (XicII, Xiv) that don't include the extraction
// package, and 2) commonality of the layer sequencing code for cross
// section display and the extraction Fast[er]Cap interface.

// Extraction Tech Variables
#define VA_AntennaTotal         "AntennaTotal"
#define VA_SubstrateEps         "SubstrateEps"
#define VA_SubstrateThickness   "SubstrateThickness"

#define VA_GroundPlaneGlobal    "GroundPlaneGlobal"
#define VA_GroundPlaneMulti     "GroundPlaneMulti"
#define VA_GroundPlaneMethod    "GroundPlaneMethod"

// Default substrate thickness (microns), dielectric constants.
#define SUBSTRATE_THICKNESS     75.0
#define SUBSTRATE_THICKNESS_MIN 0.0
#define SUBSTRATE_THICKNESS_MAX 10000.0
#define SUBSTRATE_EPS           11.9
#define SUBSTRATE_EPS_MIN       1.0
#define SUBSTRATE_EPS_MAX       20.0

// A repository for the extraction tech file keywords.
//
struct sExtKW
{
    // Layer keywords
    const char *Conductor()         const { return ("Conductor"); }
    const char *Exclude()           const { return ("Exclude"); }
    const char *Routing()           const { return ("Routing"); }
    const char *GroundPlane()       const { return ("GroundPlane"); }
    const char *GroundPlaneDark()   const { return ("GroundPlaneDark"); }
    const char *GroundPlaneClear()  const { return ("GroundPlaneClear"); }
    const char *Global()            const { return ("Global"); }
    const char *TermDefault()       const { return ("TermDefault"); }
    const char *MultiNet()          const { return ("MultiNet"); }
    const char *Contact()           const { return ("Contact"); }
    const char *Via()               const { return ("Via"); }
    const char *ViaCut()            const { return ("ViaCut"); }
    const char *Dielectric()        const { return ("Dielectric"); }
    const char *Planarize()         const { return ("Planarize"); }
    const char *DarkField()         const { return ("DarkField"); }

    const char *Thickness()         const { return ("Thickness"); }
    const char *Rho()               const { return ("Rho"); }
    const char *Sigma()             const { return ("Sigma"); }
    const char *Tau()               const { return ("Tau"); }
    const char *FH_nhinc()          const { return ("FH_nhinc"); }
    const char *FH_rh()             const { return ("FH_rh"); }
    const char *Rsh()               const { return ("Rsh"); }
    const char *EpsRel()            const { return ("EpsRel"); }
    const char *Cap()               const { return ("Cap"); }
    const char *Capacitance()       const { return ("Capacitance"); }
    const char *Lambda()            const { return ("Lambda"); }
    const char *Tline()             const { return ("Tline"); }
    const char *Antenna()           const { return ("Antenna"); }

    // Global keywords.
    const char *AntennaTotal()      const { return ("AntennaTotal"); }
    const char *SubstrateEps()      const { return ("SubstrateEps"); }
    const char *SubstrateThickness() const { return ("SubstrateThickness"); }

    // Device block keywords.
    const char *DeviceTemplate()    const { return ("DeviceTemplate"); }
    const char *Device()            const { return ("Device"); }
    const char *Template()          const { return ("Template"); }
    const char *Name()              const { return ("Name"); }
    const char *Prefix()            const { return ("Prefix"); }
    const char *Body()              const { return ("Body"); }
    const char *Find()              const { return ("Find"); }
    const char *DevContact()        const { return ("Contact"); }
    const char *BulkContact()       const { return ("BulkContact"); }
    const char *Permute()           const { return ("Permute"); }
    const char *Depth()             const { return ("Depth"); }
    const char *Bloat()             const { return ("Bloat"); }
    const char *Merge()             const { return ("Merge"); }
    const char *Measure()           const { return ("Measure"); }
    const char *LVS()               const { return ("LVS"); }
    const char *Spice()             const { return ("Spice"); }
    const char *Cmput()             const { return ("Cmput"); }
    const char *Model()             const { return ("Model"); }
    const char *Value()             const { return ("Value"); }
    const char *Param()             const { return ("Param"); }
    const char *Initc()             const { return ("Initc"); }
    const char *ContactsOverlap()   const { return ("ContactsOverlap"); }
    const char *SimpleMinDimen()    const { return ("SimpleMinDimen"); }
    const char *ContactMinDimen()   const { return ("ContactMinDimen"); }
    const char *NotMOS()            const { return ("NotMOS"); }
    const char *MOS()               const { return ("MOS"); }
    const char *NMOS()              const { return ("NMOS"); }
    const char *PMOS()              const { return ("PMOS"); }
    const char *Ntype()             const { return ("Ntype"); }
    const char *Ptype()             const { return ("Ptype"); }
    const char *End()               const { return ("End"); }

    // BulkContact types
    const char *immed()             const { return ("immed"); }
    const char *skip()              const { return ("skip"); }
    const char *defer()             const { return ("defer"); }
};

extern sExtKW Ekw;

#endif

