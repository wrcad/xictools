
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#ifndef KWSTR_PHY_H
#define KWSTR_PHY_H

#include "tech_kwords.h"

//
// Class and methods for physical attribute layer keyword processing.
//


// Code for recognized keywords.
//
enum phKW
{
    phNil,
    phPlanarize,
    phThickness,
    phRho,
    phSigma,
    phRsh,
    phEpsRel,
    phCapacitance,
    phLambda,
    phTline,
    phAntenna
};


// Maintain a list of extraction keyword/values, used when modifying
// layer definitions.
//
struct phyKWstruct : public tcKWstruct
{
    phyKWstruct()
        {
            kw_editing = false;
        }

    void load_keywords(const CDl*, const char*);
    char *insert_keyword_text(const char*, const char* = 0, const char* = 0);
    void remove_keyword_text(int, bool = false, const char* = 0,
        const char* = 0);

    char *list_keywords();
    bool inlist(phKW);
    void sort();
    char *prompt(const char*, const char*);
    char *get_string_for(int, const char*);

    static phKW kwtype(const char*);
    static char *get_settings(const CDl*);
    static char *set_settings(CDl*, const char*);

    bool is_editing()       { return (kw_editing); }

private:
    bool kw_editing;
};

#endif 

