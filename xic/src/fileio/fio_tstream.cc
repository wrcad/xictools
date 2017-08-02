
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "fio.h"
#include "fio_cvt_base.h"


//
// Read/write records to a transform stream.  A transform stream is a
// sequence of compact instance transform records.
//

#define TS_INIT 32


void
ts_writer::alloc_stream()
{
    if (!ts_stream) {
        if (!ts_length)
            ts_length = TS_INIT;
        else
            ts_length++;  // END record
        ts_stream = new unsigned char[ts_length];
    }
    else if (ts_used >= ts_length) {
        unsigned char *t = new unsigned char[ts_length + ts_length];
        memcpy(t, ts_stream, ts_length);
        delete [] ts_stream;
        ts_stream = t;
        ts_length += ts_length;
    }
}


// Write a transform record and array parameters to the stream.
//
void
ts_writer::add_record(CDtx *tx, unsigned int nx, unsigned int ny,
    int dx, int dy)
{
    unsigned char fmt = enc_byte(tx, nx, ny);
    add_uchar(fmt);
    add_signed(tx->tx);
    add_signed(tx->ty);
    if (fmt & TS_MAG)
        add_real(tx->magn);
    if (fmt & TS_ARX) {
        add_unsigned(nx);
        add_signed(dx);
    }
    if (fmt & TS_ARY) {
        add_unsigned(ny);
        add_signed(dy);
    }
    // hide an end record
    add_uchar(TS_END);
    ts_used--;
}


// Encode the first byte of a transform record.  The bit fields are
//
//  bits 0-3:  rotation/reflection code
//  bit 4   :  has magnification flag
//  bit 5   :  has x array data flag
//  bit 6   :  has y array data flag
//  bit 7   :  end of records flag (only set in last record of stream)
//
unsigned char
ts_writer::enc_byte(CDtx *tx, unsigned int nx, unsigned int ny)
{
    unsigned char f = tx->encode_transform();
    if (tx->magn != 1.0)
        f |= TS_MAG;
    if (nx > 1)
        f |= TS_ARX;
    if (ny > 1)
        f |= TS_ARY;
    return (f);
}


// Write an unsigned integer to the stream.
//
void
ts_writer::add_unsigned(unsigned int i)
{
    unsigned char b = i & 0x7f;
    i >>= 7;        // >> 7
    if (!i) {
        add_uchar(b);
        return;
    }
    b |= 0x80;
    add_uchar(b);
    b = i & 0x7f;
    i >>= 7;        // >> 14
    if (!i) {
        add_uchar(b);
        return;
    }
    b |= 0x80;
    add_uchar(b);
    b = i & 0x7f;
    i >>= 7;        // >> 21
    if (!i) {
        add_uchar(b);
        return;
    }
    b |= 0x80;
    add_uchar(b);
    b = i & 0x7f;
    i >>= 7;        // >> 28
    if (!i) {
        add_uchar(b);
        return;
    }
    b |= 0x80;
    add_uchar(b);
    b = i & 0xf;    // 4 bits left
    add_uchar(b);
}


// Write a signed integer to the stream.
//
void
ts_writer::add_signed(int i)
{
    bool neg = false;
    if (i < 0) {
        neg = true;
        i = -i;
    }
    unsigned char b = i & 0x3f;
    b <<= 1;
    if (neg)
        b |= 1;
    i >>= 6;        // >> 6
    if (!i) {
        add_uchar(b);
        return;
    }
    b |= 0x80;
    add_uchar(b);
    b = i & 0x7f;
    i >>= 7;        // >> 13
    if (!i) {
        add_uchar(b);
        return;
    }
    b |= 0x80;
    add_uchar(b);
    b = i & 0x7f;
    i >>= 7;        // >> 20
    if (!i) {
        add_uchar(b);
        return;
    }
    b |= 0x80;
    add_uchar(b);
    b = i & 0x7f;
    i >>= 7;        // >> 27
    if (!i) {
        add_uchar(b);
        return;
    }
    b |= 0x80;
    add_uchar(b);
    b = i & 0xf;    // 4 bits left (sign bit excluded)
    add_uchar(b);
}


// Write a real value to the stream.
//
void
ts_writer::add_real(double val)
{
    // ieee double
    // first byte is lsb of mantissa
    union { double n; unsigned char b[sizeof(double)]; } u;
    u.n = 1.0;
    if (u.b[0] == 0) {
        // machine is little-endian, write bytes in order
        u.n = val;
        add_uchar(u.b[0]);
        add_uchar(u.b[1]);
        add_uchar(u.b[2]);
        add_uchar(u.b[3]);
        add_uchar(u.b[4]);
        add_uchar(u.b[5]);
        add_uchar(u.b[6]);
        add_uchar(u.b[7]);   
    }
    else {
        // machine is big-endian, write bytes in reverse order
        u.n = val;
        add_uchar(u.b[7]);
        add_uchar(u.b[6]);
        add_uchar(u.b[5]);
        add_uchar(u.b[4]);
        add_uchar(u.b[3]);
        add_uchar(u.b[2]);
        add_uchar(u.b[1]);
        add_uchar(u.b[0]);
    }
}
// End of ts_writer functions.


// Read a transform record and fill in the structs passed accordingly.
//
bool
ts_reader::read_record(CDtx *tx, CDap *ap)
{
    if (!ts_stream)
        return (false);
    unsigned char c = read_uchar();
    if (c & TS_END)
        // termination record;
        return (false);
    tx->decode_transform(c);
    tx->tx = read_signed();
    tx->ty = read_signed();
    if (c & TS_MAG)
        tx->magn = read_real();
    else
        tx->magn = 1.0;
    if (c & TS_ARX) {
        ap->nx = read_unsigned();
        ap->dx = read_signed();
    }
    else {
        ap->nx = 1;
        ap->dx = 0;
    }
    if (c & TS_ARY) {
        ap->ny = read_unsigned();
        ap->dy = read_signed();
    }
    else {
        ap->ny = 1;
        ap->dy = 0;
    }
    return (true);
}


// Read an unsigned integer from the stream.
//
unsigned int
ts_reader::read_unsigned()
{
    unsigned int b = read_uchar();
    unsigned int i = (b & 0x7f);
    if (!(b & 0x80))
        return (i);
    b = read_uchar();
    i |= ((b & 0x7f) << 7);
    if (!(b & 0x80))
        return (i);
    b = read_uchar();
    i |= ((b & 0x7f) << 14);
    if (!(b & 0x80))
        return (i);
    b = read_uchar();
    i |= ((b & 0x7f) << 21);
    if (!(b & 0x80))
        return (i);
    b = read_uchar();
    i |= ((b & 0xf) << 28); // 4 bits left

    if (b & 0xf0) {
        // overflow, can't happen
        return (0);
    }
    return (i);
}


// Read a signed integer from the stream.
//
int
ts_reader::read_signed()
{
    unsigned int b = read_uchar();
    bool neg = b & 1;
    unsigned int i = (b & 0x7f) >> 1;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_uchar();
    i |= (b & 0x7f) << 6;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_uchar();
    i |= (b & 0x7f) << 13;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_uchar();
    i |= (b & 0x7f) << 20;
    if (!(b & 0x80))
        return (neg ? -i : i);
    b = read_uchar();
    i |= (b & 0xf) << 27; // 4 bits left (sign bit excluded)

    if (b & 0xf0) {
        // overflow, can't happen
        return (0);
    }
    return (neg ? -i : i);
}


// Read a real value from the stream.
//
double
ts_reader::read_real()
{
    // ieee double
    // first byte is lsb of mantissa
    union { double n; unsigned char b[sizeof(double)]; } u;
    u.n = 1.0;
    if (u.b[0] == 0) {
        // machine is little-endian, read bytes in order
        for (int i = 0; i < (int)sizeof(double); i++)
            u.b[i] = read_uchar();
    }
    else {
        // machine is big-endian, read bytes in reverse order
        for (int i = sizeof(double) - 1; i >= 0; i--)
            u.b[i] = read_uchar();
    }
    return (u.n);
}
// End of ts_reader functions.

