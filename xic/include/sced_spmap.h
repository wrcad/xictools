
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2014 Whiteley Research Inc, all rights reserved.        *
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
 $Id: sced_spmap.h,v 5.1 2014/10/05 03:17:48 stevew Exp $
 *========================================================================*/

#ifndef SCED_SPMAP_H
#define SCED_SPMAP_H

// A table and functions to expand the .include/.lib and similar lines
// in a SPICE deck, referencing files on the local machine.`
//
struct sLibMap
{
    sLibMap()
        {
            lib_tab = 0;
        }

    ~sLibMap();

    bool expand_includes(stringlist**, const char*);

private:
    stringlist *append_file_rc(stringlist*, const char*, bool, bool*);
    long find(const char*, const char*);
    SymTab *map_lib(FILE*);

    SymTab *lib_tab;
};

#endif

