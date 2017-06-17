
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
 $Id: array.h,v 5.1 2009/01/22 06:38:04 stevew Exp $
 *========================================================================*/

#ifndef ARRAY_H
#define ARRAY_H


//
// Instance array manipulation class.
//

struct
sArrayManip : public cTfmStack
{
    sArrayManip(CDs *sdesc, CDc *cdesc) { am_sdesc = sdesc; am_cdesc = cdesc; }

    bool unarray();
    bool delete_elements(unsigned int, unsigned int, unsigned int,
        unsigned int);
    bool reconfigure(unsigned int, int, unsigned int, int);

    void set_instance(CDc *cd) { am_cdesc = cd; }

private:
    bool mk_instance(const BBox *BB);

    CDs *am_sdesc;
    CDc *am_cdesc;
};

#endif

