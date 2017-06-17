
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
 $Id: fio_chd_diff.cc,v 1.15 2012/06/06 06:01:29 stevew Exp $
 *========================================================================*/

#include "fio.h"
#include "fio_chd.h"
#include "fio_chd_diff.h"
#include "cd_celldb.h"


//
// Implementation of chd-to-chd comparison.
//

CHDdiff::CHDdiff(cCHD *c1, cCHD *c2)
{
    df_st1 = 0;
    df_st2 = 0;
    df_chd1 = c1;
    df_chd2 = c2;
    df_layer_list = 0;
    df_obj_types = 0;
    df_max_diffs = 0;
    df_diff_count = 0;
    df_properties = 0;
    df_skip_layers = false;
    df_geometric = false;
    df_exp_arrays = false;
    df_sloppy_boxes = false;
    df_ignore_dups = false;
}


CHDdiff::~CHDdiff()
{
    if (df_st1 && *df_st1 && CDcdb()->findTable(df_st1)) {
        const char *curtab = CDcdb()->tableName();
        if (curtab && !strcmp(curtab, df_st1))
            curtab = 0;
        CDcdb()->switchTable(df_st1);
        if (CDcdb()->tableName() && !strcmp(CDcdb()->tableName(), df_st1))
            CDcdb()->clearTable(false);
        CDcdb()->switchTable(curtab);
    }
    if (df_st2 && *df_st2 && CDcdb()->findTable(df_st2)) {
        const char *curtab = CDcdb()->tableName();
        if (curtab && !strcmp(curtab, df_st2))
            curtab = 0;
        CDcdb()->switchTable(df_st2);
        if (CDcdb()->tableName() && !strcmp(CDcdb()->tableName(), df_st2))
            CDcdb()->clearTable(false);
        CDcdb()->switchTable(curtab);
    }
    delete [] df_st1;
    delete [] df_st2;
    delete [] df_layer_list;
    delete [] df_obj_types;
}


namespace {
    // Create a unique symbol table name.
    char *
    stname(const char *base)
    {
        static int n = 1000;
        char buf[32];
        char *s = lstring::stpcpy(buf, base);
        *s++ = '-';

        for (n++;; n++) {
            mmItoA(s, n);
            if (CDcdb()->findTable(buf))
                continue;
            return (lstring::copy(buf));
        }
    }
}


DFtype
CHDdiff::diff(const char *cname1, const char *cname2, DisplayMode mode,
    Sdiff **sdiffp)
{
    if (sdiffp)
        *sdiffp = 0;

    if (!cname2)
        cname2 = cname1;
    if (!df_st1)
        df_st1 = stname("diff");
    if (!df_st2)
        df_st2 = stname("diff");

    FIOreadPrms prms;

    OItype oiret = OIok;
    CDcbin cbin1;
    if (df_chd1) {
        if (df_chd1->findSymref(cname1, mode)) {
            const char *stbak = CDcdb()->tableName();
            CDcdb()->switchTable(df_st1);
            oiret = df_chd1->open(&cbin1, cname1, &prms, false);
            CDcdb()->switchTable(stbak);
        }
    }
    else
        oiret = CD()->OpenExisting(cname1, &cbin1);

    if (oiret == OIaborted)
        return (DFabort);
    if (oiret == OIerror)
        return (DFerror);

    CDcbin cbin2;
    if (df_chd2) {
        if (df_chd2->findSymref(cname2, mode)) {
            const char *stbak = CDcdb()->tableName();
            CDcdb()->switchTable(df_st2);
            oiret = df_chd2->open(&cbin2, cname2, &prms, false);
            CDcdb()->switchTable(stbak);
        }
    }
    else
        oiret = CD()->OpenExisting(cname2, &cbin2);

    if (oiret == OIaborted)
        return (DFabort);
    if (oiret == OIerror)
        return (DFerror);

    if (!cbin1.cellname())
        return (cbin2.cellname() ? DFnoL : DFnoLR);
    if (!cbin2.cellname())
        return (DFnoR);
    CDs *sd1 = cbin1.celldesc(mode);
    CDs *sd2 = cbin2.celldesc(mode);

    unsigned int flags = 0;
    if (df_skip_layers)
        flags |= DiffSkipLayers;
    if (df_geometric)
        flags |= DiffGeometric;
    if (df_exp_arrays)
        flags |= DiffExpArrays;
    if (df_properties)
        flags |= df_properties;;
    if (df_sloppy_boxes)
        flags |= DiffSloppyBoxes;
    if (df_ignore_dups)
        flags |= DiffIgnoreDups;

    CDdiff cdf;
    cdf.set_obj_types(df_obj_types);
    cdf.set_layer_list(df_layer_list);
    cdf.set_flags(flags);
    cdf.set_max_diffs(df_max_diffs);

    PrpFltMode m = PrpFltNone;
    if (df_properties & DiffPrpCstm)
        m = PrpFltCstm;
    else if (df_properties & DiffPrpDflt)
        m = PrpFltDflt;
    cdf.setup_filtering(mode, m);

    DFtype dft;
    if (sdiffp) {
        dft = cdf.diff(sd1, sd2, sdiffp);
        df_diff_count = cdf.diff_count();
    }
    else
        dft = cdf.diff(sd1, sd2);
    if (df_chd2) {
        const char *stbak = CDcdb()->tableName();
        CDcdb()->switchTable(df_st2);
        if (CDcdb()->tableName() && !strcmp(CDcdb()->tableName(), df_st2)) {
            CDcdb()->clearTable(false);
            CDcdb()->switchTable(stbak);
        }
    }
    if (df_chd1) {
        const char *stbak = CDcdb()->tableName();
        CDcdb()->switchTable(df_st1);
        if (CDcdb()->tableName() && !strcmp(CDcdb()->tableName(), df_st1)) {
            CDcdb()->clearTable(false);
            CDcdb()->switchTable(stbak);
        }
    }
    return (dft);
}

