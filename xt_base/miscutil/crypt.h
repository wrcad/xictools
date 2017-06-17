
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
 $Id: crypt.h,v 1.1 2008/05/08 04:59:28 stevew Exp $
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

