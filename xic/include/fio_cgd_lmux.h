
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

#ifndef FIO_CGD_LMUX_H
#define FIO_CGD_LMUX_H


//
// CGD accessories, with OASIS dependency.
//

// Element holds temp file data for layer geometry stream, for
// geometry multiplexer.
//
struct lm_t
{
    const char *tab_name()      { return (layername); }
    void set_tab_name(const char *n) { layername = n; }
    lm_t *tab_next()            { return (next); }
    void set_tab_next(lm_t *t)  { next = t; }
    lm_t *tgen_next(bool)       { return (next); }

    lm_t *next;
    const char *layername;
    accum_t *accum;
    oas_modal modal;
};


// Geometry stream multiplexer for OASIS writer.  For each cell, directs
// geometry into a separate stream for each layer.
//
struct cgd_layer_mux
{
    cgd_layer_mux(const char *cname, cCGD *db)
        {
            lm_cellname = lstring::copy(cname);
            lm_cgd = db;
            lm_accum = 0;
            lm_table = 0;
        }

    ~cgd_layer_mux();

    oas_modal *set_layer(int, int);
    void finalize();
    bool grab_results();

    bool put_byte(int);

private:
    char *lm_cellname;
    cCGD *lm_cgd;
    accum_t *lm_accum;
    table_t<lm_t> *lm_table;
    eltab_t<lm_t> lm_eltab;
};


// Interface to zlib decompressor.  If the bool arg is true, the string
// passed as the first arg will be freed in the destructor.  The optional
// last arg is an offset into the data where reading starts.
//
struct bstream_t : public oas_byte_stream
{
    bstream_t(const unsigned char*, size_t, size_t, bool=false, int offs=0);
    virtual ~bstream_t()
        {
            if (b_csize) {
                inflateEnd(&b_stream);
                delete [] b_buf;
            }
            if (b_freedata)
                delete [] b_data;
        }

    bool done()     const { return (b_nbytes == b_usize); }
    bool error()    const { return (b_errflg); }

    int get_byte();

private:
    z_stream b_stream;
    const unsigned char *b_buf;
    const unsigned char *b_data;
    size_t b_csize;
    size_t b_usize;
    size_t b_nbytes;
    int b_offs;
    bool b_errflg;
    bool b_freedata;
};

#endif

