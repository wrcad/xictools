
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
 * Ginterf Graphical Interface Library                                    *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"
#include "graphics.h"
#ifdef HAVE_MOZY
#include "imsave/imsave.h"
#endif


//-----------------------------------------------------------------------------
// GRimage functions

bool
GRimage::create_image_file(GRpkg *pkg, const char *filename)
{
    if (!im_width || !im_height || !im_data)
        return (false);
    if (!pkg || !filename)
        return (false);

#ifdef HAVE_MOZY
    unsigned char *rgbdata = new unsigned char[im_width*im_height * 3];
    Image img(im_width, im_height, rgbdata);
    // rgbdata freed im Image destructor.

    for (unsigned int i = 0; i < im_height; i++) {
        for (unsigned int j = 0; j < im_width; j++) {
            int r, g, b;
            pkg->RGBofPixel(im_data[j + i*im_width], &r, &g, &b);
            *rgbdata++ = r;
            *rgbdata++ = g;
            *rgbdata++ = b;
        }
    }
    SaveInfo sv;
    return (img.save_image(filename, &sv) == ImOK);
#else
    return (false);
#endif
}

