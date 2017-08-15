
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
 *                                                                        *
 * LEF/DEF Database and Maze Router.                                      *
 *                                                                        *
 * Stephen R. Whiteley (stevew@wrcad.com)                                 *
 * Whiteley Research Inc. (wrcad.com)                                     *
 *                                                                        *
 * Portions adapted from Qrouter by Tim Edwards,                          *
 * (www.opencircuitdesign.com) which used code by Steve Beccue.           *
 * See original headers where applicable.                                 *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include <math.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <tk.h>

#include "mrouter_prv.h"


//
// Maze Router.
//
// This is CURRENTLY NOT USED.  The MRouter has no internal graphics
// at present.
//

struct mrX11Graphics : public mrGraphics
{
    mrX11Graphics(cMRouter*);

    void    highlight_source();
    void    highlight_dest();
    void    highlight_starts(const mrPoint*);
    void    highlight_mask();
    void    highlight(int, int);
    void    draw_net(const dbNet*, u_int, int*);
    void    draw_layout();
    bool    recalc_spacing();

    int     init(Tcl_Interp*);
    void    expose(Tk_Window);
    int     redraw(ClientData, Tcl_Interp*, int, Tcl_Obj*[]);
    void    resize(Tk_Window, int, int);

private:
    void    draw_net_nodes(const dbNet*);
    void    map_obstruction();
    void    map_congestion();
    void    map_estimate();
    void    load_font();
    void    createGC();

    cMRouter    *router;
    XFontStruct *font_info;
    Pixmap      buffer;
    Display     *dpy;
    Window      win;
    Colormap    cmap;
    GC          gc;
    Dimension   width, height;

#define SHORTSPAN 10
#define LONGSPAN  127

    int spacing;
    int bluepix, greenpix, redpix, cyanpix, orangepix, goldpix;
    int blackpix, whitepix, graypix, ltgraypix, yellowpix;
    int magentapix, purplepix, greenyellowpix;
    int brownvector[SHORTSPAN];
    int bluevector[LONGSPAN];
};


mrX11Graphics::mrX11Graphics(cMRouter *mr)
{
    router              = mr;
    font_info           = 0;
    buffer              = 0;
    dpy                 = 0;
    win                 = 0;
    width               = 0;
    height              = 0;
    spacing             = 0;
    bluepix             = 0;
    greenpix            = 0;
    redpix              = 0;
    cyanpix             = 0;
    orangepix           = 0;
    goldpix             = 0;
    blackpix            = 0;
    whitepix            = 0;
    graypix             = 0;
    ltgraypix           = 0;
    yellowpix           = 0;
    magentapix          = 0;
    purplepix           = 0;
    greenyellowpix      = 0;
    memset(brownvector, 0, SHORTSPAN*sizeof(int));
    memset(bluevector, 0, LONGSPAN*sizeof(int));
}


// Highlight source (in magenta).
//
void
mrX11Graphics::highlight_source()
{
    if (!router->obs2(0))
        return;

    // Determine the number of routes per width and height, if it has
    // not yet been computed.

    int hspc = spacing >> 1;
    if (hspc == 0)
        hspc = 1;

    // Draw source pins as magenta squares.
    XSetForeground(dpy, gc, magentapix);
    for (u_int i = 0; i < router->numLayers(); i++) {
        for (int x = 0; x < router->numChannelsX(i); x++) {
            int xspc = (x + 1) * spacing - hspc;
            for (int y = 0; y < router->numChannelsY(i); y++) {
                mrProute *Pr = router->obs2Val(x, y, i);
                if (Pr->flags & PR_SOURCE) {
                    int yspc = height - (y + 1) * spacing - hspc;
                    XFillRectangle(dpy, win, gc, xspc, yspc, spacing, spacing);
                }
            }
        }
    }
    XFlush(dpy);
}


// Highlight destination (in purple).
//
void
mrX11Graphics::highlight_dest()
{
    if (!router->obs2(0))
        return;

    // Determine the number of routes per width and height, if it has
    // not yet been computed.

    int dspc = spacing + 4;         // Make target more visible.
    int hspc = dspc >> 1;

    // Draw destination pins as purple squares.
    XSetForeground(dpy, gc, purplepix);
    for (u_int i = 0; i < router->numLayers(); i++) {
        for (int x = 0; x < router->numChannelsX(i); x++) {
            int xspc = (x + 1) * spacing - hspc;
            for (int y = 0; y < router->numChannelsY(i); y++) {
                mrProute *Pr = router->obs2Val(x, y, i);
                if (Pr->flags & PR_TARGET) {
                    int yspc = height - (y + 1) * spacing - hspc;
                    XFillRectangle(dpy, win, gc, xspc, yspc, dspc, dspc);
                }
            }
        }
    }
    XFlush(dpy);
}


// Highlight all the search starting points.
//
void
mrX11Graphics::highlight_starts(const mrPoint *glist)
{
    // Determine the number of routes per width and height, if it has
    // not yet been computed.

    int hspc = spacing >> 1;

    XSetForeground(dpy, gc, greenyellowpix);
    for (const mrPoint *gpoint = glist; gpoint; gpoint = gpoint->next) {
        int xspc = (gpoint->x1 + 1) * spacing - hspc;
        int yspc = height - (gpoint->y1 + 1) * spacing - hspc;
        XFillRectangle(dpy, win, gc, xspc, yspc, spacing, spacing);
    }
    XFlush(dpy);
}


// Highlight mask (in tan).
//
void
mrX11Graphics::highlight_mask()
{
    if (!router->hasRmask())
        return;

    // Determine the number of routes per width and height, if it has
    // not yet been computed.

    int hspc = spacing >> 1;

    // Draw destination pins as tan squares.
    for (int x = 0; x < router->numChannelsX(0); x++) {
        int xspc = (x + 1) * spacing - hspc;
        for (int y = 0; y < router->numChannelsY(0); y++) {
            XSetForeground(dpy, gc, brownvector[router->rmask(x, y)]);
            int yspc = height - (y + 1) * spacing - hspc;
            XFillRectangle(dpy, win, gc, xspc, yspc, spacing, spacing);
        }
    }
    XFlush(dpy);
}


// Highlight a position on the graph.  Do this on the actual screen,
// not the buffer.
//
void
mrX11Graphics::highlight(int x, int y)
{
    // If Obs2[] at x, y is a source or dest, don't highlight Do this
    // only for layer 0; it doesn't have to be rigorous. 
    for (u_int i = 0; i < router->numLayers(); i++) {
        mrProute *Pr = router->obs2Val(x, y, i);
        if (Pr->flags & (PR_SOURCE | PR_TARGET))
            return;
    }

    int hspc = spacing >> 1;
    if (hspc == 0)
        hspc = 1;

    int xspc = (x + 1) * spacing - hspc;
    int yspc = height - (y + 1) * spacing - hspc;

    XSetForeground(dpy, gc, yellowpix);
    XFillRectangle(dpy, win, gc, xspc, yspc, spacing, spacing);
    XFlush(dpy);
}


// Draw one net on the display.
//
void
mrX11Graphics::draw_net(const dbNet *net, u_int single, int *lastlayer)
{
    if (!dpy)
        return;

    // Draw all nets, much like "emit_routes" does when writing routes
    // to the DEF file.

    dbRoute *rt = net->routes;
    if (single && rt)
        for (rt = net->routes; rt->next; rt = rt->next);

    for (; rt; rt = rt->next) {
        for (dbSeg *seg = rt->segments; seg; seg = seg->next) {
            int layer = seg->layer;
            if (layer != *lastlayer) {
                *lastlayer = layer;
                switch(layer) {
                case 0:
                    XSetForeground(dpy, gc, bluepix);
                    break;
                case 1:
                    XSetForeground(dpy, gc, redpix);
                    break;
                case 2:
                    XSetForeground(dpy, gc, cyanpix);
                    break;
                case 3:
                    XSetForeground(dpy, gc, goldpix);
                    break;
                default:
                    XSetForeground(dpy, gc, greenpix);
                    break;
                }
            }
            XDrawLine(dpy, buffer, gc, spacing * (seg->x1 + 1),
                                height - spacing * (seg->y1 + 1),
                                spacing * (seg->x2 + 1),
                                height - spacing * (seg->y2 + 1));
            if (single)
                XDrawLine(dpy, win, gc, spacing * (seg->x1 + 1),
                                height - spacing * (seg->y1 + 1),
                                spacing * (seg->x2 + 1),
                                height - spacing * (seg->y2 + 1));
        }
    }
    if (single) {
        // The following line to be removed.
        XCopyArea(dpy, buffer, win, gc, 0, 0, width, height, 0, 0);
        XFlush(dpy);
    }
}


// Graphical display of the layout.
//
void
mrX11Graphics::draw_layout()
{
    if (!dpy || !buffer)
        return;

    XSetForeground(dpy, gc, whitepix);
    XFillRectangle(dpy, buffer, gc, 0, 0, width, height);

    // Check if a netlist even exists.
    if (!router->obs(0)) {
        XCopyArea(dpy, buffer, win, gc, 0, 0, width, height, 0, 0);
        return;
    }

    switch (router->mapType() & MAP_MASK) {
    case MAP_OBSTRUCT:
        map_obstruction();
        break;
    case MAP_CONGEST:
        map_congestion();
        break;
    case MAP_ESTIMATE:
        map_estimate();
        break;
    }

    // Draw all nets, much like "emit_routes" does when writing routes
    // to the DEF file.

    int lastlayer;
    if ((router->mapType() & DRAW_ROUTES) != 0) {
        lastlayer = -1;
        for (u_int i = 0; i < router->numNets(); i++) {
            dbNet *net = router->mr_Net(i);
            draw_net(net, false, &lastlayer);
        }
    }

    // Draw unrouted nets.

    if ((router->mapType() & DRAW_UNROUTED) != 0) {
        XSetForeground(dpy, gc, blackpix);
        for (u_int i = 0; i < router->numNets(); i++) {
            dbNet *net = router->mr_Net(i);
            if (strcmp(net->netname, router->gndNet()) != 0
                    && strcmp(net->netname, router->vddNet()) != 0
                    && net->routes == 0) {
                draw_net_nodes(net);
            }
        }
    }

    // Copy double-buffer onto display window.
    XCopyArea(dpy, buffer, win, gc, 0, 0, width, height, 0, 0);
}


// Call to recalculate the spacing if numChannelsX(0) or
// numChannelsY(0) changes.
//
// Return true if the spacing changed, 0 otherwise.
//
bool
mrX11Graphics::recalc_spacing()
{
    int oldspacing = spacing;
    int xspc = width / (router->numChannelsX(0) + 1);
    int yspc = height / (router->numChannelsY(0) + 1);
    spacing = (xspc < yspc) ? xspc : yspc;
    if (spacing == 0)
        spacing = 1;

    return (spacing != oldspacing);
}


int
mrX11Graphics::init(Tcl_Interp *interp)
{
    static const char *qrouterdrawdefault = ".qrouter";

    Tk_Window tktop = Tk_MainWindow(interp);
    if (!tktop) {
        router->emitErrMesg("No Top-level Tk window available. . .\n");
        return (TCL_ERROR);
    }

    const char *qrouterdrawwin = (char*)Tcl_GetVar(interp, "drawwindow",
        TCL_GLOBAL_ONLY);
    if (!qrouterdrawwin)
        qrouterdrawwin = qrouterdrawdefault;

    Tk_Window tkwind = Tk_NameToWindow(interp, qrouterdrawwin, tktop);
   
    if (!tkwind) {
        router->emitErrMesg("The Tk window hierarchy must be rooted at "
            ".qrouter or $drawwindow must point to the drawing window\n");
        return (TCL_ERROR);
    }
   
    Tk_MapWindow(tkwind);
    dpy = Tk_Display(tkwind);
    win = Tk_WindowId(tkwind);
    cmap = DefaultColormap (dpy, Tk_ScreenNumber(tkwind));

    load_font();

    // create GC for text and drawing
    createGC();

    // Initialize colors

    XColor cvcolor, cvexact;
    XAllocNamedColor(dpy, cmap, "blue", &cvcolor, &cvexact);
    bluepix = cvcolor.pixel;
    XAllocNamedColor(dpy, cmap, "cyan", &cvcolor, &cvexact);
    cyanpix = cvcolor.pixel;
    XAllocNamedColor(dpy, cmap, "green", &cvcolor, &cvexact);
    greenpix = cvcolor.pixel;
    XAllocNamedColor(dpy, cmap, "red", &cvcolor, &cvexact);
    redpix = cvcolor.pixel;
    XAllocNamedColor(dpy, cmap, "orange", &cvcolor, &cvexact);
    orangepix = cvcolor.pixel;
    XAllocNamedColor(dpy, cmap, "gold", &cvcolor, &cvexact);
    goldpix = cvcolor.pixel;
    XAllocNamedColor(dpy, cmap, "gray70", &cvcolor, &cvexact);
    ltgraypix = cvcolor.pixel;
    XAllocNamedColor(dpy, cmap, "gray50", &cvcolor, &cvexact);
    graypix = cvcolor.pixel;
    XAllocNamedColor(dpy, cmap, "yellow", &cvcolor, &cvexact);
    yellowpix = cvcolor.pixel;
    XAllocNamedColor(dpy, cmap, "purple", &cvcolor, &cvexact);
    purplepix = cvcolor.pixel;
    XAllocNamedColor(dpy, cmap, "magenta", &cvcolor, &cvexact);
    magentapix = cvcolor.pixel;
    XAllocNamedColor(dpy, cmap, "GreenYellow", &cvcolor, &cvexact);
    greenyellowpix = cvcolor.pixel;
    blackpix = BlackPixel(dpy,DefaultScreen(dpy));
    whitepix = WhitePixel(dpy,DefaultScreen(dpy));

    cvcolor.flags = DoRed | DoGreen | DoBlue;
    for (int i = 0; i < SHORTSPAN; i++) {
        double frac = (double)i / (double)(SHORTSPAN - 1);
        // gamma correction
        frac = pow(frac, 0.5);

        cvcolor.green = (int)(53970 * frac);
        cvcolor.blue = (int)(46260 * frac);
        cvcolor.red = (int)(35980 * frac);
        XAllocColor(dpy, cmap, &cvcolor);
        brownvector[i] = cvcolor.pixel;
    }

    cvcolor.green = 0;
    cvcolor.red = 0;

    for (int i = 0; i < LONGSPAN; i++) {
        double frac = (double)i / (double)(LONGSPAN - 1);
        // gamma correction
        frac = pow(frac, 0.5);

        cvcolor.blue = (int)(65535 * frac);
        XAllocColor(dpy, cmap, &cvcolor);
        bluevector[i] = cvcolor.pixel;
    }
    return (TCL_OK);       // proceed to interpreter
}


void
mrX11Graphics::expose(Tk_Window tkwind)
{
    if (Tk_WindowId(tkwind) == 0)
        return;
    if (!dpy)
        return;
    draw_layout();
}


int
mrX11Graphics::redraw(ClientData, Tcl_Interp*, int, Tcl_Obj*[])
{
    draw_layout();
    return (TCL_OK);
}


void
mrX11Graphics::resize(Tk_Window tkwind, int locwidth, int locheight)
{

    if ((locwidth == 0) || (locheight == 0))
        return;

    if (buffer != (Pixmap)0)
        XFreePixmap (Tk_Display(tkwind), buffer);

    if (Tk_WindowId(tkwind) == 0)
        Tk_MapWindow(tkwind);
   
    buffer = XCreatePixmap(Tk_Display(tkwind), Tk_WindowId(tkwind),
        locwidth, locheight, DefaultDepthOfScreen(Tk_Screen(tkwind)));

    width = locwidth;
    height = locheight;

    recalc_spacing();

    if (dpy)
        draw_layout();
}


// Draw one unrouted net on the display.
//
void
mrX11Graphics::draw_net_nodes(const dbNet *net)
{
    // Compute bbox for each node and draw it.
    dbSeg *bboxlist = 0; // bbox list of all the nodes in the net
    dbSeg *lastbbox;
    int n = 0;
    for (dbNode *node = net->netnodes; node; node = node->next, n++) {
        if (!bboxlist) {
            lastbbox = bboxlist = new dbSeg;
        }
        else {
            lastbbox->next = new dbSeg;
            lastbbox = lastbbox->next;
        }
        bool first = true;
        for (dbDpoint *tap = node->taps; tap; tap = tap->next, first = false) {
            if (first) {
                lastbbox->x1 = lastbbox->x2 = tap->gridx;
                lastbbox->y1 = lastbbox->y2 = tap->gridy;
            }
            else {
                lastbbox->x1 = LD_MIN(lastbbox->x1, tap->gridx);
                lastbbox->x2 = LD_MAX(lastbbox->x2, tap->gridx);
                lastbbox->y1 = LD_MIN(lastbbox->y1, tap->gridy);
                lastbbox->y2 = LD_MAX(lastbbox->y2, tap->gridy);
            }
        }

        // Convert to X coordinates.
        lastbbox->x1 = spacing * (lastbbox->x1 + 1);
        lastbbox->y1 = height - spacing * (lastbbox->y1 + 1);
        lastbbox->x2 = spacing * (lastbbox->x2 + 1);
        lastbbox->y2 = height - spacing * (lastbbox->y2 + 1);

        // Draw the bbox.
        int w = lastbbox->x2 - lastbbox->x1;
        int h = lastbbox->y1 - lastbbox->y2;
        XDrawRectangle(dpy, buffer, gc, lastbbox->x1, lastbbox->y1, w, h);
    }

    // if net->numnodes == 1 don't draw a wire.
    if (n == 2) {
        XDrawLine(dpy, buffer, gc,
            (bboxlist->x1 + bboxlist->x2)/2, (bboxlist->y1 + bboxlist->y2)/2,
            (lastbbox->x1 + lastbbox->x2)/2, (lastbbox->y1 + lastbbox->y2)/2);
    }
    else if (n > 2) {
        // Compute center point.
        mrPoint *midpoint = new mrPoint;
        for (dbSeg *bboxit = bboxlist; bboxit; bboxit = bboxit->next) {
            midpoint->x1 += (bboxit->x1 + bboxit->x2)/2;
            midpoint->y1 += (bboxit->y1 + bboxit->y2)/2;
        }
        midpoint->x1 /= n;
        midpoint->y1 /= n;

        for (dbSeg *bboxit = bboxlist; bboxit; bboxit = bboxit->next) {
            XDrawLine(dpy, buffer, gc, midpoint->x1, midpoint->y1,
                (bboxit->x1 + bboxit->x2)/2, (bboxit->y1 + bboxit->y2)/2);
        }
        delete midpoint;
    }
    bboxlist->free();
}


// Draw a map of obstructions and pins.
//
void
mrX11Graphics::map_obstruction()
{
    int hspc = spacing >> 1;

    // Draw obstructions as light gray squares.
    XSetForeground(dpy, gc, ltgraypix);
    for (u_int i = 0; i < router->numLayers(); i++) {
        for (int x = 0; x < router->numChannelsX(i); x++) {
            int xspc = (x + 1) * spacing - hspc;
            for (int y = 0; y < router->numChannelsY(i); y++) {
                if (router->obsVal(x, y, i) & NO_NET) {
                    int yspc = height - (y + 1) * spacing - hspc;
                    XFillRectangle(dpy, buffer, gc, xspc, yspc,
                        spacing, spacing);
                }
            }
        }
    }

    // Draw pins as gray squares.
    XSetForeground(dpy, gc, graypix);
    for (u_int i = 0; i < router->pinLayers(); i++) {
        for (int x = 0; x < router->numChannelsX(i); x++) {
            int xspc = (x + 1) * spacing - hspc;
            for (int y = 0; y < router->numChannelsY(i); y++) {
                if (router->nodeSav(x, y, i)) {
                    int yspc = height - (y + 1) * spacing - hspc;
                    XFillRectangle(dpy, buffer, gc, xspc, yspc,
                        spacing, spacing);
                }
            }
        }
    }
}


// Draw a map of actual route congestion.
//
void
mrX11Graphics::map_congestion()
{
    int hspc = spacing >> 1;

    u_int sz = router->numChannelsX(0) * router->numChannelsY(0);
    u_char *Congestion = new u_char[sz];
    memset(Congestion, 0, sz);

    // Analyze obs array for congestion.
    for (u_int i = 0; i < router->numLayers(); i++) {
        for (int x = 0; x < router->numChannelsX(i); x++) {
            for (int y = 0; y < router->numChannelsY(i); y++) {
                int value = 0;
                int n = router->obsVal(x, y, i);
                if (n & ROUTED_NET)
                    value++;
                if (n & BLOCKED_MASK)
                    value++;
                if (n & NO_NET)
                    value++;
                if (n & PINOBSTRUCTMASK)
                    value++;
                router->setCongest(x, y, router->congest(x, y) + value);
            }
        }
    }

    int maxval = 0;
    for (int x = 0; x < router->numChannelsX(0); x++) {
        for (int y = 0; y < router->numChannelsY(0); y++) {
            int value = router->congest(x, y);
            if (value > maxval)
                maxval = value;
        }
    }
    int norm = (LONGSPAN - 1) / maxval;

    // Draw destination pins as blue squares.
    for (int x = 0; x < router->numChannelsX(0); x++) {
        int xspc = (x + 1) * spacing - hspc;
        for (int y = 0; y < router->numChannelsY(0); y++) {
            XSetForeground(dpy, gc, bluevector[norm * router->congest(x, y)]);
            int yspc = height - (y + 1) * spacing - hspc;
            XFillRectangle(dpy, buffer, gc, xspc, yspc, spacing, spacing);
        }
    }

    // Cleanup.
    delete [] Congestion;
}


// Draw a map of route congestion estimated from net bounding boxes.
//
void
mrX11Graphics::map_estimate()
{
    int hspc = spacing >> 1;

    u_int sz = router->numChannelsX(0) * router->numChannelsY(0);
    double *Congestion = new double[sz];
    memset(Congestion, 0, sz*sizeof(double));

    // Use net bounding boxes to estimate congestion.

    for (u_int i = 0; i < router->numNets(); i++) {
        dbNet *net = router->mr_Net(i);
        int nwidth = (net->xmax - net->xmin + 1);
        int nheight = (net->ymax - net->ymin + 1);
        int area = nwidth * nheight;
        int length;
        if (nwidth > nheight) {
            length = nwidth + (nheight >> 1) * net->numnodes;
        }
        else {
            length = nheight + (nwidth >> 1) * net->numnodes;
        }
        double density = (double)length / (double)area;

        for (int x = net->xmin; x < net->xmax; x++) {
            for (int y = net->ymin; y < net->ymax; y++)
                router->setCongest(x, y, router->congest(x, y) + density);
        }
    }

    double maxval = 0.0;
    for (int x = 0; x < router->numChannelsX(0); x++) {
        for (int y = 0; y < router->numChannelsY(0); y++) {
            double density = router->congest(x, y);
            if (density > maxval)
                maxval = density;
        }
    }
    double norm = (double)(LONGSPAN - 1) / maxval;

    // Draw destination pins as blue squares.
    for (int x = 0; x < router->numChannelsX(0); x++) {
        int xspc = (x + 1) * spacing - hspc;
        for (int y = 0; y < router->numChannelsY(0); y++) {
            int value = (int)(norm * router->congest(x, y));
            XSetForeground(dpy, gc, bluevector[value]);
            int yspc = height - (y + 1) * spacing - hspc;
            XFillRectangle(dpy, buffer, gc, xspc, yspc, spacing, spacing);
        }
    }

    // Cleanup
    delete [] Congestion;
}


void
mrX11Graphics::load_font()
{
    const char *fontname = "9x15";

    // Load font and get font information structure.

    if ((font_info = XLoadQueryFont (dpy,fontname)) == 0) {
        router->emitErrMesg("Cannot open 9x15 font\n");
        // exit(1);
    }
}


void
mrX11Graphics::createGC()
{
    unsigned long valuemask = 0; // ignore XGCvalues and use defaults.
    XGCValues values;
    unsigned int line_width = 1;
    int line_style = LineSolid;
    int cap_style = CapRound;
    int join_style = JoinRound;

    // Create default Graphics Context.

    gc = XCreateGC(dpy, win, valuemask, &values);

    // specify font

    if (font_info)
        XSetFont(dpy, gc, font_info->fid);

    // Specify black foreground since default window background is
    // white and default foreground is undefined.

    XSetForeground(dpy, gc, blackpix);

    // set line, fill attributes

    XSetLineAttributes(dpy, gc, line_width, line_style, cap_style,
        join_style);
    XSetFillStyle(dpy, gc, FillSolid);
    XSetArcMode(dpy, gc, ArcPieSlice);
}

