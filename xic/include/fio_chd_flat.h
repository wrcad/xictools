
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
 $Id: fio_chd_flat.h,v 5.16 2010/05/09 00:17:47 stevew Exp $
 *========================================================================*/

#ifndef FIO_CHD_FLAT_H
#define FIO_CHD_FLAT_H

struct Layer;
struct Text;


// Class for estimating memory requirements for flat_read.
//
struct fmu_t
{
    fmu_t(cCHD*, symref_t*);
    ~fmu_t();

    bool est_flat_memory_use(BBox*, double*);

private:
    bool find_totals();
    bool fmu_core_rc(symref_t*, cTfmStack*);
    bool sum_counts(double, symref_t*);

public:
    BBox AOI;       // area of interest, context set to this area
    uint64_t box_cnt;           // final box count
    uint64_t wire_cnt;          // final wire count
    uint64_t poly_cnt;          // final poly count
    uint64_t vtex_cnt;          // final vertex count

private:
    cCHD *fchd;                 // pointer to context
    symref_t *ftop;             // pointer to top symbol
    SymTab *ftab;               // symref to number table
};

#endif

