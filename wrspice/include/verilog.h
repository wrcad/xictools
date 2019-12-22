
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

#ifndef VERILOG_H
#define VERILOG_H

#include "inpline.h"


struct vl_desc;
struct vl_simulator;
struct sOUTdata;
struct VerilogBlock;

// .adc lines have format ".adc output nodename [offset] [quantum]".
//
struct sADC
{
    sADC(const char*);
    ~sADC()
        {
            delete [] a_dig_var;
            delete [] a_node;
        }

    static void destroy(sADC *a)
        {
            while (a) {
                sADC *ax = a;
                a = a->a_next;
                delete ax;
            }
        }

    sADC *next()                { return (a_next); }
    void set_next(sADC *n)      { a_next = n; }

    const char *dig_var()       { return (a_dig_var); }
    const char *range()         { return (a_range); }
    const char *node()          { return (a_node); }
    double offset()             { return (a_offset); }
    double quantum()            { return (a_quantum); }
    int indx()                  { return (a_indx); }

    void set_indx(int i)        { a_indx = i; }

    void set_var(VerilogBlock*, double*);

private:
    sADC *a_next;
    const char *a_dig_var;  // Verilog variable name
    const char *a_range;    // Verilog variable range (alloc'ed with dig_var)
    const char *a_node;     // Spice node name
    double a_offset;        // conversion offset
    double a_quantum;       // conversion lsb size
    int a_indx;             // node index into RHS vector
};

struct VerilogBlock
{
    VerilogBlock(sLine*);
    ~VerilogBlock();

    void initialize();
    void finalize(bool);
    void run_step(sOUTdata*);
    bool query_var(const char*, const char*, double*);
    bool set_var(sADC*, double);

private:
    sADC *vb_adc;
    vl_desc *vb_desc;
    vl_simulator *vb_sim;
};

#endif // VERILOG_H

