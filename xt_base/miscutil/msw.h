
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
 $Id: msw.h,v 1.8 2015/05/06 14:45:12 stevew Exp $
 *========================================================================*/

//
// General Microsoft Windows support functions.
//

#ifndef MSW_H
#define MSW_H
#ifdef WIN32

#include <winsock2.h>
#include <process.h>
#include <stdio.h>

namespace msw {
    bool IsWinNT();
    char *GetInstallDir(const char*);
    char *GetProgramRoot(const char*);
    bool GetProductID(char*, FILE*);
    char *AddPathDrives(const char*, const char*);
    PROCESS_INFORMATION *NewProcess(const char*, unsigned int, bool,
        void* = 0, void* = 0, void* = 0);
    const char *MapiSend(const char*, const char*, const char*, int = 0,
        const char** = 0);
    char *Billize(const char*);
    void UnBillize(char*);
    int ListPrinters(int*, char***);
    const char *RawFileToPrinter(const char*, const char*);

    // This must be defined in the application, and provides a suffix
    // used when creating application names as known in the registry.
    //
    extern const char *MSWpkgSuffix;
}

#endif
#endif

