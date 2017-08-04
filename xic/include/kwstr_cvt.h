
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

#ifndef KWSTR_CVT_H
#define KWSTR_CVT_H

#include "tech_kwords.h"

//
// Class and methods for layer attribute keyword processing.
//


// Code for recognized attribute keywords.
//
enum cvKW
{
    cvNil,
    cvStreamIn,
    cvStreamOut,
    cvNoDrcDataType
};


// Maintain a list of extraction keyword/values, used when modifying
// layer definitions.
//
struct cvtKWstruct : public tcKWstruct
{
    cvtKWstruct()
        {
            kw_editing = false;
        }

    void load_keywords(const CDl*, const char*);
    char *insert_keyword_text(const char*, const char* = 0, const char* = 0);
    void remove_keyword_text(int, bool = false, const char* = 0,
        const char* = 0);

    char *list_keywords();
    bool inlist(cvKW);
    void sort();
    char *prompt(const char*, const char*);
    char *get_string_for(int, const char*);

    static cvKW kwtype(const char*);
    static char *get_settings(const CDl*);
    static char *set_settings(CDl*, const char*);

    bool is_editing()       { return (kw_editing); }

private:
    bool kw_editing;
};

#endif 

