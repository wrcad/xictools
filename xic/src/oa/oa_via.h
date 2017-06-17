
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2012 Whiteley Research Inc, all rights reserved.        *
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
 $Id: oa_via.h,v 1.2 2015/03/16 04:26:30 stevew Exp $
 *========================================================================*/

#ifndef OA_VIA_H
#define OA_VIA_H


//
// Elements for a via database, used when reading an OA hierarchy.
//

class cOAvia
{
public:
    static char *getStdViaString(const oaStdViaHeader*);
    static bool parseStdViaString(const char*, oaViaParam&);
};


// Instantiation descriptor.  We keep a list of these, one for each
// unique set of instantiation parameters, for a given super master.
//
struct ViaItem
{
    friend struct ViaDesc;

    ViaItem(const oaViaParam &pm)
        {
            vi_next = 0;
            vi_params = new oaViaParam;
            *vi_params = pm;
            vi_index = 0;
        }

    ~ViaItem()
        {
            delete vi_params;
        }

    void free()
        {
            ViaItem *v0 = this;
            while (v0) {
                ViaItem *vx = v0;
                v0 = v0->vi_next;
                delete vx;
            }
        }

private:
    ViaItem *vi_next;
    oaViaParam *vi_params;
    unsigned int vi_index;
};


// Descriptor for a unique library/name, saved as data in a hash
// table.
//
struct ViaDesc
{
    ViaDesc(const char *nm)
        {
            vd_dbname = lstring::copy(nm);
            vd_instances = 0;
        }

    ~ViaDesc()
        {
            delete [] vd_dbname;
            vd_instances->free();
        }

    ViaItem *findItem(const oaViaParam &prms)
        {
            for (ViaItem *vi = vd_instances; vi; vi = vi->vi_next) {
                if (*vi->vi_params == prms)
                    return (vi);
            }
            return (0);
        }

    void addItem(ViaItem *vi)
        {
            if (!vd_instances) {
                vd_instances = vi;
                vi->vi_index = 1;
            }
            else {
                vi->vi_index = vd_instances->vi_index + 1;
                vi->vi_next = vd_instances;
                vd_instances = vi;
            }
        }

    char *cellname(const char *basename, const ViaItem *vi)
        {
            char buf[256];
            sprintf(buf, "%s_%u", basename, vi->vi_index);
            return (lstring::copy(buf));
        }

    const char *dbname()
        {
            return (vd_dbname);
        }

    oaViaParam *params(ViaItem *vi)
        {
            return (vi->vi_params);
        }

private:
    char *vd_dbname;
    ViaItem *vd_instances;

};

#endif

