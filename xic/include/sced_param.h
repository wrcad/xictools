
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
 $Id: sced_param.h,v 5.2 2013/08/20 04:47:19 stevew Exp $
 *========================================================================*/

#ifndef SCED_PARAM_H
#define SCED_PARAM_H


struct CDs;
struct sParamTab;

class cParamCx
{
public:
    struct PTlist
    {
        PTlist(sParamTab *pt, PTlist *n)
            {
                ptab = pt;
                next = n;
            }

        sParamTab *ptab;
        PTlist *next;
    };

    cParamCx(const CDs*);
    ~cParamCx();

    void push(const CDs*, const char*);
    void pop();
    void update(char**);
    bool has_param(const char*);

    static bool localParHier(const CDs*);
    static sParamTab *buildParamTab(const CDs*);

private:
    sParamTab   *pc_tab;
    PTlist      *pc_stack;
    bool        pc_local_parhier;
};

#endif

