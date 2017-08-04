
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
 * Help System Files                                                      *
 *                                                                        *
 *========================================================================*
 $Id:$
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

