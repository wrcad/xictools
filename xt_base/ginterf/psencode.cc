
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
 * Ginterf Graphical Interface Library                                    *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/**************************************************
 *                                                *
 * Run length and ascii85 encoding for Postscript *
 *                                                *
 **************************************************/

// RLE: [byte code][1-128 bytes data]
// code: 0 - 127 => 1 - 128 bytes of literal data.
//       128 => end, data byte not used.
//       129 - 255, repeat (single) data item 128 - 2 times.

// ASCII85: given binary bytes b1,...,b4 convert to
// 5-tuple a1,...,a5, where
// b1*256*256*256 + b2*256*256 + b3*256 + b4 =
// a1*85*85*85*85 + a2*85*85*85 + a3*85*85 + a4*85 + a5.
// The output is a1+'!', a2+'!',...,a5+'!' except if all
// are 0, in which case output is 'z'.
// If the data set is not divisible by four, zero pad
// the last 4-tuple and convert, not using 'z'.  Print
// the first size % 4 + 1 characters.

#include <stdio.h>

namespace ginterf
{
    extern bool PS_rll85dump(FILE*, unsigned char*, int, int, int);

    // Structure to return a run.
    struct sRL
    {
        unsigned char key;
        unsigned char data[128];
    };

    struct sPSdump
    {
        sPSdump(unsigned char *base, int maxy, int bytpline)
            {
                Ocnt = 0;
                Dcnt = 0;
                BytesPerLine = bytpline;
                I4 = 0;
                if (base) {
                    Data = base + maxy*bytpline;
                    End = base - bytpline;
                }
                else
                    Data = End = 0;
            }

        bool dump(FILE*, bool);

    private:
        sRL *get_rllrec();
        void ascii85dump(FILE*, sRL*);
        void enc85(char *, unsigned char*) const;

        // Character output, adding returns periodically.
        void outc(int c, FILE *fp)
            {
                putc(c, fp);
                Ocnt++;
                if (Ocnt == 72) {
                    putc('\n', fp);
                    Ocnt = 0;
                }
            }

        // Inlines to get data out of our upside-down matrix.
        unsigned char nextbyte()
            {
                unsigned char c = *Data++;
                if (++Dcnt == BytesPerLine) {
                    Dcnt = 0;
                    Data -= 2*BytesPerLine;
                }
                return (c);
            }

        void prevbyte()
            {
                if (Dcnt) {
                    Dcnt--;
                    Data--;
                }
                else {
                    Dcnt = BytesPerLine - 1;
                    Data += 2*BytesPerLine - 1;
                }
            }

        int Ocnt;
        int Dcnt;
        int BytesPerLine;
        int I4;
        unsigned char *Data, *End;
        sRL rl;
        unsigned char cin[4];
    };


    // Dump the bitmap in ASCII85 format.  If doRLE is true, perform
    // run length encoding first.  Return true if error.
    //
    bool
    PS_rll85dump(FILE *fp, unsigned char *base, int bytpline, int maxy,
        int doRLE)
    {
        sPSdump psd(base, maxy, bytpline);
        return (psd.dump(fp, doRLE));
    }
}


using namespace ginterf;

// Dump the file, return true on error.
//
bool
sPSdump::dump(FILE *fp, bool doRLE)
{
    if (!Data || Data <= End)
        return (true);

    if (!doRLE) {
        unsigned char c[4];
        char cout[5];
        int size = Data - End;
        int sz = size/4;
        for (int i = 0; i < sz; i++) {
            c[0] = nextbyte();
            c[1] = nextbyte();
            c[2] = nextbyte();
            c[3] = nextbyte();
            enc85(cout, c);
            outc(cout[0], fp);
            if (cout[0] != 'z') {
                outc(cout[1], fp);
                outc(cout[2], fp);
                outc(cout[3], fp);
                outc(cout[4], fp);
            }
            if (ferror(fp))
                return (true);
        }
        c[0] = c[1] = c[2] = c[3] = 0;
        switch (size % 4) {
            case 0:
                return (false);
            case 1:
                c[0] = nextbyte();
                break;
            case 2:
                c[0] = nextbyte();
                c[1] = nextbyte();
                break;
            case 3:
                c[0] = nextbyte();
                c[1] = nextbyte();
                c[2] = nextbyte();
                break;
        }
        enc85(cout, c);
        if (cout[0] == 'z')
            cout[0] = cout[1] = cout[2] = cout[3] = '!';
        outc(cout[0], fp);
        outc(cout[1], fp);
        if (size % 4 > 1)
            outc(cout[2], fp);
        if (size % 4 > 2)
            outc(cout[3], fp);
        if (ferror(fp))
            return (true);
    }
    else {
        for (;;) {
            sRL *xrl = get_rllrec();
            ascii85dump(fp, xrl);
            if (xrl->key == 128)
                break;
            if (ferror(fp))
                return (true);
        }
    }
    return (false);
}


// Get a run.
//
sRL *
sPSdump::get_rllrec()
{
    if (Data == End) {
        rl.key = 128;
        return (&rl);
    }
    rl.data[0] = nextbyte();
    int i = 1;
    while (Data != End && *Data == rl.data[0]) {
        nextbyte();
        i++;
        if (i == 128)
            break;
    }
    if (i > 1) {
        rl.key = 257 - i;
        return (&rl);
    }
    if (Data == End) {
        rl.key = 0;
        return (&rl);
    }
    rl.data[1] = nextbyte();
    if (Data == End) {
        rl.key = 1;
        return (&rl);
    }
    i = 2;
    for (;;) {
        rl.data[i] = nextbyte();
        i++;
        if (Data == End || i == 128)
            break;
        if (*Data == rl.data[i-1] && *Data == rl.data[i-2]) {
            i -= 2;
            prevbyte();
            prevbyte();
            break;
        }
    }
    rl.key = i-1;
    return (&rl);
}


// Dump a run in ASCII85 format.
//
void
sPSdump::ascii85dump(FILE *fp, sRL *xrl)
{
    char cout[5];
    int i, n;
    if (xrl->key == 128) {
        if (!I4)
            return;
        for (i = I4; i < 4; i++)
            cin[i] = 0;
        enc85(cout, cin);
        if (cout[0] == 'z')
            cout[0] = cout[1] = cout[2] = cout[3] = '!';
        outc(cout[0], fp);
        outc(cout[1], fp);
        if (I4 > 1)
            outc(cout[2], fp);
        if (I4 > 2)
            outc(cout[3], fp);
        return;
    }
    else if (xrl->key > 128)
        n = 2;
    else
        n = xrl->key + 2;

    for (i = 0; i < n; i++) {
        cin[I4] = i ? xrl->data[i-1] : xrl->key;
        I4++;
        if (I4 == 4) {
            I4 = 0;
            enc85(cout, cin);
            outc(cout[0], fp);
            if (cout[0] != 'z') {
                outc(cout[1], fp);
                outc(cout[2], fp);
                outc(cout[3], fp);
                outc(cout[4], fp);
            }
        }
    }
}


#define A4 (85*85*85*85)
#define A3 (85*85*85)
#define A2 (85*85)
#define A1 85


// Convert binary 4-tuple in ci to ASCII85 5-tuple.
//
void
sPSdump::enc85(char *co, unsigned char *ci) const
{
    unsigned l = ((unsigned int)ci[0] << 24) + ((unsigned int)ci[1] << 16) +
        ((unsigned int)ci[2] << 8) + ci[3];
    if (l == 0) {
        co[0] = 'z';
        return;
    }
    unsigned n = l/A4;
    co[0] = n+33;
    l -= n*A4;
    n = l/A3;
    co[1] = n+33;
    l -= n*A3;
    n = l/A2;
    co[2] = n+33;
    l -= n*A2;
    n = l/A1;
    co[3] = n+33;
    l -= n*A1;
    co[4] = l+33;
}

