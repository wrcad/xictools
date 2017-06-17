
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
 * Help System Files                                                      *
 *                                                                        *
 *========================================================================*
 $Id: help_startup.h,v 1.5 2013/06/08 04:05:14 stevew Exp $
 *========================================================================*/

#ifndef HELP_STARTUP_H
#define HELP_STARTUP_H

// These are the defaults of various configuration parameters for
// the help viewer widget.
//
struct HLPparams
{
    enum AnchorLinestyle
    {
        AnchorNoLine,
        AnchorSingleLine,
        AnchorDoubleLine,
        AnchorDashedLine,
        AnchorDoubleDashedLine
    };

    // How to deal with images from http source that are not in cache:
    //  (Images that are local are loaded synchronously)
    //  LoadNone         ignore them
    //  LoadSync         load when encountered
    //  LoadDelayed      load after document displayed
    //  LoadProgressive  Load progressively after document displayed
    enum LoadType { LoadNone, LoadSync, LoadDelayed, LoadProgressive };

    // How local images are loaded.  Normally, they are loaded when
    // needed, the other modes are for debugging.
    //
    enum LImode { LInormal, LIdelayed, LIprogressive };

    HLPparams(bool);
    void parse_startup_file();
    void update();
    bool dump();

    LoadType LoadMode;          // image loading mode
    int AnchorUnderlineType;    // one of
                                //  ANCHOR_NOLINE
                                //  ANCHOR_DASHED_LINE
                                //  ANCHOR_DOUBLE_LINE
                                //  ANCHOR_DOUBLE_DASHED_LINE
                                //  ANCHOR_SINGLE_LINE
    bool AnchorUnderlined;      // underline the anchor
    bool AnchorButtons;         // use buttons for anchors
    bool AnchorHighlight;       // highlight on enter
    bool FreezeAnimations;      // stop animations
    int  Timeout;               // seconds to wait for message response
    int  Retries;               // number of retries after a timeout
    int  HTTP_Port;             // port number for HTTP transactions
    int  FTP_Port;              // port number for FTP transactions
    int  CacheSize;             // number of files to save in cache
    bool NoCache;               // disable cache
    bool NoCookies;             // disable cookies
    bool DebugMode;             // extra status messages when set
    bool PrintTransact;         // dump comm bytes to screen
    bool BadHTMLwarnings;       // issue warnings about bad HTML
    LImode LocalImageTest;      // enable image debugging
    char *SourceFilePath;       // path to existing file

    bool ignore_fonts;          // don't read/write font fields
};

#endif

