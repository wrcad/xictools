
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef MISCUTIL_H
#define MISCUTIL_H


namespace miscutil
{
    // Network interfaces list element.
    //
    struct ifc_t
    {
        ifc_t(char *n, char *i, char *h)
            {
                ifc_next = 0;
                ifc_name = n;
                ifc_ip = i;
                ifc_hw = h;
            }

        ~ifc_t()
            {
                delete [] ifc_name;
                delete [] ifc_ip;
                delete [] ifc_hw;
            }

        static void destroy(const ifc_t *ifc)
            {
                while (ifc) {
                    const ifc_t *ix = ifc;
                    ifc = ifc->ifc_next;
                    delete ix;
                }
            }

        ifc_t *next()           { return (ifc_next); }
        void set_next(ifc_t *n) { ifc_next = n; }
        const char *name()      { return (ifc_name); }
        const char *ip()        { return (ifc_ip); }
        void set_ip(char *i)    { delete [] ifc_ip; ifc_ip = i; }
        const char *hw()        { return (ifc_hw); }
        void set_hw(char *h)    { delete [] ifc_hw; ifc_hw = h; }

    private:
        ifc_t *ifc_next;
        char *ifc_name;
        char *ifc_ip;
        char *ifc_hw;
    };

    const char *dateString();
    ifc_t *net_if_list();
    bool send_mail(const char*, const char*, const char*, const char* = 0);
    int fork_terminal(const char*);

#define GDB_OFILE "gdbout"
    bool dump_backtrace(const char*, const char*, const char*, const void*);
}

#endif

