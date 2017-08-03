
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Wayne A. Christopher, Norbert Jeske
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef SUBEXPAND_H
#define SUBEXPAND_H


//
// Some state exported from the subcircuit expansion processor.
//

struct sLine;
struct sParamTab;
class cUdf;
struct wordlist;
struct sModTab;
struct sSubcTab;
struct sCblk;
struct sCblkTab;

struct sSPcache
{
    friend struct sScGlobal;

    sSPcache() { cache_tab = 0; }

    void initCache();
    bool inCache(const char*);
    wordlist *listCache();
    wordlist *dumpCache(const char*);
    bool removeCache(const char*);
    void clearCache();

private:
    void add(const char*, sSubcTab*, const sParamTab*, sModTab*, const cUdf*);
    sCblk *get(const char*);

    sCblkTab *cache_tab;
};

enum ParHierMode { ParHierGlobal, ParHierLocal };

struct
sSPcx
{
    friend struct sScGlobal;

    sSPcx()
        {
            cx_pexnodes = false;
            cx_nobjthack = false;
            cx_parhier = ParHierGlobal;
            cx_catchar = DEF_SUBC_CATCHAR;
            cx_catmode = SUBC_CATMODE_WR;

            cx_start = 0;
            cx_sbend = 0;
            cx_invoke = 0;
            cx_model = 0;
        }

    void kwMatchInit();
    bool kwMatchSubstart(const char*);
    bool kwMatchSubend(const char*);
    bool kwMatchSubinvoke(const char*);
    bool kwMatchModel(const char*);

    bool pexnodes()         const { return (cx_pexnodes); }
    bool nobjthack()        const { return (cx_nobjthack); }
    int catchar()           const { return (cx_catchar); }
    int catmode()           const { return (cx_catmode); }
    ParHierMode parhier()   const { return ((ParHierMode)cx_parhier); }

    const char *start()     const { return (cx_start); }
    const char *sbend()     const { return (cx_sbend); }
    const char *invoke()    const { return (cx_invoke); }
    const char *model()     const { return (cx_model); }

private:
    void init(sFtCirc*);

    bool cx_pexnodes;
    bool cx_nobjthack;
    char cx_parhier;
    char cx_catchar;
    char cx_catmode;

    const char *cx_start;
    const char *cx_sbend;
    const char *cx_invoke;
    const char *cx_model;
};

extern sSPcache SPcache;
extern sSPcx SPcx;

#endif

