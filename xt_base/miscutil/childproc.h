
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 2010 Whiteley Research Inc, all rights reserved.        *
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
 $Id: childproc.h,v 1.2 2010/10/08 20:18:30 stevew Exp $
 *========================================================================*/

#ifndef CHILDPROC_H
#define CHILDPROC_H


// This centralizes the SIGCHLD handling.  Since there are many places
// in the application where child processes are created and monitored,
// we have to be careful that they don't clobber one another.  The
// application should not set handlers for SIGCHLD, but instead use
// the registration mechanism here.

struct sProc *Proc();

typedef void(*SigcHandlerFunc)(int, int, void*);

struct sProc
{
    friend sProc *Proc() { return (p_ptr); }

    // List of active children.
    //
    struct sSigC
    {
        sSigC(int p, SigcHandlerFunc h, void *d, sSigC *n)
            {
                pid = p;
                handler = h;
                data = d;
                next = n;
            }

        int pid;
        void *data;
        SigcHandlerFunc handler;
        sSigC *next;
    };

    sProc();

    void RegisterChildHandler(int, void(*)(int, int, void*), void*);
    void RemoveChildHandler(int, void(*)(int, int, void*));
    void SigchldHandler();

private:

    sSigC *p_siglist;

    static sProc *p_ptr;
};

#endif

