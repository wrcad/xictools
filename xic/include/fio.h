
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
 $Id: fio.h,v 5.108 2016/03/02 23:32:24 stevew Exp $
 *========================================================================*/

#ifndef FIO_H
#define FIO_H

#include "cd.h"
#include "cd_types.h"
#include "fio_if.h"


class cCHD;
class cCGD;
struct FIOaliasTab;
struct cv_out;
struct cv_in;
struct cv_alias_info;
struct cv_cgd_if;
struct sLibRef;
struct sLib;
struct symref_t;

// General-purpose three-state flag.
enum tristate_t { ts_unset, ts_set, ts_ambiguous };

// This struct is passed to functions that read cells from layout
// files into the (in memory) database.  It enables/disables various
// options.
//
struct FIOreadPrms
{
    FIOreadPrms()
        {
            rp_scale = 1.0;
            rp_alias_mask = 0;
            rp_allow_layer_mapping = false;
        }

    double scale()              const { return (rp_scale); }
    unsigned int alias_mask()   const { return (rp_alias_mask); }
    bool allow_layer_mapping()  const { return (rp_allow_layer_mapping); }

    void set_scale(double s)                { rp_scale = s; }
    void set_alias_mask(unsigned int m)     { rp_alias_mask = m; }
    void set_allow_layer_mapping(bool b)    { rp_allow_layer_mapping = b; }

private:
    double rp_scale;                // scale when reading
    unsigned int rp_alias_mask;     // cdllname alias modes enabled
    bool rp_allow_layer_mapping;    // allow layer mapping in reading
};

// Empty cell filtering level.
//  ECFnone
//    No empty cell filtering.
//  ECFall
//    Apply both pre- and post-filtering.
//  ECFpre
//    If layer filtering, apply a pre-filtering based on CHD object
//    per-cell/per-layer counts.  This is a low-overhead operation,
//    but requires cvINFOplpc info in the CHD, and only applies when
//    layer filtering is used.
//  ECFpost
//    This actually looks in the input file to determine which cells
//    are empty, recursively.  This can be fairly expensive.
//
enum ECFlevel { ECFnone, ECFall, ECFpre, ECFpost };

// This is passed to functions that write or translate layout file
// data.
//
struct FIOcvtPrms
{
    FIOcvtPrms();
    FIOcvtPrms(const FIOcvtPrms&);
    ~FIOcvtPrms();

    double scale()              const { return (cp_scale); }
    const char *destination()   const { return (cp_destination); }
    FileType filetype()         const { return (cp_filetype); }
    bool allow_layer_mapping()  const { return (cp_allow_layer_mapping); }
    const BBox *window()        const { return (&cp_window); }
    bool use_window()           const { return (cp_use_window); }
    bool flatten()              const { return (cp_flatten); }
    bool clip()                 const { return (cp_clip); }
    ECFlevel ecf_level()        const { return ((ECFlevel)cp_ecf_level); }
    unsigned int alias_mask()   const { return (cp_alias_mask); }
    bool to_cgd()               const { return (cp_to_cgd); }

    const cv_alias_info *alias_info()   const { return (cp_alias_info); }

    void set_scale(double s)                { cp_scale = s; }
    void set_destination(const char*, FileType, bool=false);
    void set_allow_layer_mapping(bool b)    { cp_allow_layer_mapping = b; }
    void set_window(const BBox *BB)         { cp_window = *BB; }
    void set_use_window(bool b)             { cp_use_window = b; }
    void set_flatten(bool b)                { cp_flatten = b; }
    void set_clip(bool b)                   { cp_clip = b; }
    void set_ecf_level(ECFlevel f)          { cp_ecf_level = f; }
    void set_alias_mask(unsigned int m)     { cp_alias_mask = m; }

    void set_alias_info(const cv_alias_info *i)     { cp_alias_info = i; }

private:
    double cp_scale;                    // conversion scaling
    const char *cp_destination;         // output file/directory
    const cv_alias_info *cp_alias_info; // alias state from CHD
    BBox cp_window;                     // window, if windowing
    unsigned int cp_alias_mask;         // flags to merge during oper
    FileType cp_filetype;               // output file format type
    bool cp_allow_layer_mapping;        // allow layer mapping in translation
    bool cp_use_window;                 // true if windowing
    bool cp_flatten;                    // true if flattening
    bool cp_clip;                       // true if clipping to window
    unsigned char cp_ecf_level;         // nonzero if filtering empties
    bool cp_to_cgd;                     // true when creating a CGD

    // If cp_to_cgd is set, the geometry output goes to an in-core
    // database rather than to a file.  The filetype must be Foas, and
    // the destination name is the name that the database will be
    // saved under.
};

// for cFIO::IsCIF()
enum CFtype { CFnative, CFigs, CFnca, CFicarus, CFsif, CFnone };

// CIF cell name extensions (after DS)
//    Xic:             9 cell_name;
//    SQUID:           9 full_name;   (not suported in output)
//    Stanford/NCA:    (cell_name);
//    Icarus:          (9 cell_name);
//    Sif:             (Name: cell_name);
//    none

enum { EXTcnameDef, EXTcnameNCA, EXTcnameICARUS, EXTcnameSIF, EXTcnameNone };
typedef unsigned char EXTcnameType;

// CIF layer specification
//    Xic              L layer_name;
//    NCA              L layer_index;

enum { EXTlayerDef, EXTlayerNCA };
typedef unsigned char EXTlayerType;

// CIF label extensions
//    Xic              94 <<label>> x y orient_code width height;
//    KIC              94 label x y;
//    NCA              92 label x y layer_index;
//    mextra           94 label x y layer_name;
//    none

enum { EXTlabelDef, EXTlabelKIC, EXTlabelNCA, EXTlabelMEXTRA, EXTlabelNone };
typedef unsigned char EXTlabelType;

// CIF: How to resolve layers when reading
//    Def              Try name match, then index match, then create
//    Name             Try name match, then create
//    Index            Try index match (only)

enum { EXTlreadDef, EXTlreadName, EXTlreadIndex };
typedef unsigned char EXTlreadType;

// Flags for misc. attributes in CIF output.
#define CIF_SCALE_EXTENSION         0x1
#define CIF_CELL_PROPERTIES         0x2
#define CIF_INST_NAME_COMMENT       0x4
#define CIF_INST_NAME_EXTENSION     0x8
#define CIF_INST_MAGN_EXTENSION     0x10
#define CIF_INST_ARRAY_EXTENSION    0x20
#define CIF_INST_BOUND_EXTENSION    0x40
#define CIF_INST_PROPERTIES         0x80
#define CIF_OBJ_PROPERTIES          0x100
#define CIF_WIRE_EXTENSION          0x200
#define CIF_WIRE_EXTENSION_NEW      0x400
#define CIF_TEXT_EXTENSION          0x800


// Global dialect parameters, set when reading CIF, used when writing CIF.
//
struct CIFstyle
{
    CIFstyle()
        {
            cs_cnametype = EXTcnameDef;
            cs_layertype = EXTlayerDef;
            cs_labeltype = EXTlabelDef;
            cs_lreadtype = EXTlreadDef;
            cs_flags = 0xfff;
            cs_flags_export = 0;
            cs_add_obj_bb = false;
        }

    bool set(const char*);
    void set_def();
    char *string();

    EXTcnameType cname_type()               { return (cs_cnametype); }
    void set_cname_type(EXTcnameType t)     { cs_cnametype = t; }

    EXTlayerType layer_type()               { return (cs_layertype); }
    void set_layer_type(EXTlayerType t)     { cs_layertype = t; }

    EXTlabelType label_type()               { return (cs_labeltype); }
    void set_label_type(EXTlabelType t)     { cs_labeltype = t; }

    EXTlreadType lread_type()               { return (cs_lreadtype); }
    void set_lread_type(EXTlreadType t)     { cs_lreadtype = t; }

    unsigned int flags()                    { return (cs_flags); }
    void set_flags(unsigned int f)          { cs_flags = f; }
    void set_flag(unsigned int f)           { cs_flags |= f; }
    void unset_flag(unsigned int f)         { cs_flags &= ~f; }

    unsigned int flags_export()             { return (cs_flags_export); }
    void set_flags_export(unsigned int f)   { cs_flags_export = f; }
    void set_flag_export(unsigned int f)    { cs_flags_export |= f; }
    void unset_flag_export(unsigned int f)  { cs_flags_export &= ~f; }

    bool add_obj_bb()                       { return (cs_add_obj_bb); }
    void set_add_obj_bb(bool b)             { cs_add_obj_bb = b; }

private:
    EXTcnameType cs_cnametype;
    EXTlayerType cs_layertype;
    EXTlabelType cs_labeltype;
    EXTlreadType cs_lreadtype;
    unsigned int cs_flags;
    unsigned int cs_flags_export;
    bool cs_add_obj_bb;
};

// Defaults to use when assigning GDSII layers when there is no other
// resolution.
#define FIO_UNKNOWN_LAYER_BASE  128
#define FIO_UNKNOWN_DATATYPE    128

// Flags for conversion info printing.
//
#define FIO_INFO_FILENAME     0x1
#define FIO_INFO_FILETYPE     0x2
#define FIO_INFO_UNIT         0x4
#define FIO_INFO_ALIAS        0x8
#define FIO_INFO_RECCOUNTS    0x10
#define FIO_INFO_OBJCOUNTS    0x20
#define FIO_INFO_DEPTHCNTS    0x40
#define FIO_INFO_ESTSIZE      0x80
#define FIO_INFO_ESTCXSIZE    0x100
#define FIO_INFO_LAYERS       0x200
#define FIO_INFO_UNRESOLVED   0x400
#define FIO_INFO_TOPCELLS     0x800
#define FIO_INFO_ALLCELLS     0x1000
#define FIO_INFO_OFFSORT      0x2000
#define FIO_INFO_OFFSET       0x4000
#define FIO_INFO_INSTANCES    0x8000
#define FIO_INFO_BBS          0x10000
#define FIO_INFO_FLAGS        0x20000
#define FIO_INFO_INSTCNTS     0x40000
#define FIO_INFO_INSTCNTSP    0x80000

// Info retainment arg to cvin::parse etc.
enum cvINFO { cvINFOnone, cvINFOtotals, cvINFOpl, cvINFOpc, cvINFOplpc };

// cvINFOnone       no info retained
// cvINFOtotals     save count totals only
// cvINFOpl         save totals per layer
// cvINFOpc         save totals per cell
// cvINFOplpc       save totals per layer and cell


// Conversion parameters.
//
struct FIOcvPrms
{
    FIOcvPrms()
        {
            cv_scale = 1.0;
            cv_use_window = false;
            cv_clip = false;
            cv_flatten = false;
            cv_ecf_level = ECFnone;
        }

    double scale()              const { return (cv_scale); }
    const BBox *window()        const { return (&cv_window); }
    bool use_window()           const { return (cv_use_window); }
    bool flatten()              const { return (cv_flatten); }
    bool clip()                 const { return (cv_clip); }
    ECFlevel ecf_level()        const { return ((ECFlevel)cv_ecf_level); }

    void set_scale(double s)                { cv_scale = s; }
    void set_window(const BBox *BB)         { cv_window = *BB; }
    void set_window_left(int l)             { cv_window.left = l; }
    void set_window_bottom(int b)           { cv_window.bottom = b; }
    void set_window_right(int r)            { cv_window.right = r; }
    void set_window_top(int t)              { cv_window.top = t; }
    void set_use_window(bool b)             { cv_use_window = b; }
    void set_clip(bool b)                   { cv_clip = b; }
    void set_flatten(bool b)                { cv_flatten = b; }
    void set_ecf_level(ECFlevel f)          { cv_ecf_level = f; }

private:
    double cv_scale;            // scale factor
    BBox cv_window;             // area filter window
    bool cv_use_window;         // use area filter window
    bool cv_clip;               // clip objects to window
    bool cv_flatten;            // flatten hierarchy
    unsigned char cv_ecf_level; // empty cell filtering level
};


// Base of struct passed to OpenImport and returned in the callback.  A
// derived struct can be used to pass user info.
//
struct OIcbData
{
    OIcbData() { lib_filename = 0; }

    // If this is returned non-null, it is the name of a library file
    // that contains the name.
    //
    const char *lib_filename;
};


// Enum for cFIO::UseLayerList.
enum ULLtype { ULLnoList, ULLonlyList, ULLskipList };

// Enum for cFIO::InUseAlias, cFIO::OutUseAlias.
enum UAtype { UAnone, UAupdate, UAread, UAwrite };

// Enum for cFIO::OasWriteCompressed.
enum OAScompType { OAScompNone, OAScompSmart, OAScompForce };
// OAScompNone     No CBLOCKS produced.
// OAScompSmart    Produce CBLOCKS only when block is large enough
//                 to benefit.
// OAScompForce    Produce CBLOCKS for all cell contents and tables,
//                 less efficient but available for comparison purposes.

// Enum for cFIO::OasWriteChecksum.
enum OASchksumType { OASchksumNone, OASchksumCRC, OASchksumBsum };

// Enum for symref resolution functions.
enum RESOLVtype { RESOLVerror = -1, RESOLVnone = 0, RESOLVok = 1 };

// Creation type for CGD, for cFIO::NewCGD.
// CGDmemory    Geometry data in memory.
// CGDfile      Geometry data obtained from CGD file.
// CGDremote    Geometry data obtained from remote Xic server.
//
enum CgdType { CGDmemory, CGDfile, CGDremote };

// Flags for cFIO::OasWritePrptyMask.
#define OAS_PRPMSK_GDS_LBL 0x1
#define OAS_PRPMSK_XIC_LBL 0x2
#define OAS_PRPMSK_ALL     0x4

// Number of bounding box registers.
#define FIO_NUM_BB_STORE 8

inline class cFIO *FIO();

struct cif_in;

class cFIO : public FIOif
{
    static cFIO *ptr()
        {
            if (!instancePtr)
                on_null_ptr();
            return (instancePtr);
        }

    static void on_null_ptr();

public:
    friend inline cFIO *FIO() { return (cFIO::ptr()); }

    // fio.cc
    cFIO();
    virtual ~cFIO();
    void ClearAll();
    FileType GetFileType(FILE*);
    static FileType TypeExt(const char*);
    static const char *GetTypeExt(FileType);
    static const char *TypeName(FileType);
    static bool IsSupportedArchiveFormat(FileType);
    char *GzFileCopy(const char*, const char*);
    void ScalePrptyStrings(CDp*, double, double, DisplayMode);
    FILE *NetOpen(const char*, char**);

    // fio_alias.cc
    FIOaliasTab *NewReadingAlias(unsigned int);
    FIOaliasTab *NewTranslatingAlias(unsigned int);
    FIOaliasTab *NewWritingAlias(unsigned int, bool);
    bool FileWillChange(FileType, const FIOcvtPrms*);

    // fio_archive.cc
    bool ConvertToNative(const stringlist*, const FIOcvtPrms*);
    bool ConvertToCif(const stringlist*, const FIOcvtPrms*);
    bool ConvertToGds(const stringlist*, const FIOcvtPrms*);
    bool ConvertToCgx(const stringlist*, const FIOcvtPrms*);
    bool ConvertToOas(const stringlist*, const FIOcvtPrms*);
    bool ConvertToText(FileType, const char*, const char*, const char*);
    bool ConvertFromNative(const char*, const FIOcvtPrms*);
    bool ConvertFromCif(const char*, const FIOcvtPrms*, const char* = 0);
    bool ConvertFromGds(const char*, const FIOcvtPrms*, const char* = 0);
    bool ConvertFromCgx(const char*, const FIOcvtPrms*, const char* = 0);
    bool ConvertFromOas(const char*, const FIOcvtPrms*, const char* = 0);
    bool ConvertFromGdsText(const char*, const char*);
    OItype OpenImport(const char*, const FIOreadPrms*, const char* = 0,
        cCHD* = 0, CDcbin* = 0, char** = 0,
        void(*)(const char*, OIcbData*) = 0, OIcbData* = 0);
    char *DefaultFilename(CDcbin*, FileType);

    // fio_assemble.cc
    bool AssembleArchive(const char*);

    // fio_cgd.cc
    cCGD *NewCGD(const char*, const char*, CgdType, cCHD* = 0);

    // fio_cgx_read.cc
    bool IsCGX(FILE*);
    OItype DbFromCGX(const char*, const FIOreadPrms*, stringlist** = 0,
        stringlist** = 0);
    OItype FromCGX(const char*, const FIOcvtPrms*, const char* = 0);

    // fio_cgx_write.cc
    OItype ToCGX(const stringlist*, const FIOcvtPrms*);

    // fio_chd.cc
    cCHD *NewCHD(const char*, FileType, DisplayMode, FIOaliasTab*,
        cvINFO = cvINFOtotals);
    bool GetCHDinfo(const char*, FileType, const char*, int64_t = 0,
        int64_t = 0, int = -1, int = -1);

    // fio_cif_read.cc
    bool IsCIF(FILE*, CFtype* = 0, bool* = 0);
    bool HasPHYSICAL(FILE*);
    OItype DbFromCIF(const char*, const FIOreadPrms*, stringlist** = 0,
        stringlist** = 0);
    OItype FromCIF(const char*, const FIOcvtPrms*, const char* = 0);
    OItype FromNative(const char*, CDcbin*, double scale);
    OItype OpenNative(const char*, CDcbin*, double scale, bool* = 0);
    OItype TranslateDir(const char*, const FIOcvtPrms*);

    // fio_chd_split.cc
    OItype SplitArchive(const char*);

    // fio_chd_write.cc
    bool WriteCHDfile(const char*);

    // fio_cif_write.cc
    OItype ToCIF(const stringlist*, const FIOcvtPrms*);

    // fio_cvt_base.cc
    cv_out *NewOutput(const char*, const char*, FileType, bool=false);
    cv_in *NewInput(FileType, bool);
    char *GdsParamSet(double*, double*, int*, int*);

    // fio_gds_read.cc
    bool IsGDSII(FILE*);
    OItype DbFromGDSII(const char*, const FIOreadPrms*, stringlist** = 0,
        stringlist** = 0);
    OItype FromGDSII(const char*, const FIOcvtPrms*, const char* = 0);

    // fio_gds_write.cc
    OItype ToGDSII(const stringlist*, const FIOcvtPrms*);
    bool GdsFromText(const char*, const char*);

    // fio_layermap.cc
    CDll *GetGdsInputLayers(unsigned int, unsigned int, DisplayMode);
    const char *GetGdsInputLayer(unsigned int, unsigned int, DisplayMode);
    CDl *MapGdsLayer(unsigned int, unsigned int, DisplayMode, char*,
        bool, bool*);

    // fio_library.cc
    bool IsLibrary(FILE *fp);
    stringlist *ListLibraries(int);
    bool OpenLibrary(const char*, const char*);
    sLib *FindLibrary(const char*);
    const stringlist *GetLibraryProperties(const char*);
    void CloseLibrary(const char*, int);
    stringlist *GetLibNamelist(const char*, int);
    FILE *OpenLibFile(const char*, const char*, int, sLibRef**, sLib**);
    OItype OpenLibCell(const char*, const char*, int, CDcbin*);
    sLib *LookupLibCell(const char*, const char*, int, sLibRef**);
    RESOLVtype ResolveLibSymref(symref_t*, cCHD**, bool*);
    RESOLVtype ResolveUnseen(cCHD**, symref_t**);

    // fio_oas_read.cc
    bool IsOASIS(FILE*);
    OItype DbFromOASIS(const char*, const FIOreadPrms*, stringlist** = 0,
        stringlist** = 0);
    OItype FromOASIS(const char*, const FIOcvtPrms*, const char* = 0);
    Zlist *ZlistFromFile(const char*, XIrt*);

    // fio_oas_write.cc
    OItype ToOASIS(const stringlist*, const FIOcvtPrms*);
    bool ZlistToFile(Zlist*, const char*, const char*);

    // fio_paths.cc
    void PSetPath(const char*);
    const char *PGetPath();
    int PSetReadPath(const char*, int);
    FILE *POpen(const char*, const char*, char** = 0, int = 0,
        sLibRef** = 0, sLib** = 0);
    static char *PGetCWD(char*, size_t);
    static char *PAppendPath(char*, const char*, bool);
    static char *PPrependPath(char*, const char*, bool);
    static bool PInPath(const char*, const char*);
    static bool PRemovePath(char*, const char*);

    // fio_to_xic.cc
    OItype ToNative(const char*, const FIOcvtPrms*);
    bool WriteNative(CDcbin*, const char*);
    bool WriteFile(CDs*, const char*, FILE*);
    bool ListToNative(CDol*, const char*);
    bool TabToNative(SymTab*, const char*);

    CIFstyle &CifStyle()                { return (fioCIF_Style); }
    char *LastCifStyle() { return (fioCIF_LastReadStyle.string()); }

    bool IsMergeControlEnabled()        { return (fioMergeControlEnabled); }
    void IncMergeControl()              { fioMergeControlEnabled++; }
    void DecMergeControl()
        {
            if (fioMergeControlEnabled) {
                fioMergeControlEnabled--;
                if (!fioMergeControlEnabled)
                    ifMergeControlClear();
            }
        }

    bool IsSkipFixBB()                  { return (fioSkipFixBB != 0); }
    void SetSkipFixBB(bool b)
        {
            if (b)
                fioSkipFixBB++;
            else if (fioSkipFixBB)
                fioSkipFixBB--;
        }

    cvINFO CvtInfo()                    { return (fioCvtInfo); }
    void SetCvtInfo(cvINFO cv)          { fioCvtInfo = cv; }

    bool CvtUseWindow()                 { return (fioCvtPrms.use_window()); }
    bool CvtClip()                      { return (fioCvtPrms.clip()); }
    bool CvtFlatten()                   { return (fioCvtPrms.flatten()); }
    ECFlevel CvtECFlevel()              { return (fioCvtPrms.ecf_level()); }
    const BBox *CvtWindow()             { return (fioCvtPrms.window()); }

    void SetCvtUseWindow(bool b)        { fioCvtPrms.set_use_window(b); }
    void SetCvtClip(bool b)             { fioCvtPrms.set_clip(b); }
    void SetCvtFlatten(bool b)          { fioCvtPrms.set_flatten(b); }
    void SetCvtECFlevel(ECFlevel f)     { fioCvtPrms.set_ecf_level(f); }
    void SetCvtWindow(const BBox *BB)   { fioCvtPrms.set_window(BB); }
    void SetCvtWindowLeft(int l)        { fioCvtPrms.set_window_left(l); }
    void SetCvtWindowBottom(int b)      { fioCvtPrms.set_window_bottom(b); }
    void SetCvtWindowRight(int r)       { fioCvtPrms.set_window_right(r); }
    void SetCvtWindowTop(int t)         { fioCvtPrms.set_window_top(t); }

    bool OutUseWindow()                 { return (fioOutPrms.use_window()); }
    bool OutClip()                      { return (fioOutPrms.clip()); }
    bool OutFlatten()                   { return (fioOutPrms.flatten()); }
    ECFlevel OutECFlevel()              { return (fioOutPrms.ecf_level()); }
    const BBox *OutWindow()             { return (fioOutPrms.window()); }

    void SetOutUseWindow(bool b)        { fioOutPrms.set_use_window(b); }
    void SetOutClip(bool b)             { fioOutPrms.set_clip(b); }
    void SetOutFlatten(bool b)          { fioOutPrms.set_flatten(b); }
    void SetOutECFlevel(ECFlevel f)     { fioOutPrms.set_ecf_level(f); }
    void SetOutWindow(const BBox *BB)   { fioOutPrms.set_window(BB); }
    void SetOutWindowLeft(int l)        { fioOutPrms.set_window_left(l); }
    void SetOutWindowBottom(int b)      { fioOutPrms.set_window_bottom(b); }
    void SetOutWindowRight(int r)       { fioOutPrms.set_window_right(r); }
    void SetOutWindowTop(int t)         { fioOutPrms.set_window_top(t); }

    const FIOreadPrms *DefReadPrms()    { return (&fioDefReadPrms); }

    double ReadScale()                  { return (fioCvtScaleRead); }
    void SetReadScale(double d)         { fioCvtScaleRead = d; }
    double WriteScale()                 { return (fioOutPrms.scale()); }
    void SetWriteScale(double d)        { fioOutPrms.set_scale(d); }
    double TransScale()                 { return (fioCvtPrms.scale()); }
    void SetTransScale(double d)        { fioCvtPrms.set_scale(d); }

    BBox *savedBB(int i)
        {
            if (i >= 0 && i < FIO_NUM_BB_STORE)
                return (fioStoreBB + i);
            return (0);
        }

    // Flags for various mode alternatives and states associated with
    // file i/o.

    bool IsAllowPrptyStrip()            { return (fioAllowPrptyStrip); }
    void SetAllowPrptyStrip(bool b)     { fioAllowPrptyStrip = b; }

    bool IsCgdSkipInvisibleLayers()     { return (fioCgdSkipInvisibleLayers); }
    void SetCgdSkipInvisibleLayers(bool b) { fioCgdSkipInvisibleLayers = b; }

    bool IsFlatReadFeedback()           { return (fioFlatReadFeedback); }
    void SetFlatReadFeedback(bool b)    { fioFlatReadFeedback = b; }

    bool IsReadingLibrary()             { return (fioReadingLibrary); }
    void SetReadingLibrary(bool b)      { fioReadingLibrary = b; }

    // Symbol Path

    bool IsNoReadExclusive()            { return (fioNoReadExclusive); }
    void SetNoReadExclusive(bool b)     { fioNoReadExclusive = b; }

    bool IsAddToBack()                  { return (fioAddToBack); }
    void SetAddToBack(bool b)           { fioAddToBack = b; }

    // PCells

    bool IsKeepPCellSubMasters()        { return (fioKeepPCellSubMasters); }
    void SetKeepPCellSubMasters(bool b) { fioKeepPCellSubMasters = b; }
    bool IsListPCellSubMasters()        { return (fioListPCellSubMasters); }
    void SetListPCellSubMasters(bool b) { fioListPCellSubMasters = b; }
    bool IsShowAllPCellWarnings()       { return (fioShowAllPCellWarnings); }
    void SetShowAllPCellWarnings(bool b){ fioShowAllPCellWarnings = b; }

    // Standard Vias

    bool IsKeepViaSubMasters()          { return (fioKeepViaSubMasters); }
    void SetKeepViaSubMasters(bool b)   { fioKeepViaSubMasters = b; }
    bool IsListViaSubMasters()          { return (fioListViaSubMasters); }
    void SetListViaSubMasters(bool b)   { fioListViaSubMasters = b; }

    // Conversion - General

    bool IsChdFailOnUnresolved()        { return (fioChdFailOnUnresolved); }
    void SetChdFailOnUnresolved(bool b) { fioChdFailOnUnresolved = b; }

    bool IsMultiLayerMapOk()            { return (fioMultiLayerMapOk); }
    void SetMultiLayerMapOk(bool b)     { fioMultiLayerMapOk = b; }

    int UnknownGdsLayerBase()           { return (fioUnknownGdsLayerBase); }
    void SetUnknownGdsLayerBase(int i)  { fioUnknownGdsLayerBase = i; }

    int UnknownGdsDatatype()            { return (fioUnknownGdsDatatype); }
    void SetUnknownGdsDatatype(int i)   { fioUnknownGdsDatatype = i; }

    bool IsNoStrictCellnames()          { return (fioNoStrictCellnames); }
    void SetNoStrictCellnames(bool b)   { fioNoStrictCellnames = b; }

    // Conversion - Import and Conversion Commands

    unsigned int ChdRandomGzip()        { return (fioChdRandomGzip); }
    void SetChdRandomGzip(unsigned int n) { fioChdRandomGzip = n; }

    bool IsAutoRename()                 { return (fioAutoRename); }
    void SetAutoRename(bool b)          { fioAutoRename = b; }

    bool IsNoCreateLayer()              { return (fioNoCreateLayer); }
    void SetNoCreateLayer(bool b)       { fioNoCreateLayer = b; }

    bool IsNoOverwritePhys()            { return (fioNoOverwritePhys); }
    void SetNoOverwritePhys(bool b)     { fioNoOverwritePhys = b; }

    bool IsNoOverwriteElec()            { return (fioNoOverwriteElec); }
    void SetNoOverwriteElec(bool b)     { fioNoOverwriteElec = b; }

    bool IsNoOverwriteLibCells()        { return (fioNoOverwriteLibCells); }
    void SetNoOverwriteLibCells(bool b) { fioNoOverwriteLibCells = b; }

    bool IsNoCheckEmpties()             { return (fioNoCheckEmpties); }
    void SetNoCheckEmpties(bool b)      { fioNoCheckEmpties = b; }

    bool IsNoReadLabels()               { return (fioNoReadLabels); }
    void SetNoReadLabels(bool b)        { fioNoReadLabels = b; }

    bool IsMergeInput()                 { return (fioMergeInput); }
    void SetMergeInput(bool b)          { fioMergeInput = b; }

    bool IsEvalOaPCells()               { return (fioEvalOaPCells); }
    void SetEvalOaPCells(bool b)        { fioEvalOaPCells = b; }

    bool IsNoEvalNativePCells()         { return (fioNoEvalNativePCells); }
    void SetNoEvalNativePCells(bool b)  { fioNoEvalNativePCells = b; }

    const char *LayerList()             { return (fioLayerList); }
    void SetLayerList(const char *ll)
        {
            char *s = lstring::copy(ll);
            delete [] fioLayerList;
            fioLayerList = s;
        }

    ULLtype UseLayerList()              { return ((ULLtype)fioUseLayerList); }
    void SetUseLayerList(ULLtype t)     { fioUseLayerList = t; }

    const char *LayerAlias()            { return (fioLayerAlias); }
    void SetLayerAlias(const char *la)
        {
            char *s = lstring::copy(la);
            delete [] fioLayerAlias;
            fioLayerAlias = s;
        }

    bool IsUseLayerAlias()              { return (fioUseLayerAlias); }
    void SetUseLayerAlias(bool b)       { fioUseLayerAlias = b; }

    bool IsInToLower()                  { return (fioInToLower); }
    void SetInToLower(bool b)           { fioInToLower = b; }

    bool IsInToUpper()                  { return (fioInToUpper); }
    void SetInToUpper(bool b)           { fioInToUpper = b; }

    UAtype InUseAlias()                 { return ((UAtype)fioInUseAlias); }
    void SetInUseAlias(UAtype t)        { fioInUseAlias = t; }

    const char *InCellNamePrefix()      { return (fioInCellNamePrefix); }
    void SetInCellNamePrefix(const char *p)
        {
            char *t = lstring::copy(p);
            delete [] fioInCellNamePrefix;
            fioInCellNamePrefix = t;
        }

    const char *InCellNameSuffix()      { return (fioInCellNameSuffix); }
    void SetInCellNameSuffix(const char *s)
        {
            char *t = lstring::copy(s);
            delete [] fioInCellNameSuffix;
            fioInCellNameSuffix = t;
        }

    bool IsNoMapDatatypes()             { return (fioNoMapDatatypes); }
    void SetNoMapDatatypes(bool b)      { fioNoMapDatatypes = b; }

    bool IsOasReadNoChecksum()          { return (fioOasReadNoChecksum); }
    void SetOasReadNoChecksum(bool b)   { fioOasReadNoChecksum = b; }

    bool IsOasPrintNoWrap()             { return (fioOasPrintNoWrap); }
    void SetOasPrintNoWrap(bool b)      { fioOasPrintNoWrap = b; }

    bool IsOasPrintOffset()             { return (fioOasPrintOffset); }
    void SetOasPrintOffset(bool b)      { fioOasPrintOffset = b; }

    // Conversion - Export Commands

    bool IsStripForExport()             { return (fioStripForExport); }
    void SetStripForExport(bool b)      { fioStripForExport = b; }

    bool IsWriteAllCells()              { return (fioWriteAllCells); }
    void SetWriteAllCells(bool b)       { fioWriteAllCells = b; }

    bool IsSkipInvisiblePhys()          { return (fioSkipInvisiblePhys); }
    void SetSkipInvisiblePhys(bool b)   { fioSkipInvisiblePhys = b; }

    bool IsSkipInvisibleElec()          { return (fioSkipInvisibleElec); }
    void SetSkipInvisibleElec(bool b)   { fioSkipInvisibleElec = b; }

    bool IsKeepBadArchive()             { return (fioKeepBadArchive); }
    void SetKeepBadArchive(bool b)      { fioKeepBadArchive = b; }

    bool IsNoCompressContext()          { return (fioNoCompressContext); }
    void SetNoCompressContext(bool b)   { fioNoCompressContext = b; }

    bool IsRefCellAutoRename()          { return (fioRefCellAutoRename); }
    void SetRefCellAutoRename(bool b)   { fioRefCellAutoRename = b; }

    bool IsUseCellTab()                 { return (fioUseCellTab); }
    void SetUseCellTab(bool b)          { fioUseCellTab = b; }

    bool IsSkipOverrideCells()          { return (fioSkipOverrideCells); }
    void SetSkipOverrideCells(bool b)   { fioSkipOverrideCells = b; }

    bool IsOutToLower()                 { return (fioOutToLower); }
    void SetOutToLower(bool b)          { fioOutToLower = b; }

    bool IsOutToUpper()                 { return (fioOutToUpper); }
    void SetOutToUpper(bool b)          { fioOutToUpper = b; }

    UAtype OutUseAlias()                { return ((UAtype)fioOutUseAlias); }
    void SetOutUseAlias(UAtype t)       { fioOutUseAlias = t; }

    const char *OutCellNamePrefix()     { return (fioOutCellNamePrefix); }
    void SetOutCellNamePrefix(const char *p)
        {
            char *t = lstring::copy(p);
            delete [] fioOutCellNamePrefix;
            fioOutCellNamePrefix = t;
        }

    const char *OutCellNameSuffix()     { return (fioOutCellNameSuffix); }
    void SetOutCellNameSuffix(const char *s)
        {
            char *t = lstring::copy(s);
            delete [] fioOutCellNameSuffix;
            fioOutCellNameSuffix = t;
        }

    int GdsOutLevel()                   { return (fioGdsOutLevel); }
    void SetGdsOutLevel(int l)          { fioGdsOutLevel = l; }

    bool GdsTruncateLongStrings()       { return (fioGdsTruncateLongStrings); }
    void SetGdsTruncateLongStrings(bool b) { fioGdsTruncateLongStrings = b; }

    double GdsMunit()                   { return (fioGdsMunit); }
    void SetGdsMunit(double u)          { fioGdsMunit = u; }

    bool IsNoGdsMapOk()                 { return (fioNoGdsMapOk); }
    void SetNoGdsMapOk(bool b)          { fioNoGdsMapOk = b; }

    OAScompType OasWriteCompressed()    { return ((OAScompType)
                                          fioOasWriteCompressed); }
    void SetOasWriteCompressed(OAScompType t) { fioOasWriteCompressed = t; }

    bool IsOasWriteNameTab()            { return (fioOasWriteNameTab); }
    void SetOasWriteNameTab(bool b)     { fioOasWriteNameTab = b; }

    const char *OasWriteRep()           { return (fioOasWriteRep); }
    void SetOasWriteRep(const char *r)
        {
            char *s = lstring::copy(r);
            delete [] fioOasWriteRep;
            fioOasWriteRep = s;
        }

    OASchksumType OasWriteChecksum()    { return ((OASchksumType)
                                          fioOasWriteChecksum); }
    void SetOasWriteChecksum(OASchksumType t) { fioOasWriteChecksum = t; }

    bool IsOasWriteNoTrapezoids()       { return (fioOasWriteNoTrapezoids); }
    void SetOasWriteNoTrapezoids(bool b) { fioOasWriteNoTrapezoids = b; }

    bool IsOasWriteWireToBox()          { return (fioOasWriteWireToBox); }
    void SetOasWriteWireToBox(bool b)   { fioOasWriteWireToBox = b; }

    bool IsOasWriteRndWireToPoly()      { return (fioOasWriteRndWireToPoly); }
    void SetOasWriteRndWireToPoly(bool b) { fioOasWriteRndWireToPoly = b; }

    bool IsOasWriteNoGCDcheck()         { return (fioOasWriteNoGCDcheck); }
    void SetOasWriteNoGCDcheck(bool b)  { fioOasWriteNoGCDcheck = b; }

    bool IsOasWriteUseFastSort()        { return (fioOasWriteUseFastSort); }
    void SetOasWriteUseFastSort(bool b) { fioOasWriteUseFastSort = b; }

    int OasWritePrptyMask()             { return (fioOasWritePrptyMask); }
    void SetOasWritePrptyMask(int m)    { fioOasWritePrptyMask = m; }

private:
    // fio_archive.cc
    OItype open_symbol_file(const char*, const char*,
        void(*)(const char*, OIcbData*), OIcbData*, FILE**, char**,
        sLibRef**, sLib**);
    OItype open_cif(const char*, const char*, cCHD*, const FIOreadPrms*,
        sLibRef*, sLib*, void(*)(const char*, OIcbData*), OIcbData*, CDcbin*);
    OItype open_gds(const char*, const char*, cCHD*, const FIOreadPrms*,
        sLibRef*, sLib*, void(*)(const char*, OIcbData*), OIcbData*, CDcbin*);
    OItype open_cgx(const char*, const char*, cCHD*, const FIOreadPrms*,
        sLibRef*, sLib*, void(*)(const char*, OIcbData*), OIcbData*, CDcbin*);
    OItype open_oas(const char*, const char*, cCHD*, const FIOreadPrms*,
        sLibRef*, sLib*, void(*)(const char*, OIcbData*), OIcbData*, CDcbin*);
    void find_top_symbol(CDcbin*, void(*)(const char*, OIcbData*), OIcbData*,
        stringlist*, stringlist*, const char*, bool*);

    CIFstyle fioCIF_Style;          // CIF dialect params
    CIFstyle fioCIF_LastReadStyle;  // Style set when reading CIF

    const char *fioSymSearchPath;   // Cell file locations
    sLib *fioLibraries;             // Open library list

    FIOcvPrms fioCvtPrms;           // Parameters for output from database
    FIOcvPrms fioOutPrms;           // Parameters for couput from conversion
    FIOreadPrms fioDefReadPrms;     // Default reading params

    cvINFO fioCvtInfo;              // Info level for CHD create

    double fioCvtScaleRead;         // Scale when reading
    int fioMergeControlEnabled;     // Merge Control pop-up enable count
    int fioSkipFixBB;               // Skip calls to fixBB after reading

    BBox fioStoreBB[FIO_NUM_BB_STORE]; // Saved rectangles, general use.

    double fioGdsMunit;
        // GDSII M-unit scale factor.

    SymTab *fioNativeImportTab;
        // If a native cell has as an instance name a path to an
        // archive, the archive is opened and the top cell is
        // referenced, when reading into the main database.  This
        // table maps top cell names to archive file paths, for all
        // such references.  When the cell is saved as a single native
        // cell file, this table is used to map the instance name back
        // into the archive path.

    SymTab *fioSubMasterTab;
        // This is used when reading a hierarchy of native cells,
        // providing persistence of this table across cv_in
        // construction/destruction.

    char *fioLayerList;
        // List of layers for layer filtering.

    char *fioLayerAlias;
        // List of name=newname specifications for layer aliasing.

    char *fioInCellNamePrefix;
        // Cell name modification prefix for input.

    char *fioInCellNameSuffix;
        // Cell name modification suffix for input.

    char *fioOutCellNamePrefix;
        // Cell name modification prefix for output.

    char *fioOutCellNameSuffix;
        // Cell name modification suffix for output.

    char *fioOasWriteRep;
        // OASIS repetition finder specification string.

    bool fioUseSubMasterTab;
        // Indicates to use fioSubMasterTab in OpenNative.

    bool fioAllowPrptyStrip;
        // Allow stripping properties when writing archive output in
        // physical mode.  Presently, applies only to OASIS.

    bool fioCgdSkipInvisibleLayers;
        // When reading layer data from a CGD, skip layers that exist
        // in the layer table and don't have CDL_VISIBLE set.  This is
        // set during image creation for performance improvement.

    bool fioFlatReadFeedback;
        // In cCHD::readFlat, report cells processed to message
        // interface.

    bool fioReadingLibrary;
        // When set, new cells read from a file will have the user
        // library flag set.

    // Symbol Path

    bool fioNoReadExclusive;
        // Don't change search path while reading native cells.

    bool fioAddToBack;
        // Add present directory to back of search path when reading
        // native cells.

    // PCells

    bool fioKeepPCellSubMasters;
        // When writing output, include pcell sub-masters.

    bool fioListPCellSubMasters;
        // When listing cells, include pcell sub-masters.

    bool fioShowAllPCellWarnings;
        // When evaluating the pcell script, some checks like for
        // coincident objects are turned off since they are the pcell
        // author's problem and are annoying.  This turns those
        // warnings back on.

    // Standard Vias

    bool fioKeepViaSubMasters;
        // When writing output, include standard via sub-masters.

    bool fioListViaSubMasters;
        // When listing cells, include standard via sub-masters.

    // Conversion - General

    bool fioChdFailOnUnresolved;
        // Terminate CHD operation on unresolved symref.

    bool fioMultiLayerMapOk;
        // Allow GDII/OASIS input layer mapping to more than one CD
        // layers (i.e., multiple database objects created from single
        // input file object.)

    int fioUnknownGdsLayerBase;
        // Default first GDSII layer assigned

    int fioUnknownGdsDatatype;
        // Default GDSII datatype assigned

    bool fioNoStrictCellnames;
        // When set, allow white space (ascii values ' ' and lower) in
        // cell names.

    // Conversion - Import and Conversion Commands

    unsigned char fioChdRandomGzip;
        // If nonzero, create a map for random access of gzipped files
        // when accessing with CHD.  The value is the number of Mb per
        // access point.

    bool fioAutoRename;
        // Turn on automatic cell renaming when reading input and cell
        // names clash.

    bool fioNoCreateLayer;
        // When set, do not create new layers for unresolved layers
        // when reading input, abort the read instead.  The default
        // (variable not set) is to create new layers as needed.

    bool fioNoOverwritePhys;
        // When unset, and when opening a cell from a file that
        // has the same name as a cell in memory, the physical part
        // of the memory cell will be overwritten.  If set, the
        // physical part of the cell from the file is ignored.

    bool fioNoOverwriteElec;
        // When unset, and when opening a cell from a file that
        // has the same name as a cell in memory, the electrical
        // part of the memory cell will be overwritten.  If set,
        // the electrical part of the cell from the file is ignored.

    bool fioNoOverwriteLibCells;
        // This will prevent cells in input from overwriting existing
        // cells opened from a library.  The default action is that
        // when overwriting is enabled, new cells will always
        // overwrite existing cells.

    bool fioNoCheckEmpties;
        // Skip empty symbol test when reading input.

    bool fioNoReadLabels;
        // Ignore text records when reading physical data.

    bool fioMergeInput;
        // Flag to indicate that objects should be merged with
        // neighbors, if possible, as cell is read.  Presently,
        // only boxes are merged.

    bool fioEvalOaPCells;
        // When an OpenAccess pcell placement is found when reading
        // input, attempt to build the sub-master from OA.  The OA
        // library must be open.  If not set, skip this, and assume
        // that the sub-master cell will be provided in the archive or
        // from another source.

    bool fioNoEvalNativePCells;
        // When a native pcell placement is found when reading input,
        // the default action is to hunt for the super-master and
        // build a sub-master.  If this is set, skip this and assume
        // that the sub-master will be provided.

    unsigned char fioUseLayerList;
        // Turn on layer filtering.

    bool fioUseLayerAlias;
        // Turn on layer aliasing.

    bool fioInToLower;
        // Map upper case cell names to lower case when reading.

    bool fioInToUpper;
        // Map lower case cell names to upper case when reading.

    unsigned char fioInUseAlias;
        // Use/create a table of cell name aliases when reading input.

    bool fioNoMapDatatypes;
        // If set, do not create new layers for varying datatypes
        // when reading GDSII.

    bool fioOasReadNoChecksum;
        // Ignore the checksum when reading OASIS.

    bool fioOasPrintNoWrap;
        // Don't wrap lines when writing OASIS ASCII text output.

    bool fioOasPrintOffset;
        // Include record offsets when writing OASIS ASCII text.

    // Conversion - Export Commands

    bool fioStripForExport;
        // Flag to indicate that no format extensions are to be used
        // when writing, including restriction to physical data only.

    bool fioWriteAllCells;
        // When set, user library cells are written to archive output
        // when writing from the main database.  Normally, library
        // cells are not included.

    bool fioSkipInvisiblePhys;
    bool fioSkipInvisibleElec;
        // When set and writing archive from main database, skip
        // invisible layers.

    bool fioKeepBadArchive;
        // On error when writing an archive file, don't remove the
        // bad file.

    bool fioNoCompressContext;
        // Don't use compressed instance lists in archive context.

    bool fioRefCellAutoRename;
        // Rename cells under reference cells when writing a hierarchy
        // if the cell name has already been written.  If not set,
        // such cells are skipped.

    bool fioUseCellTab;
        // When reading through a CHD for translation, cell names found
        // in the cCD::cdAuxCellTab will be streamed out from the main
        // database, superseding the CHD.

    bool fioSkipOverrideCells;
        // If fioUseCellTab is set, cells found in the table will
        // not appear in output.

    bool fioOutToLower;
        // Convert upper case cell names to lower case when writing
        // output.

    bool fioOutToUpper;
        // Convert lower case cell names to upper case when writing
        // output.

    unsigned char fioOutUseAlias;
        // Use/create a table of cell name aliases when writing
        // output.

    unsigned char fioGdsOutLevel;
        // GDSII output format version (0-2).

    bool fioGdsTruncateLongStrings;
        // Allow but truncate ASCII strings too long for GDSII record.

    bool fioNoGdsMapOk;
        // If set, ignore layers in GDS output with no GDS mapping.
        // Otherwise this is a fatal error.

    unsigned char fioOasWriteCompressed;
        // Use CBLOCKs in OASIS output.

    bool fioOasWriteNameTab;
        // Use strict-mode string tables in OASIS output.

    unsigned char fioOasWriteChecksum;
        // Write a CRC or checksum in OASIS output.

    bool fioOasWriteNoTrapezoids;
        // Don't use trapezoid records in OASIS output.

    bool fioOasWriteWireToBox;
        // Use rectangle records rather than two-vertex wires when
        // possible in OASIS output.

    bool fioOasWriteRndWireToPoly;
        // Write rounded-end wires as polygons in OASIS output.

    bool fioOasWriteNoGCDcheck;
        // Don't use GCD checking when writing OASIS output, faster
        // but files may be larger.

    bool fioOasWriteUseFastSort;
        // Use original modal sorting algorithm, faster but not as
        // compact.

    unsigned char fioOasWritePrptyMask;
        // Omit certain or all properties from OASIS output.

    static cFIO *instancePtr;
};

// This struct is instantiated on the stack in functions under which
// the MergeControl pop up is used.  This takes care of enabling the
// pop-up while the function is in scope, and popping down and/or
// clearing the memory used by this pop-up when out of scope.
//
struct sMCenable
{
    sMCenable() { FIO()->IncMergeControl(); }
    ~sMCenable() { FIO()->DecMergeControl(); }
};


// Strip Microsoft line termination, maybe the newline also.
//
inline void
NTstrip(char *buf, bool nltoo = false)
{
    char *s = strrchr(buf, '\n');
    if (s) {
        if (s > buf && *(s-1) == '\r') {
            if (nltoo)
                *(s-1) = 0;
            else {
                *(s-1) = '\n';
                *s = 0;
            }
        }
        else if (nltoo)
            *s = 0;
    }
}


// fio_zio.cc
extern FILE *large_fopen(const char*, const char*);
extern int64_t large_fseek(FILE*, int64_t, int);
extern int64_t large_ftell(FILE*);

enum GZIPmode { GZIPcompress, GZIPuncompress, GZIPfilecopy };
extern char *gzip(const char*, const char*, GZIPmode);

#endif

