
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
 $Id: gtkcoord.h,v 5.9 2010/07/14 06:25:28 stevew Exp $
 *========================================================================*/
 
#ifndef GTKCOORD_H
#define GTKCOORD_H


inline class cCoord *Coord();

// The coordinate readout
class cCoord : public gtk_draw
{
public:
    // update mode for cCoord::print()
    enum {COOR_BEGIN, COOR_MOTION, COOR_REL};

    friend inline cCoord *Coord() { return (cCoord::instancePtr); }

    cCoord();
    void print(int, int, int);

    void set_mode(int x, int y, bool relative, bool snap)
        {
            co_x = x;
            co_y = y;
            co_rel = relative;
            co_snap = snap;
            print(0, 0, COOR_REL);
        }

    void redraw()
        {
            co_redraw(0, 0, 0);
        }

private:
    void do_print(int, int, int);

    static int print_idle(void*);
    static int co_btn(GtkWidget*, GdkEvent*, void*);
    static void co_redraw(GtkWidget*, GdkEvent*, void*);
    static void co_font_change(GtkWidget*, void*, void*);

    GdkWindow *co_win_bak;
    GdkPixmap *co_pm;
    int co_width, co_height;
    int co_x, co_y;
    int co_lx, co_ly;
    int co_xc, co_yc;
    int co_id;
    bool co_rel, co_snap;

    static cCoord *instancePtr;
};

#endif

