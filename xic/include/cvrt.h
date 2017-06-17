
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
 $Id: cvrt.h,v 5.39 2016/03/02 00:39:43 stevew Exp $
 *========================================================================*/

#ifndef CVRT_H
#define CVRT_H

#include "cvrt_variables.h"


struct CmdDesc;
struct mitem_t;

inline class cConvert *Cvt();

class cConvert
{
    static cConvert *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cConvert *Cvt() { return (cConvert::ptr()); }

    // Arg to PopUpConvert.
    enum { cvDefault, cvLayoutFile, cvChdName, cvChdFile, cvNativeDir };

    // Notebook pages in Convert pop-up.
    enum { cvGds, cvOas, cvCif, cvCgx, cvXic, cvTxt, cvChd, cvCgd };

    // High bits of int passed to Conversion panel callback.
    enum { CVlayoutFile, CVgdsText, CVchdName, CVchdFile, CVnativeDir };

    // cvrt.cc
    cConvert();
    void UpdatePopUps();
    void CheckEmpties(bool);
    bool Export(const char*, bool);
    void ReadIntoCurrent(const char*, const char*, bool, FIOreadPrms*);
    void MergeHier(CDs*, CDs*, const char*, const char*);
    void CutWindowExec(CmdDesc*);
    void SetConvertFilename(const char*);
    void SetupConvertCut(const BBox*);
    void SetWriteFilename(const char*);
    void SetupWriteCut(const BBox*);

    const char *ConvertFilename()       { return (cvt_cv_filename); }
    const char *WriteFilename()         { return (cvt_wr_filename); }

    FILE *LogFp() { return (cvt_log_fp); }
    void SetLogFp(FILE *fp) { cvt_log_fp = fp; }
    bool ShowLog() { return (cvt_show_log); }
    void SetShowLog(bool b) { cvt_show_log = b; }

    // graphics
    void PopUpFiles(GRobject, ShowMode);
    void PopUpLibraries(GRobject, ShowMode);
    void PopUpAssemble(GRobject, ShowMode);
    void PopUpCompare(GRobject, ShowMode);
    void PopUpPropertyFilter(GRobject, ShowMode);
    void PopUpExport(GRobject, ShowMode, bool(*)(FileType, bool, void*), void*);
    void PopUpImport(GRobject, ShowMode, bool(*)(int, void*), void*);
    void PopUpConvert(GRobject, ShowMode, int, bool(*)(int, void*), void*);
    void PopUpOasAdv(GRobject, ShowMode, int, int);
    void PopUpHierarchies(GRobject, ShowMode);
    void PopUpChdOpen(GRobject, ShowMode, const char*, const char*, int, int,
        bool(*)(const char*, const char*, int, void*), void*);
    void PopUpCgdOpen(GRobject, ShowMode, const char*, const char*, int, int,
        bool(*)(const char*, const char*, int, void*), void*);
    void PopUpChdSave(GRobject, ShowMode, const char*, int, int,
        bool(*)(const char*, bool, void*), void*);
    void PopUpChdConfig(GRobject, ShowMode, const char*, int, int);
    void PopUpDisplayWindow(GRobject, ShowMode, const BBox*,
            bool(*)(bool, const BBox*, void*), void*);
    void PopUpAuxTab(GRobject, ShowMode);
    void PopUpGeometries(GRobject, ShowMode);
    bool PopUpMergeControl(ShowMode, mitem_t*);
    void PopUpEmpties(stringlist*);

private:
    // cvrt.cc
    void mergeHier_rc(CDcellName, CDcellName, const char*, const char*,
        ptrtab_t*);

    // cvrt_setif.cc
    void setupInterface();

    // cvrt_variables.cc
    void setupVariables();

    // funcs_cvrt.cc
    void load_funcs_cvrt();

    FILE *cvt_log_fp;       // Conversion log file pointer.
    bool cvt_show_log;      // Conversion log display flag.

    char *cvt_cv_filename;  // Pushed file name for Convert
    BBox cvt_cv_aoi;        // Pushed AOI name for Convert
    bool cvt_cv_win;        // Pushed UseWindow name for Convert
    bool cvt_cv_clip;       // Pushed Clip name for Convert
    bool cvt_cv_flat;       // Pushed Flaten name for Convert

    char *cvt_wr_filename;  // Pushed file name for Write
    BBox cvt_wr_aoi;        // Pushed AOI name for Write
    bool cvt_wr_win;        // Pushed UseWindow name for Write
    bool cvt_wr_clip;       // Pushed Clip name for Write
    bool cvt_wr_flat;       // Pushed Flaten name for Write

    static cConvert *instancePtr;
};

#endif

