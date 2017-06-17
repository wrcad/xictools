
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
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
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id: encode.h,v 1.3 2008/08/22 18:41:20 stevew Exp $
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

