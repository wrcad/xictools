
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "qtcvofmt.h"
#include "main.h"
#include "cvrt.h"
#include "fio.h"
#include "qtinterf/qtdblsb.h"

#include <QLayout>
#include <QGroupBox>
#include <QLabel>
#include <QComboBox>
#include <QCheckBox>
#include <QToolButton>
#include <QPushButton>
#include <QMenu>
#include <QAction>


//-----------------------------------------------------------------------------
// QTconvOutFmt;  Widget to specify an output file format.

#define FMT_MAX_ID 5

int QTconvOutFmt::fmt_gds_inp = 0;
char *QTconvOutFmt::fmt_oas_rep_string = 0;

const char *
QTconvOutFmt::fmt_gds_limits[] =
{
    "7  8000/8000",
    "3  600/200",
    "2  200/200",
    0
};

QTconvOutFmt::fmt_menu QTconvOutFmt::fmt_gds_input[] =
{
    { "archive", Fnone },
    { "gds-text",Fnone },
    { 0,         Fnone }
};

const char *
QTconvOutFmt::fmt_which_flags[] =
{
    "WITHOUT Strip For Export",
    "WITH Strip For Export"
};

QTconvOutFmt::fmt_menu QTconvOutFmt::fmt_cif_extensions[] =
{
    { "scale extension",        CIF_SCALE_EXTENSION },
    { "cell properties",        CIF_CELL_PROPERTIES },
    { "inst name comment",      CIF_INST_NAME_COMMENT },
    { "inst name extension",    CIF_INST_NAME_EXTENSION },
    { "inst magn extension ",   CIF_INST_MAGN_EXTENSION },
    { "inst array extension",   CIF_INST_ARRAY_EXTENSION },
    { "inst bound extension",   CIF_INST_BOUND_EXTENSION },
    { "inst properties",        CIF_INST_PROPERTIES },
    { "obj properties",         CIF_OBJ_PROPERTIES },
    { "wire extension",         CIF_WIRE_EXTENSION },
    { "wire extension new",     CIF_WIRE_EXTENSION_NEW },
    { "text extension",         CIF_TEXT_EXTENSION },
    { 0,                        0 }
};

QTconvOutFmt::fmt_menu QTconvOutFmt::fmt_cname_formats[] =
{
    { "9 cellname;",        EXTcnameDef },
    { "(cellname);",        EXTcnameNCA },
    { "(9 cellname);",      EXTcnameICARUS },
    { "(Name: cellname);",  EXTcnameSIF },
    { "No Cell Names",      EXTcnameNone },
    { 0,                    0 }
};

QTconvOutFmt::fmt_menu QTconvOutFmt::fmt_layer_formats[] =
{
    { "By Name",            EXTlayerDef },
    { "By Index",           EXTlayerNCA },
    { 0,                    0 }
};

QTconvOutFmt::fmt_menu QTconvOutFmt::fmt_label_formats[] = {
    { "94 text x y xform w h;",     EXTlabelDef },
    { "94 text x y;",               EXTlabelKIC },
    { "92 text x y layer_index;",   EXTlabelNCA },
    { "94 text x y layer_name;",    EXTlabelMEXTRA },
    { "No Labels",                  EXTlabelNone },
    { 0,                            0 }
};

QTconvOutFmt::QTconvOutFmt(void(*cb)(int), int init_format, cvofmt_mode fmtmode)
{
    fmt_level = 0;
    fmt_gdsmap = 0;
    fmt_gdscut = 0;
    fmt_cgxcut = 0;
    fmt_oasmap = 0;
    fmt_oascmp = 0;
    fmt_oasrep = 0;
    fmt_oastab = 0;
    fmt_oassum = 0;
    fmt_oasoff = 0;
    fmt_oasnwp = 0;
    fmt_oasadv = 0;
    fmt_gdsftlab = 0;
    fmt_gdsftopt = 0;
    fmt_cifext = 0;
    fmt_cifflags = 0;
    fmt_cifcname = 0;
    fmt_ciflayer = 0;
    fmt_ciflabel = 0;
    fmt_sb_gdsunit = 0;

    fmt_cb = cb;
    fmt_strip = FIO()->IsStripForExport();

    // GDSII page
    //
    QWidget *page = new QWidget();
    addTab(page, "GDSII");

    QMargins qmtop(2, 2, 2, 2);
    QMargins qm;
    QVBoxLayout *vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(qmtop);;
    vbox->setSpacing(2);

    QHBoxLayout *hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    fmt_gdsftopt = new QComboBox();
    hbox->addWidget(fmt_gdsftopt);;
    for (int i = 0; fmt_gds_input[i].name; i++)
        fmt_gdsftopt->addItem(tr(fmt_gds_input[i].name), i);
    fmt_gdsftopt->setCurrentIndex(QTconvOutFmt::fmt_gds_inp);
    connect(fmt_gdsftopt, SIGNAL(currentIndexChanged(int)),
        this, SLOT(gdsftopt_menu_changed(int)));

    fmt_gdsftlab = new QLabel(tr("Input File Type"));
    hbox->addWidget(fmt_gdsftlab);

    QLabel *label = new QLabel(tr("Output Format:  GDSII archive"));
    label->setAlignment(Qt::AlignCenter);
    hbox->addWidget(label);

    // next row
    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    fmt_level = new QComboBox();
    hbox->addWidget(fmt_level);
    for (int i = 0; fmt_gds_limits[i]; i++)
        fmt_level->addItem(tr(fmt_gds_limits[i]), i);
    fmt_level->setCurrentIndex(FIO()->GdsOutLevel());
    connect(fmt_level, SIGNAL(currentIndexChanged(int)),
        this, SLOT(gdslevel_menu_changed(int)));

    label = new QLabel(tr("GDSII version number, polyon/wire vertex limit"));
    label->setAlignment(Qt::AlignCenter);
    hbox->addWidget(label);

    // next row
    fmt_gdsmap = new QCheckBox(tr(
        "Skip layers without Xic to GDSII layer mapping"));
    vbox->addWidget(fmt_gdsmap);
    connect(fmt_gdsmap, SIGNAL(stateChanged(int)),
        this, SLOT(gdsmap_btn_slot(int)));

    // next row
    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    fmt_gdscut = new QCheckBox(tr("Accept but truncate too-long strings"));
    hbox->addWidget(fmt_gdscut);
    connect(fmt_gdscut, SIGNAL(stateChanged(int)),
        this, SLOT(gdscut_btn_slot(int)));
    QTdev::SetStatus(fmt_gdscut,
        CDvdb()->getVariable(VA_GdsTruncateLongStrings));

    hbox->addStretch(1);

    fmt_sb_gdsunit = new QTdoubleSpinBox();
    hbox->addWidget(fmt_sb_gdsunit);
    fmt_sb_gdsunit->setMinimum(0.01);
    fmt_sb_gdsunit->setMaximum(100.0);
    fmt_sb_gdsunit->setDecimals(5);
    double initd = 1.0;
    const char *s = CDvdb()->getVariable(VA_GdsMunit);
    if (s) {
        double v;
        if (sscanf(s, "%lf", &v) == 1 && v >= 0.01 && v <= 100.0)
            initd = v;
        else
            // bogus value, clear it
            CDvdb()->clearVariable(VA_GdsMunit);
    }
    fmt_sb_gdsunit->setValue(initd);
    connect(fmt_sb_gdsunit, SIGNAL(valueChanged(double)),
        this, SLOT(gdsunit_changed_slot(double)));

    label = new QLabel(tr("Unit Scale"));
    hbox->addWidget(label);

    // OASIS page
    //
    page = new QWidget();
    addTab(page, "OASIS");

    vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    label = new QLabel(tr("Output Format:  OASIS archive"));
    hbox->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    fmt_oasadv = new QToolButton();
    fmt_oasadv->setText(tr("Advanced"));
    hbox->addWidget(fmt_oasadv);
    fmt_oasadv->setCheckable(true);
    connect(fmt_oasadv, SIGNAL(toggled(bool)),
        this, SLOT(oasadv_btn_slot(bool)));

    // next row
    fmt_oasmap = new QCheckBox(tr(
        "Skip layers without Xic to GDSII layer mapping"));
    vbox->addWidget(fmt_oasmap);
    QTdev::SetStatus(fmt_oasmap, CDvdb()->getVariable(VA_NoGdsMapOk));
    connect(fmt_oasmap, SIGNAL(stateChanged(int)),
        this, SLOT(oasmap_btn_slot(int)));

    // next two rows, in columns
    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    QVBoxLayout *col1 = new QVBoxLayout();
    col1->setContentsMargins(qm);
    col1->setSpacing(2);
    hbox->addLayout(col1);

    QVBoxLayout *col2 = new QVBoxLayout();
    col1->setContentsMargins(qm);
    col1->setSpacing(2);
    hbox->addLayout(col2);

    fmt_oascmp = new QCheckBox(tr("Use compression"));
    col1->addWidget(fmt_oascmp);
    connect(fmt_oascmp, SIGNAL(stateChanged(int)),
        this, SLOT(oascmp_btn_slot(int)));
    QTdev::SetStatus(fmt_oascmp, CDvdb()->getVariable(VA_OasWriteCompressed));

    fmt_oasrep = new QCheckBox(tr("Find repetitions"));
    col2->addWidget(fmt_oasrep);
    connect(fmt_oasrep, SIGNAL(stateChanged(int)),
        this, SLOT(oasrep_btn_slot(int)));
    QTdev::SetStatus(fmt_oasrep, CDvdb()->getVariable(VA_OasWriteRep));

    fmt_oastab = new QCheckBox(tr("Use string tables"));
    col1->addWidget(fmt_oastab);
    connect(fmt_oastab, SIGNAL(stateChanged(int)),
        this, SLOT(oastab_btn_slot(int)));
    QTdev::SetStatus(fmt_oastab, CDvdb()->getVariable(VA_OasWriteNameTab));

    fmt_oassum = new QCheckBox(tr("Write CRC checksum"));
    col2->addWidget(fmt_oassum);
    connect(fmt_oassum, SIGNAL(stateChanged(int)),
        this, SLOT(oassum_btn_slot(int)));
    QTdev::SetStatus(fmt_oassum, CDvdb()->getVariable(VA_OasWriteChecksum));

    // CIF page
    //
    page = new QWidget();
    addTab(page, "CIF");

    vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    label = new QLabel(tr("Output Format:  CIF archive"));
    hbox->addWidget(label);
    label->setAlignment(Qt::AlignCenter);
    hbox->addSpacing(20);

    fmt_cifext = new QToolButton();
    fmt_cifext->setText(tr("Extension Flags"));
    hbox->addWidget(fmt_cifext);
    hbox->addSpacing(20);
    fmt_cifflags = new QMenu();
    fmt_cifext->setMenu(fmt_cifflags);

    unsigned int flgs = fmt_strip ?
        FIO()->CifStyle().flags_export() : FIO()->CifStyle().flags();
    fmt_cifflags->addAction(fmt_which_flags[(int)fmt_strip]);

    fmt_cifflags->addSeparator();
    int i = 1;
    for (fmt_menu *m = fmt_cif_extensions; m->name; m++) {
        QAction *a = fmt_cifflags->addAction(m->name);
        a->setCheckable(true);
        a->setChecked(m->code & flgs);
        a->setData(i++);
    }
    connect(fmt_cifflags, SIGNAL(triggered(QAction*)),
        this, SLOT(cif_flags_slot(QAction*)));

    QToolButton *tbtn = new QToolButton();
    tbtn->setText(tr("Last Seen"));
    hbox->addWidget(tbtn);
    tbtn->setMaximumWidth(100);
    connect(tbtn, SIGNAL(clicked()), this, SLOT(ciflast_btn_slot()));

    // next two rows in three columns
    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    col1 = new QVBoxLayout();
    hbox->addLayout(col1);
    col1->setContentsMargins(qm);
    col1->setSpacing(2);

    col2 = new QVBoxLayout();
    hbox->addLayout(col2);
    col2->setContentsMargins(qm);
    col2->setSpacing(2);

    QVBoxLayout *col3 = new QVBoxLayout();
    hbox->addLayout(col3);
    col3->setContentsMargins(qm);
    col3->setSpacing(2);

    label = new QLabel(tr("Cell Name Extension"));
    col1->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    fmt_cifcname = new QComboBox();
    col1->addWidget(fmt_cifcname);;

    int ax = -1;
    for (fmt_menu *m = fmt_cname_formats; m->name; m++) {
        fmt_cifcname->addItem(m->name);
        if (m->code == FIO()->CifStyle().cname_type())
            ax = m - fmt_cname_formats;
    }
    if (ax >= 0)
        fmt_cifcname->setCurrentIndex(ax);
    connect(fmt_cifcname, SIGNAL(currentIndexChanged(int)),
        this, SLOT(cifcname_menu_changed(int)));

    label = new QLabel(tr("Layer Specification"));
    col2->addWidget(label);

    fmt_ciflayer = new QComboBox();
    col2->addWidget(fmt_ciflayer);;

    ax = -1;
    for (fmt_menu *m = fmt_layer_formats; m->name; m++) {
        fmt_ciflayer->addItem(m->name);
        if (m->code == FIO()->CifStyle().layer_type())
            ax = m - fmt_layer_formats;
    }
    if (ax >= 0)
        fmt_ciflayer->setCurrentIndex(ax);
    connect(fmt_ciflayer, SIGNAL(currentIndexChanged(int)),
        this, SLOT(ciflayer_menu_changed(int)));

    label = new QLabel(tr("Label Extension"));
    col3->addWidget(label);

    fmt_ciflabel = new QComboBox();
    col3->addWidget(fmt_ciflabel);;

    ax = -1;
    for (fmt_menu *m = fmt_label_formats; m->name; m++) {
        fmt_ciflabel->addItem(m->name);
        if (m->code == FIO()->CifStyle().label_type())
            ax = m - fmt_label_formats;
    }
    if (ax >= 0)
        fmt_ciflabel->setCurrentIndex(ax);
    connect(fmt_ciflabel, SIGNAL(currentIndexChanged(int)),
        this, SLOT(ciflabel_menu_changed(int)));
        
    // CGX page
    //
    page = new QWidget();
    addTab(page, "CGX");

    vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    label = new QLabel(tr("Output Format: CGX archive"));
    vbox->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    fmt_cgxcut = new QCheckBox(tr("Accept but truncate too-long strings"));
    vbox->addWidget(fmt_cgxcut);
    connect(fmt_cgxcut, SIGNAL(stateChanged(int)),
        this, SLOT(cgxcut_btn_slot(int)));
    QTdev::SetStatus(fmt_cgxcut,
        CDvdb()->getVariable(VA_GdsTruncateLongStrings));

    // XIC page
    //
    page = new QWidget();
    addTab(page, "XIC cell files");

    vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    label = new QLabel(tr("Output Format: XIC native cell files"));
    vbox->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    // TEXT page
    //
    page = new QWidget();
    addTab(page, "ASCII text");

    vbox = new QVBoxLayout(page);
    vbox->setContentsMargins(qmtop);
    vbox->setSpacing(2);

    label = new QLabel(tr("Output Format:  ASCII text file"));
    vbox->addWidget(label);
    label->setAlignment(Qt::AlignCenter);

    hbox = new QHBoxLayout();
    vbox->addLayout(hbox);
    hbox->setContentsMargins(qm);
    hbox->setSpacing(2);

    fmt_oasoff = new QCheckBox(tr("OASIS text: print offsets"));
    hbox->addWidget(fmt_oasoff);
    connect(fmt_oasoff, SIGNAL(stateChanged(int)),
        this, SLOT(oasoff_btn_slot(int)));
    QTdev::SetStatus(fmt_oasoff, CDvdb()->getVariable(VA_OasPrintOffset));

    fmt_oasnwp = new QCheckBox(tr("OASIS text: no line wrap"));
    hbox->addWidget(fmt_oasnwp);
    connect(fmt_oasnwp, SIGNAL(stateChanged(int)),
        this, SLOT(oasnwp_btn_slot(int)));
    QTdev::SetStatus(fmt_oasnwp, CDvdb()->getVariable(VA_OasPrintNoWrap));

    // End of pages

    if (init_format >= 0 && init_format <= FMT_MAX_ID)
        setCurrentIndex(init_format);

    configure(fmtmode);

    connect(this, SIGNAL(currentChanged(int)),
        this, SLOT(format_changed_slot(int)));
}


QTconvOutFmt::~QTconvOutFmt()
{
    if (fmt_oasadv && QTdev::GetStatus(fmt_oasadv))
        QTdev::CallCallback(fmt_oasadv);
}


void
QTconvOutFmt::update()
{
    QTdev::SetStatus(fmt_gdsmap,
        CDvdb()->getVariable(VA_NoGdsMapOk));
    QTdev::SetStatus(fmt_gdscut,
        CDvdb()->getVariable(VA_GdsTruncateLongStrings));
    QTdev::SetStatus(fmt_cgxcut,
        CDvdb()->getVariable(VA_GdsTruncateLongStrings));
    QTdev::SetStatus(fmt_oasmap,
        CDvdb()->getVariable(VA_NoGdsMapOk));
    QTdev::SetStatus(fmt_oascmp,
        CDvdb()->getVariable(VA_OasWriteCompressed));
    QTdev::SetStatus(fmt_oastab,
        CDvdb()->getVariable(VA_OasWriteNameTab));
    QTdev::SetStatus(fmt_oasrep,
        CDvdb()->getVariable(VA_OasWriteRep));
    QTdev::SetStatus(fmt_oassum,
        CDvdb()->getVariable(VA_OasWriteChecksum));
    fmt_level->setCurrentIndex(FIO()->GdsOutLevel());

    double initd = 1.0;
    const char *s = CDvdb()->getVariable(VA_GdsMunit);
    if (s) {
        double v;
        if (sscanf(s, "%lf", &v) == 1 && v >= 0.01 && v <= 100.0)
            initd = v;
        else
            // bogus value, clear it
            CDvdb()->clearVariable(VA_GdsMunit);
    }
    fmt_sb_gdsunit->setValue(initd);

    unsigned int flgs = fmt_strip ?
        FIO()->CifStyle().flags_export() : FIO()->CifStyle().flags();
    QList<QAction*> acts = fmt_cifflags->actions();
    int cnt = 0;
    for (int i = 1; i < acts.size(); i++) {
        if (acts[i]->data().toInt() == 0)
            continue;
        acts[i]->setChecked(fmt_cif_extensions[cnt].code & flgs);
        cnt++;
    }

    for (int i = 0; fmt_cname_formats[i].name; i++) {
        if (fmt_cname_formats[i].code == FIO()->CifStyle().cname_type()) {
            fmt_cifcname->setCurrentIndex(i);
            break;
        }
    }
    for (int i = 0; fmt_layer_formats[i].name; i++) {
        if (fmt_layer_formats[i].code == FIO()->CifStyle().layer_type()) {
            fmt_ciflayer->setCurrentIndex(i);
            break;
        }
    }
    for (int i = 0; fmt_label_formats[i].name; i++) {
        if (fmt_label_formats[i].code == FIO()->CifStyle().label_type()) {
            fmt_ciflabel->setCurrentIndex(i);
            break;
        }
    }
    QTdev::SetStatus(fmt_oasoff,
        CDvdb()->getVariable(VA_OasPrintOffset));
    QTdev::SetStatus(fmt_oasnwp,
        CDvdb()->getVariable(VA_OasPrintNoWrap));
}


bool
QTconvOutFmt::gds_text_input()
{
    return (fmt_gds_inp);
}


void
QTconvOutFmt::configure(cvofmt_mode mode)
{
    if (mode == cvofmt_file) {
        // Converting layout file.
        fmt_gdsmap->hide();
        fmt_gdsftlab->show();
        fmt_gdsftopt->show();
        fmt_oasmap->hide();
        for (int i = 0; i < count(); i++)
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
            setTabVisible(i, true);
#else
            setTabEnabled(i, true);
#endif

    }
    else if (mode == cvofmt_native) {
        // Converting native cell files.
        fmt_gdsmap->hide();
        fmt_gdsftlab->hide();
        fmt_gdsftopt->hide();
        fmt_oasmap->hide();
        // hide 4 (XIC) and larger
        for (int i = 0; i < count(); i++)

#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
            setTabVisible(i, (i < 4));
#else
            setTabEnabled(i, (i < 4));
#endif
    }
    else if (mode == cvofmt_chd) {
        // Converting using CHD.
        fmt_gdsmap->hide();
        fmt_gdsftlab->hide();
        fmt_gdsftopt->hide();
        fmt_oasmap->hide();
        // hide 4 (XIC) and larger but keep 7 (CGD)
        for (int i = 0; i < count(); i++)
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
            setTabVisible(i, (i < 4) || i == 7);
#else
            setTabEnabled(i, (i < 4) || i == 7);
#endif
    }
    else if (mode == cvofmt_chdfile) {
        // Converting using CHD file.
        fmt_gdsmap->hide();
        fmt_gdsftlab->hide();
        fmt_gdsftopt->hide();
        fmt_oasmap->hide();
        // hide 4 (XIC) and larger but keep 6 and 7 (CHD and CGD)
        for (int i = 0; i < count(); i++)
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
            setTabVisible(i, (i < 4) || i == 6 || i == 7);
#else
            setTabEnabled(i, (i < 4) || i == 6 || i == 7);
#endif
    }
    else if (mode == cvofmt_asm) {
        // Assembling.
        fmt_gdsmap->show();
        fmt_gdsftlab->hide();
        fmt_gdsftopt->hide();
        fmt_oasmap->show();
        // hide 5 (Text) and larger
        for (int i = 0; i < count(); i++)
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
            setTabVisible(i, (i < 5));
#else
            setTabEnabled(i, (i < 5));
#endif
    }
    else if (mode == cvofmt_db) {
        // Setting options (export control).
        fmt_gdsmap->show();
        fmt_gdsftlab->hide();
        fmt_gdsftopt->hide();
        fmt_oasmap->show();
        // hide 4 (XIC) and larger
        for (int i = 0; i < count(); i++)
#if QT_VERSION >= QT_VERSION_CHECK(5,15,0)
            setTabVisible(i, (i < 4));
#else
            setTabEnabled(i, (i < 4));
#endif
    }
}


void
QTconvOutFmt::set_page(int ix)
{
    if (ix >= 0 && ix <= 7)
        setCurrentIndex(ix);
}


void
QTconvOutFmt::gdsftopt_menu_changed(int i)
{
    fmt_gds_inp = i;
    if (fmt_cb)
        (*fmt_cb)(cConvert::cvGds);
}


void
QTconvOutFmt::gdslevel_menu_changed(int i)
{
    if (i > 0)
        CDvdb()->setVariable(VA_GdsOutLevel, i == 1 ? "1" : "2");
    else if (i == 0)
        CDvdb()->clearVariable(VA_GdsOutLevel);
}


void
QTconvOutFmt::gdsmap_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoGdsMapOk, 0);
    else
        CDvdb()->clearVariable(VA_NoGdsMapOk);
}


void
QTconvOutFmt::gdscut_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_GdsTruncateLongStrings, 0);
    else
        CDvdb()->clearVariable(VA_GdsTruncateLongStrings);
}


void
QTconvOutFmt::gdsunit_changed_slot(double val)
{
    if (val == 1.0)
        CDvdb()->clearVariable(VA_GdsMunit);
    else {
        char buf[32];
        snprintf(buf, sizeof(buf), "%.5f", val);
        CDvdb()->setVariable(VA_GdsMunit, buf);
    }
}


void
QTconvOutFmt::oasadv_btn_slot(bool state)
{
    if (state) {
        int xx, yy;
        QTdev::self()->Location(fmt_oasadv, &xx, &yy);
        Cvt()->PopUpOasAdv(fmt_oasadv, MODE_ON, xx, yy);
    }
    else
        Cvt()->PopUpOasAdv(0, MODE_OFF, 0, 0);
}


void
QTconvOutFmt::oasmap_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_NoGdsMapOk, 0);
    else
        CDvdb()->clearVariable(VA_NoGdsMapOk);
}


void
QTconvOutFmt::oascmp_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_OasWriteCompressed, 0);
    else
        CDvdb()->clearVariable(VA_OasWriteCompressed);
}


void
QTconvOutFmt::oasrep_btn_slot(int state)
{
    if (state) {
        if (!rep_string())
            set_rep_string(lstring::copy(""));
        CDvdb()->setVariable(VA_OasWriteRep, rep_string());
    }
    else
        CDvdb()->clearVariable(VA_OasWriteRep);
}


void
QTconvOutFmt::oastab_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_OasWriteNameTab, 0);
    else
        CDvdb()->clearVariable(VA_OasWriteNameTab);
}


void
QTconvOutFmt::oassum_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_OasWriteChecksum, 0);
    else
        CDvdb()->clearVariable(VA_OasWriteChecksum);
}


namespace {
    // Update the CifOutStyle variable.
    //
    void resetvar()
    {
        char *string = FIO()->CifStyle().string();
        if (!strcmp(string, "x"))
            CDvdb()->clearVariable(VA_CifOutStyle);
        else
            CDvdb()->setVariable(VA_CifOutStyle, string);
        delete [] string;
    }
}


void
QTconvOutFmt::cifcname_menu_changed(int i)
{
    FIO()->CifStyle().set_cname_type((EXTcnameType)fmt_cname_formats[i].code);
    resetvar();
}


void
QTconvOutFmt::ciflayer_menu_changed(int i)
{
    FIO()->CifStyle().set_layer_type((EXTlayerType)fmt_layer_formats[i].code);
    resetvar();
}


void
QTconvOutFmt::ciflabel_menu_changed(int i)
{
    FIO()->CifStyle().set_label_type((EXTlabelType)fmt_label_formats[i].code);
    resetvar();
}


void
QTconvOutFmt::cif_flags_slot(QAction *a)
{
    if (a->data().toInt() == 0) {
        fmt_strip = !fmt_strip;
        unsigned int flgs = fmt_strip ?
            FIO()->CifStyle().flags_export() : FIO()->CifStyle().flags();
        a->setText(fmt_which_flags[(int)fmt_strip]);
        QList<QAction*> acts = fmt_cifflags->actions();
        int cnt = 0;
        for (int i = 1; i < acts.size(); i++) {
            if (acts[i]->data().toInt() == 0)
                continue;
            acts[i]->setChecked(fmt_cif_extensions[cnt].code & flgs);
            cnt++;
        }
        return;
    }
    int i = a->data().toInt() - 1;
    if (QTdev::GetStatus(a)) {
        if (fmt_strip)
            FIO()->CifStyle().set_flag_export(fmt_cif_extensions[i].code);
        else
            FIO()->CifStyle().set_flag(fmt_cif_extensions[i].code);
    }
    else {
        if (fmt_strip)
            FIO()->CifStyle().unset_flag_export(fmt_cif_extensions[i].code);
        else
            FIO()->CifStyle().unset_flag(fmt_cif_extensions[i].code);
    }
    if (FIO()->CifStyle().flags() != 0xfff ||
            FIO()->CifStyle().flags_export() != 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%d %d", FIO()->CifStyle().flags(),
            FIO()->CifStyle().flags_export());
        CDvdb()->setVariable(VA_CifOutExtensions, buf);
    }
    else
        CDvdb()->clearVariable(VA_CifOutExtensions);
}


void
QTconvOutFmt::ciflast_btn_slot()
{
    char *string = FIO()->LastCifStyle();
    if (!strcmp(string, "x"))
        CDvdb()->clearVariable(VA_CifOutStyle);
    else
        CDvdb()->setVariable(VA_CifOutStyle, string);
    delete [] string;
}


void
QTconvOutFmt::cgxcut_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_GdsTruncateLongStrings, 0);
    else
        CDvdb()->clearVariable(VA_GdsTruncateLongStrings);
}


void
QTconvOutFmt::oasoff_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_OasPrintOffset, 0);
    else
        CDvdb()->clearVariable(VA_OasPrintOffset);
}


void
QTconvOutFmt::oasnwp_btn_slot(int state)
{
    if (state)
        CDvdb()->setVariable(VA_OasPrintNoWrap, 0);
    else
        CDvdb()->clearVariable(VA_OasPrintNoWrap);
}


void
QTconvOutFmt::format_changed_slot(int page)
{
    if (fmt_cb)
        (*fmt_cb)(page);
}

