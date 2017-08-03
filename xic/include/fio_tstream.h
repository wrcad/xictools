
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

#ifndef FIO_TSTREAM_H
#define FIO_TSTREAM_H


//
// Classes to read/write a compressed sequence of instance transforms with
// array data.
//

#define TS_MAG 0x10
#define TS_ARX 0x20
#define TS_ARY 0x40
#define TS_END 0x80

// Base class for writing.
//
struct ts_writer
{
    ts_writer(unsigned char *str, unsigned long cnt, bool co)
        {
            ts_stream = str;
            ts_used = cnt;
            ts_length = 0;
            ts_cnt_only = co;
            ts_use_alloc = false;
        }

    void init(unsigned char *buf)
        {
            ts_stream = buf;
            ts_used = 0;
        }

    void add_uchar(unsigned char c)
        {
            if (ts_cnt_only) {
                ts_used++;
                return;
            }
            if (ts_use_alloc)
                alloc_stream();
            ts_stream[ts_used++] = c;
        }

    unsigned long bytes_used() { return (ts_used); }

    void alloc_stream();
    void add_record(CDtx*, unsigned int, unsigned int, int, int);
    unsigned char enc_byte(CDtx*, unsigned int, unsigned int);

    void add_unsigned(unsigned int);
    void add_signed(int);
    void add_real(double);

protected:
    unsigned char *ts_stream;
    unsigned long ts_used;
    unsigned long ts_length;
    bool ts_cnt_only;
    bool ts_use_alloc;
};


// Base class for reading.
//
struct ts_reader
{
    ts_reader(unsigned char *s)
        {
            ts_stream = s;
            ts_offset = 0;
        }

    void init(unsigned char *buf)
        {
            ts_stream = buf;
            ts_offset = 0;
        }

    unsigned char read_uchar()
        {
            return (ts_stream ? ts_stream[ts_offset++] : TS_END);
        }

    unsigned long bytes_used() { return (ts_offset); }

    bool read_record(CDtx*, CDap*);

    unsigned int read_unsigned();
    int read_signed();
    double read_real();

protected:
    unsigned const char *ts_stream;
    unsigned long ts_offset;
};

#endif

