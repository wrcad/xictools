
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
 $Id: dsp_color.h,v 5.19 2017/04/16 20:28:09 stevew Exp $
 *========================================================================*/

#ifndef DSP_COLOR_H
#define DSP_COLOR_H


// GUI color mapping.  Values passed to GRpkgIf()->GetAttrColor.
//
#define GRattrColorNo       GRattrColorApp1
#define GRattrColorYes      GRattrColorApp2
#define GRattrColorHl1      GRattrColorApp3
#define GRattrColorHl2      GRattrColorApp4
#define GRattrColorHl3      GRattrColorApp5
#define GRattrColorHl4      GRattrColorApp6
#define GRattrColorDvBg     GRattrColorApp7
#define GRattrColorDvFg     GRattrColorApp8
#define GRattrColorDvHl     GRattrColorApp9
#define GRattrColorDvSl     GRattrColorApp10


// Indices into the color table array.
//
enum
{
    BackgroundColor,
    ElecBackgroundColor,
    PhysBackgroundColor,
    GhostColor,
    ElecGhostColor,
    PhysGhostColor,
    HighlightingColor,
    ElecHighlightingColor,
    PhysHighlightingColor,
    SelectColor1,
    ElecSelectColor1,
    PhysSelectColor1,
    SelectColor2,
    ElecSelectColor2,
    PhysSelectColor2,
    MarkerColor,
    ElecMarkerColor,
    PhysMarkerColor,
    InstanceBBColor,
    ElecInstanceBBColor,
    PhysInstanceBBColor,
    InstanceNameColor,
    ElecInstanceNameColor,
    PhysInstanceNameColor,
    InstanceSizeColor,
    CoarseGridColor,
    ElecCoarseGridColor,
    PhysCoarseGridColor,
    FineGridColor,
    ElecFineGridColor,
    PhysFineGridColor,

    PromptTextColor,
    PromptEditTextColor,
    PromptHighlightColor,
    PromptCursorColor,
    PromptBackgroundColor,
    PromptEditBackgColor,
    PromptEditFocusBackgColor,

    Color2,
    Color3,
    Color4,
    Color5,
    Color6,
    Color7,
    Color8,
    Color9,
    Color10,
    Color11,
    Color12,
    Color13,
    Color14,
    Color15,
    Color16,
    Color17,
    Color18,
    Color19,

    GUIcolorSel,
    GUIcolorNo,
    GUIcolorYes,
    GUIcolorHl1,
    GUIcolorHl2,
    GUIcolorHl3,
    GUIcolorHl4,
    GUIcolorDvBg,
    GUIcolorDvFg,
    GUIcolorDvHl,
    GUIcolorDvSl,

    ColorTableEnd
};

// Size of the Attribute Colors part.  These will be swapped while
// in Print mode.
#define AttrColorSize (int)PromptTextColor

// Index of start of GUI colors.
#define GUIcolorBase GUIcolorSel

// How to print default-valued colors (for tech file).  Either skip
// printing these, print commented, or print as normal.
//
enum CTPmode { CTPnoDef, CTPcmtDef, CTPall };


// These are the attributes that are darkened in context display.
//
inline bool has_dark(int p)
{
    switch (p) {
    case HighlightingColor:
    case MarkerColor:
    case InstanceBBColor:
    case InstanceNameColor:
    case InstanceSizeColor:
        return (true);
    default:
        break;
    }
    return (false);
}

struct sColorTab
{
    // The color table element for attribute colors.
    //
    struct sColorTabEnt
    {
        void *operator new(size_t, sColorTabEnt *p) { return (p); }

        sColorTabEnt()
            {
                te_name = 0;
                te_alii = 0;
                te_defclr = 0;
                te_initclr = 0;
                te_pixel = 0;
                te_dark_pixel = 0;
                te_attr_index = -1;
                te_red = 0;
                te_green = 0;
                te_blue = 0;
                te_set = false;
            }

        sColorTabEnt(sColorTab *tab, const char *n, stringlist *l,
            const char *d, unsigned long p, unsigned char r, unsigned char g,
            unsigned char b, bool s, int attr_ix = -1)
            {
                te_name = n;
                te_alii = l;
                set_defclr(tab, d);
                te_initclr = te_defclr;
                te_pixel = p;
                te_dark_pixel = 0;
                te_attr_index = attr_ix;
                te_red = r;
                te_green = g;
                te_blue = b;
                te_set = s;
            }

        void set(sColorTab *tab, int r, int g, int b, const char *clr)
            {
                te_red = r;
                te_green = g;
                te_blue = b;
                te_pixel = GRpkgIf()->NameColor(clr);
                set_defclr(tab, clr);
                te_set = true;
            }

        void load(sColorTabEnt *c)
            {
                te_red = c->te_red;
                te_green = c->te_green;
                te_blue = c->te_blue;
                te_pixel = c->te_pixel;
                te_defclr = c->te_defclr;
            }

        bool rgb_eq(sColorTabEnt *c)
            {
                return (te_red == c->te_red && te_green == c->te_green &&
                    te_blue == c->te_blue);
            }

        void alloc()
            {
                GRpkgIf()->AllocateColor(&te_pixel, te_red, te_green, te_blue);
            }

        void set_defclr(sColorTab *tab, const char *s)
            {
                te_defclr = tab ? tab->stringtab_add(s) : 0;
            }

        void set_rgb(int r, int g, int b)
            {
                te_defclr = 0;
                te_red = r;
                te_green = g;
                te_blue = b;
            }

        void get_rgb(int *r, int *g, int *b)
            {
                *r = te_red;
                *g = te_green;
                *b = te_blue;
            }

        const char *keyword()       { return (te_name); }
        stringlist *aliases()       { return (te_alii); }
        const char *get_defclr()    { return (te_defclr); }
        const char *get_initclr()   { return (te_initclr); }
        int pixel()                 { return (te_pixel); }
        int *pixel_addr()           { return (&te_pixel); }
        int dark_pixel()            { return (te_dark_pixel); }
        int *dark_pixel_addr()      { return (&te_dark_pixel); }
        int attr_index()            { return (te_attr_index); }

        void set_dark();
        void add_alias(const char*);
        void check_print(FILE*, CTPmode);
        void print(FILE*, CTPmode);

    private:
        const char *te_name;
        stringlist *te_alii;
        const char *te_defclr;      // from string_tab
        const char *te_initclr;     // from string tab

        int te_pixel;
        int te_dark_pixel;
        int te_attr_index;          // GUI attribute color index
        unsigned char te_red, te_green, te_blue;
        bool te_set;
    };

    sColorTab();

    void init();
    void alloc();
    bool is_colorname(const char*);
    bool set_color(const char*, const char*);
    bool set_index_color(int, const char*);
    sColorTabEnt *find(const char*);
    int find_index(const char*);
    const char *gui_color_name(int);
    int color(int);
    int color(int, DisplayMode);
    void get_rgb(int, int*, int*, int*);
    void get_rgb(int, DisplayMode, int*, int*, int*);
    void set_rgb(int, int, int, int);
    void set_rgb(int, DisplayMode, int, int, int);
    void set_dark(bool, DisplayMode);
    void dump(FILE*, CTPmode);

    sColorTabEnt *color_ent(int ix)
        {
            if (ix >= 0 && ix < ColorTableEnd)
                return (colors + ix);
            return (0);
        }

    const char *stringtab_add(const char *s)    { return (string_tab.add(s)); }

private:
    sColorTabEnt colors[ColorTableEnd];
    bool use_dark;
    strtab_t string_tab;
};


inline int
cDisplay::Color(int clr, DisplayMode mode)
{
    return (d_color_table->color(clr, mode));
}


inline int
cDisplay::Color(int clr)
{
    return (d_color_table->color(clr));
}

#endif

