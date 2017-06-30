
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
 $Id: geo_zgroup.h,v 5.4 2013/10/01 05:21:04 stevew Exp $
 *========================================================================*/

#ifndef GEO_ZGROUP_H
#define GEO_ZGROUP_H

#include "geo_zlist.h"


// Group of zoid lists, each list is mutually contacting.
//
struct Zgroup
{
    Zgroup()
        {
            list = 0;
            num = 0;
        }

    ~Zgroup()
        {
            while (num--)
                Zlist::destroy(list[num]);
            delete [] list;
        }

    PolyList *to_poly_list(int, int);
    Zlist *zoids();
    CDs *mk_cell(CDl*);

    Zlist **list;
    int num;
};

#endif

