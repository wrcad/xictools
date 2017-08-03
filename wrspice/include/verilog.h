
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

#ifndef VERILOG_H
#define VERILOG_H

#include "inpline.h"


struct vl_desc;
struct vl_simulator;
struct sOUTdata;
struct VerilogBlock;

// .adc lines have format ".adc output nodename [offset] [quantum]".
struct sADC
{
    sADC(const char*);
    ~sADC() { delete [] dig_var; delete [] node; }

    static void destroy(sADC *a)
        {
            while (a) {
                sADC *ax = a;
                a = a->next;
                delete ax;
            }
        }

    void set_var(VerilogBlock*, double*);

    const char *dig_var;  // Verilog variable name
    const char *range;    // Verilog variable range (alloc'ed with dig_var)
    const char *node;     // Spice node name
    double offset;        // conversion offset
    double quantum;       // conversion lsb size
    int indx;             // node index into RHS vector
    sADC *next;
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

    sADC *adc;
    vl_desc *desc;
    vl_simulator *sim;
};

#endif // VERILOG_H

