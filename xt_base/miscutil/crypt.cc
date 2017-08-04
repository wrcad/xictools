
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

/*========================================================================*
 *	This is derived from "enigma.c" in file cbw.tar from
 *	anonymous FTP host watmsg.waterloo.edu: pub/crypt/cbw.tar.Z
 *
 *	A one-rotor machine designed along the lines of Enigma
 *	but considerably trivialized.
 *========================================================================*/

#include <stdlib.h>
#include <unistd.h>
#include "crypt.h"
#include "encode.h"


#define MASK 0377

 namespace {
    // This is prepended to encrypted files for id purposes, and is
    // followed by 16 bytes of MD5 checksum and the encoded data.
    //
    char enc_header[] = {
        char(7), char(7), char('W'|0x80), char('R'|0x80), char('e'|0x80),
        char('n'|0x80), char('c'|0x80), char('r'|0x80), char('y'|0x80),
        char('p'|0x80), char('t'|0x80), char('e'|0x80), char('d'|0x80),
        char(0) };

    char *makekey(const char*, const char*);
 }


sCrypt::sCrypt()
{
    memset(this, 0, sizeof(sCrypt));
}


// Get a key into the key field.  If failure, an error message is
// returned, 0 is returned on success.
//
const char *
sCrypt::getkey(const char *pw)
{
    char pass[9];
    char salt[3];
    strncpy(pass, pw, 8);
    pass[8] = 0;
    strncpy(salt, pw, 2);
    salt[2] = 0;
    char *k = makekey(pass, salt);
    // never fails
    if (!k)
        return ("bad password");
    memcpy(key, k, 13);
    delete [] k;
    return (0);
}


// Initialize the class data fields.
//
void
sCrypt::initialize()
{
    if (!initialized) {
        long seed = 123;
        for (int i = 0; i < 13; i++)
            seed = seed*key[i] + i;
        for (int i = 0; i < ROTORSZ; i++) {
            t1[i] = i;
            deck[i] = i;
        }
        for (int i = 0; i < ROTORSZ; i++) {
            seed = 5*seed + key[i%13];
            if (sizeof(long) > 4) {
                // Force seed to stay in 32-bit signed math
                if (seed & 0x80000000)
                    seed = seed | (-1L & ~0xFFFFFFFFL);
                else
                    seed &= 0x7FFFFFFF;
            }
            unsigned rnd = seed % 65521;
            int k = ROTORSZ-1 - i;
            int ic = (rnd&MASK) % (k+1);
            rnd >>= 8;
            int temp = t1[k];
            t1[k] = t1[ic];
            t1[ic] = temp;
            if (t3[k] != 0)
                continue;
            ic = (rnd&MASK) % k;
            while(t3[ic] != 0)
                ic = (ic+1) % k;
            t3[k] = ic;
            t3[ic] = k;
        }
        for (int i = 0; i < ROTORSZ; i++)
            t2[t1[i]&MASK] = i;
        initialized = true;
    }
    n1 = 0;
    n2 = 0;
    nr2 = 0;
}


// Translate, in place, nchars in string.  This is bi-directional, i.e.,
// useful for encryption and decryption.
//
void
sCrypt::translate(unsigned char *buffer, size_t nchars)
{
    if (!initialized)
        return;
    for (unsigned int c = 0; c < nchars; c++) {
        unsigned int i = buffer[c];
        int nr1 = n1;
        buffer[c] = t2[(t3[(t1[(i+nr1)&MASK]+nr2)&MASK]-nr2)&MASK]-nr1;
        n1++;
        if (n1 == ROTORSZ) {
            n1 = 0;
            n2++;
            if (n2 == ROTORSZ)
                n2 = 0;
            nr2 = n2;
        }
    }
}


// Begin the encryption process, returning true if success.  If an
// error occurs, an error message is returned in err.  The buffer
// contains the first CRYPT_WORKSPACE or fewer chars to encrypt, n is
// the actual count.  These will be written to fp if succesful.  The
// remaining chars, if any, should be passed through translate() and
// written to fp.
//
bool
sCrypt::begin_encryption(FILE *fp, const char **err, unsigned char *buffer,
    size_t n)
{
    *err = 0;
    if (!write_header(fp)) {
        *err = "write failed";
        return (false);
    }
    unsigned char dig[16];
    MD5cx cx;
    cx.update(buffer, n);
    cx.final(dig);
    if (fwrite(dig, 1, 16, fp) != 16) {
        *err = "write failed";
        return (false);
    }
    translate(buffer, n);
    if (fwrite(buffer, 1, n, fp) != n) {
        *err = "write failed";
        return (false);
    }
    return (true);
}


// Begin the decryption process.  This should be called after
// is_encrypted() returns true.  If there is an error, false is
// returned, and an error message will be returned in err.  The file
// pointer points to the start of data, to be run through translate().
//
bool
sCrypt::begin_decryption(FILE *fp, const char **err)
{
    *err = 0;
    unsigned char fdig[16];
    if (fread(fdig, 1, 16, fp) != 16) {
        *err = "read failed";
        return (false);
    }
    unsigned char buffer[CRYPT_WORKSPACE];
    long posn = ftell(fp);
    int n = fread(buffer, 1, CRYPT_WORKSPACE, fp);
    fseek(fp, posn, SEEK_SET);
    if (n <= 0) {
        *err = "read failed";
        return (false);
    }
    translate(buffer, n);
    MD5cx cx;
    cx.update(buffer, n);
    unsigned char dig[16];
    cx.final(dig);
    if (memcmp(dig, fdig, 16)) {
        *err = "incorrect key";
        return (false);
    }
    initialize();
    return (true);
}


//
// Static functions
//

// Write an identification word ahead of the encrypted data.
//
bool
sCrypt::write_header(FILE *fp)
{
    size_t n = fwrite(enc_header, 1, strlen(enc_header), fp);
    return (n == strlen(enc_header));
}


// Determine if the file is one of ours by checking the file header.
//
bool
sCrypt::is_encrypted(FILE *fp)
{
    if (!fp)
        return (false);
    char buf[32];
    long posn = ftell(fp);
    size_t n = fread(buf, 1, strlen(enc_header), fp);
    if (n == strlen(enc_header) &&
            !memcmp(enc_header, buf, strlen(enc_header)))
        return (true);
    fseek(fp, posn, SEEK_SET);
    return (false);
}
// End of sCrypt functions


//
// The following functions implement "makekey".  Borrowed from FreeBSD.
// Hacked for local purposes.
//

/*
 * Copyright (c) 1999
 *      University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the author nor the names of any co-contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 */

namespace {
    // 0 ... 63 => ascii - 64
    unsigned char itoa64[] =
        "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    void
    _crypt_to64(char *s, unsigned long v, int n)
    {
        while (--n >= 0) {
            *s++ = itoa64[v & 0x3f];
            v >>= 6;
        }
    }
}


/*
 * ----------------------------------------------------------------------------
 * "THE BEER-WARE LICENSE" (Revision 42):
 * <phk@login.dknet.dk> wrote this file.  As long as you retain this notice you
 * can do whatever you want with this stuff. If we meet some day, and you think
 * this stuff is worth it, you can buy me a beer in return.   Poul-Henning Kamp
 * ----------------------------------------------------------------------------
 */

#define MD5_SIZE 16

namespace {
    char *
    makekey(const char *pw, const char *salt)
    {
        // This string is magic for this algorithm.  Having it this way,
        // we can get get better later on
        const char *magic = "$1$";

        if (!pw || !salt)
            return (0);

        // Refine the Salt first
        const char *sp = salt;

        // If it starts with the magic string, then skip that
        if (!strncmp(sp, magic, strlen(magic)))
            sp += strlen(magic);

        // It stops at the first '$', max 8 chars
        const char *ep;
        for (ep = sp; *ep && *ep != '$' && ep < (sp+8); ep++)
            continue;

        // get the length of the true salt
        int sl = ep - sp;

        MD5cx ctx;
        unsigned char final[MD5_SIZE];

        // The password first, since that is what is most unknown
        ctx.update((unsigned char*)pw, strlen(pw));

        // Then our magic string
        ctx.update((unsigned char*)magic, strlen(magic));

        // Then the raw salt
        ctx.update((unsigned char*)sp, sl);

        // Then just as many characters of the MD5(pw, salt, pw)
        {
            MD5cx ctx1;
            ctx1.update((unsigned char*)pw, strlen(pw));
            ctx1.update((unsigned char*)sp, sl);
            ctx1.update((unsigned char*)pw, strlen(pw));
            ctx1.final(final);
        }
        for (int pl = strlen(pw); pl > 0; pl -= MD5_SIZE)
            ctx.update(final, pl > MD5_SIZE ? MD5_SIZE : pl);

        // Don't leave anything around in vm they could use
        memset(final, 0, sizeof(final));

        // Then something really weird...
        for (int i = strlen(pw); i ; i >>= 1) {
            if (i & 1)
                ctx.update(final, 1);
            else
                ctx.update((unsigned char*)pw, 1);
        }
        ctx.final(final);

        // and now, just to make sure things don't run too fast
        // On a 60 Mhz Pentium this takes 34 msec, so you would
        // need 30 seconds to build a 1000 entry dictionary...
        for (int i = 0; i < 1000; i++) {
            MD5cx ctx1;
            if (i & 1)
                ctx1.update((unsigned char*)pw, strlen(pw));
            else
                ctx1.update(final, MD5_SIZE);
            if (i % 3)
                ctx1.update((unsigned char*)sp, sl);
            if (i % 7)
                ctx1.update((unsigned char*)pw, strlen(pw));
            if (i & 1)
                ctx1.update(final, MD5_SIZE);
            else
                ctx1.update((unsigned char*)pw, strlen(pw));
            ctx1.final(final);
        }

        // Now make the output string
        char passwd[120];
        char *p = passwd;

        unsigned long l;
        l = (final[0] << 16) | (final[6] << 8) | final[12];
        _crypt_to64(p, l, 4); p += 4;
        l = (final[1] << 16) | (final[7] << 8) | final[13];
        _crypt_to64(p, l, 4); p += 4;
        l = (final[2] << 16) | (final[8] << 8) | final[14];
        _crypt_to64(p, l, 4); p += 4;
        l = (final[3] << 16) | (final[9] << 8) | final[15];
        _crypt_to64(p, l, 4); p += 4;
        l = (final[4] << 16) | (final[10] << 8) | final[5];
        _crypt_to64(p, l, 4); p += 4;
        l = final[11];
        _crypt_to64(p, l, 2); p += 2;
        *p = '\0';

        // Don't leave anything around in vm they could use
        memset(final, 0, sizeof(final));

        p = new char[strlen(passwd) + 1];
        strcpy(p, passwd);
        memset(passwd, 0, strlen(passwd));
        return (p);
    }
}

