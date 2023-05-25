
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
 * QtInterf Graphical Interface Library                                   *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "qtcanvas_gl.h"
#include "polysplit.h"


using namespace qtinterf;

draw_gl_w::draw_gl_w(bool, QWidget *prnt) : QGLWidget(prnt)
{
    setMouseTracking(true);
    setMinimumWidth(50);
    setMinimumHeight(50);

//    setAutoBufferSwap(false);
}


void
draw_gl_w::initializeGL()
{
    glClearColor(0.0, 0.0, 0.0, 0.0);
}


void
draw_gl_w::resizeGL(int w, int h)
{
    glViewport(0, 0, (GLint)w, (GLint)h);
    glLoadIdentity();
    glOrtho(0, w-1, h-1, 0, -1.0, 1.0); 
    glClear(GL_COLOR_BUFFER_BIT);
//    glDrawBuffer(GL_BACK);
}


void
draw_gl_w::paintGL()
{
    /*
    if (resized) {
        resized = false;
        emit resize_event(&r_event);
    }
//        emit resize_event(&r_event);
    glReadBuffer(GL_BACK);
    glDrawBuffer(GL_FRONT);
    glCopyPixels(0, 0, width(), height(), GL_COLOR);
    glDrawBuffer(GL_BACK);
    */
//    swapBuffers();
}


//
// The drawing interface.
//

void
draw_gl_w::draw_direct(bool direct)
{
    (void)direct;
}


// Paint the screen window from the pixmap.
//
void
draw_gl_w::update()
{
    makeCurrent();
    swapBuffers();


/*
    makeCurrent();
    glReadBuffer(GL_BACK);
    glDrawBuffer(GL_FRONT);
    glCopyPixels(0, 0, width(), height(), GL_COLOR);
    glDrawBuffer(GL_BACK);
*/


}


// Flood with the background color.
//
void
draw_gl_w::clear()
{
    makeCurrent();
    glClearColor(da_bg.red()/255.0, da_bg.green()/255.0, da_bg.blue()/255.0,
        0.0);
    glClear(GL_COLOR_BUFFER_BIT);
}


void
draw_gl_w::clear_area(int x0, int y0, int w, int h)
{
    if (w <= 0)
        w = width() - x0;
    if (h <= 0)
        h = height() - y0;
    makeCurrent();
    glColor3ub(da_bg.red(), da_bg.green(), da_bg.blue());
    glRecti(x0, y0, x0+w, y0+h);
    glColor3ub(da_fg.red(), da_fg.green(), da_fg.blue());
}


// Set the foreground rendering color.
//
void
draw_gl_w::set_foreground(unsigned int pix)
{
    makeCurrent();
    da_fg.setRgb(pix);
    glColor3ub(da_fg.red(), da_fg.green(), da_fg.blue());
}


// Set the background rendering color.
//
void
draw_gl_w::set_background(unsigned int pix)
{
    da_bg.setRgb(pix);
}


// Draw one pixel.
//
void
draw_gl_w::draw_pixel(int x0, int y0)
{
    makeCurrent();
    glBegin(GL_POINTS);
    glVertex2i(x0, y0);
    glEnd();
}


// Draw multiple pixels.
//
void
draw_gl_w::draw_pixels(GRmultiPt *p, int n)
{
    if (n < 1)
        return;
    makeCurrent();
    glBegin(GL_POINTS);
    p->data_ptr_init();
    while (n--) {
        glVertex2i(p->data_ptr_x(), p->data_ptr_y());
        p->data_ptr_inc();
    }
    glEnd();
}


// Set the current line texture, used when da_line_mode = 0;
//
void
draw_gl_w::set_linestyle(const GRlineType *lstyle)
{
    makeCurrent();
    if (lstyle) {
        glEnable(GL_LINE_STIPPLE);
        glLineStipple(1, lstyle->mask);
        // stipple is 16 bits, lsb first
    }
    else
        glDisable(GL_LINE_STIPPLE);
}


// Draw a line.
//
void
draw_gl_w::draw_line(int x1, int y1, int x2, int y2)
{
    makeCurrent();
    glBegin(GL_LINES);
    glVertex2i(x1, y1);
    glVertex2i(x2, y2);
    glEnd();
}


// Draw a connected line set.
//
void
draw_gl_w::draw_polyline(GRmultiPt *p, int n)
{
    if (n < 2)
        return;
    makeCurrent();
    glBegin(GL_LINE_STRIP);
    p->data_ptr_init();
    while (n--) {
        glVertex2i(p->data_ptr_x(), p->data_ptr_y());
        p->data_ptr_inc();
    }
    glEnd();
}


// Draw multiple lines.
//
void
draw_gl_w::draw_lines(GRmultiPt *p, int n)
{
    if (n < 1)
        return;
    makeCurrent();
    glBegin(GL_LINES);
    p->data_ptr_init();
    while (n--) {
        glVertex2i(p->data_ptr_x(), p->data_ptr_y());
        p->data_ptr_inc();
        glVertex2i(p->data_ptr_x(), p->data_ptr_y());
        p->data_ptr_inc();
    }
    glEnd();
}


void
draw_gl_w::define_fillpattern(GRfillType *fillp)
{
    if (!fillp)
        return;
    if (fillp->xPixmap()) {
        delete [] fillp->xPixmap();
        fillp->setXpixmap(0);
    }
    if (fillp->hasMap())
        fillp->setXpixmap(fillp->newBitmap());
}


void
draw_gl_w::set_fillpattern(const GRfillType *fillp)
{
    makeCurrent();
    if (!fillp || !fillp->xPixmap())
        glDisable(GL_POLYGON_STIPPLE);
    else {
        glEnable(GL_POLYGON_STIPPLE);
        glPolygonStipple((GLubyte*)fillp->xPixmap());
    }
}


// Draw a box, using current fill.
//
void
draw_gl_w::draw_box(int x1, int y1, int x2, int y2)
{
    makeCurrent();
    if (x1 > x2) {
        int temp = x1;
        x1 = x2;
        x2 = temp;
    }
    if (y1 > y2) {
        int temp = y1;
        y1 = y2;
        y2 = temp;
    }
    x2++;
    y2++;
    glRecti(x1, y1, x2, y2);
}


// Draw multiple boxes, using current fill.
//
void
draw_gl_w::draw_boxes(GRmultiPt *p, int n)
{
    makeCurrent();
    p->data_ptr_init();
    for (int i = 0; i < n; i++) {
        int xx = p->data_ptr_x();
        int yy = p->data_ptr_y();
        p->data_ptr_inc();
        glRecti(xx, yy, xx + p->data_ptr_x() - 1, yy + p->data_ptr_y() - 1);
        p->data_ptr_inc();
    }
}


// Draw a filled arc.
//
void
draw_gl_w::draw_arc(int x0, int y0, int r, int, double a1, double a2)
{
    /*
    if (a1 >= a2)
        a2 = 2 * M_PI + a2;
    int t1 = (int)(16 * (180.0 / M_PI) * a1);
    int t2 = (int)(16 * (180.0 / M_PI) * a2 - t1);
    if (t2 == 0)
        return;
    int dim = 2*r;
    da_painter->drawPie(x0 - r, y0 - r, dim, dim, t1, t2);
    */
    (void)x0;
    (void)y0;
    (void)r;
    (void)a1;
    (void)a2;
}


static void
PathRemoveDupVerts(ginterf::GRmultiPt::pt_i *points, int *numpts)
{
    int i = *numpts - 1;
    ginterf::GRmultiPt::pt_i *p1, *p2;
    for (p1 = points, p2 = p1 + 1; i > 0; i--, p2++) {
        if (p1->x == p2->x && p1->y == p2->y) {
            (*numpts)--;
            continue;
        }
        p1++;
        if (p1 != p2)
            *p1 = *p2;
    }
}


// Draw a filled polygon.
//
void
draw_gl_w::draw_polygon(GRmultiPt *p, int n)
{
    GRmultiPt::pt_i *pts = new GRmultiPt::pt_i[n];
    p->data_ptr_init();
    for (int i = 0; i < n; i++) {
        pts[i].x = p->data_ptr_x();
        pts[i].y = p->data_ptr_y();
        p->data_ptr_inc();
    }

    PathRemoveDupVerts(pts, &n);

    if (n > 4 && !test_convex(pts, n)) {

        polysplit ps = polysplit(pts, n, 1);
        zoid_t *zl = ps.extract_zlist();
        while (zl) {
            if (zl->is_rect())
                glRecti(zl->xll, zl->yl, zl->xur, zl->yu);
            else {
                glBegin(GL_POLYGON);
                glVertex2i(zl->xll, zl->yl);
                glVertex2i(zl->xul, zl->yu);
                glVertex2i(zl->xur, zl->yu);
                glVertex2i(zl->xlr, zl->yl);
                glEnd();
            }
            zl = zl->next;
        }
        delete [] pts;
        return;
    }

    n--;
    if (n < 3) {
        delete [] pts;
        return;
    }
    makeCurrent();
    glBegin(GL_POLYGON);
    GRmultiPt::pt_i *pt = pts;
    while (n--) {
        glVertex2i(pt->x, pt->y);
        pt++;
    }
    glEnd();
    delete [] pts;
}


void
draw_gl_w::draw_zoid(int, int, int, int, int, int)
// draw_gl_w::draw_zoid(int yl, int yu, int xll, int xul, int xlr, int xur)
{
}


void
draw_gl_w::draw_image(const GRimage*, int, int, int, int)
//draw_gl_w::draw_image(const GRimage *image, int x, int y,
//    int width, int height)
{
}


// Set the font used for rendering in the drawing area.
//
void
draw_gl_w::set_font(QFont *fnt)
{
    setFont(*fnt);
}


// Return the pixel width of len characters in str when rendered with
// the given font.
//
int
draw_gl_w::text_width(QFont *fnt, const char *str, int len)
{
    QFontMetrics fm(*fnt);
    return (fm.width(QString(str), len));
}


// Return the width and height of the string as rendered in the
// current font.
//
void
draw_gl_w::text_extent(const char *str, int *w, int *h)
{
    if (!str)
        str = "x";
    if (w)
        *w = 0;
    if (h)
        *h = 0;
    const QFont &f = font();
    QFontMetrics fm(f);
    if (w)
        *w = fm.width(QString(str));
    if (h)
        *h = fm.height();
}


// Draw text, at most len characters from str if len >= 0.
//
void
draw_gl_w::draw_text(int x0, int y0, const char *str, int len)
{
    QString qs(str);
    if (len >= 0)
        qs.truncate(len);
    renderText(x0, y0, qs, font());
}


void
draw_gl_w::set_xor_mode(bool set)
{
    makeCurrent();
    if (set) {
        glDrawBuffer(GL_FRONT);
        glEnable(GL_COLOR_LOGIC_OP);
        glLogicOp(GL_XOR);
        glColor3ub(da_ghost_fg.red(), da_ghost_fg.green(), da_ghost_fg.blue());
    }
    else {
        glDrawBuffer(GL_BACK);
        glDisable(GL_COLOR_LOGIC_OP);
        glLogicOp(GL_COPY);
        glColor3ub(da_fg.red(), da_fg.green(), da_fg.blue());
    }
    glFlush();
}


void
draw_gl_w::set_ghost_color(unsigned int pixel)
{
    da_ghost.setRgb(pixel);
    da_ghost_fg.setRgb(pixel ^ da_bg.rgb());
}
// End of exported interface


void
draw_gl_w::resizeEvent(QResizeEvent *ev)
{
    QGLWidget::resizeEvent(ev);
    emit resize_event(ev);
}


void
draw_gl_w::mousePressEvent(QMouseEvent *ev)
{
    emit press_event(ev);
}


void
draw_gl_w::mouseReleaseEvent(QMouseEvent *ev)
{
    emit release_event(ev);
}


void
draw_gl_w::mouseMoveEvent(QMouseEvent *ev)
{
    emit move_event(ev);
}


void
draw_gl_w::keyPressEvent(QKeyEvent *ev)
{
    emit key_press_event(ev);
}


void
draw_gl_w::keyReleaseEvent(QKeyEvent *ev)
{
    emit key_release_event(ev);
}


void
draw_gl_w::enterEvent(QEvent *ev)
{
    emit enter_event(ev);
}


void
draw_gl_w::leaveEvent(QEvent *ev)
{
    emit leave_event(ev);
}


void
draw_gl_w::dragEnterEvent(QDragEnterEvent *ev)
{
    emit drag_enter_event(ev);
}


void
draw_gl_w::dropEvent(QDropEvent *ev)
{
    emit drop_event(ev);
}

