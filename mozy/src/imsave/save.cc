
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
 * IMSAVE Image Dump Facility
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

/*------------------------------------------------------------------------*
 *
 * IMSAVE -- Screen Dump Utility
 *
 * S. R. Whiteley (stevew@wrcad.com)
 *------------------------------------------------------------------------*
 * Borrowed extensively from Imlib-1.9.8
 *
 * This software is Copyright (C) 1998 By The Rasterman (Carsten
 * Haitzler).  I accept no responsability for anything this software
 * may or may not do to your system - you use it completely at your
 * own risk.  This software comes under the LGPL and GPL licences.
 * The library itself is LGPL (all software in Imlib and gdk_imlib
 * directories) see the COPYING and COPYING.LIB files for full legal
 * details.
 *
 * (Rasterdude :-) <raster@redhat.com>)
 *------------------------------------------------------------------------*/

#include "imsave.h"
#include <stdio.h>
#include <string.h>


const char *Image::image_string = "Whiteley Research Inc. XicTools Image";

#ifndef WIN32
namespace {
    FILE *open_helper(const char*, const char*, const char*);
    int close_helper(FILE*);
}
#endif


ImErrType
Image::save_image(const char *file, SaveInfo *info)
{
    if (!file)
        return (ImError);

    SaveInfo defaults;
    if (!info)
        info = &defaults;
    const char *ext = strrchr(file, '.');
    if (!ext)
        ext = "";
    else
        ext++;

    // local conversions
    if (!strcasecmp(ext, "ppm") || !strcasecmp(ext, "pnm") ||
            !strcasecmp (ext, "pgm"))
        return (savePPM(file, info) ? ImOK : ImError);
    else if (!strcasecmp(ext, "ps"))
        return (savePS(file, info) ? ImOK : ImError);
    else if (!strcasecmp(ext, "jpeg") || !strcasecmp(ext, "jpg"))
        return (saveJPEG(file, info) ? ImOK : ImError);
    else if (!strcasecmp(ext, "png"))
        return (savePNG(file, info) ? ImOK : ImError);
    else if (!strcasecmp(ext, "tiff") || !strcasecmp(ext, "tif"))
        return (saveTIFF(file, info) ? ImOK : ImError);

#ifndef WIN32
    // ImageMagick
    FILE *f = open_helper("%C/convert pnm:- %s", file, "w");
    if (f) {
        if (!fprintf(f, "P6\n# %s\n%i %i\n255\n", image_string,
                rgb_width, rgb_height)) {
            close_helper(f);
            return (ImError);
        }
        if (!fwrite(rgb_data, 1, (rgb_width * rgb_height * 3), f)) {
            close_helper(f);
            return (ImError);
        }
        if (close_helper(f))
            return (ImError);
        return (ImOK);
    }

    // Netpbm and cjpeg
    char cmd[10240];
    if (!strcasecmp(ext, "jpeg"))
        sprintf(cmd,
"%%H -quality %i -progressive -outfile %%s", 100 * info->quality / 256);
    else if (!strcasecmp(ext, "jpg"))
        sprintf(cmd,
"%%H -quality %i -progressive -outfile %%s", 100 * info->quality / 256);
    else if (!strcasecmp(ext, "bmp"))
        strcpy(cmd, "%Q %N/ppmtobmp > %s");
    else if (!strcasecmp(ext, "gif"))
        strcpy(cmd, "%Q %N/ppmtogif -interlace > %s");
    else if (!strcasecmp(ext, "ilbm"))
        strcpy(cmd, "%N/ppmtoilbm -24if -hires -lace -compress > %s");
    else if (!strcasecmp(ext, "ilb"))
        strcpy(cmd, "%N/ppmtoilbm -24if -hires -lace -compress > %s");
    else if (!strcasecmp(ext, "iff"))
        strcpy(cmd, "%N/ppmtoilbm -24if -hires -lace -compress > %s");
    else if (!strcasecmp(ext, "icr"))
        strcpy(cmd, "%N/ppmtoicr > %s");
    else if (!strcasecmp(ext, "map"))
        strcpy(cmd, "%N/ppmtomap > %s");
    else if (!strcasecmp(ext, "mit"))
        strcpy(cmd, "%N/ppmtomitsu -sharpness 4 > %s");
    else if (!strcasecmp(ext, "mitsu"))
        strcpy(cmd, "%N/ppmtomitsu -sharpness 4 > %s");
    else if (!strcasecmp(ext, "pcx"))
        strcpy(cmd, "%N/ppmtopcx -24bit -packed > %s");
    else if (!strcasecmp(ext, "pgm"))
        strcpy(cmd, "%N/ppmtopgm > %s");
    else if (!strcasecmp(ext, "pi1"))
        strcpy(cmd, "%N/ppmtopi1 > %s");
    else if (!strcasecmp(ext, "pic"))
        strcpy(cmd, "%Q %N/ppmtopict > %s");
    else if (!strcasecmp(ext, "pict"))
        strcpy(cmd, "%Q %N/ppmtopict > %s");
    else if (!strcasecmp(ext, "pj"))
        strcpy(cmd, "%N/ppmtopj > %s");
    else if (!strcasecmp(ext, "pjxl"))
        strcpy(cmd, "%N/ppmtopjxl > %s");
    else if (!strcasecmp(ext, "puz"))
        strcpy(cmd, "%N/ppmtopuzz > %s");
    else if (!strcasecmp(ext, "puzz"))
        strcpy(cmd, "%N/ppmtopuzz > %s");
    else if (!strcasecmp(ext, "rgb3"))
        strcpy(cmd, "%N/ppmtorgb3 > %s");
    else if (!strcasecmp(ext, "six"))
        strcpy(cmd, "%N/ppmtosixel > %s");
    else if (!strcasecmp(ext, "sixel"))
        strcpy(cmd, "%N/ppmtosizel > %s");
    else if (!strcasecmp(ext, "tga"))
        strcpy(cmd, "%N/ppmtotga -rgb > %s");
    else if (!strcasecmp(ext, "targa"))
        strcpy(cmd, "%N/ppmtotga -rgb > %s");
    else if (!strcasecmp(ext, "uil"))
        strcpy(cmd, "%N/ppmtouil > %s");
    else if (!strcasecmp(ext, "xpm"))
        strcpy(cmd, "%Q %N/ppmtoxpm > %s");
    else if (!strcasecmp(ext, "yuv"))
        strcpy(cmd, "%N/ppmtoyuv > %s");
    else if (!strcasecmp(ext, "png"))
        strcpy(cmd, "%N/pnmtopng > %s");
    else if (!strcasecmp(ext, "ps"))
        strcpy(cmd, "%N/pnmtops -center -scale 100 > %s");
    else if (!strcasecmp(ext, "rast"))
        strcpy(cmd, "%N/pnmtorast -rle > %s");
    else if (!strcasecmp(ext, "ras"))
        strcpy(cmd, "%N/pnmtorast -rle > %s");
    else if (!strcasecmp(ext, "sgi"))
        strcpy(cmd, "%N/pnmtosgi > %s");
    else if (!strcasecmp(ext, "sir"))
        strcpy(cmd, "%N/pnmtosir > %s");
    else if (!strcasecmp(ext, "tif"))
        strcpy(cmd, "%N/pnmtotiff -lzw > %s");
    else if (!strcasecmp(ext, "tiff"))
        strcpy(cmd, "%N/pnmtotiff -lzw > %s");
    else if (!strcasecmp(ext, "xwd"))
        strcpy(cmd, "%N/pnmtoxwd > %s");
    else
        ext = "";

    if (ext[0]) {
        f = open_helper(cmd, file, "wb");
        if (f) {
            if (!fprintf(f, "P6\n# %s\n%i %i\n255\n", image_string,
                    rgb_width, rgb_height)) {
                close_helper(f);
                return (ImError);
            }
            if (!fwrite(rgb_data, 1, (rgb_width * rgb_height * 3), f)) {
                close_helper(f);
                return (ImError);
            }
            if (close_helper(f))
                return (ImError);
            return (ImOK);
        }
    }
#endif
    return (ImNoSupport);
}

#ifndef WIN32

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>

#define CONVERT_PATH  0
#define NETPBM_PATH   1
#define CJPEG_PROG    2
#define DJPEG_PROG    3

#define DEF_PATH "/usr/bin:/usr/local/bin:/opt/local/bin:/usr/X11R6/bin"

namespace {
    char *convert_path;
    char *netpbm_path;
    char *cjpeg_prog;
    char *djpeg_prog;


    // Function to find paths needed below.
    //
    char *
    find_path(int num)
    {
        const char *prog = 0;
        switch (num) {
        case CONVERT_PATH:
            if (convert_path)
                return (convert_path);
            prog = "convert";
            break;
        case NETPBM_PATH:
            if (netpbm_path)
                return (netpbm_path);
            prog = "ppmtoxpm";
            break;
        case CJPEG_PROG:
            if (cjpeg_prog)
                return (cjpeg_prog);
            prog = "cjpeg";
            break;
        case DJPEG_PROG:
            if (djpeg_prog)
                return (djpeg_prog);
            prog = "djpeg";
            break;
        default:
            return ((char*)".");
        }

        char *path = getenv("IMSAVE_PATH");
        if (!path)
            path = (char*)DEF_PATH;

        char buf[256];
        char *p = path;
        while (p) {
            while (*p == ':')
                p++;
            strcpy(buf, p);
            char *e = strchr(buf, ':');
            if (!e)
                e = buf + strlen(buf);
            *e++ = '/';
            strcpy(e, prog);

            if (!access(buf, X_OK)) {
                switch (num) {
                case CONVERT_PATH:
                    *(e-1) = 0;
                    convert_path = strdup(buf);
                    return (convert_path);
                case NETPBM_PATH:
                    *(e-1) = 0;
                    netpbm_path = strdup(buf);
                    return (netpbm_path);
                case CJPEG_PROG:
                    cjpeg_prog = strdup(buf);
                    return (cjpeg_prog);
                case DJPEG_PROG:
                    djpeg_prog = strdup(buf);
                    return (djpeg_prog);
                }
            }
            p = strchr(p, ':');
        }
        switch (num) {
        case CONVERT_PATH:
            convert_path = (char*)".";
            return (convert_path);
        case NETPBM_PATH:
            netpbm_path = (char*)".";
            return (netpbm_path);
        case CJPEG_PROG:
            cjpeg_prog = (char*)"cjpeg";
            return (cjpeg_prog);
        case DJPEG_PROG:
            djpeg_prog = (char*)"djpeg";
            return (djpeg_prog);
        }
        return ((char*)".");
    }


    //
    //    Helper library
    //

    int  hpid;
    void (*oldpiper) (int); // actually sighandler_t but BSD uses sig_t.


    FILE *
    open_helper(const char *instring, const char *fn, const char *mode)
    {
        char *ofil = 0;
        int ofd = -1;
        char buf[256];
        static char *vec[16];
        char *p = strdup(instring);
        if (!p)
            return (0);

        if (strncmp(instring, "%Q", 2) == 0) {
            // Generate a quanting pipeline
            fprintf(stderr, "Not currently supported: install ImageMagick.\n");
            free(p);
            return (0);
        }
        //
        //    Ok split the instring on spaces and translate
        //      %C %N %F and %s
        //
        //      FIXME: We need to handle a format string that begins
        //      %Q to indicate an 8bit quant in the pipeline first.
        //

        char *pp = p;

        bool redir = false;
        int vn = 0;
        while (vn < 15) {
            while (*pp && isspace(*pp))
                pp++;
            char *ep = pp;
            while (*ep && !isspace(*ep))
                ep++;
            if (*pp == 0)
                break;
            // pp->ep is now the input string block
            if (*ep)
                *ep++ = 0;

            if (strcmp(pp, "%s") == 0) {
                if (redir) {
                    ofil = strdup(fn);
                    pp = ep;
                    continue;
                }
                else
                    vec[vn++] = strdup(fn);
            }
            else if (strncmp(pp, "%N/", 3) == 0) {
                strcpy(buf, find_path(NETPBM_PATH));
                strcat(buf, pp + 2);
                if ((vec[vn++] = strdup(buf)) == 0)
                    break;
            }
            else if (strncmp(pp, "%J", 3) == 0) {
                if ((vec[vn++] = strdup(find_path(DJPEG_PROG))) == 0)
                    break;
            }
            else if (strncmp(pp, "%H", 3) == 0) {
                if ((vec[vn++] = strdup(find_path(CJPEG_PROG))) == 0)
                    break;
            }
            else if (strncmp(pp, "%C/", 3) == 0) {
                strcpy(buf, find_path(CONVERT_PATH));
                strcat(buf, pp + 2);
                if ((vec[vn++] = strdup(buf)) == 0)
                    break;
            }
            else if (*pp == '>')
                redir = true;
            else if (strncmp(pp, ">%s", 3) == 0)
                ofil = strdup(fn);
            else {
                if ((vec[vn++] = strdup(pp)) == 0)
                    break;
            }
            pp = ep;
        }

        vec[vn] = 0;

        FILE *fp = 0;
        int pfd[2];
        if (pipe(pfd) == -1)
            goto oops;

        if (*mode == 'r')
            fp = fdopen(pfd[0], "r");
        else if (*mode == 'w')
            fp = fdopen(pfd[1], "w");
        if (!fp)
            goto oops;

        if (ofil)
            if ((ofd = open(ofil, (O_WRONLY | O_TRUNC | O_CREAT), 0644)) == -1)
                goto oops;

        int pid;
        switch (pid = fork()) {
        case -1:
            break;
        case 0:
            signal(SIGPIPE, SIG_DFL);
            if (*mode == 'r')
                dup2(pfd[1], 1);
            if (*mode == 'w') {
                dup2(pfd[0], 0);
                if (ofd != -1) {
                    dup2(ofd, 1);
                    close(1);
                }
            }
            close(pfd[0]);
            close(pfd[1]);
            execv(vec[0], vec);
            perror(vec[0]);
            //
            //    This MUST be _exit or we will hit the SIGPIPE
            //      handler in ways we don't want. We want our parent
            //      to flush the inherited file buffers not us.
            //
            _exit(1);
        default:
            hpid = pid;

            if (ofd != -1)
                close(ofd);
            if (*mode == 'r')
                close(pfd[1]);
            else
                close(pfd[0]);
        }

        for (vn = 0; vn < 16; vn++) {
            if (vec[vn])
                free(vec[vn]);
        }
        if (ofil)
            free(ofil);
        free(p);
        oldpiper = signal(SIGPIPE, SIG_IGN);
        return (fp);

    oops:
        if (ofd != -1)
            close(ofd);
        if (fp)
            fclose(fp);
        for (vn = 0; vn < 16; vn++) {
            if (vec[vn])
                free(vec[vn]);
        }
        if (ofil)
            free(ofil);
        free(p);
        return (0);
    }


    int
    close_helper(FILE *fp)
    {

        fclose(fp);
        signal(SIGPIPE, oldpiper);
        int info = 0;
        waitpid(hpid, &info, 0);
        if (WIFEXITED(info))
            return (WEXITSTATUS(info));
        return (1);
    }
}

#endif

