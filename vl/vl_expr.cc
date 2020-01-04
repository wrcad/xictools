
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
 * vl -- Verilog Simulator and Verilog support library.                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "vl_list.h"
#include "vl_st.h"
#include "vl_defs.h"
#include "vl_types.h"


// Set this to machine bit width.
#define DefBits (8*(int)sizeof(int))

namespace {
    inline int max(int x, int y) { return (x > y ? x : y); }
    inline int min(int x, int y) { return (x < y ? x : y); }

    // Return status of i'th bit of integer.
    //
    inline int bit(int i, int pos)
    {
        if ((i >> pos) & 1)  
            return (BitH);
        return (BitL);
    }


    // Return status of i'th bit of vl_time_t.
    //
    inline int bit(vl_time_t t, int pos)
    {
        if ((t >> pos) & 1)  
            return (BitH);
        return (BitL);
    }


    // AND logical operation.
    //
    inline int op_and(int a, int b)
    {
        if (a == BitH && b == BitH)
            return (BitH);
        if (a == BitL || b == BitL)
            return (BitL);
        return (BitDC);
    }


    // OR logical operation.
    //
    inline int op_or(int a, int b)
    {
        if (a == BitH || b == BitH)
            return (BitH);
        if (a == BitL && b == BitL)
            return (BitL);
        return (BitDC);
    }


    // XOR logical operation.
    //
    inline int op_xor(int a, int b)
    {
        if ((a == BitH && b == BitL) || (a == BitL && b == BitH))
            return (BitH);
        if ((a == BitH && b == BitH) || (a == BitL && b == BitL))
            return (BitL);
        return (BitDC);
    }


    void add_bits(char *s0, char *s1, char *s2, int w0, int w1, int w2,
        bool setc = false)
    {
        int carry = setc ? 1 : 0;
        for (int i = 0; i < w0; i++) {
            int a = 0;
            if (i >= w1) {
                if (carry) {
                    a = 1;
                    carry = 0;
                }
            }
            else
                a = s1[i];

            int b = 0;
            if (i >= w2) {
                if (carry) {
                    b = 1;
                    carry = 0;
                }
            }
            else
                b = s2[i];

            if (a == BitDC || a == BitZ || b == BitDC || b == BitZ) {
                memset(&s0[i], BitDC, w0 - i);
                return;
            }
            int c = a + b + carry;
            s0[i] = (c & 1) ? BitH : BitL;
            carry = (c & 2) ? 1 : 0;
        }
    }

    vl_var &new_var()
    {
        return (VS()->var_factory.new_var());
    }
}


//---------------------------------------------------------------------------
//  Arithmetic and logical operator overloads
//---------------------------------------------------------------------------

// Overload '*'.
//
vl_var &
operator*(vl_var &data1, vl_var &data2)
{
    int w1 = 1;
    if (data1.data_type() == Dbit)
        w1 = data1.bits().size();
    else if (data1.data_type() == Dint)
        w1 = DefBits;
    else if (data1.data_type() == Dtime)
        w1 = 8*sizeof(vl_time_t);

    int w2 = 1;
    if (data2.data_type() == Dbit)
        w2 = data2.bits().size();
    else if (data2.data_type() == Dint)
        w2 = DefBits;
    else if (data2.data_type() == Dtime)
        w2 = 8*sizeof(vl_time_t);

    vl_var &d = new_var();
    if (data1.data_type() == Dstring || data2.data_type() == Dstring)
        d.setx(w1 + w2);
    else if ((data1.data_type() == Dbit && data1.is_x()) ||
            (data2.data_type() == Dbit && data2.is_x()))
        d.setx(w1 + w2);
    else if (data1.data_type() == Dreal || data2.data_type() == Dreal) {
        d.set_data_type(Dreal);
        d.data().r = (double)data1 * (double)data2;
    }
    else if (data1.data_type() == Dtime || data2.data_type() == Dtime) {
        d.set_data_type(Dtime);
        d.data().t = (vl_time_t)data1 * (vl_time_t)data2;
    }
    else {
        d.set_data_type(Dint);
        if (data1.data_type() == Dbit || data2.data_type() == Dbit)
            d.data().i = (int)((unsigned)data1 * (unsigned)data2);
        else
            d.data().i = (int)data1 * (int)data2;
    }
    return (d);
}


// Overload '/'.
//
vl_var &
operator/(vl_var &data1, vl_var &data2)
{
    int w1 = 1;
    if (data1.data_type() == Dbit)
        w1 = data1.bits().size();
    else if (data1.data_type() == Dint)
        w1 = DefBits;
    else if (data1.data_type() == Dtime)
        w1 = 8*sizeof(vl_time_t);

    int w2 = 1;
    if (data2.data_type() == Dbit)
        w2 = data2.bits().size();
    else if (data2.data_type() == Dint)
        w2 = DefBits;
    else if (data2.data_type() == Dtime)
        w2 = 8*sizeof(vl_time_t);

    vl_var &d = new_var();
    if (data1.data_type() == Dstring || data2.data_type() == Dstring)
        d.setx(max(w1, w2));
    else if ((data1.data_type() == Dbit && data1.is_x()) ||
            (data2.data_type() == Dbit && data2.is_x()))
        d.setx(max(w1, w2));
    else if ((int)data2 == 0)
        d.setx(max(w1, w2));
    else if (data1.data_type() == Dreal || data2.data_type() == Dreal) {
        d.set_data_type(Dreal);
        d.data().r = (double)data1 / (double)data2;
    }
    else if (data1.data_type() == Dtime || data2.data_type() == Dtime) {
        d.set_data_type(Dtime);
        d.data().t = (vl_time_t)data1 / (vl_time_t)data2;
    }
    else {
        d.set_data_type(Dint);
        if (data1.data_type() == Dbit || data2.data_type() == Dbit)
            d.data().i = (int)((unsigned)data1 / (unsigned)data2);
        else
            d.data().i = (int)data1 / (int)data2;
    }
    return (d);
}


// Overload '%'.
//
vl_var &
operator%(vl_var &data1, vl_var &data2)
{
    int w1 = 1;
    if (data1.data_type() == Dbit)
        w1 = data1.bits().size();
    else if (data1.data_type() == Dint)
        w1 = DefBits;
    else if (data1.data_type() == Dtime)
        w1 = 8*sizeof(vl_time_t);

    int w2 = 1;
    if (data2.data_type() == Dbit)
        w2 = data2.bits().size();
    else if (data2.data_type() == Dint)
        w2 = DefBits;
    else if (data2.data_type() == Dtime)
        w2 = 8*sizeof(vl_time_t);

    vl_var &d = new_var();
    if (data1.data_type() == Dstring || data2.data_type() == Dstring)
        d.setx(max(w1, w2));
    else if ((data1.data_type() == Dbit && data1.is_x()) ||
            (data2.data_type() == Dbit && data2.is_x()))
        d.setx(max(w1, w2));
    else if ((int)data2 == 0)
        d.setx(max(w1, w2));
    else if (data1.data_type() == Dreal || data2.data_type() == Dreal)
        d.setx(max(w1, w2));
    else if (data1.data_type() == Dtime || data2.data_type() == Dtime) {
        d.set_data_type(Dtime);
        d.data().t = (vl_time_t)data1 % (vl_time_t)data2;
    }
    else {
        d.set_data_type(Dint);
        if (data1.data_type() == Dbit || data2.data_type() == Dbit)
            d.data().i = (int)((unsigned)data1 % (unsigned)data2);
        else
            d.data().i = (int)data1 % (int)data2;
    }
    return (d);
}


// Overload '+' (binary).
//
vl_var &
operator+(vl_var &data1, vl_var &data2)
{
    vl_var &d = new_var();
    if (data1.data_type() == Dstring || data2.data_type() == Dstring)
        return (d);
    if (data1.data_type() == Dint) {
        if (data2.data_type() == Dint) {
            d.set_data_type(Dint);
            d.data().i = data1.data().i + data2.data().i;
        }
        else if (data2.data_type() == Dbit)
            d.addb(data2, data1.data().i);
        else if (data2.data_type() == Dtime) {
            d.set_data_type(Dtime);
            d.data().t = data1.data().i + data2.data().t;
        }
        else if (data2.data_type() == Dreal) {
            d.set_data_type(Dreal);
            d.data().r = data1.data().i + data2.data().r;
        }
    }
    else if (data1.data_type() == Dbit) {
        if (data2.data_type() == Dint)
            d.addb(data1, data2.data().i);
        else if (data2.data_type() == Dbit)
            d.addb(data1, data2);
        else if (data2.data_type() == Dtime)
            d.addb(data1, data2.data().t);
        else if (data2.data_type() == Dreal) {
            if (data1.is_x())
                d.setx(1);
            else {
                d.set_data_type(Dreal);
                d.data().r = (double)data1 + data2.data().r;
            }
        }
    }
    else if (data1.data_type() == Dtime) {
        if (data2.data_type() == Dint) {
            d.set_data_type(Dtime);
            d.data().t = data1.data().t + data2.data().i;
        }
        else if (data2.data_type() == Dbit)
            d.addb(data2, data1.data().t);
        else if (data2.data_type() == Dtime) {
            d.set_data_type(Dtime);
            d.data().t = data1.data().t + data2.data().t;
        }
        else if (data2.data_type() == Dreal) {
            d.set_data_type(Dreal);
            d.data().r = data1.data().t + data2.data().r;
        }
    }
    else if (data1.data_type() == Dreal) {
        if (data2.data_type() == Dint) {
            d.set_data_type(Dreal);
            d.data().r = data1.data().r + data2.data().i;
        }
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                d.setx(1);
            else {
                d.set_data_type(Dreal);
                d.data().r = data1.data().r + (double)data2;
            }
        }
        else if (data2.data_type() == Dtime) {
            d.set_data_type(Dreal);
            d.data().r = data1.data().r + data2.data().t;
        }
        else if (data2.data_type() == Dreal) {
            d.set_data_type(Dreal);
            d.data().r = data1.data().r + data2.data().r;
        }
    }
    return (d);
}


// Overload '-' (binary).
//
vl_var &
operator-(vl_var &data1, vl_var &data2)
{
    vl_var &d = new_var();
    if (data1.data_type() == Dstring || data2.data_type() == Dstring)
        return (d);
    if (data1.data_type() == Dint) {
        if (data2.data_type() == Dint) {
            d.set_data_type(Dint);
            d.data().i = data1.data().i - data2.data().i;
        }
        else if (data2.data_type() == Dbit)
            d.subb(data1.data().i, data2);
        else if (data2.data_type() == Dtime) {
            d.set_data_type(Dtime);
            d.data().t = data1.data().i - data2.data().t;
        }
        else if (data2.data_type() == Dreal) {
            d.set_data_type(Dreal);
            d.data().r = data1.data().i - data2.data().r;
        }
    }
    else if (data1.data_type() == Dbit) {
        if (data2.data_type() == Dint)
            d.subb(data1, data2.data().i);
        else if (data2.data_type() == Dbit)
            d.subb(data1, data2);
        else if (data2.data_type() == Dtime)
            d.subb(data1, data2.data().t);
        else if (data2.data_type() == Dreal) {
            if (data1.is_x())
                d.setx(1);
            else {
                d.set_data_type(Dreal);
                d.data().r = (double)data1 - data2.data().r;
            }
        }
    }
    else if (data1.data_type() == Dtime) {
        if (data2.data_type() == Dint) {
            d.set_data_type(Dtime);
            d.data().t = data1.data().t - data2.data().i;
        }
        else if (data2.data_type() == Dbit)
            d.subb(data1.data().t, data2);
        else if (data2.data_type() == Dtime) {
            d.set_data_type(Dtime);
            d.data().t = data1.data().t - data2.data().t;
        }
        else if (data2.data_type() == Dreal) {
            d.set_data_type(Dreal);
            d.data().r = data1.data().t - data2.data().r;
        }
    }
    else if (data1.data_type() == Dreal) {
        if (data2.data_type() == Dint) {
            d.set_data_type(Dreal);
            d.data().r = data1.data().r - data2.data().i;
        }
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                d.setx(1);
            else {
                d.set_data_type(Dreal);
                d.data().r = data1.data().r - (double)data2;
            }
        }
        else if (data2.data_type() == Dtime) {
            d.set_data_type(Dreal);
            d.data().r = data1.data().r - data2.data().t;
        }
        else if (data2.data_type() == Dreal) {
            d.set_data_type(Dreal);
            d.data().r = data1.data().r - data2.data().r;
        }
    }
    return (d);
}


// Overload '-' (unary).
//
vl_var &
operator-(vl_var &data1)
{
    vl_var &d = new_var();
    if (data1.data_type() == Dint) {
        d.set_data_type(Dint);
        d.data().i = -data1.data().i;
    }
    if (data1.data_type() == Dtime) {
        d.set_data_type(Dtime);
        d.data().t = -data1.data().t;
    }
    else if (data1.data_type() == Dbit) {
        d.bits().set(DefBits);
        d.data().s = new char[DefBits];
        d.subb((int)0, data1);
    }
    else if (data1.data_type() == Dreal) {
        d.set_data_type(Dreal);
        d.data().r = -data1.data().r;
    }
    return (d);
}


//
// Shift operators.
//

// Overload '<<' for vl_var.
//
vl_var &
operator<<(vl_var &data1, vl_var &data2)
{
    vl_var &d = new_var();
    if (data2.data_type() == Dbit && data2.is_x())
        d.setx(data1.bits().size());
    else {
        int shift = (int)data2;
        if (shift < 0)
            shift = -shift;
        int bw;
        char *s = data1.bit_elt(0, &bw);
        d.set_data_type(Dbit);
        d.bits().set(bw + shift);
        d.data().s = new char[d.bits().size()];
        for (int i = 0; i < d.bits().size(); i++) {
            if (i >= shift)
                d.data().s[i] = s[i-shift];
            else
                d.data().s[i] = BitL;
        }
    }
    return (d);
}


// Overload '<<' for int.
//
vl_var &
operator<<(vl_var &data1, int shift)
{
    vl_var &d = new_var();
    if (shift < 0)
        shift = -shift;
    int bw;
    char *s = data1.bit_elt(0, &bw);
    d.set_data_type(Dbit);
    d.bits().set(bw);
    d.data().s = new char[d.bits().size()];
    for (int i = 0; i < d.bits().size(); i++) {
        if (i >= shift)
            d.data().s[i] = s[i-shift];
        else
            d.data().s[i] = BitL;
    }
    return (d);
}


// Overload '>>' for vl_var.
//
vl_var &
operator>>(vl_var &data1, vl_var &data2)
{
    vl_var &d = new_var();
    if (data2.data_type() == Dbit && data2.is_x())
        d.setx(data1.bits().size());
    else {
        int shift = (int)data2;
        if (shift < 0)
            shift = -shift;
        int bw;
        char *s = data1.bit_elt(0, &bw);
        d.set_data_type(Dbit);
        d.bits().set(bw);
        d.data().s = new char[d.bits().size()];
        for (int i = 0; i < d.bits().size(); i++) {
            if (i + shift < bw)
                d.data().s[i] = s[i+shift];
            else
                d.data().s[i] = BitL;
        }
    }
    return (d);
}


// Overload '>>' for int.
//
vl_var &
operator>>(vl_var &data1, int shift)
{
    vl_var &d = new_var();
    if (shift < 0)
        shift = -shift;
    int bw;
    char *s = data1.bit_elt(0, &bw);
    d.set_data_type(Dbit);
    d.bits().set(bw - shift);
    d.data().s = new char[d.bits().size()];
    for (int i = 0; i < d.bits().size(); i++) {
        if (i + shift < bw)
            d.data().s[i] = s[i+shift];
        else
            d.data().s[i] = BitL;
    }
    return (d);
}


//
// Equality operators.
//

// In general, the overloaded functions are used only in vl_expr::eval(),
// so that the arguments are never Dconcat.  However, the equality
// operators may be used elsewhere for general equality testing, so that
// Dconcat handling is necessary.

// Overload '=='.
//
vl_var &
operator==(vl_var &data1, vl_var &data2)
{
    if (data1.data_type() == Dconcat) {
        vl_expr ex(&data1);
        vl_var &d = (ex.eval() == data2);
        ex.edata().mcat.var = 0;
        return (d);
    }
    if (data2.data_type() == Dconcat) {
        vl_expr ex(&data2);
        vl_var &d = (data2 == ex.eval());
        ex.edata().mcat.var = 0;
        return (d);
    }

    vl_var &d = new_var();
    d.setx(1);
    if (data1.data_type() == Dint) {
        if (data2.data_type() == Dint)
            d.data().s[0] = (data1.data().i == data2.data().i ? BitH : BitL);
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                return (d);
            d.data().s[0] = BitL;
            int i, i1 = data1.data().i;
            for (i = 0; i < data2.bits().size(); i++)
                if (bit(i1, i) != data2.data().s[i])
                    return (d);
            if (data2.bits().size() != DefBits) {
                if (data2.bits().size() > DefBits) {
                    for (i = DefBits; i < data2.bits().size(); i++)
                        if (data2.data().s[i] != BitL)
                            return (d);
                }
                else {
                    for (i = data2.bits().size(); i < DefBits; i++)
                        if (bit(i1, i) != BitL)
                            return (d);
                }
            }
            d.data().s[0] = BitH;
            return (d);
        }
        else if (data2.data_type() == Dtime) {
            d.data().s[0] =
                ((unsigned)data1.data().i == data2.data().t ? BitH : BitL);
        }
        else if (data2.data_type() == Dreal) {
            d.data().s[0] =
                ((unsigned)data1.data().i == data2.data().r ? BitH : BitL);
        }
    }
    else if (data1.data_type() == Dbit) {
        if (data1.is_x())
            return (d);
        int i2;
        if (data2.data_type() == Dint)
            i2 = data2.data().i;
        else if (data2.data_type() == Dbit) {

            if (data2.is_x())
                return (d);
            d.data().s[0] = BitL;
            int wd = min(data1.bits().size(), data2.bits().size());
            int i;
            for (i = 0; i < wd; i++)
                if (data1.data().s[i] != data2.data().s[i])
                    return (d);
            if (data1.bits().size() != data2.bits().size()) {
                if (data1.bits().size() > data2.bits().size()) {
                    for (i = data2.bits().size(); i < data1.bits().size(); i++)
                        if (data1.data().s[i] != BitL)
                            return (d);
                }
                else {
                    for (i = data1.bits().size(); i < data2.bits().size(); i++)
                        if (data2.data().s[i] != BitL)
                            return (d);
                }
            }
            d.data().s[0] = BitH;
            return (d);
        }
        else if (data2.data_type() == Dtime)
            i2 = (int)data2.data().t;
        else if (data2.data_type() == Dreal)
            i2 = (int)data2.data().r;
        else
            return (d);
        d.data().s[0] = BitL;
        int i;
        for (i = 0; i < data1.bits().size(); i++)
            if (data1.data().s[i] != bit(i2, i))
                return (d);
        if (data1.bits().size() != DefBits) {
            if (data1.bits().size() > DefBits) {
                for (i = DefBits; i < data1.bits().size(); i++)
                    if (data1.data().s[i] != BitL)
                        return (d);
            }
            else {
                for (i = data1.bits().size(); i < DefBits; i++)
                    if (bit(i2, i) != BitL)
                        return (d);
            }
        }
        d.data().s[0] = BitH;
        return (d);
    }
    else if (data1.data_type() == Dtime) {
        if (data2.data_type() == Dint) {
            d.data().s[0] =
                (data1.data().t == (unsigned)data2.data().i ? BitH : BitL);
        }
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                return (d);
            d.data().s[0] = BitL;
            int i, i1 = (int)data1.data().t;
            for (i = 0; i < data2.bits().size(); i++)
                if (bit(i1, i) != data2.data().s[i])
                    return (d);
            if (data2.bits().size() != DefBits) {
                if (data2.bits().size() > DefBits) {
                    for (i = DefBits; i < data2.bits().size(); i++)
                        if (data2.data().s[i] != BitL)
                            return (d);
                }
                else {
                    for (i = data2.bits().size(); i < DefBits; i++)
                        if (bit(i1, i) != BitL)
                            return (d);
                }
            }
            d.data().s[0] = BitH;
            return (d);
        }
        else if (data2.data_type() == Dtime)
            d.data().s[0] = (data1.data().t == data2.data().t ? BitH : BitL);
        else if (data2.data_type() == Dreal)
            d.data().s[0] = (data1.data().t == data2.data().r ? BitH : BitL);
    }
    else if (data1.data_type() == Dreal) {
        if (data2.data_type() == Dint)
            d.data().s[0] = (data1.data().r == data2.data().i ? BitH : BitL);
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                return (d);
            d.data().s[0] = BitL;
            int i, i1 = (int)data1.data().r;
            for (i = 0; i < data2.bits().size(); i++)
                if (bit(i1, i) != data2.data().s[i])
                    return (d);
            if (data2.bits().size() != DefBits) {
                if (data2.bits().size() > DefBits) {
                    for (i = DefBits; i < data2.bits().size(); i++)
                        if (data2.data().s[i] != BitL)
                            return (d);
                }
                else {
                    for (i = data2.bits().size(); i < DefBits; i++)
                        if (bit(i1, i) != BitL)
                            return (d);
                }
            }
            d.data().s[0] = BitH;
            return (d);
        }
        else if (data2.data_type() == Dtime)
            d.data().s[0] = (data1.data().r == data2.data().t ? BitH : BitL);
        else if (data2.data_type() == Dreal)
            d.data().s[0] = (data1.data().r == data2.data().r ? BitH : BitL);
    }
    return (d);
}


// Overload '!='.
//
vl_var &
operator!=(vl_var &data1, vl_var &data2)
{
    vl_var &d = (data1 == data2);
    if (d.data().s[0] == BitL)
        d.data().s[0] = BitH;
    else if (d.data().s[0] == BitH)
        d.data().s[0] = BitL;
    return (d);
}


// Case equal operation.
//
vl_var &
case_eq(vl_var &data1, vl_var &data2)
{
    if (data1.data_type() == Dconcat) {
        vl_expr ex(&data1);
        vl_var &d = case_eq(ex.eval(), data2);
        ex.edata().mcat.var = 0;
        return (d);
    }
    if (data2.data_type() == Dconcat) {
        vl_expr ex(&data2);
        vl_var &d = case_eq(data2, ex.eval());
        ex.edata().mcat.var = 0;
        return (d);
    }

    if (data1.data_type() == Dbit && data2.data_type() == Dbit) {
        vl_var &d = new_var();
        d.setx(1);
        d.data().s[0] = BitL;
        int wd = min(data1.bits().size(), data2.bits().size());
        int i;
        for (i = 0; i < wd; i++)
            if (data1.data().s[i] != data2.data().s[i])
                return (d);
        if (data1.bits().size() != data2.bits().size()) {
            if (data1.bits().size() > data2.bits().size()) {
                for (i = data2.bits().size(); i < data1.bits().size(); i++)
                    if (data1.data().s[i] != BitL)
                        return (d);
            }
            else {
                for (i = data1.bits().size(); i < data2.bits().size(); i++)
                    if (data2.data().s[i] != BitL)
                        return (d);
            }
        }
        d.data().s[0] = BitH;
        return (d);
    }
    else {
        vl_var &d = operator==(data1, data2);
        if (d.data().s[0] == BitDC)
            d.data().s[0] = BitL;
        return (d);
    }
}


// Casex equal operation.
//
vl_var &
casex_eq(vl_var &data1, vl_var &data2)
{
    if (data1.data_type() == Dconcat) {
        vl_expr ex(&data1);
        vl_var &d = casex_eq(ex.eval(), data2);
        ex.edata().mcat.var = 0;
        return (d);
    }
    if (data2.data_type() == Dconcat) {
        vl_expr ex(&data2);
        vl_var &d = casex_eq(data2, ex.eval());
        ex.edata().mcat.var = 0;
        return (d);
    }

    if (data1.data_type() == Dbit && data2.data_type() == Dbit) {
        vl_var &d = new_var();
        d.setx(1);
        d.data().s[0] = BitL;
        int wd = min(data1.bits().size(), data2.bits().size());
        int i;
        for (i = 0; i < wd; i++) {
            if (data1.data().s[i] == BitDC || data2.data().s[i] == BitDC)
                continue;
            if (data1.data().s[i] == BitZ || data2.data().s[i] == BitZ)
                continue;
            if (data1.data().s[i] != data2.data().s[i])
                return (d);
        }
        if (data1.bits().size() != data2.bits().size()) {
            if (data1.bits().size() > data2.bits().size()) {
                for (i = data2.bits().size(); i < data1.bits().size(); i++)
                    if (data1.data().s[i] != BitL)
                        return (d);
            }
            else {
                for (i = data1.bits().size(); i < data2.bits().size(); i++)
                    if (data2.data().s[i] != BitL)
                        return (d);
            }
        }
        d.data().s[0] = BitH;
        return (d);
    }
    else {
        vl_var &d = operator==(data1, data2);
        if (d.data().s[0] == BitDC)
            d.data().s[0] = BitL;
        return (d);
    }
}


// Casez equal operation.
//
vl_var &
casez_eq(vl_var &data1, vl_var &data2)
{
    if (data1.data_type() == Dconcat) {
        vl_expr ex(&data1);
        vl_var &d = casez_eq(ex.eval(), data2);
        ex.edata().mcat.var = 0;
        return (d);
    }
    if (data2.data_type() == Dconcat) {
        vl_expr ex(&data2);
        vl_var &d = casez_eq(data2, ex.eval());
        ex.edata().mcat.var = 0;
        return (d);
    }

    if (data1.data_type() == Dbit && data2.data_type() == Dbit) {
        vl_var &d = new_var();
        d.setx(1);
        d.data().s[0] = BitL;
        int wd = min(data1.bits().size(), data2.bits().size());
        int i;
        for (i = 0; i < wd; i++) {
            if (data1.data().s[i] == BitZ || data2.data().s[i] == BitZ)
                continue;
            if (data1.data().s[i] != data2.data().s[i])
                return (d);
        }
        if (data1.bits().size() != data2.bits().size()) {
            if (data1.bits().size() > data2.bits().size()) {
                for (i = data2.bits().size(); i < data1.bits().size(); i++)
                    if (data1.data().s[i] != BitL)
                        return (d);
            }
            else {
                for (i = data1.bits().size(); i < data2.bits().size(); i++)
                    if (data2.data().s[i] != BitL)
                        return (d);
            }
        }
        d.data().s[0] = BitH;
        return (d);
    }
    else {
        vl_var &d = operator==(data1, data2);
        if (d.data().s[0] == BitDC)
            d.data().s[0] = BitL;
        return (d);
    }
}


// Case not equal operation.
//
vl_var &
case_neq(vl_var &data1, vl_var &data2)
{
    vl_var &d = case_eq(data1, data2);
    if (d.data().s[0] == BitL)
        d.data().s[0] = BitH;
    else
        d.data().s[0] = BitL;
    return (d);
}


//
// Logical operators.
//

// Overload '&&'.
//
vl_var &
operator&&(vl_var &data1, vl_var &data2)
{
    vl_var &d = new_var();
    d.setx(1);
    if (data1.data_type() == Dint) {
        if (data2.data_type() == Dint)
            d.data().s[0] = (data1.data().i && data2.data().i ? BitH : BitL);
        else if (data2.data_type() == Dbit) {
            int x2 = data2.bitset();
            int i1 = data1.data().i;
            if (i1 && (x2 & Hmask))
                d.data().s[0] = BitH;
            else if (!i1 || (x2 & Lmask))
                d.data().s[0] = BitL;
        }
        else if (data2.data_type() == Dtime)
            d.data().s[0] = (data1.data().i && data2.data().t ? BitH : BitL);
        else if (data2.data_type() == Dreal) {
            d.data().s[0] =
                (data1.data().i && data2.data().r != 0.0 ? BitH : BitL);
        }
    }
    else if (data1.data_type() == Dbit) {
        int x1 = data1.bitset(); 
        int i2;
        if (data2.data_type() == Dint)
            i2 = data2.data().i;
        else if (data2.data_type() == Dbit) {
            int x2 = data2.bitset();
            if ((x1 & Hmask) && (x2 & Hmask))
                d.data().s[0] = BitH;
            else if ((x1 & Lmask) || (x2 & Lmask))
                d.data().s[0] = BitL;
            return (d);
        }
        else if (data2.data_type() == Dtime)
            i2 = data2.data().t != 0 ? 1 : 0;
        else if (data2.data_type() == Dreal)
            i2 = data2.data().r != 0.0 ? 1 : 0;
        else
            return (d);
        if ((x1 & Hmask) && i2)
            d.data().s[0] = BitH;
        else if ((x1 & Lmask) || !i2)
            d.data().s[0] = BitL;
    }
    else if (data1.data_type() == Dtime) {
        if (data2.data_type() == Dint)
            d.data().s[0] = (data1.data().t && data2.data().i ? BitH : BitL);
        else if (data2.data_type() == Dbit) {
            int x2 = data2.bitset();
            int i1 = data1.data().t != 0 ? 1 : 0;
            if (i1 && (x2 & Hmask))
                d.data().s[0] = BitH;
            else if (!i1 || (x2 & Lmask))
                d.data().s[0] = BitL;
        }
        else if (data2.data_type() == Dtime)
            d.data().s[0] = (data1.data().t && data2.data().t ? BitH : BitL);
        else if (data2.data_type() == Dreal) {
            d.data().s[0] =
                (data1.data().t && data2.data().r != 0.0 ? BitH : BitL);
        }
    }
    else if (data1.data_type() == Dreal) {
        if (data2.data_type() == Dint) {
            d.data().s[0] =
                (data1.data().r != 0.0 && data2.data().i ? BitH : BitL);
        }
        else if (data2.data_type() == Dbit) {
            int x2 = data2.bitset();
            int i1 = data1.data().r != 0.0 ? 1 : 0;
            if (i1 && (x2 & Hmask))
                d.data().s[0] = BitH;
            else if (!i1 || (x2 & Lmask))
                d.data().s[0] = BitL;
        }
        else if (data2.data_type() == Dtime) {
            d.data().s[0] =
                (data1.data().r != 0.0 && data2.data().t ? BitH : BitL);
        }
        else if (data2.data_type() == Dreal) {
            d.data().s[0] =
                (data1.data().r != 0.0 && data2.data().r != 0.0 ? BitH : BitL);
        }
    }
    return (d);
}


// Overload '||'.
//
vl_var &
operator||(vl_var &data1, vl_var &data2)
{
    vl_var &d = new_var();
    d.setx(1);
    if (data1.data_type() == Dint) {
        if (data2.data_type() == Dint)
            d.data().s[0] = (data1.data().i || data2.data().i ? BitH : BitL);
        else if (data2.data_type() == Dbit) {
            int x2 = data2.bitset();
            int i1 = data1.data().i;
            if (i1 || (x2 & Hmask))
                d.data().s[0] = BitH;
            else if (!i1 && (x2 & Lmask))
                d.data().s[0] = BitL;
        }
        else if (data2.data_type() == Dtime)
            d.data().s[0] = (data1.data().i || data2.data().t ? BitH : BitL);
        else if (data2.data_type() == Dreal) {
            d.data().s[0] =
                (data1.data().i || data2.data().r != 0.0 ? BitH : BitL);
        }
    }
    else if (data1.data_type() == Dbit) {
        int x1 = data1.bitset(); 
        int i2;
        if (data2.data_type() == Dint)
            i2 = data2.data().i;
        else if (data2.data_type() == Dbit) {
            int x2 = data2.bitset();
            if ((x1 & Hmask) || (x2 & Hmask))
                d.data().s[0] = BitH;
            else if ((x1 & Lmask) && (x2 & Lmask))
                d.data().s[0] = BitL;
            return (d);
        }
        else if (data2.data_type() == Dtime)
            i2 = data2.data().t != 0 ? 1 : 0;
        else if (data2.data_type() == Dreal)
            i2 = data2.data().r != 0.0 ? 1 : 0;
        else
            return (d);
        if ((x1 & Hmask) || i2)
            d.data().s[0] = BitH;
        else if ((x1 & Lmask) && !i2)
            d.data().s[0] = BitL;
    }
    else if (data1.data_type() == Dtime) {
        if (data2.data_type() == Dint)
            d.data().s[0] = (data1.data().t || data2.data().i ? BitH : BitL);
        else if (data2.data_type() == Dbit) {
            int x2 = data2.bitset();
            int i1 = data1.data().t != 0 ? 1 : 0;
            if (i1 || (x2 & Hmask))
                d.data().s[0] = BitH;
            else if (!i1 && (x2 & Lmask))
                d.data().s[0] = BitL;
        }
        else if (data2.data_type() == Dtime)
            d.data().s[0] = (data1.data().t || data2.data().t ? BitH : BitL);
        else if (data2.data_type() == Dreal) {
            d.data().s[0] =
                (data1.data().t || data2.data().r != 0.0 ? BitH : BitL);
        }
    }
    else if (data1.data_type() == Dreal) {
        if (data2.data_type() == Dint) {
            d.data().s[0] =
                (data1.data().r != 0.0 || data2.data().i ? BitH : BitL);
        }
        else if (data2.data_type() == Dbit) {
            int x2 = data2.bitset();
            int i1 = data1.data().r != 0.0 ? 1 : 0;
            if (i1 || (x2 & Hmask))
                d.data().s[0] = BitH;
            else if (!i1 && (x2 & Lmask))
                d.data().s[0] = BitL;
        }
        else if (data2.data_type() == Dtime) {
            d.data().s[0] =
                (data1.data().r != 0.0 || data2.data().t ? BitH : BitL);
        }
        else if (data2.data_type() == Dreal) {
            d.data().s[0] =
                (data1.data().r != 0.0 || data2.data().r != 0.0 ? BitH : BitL);
        }
    }
    return (d);
}


// Overload '!'.
//
vl_var &
operator!(vl_var &data1)
{
    vl_var &d = new_var();
    d.setx(1);
    if (data1.data_type() == Dint)
        d.data().s[0] = data1.data().i ? BitL : BitH;
    else if (data1.data_type() == Dbit) {
        int x1 = data1.bitset();
        if (x1 & Hmask)
            d.data().s[0] = BitL;
        else if (x1 & Lmask)
            d.data().s[0] = BitH;
    }
    else if (data1.data_type() == Dtime)
        d.data().s[0] = data1.data().t ? BitL : BitH;
    else if (data1.data_type() == Dreal)
        d.data().s[0] = data1.data().r != 0.0 ? BitL : BitH;
    return (d);
}


// Reduction operation.
//
vl_var &
reduce(vl_var &data1, int oper)
{
    int bw;
    char *s = data1.bit_elt(0, &bw);
    int xx = s[0];
    for (int i = 1; i < bw; i++) {
        switch (oper) {
        case UnandExpr:
        case UandExpr:
            xx = op_and(xx, s[i]);
            break;
        case UnorExpr:
        case UorExpr:
            xx = op_or(xx, s[i]);
            break;
        case UxnorExpr:
        case UxorExpr:
            xx = op_xor(xx, s[i]);
            break;
        default:
            xx = BitL;
            vl_error("(internal) in reduce, bad reduction op");
            VS()->abort();
        }
    }
    if (oper == UnandExpr || oper == UnorExpr || oper == UxnorExpr) {
        if (xx == BitL)
            xx = BitH;
        else if (xx == BitH)
            xx = BitL;
    }
    vl_var &d = new_var();
    d.setx(1);
    d.data().s[0] = xx;
    return (d);
}


//
// Relational operators.
//

// Overload '<'.
//
vl_var &
operator<(vl_var &data1, vl_var &data2)
{
    vl_var &d = new_var();
    d.setx(1);
    if (data1.data_type() == Dint) {
        if (data2.data_type() == Dint)
            d.data().s[0] = (data1.data().i < data2.data().i ? BitH : BitL);
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                return (d);
            d.data().s[0] =
                ((unsigned)data1.data().i < (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type() == Dtime) {
            d.data().s[0] =
                ((unsigned)data1.data().i < data2.data().t ? BitH : BitL);
        }
        else if (data2.data_type() == Dreal)
            d.data().s[0] = (data1.data().i < data2.data().r ? BitH : BitL);
    }
    else if (data1.data_type() == Dbit) {
        if (data1.is_x())
            return (d);
        if (data2.data_type() == Dint) {
            d.data().s[0] =
                ((unsigned)data1 < (unsigned)data2.data().i ? BitH : BitL);
        }
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                return (d);
            d.data().s[0] = ((unsigned)data1 < (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type() == Dtime)
            d.data().s[0] = ((unsigned)data1 < data2.data().t ? BitH : BitL);
        if (data2.data_type() == Dreal)
            d.data().s[0] = ((unsigned)data1 < data2.data().r ? BitH : BitL);
    }
    else if (data1.data_type() == Dtime) {
        if (data2.data_type() == Dint) {
            d.data().s[0] =
                (data1.data().t < (unsigned)data2.data().i ? BitH : BitL);
        }
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                return (d);
            d.data().s[0] = (data1.data().t < (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type() == Dtime)
            d.data().s[0] = (data1.data().t < data2.data().t ? BitH : BitL);
        else if (data2.data_type() == Dreal)
            d.data().s[0] = (data1.data().t < data2.data().r ? BitH : BitL);
    }
    else if (data1.data_type() == Dreal) {
        if (data2.data_type() == Dint)
            d.data().s[0] = (data1.data().r < data2.data().i ? BitH : BitL);
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                return (d);
            d.data().s[0] = (data1.data().r < (unsigned)data2 ? BitH : BitL);
            return (d);
        }
        else if (data2.data_type() == Dtime)
            d.data().s[0] = (data1.data().r < data2.data().t ? BitH : BitL);
        else if (data2.data_type() == Dreal)
            d.data().s[0] = (data1.data().r < data2.data().r ? BitH : BitL);
    }
    return (d);
}


// Overload '<='.
//
vl_var &
operator<=(vl_var &data1, vl_var &data2)
{
    vl_var &d = new_var();
    d.setx(1);
    if (data1.data_type() == Dint) {
        if (data2.data_type() == Dint)
            d.data().s[0] = (data1.data().i <= data2.data().i ? BitH : BitL);
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                return (d);
            d.data().s[0] =
                ((unsigned)data1.data().i <= (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type() == Dtime) {
            d.data().s[0] =
                ((unsigned)data1.data().i <= data2.data().t ? BitH : BitL);
        }
        else if (data2.data_type() == Dreal)
            d.data().s[0] = (data1.data().i <= data2.data().r ? BitH : BitL);
    }
    else if (data1.data_type() == Dbit) {
        if (data1.is_x())
            return (d);
        if (data2.data_type() == Dint) {
            d.data().s[0] =
                ((unsigned)data1 <= (unsigned)data2.data().i ? BitH : BitL);
        }
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                return (d);
            d.data().s[0] = ((unsigned)data1 <= (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type() == Dtime)
            d.data().s[0] = ((unsigned)data1 <= data2.data().t ? BitH : BitL);
        if (data2.data_type() == Dreal)
            d.data().s[0] = ((unsigned)data1 <= data2.data().r ? BitH : BitL);
    }
    else if (data1.data_type() == Dtime) {
        if (data2.data_type() == Dint) {
            d.data().s[0] =
                (data1.data().t <= (unsigned)data2.data().i ? BitH : BitL);
        }
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                return (d);
            d.data().s[0] = (data1.data().t <= (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type() == Dtime)
            d.data().s[0] = (data1.data().t <= data2.data().t ? BitH : BitL);
        else if (data2.data_type() == Dreal)
            d.data().s[0] = (data1.data().t <= data2.data().r ? BitH : BitL);
    }
    else if (data1.data_type() == Dreal) {
        if (data2.data_type() == Dint)
            d.data().s[0] = (data1.data().r <= data2.data().i ? BitH : BitL);
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                return (d);
            d.data().s[0] = (data1.data().r <= (unsigned)data2 ? BitH : BitL);
            return (d);
        }
        else if (data2.data_type() == Dtime)
            d.data().s[0] = (data1.data().r <= data2.data().t ? BitH : BitL);
        else if (data2.data_type() == Dreal)
            d.data().s[0] = (data1.data().r <= data2.data().r ? BitH : BitL);
    }
    return (d);
}


// Overload '>'.
//
vl_var &
operator>(vl_var &data1, vl_var &data2)
{
    vl_var &d = new_var();
    d.setx(1);
    if (data1.data_type() == Dint) {
        if (data2.data_type() == Dint)
            d.data().s[0] = (data1.data().i > data2.data().i ? BitH : BitL);
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                return (d);
            d.data().s[0] =
                ((unsigned)data1.data().i > (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type() == Dtime) {
            d.data().s[0] =
                ((unsigned)data1.data().i > data2.data().t ? BitH : BitL);
        }
        else if (data2.data_type() == Dreal)
            d.data().s[0] = (data1.data().i > data2.data().r ? BitH : BitL);
    }
    else if (data1.data_type() == Dbit) {
        if (data1.is_x())
            return (d);
        if (data2.data_type() == Dint) {
            d.data().s[0] =
                ((unsigned)data1 > (unsigned)data2.data().i ? BitH : BitL);
        }
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                return (d);
            d.data().s[0] = ((unsigned)data1 > (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type() == Dtime)
            d.data().s[0] = ((unsigned)data1 > data2.data().t ? BitH : BitL);
        if (data2.data_type() == Dreal)
            d.data().s[0] = ((unsigned)data1 > data2.data().r ? BitH : BitL);
    }
    else if (data1.data_type() == Dtime) {
        if (data2.data_type() == Dint) {
            d.data().s[0] =
                (data1.data().t > (unsigned)data2.data().i ? BitH : BitL);
        }
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                return (d);
            d.data().s[0] = (data1.data().t > (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type() == Dtime)
            d.data().s[0] = (data1.data().t > data2.data().t ? BitH : BitL);
        else if (data2.data_type() == Dreal)
            d.data().s[0] = (data1.data().t > data2.data().r ? BitH : BitL);
    }
    else if (data1.data_type() == Dreal) {
        if (data2.data_type() == Dint)
            d.data().s[0] = (data1.data().r > data2.data().i ? BitH : BitL);
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                return (d);
            d.data().s[0] = (data1.data().r > (unsigned)data2 ? BitH : BitL);
            return (d);
        }
        else if (data2.data_type() == Dtime)
            d.data().s[0] = (data1.data().r > data2.data().t ? BitH : BitL);
        else if (data2.data_type() == Dreal)
            d.data().s[0] = (data1.data().r > data2.data().r ? BitH : BitL);
    }
    return (d);
}


// Overload '>='.
//
vl_var &
operator>=(vl_var &data1, vl_var &data2)
{
    vl_var &d = new_var();
    d.setx(1);
    if (data1.data_type() == Dint) {
        if (data2.data_type() == Dint)
            d.data().s[0] = (data1.data().i >= data2.data().i ? BitH : BitL);
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                return (d);
            d.data().s[0] =
                ((unsigned)data1.data().i >= (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type() == Dtime) {
            d.data().s[0] =
                ((unsigned)data1.data().i >= data2.data().t ? BitH : BitL);
        }
        else if (data2.data_type() == Dreal)
            d.data().s[0] = (data1.data().i >= data2.data().r ? BitH : BitL);
    }
    else if (data1.data_type() == Dbit) {
        if (data1.is_x())
            return (d);
        if (data2.data_type() == Dint) {
            d.data().s[0] =
                ((unsigned)data1 >= (unsigned)data2.data().i ? BitH : BitL);
        }
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                return (d);
            d.data().s[0] = ((unsigned)data1 >= (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type() == Dtime)
            d.data().s[0] = ((unsigned)data1 >= data2.data().t ? BitH : BitL);
        if (data2.data_type() == Dreal)
            d.data().s[0] = ((unsigned)data1 >= data2.data().r ? BitH : BitL);
    }
    else if (data1.data_type() == Dtime) {
        if (data2.data_type() == Dint) {
            d.data().s[0] =
                (data1.data().t >= (unsigned)data2.data().i ? BitH : BitL);
        }
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                return (d);
            d.data().s[0] = (data1.data().t >= (unsigned)data2 ? BitH : BitL);
        }
        else if (data2.data_type() == Dtime)
            d.data().s[0] = (data1.data().t >= data2.data().t ? BitH : BitL);
        else if (data2.data_type() == Dreal)
            d.data().s[0] = (data1.data().t >= data2.data().r ? BitH : BitL);
    }
    else if (data1.data_type() == Dreal) {
        if (data2.data_type() == Dint)
            d.data().s[0] = (data1.data().r >= data2.data().i ? BitH : BitL);
        else if (data2.data_type() == Dbit) {
            if (data2.is_x())
                return (d);
            d.data().s[0] = (data1.data().r >= (unsigned)data2 ? BitH : BitL);
            return (d);
        }
        else if (data2.data_type() == Dtime)
            d.data().s[0] = (data1.data().r >= data2.data().t ? BitH : BitL);
        else if (data2.data_type() == Dreal)
            d.data().s[0] = (data1.data().r >= data2.data().r ? BitH : BitL);
    }
    return (d);
}


//
// Bitwise logical operators.
//

// Overload '&'.
//
vl_var &
operator&(vl_var &data1, vl_var &data2)
{
    vl_var &d = new_var();
    if (data1.data_type() == Dint) {
        if (data2.data_type() == Dint) {
            d.set_data_type(Dint);
            d.data().i = data1.data().i & data2.data().i;
        }
        else if (data2.data_type() == Dbit) {
            d.set_data_type(Dbit);
            d.bits().set(max(data2.bits().size(), DefBits));
            d.data().s = new char[d.bits().size()];
            for (int i = 0; i < d.bits().size(); i++) {
                if (i < data2.bits().size()) {
                    d.data().s[i] =
                        op_and(data2.data().s[i], bit(data1.data().i, i));
                }
                else
                    d.data().s[i] = BitL;
            }
        }
        else if (data2.data_type() == Dtime) {
            d.set_data_type(Dtime);
            d.data().t = data1.data().i & data2.data().t;
        }
        else if (data2.data_type() == Dreal)
            d.setx(1);
    }
    else if (data1.data_type() == Dbit) {
        if (data2.data_type() == Dint) {
            d.set_data_type(Dbit);
            d.bits().set(max(data1.bits().size(), DefBits));
            d.data().s = new char[d.bits().size()];
            for (int i = 0; i < d.bits().size(); i++) {
                if (i < data1.bits().size()) {
                    d.data().s[i] =
                        op_and(data1.data().s[i], bit(data2.data().i, i));
                }
                else
                    d.data().s[i] = BitL;
            }
        }
        else if (data2.data_type() == Dbit) {
            d.set_data_type(Dbit);
            d.bits().set(max(data1.bits().size(), data2.bits().size()));
            d.data().s = new char[d.bits().size()];
            for (int i = 0; i < d.bits().size(); i++) {
                int i1 =
                    (i < data1.bits().size() ? data1.data().s[i] : (int)BitL);
                int i2 =
                    (i < data2.bits().size() ? data2.data().s[i] : (int)BitL);
                d.data().s[i] = op_and(i1, i2);
            }
        }
        else if (data2.data_type() == Dtime) {
            d.set_data_type(Dbit);
            d.bits().set(max(data1.bits().size(), 8*sizeof(vl_time_t)));
            d.data().s = new char[d.bits().size()];
            for (int i = 0; i < d.bits().size(); i++) {
                if (i < data1.bits().size()) {
                    d.data().s[i] =
                        op_and(data1.data().s[i], bit(data2.data().t, i));
                }
                else
                    d.data().s[i] = BitL;
            }
        }
        else if (data2.data_type() == Dreal)
            d.setx(1);
    }
    else if (data1.data_type() == Dtime) {
        if (data2.data_type() == Dint) {
            d.set_data_type(Dtime);
            d.data().t = data1.data().t & data2.data().i;
        }
        else if (data2.data_type() == Dbit) {
            d.set_data_type(Dbit);
            d.bits().set(max(data2.bits().size(), 8*sizeof(vl_time_t)));
            d.data().s = new char[d.bits().size()];
            for (int i = 0; i < d.bits().size(); i++) {
                if (i < data2.bits().size()) {
                    d.data().s[i] =
                        op_and(data2.data().s[i], bit(data1.data().t, i));
                }
                else
                    d.data().s[i] = BitL;
            }
        }
        else if (data2.data_type() == Dtime) {
            d.set_data_type(Dtime);
            d.data().t = data1.data().t & data2.data().t;
        }
        else if (data2.data_type() == Dreal)
            d.setx(1);
    }
    else if (data1.data_type() == Dreal)
        d.setx(1);
    return (d);
}


// Overload '|'.
//
vl_var &
operator|(vl_var &data1, vl_var &data2)
{
    vl_var &d = new_var();
    if (data1.data_type() == Dint) {
        if (data2.data_type() == Dint) {
            d.set_data_type(Dint);
            d.data().i = data1.data().i | data2.data().i;
        }
        else if (data2.data_type() == Dbit) {
            d.set_data_type(Dbit);
            d.bits().set(max(data2.bits().size(), DefBits));
            d.data().s = new char[d.bits().size()];
            for (int i = 0; i < d.bits().size(); i++) {
                if (i < data2.bits().size()) {
                    d.data().s[i] =
                        op_or(data2.data().s[i], bit(data1.data().i, i));
                }
                else
                    d.data().s[i] = bit(data1.data().i, i);
            }
        }
        else if (data2.data_type() == Dtime) {
            d.set_data_type(Dtime);
            d.data().t = data1.data().i | data2.data().t;
        }
        else if (data2.data_type() == Dreal)
            d.setx(1);
    }
    else if (data1.data_type() == Dbit) {
        if (data2.data_type() == Dint) {
            d.set_data_type(Dbit);
            d.bits().set(max(data1.bits().size(), DefBits));
            d.data().s = new char[d.bits().size()];
            for (int i = 0; i < d.bits().size(); i++) {
                if (i < data1.bits().size()) {
                    d.data().s[i] =
                        op_or(data1.data().s[i], bit(data2.data().i, i));
                }
                else
                    d.data().s[i] = bit(data2.data().i, i);
            }
        }
        else if (data2.data_type() == Dbit) {
            d.set_data_type(Dbit);
            d.bits().set(max(data1.bits().size(), data2.bits().size()));
            d.data().s = new char[d.bits().size()];
            for (int i = 0; i < d.bits().size(); i++) {
                int i1 =
                    (i < data1.bits().size() ? data1.data().s[i] : (int)BitL);
                int i2 =
                    (i < data2.bits().size() ? data2.data().s[i] : (int)BitL);
                d.data().s[i] = op_or(i1, i2);
            }
        }
        else if (data2.data_type() == Dtime) {
            d.set_data_type(Dbit);
            d.bits().set(max(data1.bits().size(), 8*sizeof(vl_time_t)));
            d.data().s = new char[d.bits().size()];
            for (int i = 0; i < d.bits().size(); i++) {
                if (i < data1.bits().size()) {
                    d.data().s[i] =
                        op_or(data1.data().s[i], bit(data2.data().t, i));
                }
                else
                    d.data().s[i] = bit(data2.data().t, i);
            }
        }
        else if (data2.data_type() == Dreal)
            d.setx(1);
    }
    else if (data1.data_type() == Dtime) {
        if (data2.data_type() == Dint) {
            d.set_data_type(Dtime);
            d.data().t = data1.data().t | data2.data().i;
        }
        else if (data2.data_type() == Dbit) {
            d.set_data_type(Dbit);
            d.bits().set(max(data2.bits().size(), 8*sizeof(vl_time_t)));
            d.data().s = new char[d.bits().size()];
            for (int i = 0; i < d.bits().size(); i++) {
                if (i < data2.bits().size()) {
                    d.data().s[i] =
                        op_or(data2.data().s[i], bit(data1.data().t, i));
                }
                else
                    d.data().s[i] = bit(data1.data().t, i);
            }
        }
        else if (data2.data_type() == Dtime) {
            d.set_data_type(Dtime);
            d.data().t = data1.data().t | data2.data().t;
        }
        else if (data2.data_type() == Dreal)
            d.setx(1);
    }
    else if (data1.data_type() == Dreal)
        d.setx(1);
    return (d);
}


// Overload '^'.
//
vl_var &
operator^(vl_var &data1, vl_var &data2)
{
    vl_var &d = new_var();
    if (data1.data_type() == Dint) {
        if (data2.data_type() == Dint) {
            d.set_data_type(Dint);
            d.data().i = data1.data().i ^ data2.data().i;
        }
        else if (data2.data_type() == Dbit) {
            d.set_data_type(Dbit);
            d.bits().set(max(data2.bits().size(), DefBits));
            d.data().s = new char[d.bits().size()];
            for (int i = 0; i < d.bits().size(); i++) {
                if (i < data2.bits().size()) {
                    d.data().s[i] =
                        op_xor(data2.data().s[i], bit(data1.data().i, i));
                }
                else
                    d.data().s[i] = op_xor(BitL, bit(data1.data().i, i));
            }
        }
        else if (data2.data_type() == Dtime) {
            d.set_data_type(Dtime);
            d.data().t = data1.data().i ^ data2.data().t;
        }
        else if (data2.data_type() == Dreal)
            d.setx(1);
    }
    else if (data1.data_type() == Dbit) {
        if (data2.data_type() == Dint) {
            d.set_data_type(Dbit);
            d.bits().set(max(data1.bits().size(), DefBits));
            d.data().s = new char[d.bits().size()];
            for (int i = 0; i < d.bits().size(); i++) {
                if (i < data1.bits().size()) {
                    d.data().s[i] =
                        op_xor(data1.data().s[i], bit(data2.data().i, i));
                }
                else
                    d.data().s[i] = op_xor(BitL, bit(data2.data().i, i));
            }
        }
        else if (data2.data_type() == Dbit) {
            d.set_data_type(Dbit);
            d.bits().set(max(data1.bits().size(), data2.bits().size()));
            d.data().s = new char[d.bits().size()];
            for (int i = 0; i < d.bits().size(); i++) {
                int i1 =
                    (i < data1.bits().size() ? data1.data().s[i] : (int)BitL);
                int i2 =
                    (i < data2.bits().size() ? data2.data().s[i] : (int)BitL);
                d.data().s[i] = op_xor(i1, i2);
            }
        }
        else if (data2.data_type() == Dtime) {
            d.set_data_type(Dbit);
            d.bits().set(max(data1.bits().size(), 8*sizeof(vl_time_t)));
            d.data().s = new char[d.bits().size()];
            for (int i = 0; i < d.bits().size(); i++) {
                if (i < data1.bits().size()) {
                    d.data().s[i] =
                        op_xor(data1.data().s[i], bit(data2.data().t, i));
                }
                else
                    d.data().s[i] = op_xor(BitL, bit(data2.data().t, i));
            }
        }
        else if (data2.data_type() == Dreal)
            d.setx(1);
    }
    else if (data1.data_type() == Dtime) {
        if (data2.data_type() == Dint) {
            d.set_data_type(Dtime);
            d.data().t = data1.data().t ^ data2.data().i;
        }
        else if (data2.data_type() == Dbit) {
            d.set_data_type(Dbit);
            d.bits().set(max(data2.bits().size(), 8*sizeof(vl_time_t)));
            d.data().s = new char[d.bits().size()];
            for (int i = 0; i < d.bits().size(); i++) {
                if (i < data2.bits().size()) {
                    d.data().s[i] =
                        op_xor(data2.data().s[i], bit(data1.data().t, i));
                }
                else
                    d.data().s[i] = op_xor(BitL, bit(data1.data().t, i));
            }
        }
        else if (data2.data_type() == Dtime) {
            d.set_data_type(Dtime);
            d.data().t = data1.data().t ^ data2.data().t;
        }
        else if (data2.data_type() == Dreal)
            d.setx(1);
    }
    else if (data1.data_type() == Dreal)
        d.setx(1);
    return (d);
}


// Overload '~'.
//
vl_var &
operator~(vl_var &data1)
{
    vl_var &d = new_var();
    if (data1.data_type() == Dint) {
        d.set_data_type(Dint);
        int w = 0;
        int c = data1.data().i;
        while (c) {
            c >>= 1;
            w++;
        }
        if (!w)
            w = 1;
        int mask = 0;
        while (w) {
            mask <<= 1;
            mask |= 1;
            w--;
        }
        d.data().i = data1.data().i ^ mask;
    }
    else if (data1.data_type() == Dbit) {
        d.set_data_type(Dbit);
        d.bits().set(data1.bits().size());
        d.data().s = new char[d.bits().size()];
        for (int i = 0; i < d.bits().size(); i++) {
            if (data1.data().s[i] == BitDC || data1.data().s[i] == BitZ)
                d.data().s[i] = BitDC;
            else if (data1.data().s[i] == BitH)
                d.data().s[i] = BitL;
            else
                d.data().s[i] = BitH;
        }
    }
    else if (data1.data_type() == Dtime) {
        d.set_data_type(Dtime);
        int w = 0;
        vl_time_t c = data1.data().t;
        while (c) {
            c >>= 1;
            w++;
        }
        if (!w)
            w = 1;
        vl_time_t mask = 0;
        while (w) {
            mask <<= 1;
            mask |= 1;
            w--;
        }
        d.data().t = data1.data().t ^ mask;
    }
    else if (data1.data_type() == Dreal)
        d.setx(1);
    return (d);
}


// Evaluate the tri-conditional data1 ? e1 : e2.
//
vl_var &
tcond(vl_var &data1, vl_expr *e1, vl_expr *e2)
{
    int xx = BitL;
    if (data1.data_type() == Dbit) {
        char *s =
            data1.array().size() ? *(char**)data1.data().d : data1.data().s;
        for (int i = 0; i < data1.bits().size(); i++) {
            if (s[i] == BitH)
                xx = BitH;
            else if (s[i] == BitDC || s[i] == BitZ) {
                xx = BitDC;
                break;
            }
        }
    }
    else if (data1.data_type() == Dint)
        xx = data1.data().i ? BitH : BitL;
    else if (data1.data_type() == Dtime)
        xx = data1.data().t ? BitH : BitL;
    else if (data1.data_type() == Dreal)
        xx = data1.data().r != 0.0 ? BitH : BitL;

    if (xx == BitH)
        return (e1->eval());
    else if (xx == BitL)
        return (e2->eval());

    vl_var &d = new_var();
    vl_var &d1 = e1->eval();
    int w1;
    if (d1.data_type() == Dbit)
        w1 = d1.bits().size();
    else if (d1.data_type() == Dint)
        w1 = sizeof(int)*8;
    else if (d1.data_type() == Dtime)
        w1 = sizeof(vl_time_t)*8;
    else {
        d.set((int)0);
        return (d);
    }

    vl_var &d2 = e2->eval();
    int w2;
    if (d2.data_type() == Dbit)
        w2 = d2.bits().size();
    else if (d2.data_type() == Dint)
        w2 = sizeof(int)*8;
    else if (d2.data_type() == Dtime)
        w2 = sizeof(vl_time_t)*8;
    else {
        d.set((int)0);
        return (d);
    }

    if (w2 > w1)
        w1 = w2;
    d.setx(w1);

    for (int i = 0; i < w1; i++) {
        int b1 = BitL;
        if (d1.data_type() == Dbit)
            b1 = i < d1.bits().size() ? d1.data().s[i] : (int)BitL;
        else if (d1.data_type() == Dint)
            b1 = i < (int)sizeof(int)*8 ? bit(d1.data().i, i) : (int)BitL;
        else if (d1.data_type() == Dtime)
            b1 = i < (int)sizeof(vl_time_t)*8 ?
                (((d1.data().t >> i) & 1) ? BitH : BitL) : (int)BitL;
        int b2 = BitL;
        if (d2.data_type() == Dbit)
            b2 = i < d2.bits().size() ? d2.data().s[i] : (int)BitL;
        else if (d2.data_type() == Dint)
            b2 = i < (int)sizeof(int)*8 ? bit(d2.data().i, i) : (int)BitL;
        else if (d2.data_type() == Dtime)
            b2 = i < (int)sizeof(vl_time_t)*8 ?
                (((d2.data().t >> i) & 1) ? BitH : BitL) : (int)BitL;

        if (b1 == b2)
            d.data().s[i] = (b1 != BitZ ? b1 : BitDC);
        else
            d.data().s[i] = BitDC;
    }
    return (d);
}


//---------------------------------------------------------------------------
//  Data variables and expressions
//---------------------------------------------------------------------------

void
vl_var::addb(vl_var &data1, int ival)
{
    setb(ival);
    int w2 = bits().size();
    char *s2 = data().s;
    int size = max(data1.bits().size(), w2);
    size++;
    data().s = new char[size];
    v_data_type = Dbit;
    add_bits(data().s, data1.data().s, s2, size, data1.bits().size(), w2);
    delete [] s2;
    if (data().s[size-1] != BitH)
        size--;
    bits().set(size);
}


void
vl_var::addb(vl_var &data1, vl_time_t tval)
{
    sett(tval);
    int w2 = bits().size();
    char *s2 = data().s;
    int size = max(data1.bits().size(), w2);
    size++;
    data().s = new char[size];
    v_data_type = Dbit;
    add_bits(data().s, data1.data().s, s2, size, data1.bits().size(), w2);
    delete [] s2;
    if (data().s[size-1] != BitH)
        size--;
    bits().set(size);
}


void
vl_var::addb(vl_var &data1, vl_var &data2)
{
    int size = max(data1.bits().size(), data2.bits().size());
    size++;
    data().s = new char[size];
    v_data_type = Dbit;
    add_bits(data().s, data1.data().s, data2.data().s, size,
        data1.bits().size(), data2.bits().size());
    if (data().s[size-1] != BitH)
        size--;
    bits().set(size);
}


void
vl_var::subb(vl_var &data1, int ival)
{
    setb(ival);
    int w2 = bits().size();
    char *s2 = data().s;
    bits().set(max(data1.bits().size(), w2));
    data().s = new char[bits().size()];
    v_data_type = Dbit;

    for (int i = 0; i < w2; i++) {
        if (s2[i] == BitL)
            s2[i] = BitH;
        else if (s2[i] == BitH)
            s2[i] = BitL;
    }
    add_bits(data().s, data1.data().s, s2, bits().size(), data1.bits().size(),
        w2, true);
    delete [] s2;
}


void
vl_var::subb(int ival, vl_var &data2)
{
    setb(ival);
    int w1 = bits().size();
    char *s1 = data().s;
    bits().set(max(data2.bits().size(), w1));
    data().s = new char[bits().size()];
    v_data_type = Dbit;

    char *s2 = new char[data2.bits().size()];
    memcpy(s2, data2.data().s, data2.bits().size());
    for (int i = 0; i < data2.bits().size(); i++) {
        if (s2[i] == BitL)
            s2[i] = BitH;
        else if (s2[i] == BitH)
            s2[i] = BitL;
    }
    add_bits(data().s, s1, s2, bits().size(), w1, data2.bits().size(), true);
    delete [] s1;
    delete [] s2;
}


void
vl_var::subb(vl_var &data1, vl_time_t tval)
{
    sett(tval);
    int w2 = bits().size();
    char *s2 = data().s;
    bits().set(max(data1.bits().size(), w2));
    data().s = new char[bits().size()];
    v_data_type = Dbit;

    for (int i = 0; i < w2; i++) {
        if (s2[i] == BitL)
            s2[i] = BitH;
        else if (s2[i] == BitH)
            s2[i] = BitL;
    }
    add_bits(data().s, data1.data().s, s2, bits().size(), data1.bits().size(),
        w2, true);
    delete [] s2;
}


void
vl_var::subb(vl_time_t tval, vl_var &data2)
{
    sett(tval);
    int w1 = bits().size();
    char *s1 = data().s;
    bits().set(max(data2.bits().size(), w1));
    data().s = new char[bits().size()];
    v_data_type = Dbit;

    char *s2 = new char[data2.bits().size()];
    memcpy(s2, data2.data().s, data2.bits().size());
    for (int i = 0; i < data2.bits().size(); i++) {
        if (s2[i] == BitL)
            s2[i] = BitH;
        else if (s2[i] == BitH)
            s2[i] = BitL;
    }
    add_bits(data().s, s1, s2, bits().size(), w1, data2.bits().size(), true);
    delete [] s1;
    delete [] s2;
}


void
vl_var::subb(vl_var &data1, vl_var &data2)
{
    v_data_type = Dbit;
    bits().set(max(data1.bits().size(), data2.bits().size()));
    data().s = new char[bits().size()];

    char *s2 = new char[data2.bits().size()];
    memcpy(s2, data2.data().s, data2.bits().size());
    for (int i = 0; i < data2.bits().size(); i++) {
        if (s2[i] == BitL)
            s2[i] = BitH;
        else if (s2[i] == BitH)
            s2[i] = BitL;
    }
    add_bits(data().s, data1.data().s, s2, bits().size(), data1.bits().size(),
        data2.bits().size(), true);
    delete [] s2;
}
// End of vl_var functions


// Expressions
//
// The vl_expr class is derived from the vl_var class, and can
// represent all of the various expression types, according to the
// e_type field.  The vl_expr can serve as an input (i.e., can be
// assigned to) or as an output.  As an input, the following e_types
// are valid:
//
//  IDExpr                simple variable reference
//  BitSelExpr            bit-select reference
//  PartSelExpr           part-select reference
//  ConcatExpr            concatenation (NOT multiple)
//
// For each of the first three input types, the e_data.ide.var field
// contains a pointer to the vl_var referenced, which is the variable
// that will be assigned to.  Calling the eval() function establishes
// the e_data.ide.var pointer.  For ConcatExpr, the source is the
// e_data.mcat.var field, which is created with the vl_expr, and
// contains the list of participating variables.
//
// All other e_types are read-only.  After calling eval(), the "this"
// vl_var contains the resulting value.


vl_expr::vl_expr()
{
    e_data.exprs.e1 = 0;
    e_data.exprs.e2 = 0;
    e_data.exprs.e3 = 0;
}


vl_expr::vl_expr(vl_var *v)
{
    e_data.exprs.e1 = 0;
    e_data.exprs.e2 = 0;
    e_data.exprs.e3 = 0;
    if (v->data_type() == Dconcat) {
        e_type = ConcatExpr;
        e_data.mcat.var = v;
    }
    else {
        e_type = IDExpr;
        e_data.ide.name = vl_strdup(v->name());
        e_data.ide.var = v;
    }
}


vl_expr::vl_expr(int t, int i, double r, void *p1, void *p2, void *p3)
{
    e_type = t;
    e_data.exprs.e1 = 0;
    e_data.exprs.e2 = 0;
    e_data.exprs.e3 = 0;
    switch (t) {
    case BitExpr:
        set((vl_bitexp_parse*)p1);
        break;
    case IntExpr:
        set(i);
        break;
    case RealExpr:
        set(r);
        break;
    case StringExpr:
        set((char*)p1);
        break;
    case IDExpr:
        e_data.ide.name = (char*)p1;
        break;
    case BitSelExpr:
    case PartSelExpr:
        e_data.ide.name = (char*)p1;
        e_data.ide.range = (vl_range*)p2;
        break;
    case ConcatExpr:
        e_data.mcat.rep = (vl_expr*)p2;
        e_data.mcat.var = new vl_var(0, 0, (lsList<vl_expr*>*)p1);
        break;
    case MinTypMaxExpr:
        e_data.exprs.e1 = (vl_expr*)p1;
        e_data.exprs.e2 = (vl_expr*)p2;
        e_data.exprs.e3 = (vl_expr*)p3;
        break;
    case FuncExpr:
        e_data.func_call.name = (char*)p1;
        e_data.func_call.args = (lsList<vl_expr*>*)p2;
        break;
    case UplusExpr:
    case UminusExpr:
    case UnotExpr:
    case UcomplExpr:
    case UandExpr:
    case UnandExpr:
    case UorExpr:
    case UnorExpr:
    case UxorExpr:
    case UxnorExpr:
        e_data.exprs.e1 = (vl_expr*)p1;
        break;
    case BplusExpr:
    case BminusExpr:
    case BtimesExpr:
    case BdivExpr:
    case BremExpr:
    case Beq2Expr:
    case Bneq2Expr:
    case Beq3Expr:
    case Bneq3Expr:
    case BlandExpr:
    case BlorExpr:
    case BltExpr:
    case BleExpr:
    case BgtExpr:
    case BgeExpr:
    case BandExpr:
    case BorExpr:
    case BxorExpr:
    case BxnorExpr:
    case BlshiftExpr:
    case BrshiftExpr:
        e_data.exprs.e1 = (vl_expr*)p1;
        e_data.exprs.e2 = (vl_expr*)p2;
        break;
    case TcondExpr:
        e_data.exprs.e1 = (vl_expr*)p1;
        e_data.exprs.e2 = (vl_expr*)p2;
        e_data.exprs.e3 = (vl_expr*)p3;
        break;
    case SysExpr:
        e_data.systask = new vl_sys_task_stmt((char*)p2, (lsList<vl_expr*>*)p1);
        break;
    }
}


vl_expr::~vl_expr()
{
    switch (e_type) {
    case IDExpr:
        delete [] e_data.ide.name;
        break;
    case BitSelExpr:
    case PartSelExpr:
        delete [] e_data.ide.name;
        delete e_data.ide.range;
        break;
    case ConcatExpr:
        delete e_data.mcat.rep;
        delete e_data.mcat.var;
        break;                        
    case MinTypMaxExpr:
        delete e_data.exprs.e1;
        delete e_data.exprs.e2;
        delete e_data.exprs.e3;
        break;                        
    case FuncExpr:
        delete [] e_data.func_call.name;
        delete_list(e_data.func_call.args);
        break;
    case UplusExpr:
    case UminusExpr:
    case UnotExpr:
    case UcomplExpr:
    case UandExpr:
    case UnandExpr:
    case UorExpr:
    case UnorExpr:
    case UxorExpr:
    case UxnorExpr:
        delete e_data.exprs.e1;
        break;
    case BplusExpr:
    case BminusExpr:
    case BtimesExpr:
    case BdivExpr:
    case BremExpr:
    case Beq2Expr:
    case Bneq2Expr:
    case Beq3Expr:
    case Bneq3Expr:
    case BlandExpr:
    case BlorExpr:
    case BltExpr:
    case BleExpr:
    case BgtExpr:
    case BgeExpr:
    case BandExpr:
    case BorExpr:
    case BxorExpr:
    case BxnorExpr:
    case BlshiftExpr:
    case BrshiftExpr:
        delete e_data.exprs.e1;
        delete e_data.exprs.e2;
        break;
    case TcondExpr:
        delete e_data.exprs.e1;
        delete e_data.exprs.e2;
        delete e_data.exprs.e3;
        break;
    case SysExpr:
        delete e_data.systask;
        break;
    }
}


vl_expr *
vl_expr::copy()
{
    vl_expr *retval = new vl_expr;
    retval->e_type = e_type;

    switch (e_type) {
    case BitExpr:
    case IntExpr:
    case RealExpr:
    case StringExpr:
        retval->assign(0, this, 0);
        break;
    case IDExpr:
        retval->e_data.ide.name = vl_strdup(e_data.ide.name);
        break;
    case BitSelExpr:
    case PartSelExpr:
        retval->e_data.ide.name = vl_strdup(e_data.ide.name);
        retval->e_data.ide.range = chk_copy(e_data.ide.range);
        break;
    case ConcatExpr: {
        retval->e_data.mcat.rep = chk_copy(e_data.mcat.rep);
        retval->e_data.mcat.var = chk_copy(e_data.mcat.var);
        break;                        
    }
    case MinTypMaxExpr:
        retval->e_data.exprs.e1 = chk_copy(e_data.exprs.e1);
        retval->e_data.exprs.e2 = chk_copy(e_data.exprs.e2);
        retval->e_data.exprs.e3 = chk_copy(e_data.exprs.e3);
        break;                        
    case FuncExpr:
        retval->e_data.func_call.name = vl_strdup(e_data.func_call.name);
        retval->e_data.func_call.args = copy_list(e_data.func_call.args);
        break;
    case UplusExpr:
    case UminusExpr:
    case UnotExpr:
    case UcomplExpr:
    case UandExpr:
    case UnandExpr:
    case UorExpr:
    case UnorExpr:
    case UxorExpr:
    case UxnorExpr:
        retval->e_data.exprs.e1 = chk_copy(e_data.exprs.e1);
        break;
    case BplusExpr:
    case BminusExpr:
    case BtimesExpr:
    case BdivExpr:
    case BremExpr:
    case Beq2Expr:
    case Bneq2Expr:
    case Beq3Expr:
    case Bneq3Expr:
    case BlandExpr:
    case BlorExpr:
    case BltExpr:
    case BleExpr:
    case BgtExpr:
    case BgeExpr:
    case BandExpr:
    case BorExpr:
    case BxorExpr:
    case BxnorExpr:
    case BlshiftExpr:
    case BrshiftExpr:
        retval->e_data.exprs.e1 = chk_copy(e_data.exprs.e1);
        retval->e_data.exprs.e2 = chk_copy(e_data.exprs.e2);
        break;
    case TcondExpr:
        retval->e_data.exprs.e1 = chk_copy(e_data.exprs.e1);
        retval->e_data.exprs.e2 = chk_copy(e_data.exprs.e2);
        retval->e_data.exprs.e3 = chk_copy(e_data.exprs.e3);
        break;
    case SysExpr:
        retval->e_data.systask = chk_copy(e_data.systask);
        break;
    }
    return (retval);
}


namespace {
    // Create a new variable if it doesn't already exist.  This is how
    // implicit declarations in port lists are dealt with.
    //
    vl_var *check_var(vl_simulator *sim, const char *name)
    {
        if (!sim->context()) {
            vl_error("internal, no current context!");
            sim->abort();
            return (0);
        }
        vl_var *v = sim->context()->lookup_var(name, false);
        if (!v) {
            vl_warn("implicit declaration of %s", name);
            vl_module *cmod = sim->context()->currentModule();
            if (cmod) {
                v = new vl_var;
                v->set_name(vl_strdup(name));
                if (!cmod->sig_st())
                    cmod->set_sig_st(new table<vl_var*>);
                cmod->sig_st()->insert(v->name(), v);
                v->or_flags(VAR_IN_TABLE);
            }
            else {
                vl_error("internal, no current module!");
                sim->abort();
            }
        }
        return (v);
    }
}


vl_var &
vl_expr::eval()
{
    vl_var &vo = *this;
    switch (e_type) {
    case BitExpr:
    case IntExpr:
    case RealExpr:
    case StringExpr:
        return (vo);
    }

    // The returned net type will always be REGnone, or REGevent for
    // events.
    // 
    reset();
    switch (e_type) {

    case IDExpr:
    case BitSelExpr:
    case PartSelExpr:
        if (!e_data.ide.var) {
            e_data.ide.var = check_var(VS(), e_data.ide.name);
            if (!e_data.ide.var)
                e_data.ide.var = this;
        }
        assign(0, e_data.ide.var, e_data.ide.range);
        if (e_data.ide.var->net_type() == REGevent)
            vo.set_net_type(REGevent);
        return (vo);

    case ConcatExpr: {
        if (v_data_type != Dbit) {
            v_data_type = Dbit;
            bits().set(DefBits);
            data().s = new char[bits().size()];
            memset(data().s, BitDC, bits().size());
        }
        char *endc = data().s;

        char alen = bits().size();
        int size = 0;
        int rep = 1;
        if (e_data.mcat.rep)
            rep = (int)e_data.mcat.rep->eval();
        for (int i = 0; i < rep; i++) {
            // order is msb first, loop in reverse order
            lsGen<vl_expr*> gen(e_data.mcat.var->data().c, true);
            vl_expr *e;
            while (gen.prev(&e)) {
                vl_var &d = e->eval();
                int sz = d.array().size() ? d.array().size() : 1;
                for (int j = 0; j < sz; j++) {
                    int w;
                    char *sd = d.bit_elt(j, &w);
                    if (endc + w > data().s + alen) {
                        alen = endc + w - data().s;
                        char *str = new char[alen];
                        char *r = str;
                        char *s = data().s;
                        while (s < endc)
                            *r++ = *s++;
                        delete [] data().s;
                        data().s = str;
                        endc = r;
                    }
                    char *r = endc;
                    endc += w;
                    while (r < endc)
                        *r++ = *sd++; 
                    size += w;
                }
            }
        }
        bits().set(size);
        return (vo);
    }
    case MinTypMaxExpr: {
        // three numbers: min/typ/max
        // two numbers: min/typ/max=typ
        // one number: min=typ/typ/max=typ
        switch (VS()->dmode()) {
        case DLYmin:
            return (e_data.exprs.e1->eval());
        default:
        case DLYtyp:
            if (e_data.exprs.e2)
                return (e_data.exprs.e2->eval());
            return (e_data.exprs.e1->eval());
        case DLYmax:
            if (e_data.exprs.e3)
                return (e_data.exprs.e3->eval());
            if (e_data.exprs.e2)
                return (e_data.exprs.e2->eval());
            else
                return (e_data.exprs.e1->eval());
        }
        vl_warn("(internal) bad min/typ/max format");
        return (vo);
    }
    case FuncExpr:
        if (!e_data.func_call.func) {
            e_data.func_call.func =
                VS()->context()->lookup_func(e_data.func_call.name);
            if (!e_data.func_call.func) {
                vl_error("unresolved function %s", e_data.func_call.name);
                VS()->abort();
                return (vo);
            }
        }
        e_data.func_call.func->eval_func(&vo, e_data.func_call.args);
        return (vo);
    case UplusExpr:
        vo = e_data.exprs.e1->eval();
        return (vo);
    case UminusExpr:
        vo.setx(DefBits);
        vo = e_data.exprs.e1->eval();
        vo = -vo;
        return (vo);
    case UnotExpr:
        vo = !e_data.exprs.e1->eval();
        return (vo);
    case UcomplExpr:
        vo = ~e_data.exprs.e1->eval();
        return (vo);
    case UnandExpr:
    case UandExpr:
    case UnorExpr:
    case UorExpr:
    case UxnorExpr:
    case UxorExpr:
        vo = reduce(e_data.exprs.e1->eval(), e_type);
        return (vo);
    case BtimesExpr:
        vo = (e_data.exprs.e1->eval() * e_data.exprs.e2->eval());
        return (vo);
    case BdivExpr:  
        vo = (e_data.exprs.e1->eval() / e_data.exprs.e2->eval());
        return (vo);
    case BremExpr: 
        vo = (e_data.exprs.e1->eval() % e_data.exprs.e2->eval());
        return (vo);
    case BlshiftExpr: 
        vo = (e_data.exprs.e1->eval() << e_data.exprs.e2->eval());
        return (vo);
    case BrshiftExpr:
        vo = (e_data.exprs.e1->eval() >> e_data.exprs.e2->eval());
        return (vo);
    case Beq3Expr: 
        vo = case_eq(e_data.exprs.e1->eval(), e_data.exprs.e2->eval());
        return (vo);
    case Bneq3Expr: 
        vo = case_neq(e_data.exprs.e1->eval(), e_data.exprs.e2->eval());
        return (vo);
    case Beq2Expr: 
        vo = (e_data.exprs.e1->eval() == e_data.exprs.e2->eval());
        return (vo);
    case Bneq2Expr:
        vo = (e_data.exprs.e1->eval() != e_data.exprs.e2->eval());
        return (vo);
    case BlandExpr: 
        vo = (e_data.exprs.e1->eval() && e_data.exprs.e2->eval());
        return (vo);
    case BlorExpr:
        vo = (e_data.exprs.e1->eval() || e_data.exprs.e2->eval());
        return (vo);
    case BltExpr: 
        vo = (e_data.exprs.e1->eval() < e_data.exprs.e2->eval());
        return (vo);
    case BleExpr: 
        vo = (e_data.exprs.e1->eval() <= e_data.exprs.e2->eval());
        return (vo);
    case BgtExpr: 
        vo = (e_data.exprs.e1->eval() > e_data.exprs.e2->eval());
        return (vo);
    case BgeExpr:  
        vo = (e_data.exprs.e1->eval() >= e_data.exprs.e2->eval());
        return (vo);
    case BplusExpr: 
        vo = (e_data.exprs.e1->eval() + e_data.exprs.e2->eval());
        return (vo);
    case BminusExpr:
        vo = (e_data.exprs.e1->eval() - e_data.exprs.e2->eval());
        return (vo);
    case BandExpr: 
        vo = (e_data.exprs.e1->eval() & e_data.exprs.e2->eval());
        return (vo);
    case BorExpr:  
        vo = (e_data.exprs.e1->eval() | e_data.exprs.e2->eval());
        return (vo);
    case BxorExpr:
        vo = (e_data.exprs.e1->eval() ^ e_data.exprs.e2->eval());
        return (vo);
    case BxnorExpr:
        vo = ~(e_data.exprs.e1->eval() ^ e_data.exprs.e2->eval());
        return (vo);
    case TcondExpr:
        vo = tcond(e_data.exprs.e1->eval(), e_data.exprs.e2, e_data.exprs.e3);
        return (vo);
    case SysExpr:
        vo = (VS()->*e_data.systask->action)(e_data.systask,
            e_data.systask->args());
        return (vo);
    }
    vl_warn("(internal) bad expression type");
    return (vo);
} 


// Set up an asynchronous action to perform when data changes,
// for events if the stmt is a vl_action_item, or for continuous
// assign and formal/actual association otherwise.
//   mode 0: chain
//   mode 1: unchain
//   mode 2: unchain by context
//
void
vl_expr::chcore(vl_stmt *stmt, int mode)
{
    vl_var vo = *this;
    switch (e_type) {
    case BitExpr:
    case IntExpr:
    case RealExpr:
    case StringExpr:
        // constants
        if (mode == 0) {
            vo.chain(stmt);
            vo.trigger();
        }
        else if (mode == 1)
            vo.unchain(stmt);
        else if (mode == 2)
            vo.unchain_disabled(stmt);
        return;
    case IDExpr:
    case BitSelExpr:
    case PartSelExpr:
        if (!e_data.ide.var) {
            e_data.ide.var = check_var(VS(), e_data.ide.name);
            if (!e_data.ide.var)
                e_data.ide.var = this;
        }
        if (e_data.ide.var != this) {
            if (mode == 0)
                e_data.ide.var->chain(stmt);
            else if (mode == 1)
                e_data.ide.var->unchain(stmt);
            else if (mode == 2)
                e_data.ide.var->unchain_disabled(stmt);
            if (e_data.ide.range) {
                if (mode == 0) {
                    e_data.ide.range->left()->chain(stmt);
                    if (e_data.ide.range->right() &&
                            e_data.ide.range->right() !=
                            e_data.ide.range->left())
                        e_data.ide.range->right()->chain(stmt);
                }
                else if (mode == 1) {
                    e_data.ide.range->left()->unchain(stmt);
                    if (e_data.ide.range->right() &&
                            e_data.ide.range->right() !=
                            e_data.ide.range->left())
                        e_data.ide.range->right()->unchain(stmt);
                }
                else if (mode == 2) {
                    e_data.ide.range->left()->unchain_disabled(stmt);
                    if (e_data.ide.range->right() &&
                            e_data.ide.range->right() !=
                            e_data.ide.range->left())
                        e_data.ide.range->right()->unchain_disabled(stmt);
                }
            }
        }
        return;
    case ConcatExpr:
        if (mode == 0)
            e_data.mcat.var->chain(stmt);
        else if (mode == 1)
            e_data.mcat.var->unchain(stmt);
        else if (mode == 2)
            e_data.mcat.var->unchain_disabled(stmt);
        return;
    case MinTypMaxExpr: 
        switch (VS()->dmode()) {
        case DLYmin:
            if (e_data.exprs.e1) {
                if (mode == 0)
                    e_data.exprs.e1->chain(stmt);
                else if (mode == 1)
                    e_data.exprs.e1->unchain(stmt);
                else if (mode == 2)
                    e_data.exprs.e1->unchain_disabled(stmt);
            }
            break;
        default:
        case DLYtyp:
            if (e_data.exprs.e2) {
                if (mode == 0)
                    e_data.exprs.e2->chain(stmt);
                else if (mode == 1)
                    e_data.exprs.e2->unchain(stmt);
                else if (mode == 2)
                    e_data.exprs.e2->unchain_disabled(stmt);
            }
            else if (e_data.exprs.e1) {
                if (mode == 0)
                    e_data.exprs.e1->chain(stmt);
                else if (mode == 1)
                    e_data.exprs.e1->unchain(stmt);
                else if (mode == 2)
                    e_data.exprs.e1->unchain_disabled(stmt);
            }
            break;
        case DLYmax:
            if (e_data.exprs.e3) {
                if (mode == 0)
                    e_data.exprs.e3->chain(stmt);
                else if (mode == 1)
                    e_data.exprs.e3->unchain(stmt);
                else if (mode == 2)
                    e_data.exprs.e3->unchain_disabled(stmt);
            }
            else if (e_data.exprs.e2) {
                if (mode == 0)
                    e_data.exprs.e2->chain(stmt);
                else if (mode == 1)
                    e_data.exprs.e2->unchain(stmt);
                else if (mode == 2)
                    e_data.exprs.e2->unchain_disabled(stmt);
            }
            else if (e_data.exprs.e1) {
                if (mode == 0)
                    e_data.exprs.e1->chain(stmt);
                else if (mode == 1)
                    e_data.exprs.e1->unchain(stmt);
                else if (mode == 2)
                    e_data.exprs.e1->unchain_disabled(stmt);
            }
            break;
        }
        return;
    case FuncExpr: {
        if (!e_data.func_call.func) {
            e_data.func_call.func =
                VS()->context()->lookup_func(e_data.func_call.name);
            if (!e_data.func_call.func) {
                vl_error("unresolved function %s", e_data.func_call.name);
                VS()->abort();
                return;
            }
        }
        lsGen<vl_expr*> fgen(e_data.func_call.args);
        vl_expr *e;
        while (fgen.next(&e)) {
            if (mode == 0)
                e->chain(stmt);
            else if (mode == 1)
                e->unchain(stmt);
            else if (mode == 2)
                e->unchain_disabled(stmt);
        }
        return;
    }

    case UplusExpr:
    case UminusExpr:
    case UnotExpr:
    case UcomplExpr:
    case UnandExpr:
    case UandExpr:
    case UnorExpr:
    case UorExpr:
    case UxnorExpr:
    case UxorExpr:
        if (e_data.exprs.e1) {
            if (mode == 0)
                e_data.exprs.e1->chain(stmt);
            else if (mode == 1)
                e_data.exprs.e1->unchain(stmt);
            else if (mode == 2)
                e_data.exprs.e1->unchain_disabled(stmt);
        }
        return;
    case BtimesExpr:
    case BdivExpr:  
    case BremExpr: 
    case BlshiftExpr: 
    case BrshiftExpr:
    case Beq3Expr: 
    case Bneq3Expr: 
    case Beq2Expr: 
    case Bneq2Expr:
    case BlandExpr: 
    case BlorExpr:
    case BltExpr: 
    case BleExpr: 
    case BgtExpr: 
    case BgeExpr:  
    case BplusExpr: 
    case BminusExpr:
    case BandExpr: 
    case BorExpr:  
    case BxorExpr:
    case BxnorExpr:
        if (e_data.exprs.e1) {
            if (mode == 0)
                e_data.exprs.e1->chain(stmt);
            else if (mode == 1)
                e_data.exprs.e1->unchain(stmt);
            else if (mode == 2)
                e_data.exprs.e1->unchain_disabled(stmt);
        }
        if (e_data.exprs.e2) {
            if (mode == 0)
                e_data.exprs.e2->chain(stmt);
            else if (mode == 1)
                e_data.exprs.e1->unchain(stmt);
            else if (mode == 2)
                e_data.exprs.e1->unchain_disabled(stmt);
        }
        return;
    case TcondExpr:
        if (e_data.exprs.e1) {
            if (mode == 0)
                e_data.exprs.e1->chain(stmt);
            else if (mode == 1)
                e_data.exprs.e1->unchain(stmt);
            else if (mode == 2)
                e_data.exprs.e1->unchain_disabled(stmt);
        }
        if (e_data.exprs.e2) {
            if (mode == 0)
                e_data.exprs.e2->chain(stmt);
            else if (mode == 1)
                e_data.exprs.e2->unchain(stmt);
            else if (mode == 2)
                e_data.exprs.e2->unchain_disabled(stmt);
        }
        if (e_data.exprs.e3) {
            if (mode == 0)
                e_data.exprs.e3->chain(stmt);
            else if (mode == 1)
                e_data.exprs.e3->unchain(stmt);
            else if (mode == 2)
                e_data.exprs.e3->unchain_disabled(stmt);
        }
        return;
    case SysExpr:
        if (!strcmp(e_data.systask->name(), "$time")) {
            if (mode == 0)
                VS()->time_data().chain(stmt);
            else if (mode == 1)
                VS()->time_data().unchain(stmt);
            else if (mode == 2)
                VS()->time_data().unchain_disabled(stmt);
        }
        return;
    }
}


// Return the source vl_var for the expression, when the expression is
// a simple reference.
//
vl_var *
vl_expr::source()
{
    switch (e_type) {
    case IDExpr:
    case BitSelExpr:
    case PartSelExpr:
        if (!e_data.ide.var) {
            e_data.ide.var = check_var(VS(), e_data.ide.name);
            if (!e_data.ide.var)
                e_data.ide.var = this;
        }
        return (e_data.ide.var);
    case ConcatExpr:
        if (!e_data.mcat.rep)
            return (e_data.mcat.var);
    default:
        break;
    }
    return (0);
}
// End of vl_expr functions.

