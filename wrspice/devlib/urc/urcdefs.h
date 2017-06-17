
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 *========================================================================*
 *                                                                        *
 * Device Library                                                         *
 *                                                                        *
 *========================================================================*
 $Id: urcdefs.h,v 2.22 2016/09/26 01:48:35 stevew Exp $
 *========================================================================*/

/***************************************************************************
JSPICE3 adaptation of Spice3f2 - Copyright (c) Stephen R. Whiteley 1992
Copyright 1990 Regents of the University of California.  All rights reserved.
Authors: 1985 Thomas L. Quarles
         1993 Stephen R. Whiteley
****************************************************************************/

#ifndef URCDEFS_H
#define URCDEFS_H

#include "device.h"

//
//  structures used to describe uniform RC lines
//


#define FABS            fabs
#define REFTEMP         wrsREFTEMP       
#define CHARGE          wrsCHARGE        
#define CONSTCtoK       wrsCONSTCtoK     
#define CONSTboltz      wrsCONSTboltz    
#define CONSTvt0        wrsCONSTvt0      
#define CONSTKoverQ     wrsCONSTKoverQ   
#define CONSTroot2      wrsCONSTroot2    
#define CONSTe          wrsCONSTe        

namespace URC {

struct URCdev : public IFdevice
{
    URCdev();
    sGENmodel *newModl();
    sGENinstance *newInst();
    int destroy(sGENmodel**);
    int delInst(sGENmodel*, IFuid, sGENinstance*);
    int delModl(sGENmodel**, IFuid, sGENmodel*);

    void parse(int, sCKT*, sLine*);
    int loadTest(sGENinstance*, sCKT*)  { return (~OK); }
    int load(sGENinstance*, sCKT*)      { return (LOAD_SKIP_FLAG); }
    int setup(sGENmodel*, sCKT*, int*);  
    int unsetup(sGENmodel*, sCKT*);
    int resetup(sGENmodel*, sCKT*);
//    int temperature(sGENmodel*, sCKT*);    
//    int getic(sGENmodel*, sCKT*);  
//    int accept(sCKT*, sGENmodel*); 
//    int trunc(sGENmodel*, sCKT*, double*);  
//    int convTest(sGENmodel*, sCKT*);  

    int setInst(int, IFdata*, sGENinstance*);  
    int setModl(int, IFdata*, sGENmodel*);   
    int askInst(const sCKT*, const sGENinstance*, int, IFdata*);    
    int askModl(const sGENmodel*, int, IFdata*); 
//    int findBranch(sCKT*, sGENmodel*, IFuid); 

//    int acLoad(sGENmodel*, sCKT*); 
//    int pzLoad(sGENmodel*, sCKT*, IFcomplex*); 
//    int disto(int, sGENmodel*, sCKT*);  
//    int noise(int, int, sGENmodel*, sCKT*, sNdata*, double*);
};

struct sURCinstance : sGENinstance
{
    sURCinstance()
        {
            memset(this, 0, sizeof(sURCinstance));
            GENnumNodes = 3;
        }
    sURCinstance *next()
        { return (static_cast<sURCinstance*>(GENnextInstance)); }

    int URCposNode;   // number of positive node of URC
    int URCnegNode;   // number of negative node of URC
    int URCgndNode;   // number of the "ground" node of the URC

    double URClength; // length of line
    int URClumps;     // number of lumps in line
    unsigned URClenGiven : 1;   // flag to indicate length was specified
    unsigned URClumpsGiven : 1; // flag to indicate lumps was specified
    unsigned URCsetupDone : 1;  // set when setup called
};

struct sURCmodel : sGENmodel
{
    sURCmodel()         { memset(this, 0, sizeof(sURCmodel)); }
    sURCmodel *next()   { return (static_cast<sURCmodel*>(GENnextModel)); }
    sURCinstance *inst() { return (static_cast<sURCinstance*>(GENinstances)); }

    double URCk;        // propagation constant for URC
    double URCfmax;     // max frequence of interest
    double URCrPerL;    // resistance per unit length
    double URCcPerL;    // capacitance per unit length
    double URCisPerL;   // diode saturation current per unit length
    double URCrsPerL;   // diode resistance per unit length
    unsigned URCkGiven : 1;      // flag to indicate k was specified
    unsigned URCfmaxGiven : 1;   // flag to indicate fmax was specified
    unsigned URCrPerLGiven : 1;  // flag to indicate rPerL was specified
    unsigned URCcPerLGiven : 1;  // flag to indicate cPerL was specified
    unsigned URCisPerLGiven : 1; // flag to indicate isPerL was specified
    unsigned URCrsPerLGiven : 1; // flag to indicate rsPerL was specified
};

} // namespace URC
using namespace URC;

// device parameters
enum {
    URC_LEN = 1,
    URC_LUMPS,
    URC_POS_NODE,
    URC_NEG_NODE,
    URC_GND_NODE
};

// model parameters
enum {
    URC_MOD_K = 1000,
    URC_MOD_FMAX,
    URC_MOD_RPERL,
    URC_MOD_CPERL,
    URC_MOD_ISPERL,
    URC_MOD_RSPERL,
    URC_MOD_URC
};

#endif // URCDEFS_H

