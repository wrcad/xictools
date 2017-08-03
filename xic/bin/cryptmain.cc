
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
 * Xic Encryption/Decryption Support                                      *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include <stdlib.h>
#include <unistd.h>
#ifdef WIN32
#include <conio.h>
#else
#include <termios.h>
#endif
#include "crypt.h"

static void get_passwd(const char*, char*);

//
// Main function for wrencode/wrdecode utilities.
//
// wrencode file [file ...]
//   This will encrypt each file on the command line.  The user is prompted
//   for a password, from which the encryption key is obtained.
//
// wrdecode file [file ...]
//   This will decrypt each file on the command line which was previously
//   encrypted with wrencode, if the correct password is given.
//
// wrsetpass path_to_xic
//   This will poke a new default password into the executable file
//   given.
//
int
main(int argc, char **argv)
{
    if (argc == 1 || argv[1][0] == '?' || (argv[1][0] == '-' &&
            (argv[1][1] == '?' || argv[1][1] == 'h'))) {
#ifdef SETPASS
        fprintf(stderr,
            "\n"
            "Whiteley Research Inc. wrsetpass utility (c) 2002.\n"
            "Set the default password in supported executable file.\n"
            "usage:  wrsetpass path_to_executable\n\n");
#else
#ifdef ENCODING
        fprintf(stderr,
            "\n"
            "Whiteley Research Inc. wrencode utility (c) 2002.\n"
            "File encryption utility, use with wrdecode.\n"
            "usage:  wrencode file [files ...]\n\n");
        exit(1);
#else
        fprintf(stderr,
            "\n"
            "Whiteley Research Inc. wrdecode utility (c) 2002.\n"
            "File decryption utility, use with wrencode.\n"
            "usage:  wrdecode file [files ...]\n\n");
#endif
#endif
        exit(1);
    }

    char pwbuf[32];
    get_passwd("Enter password: ", pwbuf);
#ifdef ENCODING
    char tpw[32];
    strcpy(tpw, pwbuf);
    get_passwd("Reenter password: ", pwbuf);
    if (strcmp(tpw, pwbuf)) {
        fprintf(stderr, "Passwords don't match, aborting.\n");
        exit(1);
    }
#endif

    sCrypt cr;
    const char *err = cr.getkey(pwbuf);
    memset(pwbuf, '*', 32);
    if (err) {
        fprintf(stderr, "Error: %s.\n", err);
        exit (1);
    }

#ifdef SETPASS
    FILE *xfp = fopen(argv[1], "rb+");
    if (!xfp) {
        fprintf(stderr, "%s: can't open.\n", argv[1]);
        exit(1);
    }
    // hunt for the magic characters at start of password string
    int c;
    while ((c = getc(xfp)) != EOF) {
        if (c != '`')
            continue;
        c = getc(xfp);
        if (c != '$')
            continue;
        c = getc(xfp);
        if (c != 'Y')
            continue;
        c = getc(xfp);
        if (c != '%')
            continue;
        c = getc(xfp);
        if (c != 'Z')
            continue;
        c = getc(xfp);
        if (c != '~')
            continue;
        c = getc(xfp);
        if (c != '@')
            continue;
        char kbuf[16];
        cr.readkey(kbuf);
        long pos = ftell(xfp);
        fseek(xfp, pos, SEEK_SET);
        if (fwrite(kbuf, 1, 13, xfp) != 13) {
            fprintf(stderr, "ERROR: write failed.\n");
            exit(1);
        }
        fflush(xfp);
        fclose(xfp);
        fprintf(stderr,
            "Default password successfully changed in %s.\n", argv[1]);
        return (0);
    }
    fclose(xfp);
    fprintf(stderr, "ERROR: Password not found in binary file!\n");
    return (1);
#endif

    for (int i = 1; i < argc; i++) {
        cr.initialize();
        if (access(argv[i], W_OK)) {
            fprintf(stderr, "%s: not found or no access.\n", argv[i]);
            continue;
        }
        FILE *fp = fopen(argv[i], "rb");
        if (!fp) {
            fprintf(stderr, "%s: can't open.\n", argv[i]);
            continue;
        }
        char *tmpf = new char[strlen(argv[i]) + 10];
        strcpy(tmpf, argv[i]);
        strcat(tmpf, ".wr-tmp");
        FILE *op = fopen(tmpf, "wb");
        if (!op) {
            fprintf(stderr, "%s: can't write temporary file.\n", argv[i]);
            delete [] tmpf;
            continue;
        }

        unsigned char buffer[CRYPT_WORKSPACE];
        bool nogo = false;

#ifdef ENCODING
        size_t n = fread(buffer, 1, CRYPT_WORKSPACE, fp);
        if (n == 0) {
            fprintf(stderr, "%s: read failed.\n", argv[i]);
            nogo = true;
        }
        else if (!cr.begin_encryption(op, &err, buffer, n)) {
            fprintf(stderr, "%s: %s.\n", argv[i], err);
            nogo = true;
        }
#else
        if (!cr.is_encrypted(fp)) {
            fprintf(stderr, "%s: not encrypted.\n", argv[i]);
            nogo = true;
        }
        else if (!cr.begin_decryption(fp, &err)) {
            fprintf(stderr, "%s: %s.\n", argv[i], err);
            nogo = true;
        }
#endif
        if (!nogo) {
            for (;;) {
                size_t nchars = fread(buffer, 1, CRYPT_WORKSPACE, fp);
                if (nchars == 0)
                    break;
                cr.translate(buffer, nchars);
                if (fwrite(buffer, 1, nchars, op) != nchars) {
                    fprintf(stderr, "%s: write failed.\n", argv[i]);
                    nogo = true;
                    break;
                }
            }
        }
        fclose(op);
        fclose(fp);
        if (!nogo) {
            unlink(argv[i]);
#ifdef WIN32
            rename(tmpf, argv[i]);
#else
            link(tmpf, argv[i]);
#endif
        }
        unlink(tmpf);
        delete [] tmpf;
        if (argc == 2)
            return (nogo);
    }
    return (0);
}


// Get a password from the user, sort of like the C getpass() function,
// but echo '*' for each character.  Limit to 16 chars.
//
static void
get_passwd(const char *prompt, char *pwbuf)
{
#ifndef WIN32
    struct termios oterm;
    tcgetattr(fileno(stdin), &oterm);
    struct termios term = oterm;

    term.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL);
    term.c_oflag |=  (OPOST|ONLCR);
    term.c_cc[VMIN] = 1;
    term.c_cc[VTIME] = 0;
    tcsetattr(fileno(stdin), TCSADRAIN, &term);
#endif
    fputs(prompt, stdout);
    fflush(stdout);

    int pos = 0;
    for (;;) {
#ifdef WIN32
        int c = getch();
#else
        int c = getc(stdin);
#endif
        if (c == '\n' || c == '\r')
            break;
        if (c == '\b') {
            if (pos) {
                pos--;
                putc(c, stdout);
                putc(' ', stdout);
                putc(c, stdout);
                fflush(stdout);
            }
        }
        else {
            if (pos < 16) {
                pwbuf[pos++] = c;
                putc('*', stdout);
                fflush(stdout);
            }
        }
    }
    pwbuf[pos] = 0;
    putc('\n', stdout);
#ifndef WIN32
    tcsetattr(fileno(stdin), TCSAFLUSH, &oterm);
#endif
}

