
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

/***************************************************************************
JSPICE3 adaptation of Spice3e2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1987 Thomas L. Quarles
         1992 Stephen R. Whiteley
****************************************************************************/

#ifndef INPUT_H
#define INPUT_H

#include "inpline.h"
#include "ifdata.h"
#include "errors.h"


//
// Declarations used by Spice input processor
//

// references
class cUdf;
struct sCKT;
struct sJOB;
struct sCKTtable;
struct sTASK;
struct sGENmodel;
struct sGENinstance;
struct IFparseNode;
struct IFparseTree;
struct sModTab;

// Structure used to temporarily save models after they are read
// during pass 1.
//
struct sINPmodel
{
    sINPmodel(char *n, int t, char *l)
        {
            modName = n;
            modLine = l;
            modType = t;
        }

    ~sINPmodel()
        {
            delete [] modName;
            delete [] modLine;
        }

    char *modName;         // name of model
    char *modLine;         // text of .model line
    int modType;           // device type index
};


// For use with the SPinput::logError functions.
#define IP_CUR_LINE (sLine*)(long)(-1)

// The overall class definition.  The SPinput class handles the parsing
// of Spice input.  The actual parsing functions for analyses and devices
// are in the respective analysis and device code, but are accessed through
// the functions below.  The class also handles error reporting for input
// processing.  Errors are added to the input deck.
//
class SPinput
{
public:

    // inpdeck.cc
    void parseDeck(sCKT*, sLine*, sTASK*, bool);

    // inpdev.cc
    void devParse(sLine*, const char**, sCKT*, sGENinstance*, const char*);
    int numRefs(const char*, int*, int*, int*, int*);

    // inpdotcd.cc
    int checkDotCard(const char*);
    void parseOptions(sLine*, sJOB*);

    // inperror.cc
    void logError(sLine*, const char*, ...);
    void logError(sLine*, int, const char* = 0);
    char *errMesg(int, const char* = 0);
    const char *errMesgShort(int);

    // inpmodel.cc
    bool getMod(sLine*, sCKT*, const char*, const char*, sINPmodel**);
    bool lookMod(const char*);
    void parseMod(sLine*);
    void parseDefMod(sLine*, sCKT*);
    bool checkKey(char, int);

    // inptran.cc
    bool isTranFunc(const char*);
    char *getTranFunc(const char**, bool);
    char *fixParentheses(const char*, sCKT*, const char*);

    // inptabpa.cc
    bool tablFind(const char*, sCKTtable**, sCKT *ckt);

    // inptoken.cc
    char *getTok(const char**, bool);
    void advTok(const char**, bool);
    bool getValue(const char**, IFdata*, sCKT*, const char* = 0);
    double getFloat(const char**, int*, bool);
    bool getOption(const char**, char**, char**);

    // Current parse line, set in pass1 and pass2, used in the
    // parser functions in inpptree.cc.
    //
    sLine *currentLine() { return (ip_current_line); }

    // Swap the temporary model table.
    sModTab *swapModTab(sModTab *t)
         {
            sModTab *tmp = ip_modtab;
            ip_modtab = t;
            return (tmp);
         }

    // Set the model cache table.
    void setModCache(sModTab *m) { ip_modcache = m; }

private:
    // inpdeck.cc
    void pass1(sCKT*, sLine*);
    void pass2(sCKT*, sLine*, sTASK*);
    int findDev(const char*);

    // inpdotcd.cc
    IFanalysis *getAnalysis(const char*, int*);
    bool parseDot(sCKT*, sTASK*, sLine*);

    // inpmodel.cc
    sINPmodel *findMod(const char*);
    int addMod(const char*, int, const char*);
    void killMods();
    char *lookParam(const char*, const char*);
    void addMosLevelMapping(int, int);
    void delMosLevelMapping(int);
    void clearMosLevelMaps();
    int getMosLevelMapping(int);
    bool findLev(const char*, int*, bool);
    sINPmodel *mosFind(sLine*, const char*, double, double);
    int mosLWcnds(sLine*, sINPmodel*, double, double);
    bool instantiateMod(sLine*, sINPmodel*, sCKT*);

    // inptabpa.cc
    void tablParse(sLine*, sCKT*);
    void tablCheck(sLine*, sCKT*);
    void tablFix(sCKT*);

    sLine *ip_current_line;     // Current line being parsed
    sModTab *ip_modtab;         // Hash table of models
    sModTab *ip_badmodtab;      // Hash table of models with errors
    sModTab *ip_modcache;       // Hash table of cached models
};

extern SPinput IP;

#endif // INPUT_H

