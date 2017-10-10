
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

#include "config.h"
#include "fio.h"
#include "fio_library.h"
#include "fio_alias.h"
#include "cd_chkintr.h"
#include "miscutil/pathlist.h"
#include "miscutil/filestat.h"
#include "miscutil/proxy.h"
#ifdef HAVE_MOZY
#include "httpget/transact.h"
#endif
#include <ctype.h>


cFIO *cFIO::instancePtr = 0;

cFIO::cFIO()
{
    if (instancePtr) {
        fprintf(stderr, "Singleton class cFIO already instantiated.\n");
        exit(1);
    }
    instancePtr = this;

    fioSymSearchPath = 0;
    fioLibraries = 0;

    fioCvtInfo = cvINFOtotals;

    fioCvtScaleRead = 1.0;
    fioMergeControlEnabled = 0;
    fioSkipFixBB = 0;

    fioGdsMunit = 1.0;
    fioNativeImportTab = 0;
    fioSubMasterTab = 0;

    fioLayerList = 0;
    fioLayerAlias = 0;
    fioInCellNamePrefix = 0;
    fioInCellNameSuffix = 0;
    fioOutCellNamePrefix = 0;
    fioOutCellNameSuffix = 0;
    fioOasWriteRep = 0;

    fioUseSubMasterTab = false;
    fioAllowPrptyStrip = false;
    fioCgdSkipInvisibleLayers = false;
    fioFlatReadFeedback = false;
    fioReadingLibrary = false;

    fioNoReadExclusive = false;
    fioAddToBack = false;

    fioKeepPCellSubMasters = false;
    fioListPCellSubMasters = false;
    fioShowAllPCellWarnings = false;

    fioKeepViaSubMasters = false;
    fioListViaSubMasters = false;

    fioChdFailOnUnresolved = false;
    fioMultiLayerMapOk = false;
    fioUnknownGdsLayerBase = FIO_UNKNOWN_LAYER_BASE;
    fioUnknownGdsDatatype = FIO_UNKNOWN_DATATYPE;
    fioNoStrictCellnames = false;

    fioChdRandomGzip = false;
    fioAutoRename = false;
    fioNoCreateLayer = false;
    fioNoOverwritePhys = false;
    fioNoOverwriteElec = false;
    fioNoOverwriteLibCells = false;
    fioNoCheckEmpties = false;
    fioNoReadLabels = false;
    fioMergeInput = false;
    fioEvalOaPCells = false;
    fioNoEvalNativePCells = false;
    fioUseLayerList = ULLnoList;
    fioUseLayerAlias = false;
    fioInToLower = false;
    fioInToUpper = false;
    fioInUseAlias = UAnone;
    fioNoMapDatatypes = false;
    fioOasReadNoChecksum = false;
    fioOasPrintNoWrap = false;
    fioOasPrintOffset = false;

    fioStripForExport = false;
    fioWriteAllCells = false;
    fioSkipInvisiblePhys = false;
    fioSkipInvisibleElec = false;
    fioKeepBadArchive = false;
    fioNoCompressContext = false;
    fioRefCellAutoRename = false;
    fioUseCellTab = false;
    fioSkipOverrideCells = false;
    fioOutToLower = false;
    fioOutToUpper = false;
    fioOutUseAlias = UAnone;
    fioGdsOutLevel = 0;
    fioGdsTruncateLongStrings = false;
    fioNoGdsMapOk = false;
    fioOasWriteCompressed = OAScompNone;
    fioOasWriteNameTab = false;
    fioOasWriteChecksum = OASchksumNone;
    fioOasWriteNoTrapezoids = false;
    fioOasWriteWireToBox = false;
    fioOasWriteRndWireToPoly = false;
    fioOasWriteNoGCDcheck = false;
    fioOasWriteUseFastSort = false;
    fioOasWritePrptyMask = 0;

    // Default parameters when reading into the database.  Unit scale,
    // no layer mapping, but allow the cellname alias modes given.
    //
    fioDefReadPrms.set_alias_mask(CVAL_AUTO_NAME);

    // initialize search path
    const char *c = PGetPath();
    if (!c || !*c)
        PSetPath(".");
}


cFIO::~cFIO()
{
}


void
cFIO::ClearAll()
{
    CloseLibrary(0, LIBdevice | LIBuser);
}


// Private static error exit.
//
void
cFIO::on_null_ptr()
{
    fprintf(stderr, "Singleton class cFIO used before instantiated.\n");
    exit(1);
}


// Return the type of file accessed by fp.  We look for fully supported
// archive formats and native only.
//
FileType
cFIO::GetFileType(FILE *fp)
{
    CFtype ct;
    bool issced;
    FileType ft = Fnone;
    if (IsGDSII(fp))  // must be first!
        ft = Fgds;
    else if (IsCGX(fp))
        ft = Fcgx;
    else if (IsOASIS(fp))
        ft = Foas;
    else if (IsCIF(fp, &ct, &issced))
        ft = (ct == CFnative ? Fnative : Fcif);
    return (ft);
}


// Static function.
// Look at the file extension(s), if any, and return the file type implied
// by the extensions.  If a ".gz" extension is present, look at the
// prior extension.  If there is no extension or the extension isn't
// recognized, return Fnone.
//
FileType
cFIO::TypeExt(const char *pathname)
{
    if (!pathname)
        return (Fnone);
    char *fname = lstring::copy(lstring::strip_path(pathname));
    char *s = strrchr(fname, '.');
    if (!s) {
        delete [] fname;
        return (Fnone);
    }
    if (lstring::cieq(s, ".gz")) {
        *s = 0;
        s = strrchr(fname, '.');
    }
    FileType type = Fnone;
    if (s) {
        if (lstring::cieq(s, ".xic"))
            type = Fnative;
        else if (lstring::cieq(s, ".gds") || lstring::cieq(s, ".str") ||
                lstring::cieq(s, ".strm") || lstring::cieq(s, ".stream"))
            type = Fgds;
        else if (lstring::cieq(s, ".cgx"))
            type = Fcgx;
        else if (lstring::cieq(s, ".oas") || lstring::cieq(s, ".oasis"))
            type = Foas;
        else if (lstring::cieq(s, ".cif"))
            type = Fcif;
    }
    delete [] fname;
    return (type);
}


// Static function.
// Return the favored extension for the file type.
//
const char *
cFIO::GetTypeExt(FileType ft)
{
    switch (ft) {
    case Fnone:
        return (0);
    case Fnative:
        return (".xic");
    case Fgds:
        return (".gds");
    case Fcgx:
        return (".cgx");
    case Foas:
        return (".oas");
    case Fcif:
        return (".cif");
    default:
        return (0);
    }
}


// Static function.
// Return the text name corresonding to the file type.
//
const char *
cFIO::TypeName(FileType type)
{
    switch (type) {
    default:
    case Fnone:
        return ("non-archive");
    case Fnative:
        return ("Xic");
    case Fgds:
        return ("GDSII");
    case Fcgx:
        return ("CGX");
    case Foas:
        return ("OASIS");
    case Fcif:
        return ("CIF");
    case Foa:
        return ("OpenAccess");
    }
}


// Static function.
// Return true if ft is a fully supported archive format.
//
bool
cFIO::IsSupportedArchiveFormat(FileType ft)
{
    switch (ft) {
    case Fgds:
    case Fcgx:
    case Foas:
    case Fcif:
        return (true);
    default:
        break;
    }
    return (false);
}


// Copy the file from fsrc to fdst, applying compression or
// uncompressing depending on the ".gz" extensions.  Return an error
// message if error, 0 otherwise.
//
char *
cFIO::GzFileCopy(const char *fsrc, const char *fdst)
{
    const char *s = strrchr(fsrc, '.');
    bool in_gz = s && !strcasecmp(s, ".gz");
    s = strrchr(fdst, '.');
    bool out_gz = s && !strcasecmp(s, ".gz");
    char *err;
    if (in_gz && !out_gz)
        err = gzip(fsrc, fdst, GZIPuncompress);
    else if (!in_gz && out_gz)
        err = gzip(fsrc, fdst, GZIPcompress);
    else {
        err = gzip(fsrc, fdst, GZIPfilecopy);
    }
    return (err);
}


// If the cell scaling is being changed as the cell is being read,
// we have to change the scale of coordinates in the properties, too.
// The next two functions accomplish this.
// - Only CDs and CDc properties may require scaling
// - The GDSII_PROPERTY_BASE+xxx properties do not require scaling
//
// This is called by the converters.  The properties are all CDp type,
// i.e., the string is significant.  This is NOT the format used in
// the objects.
//
void
cFIO::ScalePrptyStrings(CDp *list, double phys_scale, double elec_scale,
    DisplayMode mode)
{
    for (CDp *pd = list; pd; pd = pd->next_prp())
        pd->scale(elec_scale, phys_scale, mode);
}


#ifdef HAVE_MOZY

namespace {
    // This is called periodically during http/ftp transfers.  Print
    // the message on the prompt line and check for interrupts.  If
    // interrupted, abort download
    //
    bool http_puts(void*, const char *s)
    {
        if (!s)
            return (false);
        while (isspace(*s))
            s++;
        if (!*s)
            return (false);
        // strip trailing white space (newline)
        char *ss = lstring::copy(s);
        char *e = ss + strlen(ss) - 1;
        while (isspace(*e))
            *e-- = 0;
        CD()->ifInfoMessage(IFMSG_INFO, "%s", ss);  // don't elide '%'
        delete [] ss;
        return (checkInterrupt("Interrupt received, abort transfer? "));
    }
}

#endif


// Callback to download and open a file given as a URL.
//
FILE *
cFIO::NetOpen(const char *url, char **filename)
{
#ifdef HAVE_MOZY
// Mozy provides internet file access.

    Transaction t;
    char *u = lstring::gettok(&url);
    t.set_url(u);
    delete [] u;

    // Set proxy.
    char *pxy = proxy::get_proxy();
    t.set_proxy(pxy);
    delete [] pxy;

    // The url can contain this trailing option.
    while (*url) {
        char *tok = lstring::gettok(&url);
        if (!strcmp(tok, "-o")) {
            delete [] tok;
            tok = lstring::getqtok(&url);
            t.set_destination(tok);
            delete [] tok;
        }
    }
    t.set_puts(http_puts);
    bool tmp_file = false;
    if (!t.destination()) {
        if (!filename || !*filename || !**filename) {
            char *tf = filestat::make_temp("ft");
            t.set_destination(tf);
            delete [] tf;
            tmp_file = true;
        }
        else
            t.set_destination(*filename);
    }
    if (t.transact() == 0) {
        FILE *fp = large_fopen(t.destination(), "r");
        if (tmp_file)
            filestat::queue_deletion(t.destination());
        if (filename && (!*filename || !**filename))
            *filename = lstring::copy(t.destination());
        return (fp);
    }
    else {
        if (tmp_file)
            filestat::queue_deletion(t.destination());
    }
#else
    (void)url;
    (void)filename;
#endif
    return (0);
}

// End of cFIO functions.


//-----------------------------------------------------------------------------
// FIOcvtPrms   Conversion specification struct.

FIOcvtPrms::FIOcvtPrms()
{
    cp_scale = 1.0;
    cp_destination = 0;
    cp_alias_info = 0;
    cp_alias_mask = 0;
    cp_filetype = Fnone;
    cp_allow_layer_mapping = false;
    cp_use_window = false;
    cp_flatten = false;
    cp_clip = false;
    cp_ecf_level = ECFnone;
    cp_to_cgd = false;
}


FIOcvtPrms::FIOcvtPrms(const FIOcvtPrms &p)
{
    cp_scale = p.cp_scale;
    cp_destination = lstring::copy(p.cp_destination);
    cp_alias_info = p.cp_alias_info;
    cp_window = p.cp_window;
    cp_alias_mask = p.cp_alias_mask;
    cp_filetype = p.cp_filetype;
    cp_allow_layer_mapping = p.cp_allow_layer_mapping;
    cp_use_window = p.cp_use_window;
    cp_flatten = p.cp_flatten;
    cp_clip = p.cp_clip;
    cp_ecf_level =  p.cp_ecf_level;
    cp_to_cgd = p.cp_to_cgd;
}


FIOcvtPrms::~FIOcvtPrms()
{
    delete [] cp_destination;
}

void
FIOcvtPrms::set_destination(const char *dst, FileType ft, bool use_cgd)
{
    delete [] cp_destination;
    cp_destination = 0;

    if (use_cgd) {
        // Will build CGD database, destination is the tag name, don't
        // add path (of course).
        cp_destination = lstring::copy(dst);
        cp_filetype = Foas;
        cp_to_cgd = true;
        return;
    }

    cp_filetype = ft;

    if (dst) {
        while (isspace(*dst))
            dst++;
    }
    if (!dst || !*dst) {
        if (ft == Fnative)
            cp_destination = cFIO::PGetCWD(0, 255);
        else
            cp_destination = 0;
    }
    else if (!lstring::is_rooted(dst)) {
        char *cwd = cFIO::PGetCWD(0, 256);
        if (!cwd)
            cp_destination = 0;
        cp_destination = pathlist::mk_path(cwd, dst);
        delete [] cwd;
    }
    else
        cp_destination = lstring::copy(dst);
}
// End of FIOcvtPrms functions

