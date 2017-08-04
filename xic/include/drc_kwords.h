
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

#ifndef DRC_KWORDS_H
#define DRC_KWORDS_H


// A repository for the DRC tech file keywords.
//
struct sDrcKW
{
    // Rule keywords.
    const char *Exist()             const { return ("Exist"); }
    const char *Connected()         const { return ("Connected"); }
    const char *NoHoles()           const { return ("NoHoles"); }
    const char *Overlap()           const { return ("Overlap"); }
    const char *IfOverlap()         const { return ("IfOverlap"); }
    const char *NoOverlap()         const { return ("NoOverlap"); }
    const char *AnyOverlap()        const { return ("AnyOverlap"); }
    const char *PartOverlap()       const { return ("PartOverlap"); }
    const char *AnyNoOverlap()      const { return ("AnyNoOverlap"); }
    const char *MinArea()           const { return ("MinArea"); }
    const char *MaxArea()           const { return ("MaxArea"); }
    const char *MinEdgeLength()     const { return ("MinEdgeLength"); }
    const char *MaxWidth()          const { return ("MaxWidth"); }
    const char *MinWidth()          const { return ("MinWidth"); }
    const char *MinSpace()          const { return ("MinSpace"); }
    const char *MinSpaceTo()        const { return ("MinSpaceTo"); }
    const char *MinSpaceFrom()      const { return ("MinSpaceFrom"); }
    const char *MinOverlap()        const { return ("MinOverlap"); }
    const char *MinNoOverlap()      const { return ("MinNoOverlap"); }

    const char *Diagonal()          const { return ("Diagonal"); }
    const char *SameNet()           const { return ("SameNet"); }
    const char *Enclosed()          const { return ("Enclosed"); }
    const char *Opposite()          const { return ("Opposite"); }
    const char *Region()            const { return ("Region"); }
    const char *Inside()            const { return ("Inside"); }
    const char *Outside()           const { return ("Outside"); }

    // Global keywords.
    const char *Drc()               const { return ("Drc"); }
    const char *DrcLevel()          const { return ("DrcLevel"); }
    const char *DrcNoPopup()        const { return ("DrcNoPopup"); }
    const char *DrcMaxErrors()      const { return ("DrcMaxErrors"); }
    const char *DrcInterMaxObjs()   const { return ("DrcInterMaxObjs"); }
    const char *DrcInterMaxTime()   const { return ("DrcInterMaxTime"); }
    const char *DrcInterMaxErrors() const { return ("DrcInterMaxErrors"); }
    const char *DrcInterSkipInst()  const { return ("DrcInterSkipInst"); }

    // User-defined rule keywords.
    const char *UserDefinedRule()   const { return ("UserDefinedRule"); }
    const char *DrcTest()           const { return ("DrcTest"); }
    const char *Edge()              const { return ("Edge"); }
    const char *Test()              const { return ("Test"); }
    const char *Evaluate()          const { return ("Evaluate"); }
    const char *MinEdge()           const { return ("MinEdge"); }
    const char *MaxEdge()           const { return ("MaxEdge"); }
    const char *TestCornerOverlap() const { return ("TestCornerOverlap"); }
    const char *End()               const { return ("End"); }
};

extern sDrcKW Dkw;

#endif

