
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#include "fio.h"
#include "fio_assemble.h"
#include "fio_chd.h"
#include "fio_cvt_base.h"
#include "cd_digest.h"
#include <ctype.h>


//------------------------------------------------------------------------
//
// This code implements a feature whereby several archives can be
// merged into a larger archive, on-the-fly so as to avoid memory
// limitations.
//
//------------------------------------------------------------------------

// There are two ways to do this:
//
// 1.  Under control of a special input file using parse() and run().
// 2.  Directly calling the more primitive functions, perhaps under
//     control of a script.  This allows flags, etc. to be controlled
//     per input file.


// Default log file name, created in CWD.
#define DEF_LOGFILE "assemble.log"

// Function to parse a specification script or an argument list.
//
bool
cFIO::AssembleArchive(const char *string)
{
    if (!string) {
        Errs()->add_error("AssembleArchive: null argument.");
        return (false);
    }
    while (isspace(*string))
        string++;
    if (!*string) {
        Errs()->add_error("AssembleArchive: empty argument.");
        return (false);
    }
    bool ret = true;
    if (*string == '-') {
        // The string contains an argument list.
        ajob_t job(0);
        ret = job.parse(string);
        if (!ret) {
            Errs()->add_error("Error processing argument list.");
            return (false);
        }
        ret = job.run(0);
    }
    else {
        // The string contains a file name.
        char *fname = lstring::getqtok(&string);
        FILE *fp = fopen(fname, "r");
        if (!fp) {
            Errs()->add_error("Can't open file %s.", fname);
            delete [] fname;
            return (false);
        }
        ajob_t job(0);
        ret = job.parse(fp);
        fclose(fp);
        if (!ret) {
            Errs()->add_error("Error reading %s.", fname);
            delete [] fname;
            return (false);
        }
        delete [] fname;
        ret = job.run(0);
    }
    return (ret);
}


ajob_t::~ajob_t()
{
    delete [] j_topcell;
    delete [] j_outfile;
    delete [] j_logfile;
    while (j_srcfiles) {
        asource_t *src = j_srcfiles;
        j_srcfiles = j_srcfiles->next_source();
        delete src;
    }
    delete j_prms;
    delete j_out;
    delete j_alias;
}


// Set up the output channel.
//
bool
ajob_t::open_stream()
{
    if (!j_prms) {
        FileType ftype = cFIO::TypeExt(j_outfile);
        if (!cFIO::IsSupportedArchiveFormat(ftype)) {
            Errs()->add_error(
                "Output file type not recognized or unsupported.");
            return (false);
        }
        j_prms = new FIOcvtPrms;
        j_prms->set_destination(j_outfile, ftype);
    }
    return (true);
}


// Define the name of a top-level cell that will be created in the
// output stream.
//
bool
ajob_t::add_topcell(const char *cname)
{
    if (j_topcell) {
        Errs()->add_error("Top cell already defined.");
        return (false);
    }
    j_topcell = lstring::copy(cname);
    return (true);
}


// Add a source specification to the job control struct.  Only one of
// fname or chd should be given.
//  fname           archive file name
//  chd             cell hierarchy digest
//  scale           scale factor to apply
//  only_layers     write only these layers (space-separated list of names)
//  skip_layers     skip these layers (space-separated list of names)
//  layer_aliases   name=newname list of layer aliases
//  cname_prefix    cell name prefix to apply
//  cname_suffix    cell name suffix to apply
//  ccvt            case conversion flags, 0-2
//
bool
ajob_t::add_source(const char *fname, cCHD *chd, double scale,
    const char *only_layers, const char *skip_layers,
    const char *layer_aliases,
    const char *cname_prefix, const char *cname_suffix, int ccvt)
{
    if (scale < CDSCALEMIN || scale > CDSCALEMAX) {
        Errs()->add_error("add_source: bad scale, out of range.");
        return (false);
    }
    asource_t *src;
    if (chd) {
        src = new asource_t(lstring::copy(chd->filename()), 0);
        src->set_chd(chd);
    }
    else {
        if (!fname || !*fname) {
            Errs()->add_error("add_source: no file name given.");
            return (false);
        }
        src = new asource_t(lstring::copy(fname), 0);
    }
    src->set_scale(scale);
    if (only_layers && *only_layers) {
        src->set_layer_list(lstring::copy(only_layers));
        src->set_only_layers(true);
    }
    else if (skip_layers && *skip_layers) {
        src->set_layer_list(lstring::copy(skip_layers));
        src->set_skip_layers(true);
    }
    if (layer_aliases && *layer_aliases)
        src->set_layer_aliases(lstring::copy(layer_aliases));
    if (cname_prefix && *cname_prefix)
        src->set_prefix(lstring::copy(cname_prefix));
    if (cname_suffix && *cname_suffix)
        src->set_suffix(lstring::copy(cname_suffix));
    if (ccvt == 1)
        src->set_to_lower(true);
    else if (ccvt == 2)
        src->set_to_upper(true);

    if (!j_srcfiles)
        j_srcfiles = src;
    else {
        asource_t *end = j_srcfiles;
        while (end->next_source())
            end = end->next_source();
        end->set_next_source(src);
    }
    return (true);
}


// Add an instance to the list for the current source.  The instance
// will be added to the top cell, if defined, using the transformation
// parameters given.  If the extraction flag is set in the source, the
// instance and its descendents will be extracted before streaming.
// Otherwise, or if no instances are given, the entire archive is
// streamed.
//
bool
ajob_t::add_instance(const char *cname, int x, int y, bool my, double rot,
    double magn, double scale, bool no_hier, ECFlevel ecf_lev, bool flatten,
    BBox *winBB, bool clip)
{
    if (!cname || !*cname) {
        Errs()->add_error("add_instance: no cell name given.");
        return (false);
    }
    if (magn < CDSCALEMIN || magn > CDSCALEMAX) {
        Errs()->add_error(
            "add_instance: bad magnification, out of range.");
        return (false);
    }
    if (!j_srcfiles) {
        Errs()->add_error("add_instance: no source has been given.");
        return (false);
    }
    asource_t *src = j_srcfiles;
    while (src->next_source())
        src = src->next_source();
    ainst_t *inst = new ainst_t(lstring::copy(cname), x, y, my, rot, magn,
        scale, no_hier, ecf_lev, flatten, winBB, clip);
    if (!src->instances())
        src->set_instances(inst);
    else {
        ainst_t *ii = src->instances();
        while (ii->next_instance())
            ii = ii->next_instance();
        ii->set_next_instance(inst);
    }
    return (true);
}


bool
ajob_t::write_stream(asource_t *src)
{
    if (src->scale() < CDSCALEMIN || src->scale() > CDSCALEMAX) {
        Errs()->add_error("write_stream: bad scale, out of range.");
        return (false);
    }

    // push layer aliasing
    FIOlayerFilt lftmp;
    lftmp.set();
    src->push_layer_alias();

    bool free_chd = false;
    FileType ft = Fnone;
    cCHD *chd = src->chd();
    if (!chd)
        chd = CDchd()->chdRecall(src->path(), false);
    if (!chd) {
        char *realname;
        FILE *fp = FIO()->POpen(src->path(), "r", &realname);
        if (!fp) {
            Errs()->add_error("write_stream: can't open %s.", src->path());
            lftmp.push();
            return (false);
        }
        ft = FIO()->GetFileType(fp);
        fclose(fp);
        if (!cFIO::IsSupportedArchiveFormat(ft)) {

            // The file might be a saved digest file, if so, read it.
            sCHDin chd_in;
            if (!chd_in.check(realname)) {
                Errs()->add_error("write_stream: unknown data format.");
                delete [] realname;
                lftmp.push();
                return (false);
            }
            chd = chd_in.read(realname, sCHDin::get_default_cgd_type());
            if (!chd) {
                Errs()->add_error("write_stream: CHD file read error.");
                delete [] realname;
                lftmp.push();
                return (false);
            }
            free_chd = true;
        }
        src->set_path(realname);

    }

    if (src->instances()) {
        // In this case, assume that a CHD is required.
        if (!chd) {

            // If any of the instances require pre-filtering for empty
            // cell removal, include full info counts in the new CHD.
            cvINFO info = cvINFOtotals;
            for (ainst_t *inst = src->instances(); inst;
                    inst = inst->next_instance()) {
                if (inst->ecf_level() == ECFall ||
                        inst->ecf_level() == ECFpre) {
                    info = cvINFOplpc;
                    break;
                }
            }

            chd = FIO()->NewCHD(src->path(), ft, Physical, 0, info);
            if (!chd) {
                Errs()->add_error(
                    "write_stream: CHD creation failed for %s.",
                    src->path());
                lftmp.push();
                return (false);
            }
            free_chd = true;
        }

        // Set up the reference pointers.  These link instances that differ
        // only in the placement transform.
        for (ainst_t *inst = src->instances(); inst;
                inst = inst->next_instance()) {

            // Set the cellname if necessary (PlaceTop lines).
            if (!inst->cellname())
                inst->set_cellname(lstring::copy(chd->defaultCell(Physical)));

            for (ainst_t *i2 = src->instances(); i2 && i2 != inst;
                    i2 = i2->next_instance()) {
                if (inst->is_similar(i2)) {
                    inst->set_refinst(i2);
                    break;
                }
            }
        }
    }

    // Note that layer mapping is always enabled.
    cv_in *in = 0;
    if (chd)
        in = chd->newInput(true);
    else {
        in = FIO()->NewInput(ft, true);
        if (in)
            in->setup_source(src->path());
    }

    bool ret = true;
    if (!in) {
        Errs()->add_error("write_stream: input channel creation failed.");
        ret = false;
    }

    if (ret) {
        in->set_show_progress(true);
        if (!j_out) {
            FIO()->SetAllowPrptyStrip(true);
            ret = in->setup_destination(j_prms->destination(),
                j_prms->filetype(), j_prms->to_cgd());
            FIO()->SetAllowPrptyStrip(false);
            if (ret) {
                j_out = in->backend();
                in->set_no_open_lib(false);
                in->set_no_end_lib(true);
            }
        }
        else {
            ret = in->setup_backend(j_out);
            in->set_no_open_lib(true);
            in->set_no_end_lib(true);
        }
    }

    if (ret) {
        // Take care of alias table initialization.
        if (!j_alias)
            j_alias = new FIOaliasTab(true, true);
        else
            j_alias->reinit();
        j_alias->set_substitutions(src->prefix(), src->suffix());
        j_alias->set_to_lower(src->to_lower());
        j_alias->set_to_upper(src->to_upper());
        if (j_prms->filetype() == Fgds) {
            j_alias->set_gds_check(true);
            j_alias->set_limit32(FIO()->GdsOutLevel());
        }
        j_out->assign_alias(j_alias);

        if (!chd) {
            FIO()->ifInfoMessage(IFMSG_CNAME, "Streaming: %s",
                lstring::strip_path(src->path()));
            if (!in->parse(Physical, false, src->scale())) {
                Errs()->add_error("write_stream: error processing %s.",
                    src->path());
                ret = false;
            }
            if (ret) {
                // Change to aliased cellnames for placement.
                for (ainst_t *inst = src->instances(); inst;
                        inst = inst->next_instance()) {
                    const char *cellname = j_out->alias(inst->cellname());
                    if (cellname != inst->cellname()) {
                        inst->set_cellname(lstring::copy(cellname));
                    }
                }
            }
        }
        else if (src->instances()) {
            for (ainst_t *inst = src->instances(); inst;
                    inst = inst->next_instance()) {

                if (inst->refinst()) {
                    // We've already processed a cell for this
                    // instance, just update the name, it will be
                    // placed later.

                    if (strcmp(inst->cellname(),
                            inst->refinst()->cellname()))
                        inst->set_cellname(
                            lstring::copy(inst->refinst()->cellname()));
                    continue;
                }
                if (inst->placename() && *inst->placename()) {
                    const char *nn =
                        j_alias->new_name(inst->placename(), false);
                    j_alias->set_alias(inst->cellname(), nn);
                }

                symref_t *p = chd->findSymref(inst->cellname(), Physical);
                if (!p) {
                    Errs()->add_error(
                        "write_stream: can't find cell %s in CHD for "
                        "archive file\n%s.",
                        inst->cellname(), chd->filename());
                    ret = false;
                }
                else {
                    FIO()->ifInfoMessage(IFMSG_CNAME,
                        "Streaming: %s from %s",
                        inst->cellname(), chd->filename());

                    OItype oiret;
                    if (inst->flatten())
                        oiret = chd->flatten(p, in, CDMAXCALLDEPTH,
                            inst->prms());
                    else
                        oiret = chd->write(p, in, inst->prms(),
                            !inst->no_hier());

                    if (oiret != OIok) {
                        Errs()->add_error("write_stream: error processing %s.",
                            src->path());
                        ret = false;
                    }

                    in->set_no_open_lib(true);
                    in->set_no_end_lib(true);

                    // Alias the cell name.
                    const char *cellname = j_out->alias(inst->cellname());
                    if (cellname != inst->cellname())
                        inst->set_cellname(lstring::copy(cellname));
                    j_alias->reinit();
                }
                if (!ret)
                    break;
            }
        }
        else {
            symref_t *p = chd->defaultSymref(Physical);
            if (!p)
                ret = false;
            else {
                const char *nn = j_alias->new_name(Tstring(p->get_name()),
                    false);
                j_alias->set_alias(Tstring(p->get_name()), nn);

                FIO()->ifInfoMessage(IFMSG_CNAME, "Streaming: %s from %s",
                    p->get_name(), chd->filename());

                FIOcvtPrms prms;
                prms.set_scale(src->scale());
                prms.set_allow_layer_mapping(true);
                OItype oiret = chd->write(p, in, &prms, true);
                if (oiret != OIok) {
                    Errs()->add_error("write_stream: error processing %s.",
                        src->path());
                    ret = false;
                }
                in->set_no_open_lib(true);
                in->set_no_end_lib(true);
                j_alias->reinit();
            }
        }
        j_out->extract_alias();
    }

    delete in;
    if (free_chd)
        delete chd;
    lftmp.push();
    return (ret);
}


// Add an end record to the stream, and close it.
//
bool
ajob_t::finish_stream()
{
    if (!j_out) {
        Errs()->add_error("finish_stream: no output context.");
        return (false);
    }

    j_alias->reinit();
    j_alias->set_substitutions(0, 0);

    Topcell *tc = new_topcell();
    bool ret = j_out->write_multi_final(tc);
    delete tc;
    delete j_out;
    j_out = 0;
    delete j_alias;
    j_alias = 0;
    if (!ret)
        Errs()->add_error("finish_stream: write_endlib failed.");
    return (ret);
}


// Create a Topcell* to pass to the translator.
//
Topcell *
ajob_t::new_topcell()
{
    if (!j_topcell)
        return (0);

    int cnt = 0;
    for (asource_t *src = j_srcfiles; src; src = src->next_source()) {
        for (ainst_t *ai = src->instances(); ai; ai = ai->next_instance())
            cnt++;
    }
    if (!cnt)
        return (0);

    Topcell *ts = new Topcell;
    ts->cellname = j_alias->new_name(j_topcell, false);
    ts->numinst = cnt;
    ts->instances = new Instance[cnt];

    cnt = 0;
    for (asource_t *src = j_srcfiles; src; src = src->next_source()) {
        for (ainst_t *ai = src->instances(); ai; ai = ai->next_instance()) {
            ts->instances[cnt].name = ai->cellname();
            ts->instances[cnt].origin.set(ai->pos_x(), ai->pos_y());
            ts->instances[cnt].angle = ai->angle();
            ts->instances[cnt].magn = ai->magn();
            ts->instances[cnt].reflection = ai->reflc();
            char *emsg = FIO()->GdsParamSet(&ts->instances[cnt].angle,
                &ts->instances[cnt].magn,
                &ts->instances[cnt].ax, &ts->instances[cnt].ay);
            if (emsg) {
                FIO()->ifPrintCvLog(IFLOG_WARN, emsg);
                delete [] emsg;
            }
            cnt++;
        }
    }
    return (ts);
}


#define matching(s) lstring::cieq(tok, s)

namespace {
    // Copy the string and remove trailing white space.  Return null if
    // copy is empty.
    //
    inline char *
    newstr(const char *str)
    {
        char *s = lstring::copy(str);
        char *t = s + strlen(s) - 1;
        while (t >= s && isspace(*t))
            *t-- = 0;
        if (!*s) {
            delete s;
            return (0);
        }
        return (s);
    }
}


// Parse a specification file, return true if successful.
// Otherwise, set the error message and return 0.
//
// The specification file has the following format:
//
// Comments start with non-alpha and are ignored.  Overall the file
// looks like
//    Header Block
//    Source Block 
//        [Placement Block]
//        [...]
//    [...]
//
// The Header Block contains:
//   OutFile out_file_name
//     This is mandatory: name of output file.
//
//   LogFile log_file_name
//     Name of log file to use.
//
//   TopCell cellname
//     Optional top-level cell added to output for placing subcells.
//
// The Header Block can also contain Source Block and Placement Block
// options, which apply in following Source Blocks.  The Header Block
// context is ahead of all and between Source Blocks.
//
// There must be at least one Source Block.  Source Blocks start with
//   Source filename
//     Name of a source file or CHD.  This starts a block of lines that
//     apply to that source.
//
// Source blocks can be terminated with
//   EndSource
//     Optional end token for Source block.  Useful only for changing
//     default options.
//
// The following are the Source Block options.  These can appear in
// the Header Block context, in which case they apply as defaults to
// following Source Blocks.  They can appear in Source Blocks, where
// they override options set in the Header Block context and apply to
// the current Source Block only.  Within a Source Block, the options
// that apply are those logically in force at the end of the block,
//
//   LayerList list_of_layer_names
//     Set the list of layers to be used with OnlyLayers or SkipLayers. 
//     This is implied when a list_of_layer_names is provided with these
//     keywords.
//
//   OnlyLayers [list_of_layer_names]
//     Use only the layers listed.  If no list_of_layer_names is found,
//     the list that applies is taken from the list provided by any of
//     OnlyLayers, SkipLayers, or LayerList currently in scope.
//
//   NoOnlyLayers
//     Turn off OnlyLayers in the present scope if a default is active.
//
//   SkipLayers [list_of_layer_names]
//     Skip the layers in the list.  If no list_of_layer_names is found,
//     the list that applies is taken from the list provided by any of
//     OnlyLayers, SkipLayers, or LayerList currently in scope.
//
//   NoSkipLayers
//     Turn off SkipLayers in the present scope if a default is active.
//
//   LayerAliases new1=old1 new2=old2 ...
//     List of layer aliases to apply.
//
//   ConvertScale scale_factor
//     Scaling to apply, only active in Source Blocks with no
//     Placement Blocks, range is 1e-3 to 1e3.
//
//   ToLower
//     Flag to indicate conversion of upper case cell names to lower
//     case.
//
//   NoToLower
//     Turn off ToLower in the present scope if a default is active.
//
//   ToUpper
//     Flag to indicate conversion of lower case cell names to upper
//     case.
//
//   NoToUpper
//     Turn off ToUpper in the present scope if a default is active.
//
//   CellNamePrefix prefix_string
//     Cell name change prefix.  This operation occurs after case
//     conversion.
//
//   CellNameSuffix suffix_string
//     Cell name change suffix.  This operation occurs after case
//     conversion.
//
// This concludes the Source Block options.
//
// Each Source Block can have zero or more Placement Blocks. 
// Placement Blocks provide a cell name to extract from the source,
// and processing options.  Cells from placement blocks (and possibly
// their hierarchy) will be streamed to output, and will be
// instantiated in the TopCell if given.  If no Placement Blocks
// appear, the entire file is streamed, or the default cell if the
// Source Block specifies a CHD.
//
// Placement Blocks can only appear within Source Blocks.  Source
// Block options can not appear within Placement Blocks.
//
// Placement blocks start with a Place or PlaceTop keyword.
//   Place cellname [placement_name]
//     The placement_name, if given, will replace cellname in output.  The
//     actual names used in output will be internally altered if necessary
//     so that each output cell name is unique, by adding "$N", where N is
//     an integer.  If a block matches a previous block except for the
//     transformation parameters (Translate, Rotate, Magnify, Reflect),
//     then if a TopCell was given, an instance will be added with the new
//     transform.  If no TopCell, there would be no addition to output.
//
//   PlaceTop [placement_name]
//     The PlaceTop line is equivalent to a Place line, except that it
//     will automatically select the first top-level cell found in the
//     source.  It is equivalent to the Place line with the name of
//     this cell as the first argument.  This is convenient when the
//     cell name is unknown,
//
//   EndPlace
//     Optional end token for Place/PlaceTop block, useful only for
//     changing default options.
//
// The following are the Placement Block options.  They can appear in
// the Header Block context, where they act as defaults in following
// Source Blocks.  They can appear in Source Blocks context, where
// they override options set in the Header Block context, and apply as
// defaults to Placement Blocks that follow in the Source Block.  They
// can appear in Placement Blocks, where they override options set
// above in scope, and apply to that Placement Block only.  These
// options are ignored in Source Blocks with no Placement Blocks.
//
// This group of options apply the instance transformation used when
// instantiating the cell in the TopVell.  They are ignored if no
// TopCell was given.
//
//   Translate x y
//   Rotate angle
//   Magnify magn
//   Reflect
//   NoReflect
//     Transformation for cellname instance in top cell.  Use NoReflect
//     to turn off Reflect in the present scope if a default is active.
//
//   NoEmpties
//     If given, empty subcells of cellname will not be written to output.
//
//   NoNoEmpties
//     Turn off NoEmpties in the present scope if a default is active.
//
//   Flatten
//     If given, all geometry under cellname will be written as part
//     of cellname, i.e., cell hierarchy will be flattened.
//
//   NoFlatten
//     Turn off Flatten in the present scope if a default is active.
//
//   Window left bottom right top
//     If given, only the subcells and objects needed to describe the
//     given area in cellname will be written.
//
//   Clip
//     If Window was given, this will cause geometry to be clipped to
//     the window.
//
//   NoClip
//     Turn off Clip in the present scope if a default is active.
//
// This concludes the Placement Block options.
//
bool
ajob_t::parse(FILE *fp)
{
    const char *msg = "Error: line %d, %s.";
    const char *plmsg = "Error: line %d, %s not allowed in Placement block.";

    if (!fp) {
        Errs()->add_error("Error: null file pointer.");
        return (false);
    }

    // This holds default values, destructor will free strings.
    asource_t defaults(0, 0);
    ainst_t inst_src_defs(0, 0);
    ainst_t inst_defs(0, 0);

    ainst_t *aend = 0;
    asource_t *send = 0;
    int linecnt = 0;
    char buf[256];
    while (fgets(buf, 256, fp) != 0) {
        linecnt++;
        char *s = buf;
        while (isspace(*s))
            s++;
        if (!*s || !isalpha(*s))
            continue;
        char *tok = lstring::gettok(&s);
        if (matching("OutFile")) {
            if (send) {
                delete [] tok;
                Errs()->add_error(msg, linecnt,
                    "OutFile can't appear in Source Block");
                return (false);
            }
            if (j_outfile) {
                delete [] tok;
                Errs()->add_error(msg, linecnt, "OutFile already specified");
                return (false);
            }
            char *fn = lstring::getqtok(&s);
            if (!fn) {
                delete [] tok;
                Errs()->add_error(msg, linecnt, "missing file name");
                return (false);
            }
            FileType ft = cFIO::TypeExt(fn);
            if (!cFIO::IsSupportedArchiveFormat(ft)) {
                delete [] tok;
                delete [] fn;
                Errs()->add_error(msg, linecnt,
                    "unsupported or unknown type extension");
                return (false);
            }
            j_outfile = fn;
        }
        if (matching("LogFile")) {
            if (send) {
                delete [] tok;
                Errs()->add_error(msg, linecnt,
                    "LogFile can't appear in Source Block");
                return (false);
            }
            if (j_logfile) {
                delete [] tok;
                Errs()->add_error(msg, linecnt, "LogFile already specified");
                return (false);
            }
            char *fn = lstring::getqtok(&s);
            if (!fn) {
                delete [] tok;
                Errs()->add_error(msg, linecnt, "missing file name");
                return (false);
            }
            j_logfile = fn;
        }
        else if (matching("TopCell")) {
            if (send) {
                delete [] tok;
                Errs()->add_error(msg, linecnt,
                    "TopCell can't appear in Source Block");
                return (false);
            }
            if (j_topcell) {
                delete [] tok;
                Errs()->add_error(msg, linecnt, "TopCell already specified");
                return (false);
            }
            char *cn = lstring::gettok(&s);
            if (!cn) {
                delete [] tok;
                Errs()->add_error(msg, linecnt, "missing cell name");
                return (false);
            }
            j_topcell = cn;
        }
        else if (matching("Source")) {
            if (!j_outfile) {
                delete [] tok;
                Errs()->add_error(msg, linecnt, "no OutFile given");
                return (false);
            }
            char *fn = lstring::getqtok(&s);
            if (!fn) {
                delete [] tok;
                Errs()->add_error(msg, linecnt, "missing file name");
                return (false);
            }
            // Don't check for type extention, format will be
            // recognized by content.
            if (!j_srcfiles)
                j_srcfiles = send = new asource_t(fn, &defaults);
            else {
                send = j_srcfiles;
                while (send->next_source())
                    send = send->next_source();
                send->set_next_source(new asource_t(fn, &defaults));
                send = send->next_source();
            }
            aend = 0;
            inst_src_defs = inst_defs;
        }
        else if (matching("EndSource")) {
            send = 0;
            aend = 0;
            inst_src_defs = inst_defs;
        }
        else if (matching("OnlyLayers")) {
            if (aend) {
                Errs()->add_error(plmsg, linecnt, tok);
                delete [] tok;
                return (false);
            }
            char *t = newstr(s);
            if (!send) {
                if (t)
                    defaults.set_layer_list(t);
                defaults.set_only_layers(true);
            }
            else {
                if (t)
                    send->set_layer_list(t);
                send->set_only_layers(true);
            }
        }
        else if (matching("NoOnlyLayers")) {
            if (aend) {
                Errs()->add_error(plmsg, linecnt, tok);
                delete [] tok;
                return (false);
            }
            if (!send)
                defaults.set_only_layers(false);
            else
                send->set_only_layers(false);
        }
        else if (matching("SkipLayers")) {
            if (aend) {
                Errs()->add_error(plmsg, linecnt, tok);
                delete [] tok;
                return (false);
            }
            char *t = newstr(s);
            if (!send) {
                if (t)
                    defaults.set_layer_list(t);
                defaults.set_skip_layers(true);
            }
            else {
                if (t)
                    send->set_layer_list(t);
                send->set_skip_layers(true);
            }
        }
        else if (matching("NoSkipLayers")) {
            if (aend) {
                Errs()->add_error(plmsg, linecnt, tok);
                delete [] tok;
                return (false);
            }
            if (!send)
                defaults.set_skip_layers(false);
            else
                send->set_skip_layers(false);
        }
        else if (matching("LayerList")) {
            if (aend) {
                Errs()->add_error(plmsg, linecnt, tok);
                delete [] tok;
                return (false);
            }
            char *t = newstr(s);
            if (!send) {
                if (t)
                    defaults.set_layer_list(t);
            }
            else {
                if (t)
                    send->set_layer_list(t);
            }
        }
        else if (matching("LayerAliases")) {
            if (aend) {
                Errs()->add_error(plmsg, linecnt, tok);
                delete [] tok;
                return (false);
            }
            char *t = newstr(s);
            if (!send) {
                if (t)
                    defaults.set_layer_aliases(t);
            }
            else {
                if (t)
                    send->set_layer_aliases(t);
            }
        }
        else if (matching("ConvertScale")) {
            if (aend) {
                Errs()->add_error(plmsg, linecnt, tok);
                delete [] tok;
                return (false);
            }
            double sc;
            if (sscanf(s, "%lf", &sc) != 1 || sc < 1e-3 || sc > 1e3) {
                delete [] tok;
                Errs()->add_error(msg, linecnt,
                    "bad or missing conversion scale");
                return (false);
            }
            if (!send)
                defaults.set_scale(sc);
            else
                send->set_scale(sc);
        }
        else if (matching("CellNamePrefix")) {
            if (aend) {
                Errs()->add_error(plmsg, linecnt, tok);
                delete [] tok;
                return (false);
            }
            char *t = newstr(s);
            if (!send)
                defaults.set_prefix(t);
            else
                send->set_prefix(t);
        }
        else if (matching("CellNameSuffix")) {
            if (aend) {
                Errs()->add_error(plmsg, linecnt, tok);
                delete [] tok;
                return (false);
            }
            char *t = newstr(s);
            if (!send)
                defaults.set_suffix(t);
            else
                send->set_suffix(t);
        }
        else if (matching("ToUpper")) {
            if (aend) {
                Errs()->add_error(plmsg, linecnt, tok);
                delete [] tok;
                return (false);
            }
            if (!send)
                defaults.set_to_upper(true);
            else
                send->set_to_upper(true);
        }
        else if (matching("NoToUpper")) {
            if (aend) {
                Errs()->add_error(plmsg, linecnt, tok);
                delete [] tok;
                return (false);
            }
            if (!send)
                defaults.set_to_upper(false);
            else
                send->set_to_upper(false);
        }
        else if (matching("ToLower")) {
            if (aend) {
                Errs()->add_error(plmsg, linecnt, tok);
                delete [] tok;
                return (false);
            }
            if (!send)
                defaults.set_to_lower(true);
            else
                send->set_to_lower(true);
        }
        else if (matching("NoToLower")) {
            if (aend) {
                Errs()->add_error(plmsg, linecnt, tok);
                delete [] tok;
                return (false);
            }
            if (!send)
                defaults.set_to_lower(false);
            else
                send->set_to_lower(false);
        }
        else if (matching("Place")) {
            char *cn = lstring::gettok(&s);
            if (!cn) {
                delete [] tok;
                Errs()->add_error(msg, linecnt, "missing cell name");
                return (false);
            }
            if (!send) {
                delete [] tok;
                Errs()->add_error(msg, linecnt, "no Source given");
                return (false);
            }
            if (!send->instances()) {
                aend = new ainst_t(cn, &inst_src_defs);
                send->set_instances(aend);
            }
            else {
                aend = send->instances();
                while (aend->next_instance())
                    aend = aend->next_instance();
                aend->set_next_instance(new ainst_t(cn, &inst_src_defs));
                aend = aend->next_instance();
            }
            cn = lstring::gettok(&s);
            if (cn)
                aend->set_placename(cn);
        }
        else if (matching("PlaceTop")) {
            if (!send) {
                delete [] tok;
                Errs()->add_error(msg, linecnt, "no Source given");
                return (false);
            }
            if (!send->instances()) {
                aend = new ainst_t(0, &inst_src_defs);
                send->set_instances(aend);
            }
            else {
                aend = send->instances();
                while (aend->next_instance())
                    aend = aend->next_instance();
                aend->set_next_instance(new ainst_t(0, &inst_src_defs));
                aend = aend->next_instance();
            }
            char *cn = lstring::gettok(&s);
            if (cn)
                aend->set_placename(cn);
        }
        else if (matching("EndPlace")) {
            if (!aend) {
                delete [] tok;
                Errs()->add_error(msg, linecnt, "no Place/PlaceTop given");
                return (false);
            }
            aend = 0;
        }
        else if (matching("Translate")) {
            double x, y;
            if (sscanf(s, "%lf %lf", &x, &y) != 2) {
                delete [] tok;
                Errs()->add_error(msg, linecnt, "bad or missing x y");
                return (false);
            }
            if (aend) {
                aend->set_pos_x(INTERNAL_UNITS(x));
                aend->set_pos_y(INTERNAL_UNITS(y));
            }
            else if (send) {
                inst_src_defs.set_pos_x(INTERNAL_UNITS(x));
                inst_src_defs.set_pos_y(INTERNAL_UNITS(y));
            }
            else {
                inst_defs.set_pos_x(INTERNAL_UNITS(x));
                inst_defs.set_pos_y(INTERNAL_UNITS(y));
            }
        }
        else if (matching("Rotate")) {
            double ang;
            if (sscanf(s, "%lf", &ang) != 1) {
                delete [] tok;
                Errs()->add_error(msg, linecnt, "bad or missing angle");
                return (false);
            }
            if (aend)
                aend->set_angle(ang);
            else if (send)
                inst_src_defs.set_angle(ang);
            else
                inst_defs.set_angle(ang);
        }
        else if (matching("Magnify")) {
            // Magnification factor for cell placements.
            double magn;
            if (sscanf(s, "%lf", &magn) != 1 || magn < 1e-3 || magn > 1e3) {
                delete [] tok;
                Errs()->add_error(msg, linecnt,
                    "bad or missing magnification");
                return (false);
            }
            if (aend)
                aend->set_magn(magn);
            else if (send)
                inst_src_defs.set_magn(magn);
            else
                inst_defs.set_magn(magn);
        }
        else if (matching("Scale")) {
            // Scale factor for cell definitions.
            double sc;
            if (sscanf(s, "%lf", &sc) != 1 || sc < 1e-3 || sc > 1e3) {
                delete [] tok;
                Errs()->add_error(msg, linecnt,
                    "bad or missing scale factor");
                return (false);
            }
            if (aend)
                aend->set_scale(sc);
            else if (send)
                inst_src_defs.set_scale(sc);
            else
                inst_defs.set_scale(sc);
        }
        else if (matching("Reflect")) {
            if (aend)
                aend->set_reflc(true);
            else if (send)
                inst_src_defs.set_reflc(true);
            else
                inst_defs.set_reflc(true);
        }
        else if (matching("NoReflect")) {
            if (aend)
                aend->set_reflc(false);
            else if (send)
                inst_src_defs.set_reflc(false);
            else
                inst_defs.set_reflc(false);
        }
        else if (matching("NoHier")) {
            if (aend)
                aend->set_no_hier(true);
            else if (send)
                inst_src_defs.set_no_hier(true);
            else
                inst_defs.set_no_hier(true);
        }
        else if (matching("NoNoHier")) {
            if (aend)
                aend->set_no_hier(false);
            else if (send)
                inst_src_defs.set_no_hier(false);
            else
                inst_defs.set_no_hier(false);
        }
        else if (matching("NoEmpties")) {
            ECFlevel lev = ECFnone;
            if (!*s || *s == '1')
                lev = ECFall;
            else if (*s == '2')
                lev = ECFpre;
            else if (*s == '3')
                lev = ECFpost;
            else if (*s == '0')
                ;
            else {
                delete [] tok;
                Errs()->add_error(msg, linecnt,
                    "unknown token following NoEmpties");
                return (false);
            }
            if (aend)
                aend->set_ecf_level(lev);
            else if (send)
                inst_src_defs.set_ecf_level(lev);
            else
                inst_defs.set_ecf_level(lev);
        }
        else if (matching("NoNoEmpties")) {
            if (aend)
                aend->set_ecf_level(ECFnone);
            else if (send)
                inst_src_defs.set_ecf_level(ECFnone);
            else
                inst_defs.set_ecf_level(ECFnone);
        }
        else if (matching("Flatten")) {
            if (aend)
                aend->set_flatten(true);
            else if (send)
                inst_src_defs.set_flatten(true);
            else
                inst_defs.set_flatten(true);
        }
        else if (matching("NoFlatten")) {
            if (aend)
                aend->set_flatten(false);
            else if (send)
                inst_src_defs.set_flatten(false);
            else
                inst_defs.set_flatten(false);
        }
        else if (matching("Window")) {
            double l, b, r, t;
            if (sscanf(s, "%lf %lf %lf %lf", &l, &b, &r, &t) != 4) {
                delete [] tok;
                Errs()->add_error(msg, linecnt, "can't parse L B R T");
                return (false);
            }
            BBox BB(INTERNAL_UNITS(l), INTERNAL_UNITS(b), INTERNAL_UNITS(r),
                INTERNAL_UNITS(t));
            if (aend) {
                aend->set_use_win(true);
                aend->set_winBB(&BB);
            }
            else if (send) {
                inst_src_defs.set_use_win(true);
                inst_src_defs.set_winBB(&BB);
            }
            else {
                inst_defs.set_use_win(true);
                inst_defs.set_winBB(&BB);
            }
        }
        else if (matching("Clip")) {
            if (aend)
                aend->set_clip(true);
            else if (send)
                inst_src_defs.set_clip(true);
            else
                inst_defs.set_clip(true);
        }
        else if (matching("NoClip")) {
            if (aend)
                aend->set_clip(false);
            else if (send)
                inst_src_defs.set_clip(false);
            else
                inst_defs.set_clip(false);
        }
        else {
            delete [] tok;
            Errs()->add_error(msg, linecnt, "unknown keyword");
            return (false);
        }
        delete [] tok;
    }
    if (!j_outfile) {
        Errs()->add_error("Error: no output file name given.");
        return (false);
    }
    if (!j_srcfiles) {
        Errs()->add_error("Error: no source file names given.");
        return (false);
    }
    return (true);
}


// Parse the string argument list.  This is pretty much logically the
// same as the file representation, the mapping is fairly obvious.
//
// Header Block:
//   -o output_file
//   [-log log_file]
//   [-t top_cell_name]
//
// Source Block:
//   -i input_file
//   [-i-]
//
// Source Block options:
//   -l layer_list
//   -n[-]
//   -k[-]
//   -a layer_aliases
//   -cs scale
//   -p prefix
//   -u suffix
//   -tlo[-]
//   -tup[-]
//
// Placement Block:
//   [-c cellname | -ctop]
//   [-ca cellname_alias]
//   [-c-]
//
// Placement Block options:
//   -m magn
//   -s scale
//   -x x
//   -y y
//   -ang angle
//   -my[-]
//   -f[-]
//   -e[-]
//   -h[-]
//   -w l,b,r,t
//   -cl[-]
//
bool
ajob_t::parse(const char *string)
{
    const char *msg = "Error: missing %s argument.";
    const char *plmsg = "Error: %s in -c|-ctop/-c- scope.";

    if (!string) {
        Errs()->add_error("Error: null specification string.");
        return (false);
    }

    // This holds default values, destructor will free strings.
    asource_t defaults(0, 0);
    ainst_t inst_src_defs(0, 0);
    ainst_t inst_defs(0, 0);

    ainst_t *aend = 0;
    asource_t *send = 0;
    char *tok;
    while ((tok = lstring::gettok(&string)) != 0) {

        if (matching("-o")) {
            if (send) {
                delete [] tok;
                Errs()->add_error("Error: -o in -i/-i- context.");
                return (false);
            }
            if (j_outfile) {
                delete [] tok;
                Errs()->add_error("Error: -o given more than once.");
                return (false);
            }
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-o");
                return (false);
            }
            FileType ft = cFIO::TypeExt(tok);
            if (!cFIO::IsSupportedArchiveFormat(ft)) {
                delete [] tok;
                Errs()->add_error(
                    "Error: -o, unsupported or unknown type extension.");
                return (false);
            }
            j_outfile = tok;
            tok = 0;
        }
        else if (matching("-log")) {
            delete [] tok;
            if (send) {
                Errs()->add_error("Error: -log in -i/-i- context.");
                return (false);
            }
            if (j_logfile) {
                Errs()->add_error("Error: -log given more than once.");
                return (false);
            }
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-log");
                return (false);
            }
            j_logfile = tok;
            tok = 0;;
        }
        else if (matching("-t")) {
            if (send) {
                delete [] tok;
                Errs()->add_error("Error: -t in -i/-i- context.");
                return (false);
            }
            if (j_topcell) {
                delete [] tok;
                Errs()->add_error("Error: -t given more than once.");
                return (false);
            }
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-t");
                return (false);
            }
            j_topcell = tok;
            tok = 0;
        }
        else if (matching("-i")) {
            if (!j_outfile) {
                delete [] tok;
                Errs()->add_error("Error: -i must follow -o.");
                return (false);
            }
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-i");
                return (false);
            }
            // Don't check for type extention, format will be
            // recognized by content.
            if (!j_srcfiles)
                j_srcfiles = send = new asource_t(tok, &defaults);
            else {
                send = j_srcfiles;
                while (send->next_source())
                    send = send->next_source();
                send->set_next_source(new asource_t(tok, &defaults));
                send = send->next_source();
            }
            tok = 0;
            aend = 0;
            inst_src_defs = inst_defs;
        }
        else if (matching("-i-")) {
            send = 0;
            aend = 0;
            inst_src_defs = inst_defs;
        }
        else if (matching("-n")) {
            if (aend) {
                Errs()->add_error(plmsg, tok);
                delete [] tok;
                return (false);
            }
            if (!send)
                defaults.set_only_layers(true);
            else
                send->set_only_layers(true);
        }
        else if (matching("-n-")) {
            if (aend) {
                Errs()->add_error(plmsg, tok);
                delete [] tok;
                return (false);
            }
            if (!send)
                defaults.set_only_layers(false);
            else
                send->set_only_layers(false);
        }
        else if (matching("-k")) {
            if (aend) {
                Errs()->add_error(plmsg, tok);
                delete [] tok;
                return (false);
            }
            if (!send)
                defaults.set_skip_layers(true);
            else
                send->set_skip_layers(true);
        }
        else if (matching("-k-")) {
            if (aend) {
                Errs()->add_error(plmsg, tok);
                delete [] tok;
                return (false);
            }
            if (!send)
                defaults.set_skip_layers(false);
            else
                send->set_skip_layers(false);
        }
        else if (matching("-l")) {
            if (aend) {
                Errs()->add_error(plmsg, tok);
                delete [] tok;
                return (false);
            }
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-l");
                return (false);
            }
            if (!send)
                defaults.set_layer_list(tok);
            else
                send->set_layer_list(tok);
            tok = 0;
        }
        else if (matching("-a")) {
            if (aend) {
                Errs()->add_error(plmsg, tok);
                delete [] tok;
                return (false);
            }
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-a");
                return (false);
            }
            if (!send)
                defaults.set_layer_aliases(tok);
            else
                send->set_layer_aliases(tok);
            tok = 0;
        }
        else if (matching("-cs")) {
            if (aend) {
                Errs()->add_error(plmsg, tok);
                delete [] tok;
                return (false);
            }
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-cs");
                return (false);
            }
            double sc;
            if (sscanf(tok, "%lf", &sc) != 1 || sc < 1e-3 || sc > 1e3) {
                delete [] tok;
                Errs()->add_error("Error: bad or missing -cs argument.");
                return (false);
            }
            if (!send)
                defaults.set_scale(sc);
            else
                send->set_scale(sc);
        }
        else if (matching("-p")) {
            if (aend) {
                Errs()->add_error(plmsg, tok);
                delete [] tok;
                return (false);
            }
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-p");
                return (false);
            }
            if (!send)
                defaults.set_prefix(tok);
            else
                send->set_prefix(tok);
            tok = 0;
        }
        else if (matching("-u")) {
            if (aend) {
                Errs()->add_error(plmsg, tok);
                delete [] tok;
                return (false);
            }
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-u");
                return (false);
            }
            if (!send)
                defaults.set_suffix(tok);
            else
                send->set_suffix(tok);
            tok = 0;
        }
        else if (matching("-tup")) {
            if (aend) {
                Errs()->add_error(plmsg, tok);
                delete [] tok;
                return (false);
            }
            if (!send)
                defaults.set_to_upper(true);
            else
                send->set_to_upper(true);
        }
        else if (matching("-tup-")) {
            if (aend) {
                Errs()->add_error(plmsg, tok);
                delete [] tok;
                return (false);
            }
            if (!send)
                defaults.set_to_upper(false);
            else
                send->set_to_upper(false);
        }
        else if (matching("-tlo")) {
            if (aend) {
                Errs()->add_error(plmsg, tok);
                delete [] tok;
                return (false);
            }
            if (!send)
                defaults.set_to_lower(true);
            else
                send->set_to_lower(true);
        }
        else if (matching("-tlo-")) {
            if (aend) {
                Errs()->add_error(plmsg, tok);
                delete [] tok;
                return (false);
            }
            if (!send)
                defaults.set_to_lower(false);
            else
                send->set_to_lower(false);
        }
        else if (matching("-c")) {
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-c");
                return (false);
            }
            if (!send) {
                delete [] tok;
                Errs()->add_error("Error: -c not in -i/-i- scope.");
                return (false);
            }
            if (!send->instances()) {
                aend = new ainst_t(tok, &inst_src_defs);
                send->set_instances(aend);
            }
            else {
                aend = send->instances();
                while (aend->next_instance())
                    aend = aend->next_instance();
                aend->set_next_instance(new ainst_t(tok, &inst_src_defs));
                aend = aend->next_instance();
            }
            tok = 0;
        }
        else if (matching("-ctop")) {
            if (!send) {
                delete [] tok;
                Errs()->add_error("Error: -ctop not in -i/-i- scope.");
                return (false);
            }
            if (!send->instances()) {
                aend = new ainst_t(0, &inst_src_defs);
                send->set_instances(aend);
            }
            else {
                aend = send->instances();
                while (aend->next_instance())
                    aend = aend->next_instance();
                aend->set_next_instance(new ainst_t(0, &inst_src_defs));
                aend = aend->next_instance();
            }
        }
        else if (matching("-ca")) {
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-ca");
                return (false);
            }
            if (!aend) {
                delete [] tok;
                Errs()->add_error("Error: -ca not in -c|-ctop/-c- scope.");
                return (false);
            }
            aend->set_placename(tok);
        }
        else if (matching("-c-")) {
            if (!aend) {
                delete [] tok;
                Errs()->add_error("Error: -c- does not follow -c/-ctop.");
                return (false);
            }
            aend = 0;
        }
        else if (matching("-x")) {
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-x");
                return (false);
            }
            double x;
            if (sscanf(tok, "%lf", &x) != 1) {
                delete [] tok;
                Errs()->add_error("Error: bad or missing -x argument.");
                return (false);
            }
            if (aend)
                aend->set_pos_x(INTERNAL_UNITS(x));
            else if (send)
                inst_src_defs.set_pos_x(INTERNAL_UNITS(x));
            else
                inst_defs.set_pos_x(INTERNAL_UNITS(x));
        }
        else if (matching("-y")) {
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-y");
                return (false);
            }
            double y;
            if (sscanf(tok, "%lf", &y) != 1) {
                delete [] tok;
                Errs()->add_error("Error: bad or missing -y argument.");
                return (false);
            }
            if (aend)
                aend->set_pos_y(INTERNAL_UNITS(y));
            else if (send)
                inst_src_defs.set_pos_y(INTERNAL_UNITS(y));
            else
                inst_defs.set_pos_y(INTERNAL_UNITS(y));
        }
        else if (matching("-ang")) {
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-ang");
                return (false);
            }
            double ang;
            if (sscanf(tok, "%lf", &ang) != 1) {
                delete [] tok;
                Errs()->add_error("Error: bad or missing -ang argument.");
                return (false);
            }
            if (aend)
                aend->set_angle(ang);
            else if (send)
                inst_src_defs.set_angle(ang);
            else
                inst_defs.set_angle(ang);
        }
        else if (matching("-m")) {
            // Magnification factor for cell placements.
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-m");
                return (false);
            }
            double magn;
            if (sscanf(tok, "%lf", &magn) != 1 || magn < 1e-3 || magn > 1e3) {
                delete [] tok;
                Errs()->add_error("Error: bad or missing -m argument.");
                return (false);
            }
            if (aend)
                aend->set_magn(magn);
            else if (send)
                inst_src_defs.set_magn(magn);
            else
                inst_defs.set_magn(magn);
        }
        else if (matching("-s")) {
            // Scale factor for cell definitions.
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-s");
                return (false);
            }
            double sc;
            if (sscanf(tok, "%lf", &sc) != 1 || sc < 1e-3 || sc > 1e3) {
                delete [] tok;
                Errs()->add_error("Error: bad or missing -s argument.");
                return (false);
            }
            if (aend)
                aend->set_scale(sc);
            else if (send)
                inst_src_defs.set_scale(sc);
            else
                inst_defs.set_scale(sc);
        }
        else if (matching("-my")) {
            if (aend)
                aend->set_reflc(true);
            else if (send)
                inst_src_defs.set_reflc(true);
            else
                inst_defs.set_reflc(true);
        }
        else if (matching("-my-")) {
            if (aend)
                aend->set_reflc(false);
            else if (send)
                inst_src_defs.set_reflc(false);
            else
                inst_defs.set_reflc(false);
        }
        else if (matching("-h")) {
            if (aend)
                aend->set_no_hier(true);
            else if (send)
                inst_src_defs.set_no_hier(true);
            else
                inst_defs.set_no_hier(true);
        }
        else if (matching("-h-")) {
            if (aend)
                aend->set_no_hier(false);
            else if (send)
                inst_src_defs.set_no_hier(false);
            else
                inst_defs.set_no_hier(false);
        }
        else if (matching("-e") || matching("-e1")) {
            if (aend)
                aend->set_ecf_level(ECFall);
            else if (send)
                inst_src_defs.set_ecf_level(ECFall);
            else
                inst_defs.set_ecf_level(ECFall);
        }
        else if (matching("-e2")) {
            if (aend)
                aend->set_ecf_level(ECFpre);
            else if (send)
                inst_src_defs.set_ecf_level(ECFpre);
            else
                inst_defs.set_ecf_level(ECFpre);
        }
        else if (matching("-e3")) {
            if (aend)
                aend->set_ecf_level(ECFpost);
            else if (send)
                inst_src_defs.set_ecf_level(ECFpost);
            else
                inst_defs.set_ecf_level(ECFpost);
        }
        else if (matching("-e-") || matching("-e0")) {
            if (aend)
                aend->set_ecf_level(ECFnone);
            else if (send)
                inst_src_defs.set_ecf_level(ECFnone);
            else
                inst_defs.set_ecf_level(ECFnone);
        }
        else if (matching("-f")) {
            if (aend)
                aend->set_flatten(true);
            else if (send)
                inst_src_defs.set_flatten(true);
            else
                inst_defs.set_flatten(true);
        }
        else if (matching("-f-")) {
            if (aend)
                aend->set_flatten(false);
            else if (send)
                inst_src_defs.set_flatten(false);
            else
                inst_defs.set_flatten(false);
        }
        else if (matching("-w")) {
            delete [] tok;
            tok = lstring::getqtok(&string);
            if (!tok) {
                Errs()->add_error(msg, "-w");
                return (false);
            }
            double l, b, r, t;
            if (sscanf(tok, "%lf,%lf,%lf,%lf", &l, &b, &r, &t) != 4) {
                delete [] tok;
                Errs()->add_error("Error: can't parse -w argument.");
                return (false);
            }
            BBox BB(INTERNAL_UNITS(l), INTERNAL_UNITS(b), INTERNAL_UNITS(r),
                INTERNAL_UNITS(t));
            BB.fix();
            if (aend) {
                aend->set_use_win(true);
                aend->set_winBB(&BB);
            }
            else if (send) {
                inst_src_defs.set_use_win(true);
                inst_src_defs.set_winBB(&BB);
            }
            else {
                inst_defs.set_use_win(true);
                inst_defs.set_winBB(&BB);
            }
        }
        else if (matching("-cl")) {
            if (aend)
                aend->set_clip(true);
            else if (send)
                inst_src_defs.set_clip(true);
            else
                inst_defs.set_clip(true);
        }
        else if (matching("-cl-")) {
            if (aend)
                aend->set_clip(false);
            else if (send)
                inst_src_defs.set_clip(false);
            else
                inst_defs.set_clip(false);
        }
        else {
            Errs()->add_error("Error: unknown token %s.", tok);
            delete [] tok;
            return (false);
        }
        delete [] tok;
    }
    if (!j_outfile) {
        Errs()->add_error("Error: no output file name given.");
        return (false);
    }
    if (!j_srcfiles) {
        Errs()->add_error("Error: no source file names given.");
        return (false);
    }
    return (true);
}


// Do the job.  This is typically called after parse() to implement
// a simple command.
//
bool
ajob_t::run(const char *logfname)
{
    const char *logf = j_logfile;
    if (!logf || !*logf)
        logf = logfname;
    if (!logf || !*logf)
        logf = DEF_LOGFILE;

    if (!open_stream())
        return (false);
    FILE *fp = FIO()->ifInitCvLog(logf);
    bool ret = true;
    for (asource_t *sf = j_srcfiles; sf; sf = sf->next_source()) {
        FIO()->ifPrintCvLog(IFLOG_INFO, "Streaming: %s\n", sf->path());
        if (!write_stream(sf)) {
            FIO()->ifPrintCvLog(IFLOG_FATAL,
                "Streaming terminated abnormally.\n%s\n",
                Errs()->get_error());
            ret = false;
            break;
        }
    }
    if (!finish_stream()) {
        FIO()->ifPrintCvLog(IFLOG_FATAL, "finish_stream failed.\n%s\n",
            Errs()->get_error());
        ret = false;
    }
    FIO()->ifShowCvLog(fp, ret ? OIok : OIerror, logf);
    return (ret);
}


bool
ajob_t::dump(FILE *fp)
{
    if (!fp)
        return (false);
    if (j_outfile)
        fprintf(fp, "OutFile %s\n", j_outfile);
    if (j_topcell)
        fprintf(fp, "TopCell %s\n", j_topcell);
    for (asource_t *src = j_srcfiles; src; src = src->next_source()) {
        fprintf(fp, "Source %s\n", src->path());
        if (src->layer_list()) {
            fprintf(fp, "LayerList %s\n", src->layer_list());
            if (src->only_layers())
                fprintf(fp, "OnlyLayers\n");
            else if (src->skip_layers())
                fprintf(fp, "SkipLayers\n");
        }
        if (src->layer_aliases())
            fprintf(fp, "LayerAliases %s\n", src->layer_aliases());
        if (src->scale() != 1.0)
            fprintf(fp, "ConvertScale %1.5f\n", src->scale());
        if (src->prefix())
            fprintf(fp, "CellNamePrefix %s\n", src->prefix());
        if (src->suffix())
            fprintf(fp, "CellNameSuffix %s\n", src->suffix());
        if (src->to_lower())
            fprintf(fp, "ToLower\n");
        if (src->to_upper())
            fprintf(fp, "ToUpper\n");
        for (ainst_t *inst = src->instances(); inst;
                inst = inst->next_instance()) {
            if (inst->placename()) {
                if (inst->cellname())
                    fprintf(fp, "Place %s %s\n", inst->cellname(),
                        inst->placename());
                else
                    fprintf(fp, "PlaceTop %s\n", inst->placename());
            }
            else {
                if (inst->cellname())
                    fprintf(fp, "Place %s\n", inst->cellname());
                else
                    fprintf(fp, "PlaceTop\n");
            }
            fprintf(fp, "Translate %1.4f %1.4f\n", MICRONS(inst->pos_x()),
                MICRONS(inst->pos_y()));
            if (inst->angle() != 0.0)
                fprintf(fp, "Rotate %d\n", (int)inst->angle());
            if (inst->magn() != 1.0)
                fprintf(fp, "Magnify %1.5f\n", inst->magn());
            if (inst->scale() != 1.0)
                fprintf(fp, "Scale %1.5f\n", inst->scale());
            if (inst->reflc())
                fprintf(fp, "Reflect\n");
            if (inst->no_hier())
                fprintf(fp, "NoHier\n");
            if (inst->ecf_level() == ECFall)
                fprintf(fp, "NoEmpties\n");
            else if (inst->ecf_level() == ECFpre)
                fprintf(fp, "NoEmpties 2\n");
            else if (inst->ecf_level() == ECFpost)
                fprintf(fp, "NoEmpties 3\n");
            if (inst->flatten())
                fprintf(fp, "Flatten\n");
            if (inst->use_win())
                fprintf(fp, "Window %1.4f %1.4f %1.4f %1.4f\n",
                    MICRONS(inst->winBB()->left),
                    MICRONS(inst->winBB()->bottom),
                    MICRONS(inst->winBB()->right),
                    MICRONS(inst->winBB()->top));
            if (inst->clip())
                fprintf(fp, "Clip\n");
        }
    }
    return (true);

}
// End of ajob_t functions.


// Set the chd, if it matches, and return true.
//
bool
asource_t::set_match_chd(cCHD *c)
{
    if (!c)
        return (false);
    if (!s_path || !*s_path || strcmp(s_path, c->filename()))
        return (false);
    const char *cpfx = c->aliasInfo()->prefix();
    if (!s_prefix || !*s_prefix) {
        if (cpfx && *cpfx)
            return (false);
    }
    else {
        if (!cpfx || !*cpfx)
            return (false);
        if (strcmp(s_prefix, cpfx))
            return (false);
    }
    const char *csfx = c->aliasInfo()->suffix();
    if (!s_suffix || !*s_suffix) {
        if (csfx && *csfx)
            return (false);
    }
    else {
        if (!csfx || !*csfx)
            return (false);
        if (strcmp(s_suffix, csfx))
            return (false);
    }
    if (s_tolower != c->aliasInfo()->to_lower())
        return (false);
    if (s_toupper != c->aliasInfo()->to_upper())
        return (false);

    s_chd = c;
    return (true);
}

