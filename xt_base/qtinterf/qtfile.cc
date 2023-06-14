
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

#include "qtfile.h"
#include "qtaffirm.h"
#include "qtinput.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"

#include <dirent.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include <fnmatch.h>

#include <QApplication>
#include <QComboBox>
#include <QDrag>
#include <QGroupBox>
#include <QHeaderView>
#include <QItemDelegate>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QMenuBar>
#include <QMimeData>
#include <QMouseEvent>
#include <QPushButton>
#include <QSplitter>
#include <QTimer>
#include <QTreeWidget>

// File selection pop-up.  The panel consists of a CTree which
// maintains a visual representation of the directory hierarchy, and a
// text window for listing files.  Selecting a directory in the tree
// will display the files in the list window, where they can be
// selected for operations.  Both windows are drag sources and drop
// sites.

// Help keywords used in this file:
// filesel

#define COLUMN_SPACING 20

enum ActionType { A_NOOP, A_COPY, A_MOVE, A_LINK, A_ASK };
static void DoFileAction(QTfilePopup*, const char*, const char*, ActionType);

// XPM
static const char* const up_xpm[] = {
"32 16 3 1",
"     c none",
".    c blue",
"x    c sienna",
"                                ",
"                                ",
"                                ",
"                                ",
"               .                ",
"              ...x              ",
"             .....x             ",
"            .......x            ",
"           .........x           ",
"          ...........x          ",
"         .............x         ",
"        ...............x        ",
"        x.............xx        ",
"         xxxxxxxxxxxxxx         ",
"                                ",
"                                "};

// XPM
static const char * const go_xpm[] = {
"32 16 4 1",
"   c none",
".  c green",
"x  c white",
"+  c black",
"                                ",
"             xxxxx              ",
"            xx....x             ",
"           xx......x            ",
"           x........x           ",
"          x..........x          ",
"         x............          ",
"          ............          ",
"          ............          ",
"          ............+         ",
"          +..........+          ",
"           +........+           ",
"            +......++           ",
"             +....++            ",
"              +++++             ",
"                                "};

// XPM
static const char* const folder_xpm[] = {
"16 16 8 1",
"   c None",
".  c #909000",
"+  c #000000",
"@  c #EFE8EF",
"#  c #FFF8CF",
"$  c #FFF890",
"%  c #CFC860",
"&  c #FFC890",
"                ",
"  .....+        ",
" .@##$$.+       ",
".%%%%%%%......  ",
".###########$%+ ",
".#$$$$$$$$$$&%+ ",
".#$$$$$$$&$&$%+ ",
".#$$$$$$$$&$&%+ ",
".#$$$$$&$&$&$%+ ",
".#$$$$$$&$&$&%+ ",
".#$$$&$&$&$&&%+ ",
".#&$&$&$&$&&&%+ ",
".%%%%%%%%%%%%%+ ",
" ++++++++++++++ ",
"                ",
"                "};

// XPM
static const char* const ofolder_xpm[] = {
"16 16 12 1",
"   c None",
".  c #808080",
"+  c #E0E0D0",
"@  c #4F484F",
"#  c #909000",
"$  c #FFF8EF",
"%  c #CFC860",
"&  c #003090",
"*  c #7F7800",
"=  c #FFC890",
"-  c #FFF890",
";  c #2F3000",
"        .       ",
"       .+@      ",
"   ###.$$+@     ",
"  #%%.$$$$+@    ",
"  #%.$$$&$$+@** ",
"  #.+++&+&+++@* ",
"############++@ ",
"#$$$$$$$$$=%#++@",
"#$-------=-=#+; ",
" #---=--=-==%#; ",
" #-----=-=-==#; ",
" #-=--=-=-=-=#; ",
"  #=-=-=-=-==#; ",
"  ############; ",
"   ;;;;;;;;;;;  ",
"                "};

// XPM
static const char* const file_xpm[] = {
"16 16 5 1",
". c #7f7f7f",
"# c None",
"c c #000000",
"b c #bfbfbf",
"a c #ffffff",
"################",
"..........######",
".aaaaaaaab.#####",
".aaaaaaaaba.####",
".aaaaaaaacccc###",
".aaaaaaaaaabc###",
".aaaaaaaaaabc###",
".aaaaaaaaaabc###",
".aaaaaaaaaabc###",
".aaaaaaaaaabc###",
".aaaaaaaaaabc###",
".aaaaaaaaaabc###",
".aaaaaaaaaabc###",
".aaaaaaaaaabc###",
".bbbbbbbbbbbc###",
"ccccccccccccc###"};

// The supported modes
// fsSEL:
//  File selection mode, title:  "File Selection"
//  Inputs: callback, arg, cancel, root (def to cwd)
//  Menu?  yes, file/open
//  Footer?  root label
// fsDOWNLOAD:
//  Download mode, title:  "Target Selection"
//  Inputs: callback, arg, cancel, fname (return 0 if null)
//  Menu?  no
//  Footer?  name entry
// fsSAVE:
//  Save file mode, title:  "Path Selection"
//  Inputs: callback, arg, cancel, path get, path set, fname
//  Menu?  yes, no File/Open
//  Footer?  none, and no file listing
// fsOPEN:
//  Open file mode, title;  "File Selection"
//  Inputs: callback, arg, cancel, path get, path set, fname
//  Menu?  yes, no File/Open
//  Footer?  none

namespace qtinterf
{
    struct QTfsMon : public GRmonList
    {
        char *any_selection();
    };

    // Subclass QTreeWidgetItem to hold directory modification time.
    //
    class file_tree_item : public QTreeWidgetItem
    {
    public:
        file_tree_item(file_tree_item *prnt) :
            QTreeWidgetItem((QTreeWidgetItem*)prnt) { mtime = 0; }
        file_tree_item(QTreeWidget *prnt) :
            QTreeWidgetItem(prnt) { mtime = 0; }

        unsigned int mtime;
    };


    // Subclass QTreeWidget to support our drag/drop.
    //
    class file_tree_widget : public QTreeWidget
    {
    public:
        file_tree_widget(QWidget*);

        void register_fsel(QTfilePopup *f) { fsel = f; }

    protected:
        void mousePressEvent(QMouseEvent*);
        void mouseMoveEvent(QMouseEvent*);
        void dragEnterEvent(QDragEnterEvent*);
        void dragMoveEvent(QDragMoveEvent*);
        void dragLeaveEvent(QDragLeaveEvent*);
        void dropEvent(QDropEvent*);

    private:
        void start_drag();

        QPoint drag_pos;            // drag reference location
        QTfilePopup *fsel;          // parent widget
        QTreeWidgetItem *target;    // current drop target
    };
}


file_tree_widget::file_tree_widget(QWidget *prnt) : QTreeWidget(prnt)
{
    fsel = 0;
    setAcceptDrops(true);
    target = 0;
    QSizePolicy policy = sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Ignored);
    policy.setVerticalPolicy(QSizePolicy::Ignored);
    setSizePolicy(policy);
}


void
file_tree_widget::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton)
        drag_pos = ev->pos();
    QTreeWidget::mousePressEvent(ev);
}


void
file_tree_widget::mouseMoveEvent(QMouseEvent *ev)
{
    if (ev->buttons() & Qt::LeftButton) {
        int dist = (ev->pos() - drag_pos).manhattanLength();
        if (dist > QApplication::startDragDistance())
            start_drag();
    }
    QTreeWidget::mouseMoveEvent(ev);
}


void
file_tree_widget::dragMoveEvent(QDragMoveEvent *ev)
{
    QTreeWidget::dragMoveEvent(ev);  // handles scrolling at edges
    if (ev->mimeData()->hasText()) {

        // Yes, this must be done here.
        ev->acceptProposedAction();

        // Paint target background
        QTreeWidgetItem *it = itemAt(ev->pos());
        if (it && target != it) {
            if (target)
                target->setBackgroundColor(0, QColor(255,255,255));
            it->setBackgroundColor(0, QColor(255,255,120));
            target = it;
        }
    }
}


void
file_tree_widget::dragEnterEvent(QDragEnterEvent *ev)
{
    if (ev->mimeData()->hasText()) {
        ev->accept();
        ev->acceptProposedAction();
        target = 0;
    }
    else
        ev->ignore();
}


void
file_tree_widget::dragLeaveEvent(QDragLeaveEvent*)
{
    if (target)
        target->setBackgroundColor(0, QColor(255,255,255));
    target = 0;
}


void
file_tree_widget::dropEvent(QDropEvent *ev)
{
    QTreeWidgetItem *it = itemAt(ev->pos());
    if (it) {
        int proposed_action = ev->proposedAction();
        const QMimeData *md = ev->mimeData();
        char *src = lstring::copy(md->text().toLatin1().constData());
        char *dst = fsel->get_dir(it);

        if (src && dst) {
            ActionType a = A_NOOP;
            if (proposed_action & Qt::CopyAction)
                a = A_COPY;
            else if (proposed_action & Qt::MoveAction)
                a = A_MOVE;
            else if (proposed_action & Qt::LinkAction)
                a = A_LINK;

            DoFileAction(fsel, src, dst, a);
        }
        delete [] src;
        delete [] dst;
        fsel->flash(it);
    }
}


void
file_tree_widget::start_drag()
{
    if (fsel) {
        char *path = fsel->get_dir();
        if (path) {
            QDrag *drag = new QDrag(this);
            QMimeData *md = new QMimeData();
            md->setText(QString(path));
            drag->setMimeData(md);
            drag->setPixmap(QPixmap(folder_xpm));
            drag->start(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction);
            delete [] path;
        }
    }
}
// End of file_tree_widget functions.


namespace qtinterf
{
    // Subclass QListWidget to support our drag/drop.
    //
    class file_list_widget : public QListWidget
    {
    public:
        file_list_widget(QWidget *parent);

        QListWidgetItem *item_of(const QModelIndex &index)
            {
                return (itemFromIndex(index));
            }

        void register_fsel(QTfilePopup *f) { fsel = f; }

    protected:
        void mousePressEvent(QMouseEvent*);
        void mouseMoveEvent(QMouseEvent*);
        void dragEnterEvent(QDragEnterEvent*);
        void dragMoveEvent(QDragMoveEvent*);
        void dropEvent(QDropEvent*);

    private:
        void start_drag();

        QPoint drag_pos;        // drag reference location
        QTfilePopup *fsel;      // parent widget
    };

    // This is used to increase spacing between file_list_widget columns
    //
    class file_delegate : public QItemDelegate
    {
    public:
        file_delegate(file_list_widget *w) : QItemDelegate(w)
            {
                widget = w;
            }

        QSize sizeHint(const QStyleOptionViewItem&,
            const QModelIndex&)  const;

    private:
        file_list_widget *widget;
    };
}


QSize
file_delegate::sizeHint(const QStyleOptionViewItem&,
    const QModelIndex &index) const
{
    QListWidgetItem *item = widget->item_of(index);
    QFontMetrics fm(item->font());
#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
    return (QSize(fm.horizontalAdvance(item->text()) + COLUMN_SPACING,
        fm.height()));
#else
    return (QSize(fm.width(item->text()) + COLUMN_SPACING, fm.height()));
#endif
}
// End of file_delegate functions (for file_list_widget).


file_list_widget::file_list_widget(QWidget *prnt) : QListWidget(prnt)
{
    fsel = 0;
    setAcceptDrops(true);
    QSizePolicy policy = sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Ignored);
    policy.setVerticalPolicy(QSizePolicy::Ignored);
    setSizePolicy(policy);
    setItemDelegate(new file_delegate(this));
    setResizeMode(QListView::Adjust);
}


void
file_list_widget::mousePressEvent(QMouseEvent *ev)
{
    if (ev->button() == Qt::LeftButton)
        drag_pos = ev->pos();
    QListWidget::mousePressEvent(ev);
/*XXX
    if (fsel) {
        if (fsel->p_path_set)
            (*fsel->p_path_set)(fsel->get_selection());   
    }
*/
}


void
file_list_widget::mouseMoveEvent(QMouseEvent *ev)
{
    if (ev->buttons() & Qt::LeftButton) {
        int dist = (ev->pos() - drag_pos).manhattanLength();
        if (dist > QApplication::startDragDistance())
            start_drag();
    }
    QListWidget::mouseMoveEvent(ev);
}


void
file_list_widget::dragMoveEvent(QDragMoveEvent *ev)
{
    QListWidget::dragMoveEvent(ev);  // handles scrolling at edges
    if (ev->mimeData()->hasText()) {

        // Yes, this must be done here.
        ev->acceptProposedAction();
    }
}


void
file_list_widget::dragEnterEvent(QDragEnterEvent *ev)
{
    if (ev->mimeData()->hasText()) {
        ev->accept();
        ev->acceptProposedAction();
    }
    else
        ev->ignore();
}


void
file_list_widget::dropEvent(QDropEvent *ev)
{
    int proposed_action = ev->proposedAction();
    const QMimeData *md = ev->mimeData();
    char *src = lstring::copy(md->text().toLatin1().constData());
    char *dst = fsel->get_dir();

    if (src && dst) {
        ActionType a = A_NOOP;
        if (proposed_action & Qt::CopyAction)
            a = A_COPY;
        else if (proposed_action & Qt::MoveAction)
            a = A_MOVE;
        else if (proposed_action & Qt::LinkAction)
            a = A_LINK;
        DoFileAction(fsel, src, dst, a);
    }
    delete [] src;
    delete [] dst;
}


void
file_list_widget::start_drag()
{
    if (fsel) {
        char *path = fsel->get_selection();
        if (path) {
            QDrag *drag = new QDrag(this);
            QMimeData *md = new QMimeData();
            md->setText(QString(path));
            drag->setMimeData(md);
            drag->setPixmap(QPixmap(file_xpm));
            drag->start(Qt::CopyAction | Qt::MoveAction | Qt::LinkAction);
            delete [] path;
        }
    }
}
// End of file_list_widget functions.


// Keep a list of all active file selection pop-ups so we can find
// selected text.
//
namespace { QTfsMon FSmonitor; }


// Return the selection from any file selection pop-up.  The window
// manager probably allows only one selection.
//
char *
QTfsMon::any_selection()
{
    for (elt *e = list; e; e = e->next) {
        QTfilePopup *fs = static_cast<QTfilePopup*>(e->obj);
        if (fs) {
            char *s = fs->get_selection();
            if (s)
                return (s);
        }
    }
    return (0);
}


namespace {
    // The default strings for the filter combo.  The first two are not
    // editable, the rest can be set arbitrarily by the user.
    //
    const char *filter_options[] =
    {
        "all:",
        "archive: *.cif *.cgx *.gds *.oas *.strm *.stream",
        0,
        0,
        0,
        0,
        0
    };
}

QTfilePopup::QTfilePopup(QTbag *owner, FsMode mode, void *arg,
    const char *root_or_fname) :
    QDialog(owner ? owner->Shell() : 0), QTbag()
{
wb_shell = this;
    p_parent = owner;
    p_cb_arg = arg;
    menubar = 0;
    tree = 0;
    list = 0;
    label = 0;
    filter = 0;
    a_Up = 0;
    a_Go = 0;
    a_UpMenu = 0;
    a_Open = 0;
    a_New = 0;
    a_Delete = 0;
    a_Rename = 0;
    entry = 0;
    filemenu = 0;
    upmenu = 0;
    listmenu = 0;
    helpmenu = 0;
    timer = 0;
    flasher = 0;
    flasher_cnt = 0;
    flasher_item = 0;

    config = mode;
    curnode = 0;
    curfile = 0;
    rootdir = 0;
    cwd_bak = getcwd(0, 256);
    temp_string = 0;
    filter_index = 0;
    no_disable_go = false;

    if (owner)
        owner->MonitorAdd(this);
    FSmonitor.add(this);
    setAttribute(Qt::WA_DeleteOnClose);

    // initialize editable filter lines
    if (!filter_options[2])
        filter_options[2] = lstring::copy("user1:");
    if (!filter_options[3])
        filter_options[3] = lstring::copy("user2:");
    if (!filter_options[4])
        filter_options[4] = lstring::copy("user3:");
    if (!filter_options[5])
        filter_options[5] = lstring::copy("user4:");

    QSizePolicy policy = sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    policy.setVerticalPolicy(QSizePolicy::Expanding);
    setSizePolicy(policy);

    closed_folder_icon.addPixmap(QPixmap(folder_xpm));
    open_folder_icon.addPixmap(QPixmap(ofolder_xpm));

    if (config == fsSEL) {
        setWindowTitle(QString(tr("File Selection")));
        rootdir = lstring::copy(root_or_fname);
        if (!rootdir)
            rootdir = lstring::copy(cwd_bak);
        if (!rootdir)
            rootdir = lstring::copy("/");
    }
    else if (config == fsDOWNLOAD) {
        setWindowTitle(QString(tr("Target Selection")));
        curfile = lstring::copy(root_or_fname);
        if (!curfile)
            // should be fatal error
            curfile = lstring::copy("unnamed");
        rootdir = lstring::copy(cwd_bak);
        if (!rootdir)
            rootdir = lstring::copy("/");
    }
    else if (config == fsSAVE || config == fsOPEN) {
        // name should be tilde and dot expanded
        char *fn;
        if (root_or_fname && *root_or_fname) {
            if (!lstring::is_rooted(root_or_fname)) {
                const char *cwd = cwd_bak;
                if (!cwd)
                    cwd = "";
                int len = strlen(cwd) + strlen(root_or_fname) + 2;
                fn = new char[len];
                snprintf(fn, len, "%s/%s", cwd, root_or_fname);
            }
            else
                fn = lstring::copy(root_or_fname);
            char *s = lstring::strip_path(fn);
            if (s) {
                *s = 0;
                if (s-1 > fn)
                    *(s-1) = 0;
            }
        }
        else {
            fn = lstring::copy(cwd_bak);
            if (!fn)
                fn = lstring::copy("/");
        }
        rootdir = fn;
        if (config == fsSAVE)
            setWindowTitle(QString(tr("Path Selection")));
        else
            setWindowTitle(QString(tr("File Selection")));
    }

    tree = new file_tree_widget(this);
    tree->setColumnCount(1);
    tree->header()->hide();
    tree->setMinimumWidth(100);
    tree->register_fsel(this);

    menubar = new QMenuBar(this);
    a_Up = menubar->addAction(QString("up"), this, SLOT(up_slot()));
    a_Up->setIcon(QIcon(QPixmap(up_xpm)));

    a_Go = menubar->addAction(QString("go"), this, SLOT(open_slot()));
    a_Go->setIcon(QIcon(QPixmap(go_xpm)));

    if (config == fsSEL || config == fsSAVE || config == fsOPEN) {
        filemenu = new QMenu(this);
        filemenu->setTitle(QString(tr("&File")));
        menubar->addMenu(filemenu);

        if (config == fsSEL)
            a_Open = filemenu->addAction(QString(tr("&Open")),
                this, SLOT(open_slot()), Qt::CTRL+Qt::Key_O);
        a_New = filemenu->addAction(QString(tr("New &Folder")),
            this, SLOT(new_folder_slot()), Qt::CTRL+Qt::Key_F);
        a_Delete = filemenu->addAction(QString(tr("&Delete")),
            this, SLOT(delete_slot()), Qt::CTRL+Qt::Key_D);
        a_Rename = filemenu->addAction(QString(tr("&Rename")),
            this, SLOT(rename_slot()), Qt::CTRL+Qt::Key_R);
        filemenu->addAction(QString(tr("N&ew Root")),
            this, SLOT(new_root_slot()), Qt::CTRL+Qt::Key_E);
        filemenu->addAction(QString(tr("New C&WD")),
            this, SLOT(new_cwd_slot()), Qt::CTRL+Qt::Key_W);
        filemenu->addSeparator();
        filemenu->addAction(QString(tr("&Quit")),
            this, SLOT(quit_slot()), Qt::CTRL+Qt::Key_Q);
    }

    upmenu = new QMenu(this);
    upmenu->setTitle(QString(tr("&Up")));
    a_UpMenu = menubar->addMenu(upmenu);
    connect(upmenu, SIGNAL(triggered(QAction*)),
        this, SLOT(up_menu_slot(QAction*)));

    if (config == fsSEL || config == fsOPEN) {
        listmenu = new QMenu(this);
        listmenu->setTitle(QString(tr("&Listing")));
        menubar->addMenu(listmenu);

        QAction *a = listmenu->addAction(QString(tr("&Show Filter")));
        a->setCheckable(true);
        a->setShortcut(Qt::CTRL+Qt::Key_S);
        connect(a, SIGNAL(toggled(bool)),
            this, SLOT(show_filter_slot(bool)));
        listmenu->addAction(QString(tr("Re&list")),
            this, SLOT(list_files_slot()), Qt::CTRL+Qt::Key_L);
    }

    if (config == fsSEL || config == fsSAVE || config == fsOPEN) {
        menubar->addSeparator();

        helpmenu = new QMenu(this);
        helpmenu->setTitle(QString(tr("&Help")));
        menubar->addMenu(helpmenu);
        helpmenu->addAction(QString(tr("&Help")),
            this, SLOT(help_slot()), Qt::CTRL+Qt::Key_H);
    }

    QGroupBox *gbox = 0;
    label = 0;
    entry = 0;
    if (config == fsSEL) {
        gbox = new QGroupBox(this);
        QVBoxLayout *vbox = new QVBoxLayout(gbox);
        vbox->setMargin(2);
        label = new QLabel(gbox);
        label->setText(QString("Root:\nCwd:"));
        label->setMinimumHeight(24);
        label->setMaximumHeight(32);
        vbox->addWidget(label);
        policy = gbox->sizePolicy();
        policy.setVerticalPolicy(QSizePolicy::Fixed);
        gbox->setSizePolicy(policy);
    }
    else if (config == fsDOWNLOAD) {
        char *path = pathlist::mk_path(rootdir, root_or_fname);
        entry = new QLineEdit(this);
        entry->setText(QString(path));
        delete [] path;
    }

    QVBoxLayout *vbox = new QVBoxLayout(this);
    vbox->setMargin(4);
    vbox->setSpacing(2);
    vbox->setMenuBar(menubar);
    if (config == fsSEL || config == fsOPEN) {
        QSplitter *sp = new QSplitter(this);
        sp->setChildrenCollapsible(false);

        sp->addWidget(tree);

        QWidget *holder = new QWidget(this);

        list = new file_list_widget(holder);
        list->setWrapping(true);
        list->setFlow(QListView::TopToBottom);
        list->register_fsel(this);

        filter = new QComboBox(holder);
        filter->setDuplicatesEnabled(true);
        for (const char **s = filter_options; *s; s++)
            filter->addItem(*s);
        filter->hide();
        connect(filter, SIGNAL(activated(int)),
            this, SLOT(filter_choice_slot(int)));
        connect(filter, SIGNAL(editTextChanged(const QString&)),
            this, SLOT(filter_change_slot(const QString&)));
        filter->setInsertPolicy(QComboBox::NoInsert);

        QVBoxLayout *vb = new QVBoxLayout(holder);
        vb->setMargin(0);
        vb->setSpacing(2);
        vb->addWidget(list);
        vb->addWidget(filter);

        sp->addWidget(holder);
        vbox->addWidget(sp);
        QList<int> szs;
        szs.append(150);
        szs.append(350);
        sp->setSizes(szs);
    }
    else
        vbox->addWidget(tree);
    if (gbox)
        vbox->addWidget(gbox);
    else if (entry)
        vbox->addWidget(entry);
    if (config == fsDOWNLOAD || config == fsSAVE || config == fsOPEN) {
        a_Go->setEnabled(true);
        no_disable_go = true;
    }

    connect(
        tree, SIGNAL(currentItemChanged(QTreeWidgetItem*, QTreeWidgetItem*)),
        this, SLOT(tree_select_slot(QTreeWidgetItem*, QTreeWidgetItem*)));
    connect(
        tree, SIGNAL(itemCollapsed(QTreeWidgetItem*)),
        this, SLOT(tree_collapse_slot(QTreeWidgetItem*)));
    connect(
        tree, SIGNAL(itemExpanded(QTreeWidgetItem*)),
        this, SLOT(tree_expand_slot(QTreeWidgetItem*)));
    if (list) {
        connect(list,
            SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)),
            this, SLOT(list_select_slot(QListWidgetItem*, QListWidgetItem*)));
        connect(list,
            SIGNAL(itemDoubleClicked(QListWidgetItem*)),
            this, SLOT(list_double_clicked_slot(QListWidgetItem*)));
    }
    init();

    timer = new QTimer(this);
    timer->setInterval(500);
    connect(timer, SIGNAL(timeout()), this, SLOT(check_slot()));
    timer->start();
    flasher = new QTimer(this);
    flasher->setInterval(100);
    flasher_cnt = 0;
    flasher_item = 0;
    connect(flasher, SIGNAL(timeout()), this, SLOT(flash_slot()));
}


QTfilePopup::~QTfilePopup()
{
    timer->stop();
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
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (owner)
            owner->MonitorRemove(this);
    } 
    FSmonitor.remove(this);
    if (p_cancel)
        (*p_cancel)(this, p_cb_arg);

    delete [] curfile;
    delete [] rootdir;
    delete [] cwd_bak;
}

    
// GRpopup override
//
void
QTfilePopup::popdown()
{       
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return;
    }
    delete this;
}


// GRfselPopup override
// Return the full path to the currently selected file.
//
char *
QTfilePopup::get_selection()
{
    if (p_parent) {
        QTbag *owner = dynamic_cast<QTbag*>(p_parent);
        if (!owner || !owner->MonitorActive(this))
            return (0);
    }
    if (!curnode || !curfile)
        return (0);
    char *dir = get_path(curnode, false);
    char *path = pathlist::mk_path(dir, curfile);
    delete [] dir;
    return (path);
}


// Update the text in the "Root" label and rebuild the Up menu.
//
void
QTfilePopup::set_label()
{
    if (!rootdir)
        return;
    if (label) {
        const char *cwd = cwd_bak;
        if (!cwd)
            cwd = "";
        label->setText(QString("Root: %1\nCwd: %2").
            arg(QString(rootdir)).arg(QString(cwd)));
    }

    upmenu->clear();
    char buf[256];
    char *s = rootdir;
    char *e = rootdir + 1;
    QAction *a;
    for (;;) {
        strncpy(buf, s, e-s);
        buf[e-s] = 0;
        s = e;
        a = upmenu->addAction(QString(buf));
        a->setData(QVariant((int)(e - rootdir - 1)));
        if (!*s)
            break;
        e = lstring::strdirsep(s);
        if (!e)
            break;
        e++;
    }
}


// Flash the tree item background after a drop.
//
void
QTfilePopup::flash(QTreeWidgetItem *it)
{
    flasher_item = it;
    flasher_cnt = 1;
    flasher->start();
}


void
QTfilePopup::up_slot()
{
    if (!rootdir || !strcmp(rootdir, "/"))
        return;

    char *s = lstring::strrdirsep(rootdir);
    if (s) {
        if (s == rootdir)
            s++;
        *s = 0;
        s = lstring::copy(rootdir);
        delete [] rootdir;
        rootdir = s;
    }
    init();
}


void
QTfilePopup::open_slot()
{
    char *sel;
    if (entry)
        sel = lstring::copy(entry->text().toLatin1().constData());
    else { 
        sel = get_selection(); 
        if (!sel && !no_disable_go) 
            return;
    } 
    if (p_callback)
        (*p_callback)(sel, p_cb_arg);
    emit file_selected(sel, p_cb_arg);
    delete [] sel;
}


void
QTfilePopup::new_folder_slot()
{
    if (!curnode) {
        PopUpMessage("No parent directory selected.", true);
        return;
    }
    PopUpInput("Enter new folder name:", 0, "Create", 0, 0);
    connect(wb_input, SIGNAL(action_call(const char*, void*)),
        this, SLOT(new_folder_cb_slot(const char*, void*)));
}


void
QTfilePopup::new_folder_cb_slot(const char *string, void*)
{
    if (lstring::strdirsep(string))
        wb_input->set_message("Invalid name, try again:");
    else {
        char *path = get_path(curnode, false);
        char *dir = pathlist::mk_path(path, string);
        if (mkdir(dir, 0755) != 0)
            PopUpMessage(strerror(errno), true);
        delete [] dir;
        delete [] path;
    }
    // pop down
    if (wb_input)
        wb_input->popdown();
}


void
QTfilePopup::delete_slot()
{
    char *path = get_selection();
    if (!path) {
        PopUpMessage("Nothing selected to delete.", true);
        return;
    }
    char buf[256];
    if (strlen(path) < 80)
        snprintf(buf, sizeof(buf), "Delete %s?", path);
    else
        snprintf(buf, sizeof(buf), "Delete .../%s?", lstring::strip_path(path));
    GRaffirmPopup *a = PopUpAffirm(0, GRloc(), buf, 0, 0);
    QTaffirmPopup *affirm = dynamic_cast<QTaffirmPopup*>(a);
    if (a)
        connect(affirm, SIGNAL(affirm(bool, void*)),
            this, SLOT(delete_cb_slot(bool, void*)));
    delete [] path;
}


// Action for "Delete"
//
void
QTfilePopup::delete_cb_slot(bool yesno, void*)
{
    if (yesno) {
        char *path = get_selection();
        if (!path)
            return;
        if (filestat::is_directory(path)) {
            if (rmdir(path) != 0)
                PopUpMessage(strerror(errno), true);
        }
        else {
            if (unlink(path) != 0)
                PopUpMessage(strerror(errno), true);
        }
        delete [] path;
    }
}


void
QTfilePopup::rename_slot()
{
    char *path = get_selection();
    if (!path) {
        PopUpMessage("Nothing selected to renme.", true);
        return;
    }
    char buf[256];
    snprintf(buf, sizeof(buf), "Enter new name for %s?",
        lstring::strip_path(path));
    PopUpInput(buf, 0, "Rename", 0, 0);
    connect(wb_input, SIGNAL(action_call(const char*, void*)),
        this, SLOT(rename_sb_slot(const char*, void*)));
    delete [] path;
}


// Action for "Rename"
//
void
QTfilePopup::rename_cb_slot(const char *string, void*)
{
    char *path = get_selection();
    if (!path)
        return;
    if (lstring::strdirsep(path))
        wb_input->set_message("Invalid name, try again");
    else {
        char *npath = lstring::copy(path);
        char *t = lstring::strrdirsep(npath);
        if (t) {
            *t++ = 0;
            t = pathlist::mk_path(npath, string);
            if (rename(npath, t) != 0)
                PopUpMessage(strerror(errno), true);
            delete [] t;
        }
        else
            PopUpMessage("Internal error - bad path.", true);
        delete [] npath;
    }
    delete [] path;
    if (wb_input)
        wb_input->popdown();
}


void
QTfilePopup::new_root_slot()
{
    PopUpInput("Enter full path to new directory", rootdir, "Apply",
        0, 0, 300);
    connect(wb_input, SIGNAL(action_call(const char*, void*)),
        this, SLOT(root_cb_slot(const char*, void*)));
}


// Action for "New Root"
//
void
QTfilePopup::root_cb_slot(const char *rootin, void*)
{
    char *root = get_newdir(rootin);
    if (root) {
        delete [] rootdir;
        rootdir = root;
        init();
        if (wb_input)
            wb_input->popdown();
    }
}


void
QTfilePopup::new_cwd_slot()
{
    PopUpInput("Enter new current directory", cwd_bak, "Apply", 0, 0, 300);
    connect(wb_input, SIGNAL(action_call(const char*, void*)),
        this, SLOT(new_cwd_cb_slot(const char*, void*)));
}


void
QTfilePopup::new_cwd_cb_slot(const char *wd, void*)
{
    if (!wd || !*wd) {
#ifdef WIN32
        wd = getenv("HOME");
        if (!wd)
            wd = "c:/";
#else
        wd = "~";
#endif
    }
    char *nwd = get_newdir(wd);
    if (nwd) {
        if (!chdir(nwd)) {
            if (wb_input)
                wb_input->popdown();
            delete [] rootdir;
            rootdir = nwd;
            delete [] cwd_bak;
            cwd_bak = getcwd(0, 256);
            init();
        }
        else {
            char buf[256];
            snprintf(buf, sizeof(buf), "Directory change failed:\n%s",
                strerror(errno));
            PopUpMessage(buf, true);
            delete [] nwd;
        }
    }
}


void
QTfilePopup::show_filter_slot(bool shw)
{
    if (shw)
        filter->show();
    else
        filter->hide();
}


void
QTfilePopup::filter_choice_slot(int index)
{
    filter->setEditable((index > 1));
    filter_index = index;
    QLineEdit *ed = filter->lineEdit();
    if (ed)
        connect(ed, SIGNAL(editingFinished()),
            this, SLOT(list_files_slot()));
}


void
QTfilePopup::filter_change_slot(const QString &qs)
{
    char *text = lstring::copy(qs.toLatin1().constData());
    int i = filter->currentIndex();
    if (i > 1) {
        delete [] filter_options[i];
        filter_options[i] = text;
        filter->setItemText(i, QString(text));
    }
    else
        delete [] text;
}


void
QTfilePopup::quit_slot()
{
    emit dismiss();
    delete this;
}


void
QTfilePopup::help_slot()
{
    if (QTdev::self()->MainFrame())
        QTdev::self()->MainFrame()->PopUpHelp("filesel");
    else
        PopUpHelp("filesel");
}


// Handler for the Up menu.
//
void
QTfilePopup::up_menu_slot(QAction *a)
{
    // We can't call the root_cb_slot from here, since this destroys
    // the menu which we are currently in, causing a fault.  Uas an
    // idle procedure to avoid this.

    int offs = a->data().toInt();
    if (offs <= 0)
        offs = 1;
    if (offs >= 256)
        return;
    temp_string = lstring::copy(rootdir);
    temp_string[offs] = 0;
    QTimer *qt = new QTimer(this);
    qt->setSingleShot(true);
    qt->setInterval(0);
    connect(qt, SIGNAL(timeout()), this, SLOT(menu_update_slot()));
    qt->start();
}


void
QTfilePopup::menu_update_slot()
{
    root_cb_slot(temp_string, 0);
    delete [] temp_string;
    temp_string = 0;
}


// Tree selection handler
//
void
QTfilePopup::tree_select_slot(QTreeWidgetItem *cur, QTreeWidgetItem*)
{
    select_dir(cur);
    if (!cur) {
        if (list)
            list->clear();
        return;
    }
    char *dir = get_path(curnode, true);
    add_dir(curnode, dir);
    delete [] dir;
    list_files_slot();

    if (entry) {
        char *path = lstring::copy(entry->text().toLatin1().constData());
        if (path && *path) {
            const char *fname = lstring::strip_path(path);
            dir = get_path(curnode, false);
            char *fpath = pathlist::mk_path(dir, fname);
            entry->setText(QString(fpath));
            delete [] fpath;
            delete [] dir;
        }
        delete [] path;
    }
    else if (p_path_get && p_path_set) {
        dir = get_path(curnode, false);
        char *path = (*p_path_get)();
        if (path && *path) {
            char *fname = lstring::strip_path(path);
            fname = pathlist::mk_path(dir, fname);
            (*p_path_set)(fname);  // frees fname
            delete [] path;
            delete [] dir;
        }
        else {
            (*p_path_set)(dir);  // frees dir
            delete [] path;
        }
    }
}


// If the selected row is collapsed, deselect.  This does not happen
// automatically.
//
void
QTfilePopup::tree_collapse_slot(QTreeWidgetItem *item)
{
    item->setIcon(0, closed_folder_icon);
    QTreeWidgetItem *w = curnode;
    while (w) {
        if (w == item) {
            if (list)
                list->clear();
            select_dir(0);
            break;
        }
        w = w->parent();
    }
}


void
QTfilePopup::tree_expand_slot(QTreeWidgetItem *item)
{
    item->setIcon(0, open_folder_icon);
    QTreeWidgetItem *cur = tree->currentItem();
    QTreeWidgetItem *w = cur;
    while (w) {
        if (w == item) {
            tree_select_slot(cur, 0);
            break;
        }
        w = w->parent();
    }
}


// File matching, return true if fname should be listed.
//
inline bool
is_match(stringlist *s0, const char *fname)
{
    if (!s0)
        return (true);
    for (stringlist *s = s0; s; s = s->next) {
        if (!fnmatch(s->string, fname, 0))
            return (true);
    }
    return (false);
}


// List the files in the currently selected subdir, in the list window.
//
void
QTfilePopup::list_files_slot()
{
    if (!list)
        return;
    list->clear();
    char *dir = get_path(curnode, false);
    if (!dir)
        return;

    DIR *wdir;
    if (!(wdir = opendir(dir))) {
        delete [] dir;
        return;
    }
    char *p = new char[strlen(dir) + 128];
    strcpy(p, dir);
    delete [] dir;
    char *dt = p + strlen(p) - 1;
    if (!lstring::is_dirsep(*dt)) {
        *++dt = '/';
        *++dt = 0;
    }
    else
        dt++;
    stringlist *filt = tokenize_filter();
    struct dirent *de;
    while ((de = readdir(wdir)) != 0) {
        if (!strcmp(de->d_name, "."))
            continue;
        if (!strcmp(de->d_name, ".."))
            continue;
#ifdef DT_DIR
        if (de->d_type != DT_UNKNOWN) {
            // Linux glibc returns DT_UNKNOWN for everything
            if (de->d_type == DT_DIR)
                continue;
            if (de->d_type == DT_LNK) {
                strcpy(dt, de->d_name);
                if (filestat::is_directory(p))
                    continue;
            }
            if (!is_match(filt, de->d_name))
                continue;
            list->addItem(QString(de->d_name));
            continue;
        }
#endif
        if (!is_match(filt, de->d_name))
            continue;
        strcpy(dt, de->d_name);
        if (filestat::is_directory(p))
            continue;
        list->addItem(QString(de->d_name));
    }
    delete [] p;
    closedir(wdir);
    list->sortItems();
    stringlist::destroy(filt);

    select_file(0);
}


void
QTfilePopup::list_select_slot(QListWidgetItem *cur, QListWidgetItem*)
{
    if (cur) {
        char *str = lstring::copy(cur->text().toLatin1().constData());
        select_file(str);
        delete [] str;
    }
    else
        select_file(0);
}


void
QTfilePopup::list_double_clicked_slot(QListWidgetItem *item)
{
    if (item) {
        char *str = lstring::copy(item->text().toLatin1().constData());
        select_file(str);
        open_slot();
        delete [] str;
    }
}


void
QTfilePopup::flash_slot()
{
    if (flasher_cnt == 1)
        flasher_item->setBackgroundColor(0, QColor(255,255,255));
    else if (flasher_cnt == 2)
        flasher_item->setBackgroundColor(0, QColor(0,255,0));
    else if (flasher_cnt == 3)
        flasher_item->setBackgroundColor(0, QColor(255,255,255));
    else {
        flasher->stop();
        flasher_cnt = 0;
        flasher_item = 0;
        return;
    }
    flasher_cnt++;
}


inline bool
is_root(const char *str)
{
    if (lstring::is_dirsep(str[0]) && !str[1])
        return (true);
#ifdef WIN32
    if (strlen(rootdir) == 3 && isalpha(rootdir[0]) &&
            rootdir[1] == ':' && is_dirsep(rootdir[2]))
        return (true);
#endif
    return (false);
}


// Initialize the pop-up for a new root path.
//
void
QTfilePopup::init()
{
    if (list)
        list->clear();
    tree->clear();
    select_dir(0);
    select_file(0);
    if (rootdir && !is_root(rootdir)) {
        a_Up->setEnabled(true);
        a_UpMenu->setEnabled(true);
    }
    else {
        a_Up->setEnabled(false);
        a_UpMenu->setEnabled(false);
    }
    QTreeWidgetItem *prnt = insert_node(rootdir, 0);
    // prnt == 0 when rootdir == "/"
    add_dir(prnt, rootdir);
    set_label();
    if (prnt && (config == fsSEL || config == fsOPEN))
        tree->setItemSelected(prnt, true);
}


// Set/unset the sensitivity status of the file operations.
//
void
QTfilePopup::select_file(const char *fname)
{
    if (!list)
        return;
    delete [] curfile;
    if (fname) {
        curfile = lstring::copy(fname);
        a_Go->setEnabled(true);
        if (a_Open)
            a_Open->setEnabled(true);
    }
    else {
        curfile = 0;
        if (!no_disable_go)
            a_Go->setEnabled(false);
        if (a_Open)
            a_Open->setEnabled(false);
    }
}


// Set/unset the sensitivity status of the operations which work on
// directories.
//
void
QTfilePopup::select_dir(QTreeWidgetItem *node)
{
    if (node) {
        curnode = node;
        if (a_New)
            a_New->setEnabled(true);
        if (a_Delete)
            a_Delete->setEnabled(true);
        if (a_Rename)
            a_Rename->setEnabled(true);
    }
    else {
        curnode = 0;
        if (a_New)
            a_New->setEnabled(false);
        if (a_Delete)
            a_Delete->setEnabled(false);
        if (a_Rename)
            a_Rename->setEnabled(false);
    }
}


// Return the complete directory path to the node.  If noexpand is true,
// return 0 if the node has already been expanded.
//
char *
QTfilePopup::get_path(QTreeWidgetItem *node, bool noexpand)
{
    if (!node)
        return (0);
    if (noexpand && node->childCount() > 0)
        return (0);

    // construct the path
    char *s = lstring::copy(node->text(0).toLatin1().constData());
    while (node->parent()) {
        node = node->parent();
        char *c = lstring::copy(node->text(0).toLatin1().constData());
        char *t = new char[strlen(s) + strlen(c) + 2];
        strcpy(t, c);
        delete [] c;
        char *tt = t + strlen(t);
        *tt++ = '/';
        strcpy(tt, s);
        delete [] s;
        s = t;
    }
    if (!strcmp(rootdir, "/")) {
        if (*s == '/')
            return (s);
        else {
            char *t = new char[strlen(s) + 2];
            *t = '/';
            strcpy(t+1, s);
            delete s;
            return (t);
        }
    }
    if (!lstring::strrdirsep(s)) {
        delete [] s;
        return (lstring::copy(rootdir));
    }
    char *t = pathlist::mk_path(rootdir, lstring::strdirsep(s) + 1);
    delete [] s;
    return (t);
}


// Insert a new node into the tree.
//
QTreeWidgetItem *
QTfilePopup::insert_node(char *dir, QTreeWidgetItem *prnt)
{
    char *name = lstring::strrdirsep(dir);
    if (!name)
        return (0);
    name++;
    if (!*name)
        return (0);

    QTreeWidgetItem *node;
    if (prnt)
        node = new file_tree_item((file_tree_item*)prnt);
    else 
        node = new file_tree_item(tree);
    node->setText(0, QString(name));
    node->setIcon(0, closed_folder_icon);
    return (node);
}


// Add a directory's subdirectories to the tree.
//
void
QTfilePopup::add_dir(QTreeWidgetItem *prnt, char *dir)
{
    if (!dir || !*dir)
        return;
    char *p = new char[strlen(dir) + 64];
    strcpy(p, dir);
    char *dt = p + strlen(p) - 1;
    if (!lstring::is_dirsep(*dt)) {
        *++dt = '/';
        *++dt = 0;
    }
    else
        dt++;
    DIR *wdir = opendir(dir);
    stringlist *s0 = 0;
    if (wdir) {
        struct dirent *de;
        while ((de = readdir(wdir)) != 0) {
            if (!strcmp(de->d_name, "."))
                continue;
            if (!strcmp(de->d_name, ".."))
                continue;
            strcpy(dt, de->d_name);
#ifdef DT_DIR
            if (de->d_type != DT_UNKNOWN) {
                // Linux glibc returns DT_UNKNOWN for everything
                if (de->d_type == DT_DIR)
                    s0 = new stringlist(lstring::copy(p), s0);
                if (de->d_type != DT_LNK)
                    continue;
            }
#endif
            if (filestat::is_directory(p))
                s0 = new stringlist(lstring::copy(p), s0);
        }
        closedir(wdir);
        stringlist::sort(s0);
        for (stringlist *s = s0; s; s = s->next)
            insert_node(s->string, prnt);
        stringlist::destroy(s0);
    }
    delete [] p;
}


// Return a stringlist of the tokens in the present filename filter,
// or null if no filter.
//
stringlist *
QTfilePopup::tokenize_filter()
{
    const char *str = filter_options[filter_index];
    const char *s = strchr(str, ':');
    if (!s)
        s = str;
    else
        s++;
    while (isspace(*s))
        s++;
    if (!*s)
        return (0);
    stringlist *s0 = 0, *se = 0;
    char *tok;
    while ((tok = lstring::gettok(&s)) != 0) {
        if (s0) {
            se->next = new stringlist(tok, 0);
            se = se->next;
        }
        else
            s0 = se = new stringlist(tok, 0);
    }
    return (s0);
}


// Preprocess directory string input.
//
char *
QTfilePopup::get_newdir(const char *rootin)
{    
    char *root = pathlist::expand_path(rootin, true, true);
    if (!root || !*root) {
        PopUpMessage("Input syntax error", true);
        delete [] root;
        return (0);
    }
    if (!lstring::is_rooted(root)) {
        char *t = root;
        root = pathlist::mk_path(cwd_bak, root);
        delete [] t;
    }

    // remove trailing separator, if any
    char *t = root + strlen(root) - 1;
    while (t >= root && lstring::is_dirsep(*t))
        *t-- = 0;
    if (!root[0]) {
        root[0] = '/';
        root[1] = 0;
    }
    if (!filestat::is_directory(root)) {
        PopUpMessage("Input is not a readable directory", true);
        delete [] root;
        return (0);
    }
    return (root);
}


// Invoke the shell function in buf, and return any output
//
static char *
doit(char *buf)
{
    FILE *fp = popen(buf, "r");
    if (!fp)
        return (0);
    sLstr lstr;
    while (fgets(buf, 256, fp) != 0)
        lstr.add(buf);
    pclose(fp);
    return (lstr.string_trim());
}


// Actually perform the move/copy/link.
//
static void
DoFileAction(QTfilePopup *fs, const char *src, const char *dst,
    ActionType action)
{
    if (!src || !*src || !dst || !*dst || !strcmp(src, dst))
        return;
    if (action == A_NOOP)
        return;

    // prohibit src = path/file, dst = path
    if (lstring::prefix(dst, src)) {
        const char *s = src + strlen(dst);
        if (*s == '/') {
            s++;
            if (!strchr(s, '/'))
                return;
        }
    }

    // if src is a directory, prohibit a recursive hierarchy
    bool isdir = filestat::is_directory(src);
    if (isdir && lstring::prefix(src, dst))
        return;

    char *tbuf = 0, *err = 0;
    int len;
    switch (action) {
    case A_NOOP:
        break;
    case A_COPY:
        len = strlen(src) + strlen(dst) + 256;
        tbuf = new char[len];
        if (isdir)
            snprintf(tbuf, len, "cp -R %s %s 2>&1", src, dst);
        else
            snprintf(tbuf, len, "cp %s %s 2>&1", src, dst);
        err = doit(tbuf);
        break;
    case A_MOVE:
        len = strlen(src) + strlen(dst) + 256;
        tbuf = new char[len];
        snprintf(tbuf, len, "mv %s %s 2>&1", src, dst);
        err = doit(tbuf);
        break;
    case A_LINK:
        len = strlen(src) + strlen(dst) + 256;
        tbuf = new char[len];
        snprintf(tbuf, len, "ln -s %s %s 2>&1", dst, src);
        err = doit(tbuf);
        break;
    case A_ASK:
        return;
    }
    delete [] tbuf;

    if (err) {
        if (*err)
            fs->PopUpMessage(err, true);
        delete [] err;
    }
}


// File monitor - periodically check for changes in directory
// hierarchy and update display when necessary.  Set up a timer to
// monitor the currently displayed hierarchy.  If the modification
// time of any directory changes, update the directory and file
// listings.

static stringlist *list_node_children(QTreeWidgetItem*);
static stringlist *list_subdirs(const char*);
static void stringdiff(stringlist**, stringlist**);


// Return the modification time for dir.
//
inline unsigned int
dirtime(const char *dir)
{
    struct stat st;
    if (!stat(dir, &st))
        return (st.st_mtime);
    return (0);
}


// Return the child node of node with the given name.
//
inline QTreeWidgetItem *
find_child(QTreeWidgetItem *node, char *name)
{
    int n = node->childCount();
    for (int i = 0; i < n; i++) {
        QTreeWidgetItem *child = node->child(i);
        char *t = lstring::copy(child->text(0).toLatin1().constData());
        if (!strcmp(name, t)) {
            delete [] t;
            return (child);
        }
        delete [] t;
    }
    return (0);
}


// This is called periodically to update the displayed directory and
// file listings.
//
void
QTfilePopup::check_slot()
{
    char *cwd = getcwd(0, 256);
    if (cwd) {
        if (!cwd_bak || strcmp(cwd, cwd_bak)) {
            delete [] cwd_bak;
            cwd_bak = cwd;
            set_label();
        }
        else
            delete [] cwd;
    }

    // Hold nodes to delete until list iteration is through.
    QList<QTreeWidgetItem*> dead_list;

    QList<QTreeWidgetItem*> ilist = tree->findItems(QString("*"),
        Qt::MatchWildcard | Qt::MatchRecursive);
    for (int row = 0; row < ilist.size(); row++) {

        file_tree_item *node = (file_tree_item*)ilist[row];
        if (!node)
            continue;

        // There must be a better way to determine if a node is visible.
        QRect r = tree->visualItemRect(node);
        if (r.height() == 0)
            continue;

        char *dir = get_path(node, false);

        unsigned int ptime = dirtime(dir);
        if (node->mtime == ptime) {
            delete [] dir;
            continue;
        }
	    // we know that the content of the node directory has changed
        node->mtime = ptime;
		if (node == curnode)
			list_files_slot();  // this is selected node, update files list

        stringlist *sa = list_subdirs(dir);
        stringlist *sd = list_node_children(node);
	    stringdiff(&sa, &sd);

        if (sa || sd) {
            for (stringlist *s = sd; s; s = s->next) {
                QTreeWidgetItem *child = find_child(node, s->string);
                dead_list.append(child);
            }
            for (stringlist *s = sa; s; s = s->next) {
                char *ndir = pathlist::mk_path(dir, s->string);
                insert_node(ndir, node);
                delete [] ndir;
            }
            stringlist::destroy(sa);
            stringlist::destroy(sd);
        }
        delete [] dir;
    }

    for (int i = 0; i < dead_list.size(); i++)
        delete dead_list.at(i);
}


// Return a list of the names associated with the children of node.
//
static stringlist *
list_node_children(QTreeWidgetItem *node)
{
    stringlist *s0 = 0;
    int n = node->childCount();
    for (int i = 0; i < n; i++) {
        QTreeWidgetItem *child = node->child(i);
        char *t = lstring::copy(child->text(0).toLatin1().constData());
        s0 = new stringlist(t, s0);
    }
    return (s0);
}


// Return a list of subdirs under dir.
//
static stringlist *
list_subdirs(const char *dir)
{
    stringlist *s0 = 0;
    DIR *wdir = opendir(dir);
    if (wdir) {
        char *p = new char[strlen(dir) + 64];
        strcpy(p, dir);
        char *dt = p + strlen(p) - 1;
        if (!lstring::is_dirsep(*dt)) {
            *++dt = '/';
            *++dt = 0;
        }
        else
            dt++;
        struct dirent *de;
        while ((de = readdir(wdir)) != 0) {
            if (!strcmp(de->d_name, "."))
                continue;
            if (!strcmp(de->d_name, ".."))
                continue;
#ifdef DT_DIR
            if (de->d_type != DT_UNKNOWN) {
                // Linux glibc returns DT_UNKNOWN for everything
                if (de->d_type == DT_DIR)
                    s0 = new stringlist(lstring::copy(de->d_name), s0);
                if (de->d_type != DT_LNK)
                    continue;
            }
#endif
            strcpy(dt, de->d_name);
            if (filestat::is_directory(p))
                s0 = new stringlist(lstring::copy(de->d_name), s0);
        }
        delete [] p;
        closedir(wdir);
    }
    return (s0);
}


// Remove the entries that appear in both lists from both lists.
//
static void
stringdiff(stringlist **s1p, stringlist **s2p)
{
	stringlist *s1 = *s1p;
	stringlist *s2 = *s2p;
	for (stringlist *s = s1; s; s = s->next) {
		for (stringlist *ss = s2; ss; ss = ss->next) {
			if (!ss->string)
			    continue;
		    if (!strcmp(s->string, ss->string)) {
				delete [] ss->string;
				ss->string = 0;
				delete [] s->string;
				s->string = 0;
				break;
			}
		}
	}
	stringlist *s = s1;
	s1 = 0;
	while (s) {
		stringlist *sn = s->next;
		if (s->string) {
			s->next = s1;
			s1 = s;
		}
		else
		    delete s;
		s = sn;
	}
	s = s2;
	s2 = 0;
	while (s) {
		stringlist *sn = s->next;
		if (s->string) {
			s->next = s2;
			s2 = s;
		}
		else
		    delete s;
		s = sn;
	}
	*s1p = s1;
	*s2p = s2;
}


