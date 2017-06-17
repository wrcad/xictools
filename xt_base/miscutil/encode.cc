
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
 $Id: encode.cc,v 1.4 2009/10/06 06:22:03 stevew Exp $
 *========================================================================*/

/* (C) Copyright 1993,1994 by Carnegie Mellon University
 * All Rights Reserved.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without
 * fee, provided that the above copyright notice appear in all copies
 * and that both that copyright notice and this permission notice
 * appear in supporting documentation, and that the name of Carnegie
 * Mellon University not be used in advertising or publicity
 * pertaining to distribution of the software without specific,
 * written prior permission.  Carnegie Mellon University makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied
 * warranty.
 *
 * CARNEGIE MELLON UNIVERSITY DISCLAIMS ALL WARRANTIES WITH REGARD TO
 * THIS SOFTWARE, INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS, IN NO EVENT SHALL CARNEGIE MELLON UNIVERSITY BE LIABLE
 * FOR ANY SPECIAL, INDIRECT OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN
 * AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING
 * OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS
 * SOFTWARE.
 */

/*=======================================================================
 *
 * This was cobbled together from pieces of the mpack program for unix
 * SRW 11/15/99
 =======================================================================*/

#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#ifndef WIN32
#include <netdb.h>
#else
#include <process.h>
#include <winsock2.h>
#endif
#include <time.h>
#include "encode.h"


namespace {
    const char *magic_look(FILE*);
    char *os_genid();
    char *md5digest(FILE*, long*);
    int to64(FILE*, FILE*, long);
    char *strsave(char*);

    const char *boundary = "--------------44380BBD5B9D64F6C72082A1";
}

#define NUMREFERENCES 4

// Encode a file into one or more MIME messages, each no larger than
// 'maxsize'.  A 'maxsize' of zero means no size limit.  pair
//
int
encode(struct MimeState *state, FILE *infile, const char *fname,
    FILE *descfile, const char *subject, const char *headers,
    long maxsize, const char *typeoverride)
{
    int wrotefiletype = 0;
    char buf[1024];

    // Clean up fname for printing
    const char *cleanfname = fname, *p;
    if ((p = strrchr(cleanfname, '/')) != 0)
        cleanfname = p+1;
    if ((p = strrchr(cleanfname, '\\')) != 0)
        cleanfname = p+1;
    if ((p = strrchr(cleanfname, ':')) != 0)
        cleanfname = p+1;

    // Find file type
    const char *type = typeoverride ? typeoverride : magic_look(infile);
    bool text = false;
    if (!strncmp(type, "text", 4))
        text = true;

    // Compute MD5 digests
    long filesize;
    char *digest = md5digest(infile, &filesize);

    // See if we have to do multipart
    int numparts = 1;
    if (maxsize) {
        if (!text)
            filesize = (filesize / 54) * 73; // Allow for base64 expansion

        // Add in size of desc file
        if (descfile) {
            long l;
            delete [] md5digest(descfile, &l);
            filesize += l;
        }
        if (filesize + state->written < maxsize)
            numparts = 1;
        else {
            filesize -= (maxsize - state->written);
            numparts = (filesize-1000)/maxsize + 2;
            if (numparts < 2)
                numparts = 2;
        }
    }

    char *multipartid = os_genid();
    char *referenceid[NUMREFERENCES];
    for (int i = 0; i < NUMREFERENCES; i++)
        referenceid[i] = 0;

    for (int thispart = 1; thispart <= numparts; thispart++) {

        if (!state->outfile) {
            state->outfile = fopen(state->outfname, "w");
            if (!state->outfile)
                perror(state->outfname);
            else {
                state->nfiles = 1;
                state->fnames = new char*[2];
                state->fnames[0] = strsave(state->outfname);
                state->fnames[1] = 0;
                state->written = 0;
            }
        }
        if (thispart > 1) {
            fclose(state->outfile);
            sprintf(buf, "%s.%02d", state->outfname, state->nfiles - 1);
            state->outfile = fopen(buf, "w");
            if (!state->outfile)
                perror(buf);
            else {
                state->nfiles++;
                char **t = new char*[state->nfiles + 1];
                memcpy(t, state->fnames, (state->nfiles - 1)*sizeof(char*));
                delete [] state->fnames;
                state->fnames = t;
                state->fnames[state->nfiles - 1] = strsave(buf);
                state->fnames[state->nfiles] = 0;
                state->written = 0;
            }
        }
        if (!state->outfile) {
            delete [] digest;
            delete [] multipartid;
            return (1);
        }

        char *msgid = os_genid();
        if (state->written == 0) {
            fprintf(state->outfile, "Message-ID: <%s>\n",
                numparts == 1 ? msgid : multipartid);
            fprintf(state->outfile, "MIME-Version: 1.0\n");
            if (headers)
                fputs(headers, state->outfile);
        }
        if (numparts > 1) {
            fprintf(state->outfile, "Subject: %s (%02d/%02d)\n", subject,
                thispart, numparts);
            if (thispart == 1)
                referenceid[0] = msgid;
            else {
                // Put out References: header pointing to previous parts
                fprintf(state->outfile, "References: <%s>\n", referenceid[0]);
                for (int i = 1; i < NUMREFERENCES; i++) {
                    if (referenceid[i])
                        fprintf(state->outfile, "\t <%s>\n", referenceid[i]);
                }
                for (int i = 2; i < NUMREFERENCES; i++)
                    referenceid[i-1] = referenceid[i];
                referenceid[NUMREFERENCES-1] = msgid;
            }
            fprintf(state->outfile,
                "Content-Type: message/partial; number=%d; total=%d;\n",
                thispart, numparts);
            fprintf(state->outfile, "\t id=\"%s\"\n", multipartid);
            fprintf(state->outfile, "\n");
        }

        if (thispart == 1) {
            if (state->written == 0) {
                fprintf(state->outfile, "Subject: %s\n", subject);
                fprintf(state->outfile,
                    "Content-Type: multipart/mixed;\n boundary=\"%s\"\n",
                    boundary + 2);
                fprintf(state->outfile, "\nThis is a MIME encoded message.\n");
                state->written += 300;

                // Spit out description section
                if (descfile) {
                    fprintf(state->outfile, "%s\n", boundary);
                    fprintf(state->outfile,
                        "Content-Type: text/plain; charset=us-ascii\n");
                    fprintf(state->outfile,
                        "Content-Transfer-Encoding: 7bit\n\n");
                    while (fgets(buf, sizeof(buf), descfile)) {
                        fputs(buf, state->outfile);
                        state->written += strlen(buf);
                    }
                    fprintf(state->outfile, "\n");
                }
                fprintf(state->outfile, "%s\n", boundary);
            }
        }

        if (!wrotefiletype++) {
            if (text) {
                fprintf(state->outfile,
                    "Content-Type: %s; charset=us-ascii;\n name=\"%s\"\n",
                    type, cleanfname);
                fprintf(state->outfile, "Content-Transfer-Encoding: 7bit\n");
            }
            else {
                fprintf(state->outfile, "Content-Type: %s; name=\"%s\"\n",
                    type, cleanfname);
                fprintf(state->outfile, "Content-Transfer-Encoding: base64\n");
            }
            fprintf(state->outfile,
                "Content-Disposition: inline;\n filename=\"%s\"\n",
                cleanfname);
            fprintf(state->outfile, "Content-MD5: %s\n\n", digest);
            state->written += 80;
        }

        if (state->written == maxsize)
            state->written--;
        // avoid a nasty fencepost error

        if (text) {
            while (fgets(buf, sizeof(buf), infile)) {
                fputs(buf, state->outfile);
                state->written += strlen(buf);
                if (maxsize && state->written >= maxsize)
                    break;
            }
        }
        else {
            state->written += to64(infile, state->outfile,
                (thispart == numparts) ? 0 : (maxsize - state->written));
        }
        if (thispart == numparts)
            fprintf(state->outfile, "%s\n", boundary);
    }

    delete [] digest;
    delete [] multipartid;
    return (0);
}


#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64
#endif

namespace {
    // Generate a message-id
    //
    char *
    os_genid()
    {
        static int pid = 0;
        static time_t curtime;
        static char hostname[MAXHOSTNAMELEN+1];

        if (pid == 0) {
            pid = getpid();
            time(&curtime);
            gethostname(hostname, sizeof(hostname));

            // If we don't have a FQDN, try canonicalizing with gethostbyname
            if (!strchr(hostname, '.')) {
                hostent *hp = gethostbyname(hostname);
                if (hp)
                    strcpy(hostname, hp->h_name);
            }
        }

        char *result = new char[25 + strlen(hostname)];
        sprintf(result, "%d.%d@%s", pid, (int)curtime++, hostname);
        return (result);
    }
}


// Description of the various file formats and their magic numbers
struct magic
{
    const char *name;   // Name of the file format
    const char *num;    // The magic number
    int len;            // Length of same (0 means strlen(magicnum))
};

namespace {
    // The magic numbers of the file formats we know about
    struct magic magic[] = {
        { "image/gif", "GIF", 0 },
        { "image/jpeg", "\377\330\377", 0 },
        { "video/mpeg", "\0\0\001\263", 4 },
        { "application/postscript", "%!", 0 },
    };
    int num_magic = (sizeof(magic)/sizeof(magic[0]));
    const char *default_type = "application/octet-stream";

    // The longest magic number
    int max_magiclen = 0;

    // Determins the format of the file "inputf".  The name
    // of the file format (or 0 on error) is returned.
    //
    const char *
    magic_look(FILE *infile)
    {
        if (max_magiclen == 0) {
            for (int i = 0; i < num_magic; i++) {
                if (magic[i].len == 0)
                    magic[i].len = strlen(magic[i].num);
                if (magic[i].len > max_magiclen)
                    max_magiclen = magic[i].len;
            }
        }

        char buf[512];
        int numread = fread(buf, 1, max_magiclen, infile);
        rewind(infile);

        for (int i = 0; i < num_magic; i++) {
            if (numread >= magic[i].len) {
                int j;
                for (j = 0; j < magic[i].len; j++)
                    if (buf[j] != magic[i].num[j])
                        break;
                if (j == magic[i].len)
                    return (magic[i].name);
            }
        }

        numread = fread(buf, 1, 512, infile);
        rewind(infile);

        for (int i = 0; i < numread; i++) {
            if (buf[i] & 0x80)
                return (default_type);
        }
        return ("text/plain");
    }


    char basis_64[] =
       "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

    void output64chunk(int, int, int, int, FILE*);


    int
    to64(FILE *infile, FILE *outfile, long limit)
    {
        if (limit && limit < 73)
            return (1);

        int ct = 0, written = 0;
        int c1;
        while ((c1 = getc(infile)) != EOF) {
            int c2 = getc(infile);
            if (c2 == EOF)
                output64chunk(c1, 0, 0, 2, outfile);
            else {
                int c3 = getc(infile);
                if (c3 == EOF)
                    output64chunk(c1, c2, 0, 1, outfile);
                else
                    output64chunk(c1, c2, c3, 0, outfile);
            }
            ct += 4;
            if (ct > 71) {
                putc('\n', outfile);
                if (limit) {
                    limit -= ct + 1;
                    if (limit < 73)
                        return (1);
                }
                written += 73;
                ct = 0;
            }
        }
        if (ct) {
            putc('\n', outfile);
            ct++;
        }
        return (written + ct);
    }


    void
    output64chunk(int c1, int c2, int c3, int pads, FILE *outfile)
    {
        putc(basis_64[c1>>2], outfile);
        putc(basis_64[((c1 & 0x3)<< 4) | ((c2 & 0xF0) >> 4)], outfile);
        if (pads == 2) {
            putc('=', outfile);
            putc('=', outfile);
        }
        else if (pads) {
            putc(basis_64[((c2 & 0xF) << 2) | ((c3 & 0xC0) >>6)], outfile);
            putc('=', outfile);
        }
        else {
            putc(basis_64[((c2 & 0xF) << 2) | ((c3 & 0xC0) >>6)], outfile);
            putc(basis_64[c3 & 0x3F], outfile);
        }
    }


    char *
    md5contextTo64(MD5cx *context)
    {
        unsigned char digest[18];
        context->final(digest);
        digest[sizeof(digest)-1] = digest[sizeof(digest)-2] = 0;

        char encodedDigest[25];
        char *p = encodedDigest;
        for (unsigned int i = 0; i < sizeof(digest); i += 3) {
            *p++ = basis_64[digest[i]>>2];
            *p++ = basis_64[((digest[i] & 0x3)<<4) |
                ((digest[i+1] & 0xF0)>>4)];
            *p++ = basis_64[((digest[i+1] & 0xF)<<2) |
                ((digest[i+2] & 0xC0)>>6)];
            *p++ = basis_64[digest[i+2] & 0x3F];
        }
        *p-- = '\0';
        *p-- = '=';
        *p-- = '=';
        return (strsave(encodedDigest));
    }


    char *
    md5digest(FILE *infile, long *len)
    {
        unsigned char buf[1000];
        long length = 0;
        MD5cx context;
        int nbytes;
        while ((nbytes = fread(buf, 1, sizeof(buf), infile)) != 0) {
            length += nbytes;
            context.update(buf, nbytes);
        }
        rewind(infile);
        if (len)
            *len = length;
        return (md5contextTo64(&context));
    }


    char *
    strsave(char *str)
    {
        if (!str)
            return (0);
        char *p = new char[strlen(str) + 1];
        strcpy(p, str);
        return (p);
    }
}

