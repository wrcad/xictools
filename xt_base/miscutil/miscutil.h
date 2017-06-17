
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2005 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *                                                                        *
 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,      *
 *   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES      *
 *   OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-        *
 *   INFRINGEMENT.  IN NO EVENT SHALL STEPHEN R. WHITELEY BE LIABLE       *
 *   FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION      *
 *   OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN           *
 *   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN         *
 *   THE SOFTWARE.                                                        *
 *                                                                        *
 * This library is available for non-commercial use under the terms of    *
 * the GNU Library General Public License as published by the Free        *
 * Software Foundation; either version 2 of the License, or (at your      *
 * option) any later version.                                             *
 *                                                                        *
 * A commercial license is available to those wishing to use this         *
 * library or a derivative work for commercial purposes.  Contact         *
 * Whiteley Research Inc., 456 Flora Vista Ave., Sunnyvale, CA 94086.     *
 * This provision will be enforced to the fullest extent possible under   *
 * law.                                                                   *
 *                                                                        *
 * This library is distributed in the hope that it will be useful,        *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of         *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU      *
 * Library General Public License for more details.                       *
 *                                                                        *
 * You should have received a copy of the GNU Library General Public      *
 * License along with this library; if not, write to the Free             *
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.     *
 *========================================================================*
 *                                                                        *
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id: miscutil.h,v 1.10 2015/05/06 16:12:27 stevew Exp $
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

        void free()
            {
                ifc_t *ifc = this;
                while (ifc) {
                    ifc_t *ix = ifc;
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

