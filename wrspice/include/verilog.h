
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
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: verilog.h,v 2.40 2010/03/19 02:33:20 stevew Exp $
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

