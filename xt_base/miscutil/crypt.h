
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef CRYPT_H
#define CRYPT_H

#include <stdio.h>
#include <string.h>

#define CRYPT_WORKSPACE 4096
#define ROTORSZ 256

struct sCrypt
{
    sCrypt();
    const char *getkey(const char*);
    void initialize();
    void translate(unsigned char*, size_t);
    bool begin_encryption(FILE*, const char**, unsigned char*, size_t);
    bool begin_decryption(FILE*, const char**);

    void readkey(char *bf) { for (int i = 0; i < 13; i++) bf[i] = key[i]; }
    void savekey(char *bf) { for (int i = 0; i < 13; i++) key[i] = bf[i]; }

    static bool write_header(FILE*);
    static bool is_encrypted(FILE*);

private:
    int     n1;
    int     n2;
    int     nr2;
    char    t1[ROTORSZ];
    char    t2[ROTORSZ];
    char    t3[ROTORSZ];
    char    deck[ROTORSZ];
    char    key[13];
    bool    initialized;
};

#endif

