
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2007 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  WHITELEY RESEARCH INCORPORATED PROPRIETARY SOFTWARE                   *
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
 *                                                                        *
 * XIC Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id: fio_tstream.h,v 5.6 2010/05/31 05:53:44 stevew Exp $
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

