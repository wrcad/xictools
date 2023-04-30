
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
    fnt_t( "Text Editor Font",                0, false, false ),
    fnt_t( "HTML Viewer Proportional Font",   0, false, true ),
    fnt_t( "HTML Viewer Fixed Pitch Font",    0, true, false )
};

int GRfont::num_app_fonts =
    sizeof(GRfont::app_fonts)/sizeof(GRfont::app_fonts[0]);

//#define DEF_FIXED_FACE "Courier New"
//#define DEF_PROP_FACE "Helvetica"
#define DEF_FIXED_FACE "Andale Mono"
#define DEF_PROP_FACE "Menlo"


// This sets the default font names and sizes.
//
void
QTfont::initFonts()
{
    // Just call this once.
    if (app_fonts[0].default_fontname != 0)
        return;

    //XXX
    int def_size = 11;

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


//XXX
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
            }
        }
        else if (!fonts[fnum].name || strcmp(fonts[fnum].name, name)) {
            QFont *newfont = new_font(name, isFixed(fnum));
            if (newfont) {
                delete [] fonts[fnum].name;
                fonts[fnum].name = lstring::copy(name);
                delete fonts[fnum].font;
                fonts[fnum].font = newfont;
            }
        }
        refresh(fnum);
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


/*XXX
// Static function.
// Call this to track font changes.
//
void
QTfont::trackFontChange(GtkWidget *widget, int fnum)
{
}


// Static function.
// Set the default font for the widget to the index (FNT_???).  If
// track is true, the widget font can be updated from the font
// selection pop-up.
//
void
QTfont::setupFont(GtkWidget *widget, int font_index, bool track)
{
}
*/


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


// Create a font for the string given in name, which has the form
// "family [style keywords] [size]".
//
QFont *
QTfont::new_font(const char *name, bool fixed)
{
    char *family;
    stringlist *style;
    int size;
    parse_freeform_font_string(name, &family, &style, &size);
    if (!family) {
        if (fixed)
            family = lstring::copy("courier");
        else
            family = lstring::copy("helvetica");
    }
    QFont *font = new QFont(QString(family), size);
    delete family;
    font->setStyleHint(fixed ? QFont::TypeWriter : QFont::Helvetica);
    font->setFixedPitch(fixed);
    for (stringlist *s = style; s; s = s->next) {
        if (!strcasecmp(s->string, "bold"))
            font->setBold(true);
        else if (!strcasecmp(s->string, "italic"))
            font->setItalic(true);
    }
    stringlist::destroy(style);

    // Reset the critical info in case there was not a perfect match.
    QFontInfo fi(*font);
    font->setFamily(fi.family());
    font->setPointSize(fi.pointSize());
    return (font);
}


// Private function to refresh the text widgets.
//
void
QTfont::refresh(int fnum)
{
    if (getFont(0, fnum)) {
        for (FcbRec *f = fonts[fnum].cbs; f; f = f->next) {
            if (fonts[fnum].font) {
                f->widget->setFont(*fonts[fnum].font);
            }
        }
    }
}


//-----------------------------------------------------------------------------
// Font Selection Dialog

#define PREVIEW_STRING "abcdefghijk ABCDEFGHIJK"

namespace {
    class font_list_widget : public QListWidget
    {
    public:
        font_list_widget(int w, QWidget *prnt) : QListWidget(prnt)
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
}

QTfontPopup::QTfontPopup(QTbag *owner, int indx, void *arg) :
    QDialog(owner ? owner->shell : 0)
{
    p_parent = owner;
    p_cb_arg = arg;

    if (owner)
        owner->monitor.add(this);

    setWindowTitle(QString(tr("Font Selection")));
    face_list = new font_list_widget(180, this);
    style_list = new font_list_widget(120, this);
    size_list = new font_list_widget(60, this);
    quit = new QPushButton(this);
    quit->setText(QString(tr("Dismiss")));
    apply = new QPushButton(this);
    apply->setText(QString(tr("Apply")));
    apply->setAutoDefault(false);
    menu = new QComboBox(this);

    QSize qs = apply->sizeHint();
    menu->setMinimumHeight(qs.height());
    menu->setEditable(false);

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(4);
    vbox->setSpacing(4);
    QHBoxLayout *hbox = new QHBoxLayout(0);
    hbox->setMargin(4);
    hbox->setSpacing(2);
    QVBoxLayout *vb = new QVBoxLayout(0);
    vb->setMargin(2);
    QLabel *label = new QLabel(this);
    label->setText(QString(tr("Faces")));
    vb->addWidget(label);
    vb->addWidget(face_list);
    hbox->addLayout(vb);

    vb = new QVBoxLayout(0);
    vb->setMargin(2);
    label = new QLabel(this);
    label->setText(QString(tr("Styles")));
    vb->addWidget(label);
    vb->addWidget(style_list);
    hbox->addLayout(vb);

    vb = new QVBoxLayout(0);
    vb->setMargin(2);
    label = new QLabel(this);
    label->setText(QString(tr("Sizes")));
    vb->addWidget(label);
    vb->addWidget(size_list);
    hbox->addLayout(vb);
    vbox->addLayout(hbox);

    QGroupBox *gb = new QGroupBox(this);
    gb->setTitle(QString(tr("Preview")));
    preview = new QTextEdit(gb);
    preview->setFixedHeight(50);
    vb = new QVBoxLayout(gb);
    vb->setMargin(2);
    vb->setSpacing(4);
    vb->addWidget(preview);
    vbox->addWidget(gb);

    hbox = new QHBoxLayout(0);
    hbox->setMargin(4);
    hbox->setSpacing(2);
    hbox->addWidget(apply);
    hbox->addWidget(menu);
    hbox->addWidget(quit);
    vbox->addLayout(hbox);

    connect(face_list,
        SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this,
        SLOT(face_changed_slot(QListWidgetItem*, QListWidgetItem*)));
    connect(style_list,
        SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this,
        SLOT(style_changed_slot(QListWidgetItem*, QListWidgetItem*)));
    connect(size_list,
        SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), this,
        SLOT(size_changed_slot(QListWidgetItem*, QListWidgetItem*)));
    connect(menu, SIGNAL(activated(int)), this, SLOT(menu_choice_slot(int)));

    connect(apply, SIGNAL(clicked()), this, SLOT(action_slot()));
    connect(quit, SIGNAL(clicked()), this, SLOT(quit_slot()));

    for (int i = 1; i < FC.num_app_fonts; i++) {
        QFont *fnt;
        FC.getFont(&fnt, i);
        add_choice(fnt, FC.getLabel(i));
    }
    menu->setCurrentIndex(indx-1);

    fdb = new QFontDatabase();
    menu_choice_slot(indx - 1);
}


QTfontPopup::~QTfontPopup()
{
    if (p_usrptr)
        *p_usrptr = 0;
    if (p_caller) {
        QObject *o = (QObject*)p_caller;
        if (o->isWidgetType()) {
            QPushButton *btn = dynamic_cast<QPushButton*>(o);
            if (btn)
                btn->setChecked(false);
        }
        else {
            QAction *a = dynamic_cast<QAction*>(o);
            if (a)
                a->setChecked(false);
        }
    }
    if (p_callback)
        (*p_callback)(0, 0, p_cb_arg);
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (owner) {
            owner->monitor.remove(this);
            if (owner->fontsel == this)
                owner->fontsel = 0;
        }
    }
}


// GRpopup override
// Register the calling button, and set up:
//  1.  whether or not the caller is deselected on popdwon.
//  2.  whether or not deselecting the caller causes popdown.
//
void
QTfontPopup::register_caller(GRobject c, bool no_dsl, bool handle_popdn)
{
    p_caller = c;
    p_no_desel = no_dsl;
    if (handle_popdn) {
        QObject *o = (QObject*)c;
        if (o) {
            if (o->isWidgetType()) {
                QPushButton *btn = dynamic_cast<QPushButton*>(o);
                if (btn)
                    connect(btn, SIGNAL(clicked()),
                        this, SLOT(quit_slot()));
            }
            else {
                QAction *a = dynamic_cast<QAction*>(o);
                if (a)
                    connect(a, SIGNAL(triggered()),
                        this, SLOT(quit_slot()));
            }
        }
    }
}


// GRpopup override
//
void
QTfontPopup::popdown()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->monitor.is_active(this))
            return;
    }
    delete this;
}


// GRfontPopup override
// Set the font.
void
QTfontPopup::set_font_name(const char *fontname)
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->monitor.is_active(this))
            return;
    }

    if (!fontname || !*fontname)
        return;
//XXX
}


// GRfontPopup override
// Update the label text (if a label is being user).
//
void
QTfontPopup::update_label(const char *text)
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->monitor.is_active(this))
            return;
    }
    (void)text;
//XXX
}


void
QTfontPopup::select_font(const QFont *fnt)
{
    if (!fnt)
        return;
    QString fam = fnt->family();
    QList<QListWidgetItem*> list = face_list->findItems(fam, Qt::MatchExactly);
    if (list.size() > 0) {
        face_list->setCurrentItem(list.at(0));
        face_changed_slot(list.at(0), 0);
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
            list = face_list->findItems(fam, Qt::MatchExactly);
            if (list.size() > 0) {
                face_list->setCurrentItem(list.at(0));
                face_changed_slot(list.at(0), 0);
                found = true;
            }
        }
        if (!found)
            return;
    }
    QString sty = fdb->styleString(*fnt);
    list = style_list->findItems(sty, Qt::MatchExactly);
    if (list.size() > 0)
        style_list->setCurrentItem(list.at(0));
    else
        return;
    int sz = fnt->pointSize();
    if (sz < 0)
        sz = fnt->pixelSize();
    QString qs =  QString("%1").arg(sz);
    list = size_list->findItems(qs, Qt::MatchExactly);
    if (list.size() > 0)
        size_list->setCurrentItem(list.at(0));

    preview->setFont(*fnt);
    preview->setPlainText(QString(PREVIEW_STRING));
}


QFont *
QTfontPopup::current_selection()
{
    QListWidgetItem *item = face_list->currentItem();
    if (!item)
        return (0);
    QString qface = item->text();
    item = style_list->currentItem();
    if (!item)
        return (0);
    QString qstyle = item->text();
    item = size_list->currentItem();
    if (!item)
        return (0);
    int sz = item->text().toInt();
    return (new QFont(fdb->font(qface, qstyle, sz)));
}


char *
QTfontPopup::current_face()
{
    QListWidgetItem *item = face_list->currentItem();
    if (!item)
        return (0);
    QString qface = item->text();
    return (lstring::copy(qface.toLatin1().constData()));
}


char *
QTfontPopup::current_style()
{
    QListWidgetItem *item = style_list->currentItem();
    if (!item)
        return (0);
    QString qstyle = item->text();
    return (lstring::copy(qstyle.toLatin1().constData()));
}


int
QTfontPopup::current_size()
{
    QListWidgetItem *item = size_list->currentItem();
    if (!item)
        return (0);
    return (item->text().toInt());
}


void
QTfontPopup::add_choice(const QFont *, const char *descr)
{
    menu->addItem(QString(descr));
}


void
QTfontPopup::action_slot()
{
    char *face = current_face();
    if (!face)
        return;
    int sz = current_size();
    if (sz <= 0) {
        delete [] face;
        return;
    }
    char *sty = current_style();
    if (sty) {
        if (!strcasecmp(sty, "normal")) {
            delete [] sty;
            sty = 0;
        }
    }
    int len = strlen(face);
    if (sty)
        len += strlen(sty) + 1;
    len += 8;
    char *spec = new char[len];
    strcpy(spec, face);
    char *t = spec + strlen(face);
    delete [] face;
    if (sty) {
        *t++ = ' ';
        strcpy(t, sty);
        t += strlen(sty);
        delete [] sty;
    }
    *t++ = ' ';
    snprintf(t, 8, "%d", sz);
    int fnum = menu->currentIndex() + 1;
    FC.setName(spec, fnum);
    if (p_callback)
        (*p_callback)(FC.getLabel(fnum), spec, p_cb_arg);
    emit select_action(fnum, spec, p_cb_arg);
    delete [] spec;
}


void
QTfontPopup::quit_slot()
{
    emit dismiss();
    delete this;
}


void
QTfontPopup::face_changed_slot(QListWidgetItem *new_item, QListWidgetItem*)
{
    if (!new_item) {
        preview->setPlainText(QString(""));
        return;
    }
    QString qface = new_item->text();

    QString qstyle;
    if (style_list->currentItem())
        qstyle = style_list->currentItem()->text();
    style_list->clear();

    QString qsize;
    if (size_list->currentItem())
        qsize = size_list->currentItem()->text();
    size_list->clear();

    QStringList styles = fdb->styles(qface);
    for (int i = 0; i < styles.size(); i++)
        style_list->addItem(styles.at(i));

    QList<int> sizes = fdb->smoothSizes(qface, styles.at(0));
    for (int i = 0; i < sizes.size(); i++)
        size_list->addItem(QString("%1").arg(sizes.at(i)));

    QList<QListWidgetItem*> list = style_list->findItems(qstyle,
        Qt::MatchExactly);
    if (list.size() > 0)
        style_list->setCurrentItem(list.at(0));
    else
        style_list->setCurrentRow(0);

    list = size_list->findItems(qsize, Qt::MatchExactly);
    if (list.size() > 0)
        size_list->setCurrentItem(list.at(0));
    else
        size_list->setCurrentRow(0);

    QFont *fnt = current_selection();
    if (fnt) {
        preview->setFont(*fnt);
        delete fnt;
    }
    else
        preview->setPlainText(QString(""));
}


void
QTfontPopup::style_changed_slot(QListWidgetItem *new_item, QListWidgetItem*)
{
    (void)new_item;
    QFont *fnt = current_selection();
    if (fnt) {
        preview->setFont(*fnt);
        delete fnt;
    }
    else
        preview->setPlainText(QString(""));
}


void
QTfontPopup::size_changed_slot(QListWidgetItem *new_item, QListWidgetItem*)
{
    (void)new_item;
    QFont *fnt = current_selection();
    if (fnt) {
        preview->setFont(*fnt);
        delete fnt;
        preview->setPlainText(QString(PREVIEW_STRING));
    }
    else
        preview->setPlainText(QString(""));
}


void
QTfontPopup::menu_choice_slot(int indx)
{
    indx++;
    face_list->clear();
    style_list->clear();
    size_list->clear();

    QStringList families = fdb->families();
    for (int i = 0; i < families.size(); i++) {

        // The isFixedPitch function lies, have to identify fixed
        // pitch fonts ourselves.  Oddly, for some known fixed-pitch
        // fonts, the 'i' measures wider than 'm'.

        QFont f(families.at(i), 10);
        QFontMetrics fm(f);
        int w1 = fm.width(QChar('i'));
        int w2 = fm.width(QChar('m'));
        bool fixed = (w1 >= w2);

        if (!FC.isFixed(indx) || fixed)
            face_list->addItem(families.at(i));
    }
    QFont *fnt;
    FC.getFont(&fnt, indx);
    select_font(fnt);
}

