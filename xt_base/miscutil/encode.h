
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

//
// Copyright (C) 1991-2, RSA Data Security, Inc. Created 1991. All
// rights reserved.
//
//  License to copy and use this software is granted provided that it
//  is identified as the "RSA Data Security, Inc. MD5 Message-Digest
//  Algorithm" in all material mentioning or referencing this software
//  or this function.
//
//  License is also granted to make and use derivative works provided
//  that such works are identified as "derived from the RSA Data
//  Security, Inc. MD5 Message-Digest Algorithm" in all material
//  mentioning or referencing the derived work.
//
//  RSA Data Security, Inc. makes no representations concerning either
//  the merchantability of this software or the suitability of this
//  software for any particular purpose. It is provided "as is"
//  without express or implied warranty of any kind.
//
//  These notices must be retained in any copies of any part of this
//  documentation and/or software.
//========================================================================

#ifndef ENCODE_H
#define ENCODE_H

#include <stdio.h>
#include <string.h>

typedef unsigned long UINT4;

// MD5 context
struct MD5cx
{
    MD5cx() { reinit(); }

    void reinit()
        {
            // Load magic initialization constants
            state[0] = 0x67452301;
            state[1] = 0xefcdab89;
            state[2] = 0x98badcfe;
            state[3] = 0x10325476;
            memset(count, 0, sizeof(count));
            memset(buffer, 0, sizeof(buffer));
        }

    void update(const unsigned char*, unsigned int);
    void final(unsigned char[16]);

private:
    UINT4 state[4];	// state (ABCD)
    UINT4 count[2];	// number of bits, modulo 2^64 (lsb first)
    unsigned char buffer[64];	// input buffer
};

struct MimeState
{
    MimeState() { outfname = 0; outfile = 0; fnames = 0; nfiles = 0;
        written = 0; }

    char *outfname;   // out file base, must be provided
    FILE *outfile;    // current file pointer, close when finished if non-null
    char **fnames;    // list of output file names used (return, malloc'ed)
    int nfiles;       // size of list (return)
    long written;     // bytes written
};

extern int encode(struct MimeState*, FILE*, const char*, FILE*,
    const char*, const char*, long, const char*);

#endif

