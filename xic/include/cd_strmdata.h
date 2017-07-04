
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
 $Id: cd_strmdata.h,v 5.6 2014/11/06 06:01:08 stevew Exp $
 *========================================================================*/

#ifndef CD_STRMDATA_H
#define CD_STRMDATA_H

// These are the maximum number of layers/datatypes as per Cadence spec.
#define GDS_MAX_SPEC_LAYERS 256
#define GDS_MAX_SPEC_DTYPES 256

// These are the maximum number of layers/datatypes we support.  The
// GDSII file storage for layer and datatype is a 16-bit integer.
#define GDS_MAX_LAYERS 65536
#define GDS_MAX_DTYPES 65536

struct sLstr;

// Structure for stream input layer and datatype mapping.
//
struct strm_idata
{
    struct idata_list
    {
        idata_list(unsigned int n, unsigned int x)
            {
                next = 0;
                min = n;
                max = x;
            }

        static void destroy(idata_list *l)
            {
                while (l) {
                    idata_list *lx = l;
                    l = l->next;
                    delete lx;
                }
            }

        idata_list *next;
        unsigned int min;
        unsigned int max;
    };

    strm_idata()
        {
            si_layer_list = 0;
            si_dtype_list = 0;
        }

    ~strm_idata()
        {
            idata_list::destroy(si_layer_list);
            idata_list::destroy(si_dtype_list);
        }

    void print(FILE*, sLstr*);
    bool parse_lspec(const char**);
    bool check(unsigned int, unsigned int);
    void set_lspec(unsigned int, unsigned int, bool);

    void enable_all_dtypes()
        {
            set_lspec(0, GDS_MAX_DTYPES - 1, true);
        }

private:

    idata_list *si_layer_list;
    idata_list *si_dtype_list;
};

// Structure for stream output layer and datatype mapping.
//
struct strm_odata
{
    strm_odata()
        {
            so_next = 0;
            so_layer = 0;
            so_dtype = 0;
        }

    strm_odata(unsigned int l, unsigned int d)
        {
            so_next = 0;
            so_layer = l;
            so_dtype = d;
        }

    static void destroy(const strm_odata *sd)
        {
            while (sd) {
                const strm_odata *sx = sd;
                sd = sd->next();
                delete sx;
            }
        }

    strm_odata *next()          const { return (so_next); }
    void set_next(strm_odata *n)      { so_next = n; }
    unsigned int layer()        const { return (so_layer); }
    unsigned int dtype()        const { return (so_dtype); }

protected:
    strm_odata *so_next;
    unsigned int so_layer;
    unsigned int so_dtype;
};


namespace strmdata {

    // Return value of hex char passed.
    //
    inline int
    hexval(int c)
    {
        if (c >= '0' && c <= '9')
            return (c - '0');
        if (c >= 'A' && c <= 'F')
            return (c - 'A' + 10);
        if (c >= 'a' && c <= 'f')
            return (c - 'a' + 10);
        return (0);
    }

    // cd_strmdata.cc
    char *dec_to_hex(const char*);
    char *hex_to_dec(const char*);
    void hexpr(char*, int, int);
    bool hextrn(const char*, int*, int*);
}

#endif

