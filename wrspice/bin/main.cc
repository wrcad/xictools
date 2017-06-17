
/*========================================================================*
 *                                                                        *
 *  XICTOOLS Integrated Circuit Design System                             *
 *  Copyright (c) 1996 Whiteley Research Inc, all rights reserved.        *
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
 *========================================================================*
 *                                                                        *
 * Whiteley Research Circuit Simulation and Analysis Tool                 *
 *                                                                        *
 *========================================================================*
 $Id: main.cc,v 2.1 2011/12/13 04:24:01 stevew Exp $
 *========================================================================*/

#include <windows.h>
#include <stdio.h>

// If we want to use plugins under Windows, we link WRspice as a DLL,
// and use this stub to run the program.  The DLL is used when linking
// plugins.


typedef int(*mainfunc)(int, char**);

int main(int argc, char **argv)
{
    // The wrspice.dll has the main function, still called "main". 
    // Have to find its address and jump to it.

    HMODULE h = LoadLibrary("wrspice.dll");
    if (!h) {
        fprintf(stderr, "Failed to load the WRspice DLL.\n");
        return (1);
    }
    mainfunc p = (mainfunc)GetProcAddress(h, "main");
    if (!p) {
        fprintf(stderr,
            "Failed to find the main function in the WRspice DLL.\n");
        return (1);
    }
    return ((*p)(argc, argv));
}

