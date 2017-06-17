
/*========================================================================*
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
 *========================================================================*
 * $Id: ppm.cc,v 1.8 2010/02/14 18:34:31 stevew Exp $
 *========================================================================*/

#include "imsave.h"
#include <stdio.h>
#include <string.h>
#include <setjmp.h>


bool
Image::savePPM(const char *file, SaveInfo*)
{
    FILE *f = fopen(file, "wb");
    if (!f)
        return (false);

    const char *ext = strrchr(file, '.');
    if (!ext) {
        fclose(f);
        return (false);
    }
    ext++;
    if (strcmp(ext, "pgm") == 0){
        if (!fprintf(f, "P5\n# %s\n%i %i\n255\n", image_string,
                rgb_width, rgb_height)) {
            fclose(f);
            return (false);
        }
        unsigned char *ptr = rgb_data;
        for (int y = 0; y < rgb_height; y++) {
            for (int x = 0; x < rgb_width; x++) {
                int v = (int)(*ptr++);
                v += (int)(*ptr++);
                v += (int)(*ptr++);
                unsigned char val = (unsigned char)(v / 3);
                if (!fwrite(&val, 1, 1, f)) {
                    fclose(f);
                    return (false);
                }
            }
        }
    }
    else {
        if (!fprintf(f, "P6\n# %s\n%i %i\n255\n", image_string,
                rgb_width, rgb_height)) {
            fclose(f);
            return (false);
        }
        if (!fwrite(rgb_data, 1, (rgb_width * rgb_height * 3), f)) {
            fclose(f);
            return (false);
        }
    }
    fclose(f);
    return (true);
}

