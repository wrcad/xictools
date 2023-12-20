
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

#include "qtinterf.h"
#include "qtfont.h"
#include "miscutil/lstring.h"

#include <QAction>
#include <QFont>
#include <QFontInfo>
#include <QFontDatabase>
#include <QFontMetrics>
#include <QListWidget>
#include <QLayout>
#include <QLabel>
#include <QGroupBox>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>


namespace { QTfont _qt_font_; }
GRfont &FC = _qt_font_;

GRfont::fnt_t GRfont::app_fonts[] =
{
    fnt_t( 0, 0, false, false ),  // not used
    fnt_t( "Fixed Pitch Text Window Font",    0, true, false ),
    fnt_t( "Proportional Text Window Font",   0, false, false ),
    fnt_t( "Fixed Pitch Drawing Window Font", 0, true, false ),
    fnt_t( "Text Editor Font",                0, true, false ),
    fnt_t( "HTML Viewer Proportional Font",   0, false, true ),
    fnt_t( "HTML Viewer Fixed Pitch Font",    0, true, true )
};

int GRfont::num_app_fonts =
    sizeof(GRfont::app_fonts)/sizeof(GRfont::app_fonts[0]);

#ifdef __APPLE__
#define DEF_SIZE 11
#define DEF_FIXED_FACE "Menlo"
#define DEF_PROP_FACE "Arial"
#else
#ifdef WIN32
#define DEF_SIZE 9
#define DEF_FIXED_FACE "Menlo"
#define DEF_PROP_FACE "Arial"
#else
#define DEF_SIZE 9
#define DEF_FIXED_FACE "Courier New"
#define DEF_PROP_FACE "Helvetica"
#endif
#endif

QTfont *QTfont::instancePtr = 0;

QTfont::QTfont()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class QTfont already instantiated.\n");
        exit(1);
    }
    instancePtr = this;
}


QTfont::~QTfont()
{
    instancePtr = 0;
}


// This sets the default font names and sizes.
//
void
QTfont::initFonts()
{
    // Just call this once.
    if (app_fonts[0].default_fontname != 0)
        return;

    // Do better than this?
    int def_size = DEF_SIZE;

    char buf[80];
    snprintf(buf, 80, "%s %d", DEF_FIXED_FACE, def_size);
    char *def_fx_font_name = lstring::copy(buf);
    snprintf(buf, 80, "%s %d", DEF_PROP_FACE, def_size);
    char *def_pr_font_name = lstring::copy(buf);

    // Poke in the names.
    for (int i = 0; i < num_app_fonts; i++) {
        if (app_fonts[i].fixed)
            app_fonts[i].default_fontname = def_fx_font_name;
        else
            app_fonts[i].default_fontname = def_pr_font_name;
    }
}


GRfontType
QTfont::getType()
{
    return (GRfontQ);
}


// Set the name for the font fnum, invalidating any present font for
// that index.
//
void
QTfont::setName(const char *name, int fnum)
{
    if (fnum > 0 && fnum < num_app_fonts) {
        if (!name || !*name) {
            if (fonts[fnum].name) {
                delete [] fonts[fnum].name;
                fonts[fnum].name = 0;
                QFont *newfont = new_font(app_fonts[fnum].default_fontname,
                    isFixed(fnum));
                delete fonts[fnum].font;
                fonts[fnum].font = newfont;
                emit fontChanged(fnum);
            }
        }
        else if (!fonts[fnum].name || strcmp(fonts[fnum].name, name)) {
            QFont *newfont = new_font(name, isFixed(fnum));
            if (newfont) {
                delete [] fonts[fnum].name;
                fonts[fnum].name = lstring::copy(name);
                delete fonts[fnum].font;
                fonts[fnum].font = newfont;
                emit fontChanged(fnum);
            }
        }
    }
}


// Return the saved name of the font associated with fnum.
//
const char *
QTfont::getName(int fnum)
{
    if (fnum > 0 && fnum < num_app_fonts) {
        if (fonts[fnum].name)
            return (fonts[fnum].name);
        return (app_fonts[fnum].default_fontname);
    }
    return (0);
}


// Return the font family name, as used in the help viewer.
//
char *
QTfont::getFamilyName(int fnum)
{
    if (fnum > 0 && fnum < num_app_fonts) {
        char *family;
        int sz;
        parse_freeform_font_string(fonts[fnum].name, &family, 0, &sz, 0);
        if (family) {
            int len = strlen(family) + 8;
            char *str = new char[len];
            snprintf(str, len, "%s %d", family, sz);
            delete [] family;
            return (str);
        }
    }
    return (0);
}


bool
QTfont::getFont(void *fontp, int fnum)
{
    QFont **font = (QFont**)fontp;
    if (font)
        *font = 0;
    if (fnum > 0 && fnum < num_app_fonts) {
        if (fonts[fnum].font) {
            if (font)
                *font = fonts[fnum].font;
            return (true);
        }
        fonts[fnum].font = new_font(app_fonts[fnum].default_fontname,
            isFixed(fnum));
        if (fonts[fnum].font) {
            if (font)
                *font = fonts[fnum].font;
            return (true);
        }
    }
    return (false);
}


// Register a font-change callback.
// DO NOT USE, connect to the fontChanged signal instead.
//
void
QTfont::registerCallback(void *pwidget, int fnum)
{
    QWidget *widget = (QWidget*)pwidget;
    if (fnum < 1 || fnum >= num_app_fonts)
        return;
    fonts[fnum].cbs = new FcbRec(widget, fonts[fnum].cbs);
}


// Unregister a font-change callback.  The widget is the dialog parent of
// the text widget recorded.
// DO NOT USE, connect to the fontChanged signal instead.
//
void
QTfont::unregisterCallback(void *pwidget, int fnum)
{
    QWidget *widget = (QWidget*)pwidget;
    if (fnum < 1 || fnum >= num_app_fonts)
        return;
    FcbRec *fp = 0, *fn;
    for (FcbRec *f = fonts[fnum].cbs; f; f = fn) {
        fn = f->next;
        if (widget == f->widget->parentWidget()) {
            if (fp)
                fp->next = fn;
            else
                fonts[fnum].cbs = fn;
            delete f;
            continue;
        }
        fp = f;
    }
}


// Create a font for the string given in name, which has the form
// "family [style keywords] [size]".
//
QFont *
QTfont::new_font(const char *name, bool fixed)
{
    char *family;
    stringlist *style;
    char *sty = 0;
    int size;
    parse_freeform_font_string(name, &family, &style, &size);
    if (!family) {
        if (fixed)
            family = lstring::copy("courier");
        else
            family = lstring::copy("helvetica");
    }
    if (style) {
        sty = stringlist::flatten(style, " ");
        char *t = sty + strlen(sty) - 1;
        if (*t == ' ')
            *t = 0;
    }
    QFont *font = new QFont(QString(family), size);
    delete family;
    font->setStyleHint(fixed ? QFont::TypeWriter : QFont::Helvetica);
    font->setFixedPitch(fixed);
    if (sty)
        font->setStyleName(sty);
/*
    for (stringlist *s = style; s; s = s->next) {
        if (!strcasecmp(s->string, "bold"))
            font->setBold(true);
        else if (!strcasecmp(s->string, "italic"))
            font->setItalic(true);
    }
*/
    stringlist::destroy(style);

    // Reset the critical info in case there was not a perfect match.
    QFontInfo fi(*font);
    font->setFamily(fi.family());
    font->setPointSize(fi.pointSize());
    return (font);
}


// Static function.
// Return the string width/height for the font index provided.
//
bool
QTfont::stringBounds(const char *string, int fnum, int *w, int *h)
{
    QFont *f;
    if (FC.getFont(&f, fnum)) {
        QFontMetrics fm(*f);
        if (!string)
            string = "X";
        if (w)
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
            *w = fm.horizontalAdvance(string);
#else
            *w = fm.width(string);
#endif
        if (h)
            *h = fm.height();
        return (true);
    }
    return (false);
}


// Static function.
// Return the string width/height for the widget provided.
//
bool
QTfont::stringBounds(const char *string, const QWidget *widget, int *w, int *h)
{
    if (widget) {
        QFontMetrics fm = widget->fontMetrics();
        if (!string)
            string = "X";
        if (w)
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
            *w = fm.horizontalAdvance(string);
#else
            *w = fm.width(string);
#endif
        if (h)
            *h = fm.height();
        return (true);
    }
    return (false);
}


// Static function.
// Return the string width for the font index provided.
//
int
QTfont::stringWidth(const char *string, int fnum)
{
    QFont *f;
    if (FC.getFont(&f, fnum)) {
        QFontMetrics fm(*f);
        if (!string)
            string = "X";
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
        return (fm.horizontalAdvance(string));
#else
        return (fm.width(string));
#endif
    }
    return (8*strlen(string));
}


// Static function.
// Return the string width for the widget provided.
//
int
QTfont::stringWidth(const char *string, const QWidget *widget)
{
    if (widget) {
        if (!string)
            string = "X";
        QFontMetrics fm = widget->fontMetrics();
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
        return (fm.horizontalAdvance(string));
#else
        return (fm.width(string));
#endif
    }
    return (8*strlen(string));
}


// Static function.
// Return the line height for the font index provided.
//
int
QTfont::lineHeight(int fnum)
{
    QFont *f;
    if (FC.getFont(&f, fnum)) {
        QFontMetrics fm(*f);
        return (fm.height());
    }
    return (16);
}


// Static function.
// Return the line height for the widget provided.
//
int
QTfont::lineHeight(const QWidget *widget)
{
    if (widget) {
        QFontMetrics fm = widget->fontMetrics();
        return (fm.height());
    }
    return (16);
}


// Private function to refresh the text widgets.
//
void
QTfont::refresh(int fnum)
{
    (void)fnum;
    // not used
}


// Private static error exit.
//
void
QTfont::on_null_ptr()
{
    fprintf(stderr, "Singleton class QTfont used before instantiated.\n");
    exit(1);
}


//-----------------------------------------------------------------------------
// Font Selection Dialog

#define PREVIEW_STRING "abcdefghijk ABCDEFGHIJK 0123456789"

namespace {
    class font_list_widget : public QListWidget
    {
    public:
        font_list_widget(int w, QWidget *prnt=0) : QListWidget(prnt)
        {
            pref_width = w;
            QSizePolicy p = sizePolicy();
            p.setHorizontalPolicy(QSizePolicy::Expanding);
            p.setVerticalPolicy(QSizePolicy::Expanding);
            setSizePolicy(p);
        }

        QSize sizeHint() const
        {
            QSize qs = QListWidget::sizeHint();
            qs.setWidth(pref_width);
            return (qs);
        }

        QSize minimumSizeHint() const { return (sizeHint()); }

    private:
        int pref_width;
    };

    class preview_widget : public QTextEdit
    {
    public:
        preview_widget(QWidget *prnt = 0) : QTextEdit(prnt) { }

        QSize sizeHint() const
        {
            QSize qs = QTextEdit::sizeHint();
            qs.setHeight(50);
            return (qs);
        }

        QSize minimumSizeHint() const { return (sizeHint()); }
    };
}


#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
#else
namespace {
    QFontDatabase *fdb;
}
#endif


QTfontDlg *QTfontDlg::activeFontSels[4];

QTfontDlg::QTfontDlg(QTbag *owner, int indx, void *arg) :
    QDialog(owner ? owner->Shell() : 0)
{
    p_parent = owner;
    p_cb_arg = arg;

    for (int i = 0; i < 4; i++) {
        if (!activeFontSels[i]) {
            activeFontSels[i] = this;
            break;
        }
    }
    if (owner)
        owner->MonitorAdd(this);

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
#else
    if (!fdb)
        fdb = new QFontDatabase();
#endif

    setWindowTitle(QString(tr("Font Selection")));
    setAttribute(Qt::WA_DeleteOnClose);

    QMargins qmtop(2, 2, 2, 2);
    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(4);

    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qmtop);
    hbox->setSpacing(2);

    QVBoxLayout *vb = new QVBoxLayout();
    hbox->addLayout(vb);
    vb->setContentsMargins(qmtop);
    QLabel *label = new QLabel(tr("Faces"));
    vb->addWidget(label);
    ft_face_list = new font_list_widget(180);
    connect(ft_face_list,
        SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this,
        SLOT(face_changed_slot(QListWidgetItem*, QListWidgetItem*)));
    vb->addWidget(ft_face_list);

    vb = new QVBoxLayout();
    hbox->addLayout(vb);
    vb->setContentsMargins(qmtop);
    label = new QLabel(tr("Styles"));
    vb->addWidget(label);
    ft_style_list = new font_list_widget(120);
    connect(ft_style_list,
        SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this,
        SLOT(style_changed_slot(QListWidgetItem*, QListWidgetItem*)));
    vb->addWidget(ft_style_list);

    vb = new QVBoxLayout();
    hbox->addLayout(vb);
    vb->setContentsMargins(qmtop);
    label = new QLabel(tr("Sizes"));
    vb->addWidget(label);
    ft_size_list = new font_list_widget(60);
    connect(ft_size_list,
        SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this,
        SLOT(size_changed_slot(QListWidgetItem*, QListWidgetItem*)));
    vb->addWidget(ft_size_list);

    QGroupBox *gb = new QGroupBox(tr("Preview"));
    vbox->addWidget(gb);
    vb = new QVBoxLayout(gb);
    vb->setContentsMargins(qmtop);
    vb->setSpacing(2);
    ft_preview = new preview_widget();
    vb->addWidget(ft_preview);

    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qmtop);
    hbox->setSpacing(2);
    ft_apply = new QPushButton(tr("Apply"));
    hbox->addWidget(ft_apply);
    ft_apply->setAutoDefault(false);
    connect(ft_apply, SIGNAL(clicked()), this, SLOT(action_slot()));

    ft_menu = new QComboBox(this);
    hbox->addWidget(ft_menu);
    QSize qs = ft_apply->sizeHint();
    ft_menu->setMinimumHeight(qs.height());
    ft_menu->setEditable(false);
    connect(ft_menu, SIGNAL(activated(int)), this, SLOT(menu_choice_slot(int)));
    if (indx <= 0)
        ft_menu->hide();

    ft_quit = new QPushButton(tr("Dismiss"));
    hbox->addWidget(ft_quit);
    ft_quit->setAutoDefault(false);
    connect(ft_quit, SIGNAL(clicked()), this, SLOT(quit_slot()));

    for (int i = 1; i < FC.num_app_fonts; i++) {
        QFont *fnt;
        FC.getFont(&fnt, i);
        add_choice(fnt, FC.getLabel(i));
    }
    ft_menu->setCurrentIndex(indx-1);
    menu_choice_slot(indx - 1);
}


QTfontDlg::~QTfontDlg()
{
    for (int i = 0; i < 4; i++) {
        if (activeFontSels[i] == this) {
            activeFontSels[i] = 0;
            break;
        }
    }
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (owner)
            owner->ClearPopup(this);
    }
    if (p_callback)
        (*p_callback)(0, 0, p_cb_arg);
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller)
        QTdev::Deselect(p_caller);
}


// GRpopup override
// Register the calling button, and set up:
//  1.  whether or not the caller is deselected on popdwon.
//  2.  whether or not deselecting the caller causes popdown.
//
void
QTfontDlg::register_caller(GRobject c, bool no_dsl, bool handle_popdn)
{
    p_caller = c;
    p_no_desel = no_dsl;
    if (handle_popdn) {
        QObject *o = (QObject*)c;
        if (o) {
            if (o->isWidgetType()) {
                QAbstractButton *btn = dynamic_cast<QAbstractButton*>(o);
                if (btn) {
                    connect(btn, SIGNAL(clicked()),
                        this, SLOT(quit_slot()));
                }
            }
            else {
                QAction *a = dynamic_cast<QAction*>(o);
                if (a) {
                    connect(a, SIGNAL(triggered()),
                        this, SLOT(quit_slot()));
                }
            }
        }
    }
}


// GRpopup override
//
void
QTfontDlg::popdown()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


// GRfontDlg override
// Set the font, return false on error.
void
QTfontDlg::set_font_name(const char *fontname)
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }

    if (!fontname || !*fontname)
        return;
    QFont *font = QTfont::self()->new_font(fontname, false);
    if (font) {
        select_font(font);
        delete font;
    }
}


// GRfontPopup override
// Update the sample text (if a label is being used).
//
void
QTfontDlg::update_label(const char *text)
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    if (ft_preview) {
        if (text)
            ft_preview->setPlainText(text);
        else
            ft_preview->setPlainText(PREVIEW_STRING);
    }
}


void
QTfontDlg::select_font(const QFont *fnt)
{
    if (!fnt)
        return;
    QString fam = fnt->family();
    QList<QListWidgetItem*> list =
        ft_face_list->findItems(fam, Qt::MatchExactly);
    if (list.size() > 0) {
        ft_face_list->setCurrentItem(list.at(0));
//        face_changed_slot(list.at(0), 0);
    }
    else {
        // Try again, stripping the foundry.  QFontInfo always appends
        // the foundry, whereas QFontDatabase omits the foundry unless
        // there are multiple foundries for a font.
        bool found = false;
        int ix = fam.lastIndexOf(QChar('['));
        if (ix > 0) {
            while (ix && fam.at(ix - 1) == QChar(' '))
                ix--;
            fam.truncate(ix);
            list = ft_face_list->findItems(fam, Qt::MatchExactly);
            if (list.size() > 0) {
                ft_face_list->setCurrentItem(list.at(0));
                face_changed_slot(list.at(0), 0);
                found = true;
            }
        }
        if (!found)
            return;
    }
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    QString sty = QFontDatabase::styleString(*fnt);
#else
    QString sty = fdb->styleString(*fnt);
#endif
    list = ft_style_list->findItems(sty, Qt::MatchExactly);
    if (list.size() > 0)
        ft_style_list->setCurrentItem(list.at(0));
    int sz = fnt->pointSize();
    if (sz < 0)
        sz = fnt->pixelSize();
    QString qs =  QString("%1").arg(sz);
    list = ft_size_list->findItems(qs, Qt::MatchExactly);
    if (list.size() > 0)
        ft_size_list->setCurrentItem(list.at(0));
    else {
        // Find the next larger size.
        for (int i = 0; i < ft_size_list->count(); i++) {
            QListWidgetItem *wi = ft_size_list->item(i);
            QString qswi = wi->text();
            if (qswi.toInt() > qs.toInt()) {
                ft_size_list->setCurrentItem(wi);
                break;
            }
        }
    }

    ft_preview->setFont(*fnt);
    ft_preview->setPlainText(PREVIEW_STRING);
}


QFont *
QTfontDlg::current_selection()
{
    QListWidgetItem *item = ft_face_list->currentItem();
    if (!item)
        return (0);
    QString qface = item->text();
    item = ft_style_list->currentItem();
    if (!item)
        return (0);
    QString qstyle = item->text();
    item = ft_size_list->currentItem();
    if (!item)
        return (0);
    int sz = item->text().toInt();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    return (new QFont(QFontDatabase::font(qface, qstyle, sz)));
#else
    return (new QFont(fdb->font(qface, qstyle, sz)));
#endif
}


char *
QTfontDlg::current_face()
{
    QListWidgetItem *item = ft_face_list->currentItem();
    if (!item)
        return (0);
    QString qface = item->text();
    return (lstring::copy(qface.toLatin1().constData()));
}


char *
QTfontDlg::current_style()
{
    QListWidgetItem *item = ft_style_list->currentItem();
    if (!item)
        return (0);
    QString qstyle = item->text();
    return (lstring::copy(qstyle.toLatin1().constData()));
}


int
QTfontDlg::current_size()
{
    QListWidgetItem *item = ft_size_list->currentItem();
    if (!item)
        return (0);
    return (item->text().toInt());
}


void
QTfontDlg::add_choice(const QFont *, const char *descr)
{
    ft_menu->addItem(QString(descr));
}


void
QTfontDlg::set_apply_btn_name(const char *bname)
{
    ft_apply->setText(bname);
}


void
QTfontDlg::action_slot()
{
    sLstr lstr;
    char *face = current_face();
    if (!face)
        return;
    if (strchr(face, ' ')) {
        lstr.add_c('"');
        lstr.add(face);
        lstr.add_c('"');
    }
    else
        lstr.add(face);
    delete [] face;
        
    int sz = current_size();
    if (sz <= 0)
        return;
    char *sty = current_style();
    if (sty) {
        if (!strcasecmp(sty, "normal")) {
            delete [] sty;
            sty = 0;
        }
    }
    if (sty) {
        lstr.add_c(' ');
        lstr.add(sty);
    }
    lstr.add_c(' ');
    lstr.add_d(sz);

    if (ft_menu->isHidden()) {
        QByteArray ba = ft_apply->text().toLatin1();
        if (p_callback)
            (*p_callback)(ba.constData(), lstr.string(), p_cb_arg);
        emit select_action(0, lstr.string(), p_cb_arg);
    }
    else {
        int fnum = ft_menu->currentIndex() + 1;
        FC.setName(lstr.string(), fnum);
        if (p_callback)
            (*p_callback)(FC.getLabel(fnum), lstr.string(), p_cb_arg);
        emit select_action(fnum, lstr.string(), p_cb_arg);
    }
}


void
QTfontDlg::quit_slot()
{
    delete this;
}


void
QTfontDlg::face_changed_slot(QListWidgetItem *new_item, QListWidgetItem*)
{
    if (!new_item) {
        ft_preview->setPlainText("");
        return;
    }
    QString qface = new_item->text();

    QString qstyle;
    if (ft_style_list->currentItem())
        qstyle = ft_style_list->currentItem()->text();
    ft_style_list->clear();

    QString qsize;
    if (ft_size_list->currentItem())
        qsize = ft_size_list->currentItem()->text();
    ft_size_list->clear();

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    QStringList styles = QFontDatabase::styles(qface);
#else
    QStringList styles = fdb->styles(qface);
#endif
    for (int i = 0; i < styles.size(); i++)
        ft_style_list->addItem(styles.at(i));

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    QList<int> sizes = QFontDatabase::smoothSizes(qface, styles.at(0));
#else
    QList<int> sizes = fdb->smoothSizes(qface, styles.at(0));
#endif
    for (int i = 0; i < sizes.size(); i++)
        ft_size_list->addItem(QString("%1").arg(sizes.at(i)));

    QList<QListWidgetItem*> list = ft_style_list->findItems(qstyle,
        Qt::MatchExactly);
    if (list.size() > 0)
        ft_style_list->setCurrentItem(list.at(0));
    else
        ft_style_list->setCurrentRow(0);

    list = ft_size_list->findItems(qsize, Qt::MatchExactly);
    if (list.size() > 0)
        ft_size_list->setCurrentItem(list.at(0));
    else
        ft_size_list->setCurrentRow(0);

    QFont *fnt = current_selection();
    if (fnt) {
        ft_preview->setFont(*fnt);
        delete fnt;
        ft_preview->setPlainText(PREVIEW_STRING);
    }
    else
        ft_preview->setPlainText("");
}


void
QTfontDlg::style_changed_slot(QListWidgetItem *new_item, QListWidgetItem*)
{
    if (!new_item)
        return;
    if (!ft_face_list->currentItem())
        return;

    // Revise the list of sizes.  This appears unneeded at least for
    // QT6 on Apple as the size list seems the same for everything.

    QString qstyle = new_item->text();
    QString qface = ft_face_list->currentItem()->text();
    QString qsize;
    if (ft_size_list->currentItem())
        qsize = ft_size_list->currentItem()->text();
#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    QList<int> sizes = QFontDatabase::smoothSizes(qface, qstyle);
#else
    QList<int> sizes = fdb->smoothSizes(qface, qstyle);
#endif
    ft_size_list->clear();
    for (int i = 0; i < sizes.size(); i++)
        ft_size_list->addItem(QString("%1").arg(sizes.at(i)));
    QList<QListWidgetItem*> list = ft_size_list->findItems(qsize,
        Qt::MatchExactly);
    if (list.size() > 0)
        ft_size_list->setCurrentItem(list.at(0));
    else
        ft_size_list->setCurrentRow(0);

    QFont *fnt = current_selection();
    if (fnt) {
        ft_preview->setFont(*fnt);
        delete fnt;
        ft_preview->setPlainText(PREVIEW_STRING);
    }
    else
        ft_preview->setPlainText("");
}


void
QTfontDlg::size_changed_slot(QListWidgetItem *new_item, QListWidgetItem*)
{
    if (!new_item)
        return;
    if (!ft_face_list->currentItem())
        return;
    if (!ft_style_list->currentItem())
        return;

    QFont *fnt = current_selection();
    if (fnt) {
        ft_preview->setFont(*fnt);
        delete fnt;
        ft_preview->setPlainText(PREVIEW_STRING);
    }
    else
        ft_preview->setPlainText("");
}


void
QTfontDlg::menu_choice_slot(int indx)
{
    indx++;
    ft_face_list->clear();
    ft_style_list->clear();
    ft_size_list->clear();

#if QT_VERSION >= QT_VERSION_CHECK(6,0,0)
    QStringList families = QFontDatabase::families();
#else
    QStringList families = fdb->families();
#endif
    for (int i = 0; i < families.size(); i++) {

        // The isFixedPitch function lies, have to identify fixed
        // pitch fonts ourselves.  Oddly, for some known fixed-pitch
        // fonts, the 'i' measures wider than 'm'.

        QFont f(families.at(i), 10);
        QFontMetrics fm(f);
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
        int w1 = fm.horizontalAdvance(QChar('i'));
        int w2 = fm.horizontalAdvance(QChar('m'));
#else
        int w1 = fm.width(QChar('i'));
        int w2 = fm.width(QChar('m'));
#endif
        bool fixed = (w1 >= w2);

        if (!FC.isFixed(indx) || fixed)
            ft_face_list->addItem(families.at(i));
    }
    if (indx > 0) {
        QFont *fnt;
        FC.getFont(&fnt, indx);
        select_font(fnt);
    }
}

