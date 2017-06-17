
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
 * QtInterf Graphical Interface Library                                   *
 *                                                                        *
 *========================================================================*
 $Id: fontutil.h,v 2.16 2013/03/13 05:05:43 stevew Exp $
 *========================================================================*/

#ifndef FONTUTIL_H
#define FONTUTIL_H

// Number of font storage slots.  Index 0 is not used.
#define MAX_NUM_APP_FONTS 8

struct stringlist;

namespace ginterf { }
using namespace ginterf;

namespace ginterf
{
    extern bool string_is_xfd(const char*);

    // Font type: Pango, X11, QT, Win32
    enum GRfontType { GRfontP, GRfontX, GRfontQ, GRfontW };

    // Base class for application font repository.
    //
    struct GRfont
    {
        struct fnt_t
        {
            fnt_t(const char *l, const char *d, bool f, bool fo)
                {
                    label = l;
                    default_fontname = d;
                    fixed = f;
                    family_only = fo;
                }

            const char *label;
            const char *default_fontname;
            bool fixed;         // restricted to monospace fonts
            bool family_only;   // ignore slant, etc.
        };
        static fnt_t app_fonts[];
        static int num_app_fonts;

        virtual ~GRfont() { }

        virtual GRfontType getType() = 0;
        virtual void setName(const char*, int) = 0;
        virtual const char *getName(int) = 0;
        virtual char *getFamilyName(int) = 0;
        virtual bool getFont(void*, int) = 0;
        virtual void registerCallback(void*, int) = 0;
        virtual void unregisterCallback(void*, int) = 0;

        const char *getDefaultName(int);
        const char *getLabel(int);
        bool isFixed(int);
        bool isFamilyOnly(int);

        static void parse_freeform_font_string(const char*, char**,
            stringlist**, int*, int = 12);
    };


    // Parser for X font description names.
    //
    struct xfd_t
    {
        static bool is_xfd(const char*);

        xfd_t(const char*);
        ~xfd_t();
        void set_foundry(const char*);
        void set_family(const char*);
        void set_weight(const char*);
        void set_slant(const char*);
        void set_width(const char*);
        void set_pixsize(int);
        void set_pointsz(int);
        char *font_xfd(bool = false);
        char *font_freeform();
        char *family_xfd(int* = 0, bool = false);

        const char *get_foundry() { return (foundry); }
        const char *get_family() { return (family); }
        const char *get_weight() { return (weight); }
        const char *get_slant() { return (slant); }
        const char *get_width() { return (width); }
        const char *get_pixsize() { return (pixsize); }
        const char *get_pointsz() { return (pointsz); }
        const char *get_resx() { return (resol_x); }
        const char *get_resy() { return (resol_y); }

    private:
        char *foundry;
        char *family;
        char *weight;
        char *slant;
        char *width;
        char *style;
        char *pixsize;
        char *pointsz;
        char *resol_x;
        char *resol_y;
        char *spacing;
        char *avgwid;
        char *charset;
        char *encoding;
        bool was_xfd;
        bool was_fixed;
    };
}

extern GRfont &FC;

#endif

