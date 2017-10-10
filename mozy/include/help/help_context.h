
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

#ifndef HELP_CONTEXT_H
#define HELP_CONTEXT_H

//#include "help_topic.h"
#include "ginterf/fontutil.h"

#include <string.h>

class Transaction;
struct ali_t;
struct HLPauthList;
struct HLPcache;
struct HLPparams;
struct HLPtopic;
struct htmImageInfo;
struct htmPLCStream;
struct htmFormCallbackStruct;

// Default colors.
#define HLP_DEF_BG_COLOR    "white smoke"
#define HLP_DEF_FG_COLOR    "black"
#define HLP_DEF_LN_COLOR    "blue"
#define HLP_DEF_VL_COLOR    "steel blue"
#define HLP_DEF_AL_COLOR    "red"
#define HLP_DEF_SL_COLOR    "pale turquoise"
#define HLP_DEF_IM_COLOR    "blue"

// Default color keywords.
#define HLP_DefaultBgImage          "DefaultBgImage"
#define HLP_DefaultBgColor          "DefaultBgColor"
#define HLP_DefaultFgText           "DefaultFgText"
#define HLP_DefaultFgLink           "DefaultFgLink"
#define HLP_DefaultFgVisitedLink    "DefaultFgVisitedLink"
#define HLP_DefaultFgActiveLink     "DefaultFgActiveLink"
#define HLP_DefaultBgSelect         "DefaultBgSelect"
#define HLP_DefaultFgImagemap       "DefaultFgImagemap"

// Base of idle controller that will call a procedure
// (HLPcontext::processList in our case) repeatedly when the
// application's event loop is idle, until the procedure returns
// false.
//
struct QueueLoop
{
    virtual ~QueueLoop() { }

    virtual void start() = 0;
        // Start calling the procedure repeatedly when application's
        // event loop is idle.
    virtual void suspend() = 0;
        // Suspend calling the procedure.  It may be necessary to use
        // this to avoid blocking.
    virtual void resume() = 0;
        // Restart after a suspension.
};

// This is a toolkit-independent base class for a basic html display
// widget.  This should be implemented in the viewer to provide an
// interface to the image retrieval and caching functionality
// implemented in the HLPcontext class.
//
struct ViewerWidget
{
    virtual ~ViewerWidget() { }

    //--- display control
    virtual void freeze() = 0;
        // Prevent redisplays, increment a "freeze count".

    virtual void thaw() = 0;
        // Decrement freeze count, redisplay if the freeze count hits zero.

    //--- download control
    virtual void set_transaction(Transaction*, const char*) = 0;
        // Save an internal pointer to the Transaction struct passed,
        // to be retrieved later (used for http/ftp downloading).
        // The second arg is a directory path for the cookies file.
        // When called with nonzero first arg, setup params can be
        // applied to the Transaction pointer.  If the first arg is
        // null, the second arg is ignored, and the internal pointer
        // is cleared.

    virtual Transaction *get_transaction() = 0;
        // Return a pointer to the Transaction struct saved previously.

    virtual bool check_halt_processing(bool) = 0;
        // Check for interrupts or button presses that indicate that
        // processing should be stopped, return true if so.  If the
        // argument is true, the application should process any
        // pending window events.  If false, window events are not
        // checked.

    virtual void set_halt_proc_sens(bool) = 0;
        // Set the sensitivity of buttons that stop processing, if
        // any.  This is called with arg true when downloading begins,
        // and again with arg false when finished.

    virtual void set_status_line(const char*) = 0;
        // Set the text into the viewer's status line (if there is one)
        // or otherwise dispatch the message.

    //--- image support
    virtual htmImageInfo *new_image_info(const char*, bool) = 0;
        // Create a new htmImageInfo for progressive (second arg true)
        // or delayed loading, for the url given in the first arg.

    virtual bool call_plc(const char*) = 0;
        // Progressive image loading support.  Read a chunk of the
        // image whose url is passed as an argument.  The viewer
        // should update the display of the image.

    virtual htmImageInfo *image_procedure(const char*) = 0;
        // This function takes the file path argument to an image file
        // and returns an htmImageInfo struct describing the image.
        // The image is added to the viewer's display list.

    virtual void image_replace(htmImageInfo*, htmImageInfo*) = 0;
        // Replace an image already in the viewer (first arg) with a
        // different one (second arg).

    virtual bool is_body_image(const char*) = 0;
        // Return true if the url passed is that of the body image of
        // the current page.

    virtual const char *get_url() = 0;
        // Return the url currently being displayed (or null).  If the
        // topic support is included, this is just the topic's keyword.

    virtual bool no_url_cache() = 0;
        // Return true if we should not use the url cache.

    virtual int image_load_mode() = 0;
        // Return the image loading mode (HLPparams::LImode).

    virtual int image_debug_mode() = 0;
        // Return the image debugging mode (HLPparams::LoadMode).

    //--- misc functions
    virtual GRwbag *get_widget_bag() = 0;
        // Return a widget bag for use with the pop-up functions
        // such as PopUpErr.  Returns null if no bag is available.
};

// This includes additional functionality, including interface to the
// help database.
//
struct HelpWidget : public ViewerWidget
{
    virtual ~HelpWidget() { }

    //--- help topic display
    virtual void link_new(HLPtopic*) = 0;
        // Display a new topic.

    virtual void reuse(HLPtopic*, bool) = 0;
        // Reuse the current window to display the passed topic.  If the
        // second arg is true, link it into the topic list.

    //--- display control
    virtual void redisplay() = 0;
        // Redisplay the currently sourced text.

    //--- info retrieval
    virtual HLPtopic *get_topic() = 0;
        // Return a pointer to the current topic (or null).

    //--- processing control
    virtual void unset_halt_flag() = 0;
        // Unset any internal flags that indicate that processing should
        // be halted.

    //--- image support
    virtual void halt_images() = 0;
        // Halt all image loading in progress.

    //--- url cache
    virtual void show_cache(int) = 0;
        // Display a listing of the urls in the cache.  The argument
        // is MODE_ON, MODE_UPD, MODE_OFF to pop-up the list, update
        // the list, dismiss the list.  Implementation is obviously
        // optional.

    //--- static functions
    static HelpWidget *get_widget(HLPtopic*);
        // Function to return a pointer to this interface from a topic.
        // If topic support, return a casted pointer to topic->devdep.

    static HelpWidget *new_widget(GRwbag**, int = -1, int = -1);
        // Function to return a new widget, simply a call to the
        // constructor.
};

// List of image files that are in the process of being downloaded.  This
// can be subclassed and used by the application.
//
struct HLPimageList
{
    // Status flag values
    enum IMstatus { IMnone, IMtodo, IMinprogress, IMdone, IMabort };

    HLPimageList(ViewerWidget*, const char*, const char*, htmImageInfo*,
        IMstatus, HLPimageList*);

    ~HLPimageList()
        {
            delete [] filename;
            delete [] url;
            delete [] orig_url;
            // delete tmp_image;
        }

    bool operator==(HLPimageList &i)
        {
            if (widget == i.widget) {
                if (url && i.url && !strcmp(url, i.url))
                    return (true);
                if (filename && i.filename && !strcmp(filename, i.filename))
                    return (true);
            }
            return (false);
        }

    ViewerWidget *widget;   // widget pointer
    HLPimageList *next;     // list link
    char *filename;         // cache filename for image
    char *url;              // url for image (might be relocated)
    char *orig_url;         // original url for image
    htmImageInfo *tmp_image;// data storage
    IMstatus status;        // current or final status
    bool load_prog;         // use progressive loading
    bool start_prog;        // progressive loading has started
    bool local_image;       // image is local
    bool inactive;          // done with this
};

// Bookmark entry.
//
struct HLPbookMark
{
    HLPbookMark(char *u, char *t) { url = u; title = t; next = 0; }
    ~HLPbookMark() { delete [] url; delete [] title; }

    static void destroy(HLPbookMark *b)
        {
            while (b) {
                HLPbookMark *bx = b;
                b = b->next;
                delete bx;
            }
        }

    char *url;
    char *title;
    HLPbookMark *next;
};

struct strtab_t;

// Global context for help system.
//
class HLPcontext
{
public:
    HLPcontext();

    bool no_cache(ViewerWidget *w) { return (w && w->no_url_cache()); }

    // Callback registration, exit help mode procedure
    //
    void registerQuitHelpProc(void(*callback)(void*))
        {
            hcxQuitProc = callback;
        }

    // Callback registration, local form processing procedure
    //
    void registerFormSubmitProc(void(*callback)(void*))
        {
            hcxSubmitProc = callback;
        }

    void quitHelp();

    // URL Cache Control
    void clearCache();
    void reloadCache();
    stringlist *listCache();

    // Visited URL Table
    void addVisited(const char*);
    void clearVisited();
    bool isVisited(const char*);

    // Font Initialization
    void setFont(const char*);
    void setFixedFont(const char*);

    // Color Initialization
    void setDefaultColor(const char*, const char*);
    const char *getDefaultColor(const char*);
    char *getBodyTag();

    // Topic List Maintenance
    void linkNewTopic(HLPtopic*);
    bool removeTopic(HLPtopic*);
    HLPtopic *findUrlTopic(const char*);
    HLPtopic *findKeywordTopic(const char*);
    bool isCurrent(const char*);

    // Resolve Keyword/URL
    bool resolveKeyword(const char*, HLPtopic**, char*, HelpWidget*, HLPtopic*,
        bool, bool);

    // Network Access
    FILE *httpRetrieve(char**, char**, char*, ViewerWidget*, bool,
        HLPimageList* = 0);
    bool checkLocation(char**, FILE*, ViewerWidget*);
    void download(ViewerWidget*, char*);
    void download(ViewerWidget*, stringlist*, bool);

    // Image Utilities
    bool isImage(const char*);
    HLPtopic *checkImage(const char*, ViewerWidget*);

    // Image Download
    void flushImages(ViewerWidget*);
    void abortImageDownload(ViewerWidget*);
    void inactivateImages(HLPimageList*);
    bool isImageInList(ViewerWidget*);
    htmImageInfo *imageResolve(const char*, ViewerWidget*);
    htmImageInfo *httpGetImage(char**, char**, char*, ViewerWidget*);
    htmImageInfo *localProgTest(ViewerWidget*, const char*);
    int getImageData(HLPimageList*, htmPLCStream*, void*);
    bool processList();
    void startImageDownload(HLPimageList*);
    void loadImages(HLPimageList*);

    // URL Utilities
    bool isPlain(const char*);
    bool isProtocol(const char*);
    char *getProtocol(const char*, const char**);
    char *hrefExpand(char*);
    const char *findAnchorRef(const char*);
    char *findAnchorRef(char*);
    char *urlcat(const char*, const char*);
    void setRootAlias(const char*);
    int dumpRootAliases(FILE*);
    char *rootAlias(char*);
    HLPtopic *findFile(const char*, HLPtopic*, bool);
    char *findFileInPath(const char*);
    HLPtopic *checkAuth(FILE*, char*);

    // Bookmark Maintenance
    void readBookmarks();
    HLPbookMark *bookmarkUpdate(const char*, const char*);

    // Forms Handling
    void formProcess(htmFormCallbackStruct*, HelpWidget*);
    void submit(void *s) { if (hcxSubmitProc) (*hcxSubmitProc)(s); }
    bool hasSubmitProc() { return (hcxSubmitProc); }

    bool haveRootAlias()            { return (hcxRootAlii != 0); }

    // Access to Private Members
    HLPbookMark *bookmarks()        { return (hcxBookmarks); }
    HLPtopic *topList()             { return (hcxTopList); }
    HLPimageList *imageList()       { return (hcxImageList); }
    bool isFontSet()                { return (hcxFontSet); }
    void setFontSet(bool b)         { hcxFontSet = b; }
    bool isFixedFontSet()           { return (hcxFixedFontSet); }
    void setFixedFontSet(bool b)    { hcxFixedFontSet = b; }
    static void imageLoopStart()    { hcxImageQueueLoop.start(); }

private:
    HLPcache *hcxCache;             // The url cache.
    HLPbookMark *hcxBookmarks;      // List of bookmarks.
    HLPauthList *hcxAuthList;       // List if authorization passwords for
                                    // protected web pages.

    strtab_t *hcxVisitedTab;        // Visited URL table.

    HLPtopic *hcxTopList;           // List of "top level" topics on view,
                                    // eash is associated with a window.

    HLPimageList *hcxImageList;     // List of images being downloaded.
    HLPimageList *hcxImageFreeList; // Temporary list of freed images.
    HLPimageList *hcxLocalTest;     // For diagnositcs.

    char *hcxBGimage;               // Default image.
    char *hcxBGcolor;               // Default help background.
    char *hcxFGcolor;               // Default help text color.
    char *hcxLNcolor;               // Default help link color.
    char *hcxVLcolor;               // Default help visited link color.
    char *hcxALcolor;               // Default help activated link color.

    ali_t *hcxRootAlii;             // Path root aliases for local redir.

    void (*hcxQuitProc)(void*);     // Callback to application when quitting
                                    // help.  The exits "help mode" when the
                                    // last window is destroyed.

    void (*hcxSubmitProc)(void*);   // Callback to intercept form return data
                                    // for use in application.

    int hcxWindowCnt;               // Count the created windows to apply
                                    // crude positioning.

    bool hcxFontSet;                // Remember if fonts were initialized.
    bool hcxFixedFontSet;

    static QueueLoop &hcxImageQueueLoop;
                                    // Image queue processing controller.
};

#endif

