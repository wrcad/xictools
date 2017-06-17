
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
 $Id: kwstr_lyr.h,v 5.2 2014/07/03 22:53:07 stevew Exp $
 *========================================================================*/

#ifndef KWSTR_LYR_H
#define KWSTR_LYR_H

#include "tech_kwords.h"

//
// Class and methods for layer attribute keyword processing.
//


// Code for recognized attribute keywords.
//
enum lpKW
{
    lpNil,
    lpLppName,
    lpDescription,
    lpNoSelect,
    lpNoMerge,
    lpWireActive,
    lpSymbolic,
    lpInvalid,
    lpInvisible,
    lpNoInstView,
    lpWireWidth,
    lpCrossThick
};


// Maintain a list of extraction keyword/values, used when modifying
// layer definitions.
//
struct lyrKWstruct : public tcKWstruct
{
    lyrKWstruct()
        {
            kw_editing = false;
        }

    void load_keywords(const CDl*, const char*);
    char *insert_keyword_text(const char*, const char* = 0, const char* = 0);
    void remove_keyword_text(int, bool = false, const char* = 0,
        const char* = 0);

    char *list_keywords();
    bool inlist(lpKW);
    void sort();
    char *prompt(const char*, const char*);
    char *get_string_for(int, const char*);

    static lpKW kwtype(const char*);
    static char *get_settings(const CDl*);
    static char *set_settings(CDl*, const char*);

    bool is_editing()       { return (kw_editing); }

private:
    bool kw_editing;
};

#endif 

