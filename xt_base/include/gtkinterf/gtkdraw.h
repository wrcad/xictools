
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
 * GtkInterf Graphical Interface Library                                  *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef GTKDRAW_H
#define GTKDRAW_H


#ifndef WITH_QUARTZ
#ifndef WIN32
#define WITH_X11
#endif
#endif

#ifdef WITH_X11
#include "gtkinterf/gtkx11.h"
#endif
#if GTK_CHECK_VERSION(3,0,0)
#include "gtkinterf/ndkpixmap.h"
#include "gtkinterf/ndkdrawable.h"
#include "gtkinterf/ndkimage.h"
#include "gtkinterf/ndkgc.h"
#include "gtkinterf/ndkcursor.h"
#endif

namespace ginterf
{
    struct GRlineDb;
}

namespace gtkinterf {
    // Encapsulation of the window ghost-drawing capability.
    //
    struct sGdraw
    {
        sGdraw() {
            gd_draw_ghost = 0;
            gd_linedb = 0;
            gd_ref_x = 0;
            gd_ref_y = 0;
            gd_last_x = 0;
            gd_last_y = 0;
            gd_ghost_cx_cnt = 0;
            gd_first_ghost = false;
            gd_show_ghost = false;
            gd_undraw = false;
        }

        void set_ghost(GhostDrawFunc, int, int);
        void show_ghost(bool);
        void undraw_ghost(bool);
        void draw_ghost(int, int);

        bool has_ghost()    { return (gd_draw_ghost != 0); }
        bool showing()      { return (gd_show_ghost); }
        GRlineDb *linedb()  { return (gd_linedb); }

        GhostDrawFunc get_ghost_func()  { return (gd_draw_ghost); }
        void set_ghost_func(GhostDrawFunc f)
            {
                gd_draw_ghost = f;
                gd_first_ghost = true;
            }

    private:
        GhostDrawFunc gd_draw_ghost;
        GRlineDb *gd_linedb;        // line clipper for XOR mode
        int gd_ref_x;
        int gd_ref_y;
        int gd_last_x;
        int gd_last_y;
        int gd_ghost_cx_cnt;
        bool gd_first_ghost;
        bool gd_show_ghost;
        bool gd_undraw;
    };

    // Graphical context, may be used by multiple windows.
    struct sGbag
    {
        sGbag();
        ~sGbag();

        void set_xor(bool x)
            {
                if (x) {
                    gb_gcbak = gb_gc;
                    gb_gc = gb_xorgc;
                }
                else
                    gb_gc = gb_gcbak;
            }

#if GTK_CHECK_VERSION(3,0,0)
        ndkGC *main_gc()
#else
        GdkGC *main_gc()
#endif
            {
                return (gb_gc != gb_xorgc ? gb_gc : gb_gcbak);
            }

        void set_ghost(GhostDrawFunc cb, int x, int y)
            {
                set_xor(true);
                gb_gdraw.set_ghost(cb, x, y);
                set_xor(false);
            }

        void show_ghost(bool show)
            {
                set_xor(true);
                gb_gdraw.show_ghost(show);
                set_xor(false);
            }

        void undraw_ghost(bool rst)
            {
                set_xor(true);
                gb_gdraw.undraw_ghost(rst);
                set_xor(false);
            }

        void draw_ghost(int x, int y)
            {
                set_xor(true);
                gb_gdraw.draw_ghost(x, y);
                set_xor(false);
            }

        bool has_ghost()
            {
                return (gb_gdraw.has_ghost());
            }

        bool showing_ghost()
            {
                return (gb_gdraw.showing());
            }

        GhostDrawFunc get_ghost_func()
            {
                return (gb_gdraw.get_ghost_func());
            }

        void set_ghost_func(GhostDrawFunc f)
            {
                gb_gdraw.set_ghost_func(f);
            }

#if GTK_CHECK_VERSION(3,0,0)
        void set_gc(ndkGC *gc)                  { gb_gc = gc; }
        ndkGC *get_gc()                         { return (gb_gc); }
        void set_xorgc(ndkGC *gc)               { gb_xorgc = gc; }
        ndkGC *get_xorgc()                      { return (gb_xorgc); }
#else
        void set_gc(GdkGC *gc)                  { gb_gc = gc; }
        GdkGC *get_gc()                         { return (gb_gc); }
        void set_xorgc(GdkGC *gc)               { gb_xorgc = gc; }
        GdkGC *get_xorgc()                      { return (gb_xorgc); }
#endif
        void set_cursor_type(unsigned int t)    { gb_cursor_type = t; }
        unsigned int get_cursor_type()          { return (gb_cursor_type); }
        GRlineDb *linedb()                      { return (gb_gdraw.linedb()); }

#define NUMGCS 10
        static sGbag *app_gbags[NUMGCS];

#ifdef WIN32
        void set_fillpattern(const GRfillType *fp) { gb_fillpattern = fp; }
        const GRfillType *get_fillpattern()     { return (gb_fillpattern); }
#endif

        static sGbag *default_gbag(int = 0);

    private:
#if GTK_CHECK_VERSION(3,0,0)
        ndkGC *gb_gc;
        ndkGC *gb_xorgc;
        ndkGC *gb_gcbak;
#else
        GdkGC *gb_gc;
        GdkGC *gb_xorgc;
        GdkGC *gb_gcbak;
#endif
#ifdef WIN32
        const GRfillType *gb_fillpattern;
#endif
        unsigned int gb_cursor_type;
        sGdraw gb_gdraw;
    };

    struct GTKdraw : virtual public GRdraw
    {
        GTKdraw(int);
        virtual ~GTKdraw();

#if GTK_CHECK_VERSION(3,0,0)
        void SetViewport(GtkWidget*);
        void *WindowID();
#else
        void *WindowID()                    { return (gd_window); }
#endif

        // gtkinterf.cc
        void Halt();
        void Clear();
        void ResetViewport(int, int)                    { }
        void DefineViewport()                           { }
        void Dump(int)                                  { }
        void Pixel(int, int);
        void Pixels(GRmultiPt*, int);
        void Line(int, int, int, int);
        void PolyLine(GRmultiPt*, int);
        void Lines(GRmultiPt*, int);
        void Box(int, int, int, int);
        void Boxes(GRmultiPt*, int);
        void Arc(int, int, int, int, double, double);
        void Polygon(GRmultiPt*, int);
        void Zoid(int, int, int, int, int, int);
        void Text(const char*, int, int, int, int = -1, int = -1);
        void TextExtent(const char*, int*, int*);

        void SetGhost(GhostDrawFunc cb, int x, int y)
                                            { gd_gbag->set_ghost(cb, x, y); }
        void ShowGhost(bool show)           { gd_gbag->show_ghost(show); }
        void UndrawGhost(bool reset = false)
                                            { gd_gbag->undraw_ghost(reset); }
        void DrawGhost(int x, int y)        { gd_gbag->draw_ghost(x, y); }

        void MovePointer(int, int, bool);
        void QueryPointer(int*, int*, unsigned*);
        void DefineColor(int*, int, int, int);
        void SetBackground(int);
        void SetWindowBackground(int);
        void SetGhostColor(int);
        void SetColor(int);
        void DefineLinestyle(GRlineType*)               { }
        void SetLinestyle(const GRlineType*);
        void DefineFillpattern(GRfillType*);
        void SetFillpattern(const GRfillType*);
        void Refresh(int, int, int, int)                { }
        void Refresh()                                  { }
        void Update(int, int, int, int)                 { }
        void Update();
        void Input(int*, int*, int*, int*);
        void SetXOR(int);
        void ShowGlyph(int, int, int);
        GRobject GetRegion(int, int, int, int);
        void PutRegion(GRobject, int, int, int, int);
        void FreeRegion(GRobject);
        void DisplayImage(const GRimage*, int, int, int, int);
        double Resolution()     { return (1.0); }

        // non-overrides
#if GTK_CHECK_VERSION(3,0,0)
        ndkGC *GC()         { return (gd_gbag ? gd_gbag->get_gc() : 0); }
        ndkGC *XorGC()      { return (gd_gbag ? gd_gbag->get_xorgc() : 0); }
        ndkGC *CpyGC()      { return (gd_gbag ? gd_gbag->main_gc() : 0); }
#else
        GdkGC *GC()             { return (gd_gbag ? gd_gbag->get_gc() : 0); }
        GdkGC *XorGC()          { return (gd_gbag ? gd_gbag->get_xorgc() : 0); }
        GdkGC *CpyGC()          { return (gd_gbag ? gd_gbag->main_gc() : 0); }
#endif

        GRlineDb *XorLineDb()   { return (gd_gbag ? gd_gbag->linedb() : 0); }

        sGbag *Gbag()           { return (gd_gbag); }
        void SetGbag(sGbag *b)  { gd_gbag = b; }

        GtkWidget *Viewport()           { return (gd_viewport); }
#if GTK_CHECK_VERSION(3,0,0)
        ndkDrawable *GetDrawable()      { return (&gd_dw); }
#else
        void SetViewport(GtkWidget *w)  { gd_viewport = w; }
        GdkWindow *Window()             { return (gd_window); }
        void SetWindow(GdkWindow *w)    { gd_window = w; }
#endif

        unsigned int GetBackgPixel()          { return (gd_backg); }
        unsigned int GetForegPixel()          { return (gd_foreg); }

    protected:
        GtkWidget *gd_viewport;         // drawing widget
#if GTK_CHECK_VERSION(3,0,0)
        ndkDrawable gd_dw;              // drawing context
#else
        GdkWindow *gd_window;           // drawing window
#endif
        sGbag *gd_gbag;                 // graphics rendering context
        unsigned int gd_backg;          // background pixel, same in both GCs
        unsigned int gd_foreg;          // foreground drawing pixel
        unsigned int gd_xor_fg;         // ghost drawing pixel
    };
}

#endif  // GTKDRAW_H

