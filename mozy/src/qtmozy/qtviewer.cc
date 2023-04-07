
/*========================================================================
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
 * Qt MOZY help viewer.
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "qtviewer.h"
#include "qtfont.h"
#include "draw_qt_w.h"
#include "form_button_w.h"
#include "form_combo_w.h"
#include "form_list_w.h"
#include "form_file_w.h"
#include "interval_timer.h"

#include "help/help_startup.h"
#include "htm/htm_format.h"

#include <QBitmap>
#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPainter>
#include <QPushButton>
#include <QResizeEvent>
#include <QScrollBar>
#include <QTextEdit>


using namespace qtinterf;

//-----------------------------------------------------------------------------
// Special widgets for forms

form_button_w::form_button_w(htmForm *entry, QWidget *prnt) :
    QPushButton(prnt)
{
    form_entry = entry;
    setAutoDefault(false);

    QFontMetrics fm(font());
    if (entry->type == FORM_RESET || entry->type == FORM_SUBMIT) {
        const char *str = entry->value ? entry->value : entry->name;
        if (!str || !*str)
            str = "X";
        else
            setText(QString(str));
        entry->width = fm.width(QString(str)) + 4;
        entry->height = fm.height() + 4;
    }
    else {
        entry->width = fm.width(QString("X")) + 4;
        entry->height = entry->width;
        setCheckable(true);
        setChecked(entry->checked);
    }
    setFixedSize(QSize(entry->width, entry->height));

    connect(this, SIGNAL(pressed()), this, SLOT(pressed_slot()));
    connect(this, SIGNAL(released()), this, SLOT(released_slot()));
}


void
form_button_w::pressed_slot()
{
    if (form_entry->type == FORM_RADIO) {
        // get start of this radiobox
        htmForm *tmp;
        for (tmp = form_entry->parent->components; tmp;
                tmp = tmp->next)
            if (tmp->type == FORM_RADIO &&
                    !(strcasecmp(tmp->name, form_entry->name)))
                break;

        if (tmp == 0)
            return;

        // unset all other toggle buttons in this radiobox
        for ( ; tmp != 0; tmp = tmp->next) {
            if (tmp->type == FORM_RADIO && tmp != form_entry) {
                if (!strcasecmp(tmp->name, form_entry->name)) {
                    // same group, unset it
                    form_button_w *btn = (form_button_w*)tmp->widget;
                    btn->setChecked(false);
                }
                else
                    // Not a member of this group, we processed all
                    // elements in this radio box, break out.
                    break;
            }
        }
        form_entry->checked = true;
    }
    emit pressed(form_entry);
}


void
form_button_w::released_slot()
{
    if (form_entry->type == FORM_RADIO)
        setChecked(true);
}


form_combo_w::form_combo_w(htmForm *entry, QWidget *prnt) :
    QComboBox(prnt)
{
    form_entry = entry;
    setEditable(false);
}


void
form_combo_w::setSize()
{
    QFontMetrics fm(font());
    form_entry->height = form_entry->size * fm.height();
    form_entry->width = 0;
    for (htmForm *f = form_entry->options; f; f = f->next) {
        unsigned int w = fm.width(QString(f->name));
        if (w > form_entry->width)
            form_entry->width = w;
    }
    form_entry->width += 30;  // drop button
    setFixedSize(QSize(form_entry->width, form_entry->height));
}


form_list_w::form_list_w(htmForm *entry, QWidget *prnt) :
    QListWidget(prnt)
{
    form_entry = entry;
}


void
form_list_w::setSize()
{
    QFontMetrics fm(font());
    form_entry->height = form_entry->size * fm.height();
    form_entry->width = 0;
    for (htmForm *f = form_entry->options; f; f = f->next) {
        unsigned int w = fm.width(QString(f->name));
        if (w > form_entry->width)
            form_entry->width = w;
    }
    form_entry->width += 30;  // scrollbar
    setFixedSize(QSize(form_entry->width, form_entry->height));
}


//-----------------------------------------------------------------------------
// The viewer widget, public methods

viewer_w::viewer_w(int wid, int hei, htmDataInterface *dta, QWidget *prnt) :
    QScrollArea(prnt), htmWidget(this, dta), btn_timer(this)
{
    width_hint = wid;
    height_hint = hei;
    darea = new draw_qt_w(false, this);
    rband = 0;
    transact = 0;
    setWidget(darea);
    frame_style = frameStyle();

    connect(darea, SIGNAL(press_event(QMouseEvent*)),
        this, SLOT(press_event_slot(QMouseEvent*)));
    connect(darea, SIGNAL(release_event(QMouseEvent*)),
        this, SLOT(release_event_slot(QMouseEvent*)));
    connect(darea, SIGNAL(move_event(QMouseEvent*)),
        this, SLOT(move_event_slot(QMouseEvent*)));

    timers = 0;

    // timer to identify button clicks
    btn_pressed = false;
    btn_timer.setInterval(250);
    btn_timer.setSingleShot(true);
    connect(&btn_timer, SIGNAL(timeout()), this, SLOT(btn_timer_slot()));
}


//-----------------------------------------------------------------------------
// General interface (ViewerWidget function implementations)

void
viewer_w::set_transaction(Transaction *t, const char*)
{
    transact = t;
}


Transaction *
viewer_w::get_transaction()
{
    return (transact);
}


bool
viewer_w::check_halt_processing(bool)
{
    return (false);
}


void
viewer_w::set_halt_proc_sens(bool)
{
}


void
viewer_w::set_status_line(const char*)
{
}


htmImageInfo *
viewer_w::new_image_info(const char *url, bool progressive)
{
    htmImageInfo *image = new htmImageInfo();
    if (progressive)
        image->options = IMAGE_PROGRESSIVE | IMAGE_ALLOW_SCALE;
    else
        image->options = IMAGE_DELAYED;
    image->url = lstring::copy(url);
    return (image);
}


const char *
viewer_w::get_url()
{
    return (0);
}


bool
viewer_w::no_url_cache()
{
    return (true);
}


int
viewer_w::image_load_mode()
{
    return (HlpParams::LoadSync);
}


int
viewer_w::image_debug_mode()
{
    return (HlpParams::LInormal);
}


GRwbag *
viewer_w::get_widget_bag()
{
    return (0);
}


//-----------------------------------------------------------------------------
// Misc. public functions

// Set the proportional font used.  The name is in the form
// "face [style keywords] [size]".  The style keywords are ignored. 
//
void
viewer_w::set_font(const char *fontspec)
{
    char *family;
    int sz;
    FC.parse_freeform_font_string(fontspec, &family, 0, &sz, 0);
    setFontFamily(family, sz);
    delete [] family;
}


void
viewer_w::set_fixed_font(const char *fontspec)
{
    char *family;
    int sz;
    FC.parse_freeform_font_string(fontspec, &family, 0, &sz, 0);
    setFixedFontFamily(family, sz);
    delete [] family;
}


void
viewer_w::hide_drawing_area(bool hd)
{
    if (hd) {
        darea->hide();
        frame_style = frameStyle();
        setFrameStyle(0);
        setEnabled(false);
    }
    else {
        darea->show();
        setFrameStyle(frame_style);
        setEnabled(true);
    }
}


//-----------------------------------------------------------------------------
// Rendering interface (htmInterface function implementations)

// Resize the drawing area.
//
void
viewer_w::tk_resize_area(int wid, int hei)
{
    QRect r = contentsRect();
    if (wid < r.width())
        wid = r.width();
    if (hei < r.height())
        hei = r.height();
    r.setWidth(r.width() + verticalScrollBar()->width());
    r.setHeight(r.height() + horizontalScrollBar()->height());
    if (hei <= r.height() && verticalScrollBar()->isVisible()) {
        if (wid < r.width())
            wid = r.width();
    }
    if (wid <= r.width() && horizontalScrollBar()->isVisible()) {
        if (hei < r.height())
            hei = r.height();
    }

    QSize qda(wid, hei);
    if (qda != darea->size())
        darea->resize(wid, hei);
}


// Blit the area from the pixmap.
//
void
viewer_w::tk_refresh_area(int xx, int yy, int w, int h)
{
    darea->repaint(xx, yy, w, h);
}


// Return the size of the viewport.  The widget will use the width for
// formatting, and resize the drawing area to the actual formatted
// size.
//
void
viewer_w::tk_window_size(htmInterface::WinRetMode mode,
    unsigned int *wp, unsigned int *hp)
{
    if (mode == htmInterface::VIEWABLE) {
        // Return the total available viewable size, assuming no
        // scrollbars.
        QRect r = contentsRect();
        QScrollBar *sb = verticalScrollBar();
        if (sb && sb->isVisible())
            r.setWidth(r.width() + sb->width());
        sb = horizontalScrollBar();
        if (sb && sb->isVisible())
            r.setHeight(r.height() + sb->height());
        *wp = r.width();
        *hp = r.height();
    }
    else {
        // Return the size of the drawing pixmap.
        *wp = darea->width();
        *hp = darea->height();
    }
}


unsigned int
viewer_w::tk_scrollbar_width()
{
    QScrollBar *sb = verticalScrollBar();
    if (sb)
        return (sb->width());
    return (0);
}


// Set/unset a special cursor when the pointer is over an anchor.
//
void
viewer_w::tk_set_anchor_cursor(bool set)
{
    if (set)
        darea->setCursor(QCursor(Qt::PointingHandCursor));
    else
        darea->unsetCursor();
}


// Create a new timer.
//
unsigned int
viewer_w::tk_add_timer(int(*cb)(void*), void *arg)
{
    timers = new interval_timer(cb, arg, timers, this);
    timers->register_list(&timers);
    return (timers->id());
}


// Remove a timer.
//
void
viewer_w::tk_remove_timer(int id)
{
    for (interval_timer *t = timers; t; t = t->nextTimer()) {
        if (t->id() == id) {
            delete t;
            break;
        }
    }
}


// Start a timer, this is not done upon creation.  The timer will fire
// once only, but can be restarted.
//
void
viewer_w::tk_start_timer(unsigned int id, int msec)
{
    for (interval_timer *t = timers; t; t = t->nextTimer()) {
        if (t->id() == (int)id) {
            t->start(msec);
        }
    }
}


void
viewer_w::tk_claim_selection(const char*)
{
}


// Allocate a new font.  The "family" is a QT face name.
//
htmFont *
viewer_w::tk_alloc_font(const char *family, int sz, unsigned char sty)
{
    QFont *xfont = new QFont(QString(family), sz);
    if (sty & FONT_FIXED)
        xfont->setFixedPitch(true);
    if (sty & FONT_BOLD)
        xfont->setBold(true);
    if (sty & FONT_ITALIC)
        xfont->setItalic(true);

    htmFont *fnt = new htmFont(this, family, sz, sty);

    // default items
    fnt->xfont = xfont;

    QFontMetrics fm(*xfont);

    fnt->ascent = fm.ascent();
    fnt->descent = fm.descent();
    fnt->width = fm.maxWidth();
    fnt->lbearing = fm.width(QChar(' '));
    fnt->rbearing = 0;
    fnt->height = fm.height();
    fnt->lineheight = fm.lineSpacing();

    fnt->isp = fm.width(QChar(' '));
    fnt->sup_yoffset = (int)(fnt->ascent  * -.4);
    fnt->sub_yoffset = (int)(fnt->descent * .8);

    fnt->ul_offset = fm.underlinePos();
    fnt->ul_thickness = 1;
    fnt->st_offset = fm.strikeOutPos();
    fnt->st_thickness = 1;

    return (fnt);
}


// Delete a font.
//
void
viewer_w::tk_release_font(void *fntp)
{
    QFont *fnt = (QFont*)fntp;
    delete fnt;
}


// Set the font used for rendering in the drawing area.
//
void
viewer_w::tk_set_font(htmFont *fnt)
{
    darea->set_font((QFont*)fnt->xfont);
}


// Return the pixel width of len characters in str when rendered with
// the given font.
//
int
viewer_w::tk_text_width(htmFont *fnt, const char *str, int len)
{
    return (darea->text_width((QFont*)fnt->xfont, str, len));
}


// Return a code specifying the type of visual.
//
CCXmode
viewer_w::tk_visual_mode()
{
    QColormap::Mode m = QColormap::instance().mode();
    if (m == QColormap::Direct)
        return (MODE_TRUE);
    if (m == QColormap::Indexed)
        return (MODE_PALETTE);
    if (m == QColormap::Gray) {
        if (darea->depth() == 2)
            return (MODE_BW);
        else
            return (MODE_MY_GRAY);
    }
    return (MODE_UNDEFINED);
}


// Return the color depth in use.
//
int
viewer_w::tk_visual_depth()
{
    return (darea->depth());
}


// Create and return a new pixmap.
//
htmPixmap *
viewer_w::tk_new_pixmap(int w, int h)
{
    return ((htmPixmap*)new QPixmap(w, h));
}


// Destroy a pixmap.
//
void
viewer_w::tk_release_pixmap(htmPixmap *pmap)
{
    QPixmap *p = (QPixmap*)pmap;
    delete p;
}


// Create a pixmap, possibly with a masking bitmap, from the image info.
//
htmPixmap *
viewer_w::tk_pixmap_from_info(htmImage*, htmImageInfo *info,
    unsigned int *color_map)
{
    image_t image(info->width, info->height);

    unsigned int *dp = image.image_data();
    unsigned char *cp = info->data;

    unsigned int sz = info->width * info->height;
    for (unsigned int i = 0; i < sz; i++)
        *dp++ = color_map[(unsigned int)*cp++];

    return (new QPixmap(QPixmap::fromImage(image)));
}


// Switch the drawing context to the supplied pixmap, or back to the
// main pixmap if 0 is passed.
//
void
viewer_w::tk_set_draw_to_pixmap(htmPixmap *pixmap)
{
    darea->set_draw_to_pixmap((QPixmap*)pixmap);
}


// Return a bitmap created from the supplied data.
//
htmPixmap *
viewer_w::tk_bitmap_from_data(int w, int h, unsigned char *dta)
{
    if (!dta)
        return (0);
    QBitmap xp = QBitmap::fromData(QSize(w, h), dta,
        QImage::Format_MonoLSB);
    return ((htmPixmap*)(new QBitmap(xp)));
}


// Free the bitmap.
//
void
viewer_w::tk_release_bitmap(htmBitmap *bitmap)
{
    QBitmap *qbm = (QBitmap*)bitmap;
    delete qbm;
}


// Return a new image struct.
//
htmXImage *
viewer_w::tk_new_image(int w, int h)
{
    return ((htmXImage*)(new image_t(w, h)));
}


// Fill the image data from offset lo to hi-1 from the data array and
// supplied colormap.
//
void
viewer_w::tk_fill_image(htmXImage *ximage, unsigned char *dta,
    unsigned int *color_map, int lo, int hi)
{
    image_t *image = (image_t*)ximage;
    unsigned int *dp = image->image_data() + lo;
    unsigned char *cp = dta + lo;

    for (int i = lo; i < hi; i++)
        *dp++ = color_map[(unsigned int)*cp++];
}


// Draw the xi,yi,wi,hi rectangle in the image at xw,yw.
//
void
viewer_w::tk_draw_image(int xw, int yw, htmXImage *image, int xi, int yi,
    int wi, int hi)
{
    darea->draw_image(xw, yw, (QImage*)image, xi, yi, wi, hi);
}


void
viewer_w::tk_release_image(htmXImage *image)
{
    image_t *im = (image_t*)image;
    delete im;
}


// Set the foreground rendering color.
//
void
viewer_w::tk_set_foreground(unsigned int pix)
{
    darea->set_foreground(pix);
}


// Set the background rendering color.
//
void
viewer_w::tk_set_background(unsigned int pix)
{
    darea->set_background(pix);
}


// Message handler to shut up the warning message issued when
// QColor::setNamedColor fails to resolve the color.
//
static void
messageOutput(QtMsgType type, const char *msg)
{
    switch (type) {
    case QtDebugMsg:
    case QtWarningMsg:
        break;
    case QtCriticalMsg:
        fprintf(stderr, "Critical: %s\n", msg);
        break;
    case QtFatalMsg:
        fprintf(stderr, "Fatal: %s\n", msg);
        abort();
    }
}


// Parse the color specification string and if successful return the
// rgb components and pixel value in c.
//
bool
viewer_w::tk_parse_color(const char *name, htmColor *c)
{
    QColor q;
    qInstallMsgHandler(messageOutput);
    q.setNamedColor(name);
    qInstallMsgHandler(0);
    if (!q.isValid())
        return (false);
    c->red = q.red();
    c->green = q.green();
    c->blue = q.blue();
    c->pixel = q.rgba();
    return (true);
}


// Allocate a pixel for the rgb given in c, return the pixel in c. 
// This isn't needed in true-color displays.
//
bool
viewer_w::tk_alloc_color(htmColor *c)
{
    QColor q(c->red, c->green, c->blue);
    c->pixel = q.rgba();
    return (true);
}


// Fill in the r,g,b values fron the pixel initially set in htmColor,
// c is an array of num structs.
//
int
viewer_w::tk_query_colors(htmColor *c, unsigned int num)
{
    for (unsigned int i = 0; i < num; i++) {
        QColor q(c[i].pixel);
        if (q.isValid()) {
            c[i].red = q.red();
            c[i].green = q.green();
            c[i].blue = q.blue();
        }
        else {
            c[i].red = 0;
            c[i].green = 0;
            c[i].blue = 0;
        }
    }
    return (num);
}


void
viewer_w::tk_free_colors(unsigned int*, unsigned int)
{
}


// Fill an array of pixel values correspondint to the colors in the
// r,g,b arrays.
//
bool
viewer_w::tk_get_pixels(unsigned short *r, unsigned short *g,
    unsigned short *b, unsigned int num, unsigned int *pixels)
{
    for (unsigned int i = 0; i < num; i++) {
        QColor q(r[i], g[i], b[i]);
        pixels[i] = q.rgb();
    }
    return (true);
}


// In QT, a clip mask is stored in the QPixmap itself, so we don't
// have to worry setting origins, etc.

void
viewer_w::tk_set_clip_mask(htmPixmap *pix, htmBitmap *bits)
{
    if (pix && bits)
        ((QPixmap*)pix)->setMask(*(QBitmap*)bits);
}


void
viewer_w::tk_set_clip_origin(int, int)
{
}


void
viewer_w::tk_set_clip_rectangle(htmRect*)
{
}


// Switch between solid fill and pixmap tiling, both are accomplished
// with tk_draw_rectangle(true, ...).
//
void
viewer_w::tk_set_fill(htmInterface::FillMode mode)
{
    darea->set_fill((mode == htmInterface::TILED));
}


// Set the pixmap for use in tiling (see tk_set_fill).
//
void
viewer_w::tk_set_tile(htmPixmap *pixmap)
{
    darea->set_tile((QPixmap*)pixmap);
}


// Set the origin used when drawing tiled pixmaps.
//
void
viewer_w::tk_set_ts_origin(int xx, int yy)
{
    darea->set_tile_origin(xx, yy);
}


// Copy out the part of a pixmap (xp,yp,wp,hp) to xw,yw.
//
void
viewer_w::tk_draw_pixmap(int xw, int yw, htmPixmap *pmap,
    int xp, int yp, int wp, int hp)
{
    darea->draw_pixmap(xw, yw, (QPixmap*)pmap, xp, yp, wp, hp);
}


// Draw a solid or open rectangle, or pixmap tiles.
//
void
viewer_w::tk_draw_rectangle(bool filled, int xx, int yy, int w, int h)
{
    darea->draw_rectangle(filled, xx, yy, w, h);
}


// Switch between solid or dashed lines.
//
void
viewer_w::tk_set_line_style(htmInterface::FillMode styleid)
{
    darea->set_line_mode((styleid == htmInterface::TILED));
}


// Draw a line.
//
void
viewer_w::tk_draw_line(int x1, int y1, int x2, int y2)
{
    darea->draw_line(x1, y1, x2, y2);
}


// Draw text, at most len characters from str.
//
void
viewer_w::tk_draw_text(int xx, int yy, const char *str, int len)
{
    darea->draw_text(xx, yy, str, len);
}


// Draw a solid or open polygon.
//
void
viewer_w::tk_draw_polygon(bool filled, htmPoint *points, int numpts)
{
    darea->draw_polygon(filled, (QPoint*)points, numpts);
}


// Draw an arc or pie-slice.
//
void
viewer_w::tk_draw_arc(bool filled, int xx, int yy, int w, int h, int st,
    int sp)
{
    darea->draw_arc(filled, xx, yy, w, h, st, sp);
}


//
// Forms Handling
//

inline int
char_width(QWidget *w)
{
    QFont f = w->font();
    QFontMetrics fm(f);
    return (fm.width(QString("X")));
}

inline int
line_height(QWidget *w)
{
    QFont f = w->font();
    QFontMetrics fm(f);
    return (fm.height());
}


// Create the appropriate widget(s) for the entry.  The accurate
// width and height must be set/determined here.
//
void
viewer_w::tk_add_widget(htmForm *entry, htmForm *prnt)
{
    if (entry->type == FORM_IMAGE || entry->type == FORM_HIDDEN)
        return;

    switch (entry->type) {
    // text field, set args and create it
    case FORM_TEXT:
    case FORM_PASSWD:
        {
            QLineEdit *ed = new QLineEdit(darea);
            if (entry->maxlength != -1)
                ed->setMaxLength(entry->maxlength);

            entry->width = entry->size * char_width(ed) + 4;
            entry->height = line_height(ed);
            ed->resize(entry->width, entry->height);

            if (entry->type == FORM_TEXT && entry->value)
                ed->setText(QString(entry->value));
            if (entry->type == FORM_PASSWD)
                ed->setEchoMode(QLineEdit::Password);
            entry->widget = ed;
        }
        break;

    case FORM_CHECK:
    case FORM_RADIO:
        {
            QPushButton *cb = new form_button_w(entry, darea);
            entry->widget = cb;
        }
        break;

    case FORM_FILE:
        entry->widget = new form_file_w(entry, darea);
        break;
        
    case FORM_RESET:
    case FORM_SUBMIT:
        {
            QPushButton *btn = new form_button_w(entry, darea);
            entry->widget = btn;

            if (entry->type == FORM_SUBMIT)
                connect(btn, SIGNAL(pressed(htmForm*)),
                    this, SLOT(form_submit_slot(htmForm*)));
            else
                connect(btn, SIGNAL(pressed(htmForm*)),
                    this, SLOT(form_reset_slot(htmForm*)));
        }
        break;

    case FORM_SELECT:
        // multiple select or more than one item visible: it's a listbox
        if (entry->multiple || entry->size > 1) {
            form_list_w *lb = new form_list_w(entry, darea);
            entry->widget = lb;
        }
        else {
            form_combo_w *cb = new form_combo_w(entry, darea);
            entry->widget = cb;
        }
        break;

    case FORM_OPTION:
        // list box selection
        if (prnt->multiple || prnt->size > 1) {
            form_list_w *lb = (form_list_w*)prnt->widget;
            if (!lb)
                return;
            lb->addItem(QString(entry->name));

            // add this item to the list of selected items
            if (entry->selected)
                // single selection always takes the last inserted item
                lb->setItemSelected(lb->item(prnt->maxlength), true);
        }
        else {
            form_combo_w *cb = (form_combo_w*)prnt->widget;
            if (!cb)
                return;
            cb->addItem(QString(entry->name));

            // establish the selection
            if (entry->selected)
                cb->setCurrentIndex(prnt->maxlength);
        }
        break;

    case FORM_TEXTAREA:
        {
            QTextEdit *te = new QTextEdit(darea);
            if (entry->value)
                te->setPlainText(entry->value);
            entry->width = entry->size * char_width(te);
            entry->height = entry->maxlength * line_height(te);
            te->setFixedSize(entry->width, entry->height);
            entry->widget = te;
        }
        break;

    default:
        break;
    }
}


// Take care of sizing FORM_SELECT menus.
//
void
viewer_w::tk_select_close(htmForm *entry)
{
    if (entry->multiple || entry->size > 1) {
        form_list_w *lb = (form_list_w*)entry->widget;
        if (!lb)
            return;
        lb->setSize();
    }
    else {
        form_combo_w *cb = (form_combo_w*)entry->widget;
        if (!cb)
            return;
        cb->setSize();
    }
}


// Return the text entered into the text entry area.
//
char *
viewer_w::tk_get_text(htmForm *entry)
{
    switch (entry->type) {
    case FORM_PASSWD:
    case FORM_TEXT:
        {
            QLineEdit *ed = (QLineEdit*)entry->widget;
            if (ed)
                return (lstring::copy(ed->text().toAscii().constData()));
        }
        break;

    case FORM_FILE:
        {
            form_file_w *fb = (form_file_w*)entry->widget;
            if (fb)
                return (lstring::copy(
                    fb->editor()->text().toAscii().constData()));
        }
        break;

    case FORM_TEXTAREA:
        {
            QTextEdit *te = (QTextEdit*)entry->widget;
            if (te)
                return (lstring::copy(
                    te->toPlainText().toAscii().constData()));
        }
        break;

    default:
        break;
    }
    return (0);
}


// Set the text in the text entry area.
//
void
viewer_w::tk_set_text(htmForm *entry, const char *string)
{
    switch (entry->type) {
    case FORM_PASSWD:
    case FORM_TEXT:
        {
            QLineEdit *ed = (QLineEdit*)entry->widget;
            if (ed)
                ed->setText(QString(string));
        }
        break;

    case FORM_FILE:
        {
            form_file_w *fb = (form_file_w*)entry->widget;
            if (fb)
                fb->editor()->setText(QString(string));
        }
        break;

    case FORM_TEXTAREA:
        {
            QTextEdit *te = (QTextEdit*)entry->widget;
            if (te)
                te->setPlainText(QString(string));
        }
        break;

    default:
        break;
    }
}


// Return the state of the check box, or for a FORM_SELECT, set the
// checked field in the entries of seleted items.
//
bool
viewer_w::tk_get_checked(htmForm *entry)
{
    switch (entry->type) {
    case FORM_CHECK:
    case FORM_RADIO:
        {
            QCheckBox *cb = (QCheckBox*)entry->widget;
            if (cb)
                return (cb->isChecked());
        }
        break;
    case FORM_SELECT:
        if (entry->multiple || entry->size > 1) { 
            form_list_w *lb = (form_list_w*)entry->widget;
            if (!lb)
                return (false);
            int cnt = 0;
            for (htmForm *f = entry->options; f; f = f->next)
                f->checked = lb->isItemSelected(lb->item(cnt++));
        }
        else {
            form_combo_w *cb = (form_combo_w*)entry->widget;
            if (!cb)
                return (false);
            int cnt = 0;
            for (htmForm *f = entry->options; f; f = f->next)
                f->checked = (cnt++ == cb->currentIndex());
        }
        break;
    default:
        break;
    }
    return (false);
}


// Set the state of the check box, or for a FORM_SELECT, set the
// state of each entry according to the selected field.
//
void
viewer_w::tk_set_checked(htmForm *entry)
{
    switch (entry->type) {
    case FORM_CHECK:
    case FORM_RADIO:
        {
            QCheckBox *cb = (QCheckBox*)entry->widget;
            cb->setChecked(entry->selected);
        }
        break;
    case FORM_SELECT:
        if (entry->multiple || entry->size > 1) { 
            form_list_w *lb = (form_list_w*)entry->widget;
            if (!lb)
                return;
            int cnt = 0;
            for (htmForm *f = entry->options; f; f = f->next) {
                lb->setItemSelected(lb->item(cnt++), f->selected);
                cnt++;
            }
        }
        else {
            form_combo_w *cb = (form_combo_w*)entry->widget;
            if (!cb)
                return;
            int cnt = 0;
            for (htmForm *f = entry->options; f; f = f->next) {
                if (f->selected)
                    cb->setCurrentIndex(cnt);
                cnt++;
            }
        }
        break;
    default:
        break;
    }
}


// Set the widget locations and visibility.
//
void
viewer_w::tk_position_and_show(htmForm *entry, bool shw)
{
    if (entry->widget) {
        if (shw) {
            if (entry->mapped) {
                QWidget *w = (QWidget*)entry->widget;
                w->show();
            }
        }
        else {
            QWidget *w = (QWidget*)entry->widget;
            w->hide();
            if (htm_viewarea.intersect(entry->data->area)) {
                int xv = viewportX(entry->data->area.x);
                int yv = viewportY(entry->data->area.y);
                w->move(xv, yv);
                entry->mapped = true;
            }
            else
                entry->mapped = false;
        }
    }
}


// Destroy the form widget, this removes it from the screen.
//
void
viewer_w::tk_form_destroy(htmForm *entry)
{
    if (entry->widget) {
        QWidget *w = (QWidget*)entry->widget;
        entry->widget = 0;
        delete w;
    }
}


//-----------------------------------------------------------------------------
// viewer_w slots

void
viewer_w::press_event_slot(QMouseEvent *ev)
{
    switch (ev->button()) {
    case Qt::LeftButton:
        extendStart(ev, 1, ev->x(), ev->y());
        break;
    case Qt::MidButton:
        extendStart(ev, 2, ev->x(), ev->y());
        break;
    case Qt::RightButton:
        extendStart(ev, 3, ev->x(), ev->y());
        break;
    default:
        return;
    } 
    btn_pressed = true;
    btn_timer.start();
}


void
viewer_w::release_event_slot(QMouseEvent *ev)
{
    delete rband;
    rband = 0;

    QRect r = contentsRect();
    QScrollBar *sb = horizontalScrollBar();
    if (sb)
        r.moveLeft(sb->value());
    sb = verticalScrollBar();
    if (sb)
        r.moveTop(sb->value());

    if (r.contains(ev->x(), ev->y())) {
        switch (ev->button()) {
        case Qt::LeftButton:
            extendEnd(ev, 1, btn_pressed, ev->x(), ev->y());
            break;
        case Qt::MidButton:
            extendEnd(ev, 2, btn_pressed, ev->x(), ev->y());
            break;
        case Qt::RightButton:
            extendEnd(ev, 3, btn_pressed, ev->x(), ev->y());
            break;
        default:
            return;
        } 
    }
    btn_pressed = false;
}


void
viewer_w::move_event_slot(QMouseEvent *ev)
{
    QRect r = contentsRect();
    QScrollBar *sb = horizontalScrollBar();
    if (sb)
        r.moveLeft(sb->value());
    sb = verticalScrollBar();
    if (sb)
        r.moveTop(sb->value());

    if (r.contains(ev->x(), ev->y())) {
        if (rband) {
            rband->show();
            int lastx = viewportX(htm_press_x);
            int lasty = viewportY(htm_press_y);
            int xx = ev->x() < lastx ? ev->x() : lastx;
            int yy = ev->y() < lasty ? ev->y() : lasty;

            // hmmm, have to use QT's viewport
            sb = horizontalScrollBar();
            if (sb)
                xx -= sb->value();
            sb = verticalScrollBar();
            if (sb)
                yy -= sb->value();

            QRect rb(xx, yy, abs(ev->x() - lastx), abs(ev->y() - lasty));
            rband->setGeometry(rb);
            return;
        }
        anchorTrack(ev, ev->x(), ev->y());
    }
    else if (rband)
        rband->hide();
}


// This is called a short time after a button press.  It signals the
// end of a "click" and starts the rectangle sprite for selection if
// the mouse button is still down.
//
void
viewer_w::btn_timer_slot()
{
    if (btn_pressed)
        rband = new QRubberBand(QRubberBand::Rectangle, this);
    btn_pressed = false;
}


void
viewer_w::form_submit_slot(htmForm *entry)
{
    formActivate(0, entry);
}


void
viewer_w::form_reset_slot(htmForm *entry)
{
    formReset(entry);
}


//-----------------------------------------------------------------------------
// viewer_w protected handler functions

void
viewer_w::resizeEvent(QResizeEvent *ev)
{
    QScrollArea::resizeEvent(ev);

    // If the size increases, resize the drawing area immediately so it
    // won't look strange while reformatting.  The reformatting will
    // resize it again.
    QSize dasize = darea->size();
    if (dasize.width() < ev->size().width())
        dasize.setWidth(ev->size().width());
    if (dasize.height() < ev->size().height())
        dasize.setHeight(ev->size().height());
    if (dasize != darea->size())
        darea->resize(dasize);

    htmWidget::resize();
}

