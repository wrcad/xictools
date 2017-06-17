
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
 * $Id: imsave.h,v 1.3 2010/02/14 18:34:31 stevew Exp $
 *========================================================================*/


#define PAGE_SIZE_EXECUTIVE    0
#define PAGE_SIZE_LETTER       1
#define PAGE_SIZE_LEGAL        2
#define PAGE_SIZE_A4           3
#define PAGE_SIZE_A3           4
#define PAGE_SIZE_A5           5
#define PAGE_SIZE_FOLIO        6

enum ImErrType { ImOK, ImNoSupport, ImError };

struct SaveInfo
{
    SaveInfo() {
        quality = 208;
        scaling = 1024;
        xjustification = 512;
        yjustification = 512;
        page_size = PAGE_SIZE_LETTER;
        color = true;
    }

    int     quality;
    int     scaling;
    int     xjustification;
    int     yjustification;
    int     page_size;
    bool    color;
};

struct Image
{
    Image(int w, int h, unsigned char *d) {
        rgb_width = w;
        rgb_height = h;
        rgb_data = d;
    }
    ~Image() { delete [] rgb_data; }

    ImErrType save_image(const char*, SaveInfo*);

private:
    bool savePPM(const char*, SaveInfo*);
    bool savePS(const char*, SaveInfo*);
    bool saveJPEG(const char*, SaveInfo*);
    bool savePNG(const char*, SaveInfo*);
    bool saveTIFF(const char*, SaveInfo*);

    int             rgb_width, rgb_height;
    unsigned char   *rgb_data;

    static const char *image_string;
};

extern Image *create_image_from_drawable(void*, unsigned long, int, int, int,
    int);

