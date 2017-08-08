
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "dsp.h"
#include "tech.h"
#include "errorlog.h"
#include "main_variables.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"
#include <time.h>


//
// Functions to read/write the default stipple patterns.
//

namespace {
    unsigned char c0[] = {0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa};
    unsigned char c1[] = {0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55, 0xaa, 0x55};
    unsigned char c2[] = {0x88, 0x44, 0x22, 0x11, 0x88, 0x44, 0x22, 0x11};
    unsigned char c3[] = {0x11, 0x22, 0x44, 0x88, 0x11, 0x22, 0x44, 0x88};
    unsigned char c4[] = {0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};
    unsigned char c5[] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80};
    unsigned char c6[] = {0x81, 0xc0, 0x60, 0x30, 0x18, 0xc, 0x6, 0x3};
    unsigned char c7[] = {0x81, 0x3, 0x6, 0xc, 0x18, 0x30, 0x60, 0xc0};
    unsigned char c8[] = {0xc1, 0xe0, 0x70, 0x38, 0x1c, 0xe, 0x7, 0x83};
    unsigned char c9[] = {0x83, 0x7, 0xe, 0x1c, 0x38, 0x70, 0xe0, 0xc1};
    unsigned char c10[] = {0xc3, 0xe1, 0xf0, 0x78, 0x3c, 0x1e, 0xf, 0x87};
    unsigned char c11[] = {0xc3, 0x87, 0xf, 0x1e, 0x3c, 0x78, 0xf0, 0xe1};
    unsigned short c12[] = {0x8000, 0x4000, 0x2000, 0x1000, 0x800, 0x400,
        0x200, 0x100, 0x80, 0x40, 0x20, 0x10, 0x8, 0x4, 0x2, 0x1};
    unsigned short c13[] = {0x1, 0x2, 0x4, 0x8, 0x10, 0x20, 0x40, 0x80,
        0x100, 0x200, 0x400, 0x800, 0x1000, 0x2000, 0x4000, 0x8000};
    unsigned short c14[] = {0x101, 0x8080, 0x4040, 0x2020, 0x1010, 0x808,
        0x404, 0x202, 0x101, 0x8080, 0x4040, 0x2020, 0x1010, 0x808, 0x404,
        0x202};
    unsigned short c15[] = {0x101, 0x202, 0x404, 0x808, 0x1010, 0x2020,
        0x4040, 0x8080, 0x101, 0x202, 0x404, 0x808, 0x1010, 0x2020, 0x4040,
        0x8080};
    unsigned char c16[] = {0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81};
    unsigned char c17[] = {0x81, 0x42, 0x3c, 0x24, 0x24, 0x3c, 0x42, 0x81};
    unsigned char c18[] = {0x0, 0x0, 0x3c, 0x24, 0x24, 0x3c, 0x0, 0x0};
    unsigned char c19[] = {0x0, 0x7e, 0x7e, 0x66, 0x66, 0x7e, 0x7e, 0x0};
    unsigned char c20[] = {0x0, 0x7e, 0x42, 0x42, 0x42, 0x42, 0x7e, 0x0};
    unsigned char c21[] = {0x0, 0x7e, 0x42, 0x5a, 0x5a, 0x42, 0x7e, 0x0};
    unsigned char c22[] = {0x81, 0x7e, 0x66, 0x42, 0x42, 0x66, 0x7e, 0x81};
    unsigned char c23[] = {0x0, 0x3c, 0x66, 0x42, 0x42, 0x66, 0x3c, 0x0};
    unsigned char c24[] = {0x0, 0x0, 0x18, 0x24, 0x24, 0x18, 0x0, 0x0};
    unsigned char c25[] = {0x81, 0x42, 0x18, 0x24, 0x24, 0x18, 0x42, 0x81};
    unsigned char c26[] = {0x0, 0x0, 0x18, 0x3c, 0x3c, 0x18, 0x0, 0x0};
    unsigned char c27[] = {0x0, 0x0, 0x0, 0x18, 0x18, 0x0, 0x0, 0x0};
    unsigned char c28[] = {0x0, 0x66, 0x66, 0x0, 0x0, 0x66, 0x66, 0x0};
    unsigned char c29[] = {0x0, 0x44, 0x22, 0x0, 0x0, 0x44, 0x22, 0x0};
    unsigned char c30[] = {0x0, 0x60, 0x6, 0x0, 0x0, 0x60, 0x6, 0x0};
    unsigned char c31[] = {0x0, 0x60, 0x0, 0x6, 0x0, 0x60, 0x0, 0x6};
    unsigned char c32[] = {0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1, 0x1};
    unsigned char c33[] = {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0xff};
    unsigned char c34[] = {0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11};
    unsigned char c35[] = {0x0, 0x0, 0x0, 0xff, 0x0, 0x0, 0x0, 0xff};
    unsigned char c36[] = {0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55, 0x55};
    unsigned char c37[] = {0x0, 0xff, 0x0, 0xff, 0x0, 0xff, 0x0, 0xff};
    unsigned char c38[] = {0x22, 0x22, 0xff, 0x22, 0x22, 0x22, 0xff, 0x22};
    unsigned char c39[] = {0x4, 0x4, 0x4, 0x4, 0x4, 0xff, 0x4, 0x4};
    unsigned char c40[] = {0x80, 0x80, 0x80, 0xf8, 0x8, 0x8, 0x8, 0x8f};
    unsigned char c41[] = {0x1, 0x1, 0x1, 0x1f, 0x10, 0x10, 0x10, 0xf1};
    unsigned char c42[] = {0x0, 0x0, 0x0, 0x10, 0x28, 0x44, 0x82, 0x1};
    unsigned char c43[] = {0x2, 0x4, 0x8, 0x10, 0x8, 0x4, 0x2, 0x1};
    unsigned char c44[] = {0x12, 0x21, 0x21, 0x21, 0x21, 0x12, 0x12, 0x12};
    unsigned char c45[] = {0xc3, 0x3c, 0x0, 0x0, 0x3c, 0xc3, 0x0, 0x0};
    unsigned char c46[] = {0x0, 0x11, 0x0, 0x44, 0x0, 0x11, 0x0, 0x44};
    unsigned char c47[] = {0x50, 0x22, 0x50, 0x0, 0x5, 0x22, 0x5, 0x0};
    unsigned char c48[] = {0x91, 0x18, 0x24, 0x43, 0xc2, 0x24, 0x18, 0x89};
    unsigned char c49[] = {0x11, 0x12, 0x24, 0xc8, 0x13, 0x24, 0x48, 0x88};
    unsigned char c50[] = {0x42, 0x81, 0x3c, 0x42, 0x42, 0x3c, 0x81, 0x42};
    unsigned char c51[] = {0x90, 0x48, 0x28, 0x91, 0x62, 0x2, 0xe, 0x11};
    unsigned char c52[] = {0x81, 0x42, 0x24, 0x24, 0x0, 0x3c, 0x42, 0x81};
    unsigned char c53[] = {0x89, 0x4a, 0x26, 0x10, 0x8, 0x64, 0x52, 0x91};
    unsigned char c54[] = {0x44, 0x28, 0x93, 0x82, 0x82, 0x93, 0x28, 0x44};
    unsigned char c55[] = {0x81, 0x42, 0x24, 0x42, 0x99, 0x24, 0x42, 0x81};
    unsigned char c56[] = {0x2a, 0x29, 0xc4, 0x3, 0xc0, 0x23, 0x94, 0x54};
    unsigned char c57[] = {0x8, 0x78, 0x40, 0x40, 0xcf, 0x8, 0x8, 0x8};
    unsigned char c58[] = {0x0, 0x42, 0x0, 0x24, 0x0, 0x42, 0x0, 0x0};
    unsigned char c59[] = {0x88, 0x44, 0x2, 0x11, 0x88, 0x40, 0x22, 0x11};
    unsigned char c60[] = {0x3e, 0x40, 0x9c, 0xa2, 0xaa, 0xa2, 0x9c, 0x1};
    unsigned char c61[] = {0x1, 0x39, 0x27, 0x20, 0x20, 0x27, 0x39, 0x1};
    unsigned char c62[] = {0x55, 0x0, 0x82, 0x10, 0x82, 0x0, 0x55, 0x0};
    unsigned char c63[] = {0x0, 0x40, 0x3e, 0x15, 0x14, 0x14, 0x32, 0x0};


    // Read a logoical line, accepting backslash continuation.
    //
    int read_line(FILE *fp, char *buf, int bufsz, int *plcnt)
    {
        int bytesread = 0;
        char *bp = buf;
        for (;;) {
            bool contd = false;
            char *s = fgets(bp, bufsz - bytesread, fp);
            if (!s) {
                buf[bytesread] = 0;
                return (bytesread);
            }
            (*plcnt)++;

            // Ignore comment lines.
            char *t = s;
            while (isspace(*t))
                t++;
            if (*t == '#' || (*t == '/' && *(t+1) == '/'))
                continue;

            // Strip out Microsoft line termination.
            while (*bp) {
                if (*bp == '\\') {
                    if (bp[1] == '\r' && bp[2] == '\n')
                        bp[1] = '\n';
                    if (bp[1] == '\n') {
                        *bp = 0;
                        contd = true;
                        break;
                    }
                }
                bytesread++;
                if (bytesread == bufsz) {
                    Errs()->add_error("line too long!");
                    return (-1);
                }
                if (*bp == '\r' && bp[1] == '\n')
                    *bp = '\n';
                if (*bp == '\n')
                    break;
                bp++;
            }
            if (contd)
                continue;
            break;
        }
        return (bytesread);
    }
}


// Read the default stipple map file, done once at program startup.
//
void
cTech::ReadDefaultStipples()
{
    char *stfile;
    const char *libpath = CDvdb()->getVariable(VA_LibPath);
    FILE *fp = pathlist::open_path_file(TECH_STIPPLE_FILE, libpath, "r",
        &stfile, true);
    if (!fp) {
        tc_default_maps[0] = c0;
        tc_default_maps[1] = c1;
        tc_default_maps[2] = c2;
        tc_default_maps[3] = c3;
        tc_default_maps[4] = c4;
        tc_default_maps[5] = c5;
        tc_default_maps[6] = c6;
        tc_default_maps[7] = c7;
        tc_default_maps[8] = c8;
        tc_default_maps[9] = c9;
        tc_default_maps[10] = c10;
        tc_default_maps[11] = c11;
        tc_default_maps[12] = c12;
        tc_default_maps[13] = c13;
        tc_default_maps[14] = c14;
        tc_default_maps[15] = c15;
        tc_default_maps[16] = c16;
        tc_default_maps[17] = c17;
        tc_default_maps[18] = c18;
        tc_default_maps[19] = c19;
        tc_default_maps[20] = c20;
        tc_default_maps[21] = c21;
        tc_default_maps[22] = c22;
        tc_default_maps[23] = c23;
        tc_default_maps[24] = c24;
        tc_default_maps[25] = c25;
        tc_default_maps[26] = c26;
        tc_default_maps[27] = c27;
        tc_default_maps[28] = c28;
        tc_default_maps[29] = c29;
        tc_default_maps[30] = c30;
        tc_default_maps[31] = c31;
        tc_default_maps[32] = c32;
        tc_default_maps[33] = c33;
        tc_default_maps[34] = c34;
        tc_default_maps[35] = c35;
        tc_default_maps[36] = c36;
        tc_default_maps[37] = c37;
        tc_default_maps[38] = c38;
        tc_default_maps[39] = c39;
        tc_default_maps[40] = c40;
        tc_default_maps[41] = c41;
        tc_default_maps[42] = c42;
        tc_default_maps[43] = c43;
        tc_default_maps[44] = c44;
        tc_default_maps[45] = c45;
        tc_default_maps[46] = c46;
        tc_default_maps[47] = c47;
        tc_default_maps[48] = c48;
        tc_default_maps[49] = c49;
        tc_default_maps[50] = c50;
        tc_default_maps[51] = c51;
        tc_default_maps[52] = c52;
        tc_default_maps[53] = c53;
        tc_default_maps[54] = c54;
        tc_default_maps[55] = c55;
        tc_default_maps[56] = c56;
        tc_default_maps[57] = c57;
        tc_default_maps[58] = c58;
        tc_default_maps[59] = c59;
        tc_default_maps[60] = c60;
        tc_default_maps[61] = c61;
        tc_default_maps[62] = c62;
        tc_default_maps[63] = c63;

        return;
    }

    char buf[2048];
    int linecnt = 0;
    int bc;
    while ((bc = read_line(fp, buf, 2048, &linecnt)) != 0) {
        if (bc < 0) {
            Log()->WarningLogV(mh::Techfile, "Error in %s, at line %d: %s\n",
                stfile, linecnt, Errs()->get_error());
            break;
        }
        const char *s = buf;
        while (isspace(*s))
            s++;
        if (!*s || *s == '#' || (*s == '/' && *(s+1) == '/'))
            continue;
        char *tok = lstring::gettok(&s);
        if (!tok)
            continue;
        if (!isdigit(*tok)) {
            Log()->WarningLogV(mh::Techfile,
                "Syntax error in %s, line %d.\n", stfile, linecnt);
            continue;
        }
        int ix = atoi(tok);
        delete [] tok;
        if (ix < 0 || ix >= TECH_MAP_SIZE) {
            Log()->WarningLogV(mh::Techfile,
                "Bad index %d in %s, line %d.\n", ix, stfile, linecnt);
            continue;
        }
        while (isspace(*s))
            s++;
        if (!*s)
            continue;
        int nx, ny;
        unsigned char array[128];    // 32x32 max
        if (GetPmap(&s, array, &nx, &ny)) {
            int nb = ny*(nx/8);
            tc_default_maps[ix].nx = nx;
            tc_default_maps[ix].ny = ny;
            tc_default_maps[ix].map = new unsigned char[nb];
            memcpy(tc_default_maps[ix].map, array, nb);
        }
        else {
            Log()->WarningLogV(mh::Techfile,
                "Syntax error in %s, line %d.\n%s\n", stfile, linecnt,
                Errs()->get_error());
            continue;
        }
    }
    delete [] stfile;
    fclose(fp);
}


// Dump the default stipple map file into the current directory.
//
const char *
cTech::DumpDefaultStipples()
{
    const char *msg = "Could not open default stipple file.";
    if (!filestat::create_bak(TECH_STIPPLE_FILE))
        return (filestat::error_msg());
    FILE *fp = fopen(TECH_STIPPLE_FILE, "w");
    if (!fp)
        return (msg);
    fprintf(fp, "# Xic Default Fillpatterns\n");
    time_t now = time(0);;
    struct tm *tgmt = gmtime(&now);
    char *gm = lstring::copy(asctime(tgmt));
    char *s = strchr(gm, '\n');
    if (s)
        *s = 0;
    fprintf(fp, "# Created %.24s GMT\n", gm);
    delete [] gm;

    for (int i = 0; i < TECH_MAP_SIZE; i++) {
        sTpmap *p = &tc_default_maps[i];
        fprintf(fp, "%d \\\n", i);
        PrintPmap(fp, 0, p->map, p->nx, p->ny);
        putc('\n', fp);
    }
    fclose(fp);
    return (0);
}

