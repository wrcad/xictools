
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef KEYWORDS_H
#define KEYWORDS_H

#include "hash.h"


//
// Data types for internal variables.
//

// Basic keyword.
struct sKW
{
    sKW() { }
    sKW(const char *w, const char *d)
        { word = w; descr = d; type = VTYP_NONE; }
    virtual ~sKW () { }
    virtual void print(char **rstr);

    const char *word;
    const char *descr;
    VTYPenum type;
};

struct userEnt
{
    virtual ~userEnt() { }

    virtual void callback(bool, variable*) { }
};

template<class T> struct sKWent : public sKW
{
    sKWent(const char *w=0, VTYPenum t=VTYP_NONE, const char *d=0)
        { set(w, t, 0.0, 0.0, d); lastv1 = lastv2 = 0; ent = 0; }
    void set(const char *w, VTYPenum t, double mi, double mx, const char *d)
        { word = w; type = t; min = mi; max = mx; descr = d; }
    void init()
        { Sp.Options()->add(word, this);
        CP.AddKeyword(CT_VARIABLES, word); }
    virtual void callback(bool isset, variable *v)
        { if (ent) ent->callback(isset, v); }

    double min, max;     // for numeric variables
    const char *lastv1;  // previous value set, for graphics
    const char *lastv2;  // previous value set, for graphics
    T *ent;              // used in graphics subsystem
};

class cKeyWords
{
public:
    void initDatabase();

    sKW *pstyles(int i)     { return (KWpstyles[i]); }
    sKW *gstyles(int i)     { return (KWgstyles[i]); }
    sKW *scale(int i)       { return (KWscale[i]); }
    sKW *plot(int i)        { return (KWplot[i]); }
    sKW *color(int i)       { return (KWcolor[i]); }
    sKW *dbargs(int i)      { return (KWdbargs[i]); }
    sKW *debug(int i)       { return (KWdebug[i]); }
    sKW *ft(int i)          { return (KWft[i]); }
    sKW *level(int i)       { return (KWlevel[i]); }
    sKW *units(int i)       { return (KWunits[i]); }
    sKW *spec(int i)        { return (KWspec[i]); }
    sKW *cmds(int i)        { return (KWcmds[i]); }
    sKW *shell(int i)       { return (KWshell[i]); }
    sKW *step(int i)        { return (KWstep[i]); }
    sKW *method(int i)      { return (KWmethod[i]); }
    sKW *optmerge(int i)    { return (KWoptmerge[i]); }
    sKW *parhier(int i)     { return (KWparhier[i]); }
    sKW *sim(int i)         { return (KWsim[i]); }

private:
    static sKW *KWpstyles[];
    static sKW *KWgstyles[];
    static sKW *KWscale[];
    static sKW *KWplot[];
    static sKW *KWcolor[];
    static sKW *KWdbargs[];
    static sKW *KWdebug[];
    static sKW *KWft[];
    static sKW *KWlevel[];
    static sKW *KWunits[];
    static sKW *KWspec[];
    static sKW *KWcmds[];
    static sKW *KWshell[];
    static sKW *KWstep[];
    static sKW *KWmethod[];
    static sKW *KWoptmerge[];
    static sKW *KWparhier[];
    static sKW *KWsim[];
};

extern cKeyWords KW;

#endif // KEYWORDS_H

