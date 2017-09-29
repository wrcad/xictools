
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

#ifndef GRAPHICS_H_INCLUDED
#define GRAPHICS_H_INCLUDED

#include <stdlib.h>
#include <string.h>

// These are the values that shouled be ORed together to configure
// the library.

// Screen graphics, give devNULL, or a combination of the others
// that are allowable.  The actual package used depends on the
// build.
#define         _devNULL_        0x0
#define         _devGTK_         0x1
#define         _devW32_         0x2
#define         _devQT_          0x4

// Printer drivers, any combination can be specified.
#define         _devMSP_         0x0010
#define         _devPS_          0x0020
#define         _devPSBM_        0x0040
#define         _devPSBC_        0x0080
#define         _devHP_          0x0100
#define         _devPCL_         0x0200
#define         _devXF_          0x0400
#define         _devX_           0x0800     // xdraw interface

// Turn on any supported graphics package.
#define GR_ALL_PKGS    0xf

// Turn on all supported drivers.
#define GR_ALL_DRIVERS 0xfff0

namespace ginterf { }
using namespace ginterf;

//-----------------------------------------------------------------------------
// Some basic types

namespace ginterf
{
    // Driver dependent pointer to a window or widget.
    typedef void* GRobject;

    // Type for x-y data storage passed to multiple object rendering
    // functions.  This is used to avoid short/int data conversions.
    // The static function set_short_data should be called in the
    // screen device driver constructor, to set the data size as
    // appropriate (short for X functions, int for everything else).
    //
    // Rules
    // 1. Hard-copy drivers are datum size independent, i.e., they
    //    don't use the block data returned from data().
    // 2. The Boxes function takes l,t,w,h values.  The w, h must be
    //    positive, the caller must ensure this.  Also, we add 1 to
    //    w, h (e.g., w = r - l + 1) so that the filled box occupies
    //    the same area as an ouline box.
    // 3. Screen drivers that require short data call set_short_data
    //    In the constructor.  In QT, different widgets can have
    //    different sizes (X vs GL) so you can NOT use GRmultiPt
    //    functions in both windows.
    //
    struct GRmultiPt
    {
        struct pt_s { short x, y; };
        struct pt_i { int x, y; };
        union pt_u { pt_s *sbase; pt_i *ibase; };

        GRmultiPt(int n)
            {
                if (xshort)
                    u.sbase = new pt_s[n];
                else
                    u.ibase = new pt_i[n];
                size = n;
                ptr = 0;
            }

        ~GRmultiPt()
            {
                if (xshort)
                    delete [] u.sbase;
                else
                    delete [] u.ibase;
            }

        void init(int n)
            {
                n += 10;
                if (n > size) {
                    if (xshort) {
                        delete [] u.sbase;
                        u.sbase = new pt_s[n];
                    }
                    else {
                        delete [] u.ibase;
                        u.ibase = new pt_i[n];
                    }
                    size = n;
                }
            }

        void assign(int ix, int x, int y)
            {
                if (xshort) {
                    u.sbase[ix].x = x;
                    u.sbase[ix].y = y;
                }
                else {
                    u.ibase[ix].x = x;
                    u.ibase[ix].y = y;
                }
            }

        void *data() { return (u.ibase); }

        pt_i at(int i) const
            {
                pt_i p;
                if (xshort) {
                    p.x = u.sbase[i].x;
                    p.y = u.sbase[i].y;
                }
                else {
                    p.x = u.ibase[i].x;
                    p.y = u.ibase[i].y;
                }
                return (p);
            }

        // iteration functions
        //
        void data_ptr_init() { ptr = u.ibase; }

        int data_ptr_x() const
            {
                if (xshort)
                    return (((pt_s*)ptr)->x);
                else
                    return (((pt_i*)ptr)->x);
            }

        int data_ptr_y() const
            {
                if (xshort)
                    return (((pt_s*)ptr)->y);
                else
                    return (((pt_i*)ptr)->y);
            }

        void data_ptr_inc()
            {
                if (xshort)
                    ptr = ((pt_s*)ptr + 1);
                else
                    ptr = ((pt_i*)ptr + 1);
            }

        static void set_short_data(bool set) { xshort = set; }
        static bool is_short_data() { return (xshort); }

    private:
        pt_u u;
        void *ptr;
        int size;

        static bool xshort;
    };


    // Type for fill pattern data, driver dependent.
    typedef void* GRfillData;

    // Type for fill pattern data.
    //
    // The convention is that if a pointer to a GRfillType is null,
    // empty fill is indicated.  If the struct exists but has a null
    // map, solid fill is indicated.
    //
    // The unsigned char pixel maps passed in have the form below,
    // where the lsb of the lowest-address byte is the upper-left
    // corner of the image.
    //
    //    L      M L      M
    //   |xxxxxxxx|xxxxxxxx| ..   (nx+7)/8 bytes per row)
    //   ...
    //   (ny rows)
    //
    // Internally, an unsigned int array is used for pixel storage, so
    // maps can be up to 32 pixels wide.  For each row, the lowest
    // address byte maps to the lowest address byte in the int.  Any
    // unused part of a row (the highest bits) are zeroed.
    //
    struct GRfillType
    {
        GRfillType()
            {
                ft_nx = ft_ny = 0;
                ft_map = 0;
                ft_xpixmap = 0;
            }

        GRfillType(int nx, int ny, const unsigned char *mp, GRfillData xp)
            {
                ft_ny = 0;
                ft_xpixmap = xp;
                newMap(nx, ny, mp);
            }

        ~GRfillType()
            {
                delete [] ft_map;
            }

        // Call this with the start coords of a scan line in the
        // passed addresses.  Then call getNextColPixel to get the data
        // for columns along the line, passing the same variables.
        //
        void initPixelScan(unsigned int *xp, unsigned int *yp) const
            {
                (*xp) %= ft_nx;
                (*yp) %= ft_ny;
            }

        // Function to get pixels in a scan line, called after
        // initPixelScan.  The pattern wraps.
        //
        bool getNextColPixel(unsigned int *xp, unsigned int y) const
            {
                int x = *xp;
                bool p = (ft_map[y] & (1 << x));
                if (++x == ft_nx)
                    x = 0;
                *xp = x;
                return (p);
            }

        // Call with the initial byte posistion and Y value at the
        // passed addresses.  Then call getNextColByte to get the
        // raster bytes along the row, passing the same variables.
        //
        void initByteScan(unsigned int *xp, unsigned int *yp) const
            {
                unsigned int x = *xp;
                *xp = (x << 3) % ft_nx;
                (*yp) %= ft_ny;
            }

        // Function to get bytes in a scan line, called after
        // initByteScan.  The pattern wraps.
        //
        unsigned char getNextColByte(unsigned int *xp, unsigned int y) const
            {
                unsigned int x = *xp;
                unsigned int m = ft_map[y];
                unsigned char c = 0;
                for (unsigned char mask = 0x80; mask; mask >>= 1) {
                    if (m & (1 << x))
                        c |= mask;
                    if (++x == ft_nx)
                        x = 0;
                }
                *xp = x;
                return (c);
            }

        // Destroy the current map and create a new one from the data
        // passed.
        //
        void newMap(unsigned int x, unsigned int y, const unsigned char *mp)
            {
                delete [] ft_map;
                ft_map = 0;
                ft_nx = 0;
                ft_ny = 0;
                if (x > 0 && x <= 32 && y > 0 && y <= 32 && mp) {
                    ft_map = new unsigned int[y];
                    ft_nx = x;
                    ft_ny = y;

                    int bpl = (ft_nx + 7) >> 3;
                    for (unsigned int i = 0; i < y; i++) {
                        unsigned int d = *mp++;
                        for (int j = 1; j < bpl; j++)
                            d |= (*mp++ << 8*j);
                        ft_map[i] = d;
                    }
                }
            }

        // Create a new unsigned char-based bitmap.
        //
        unsigned char *newBitmap() const
            {
                if (!ft_map)
                    return (0);
                int bpl = (ft_nx + 7) >> 3;
                unsigned char *ary = new unsigned char[ft_ny*bpl];
                unsigned char *a = ary;
                for (unsigned int i = 0; i < ft_ny; i++) {
                    unsigned int d = ft_map[i];
                    *a++ = d;
                    for (int j = 1; j < bpl; j++)
                        *a++ = d >> 8*j;
                }
                return (ary);
            }

        unsigned int nX()           const { return (ft_nx); }
        unsigned int nY()           const { return (ft_ny); }
        bool hasMap()               const { return (ft_map != 0); }

        // This is for convenience when using X or similar, we can
        // hang the pixmap here.  User must manage this!

        void setXpixmap(GRfillData f)     { ft_xpixmap = f; }
        GRfillData xPixmap()        const { return (ft_xpixmap); }

    private:
        unsigned short ft_nx;
        unsigned short ft_ny;
        unsigned int *ft_map;
        GRfillData ft_xpixmap;    // used by X only
    };

    // Type for linestyle data
    struct GRlineType
    {
        GRlineType() {
            for (unsigned int i = 0; i < sizeof(dashes); i++)
                dashes[i] = 0;
            offset = 0; length = 0; mask = 0;
        }

        unsigned char dashes[8];
        unsigned char offset;
        unsigned char length;
        int mask;
    };
}


//-----------------------------------------------------------------------------
// Application callback struct

namespace ginterf
{
    struct pix_list
    {
        pix_list()
            {
                pixel = 0;
                r = g = b = 0;
                next = 0;
            }

        pix_list(int p, int rr, int gg, int bb)
            {
                pixel = p;
                r = rr;
                g = gg;
                b = bb;
                next = 0;
            }

        static void destroy(pix_list *p)
            {
                while (p) {
                    pix_list *x = p;
                    p = p->next;
                    delete x;
                }
            }

        int pixel;
        int r, g, b;  // 0-255
        pix_list *next;
    };

    struct GRdraw;
    struct GRwbag;

    extern inline class GRappCalls *GRappIf();

    // Interface to the application, mostly for specialized driver
    // parameters.  The application must instantiate a subclass of
    // this struct, or the GRappCallStubs for convenience if these
    // features are not used.
    //
    class GRappCalls
    {
        static GRappCalls *ptr()
            {
                if (!instancePtr)
                    on_null_ptr();
                return (instancePtr);
            }

        static void on_null_ptr();

    public:
        friend inline GRappCalls *GRappIf() { return (GRappCalls::ptr()); }

        GRappCalls();
        virtual ~GRappCalls() { }

        virtual const char *GetPrintCmd()               = 0;
            // Return the default print command.  The characters "%s" will
            // be replaced by the file name, otherwise the file name is
            // appended.

        virtual char *ExpandHelpInput(char*)            = 0;
            // The argument is a keyword/url to the help system.  This
            // function can modify the text.  It either returns the
            // argument, or *FREEs* the argument and returns an
            // update.  This is for, e.g., replacing special tokens
            // with app-specific data, such as installation location.

        virtual bool ApplyHelpInput(const char*)        = 0;
            // Intercept keywords given to the help system, return
            // true if the keyword is handled.  This is for, e.g.,
            // performing an application operation in response to a
            // special token rather than displaying a new page.

        virtual pix_list *ListPixels()                  = 0;
            // Return a list of pixels that might be used in a rendering.

        virtual int PixelIndex(int)                     = 0;
            // Map a pixel value into a layer (pen) number.

        virtual int BackgroundPixel()                   = 0;
            // Return the pixel used in the current background.

        virtual int LineTypeIndex(const GRlineType*)    = 0;
            // Map a line style into a line style index.

        virtual int FillTypeIndex(const GRfillType*)    = 0;
            // Map a fill type into a fill pattern index.

        virtual int FillStyle(int, int, int* = 0, int* = 0) = 0;
            // Fill style for driver id and layer index, with options
            // used by the HP-GL plot driver.

        virtual bool DrawCallback(void*, GRdraw*, int, int, int,
            int, int, int)                              = 0;
            // Rendering callback for xdraw package.

        virtual void *SetupLayers(void*, GRdraw*,
            void*)                                      = 0;
            // Layer table manipulator for xdraw package.

        virtual bool MenuItemLocation(int, int*,
            int*)                                   = 0;
            // Windows-only menu location function, returns top-right
            // coordinate.

    private:
        static GRappCalls *instancePtr;
    };

    class GRappCallStubs : public GRappCalls
    {
    public:
        virtual ~GRappCallStubs() { }

        virtual const char *GetPrintCmd()               { return ("lpr -h "); }
        virtual char *ExpandHelpInput(char *s)          { return (s); }
        virtual bool ApplyHelpInput(const char*)        { return (false); }

        virtual pix_list *ListPixels()                  { return (0); }
        virtual int PixelIndex(int)                     { return (0); }
        virtual int BackgroundPixel()                   { return (0); }
        virtual int LineTypeIndex(const GRlineType*)    { return (0); }
        virtual int FillTypeIndex(const GRfillType*)    { return (0); }
        virtual int FillStyle(int, int, int* = 0, int* = 0)
                                                        { return (0); }
        virtual bool DrawCallback(void*, GRdraw*, int, int, int,
            int, int, int)                              { return (false); };
        virtual void *SetupLayers(void*, GRdraw*,
            void*)                                      { return (0); }
        virtual bool MenuItemLocation(int, int*, int*)  { return (false); }
    };
}


//-----------------------------------------------------------------------------
// Hard copy package

namespace ginterf
{
    // Flags for image orientation, if "best" allow rotation if this
    // improves fit to aspect ratio of page.
    //
    typedef int HCorientFlags;
    enum
    {
        HCportrait      = 0x0,
        HClandscape     = 0x1,
        HCbest          = 0x2
    };

    // Enum for legend on/off/none button.
    //
    enum HClegType { HClegOff, HClegOn, HClegNone };

    // Enum for the hcframe callback.
    //  HCframeCmd      run the command to allow user to set the frame
    //  HCframeIsOn     return frame active status
    //  HCframeOn       set frame active, return previous active status
    //  HCframeOff      set frame inactive, return previous active status
    //  HCframeGet      get frame box (window space), return active status
    //  HCframeGetV     get frame box (viewport space), return active status
    //  HCframeSet      set frame box (window space), return active status
    //
    enum HCframeMode { HCframeCmd, HCframeIsOn, HCframeOn, HCframeOff,
        HCframeGet, HCframeGetV, HCframeSet };

    // Structure to pass callbacks to device dependent hard copy
    // functions.
    //
    struct HCcb
    {
        // static, no constructor needed.

        // The following values are passed to the hardcopy function and
        // set default operation.  They are also filled in by
        // HCupdate().

        bool (*hcsetup)(bool, int, bool, GRdraw*);
        int (*hcgo)(HCorientFlags, HClegType, GRdraw*);
        bool (*hcframe)(HCframeMode, GRobject, int*, int*, int*, int*,
            GRdraw*);
        int format;          // initial format index
        int drvrmask;        // driver index values with the corresponding
                             // bit set are indicates driver disabled

        // The following are ignored when passed to the hardcopy function,
        // but are filled in by HCupdate().  (Actually, command is used if
        // there is no default).
        HClegType legend;       // HClegOff off, HClegOn on, HClegNone no btn
        HCorientFlags orient;   // portrait, landscape, or best
        int resolution;         // initial resolution index
        const char *command;    // initial print command
        bool tofile;            // true if to file
        const char *tofilename; // name of file
        double left;            // left margin in inches
        double top;             // top margin in inches
        double width;           // width in inches
        double height;          // height in inches
    };

    // Structure provided by driver expressing parameter limits.
    struct HClimits
    {
        HClimits(
            double nxo,
            double xxo,
            double nyo,
            double xyo,
            double nw,
            double xw,
            double nh,
            double xh,
            unsigned int f,
            const char **r) :
                minxoff(nxo),
                maxxoff(xxo),
                minyoff(nyo),
                maxyoff(xyo),
                minwidth(nw),
                maxwidth(xw),
                minheight(nh),
                maxheight(xh),
                flags(f),
                resols(r) { }

        double minxoff, maxxoff;
        double minyoff, maxyoff;
        double minwidth, maxwidth;
        double minheight, maxheight;
        unsigned flags;
        // bit set to indicate unused input
#define HCtopMargin      (1 << 0)
#define HClandsSwpYmarg  (1 << 1)
#define HCconfirmGo      (1 << 2)
#define HCdontCareXoff   (1 << 3)
#define HCdontCareYoff   (1 << 4)
#define HCdontCareWidth  (1 << 5)
#define HCdontCareHeight (1 << 6)
#define HCnoLandscape    (1 << 7)
#define HCnoBestOrient   (1 << 8)
#define HCnoCanRotate    (1 << 9)
#define HCnoAutoWid      (1 << 10)
#define HCnoAutoHei      (1 << 11)
#define HCfileOnly       (1 << 12)
#define HCfixedResol     (1 << 13)
        const char **resols;     // strings representing dpi choices
    };
    // FLAG BITS:
    // HCtopMargin        Set if y-offset is relative to top of page.
    // HClandsSwpYmarg    Set if y-offset reference (top or bottom) changes in
    //                     landscape mode (as for HP PCL).
    // HCconfirmGo        Issue a popup after 'go' to confirm (Versatec).
    // HCdontCare???      Driver doesn't use these values.
    // HCnoLandscape      No landscape mode.
    // HCnoBestOrient     No optional rotation.
    // HCnoCanRotate      Driver can't rotate (must do this externally).
    // HCnoAutoWid        No auto-width capability
    // HCnoAutoHei        No auto-height capability
    // HCfileOnly         Output to file only.
    // HCfixedResol       No choice of resolution.

    // Structure expressing parameter defaults.
    //
    struct HCdefaults
    {
        HCdefaults(
            double x,
            double y,
            double w,
            double h,
            const char *c,
            int r,
            HClegType l,
            HCorientFlags o) :
                defxoff(x),
                defyoff(y),
                defwidth(w),
                defheight(h),
                command(c),
                defresol(r),
                legend(l),
                orient(o) { }

        double defxoff, defyoff;
        double defwidth, defheight;
        const char *command;    // command string to print file
        int defresol;           // index into resols
        HClegType legend;       // HClegOff off, HClegOn on, HClegNone no btn
        HCorientFlags orient;   // portrait, landscape, or best

        static const char *default_print_cmd;
    };

    // Structure that defines the various formats used for hardcopy
    // generation.
    //
    struct HCdesc
    {
        HCdesc(
            const char *nm,
            const char *ds,
            const char *kw,
            const char *al,
            const char *ft,
            HClimits l,
            HCdefaults d,
            bool ld=false,
            bool lw=false) :
                drname(nm),
                descr(ds),
                keyword(kw),
                alias(al),
                fmtstring(ft),
                last_w(0.0),
                last_h(0.0),
                limits(l),
                defaults(d),
                line_draw(ld),
                line_width(lw) { }

        const char *drname;     // name of device driver
        const char *descr;      // description for menu
        const char *keyword;    // reference name
        const char *alias;      // an alternative name
        const char *fmtstring;  // format string for DDinit()
        double last_w;          // width backup
        double last_h;          // height backup
        HClimits limits;        // limits
        HCdefaults defaults;    // defaults
        bool line_draw;         // set for "line draw" drivers
        bool line_width;        // line width can be set in line_draw
    };

    // Structure to hold data parsed from hardcopy format string.
    //
    struct HCdata
    {
        HCdata()
            {
                xoff = yoff = 0.0;
                width = height = 0.0;
                linewidth = 0.0;
                filename = 0;
                resol = hctype = 0;
                doRLE = encode = landscape = false;
            }

        double xoff;
        double yoff;
        double width;
        double height;
        double linewidth;
        const char *filename;
        int resol;
        int hctype;
        bool encode;
        bool doRLE;
        bool landscape;
    };
}


//-----------------------------------------------------------------------------
// Graphics context

// defined in lstring.h
struct stringlist;

// Masks for downstate and upstate (same as X).
#define GR_SHIFT_MASK   1
#define GR_CONTROL_MASK 4
#define GR_ALT_MASK     8

// Values for dev_flags below:
// The (hard copy only) driver has a text renderer which can be used for
// label text.
#define GRhasOwnText 1

// Prototype for ghost drawing core.
typedef void (*GhostDrawFunc)(int, int, int, int, bool);

namespace ginterf
{
    class GRpkg;

    // Generic bitmap.  The shmid field is the SYSV SHM id of the
    // data storage, if any.  We need to send this to X when using
    // the SHM extension.
    //
    struct GRimage
    {
        GRimage(unsigned int w, unsigned int h, unsigned int *d,
            bool k = false, int smid = 0)
            {
                im_width = w;
                im_height = h;
                im_data = d;
                im_shmid = smid;
                im_keep_data = k;
            }

        ~GRimage()
            {
                if (!im_keep_data && !im_shmid)
                    delete [] im_data;
            }

        // If the data have external ownership (keep_data or shmid set),
        // replace the data with a local copy, and unset keep_data.
        //
        void set_own_data()
            {
                if (im_keep_data || im_shmid) {
                    unsigned int sz = im_width*im_height;
                    unsigned int *tmp = im_data;
                    im_data = sz ? new unsigned int[sz] : 0;
                    if (tmp && sz)
                        memcpy(im_data, tmp, sz*sizeof(unsigned int));
                    im_keep_data = false;
                    im_shmid = 0;
                }
            }

        bool create_image_file(GRpkg*, const char*);

        unsigned int width()        const { return (im_width); }
        unsigned int height()       const { return (im_height); }
        unsigned int *data()        const { return (im_data); }
        unsigned int shmid()        const { return (im_shmid); }

    private:
        unsigned int im_width;
        unsigned int im_height;
        unsigned int *im_data;
        int im_shmid;
        bool im_keep_data;
    };

    // Callback flags for editor functions.
    enum XEtype
    {
        XE_SAVE,        // save text to file
        XE_SOURCE,      // send text to application
        XE_LOAD,        // read text from file
        XE_HELP,        // pop-up help
        XE_QUIT         // edit editor
    };

    // Values for SetXOR() function.
    enum {GRxNone, GRxXor, GRxHlite, GRxUnhlite};

    // Returns from callback arg to PopUpEditString().
    enum ESret {ESTR_IGN, ESTR_DN, ESTR_DN_NODESEL};

    // Codes for locator.
    enum LWenum
    {
        LW_CENTER,      // centered
        LW_LL,          // align to lower left
        LW_LR,          // align to lower right
        LW_UL,          // align to upper left
        LW_UR,          // align to upper right
        LW_XYR,         // relative to parent shell
        LW_XYA          // relative to screen
    };

    // Struct passed to pop-ups to set initial position.
    //
    struct GRloc
    {
        GRloc(LWenum c = LW_CENTER, int x = 0, int y = 0)
            { code = c; xpos = x; ypos = y; }

        LWenum code;
        int xpos;
        int ypos;
    };

    // Arg passed to PopUp??? functions: pop down, pop_up, update.
    enum ShowMode {MODE_OFF, MODE_ON, MODE_UPD};

    // Argument to PopUpFileSelector.
    enum FsMode { fsSEL, fsDOWNLOAD, fsSAVE, fsOPEN };

    // Print pop-up mode, passed to PopUpPrint()
    //  HCgraphical:  used for graphics plotting
    //  HCtextPS:     PostScript and HTML printing
    //  HCtext:       plain text only
    //
    enum HCmode { HCgraphical, HCtextPS, HCtext };

    // Second arg to PopUpErr and others.
    //  STY_NORM:   plain text, default font
    //  STY_FIXED:  plain text, fixed font
    //  STY_HTML:   HTML text
    //
    enum STYtype { STY_NORM, STY_FIXED, STY_HTML };

    // The indx argument to PopUpFontSel specifies an index to a set
    // of font registers.
    //
    enum FNTindex
    {
        FNT_ANY    = 0,     // open index, no preassignment
        FNT_FIXED  = 1,     // fixed font for text pop-up windows
        FNT_PROP   = 2,     // proportional font for text pop-up windows
        FNT_SCREEN = 3,     // fixed font for main display areas
        FNT_EDITOR = 4,     // font for text editor
        FNT_MOZY   = 5,     // proportional base font for HTML viewer
        FNT_MOZY_FIXED = 6  // fixed base font for HTML viewer
    };

    // The following two structs are the base classes for exported
    // graphics - a drawing area and a collection of pop-ups.  These
    // will be used to derive the application classes.  Originally,
    // there was only one base class, the present scheme is more
    // flexible.  For example, a single derived class can inherit
    // both, or the classes can be kept separate which facilitates
    // switching the drawing class to a print device.  Obviously, in
    // many cases only one of these classes needs to be instantiated.

    // The Resolution method returns the driver resolution in use
    // relative to an assumed screen resolution.  This is so that
    // attribute thresholding, such as minimum pixel spacing for grid
    // display, can be made to match what is displayed on-screen.

#define GR_SCREEN_RESOL 100.0

    // Drawing area interface.
    //
    struct GRdraw
    {
        GRdraw() { dev_flags = 0; user_data = 0; }
        virtual ~GRdraw() { }

        void defineLinestyle(GRlineType*, int);
        void setDefaultLinestyle(int);
        void defineFillpattern(GRfillType*, int, int, const unsigned char*);

        // graphics primitives
        virtual unsigned long WindowID()                                = 0;
        virtual void Halt()                                             = 0;
        virtual void Clear()                                            = 0;
        virtual void ResetViewport(int, int)                            = 0;
        virtual void DefineViewport()                                   = 0;
        virtual void Dump(int)                                          = 0;
        virtual void Pixel(int, int)                                    = 0;
        virtual void Pixels(GRmultiPt*, int)                            = 0;
        virtual void Line(int, int, int, int)                           = 0;
        virtual void PolyLine(GRmultiPt*, int)                          = 0;
        virtual void Lines(GRmultiPt*, int)                             = 0;
        virtual void Box(int, int, int, int)                            = 0;
        virtual void Boxes(GRmultiPt*, int)                             = 0;
        virtual void Arc(int, int, int, int, double, double)            = 0;
        virtual void Polygon(GRmultiPt*, int)                           = 0;
        virtual void Zoid(int, int, int, int, int, int)                 = 0;
        virtual void Text(const char*, int, int, int, int = -1, int = -1) = 0;
        virtual void TextExtent(const char*, int*, int*)                = 0;
        virtual void SetGhost(GhostDrawFunc, int, int)                  = 0;
        virtual void ShowGhost(bool)                                    = 0;
        virtual void UndrawGhost(bool = false)                          = 0;
        virtual void DrawGhost(int, int)                                = 0;
        virtual void MovePointer(int, int, bool)                        = 0;
        virtual void QueryPointer(int*, int*, unsigned*)                = 0;
        virtual void DefineColor(int*, int, int, int)                   = 0;
        virtual void SetBackground(int)                                 = 0;
        virtual void SetWindowBackground(int)                           = 0;
        virtual void SetGhostColor(int)                                 = 0;
        virtual void SetColor(int)                                      = 0;
        virtual void DefineLinestyle(GRlineType*)                       = 0;
        virtual void SetLinestyle(const GRlineType*)                    = 0;
        virtual void DefineFillpattern(GRfillType*)                     = 0;
        virtual void SetFillpattern(const GRfillType*)                  = 0;
        virtual void Update()                                           = 0;
        virtual void Input(int*, int*, int*, int*)                      = 0;
        virtual void SetXOR(int)                                        = 0;
        virtual void ShowGlyph(int, int, int)                           = 0;
        virtual GRobject GetRegion(int, int, int, int)                  = 0;
        virtual void PutRegion(GRobject, int, int, int, int)            = 0;
        virtual void FreeRegion(GRobject)                               = 0;
        virtual void DisplayImage(const GRimage*, int, int, int, int)   = 0;
        virtual double Resolution()                                     = 0;

        unsigned int DevFlags()     { return (dev_flags); }
        void *UserData()            { return (user_data); }
        void SetUserData(void *d)   { user_data = d; };

    protected:
        unsigned int dev_flags;     // indicate some features of driver
        void *user_data;            // whatever (used in WRspice)
    };

    // Base class for hard copy drivers, provides dummy stubs for unused
    // methods.
    //
    struct HCdraw : public GRdraw
    {
        virtual ~HCdraw() { }

        // Subclass must implement:
        //   void Halt()
        //   void ResetViewport(int, int)
        //   void DefineViewport()
        //   void Dump(int)
        //   void Pixel(int, int)
        //   void Pixels(GRmultiPt*, int)
        //   void Line(int, int, int, int)
        //   void PolyLine(GRmultiPt*, int)
        //   void Lines(GRmultiPt*, int)
        //   void Box(int, int, int, int)
        //   void Boxes(GRmultiPt*, int)
        //   void Arc(int, int, int, int, double, double)
        //   void Polygon(GRmultiPt*, int)
        //   void Zoid(int, int, int, int, int, int)
        //   void Text(const char*, int, int, int, int = -1, int = -1)
        //   void TextExtent(const char*, int*, int*)
        //   void SetColor(int)
        //   void SetLinestyle(const GRlineType*)
        //   void SetFillpattern(const GRfillType*)
        //   double Resolution()

        unsigned long WindowID()                            { return (0); }
        void Clear()                                            { }

        void SetGhost(GhostDrawFunc, int, int)                  { }
        void ShowGhost(bool)                                    { }
        void UndrawGhost(bool = false)                          { }
        void DrawGhost(int, int)                                { }

        void MovePointer(int, int, bool)                        { }
        void QueryPointer(int*, int*, unsigned*)                { }

        void DefineColor(int*, int, int, int)                   { }
        void SetBackground(int)                                 { }
        void SetWindowBackground(int)                           { }
        void SetGhostColor(int)                                 { }
        void SetLevel(int)                                      { }
        void DefineLinestyle(GRlineType*)                       { }
        void DefineFillpattern(GRfillType*)                     { }

        void Update()                                           { }
        void Input(int*, int*, int*, int*)                      { }
        void SetXOR(int)                                        { }
        void ShowGlyph(int, int, int)                           { }
        GRobject GetRegion(int, int, int, int)              { return (0); }
        void PutRegion(GRobject, int, int, int, int)            { }
        void FreeRegion(GRobject)                               { }
    };

    struct GRpopup;

    // A simple monitor for keeping track of active pop-ups.
    //
    struct GRmonList
    {
        struct elt
            {
                elt(GRpopup *o, elt *n) { obj = o; next = n; }

                elt *next;
                GRpopup *obj;
            };

        GRmonList() { list = 0; }

        bool is_active(GRpopup *o)
            {
                for (elt *e = list; e; e = e->next) {
                    if (e->obj == o)
                        return (true);
                }
                return (false);
            }

        void add(GRpopup *o)
            {
                if (o && !is_active(o))
                    list = new elt(o, list);
            }

        void remove(GRpopup *o)
            {
                elt *ep = 0;
                for (elt *e = list; e; e = e->next) {
                    if (e->obj == o) {
                        if (ep)
                            ep->next = e->next;
                        else
                            list = e->next;
                        delete e;
                        return;
                    }
                    ep = e;
                }
            }

        GRpopup *first_object()
            {
                if (list)
                    return (list->obj);
                return (0);
            }

    protected:
        elt *list;
    };

    // Base class for generic pop-ups.
    //
    struct GRpopup
    {
        GRpopup()
            {
                p_caller = 0;
                p_caller_data = 0;
                p_usrptr = 0;
                p_parent = 0;
                p_no_desel = false;
            }

        virtual ~GRpopup() { }

        virtual void register_caller(GRobject c, bool = false,
                bool = false)                       { p_caller = c; }
            // Register initiating toggle button.
            // First bool: don't deselect caller on popdown.
            // Second bool: destroy popup when caller deselected.
            // Note true, false will do nothing.

        void set_no_desel(bool b)                   { p_no_desel = b; }
            // Set/inhibit caller desel on popdown.

        virtual void register_usrptr(void **u)      { p_usrptr = u; }
            // Register an arbitrary pointer address, which will
            // be zeroed when the widget is destroyed.

        virtual void set_visible(bool) = 0;
            // Show or hide the widget.

        virtual void popdown() = 0;
            // Destroy the widget.

    protected:
        GRobject p_caller;      // calling button
        void *p_caller_data;    // toolkit-specific, for intercepting
                                // caller press
        void **p_usrptr;        // user's memory loc for object
        GRwbag *p_parent;       // owning container
        bool p_no_desel;        // don't deselect caller on exit
    };

    // Generic affirmation pop-up.
    //
    struct GRaffirmPopup : public GRpopup
    {
        typedef void(*GRaffirmCallback)(bool, void*);

        GRaffirmPopup()
            {
                p_callback = 0;
                p_cb_arg = 0;
            }

        GRaffirmCallback register_callback(GRaffirmCallback cb)
            {
                GRaffirmCallback ctmp = p_callback;
                p_callback = cb;
                return (ctmp);
            }

    protected:
        GRaffirmCallback p_callback;
        void *p_cb_arg;
    };

    // Generic numerical entry pop-up.
    //
    struct GRnumPopup : public GRpopup
    {
        typedef void(*GRnumCallback)(double, bool, void*);

        GRnumPopup()
            {
                p_callback = 0;
                p_cb_arg = 0;
            }

        GRnumCallback register_callback(GRnumCallback cb)
            {
                GRnumCallback ctmp = p_callback;
                p_callback = cb;
                return (ctmp);
            }

    protected:
        GRnumCallback p_callback;
        void *p_cb_arg;
    };

    // Generic line editor pop-up.
    //
    struct GRledPopup : public GRpopup
    {
        typedef ESret(*GRledCallback)(const char*, void*);
        typedef void(*GRledQuitFunc)(bool);

        GRledPopup()
            {
                p_callback = 0;
                p_cancel = 0;
                p_cb_arg = 0;
            }

        GRledCallback register_callback(GRledCallback cb)
            {
                GRledCallback ctmp = p_callback;
                p_callback = cb;
                return (ctmp);
            }

        GRledQuitFunc register_quit_callback(GRledQuitFunc cb)
            {
                GRledQuitFunc ctmp = p_cancel;
                p_cancel = cb;
                return (ctmp);
            }

        virtual void update(const char*, const char*) = 0;

    protected:
        GRledCallback p_callback;
        GRledQuitFunc p_cancel;
        void *p_cb_arg;
    };

    // Generic simple message box.
    //
    struct GRmsgPopup : public GRpopup
    {
    };

    // Generic text panel.
    //
    struct GRtextPopup : public GRmsgPopup
    {
#define GRtextPopupHelp (void*)-1
        // The 2-button info window has a help button, when pressed it
        // calls the callback with (false, GRtextPopupHelp).

        typedef bool(*GRtextCallback)(bool, void*);

        GRtextPopup()
            {
                p_callback = 0;
                p_cb_arg = 0;
            }

        GRtextCallback register_callback(GRtextCallback cb)
            {
                GRtextCallback ctmp = p_callback;
                p_callback = cb;
                return (ctmp);
            }

        virtual bool get_btn2_state() = 0;
        virtual void set_btn2_state(bool) = 0;

    protected:
        GRtextCallback p_callback;
        void *p_cb_arg;
    };

    // Generic font selection pop-up.
    //
    struct GRfontPopup : public GRpopup
    {
        typedef void(*GRfontCallback)(const char*, const char*, void*);

        GRfontPopup()
            {
                p_callback = 0;
                p_cb_arg = 0;
            }

        GRfontCallback register_callback(GRfontCallback cb)
            {
                GRfontCallback ctmp = p_callback;
                p_callback = cb;
                return (ctmp);
            }

        virtual void set_font_name(const char*) = 0;
        virtual void update_label(const char*) = 0;

    protected:
        GRfontCallback p_callback;
        void *p_cb_arg;
    };

    // Generic list pop-up.
    //
    struct GRlistPopup : public GRpopup
    {
        typedef void(*GRlistCallback)(const char*, void*);

        GRlistPopup()
            {
                p_callback = 0;
                p_cb_arg = 0;
            }

        GRlistCallback register_callback(GRlistCallback cb)
            {
                GRlistCallback ctmp = p_callback;
                p_callback = cb;
                return (ctmp);
            }

        virtual void update(stringlist*, const char*, const char*) = 0;
        virtual void update(bool(*)(const char*)) = 0;
        virtual void unselect_all() = 0;

    protected:
        GRlistCallback p_callback;  // the callback
        void *p_cb_arg;             // callback arg;
    };

    // Multicolumn list pop-up.
    //
    struct GRmcolPopup : public GRpopup
    {
        typedef void(*GRmcolCallback)(const char*, void*);

        GRmcolPopup()
            {
                p_callback = 0;
                p_cb_arg = 0;
                p_no_dd = false;
            }

        GRmcolCallback register_callback(GRmcolCallback cb)
            {
                GRmcolCallback ctmp = p_callback;
                p_callback = cb;
                return (ctmp);
            }

        void set_no_dragdrop(bool b) { p_no_dd = b; }

        virtual void update(stringlist*, const char*) = 0;
        virtual char *get_selection() = 0;
        virtual void set_button_sens(int) = 0;

    protected:
        GRmcolCallback p_callback;  // the callback
        void *p_cb_arg;             // callback arg;
        bool p_no_dd;               // suppress drag source
    };

    // Generic file selection pop-up.
    //
    struct GRfilePopup : public GRpopup
    {
        typedef void(*GRfileCallback)(const char*, void*);
        typedef void(*GRfileCancelFunc)(GRfilePopup*, void*);
        typedef char*(*GRfilePathGetFunc)();
        typedef void(*GRfilePathSetFunc)(const char*);

        GRfilePopup()
            {
                p_callback = 0;
                p_cancel = 0;
                p_path_get = 0;
                p_path_set = 0;
                p_cb_arg = 0;
            }

        GRfileCallback register_callback(GRfileCallback cb)
            {
                GRfileCallback ctmp = p_callback;
                p_callback = cb;
                return (ctmp);
            }

        GRfileCancelFunc register_quit_callback(GRfileCancelFunc cb)
            {
                GRfileCancelFunc ctmp = p_cancel;
                p_cancel = cb;
                return (ctmp);
            }

        GRfilePathGetFunc register_get_callback(GRfilePathGetFunc cb)
            {
                GRfilePathGetFunc ctmp = p_path_get;
                p_path_get = cb;
                return (ctmp);
            }

        GRfilePathSetFunc register_set_callback(GRfilePathSetFunc cb)
            {
                GRfilePathSetFunc ctmp = p_path_set;
                p_path_set = cb;
                return (ctmp);
            }

        virtual char *get_selection() = 0;

    protected:
        GRfileCallback p_callback;      // the callback
        GRfileCancelFunc p_cancel;      // the cancel func
        GRfilePathGetFunc p_path_get;   // the path_get func
        GRfilePathSetFunc p_path_set;   // the path_set func
        void *p_cb_arg;                 // callback arg;
    };

    // Generic text editor/mail client pop-up.
    //
    struct GReditPopup : public GRpopup
    {
        typedef bool(*GReditCallback)(const char*, void*, XEtype);
        typedef void(*GReditCancelFunc)(GReditPopup*);

        GReditPopup()
            {
                p_callback = 0;
                p_cancel = 0;
                p_cb_arg = 0;
            }

        GReditCallback register_callback(GReditCallback cb)
            {
                GReditCallback ctmp = p_callback;
                p_callback = cb;
                return (ctmp);
            }

        GReditCancelFunc register_quit_callback(GReditCancelFunc cb)
            {
                GReditCancelFunc ctmp = p_cancel;
                p_cancel = cb;
                return (ctmp);
            }

        bool call_callback(const char *s, void *a, XEtype t)
            {
                if (p_callback)
                    return ((*p_callback)(s, a, t));
                return (false);
            }

    protected:
        GReditCallback p_callback;      // the callback
        GReditCancelFunc p_cancel;      // the cancel func
        void *p_cb_arg;                 // callback arg;
    };

    // A bag of pop-up interface widgets.
    //
    struct GRwbag
    {
        GRwbag() { create_top_level = false; }
        virtual ~GRwbag() { }

        virtual void Title(const char*, const char*)                    = 0;
        virtual void ClearPopups()                                      = 0;

        // editor
        virtual GReditPopup *PopUpTextEditor(const char*,
            bool(*)(const char*, void*, XEtype), void*, bool)           = 0;
        virtual GReditPopup *PopUpFileBrowser(const char*)              = 0;
        virtual GReditPopup *PopUpStringEditor(const char*,
            bool(*)(const char*, void*, XEtype), void*)                 = 0;
        virtual GReditPopup *PopUpMail(const char*, const char*,
            void(*)(GReditPopup*) = 0, GRloc = GRloc())                 = 0;

        // file selection
        virtual GRfilePopup *PopUpFileSelector(FsMode, GRloc,
            void(*)(const char*, void*),
            void(*)(GRfilePopup*, void*), void*, const char*)           = 0;

        // fonts
        virtual void PopUpFontSel(GRobject, GRloc, ShowMode,
            void(*)(const char*, const char*, void*),
            void*, int, const char** = 0, const char* = 0)              = 0;

        // hard copy
        virtual void PopUpPrint(GRobject, HCcb*, HCmode, GRdraw* = 0)   = 0;
        virtual void HCupdate(HCcb*, GRobject)                          = 0;
        virtual void HCsetFormat(int)                                   = 0;

        // help
        virtual bool PopUpHelp(const char*)                             = 0;

// Suggested per-page default maximum number of entries for list pop-up
// in "multicol" mode.
#define DEF_LIST_MAX_PER_PAGE 5000

        // generic list
        virtual GRlistPopup *PopUpList(stringlist*, const char*,
            const char*, void(*)(const char*, void*), void*, bool,
            bool)                                                       = 0;

        // multicolumn list
        virtual GRmcolPopup *PopUpMultiCol(stringlist*, const char*,
            void(*)(const char*, void*), void*, const char**,
            int = 0, bool = false)                                      = 0;

        // utilities
        virtual GRaffirmPopup *PopUpAffirm(GRobject, GRloc,
            const char*, void(*)(bool, void*), void*)                   = 0;
        virtual GRnumPopup *PopUpNumeric(GRobject, GRloc,
            const char*, double, double, double, double, int,
            void(*)(double, bool, void*), void*)                        = 0;
        virtual GRledPopup *PopUpEditString(GRobject, GRloc,
            const char*, const char*, ESret(*)(const char *, void*),
            void*, int, void(*)(bool), bool = false, const char* = 0)   = 0;
        virtual void PopUpInput(const char*, const char*,
            const char*, void(*)(const char*, void*), void*, int = 0)   = 0;
        virtual GRmsgPopup *PopUpMessage(const char*, bool,
            bool = false, bool = false, GRloc = GRloc())                = 0;
        virtual int PopUpWarn(ShowMode, const char*,
            STYtype = STY_NORM, GRloc = GRloc())                        = 0;
        virtual int PopUpErr(ShowMode, const char*,
            STYtype = STY_NORM, GRloc = GRloc())                        = 0;
        virtual GRtextPopup *PopUpErrText(const char*,
            STYtype = STY_NORM, GRloc = GRloc(LW_UL))                   = 0;
        virtual int PopUpInfo(ShowMode, const char*,
            STYtype = STY_NORM, GRloc = GRloc(LW_LL))                   = 0;
        virtual int PopUpInfo2(ShowMode, const char*,
            bool(*)(bool, void*), void*,
            STYtype = STY_NORM, GRloc = GRloc(LW_LL))                   = 0;
        virtual int PopUpHTMLinfo(ShowMode, const char*,
            GRloc = GRloc(LW_LL))                                       = 0;

        // These provide access to the data structs of the active
        // sub-widgets that have references in the bag.
        virtual GRledPopup *ActiveInput()                               = 0;
        virtual GRmsgPopup *ActiveMessage()                             = 0;
        virtual GRtextPopup *ActiveInfo()                               = 0;
        virtual GRtextPopup *ActiveInfo2()                              = 0;
        virtual GRtextPopup *ActiveHtinfo()                             = 0;
        virtual GRtextPopup *ActiveError()                              = 0;
        virtual GRfontPopup *ActiveFontsel()                            = 0;

        // If set, a button will display this file from the error pop-up
        // window.
        virtual void SetErrorLogName(const char*)                       = 0;

        // If this is set, then the first child pop-up will use the
        // existing shell and thus be top-level.
        //
        void SetCreateTopLevel()    { create_top_level = true; }
        bool IsSetTopLevel()        { return (create_top_level); }

        // If this is set, the GRfilePopup and path files list won't
        // confirm before move/copy/link of drag/drop files or
        // directories.
        //
        static void SetNoAskFileAction(bool b)
                                    { no_ask_file_action = b; }
        static bool IsNoAskFileAction()
                                    { return (no_ask_file_action); }

private:
        bool create_top_level;
        static bool no_ask_file_action;
    };

} // namespace


//-----------------------------------------------------------------------------
// Device driver

namespace ginterf
{
    // Type of graphics device.
    enum GRdevType {GRnodev, GRmultiWindow, GRfullScreen, GRhardcopy};

    // Base class for graphics display.
    //
    struct GRdev
    {
        GRdev()
            {
                name = 0;
                ident = 0;
                devtype = GRnodev;
                width = height = 0;
                xoff = yoff = 0;
                numlinestyles = numcolors = 0;
            }
        virtual ~GRdev() { }

    protected:
        bool HCdevParse(HCdata*, int*, char**);
        void HCcheckEntries(HCdata*, HCdesc*);

    public:
        bool LineClip(int*, int*, int*, int*);

        virtual bool Init(int*, char**)                         = 0;
        virtual GRdraw *NewDraw(int = 0)                        = 0;

        const char *name;               // device name
        int ident;                      // numerical id
        GRdevType devtype;              // type of graphics device
        int width;                      // display width
        int height;                     // display height
        int xoff;                       // left offset
        int yoff;                       // top offset;

        int numlinestyles;              // number of line styles supported
        int numcolors;                  // number of colors supported
        // if 0, there is no internal limit
    };

    typedef void(*GRsigintHdlr)();

    // Derived class for screen graphics.
    //
    struct GRscreenDev : public GRdev
    {
        virtual GRwbag *NewWbag(const char*, GRwbag*)           = 0;
        virtual bool InitColormap(int, int, bool)               = 0;
        virtual int AllocateColor(int*, int, int, int)          = 0;
        virtual int NameColor(const char*)                      = 0;
        virtual bool NameToRGB(const char*, int*)               = 0;
        virtual void RGBofPixel(int, int*, int*, int*)          = 0;

        virtual int AddTimer(int, int(*)(void*), void*)         = 0;
        virtual void RemoveTimer(int)                           = 0;
        virtual GRsigintHdlr RegisterSigintHdlr(GRsigintHdlr)   = 0;
        virtual bool CheckForEvents()                           = 0;
        virtual int Input(int, int, int *k)                     = 0;
        virtual void MainLoop(bool=false)                       = 0;
        virtual int LoopLevel()                                 = 0;
        virtual void BreakLoop()                                = 0;
        virtual void HCmessage(const char*)                     = 0;
        virtual int UseSHM()                                    = 0;
    };
}


//-----------------------------------------------------------------------------
// Graphics container

namespace ginterf
{
    // Returns for GRpkg::SwitchDev
    enum HCswitchErr { HCSok, HCSinhc, HCSnotfnd, HCSinit };

    // Type values for GRpkg::ErrPrintf()
    enum Etype {ET_MSG, ET_MSGS, ET_WARN, ET_WARNS, ET_ERROR, ET_ERRORS,
            ET_INTERR, ET_INTERRS};

    typedef void(*GRsigintHandler)();
    typedef void(*GRerrorCallback)(Etype, bool, const char*);

    // Interface to set misc. colors in the interface.  Used by
    // GRpkg::Get/SetAttrColor.
    enum GRattrColor
    {
        GRattrColorLocSel,  // Local selection background, misc. uses.
        GRattrColorApp1,    // Application-specified colors.
        GRattrColorApp2,
        GRattrColorApp3,
        GRattrColorApp4,
        GRattrColorApp5,
        GRattrColorApp6,
        GRattrColorApp7,
        GRattrColorApp8,
        GRattrColorApp9,
        GRattrColorApp10,
        GRattrColorApp11,
        GRattrColorApp12,
        GRattrColorApp13,
        GRattrColorApp14,
        GRattrColorEnd      // End flag.
    };

    extern inline class GRpkg *GRpkgIf();

    // The main class for the graphics package.
    //
    class GRpkg
    {
        static GRpkg *ptr()
            {
                if (!instancePtr)
                    on_null_ptr();
                return (instancePtr);
            }

        static void on_null_ptr();

    public:
        friend inline GRpkg *GRpkgIf() { return (GRpkg::ptr()); }

        GRpkg();
        virtual ~GRpkg() { }

        bool InitPkg(unsigned int, int*, char**);
        bool SetNullGraphics();
        HCswitchErr SwitchDev(const char*, int*, char**);
        GRdev *FindDev(const char*);
        HCdesc *FindHCdesc(const char*);
        int FindHCindex(const char*);

        int Input(int, int, int*);
        int GetChar(int);
        void Perror(const char*);
        void RegisterErrorCallback(GRerrorCallback);
        void ErrPrintf(Etype, const char*, ...);

        void HCmessage(const char *msg)
            {
                if (pkg_main_dev)
                    pkg_main_dev->HCmessage(msg);
            }

        HCdesc *HCof(int i)
            {
                if (i < 0 || i >= pkg_hc_desc_count)
                    return (0);
                if (pkg_hc_descs[i] && pkg_hc_descs[i]->descr)
                    return (pkg_hc_descs[i]);
                return (0);
            }

        void HCabort(const char *msg)
            {
                if (msg) {
                    pkg_abort = true;
                    pkg_abort_msg = msg;
                }
                else {
                    pkg_abort = false;
                    pkg_abort_msg = 0;
                }
            }

        bool HCaborted()                { return (pkg_abort); }
        const char *HCabortMsg()        { return (pkg_abort_msg); }

        void ResetMain(GRdev *d)
            {
                delete pkg_devices[0];
                pkg_devices[0] = d;
                pkg_main_dev = 0;
                pkg_cur_dev = 0;
            }

        void RegisterMainWbag(GRwbag *cx) { pkg_main_wbag = cx; }
        GRwbag *MainWbag()              { return (pkg_main_wbag); }

        // Redirected driver calls
        bool Init(int *ac, char **av)
            {
                return (pkg_cur_dev ? pkg_cur_dev->Init(ac, av) : false);
            }

        GRdraw *NewDraw(int at = 0)
            {
                return (pkg_cur_dev ? pkg_cur_dev->NewDraw(at) : 0);
            }

        GRwbag *NewWbag(const char *s, GRwbag *b)
            {
                return (pkg_main_dev ? pkg_main_dev->NewWbag(s, b) : 0);
            }

        // These two functions are virtual, since the application
        // (WRspice) may keep a pixel/rgb mapping for use when
        // screen graphics is not present.
        //
        virtual bool InitColormap(int mn, int mx, bool dp)
            {
                return (pkg_main_dev ?
                    pkg_main_dev->InitColormap(mn, mx, dp) : false);
            }

        virtual void RGBofPixel(int p, int *r, int *g, int *b)
            {
                if (pkg_main_dev)
                    pkg_main_dev->RGBofPixel(p, r, g, b);
                else
                    *r = *g = *b = 0;
            }

        int AllocateColor(int *p, int r, int g, int b)
            {
                if (pkg_main_dev)
                    return (pkg_main_dev->AllocateColor(p, r, g, b));
                 *p = 0;
                 return (0);
             }

        int NameColor(const char *nm)
            {
                return (pkg_main_dev ? pkg_main_dev->NameColor(nm) : 0);
            }

        bool NameToRGB(const char *nm, int *p)
            {
                if (pkg_main_dev)
                    return (pkg_main_dev->NameToRGB(nm, p));
                 p[0] = p[1] = p[2] = 0;
                 return (false);
             }

        void SetAttrColor(GRattrColor, const char*);
        const char *GetAttrColor(GRattrColor);

        int AddTimer(int ms, int(*cb)(void*), void *a)
            {
                return (pkg_main_dev ? pkg_main_dev->AddTimer(ms, cb, a) : 0);
            }

        void RemoveTimer(int id)
            {
                if (pkg_main_dev)
                    pkg_main_dev->RemoveTimer(id);
            }

        GRsigintHandler RegisterSigintHdlr(GRsigintHandler f)
            {
                return (pkg_main_dev ?
                    pkg_main_dev->RegisterSigintHdlr(f) : 0);
            }

        bool CheckForEvents()
            {
                return (pkg_main_dev ? pkg_main_dev->CheckForEvents() : false);
            }

        void MainLoop()
            {
                if (pkg_main_dev)
                    pkg_main_dev->MainLoop();
            }

        int LoopLevel()
            {
                return (pkg_main_dev ? pkg_main_dev->LoopLevel() : 0);
            }

        void BreakLoop()
            {
                if (pkg_main_dev)
                    pkg_main_dev->BreakLoop();
            }

        GRscreenDev *MainDev()  { return (pkg_main_dev); }
        GRdev *CurDev()         { return (pkg_cur_dev); }

        int UseSHM()
            {
                if (pkg_main_dev)
                    return (pkg_main_dev->UseSHM());
                return (0);
            }

    private:
        void RegisterDevice(GRdev*);
        void RegisterHcopyDesc(HCdesc*);

        // Supplied by device-dependent graphics.
        void DevDepInit(unsigned int);

        GRdev *pkg_cur_dev;     // Current graphics package, switch for
                                // printing.
        GRscreenDev *pkg_main_dev;  // Base (screen) graphics package.
        HCdesc **pkg_hc_descs;   // Hardcopy driver descriptors.

        GRdev *pkg_last_dev;    // Keep track of device for mode switch.
        GRdev **pkg_devices;    // Container for packages.
        int pkg_hc_desc_count;  // Count of hardcopy descriptors registered.
        int pkg_device_count;   // Count of devices registered.
        int pkg_max_count;      // Max registered devices or hc descs.

        bool  pkg_abort;        // Hardcopy abort indicator.
        const char *pkg_abort_msg;  // Abort error message.

        GRerrorCallback pkg_error_callback;
                                // Registered callback for ErrPrintf.

        GRwbag *pkg_main_wbag;  // Global widget bag.

        const char *pkg_colors[GRattrColorEnd];
                                // Misc. color strings.

        static GRpkg *instancePtr;
    };
}

//-----------------------------------------------------------------------------
// Color name and RGB values database

namespace ginterf
{
    // A list of rgb.txt colors.
    //
    struct GRcolorList
    {
        static bool lookupColor(const char*, int*, int*, int*);
        static stringlist *listColors();

    private:
        GRcolorList(const char *n, int r, int g, int b)
            {
                name = n;
                red = r;
                green = g;
                blue = b;
            }

        const char *name;
        unsigned char red, green, blue;

        static GRcolorList color_list[];
    };
}

#endif

