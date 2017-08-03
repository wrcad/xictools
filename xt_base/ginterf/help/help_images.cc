
/*========================================================================*
 *                                                                        *
 *  Whiteley Research Inc, Sunnyvale, CA USA, http://wrcad.com            *
 *  Copyright (C) 2017 Whiteley Research Inc, all rights reserved.        *
 *                                                                        *
 *  Licensed under the Apache License, Version 2.0 (the "License");       *
 *  you may not use this file except in compliance with the License.      *
 *  You may obtain a copy of the License at                               *
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

#include "help_defs.h"
#include "help_context.h"
#include "help_startup.h"
#include "help_cache.h"
#include "help_topic.h"
#include "httpget/transact.h"
#include "pathlist.h"

#include "htm/htm_widget.h"
#include "htm/htm_image.h"

#include <sys/stat.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>


//-----------------------------------------------------------------------------
// Image Utilities

// Return true if the first 8 bytes in data look like an image file.
//
bool
HLPcontext::isImage(const char *data)
{
    static unsigned char png_magic[8] = {137, 80, 78, 71, 13, 10, 26, 10};
    if (data[0] == '3' && data[1] == '0')
        // from Transaction package: "301/2 Location url"
        return (false);  // relocation return
    if (lstring::prefix("GIF", data) || lstring::prefix("GZF", data))
        return (true);   // Gif/Gzf
    if (lstring::prefix("eX!flg", data))
        return (true);   // flg
    if ((unsigned char)data[0] == 0xff && (unsigned char)data[1] == 0xd8 &&
            (unsigned char)data[2] == 0xff)
        return (true);   // jpeg
    if (!memcmp(data, png_magic, 8))
        return (true);   // png
    if (lstring::prefix("/* XPM", data) || lstring::prefix("// XPM", data))
        return (true);   // xpm
    if (lstring::prefix("#define", data))
        return (true);   // xbm
    if ((data[0] == 0x49 && data[1] == 0x49 &&
            data[2] == 0x2a && data[3] == 0) ||
            (data[0] == 0x4d && data[1] == 0x4d &&
            data[2] == 0 && data[3] == 0x2a))
        return (true);   // tiff
    if (data[0] == 'P' && data[1] >= '1' && data[1] <= '6' &&
            isspace(data[2]))
        return (true);   // pnm/ppm/pgm/pbm
    return (false);
}



namespace {
    // Return a dummy topic for the image reference passed.
    //
    HLPtopic *
    new_image_topic(const char *fname)
    {
        char *buf = new char [strlen(fname) + 15];
        sprintf(buf, "<img src=\"%s\">", fname);
        HLPtopic *top = new HLPtopic(fname, "");
        top->get_string(buf);
        top->set_need_body(true);
        delete [] buf;
        return (top);
    }
}


// Return a topic if passed an image file reference.
//
HLPtopic *
HLPcontext::checkImage(const char *fname, ViewerWidget *w)
{
    // Is this an image file?  If so, create some dummy stuff to display
    // the image
    if (hcxCache) {
        // check cache file if match
        HLPcacheEnt *cent = hcxCache->get(fname, no_cache(w));
        if (cent && cent->get_status() == DLok) {
            FILE *fp = fopen(cent->filename, "rb");
            if (fp) {
                char data[32];
                fread(data, 1, 32, fp);
                bool ret = isImage(data);
                fclose(fp);
                if (ret)
                    return (new_image_topic(fname));
                return (0);
            }
        }
    }
    // check path on this machine
    FILE *fp = fopen(fname, "rb");
    if (fp) {
        char data[32];
        fread(data, 1, 32, fp);
        bool ret = isImage(data);
        fclose(fp);
        if (ret)
            return (new_image_topic(fname));
        return (0);
    }
    // must be a protocol spec, look at extension
    const char *sf = strrchr(fname, '.');
    if (sf) {
        sf++;
        if (lstring::cieq(sf, "gif") || lstring::cieq(sf, "jpg") ||
                lstring::cieq(sf, "jpeg") || lstring::cieq(sf, "tif") ||
                lstring::cieq(sf, "tiff") || lstring::cieq(sf, "png") ||
                lstring::cieq(sf, "pnm") || lstring::cieq(sf, "ppm") ||
                lstring::cieq(sf, "pgm") || lstring::cieq(sf, "pbm") ||
                lstring::cieq(sf, "xbm") || lstring::cieq(sf, "xpm"))
            return (new_image_topic(fname));
    }
    return (0);
}


//-----------------------------------------------------------------------------
// Image download

// Delete inactive image list elements for w.
//
void
HLPcontext::flushImages(ViewerWidget *w)
{
    HLPimageList *ip = 0, *ix;
    for (HLPimageList *im = hcxImageList; im; im = ix) {
        ix = im->next;
        if (im->widget == w && im->inactive) {
            if (!ip)
                hcxImageList = ix;
            else
                ip->next = ix;
            im->next = hcxImageFreeList;
            hcxImageFreeList = ix;
            continue;
        }
        ip = im;
    }
    while (hcxImageFreeList) {
        HLPimageList *nxt = hcxImageFreeList->next;
        if (no_cache(w) && hcxImageFreeList->filename)
            unlink(hcxImageFreeList->filename);

        delete hcxImageFreeList;
        hcxImageFreeList = nxt;
    }
}


// Inactivate all image downloads associated with w.
//
void
HLPcontext::abortImageDownload(ViewerWidget *w)
{
    for (HLPimageList *im = hcxImageList; im; im = im->next) {
        if (im->widget != w)
            continue;
        if (im->status == HLPimageList::IMinprogress) {
            unlink(im->filename);
            im->status = HLPimageList::IMabort;
        }
        else if (im->status == HLPimageList::IMtodo)
            im->status = HLPimageList::IMabort;
        im->inactive = true;
    }
}


// Inactivate loading of images matching im.
//
void
HLPcontext::inactivateImages(HLPimageList *im)
{
    for (HLPimageList *ix = hcxImageList; ix; ix = ix->next) {
        if (*im == *ix)
            ix->inactive = true;
    }
}


// Return true if an image for w appears in the list.
//
bool
HLPcontext::isImageInList(ViewerWidget *w)
{
    for (HLPimageList *im = hcxImageList; im; im = im->next) {
        if (im->widget == w)
            return (true);
    }
    return (false);
}


htmImageInfo *
HLPcontext::imageResolve(const char *fname, ViewerWidget *w)
{
    char *url = 0;
    if (lstring::is_rooted(fname)) {
        url = lstring::copy(fname);
        url = rootAlias(url);
        if (access(url, R_OK)) {
            delete [] url;
            url = 0;
        }
    
    }
    if (!url)
        url = urlcat(w->get_url(), fname);
    if (isProtocol(url)) {
        htmImageInfo *im = httpGetImage(0, &url, 0, w);
        delete [] url;
        return (im);
    }
    if (*url == '/') {
        htmImageInfo *im;
        if (w->image_debug_mode() == HLPparams::LIprogressive)
            im = localProgTest(w, url);
        else
            im = w->image_procedure(url);
        delete [] url;
        return (im);
    }
    delete [] url;

    char *path;
    if (!HLP()->get_path(&path) || !path)
        return (0);
    pathgen pg(path);
    delete [] path;
    char *p;
    while ((p = pg.nextpath(false)) != 0) {
        char *ptmp = pathlist::mk_path(p, fname);
        delete [] p;
        p = ptmp;
        if (!access(p, F_OK)) {
            htmImageInfo *ret;
            if (w->image_debug_mode() == HLPparams::LIprogressive)
                ret = localProgTest(w, p);
            else
                ret = w->image_procedure(p);
            delete [] p;
            return (ret);
        }
        delete [] p;
    }
    if (w->image_debug_mode() == HLPparams::LIprogressive)
        return (localProgTest(w, fname));
    else
        return (w->image_procedure(fname));
}


// This takes care of image retrieval, for non-local images
//
htmImageInfo *
HLPcontext::httpGetImage(char **fname, char **url, char *args,
    ViewerWidget *w)
{
    HLPcacheEnt *cent = 0;

    // Don't bother looking at the dates, if an image is in cache, assume
    // it is current
    for (;;) {
        if (hcxCache) {
            cent = hcxCache->get(*url, no_cache(w));
            if (cent && cent->get_status() == DLnogo)
                return (0);
            if (w->image_debug_mode() != HLPparams::LIdelayed) {
                // This is bypassed for debugging delayed loading using
                // local image files
                if (cent && cent->get_status() == DLok) {
                    FILE *fp = fopen(cent->filename, "rb");
                    if (fp) {
                        if (checkLocation(url, fp, w)) {
                            fclose(fp);
                            continue;
                        }
                        fclose(fp);
                        if (fname) {
                            delete [] *fname;
                            *fname = lstring::copy(cent->filename);
                        }
                        return (w->image_procedure(cent->filename));
                    }
                    hcxCache->set_complete(cent, DLincomplete);
                }
            }
        }
        else
            hcxCache = new HLPcache;
        break;
    }

    if (w->image_load_mode() == HLPparams::LoadNone)
        return (0);

    if (w->image_load_mode() == HLPparams::LoadDelayed ||
            w->image_load_mode() == HLPparams::LoadProgressive) {
        // always use normal loading for the body image, which will be the
        // first image requested
        if (!w->is_body_image(*url)) {

            // Progresive loading not supported for PNG, XPM
            // Not used for gif since animations are lost
            bool progressive = false;
            if (w->image_load_mode() == HLPparams::LoadProgressive) {
                progressive = true;
                char *t = strrchr(*url, '.');
                if (t) {
                    t++;
                    if (lstring::cieq(t, "png") || lstring::cieq(t, "xpm") ||
                            lstring::cieq(t, "gif"))
                        progressive = false;
                }
            }
            htmImageInfo *image = w->new_image_info(*url, progressive);

            // See if there is already an instance of this image to be
            // downloaded
            bool inlist = false;
            for (HLPimageList *ix = hcxImageList; ix; ix = ix->next) {
                if (ix->inactive)
                    continue;
                if (ix->url && !strcmp(*url, ix->url)) {
                    switch (ix->status) {
                    case HLPimageList::IMnone:
                        continue;
                    case HLPimageList::IMtodo:
                    case HLPimageList::IMinprogress:
                    case HLPimageList::IMdone:
                        inlist = true;
                        break;
                    case HLPimageList::IMabort:
                        return (0);
                    }
                    if (inlist)
                        break;
                }
            }
            HLPimageList *im = new HLPimageList(w, 0, *url, image,
                    inlist ? HLPimageList::IMnone : HLPimageList::IMtodo, 0);
            if (progressive) {
                im->load_prog = true;
                image->user_data = im;
            }

            if (!hcxImageList)
                hcxImageList = im;
            else {
                HLPimageList *ix = hcxImageList;
                while (ix->next)
                    ix = ix->next;
                ix->next = im;
            }
            return (image);
        }
    }
    char *fn = 0;
    FILE *fp = httpRetrieve(&fn, url, args, w, false);
    htmImageInfo *image = 0;
    if (fp) {
        fclose(fp);
        image = w->image_procedure(fn);
    }
    if (fname)
        *fname = fn;
    else
        delete [] fn;
    return (image);
}


// This is for debugging progressive loading using local image files.
//
htmImageInfo *
HLPcontext::localProgTest(ViewerWidget *w, const char *fname)
{
    // Progresive loading not supported for PNG, XPM
    // Not used for gif since animations are lost
    const char *t = strrchr(fname, '.');
    if (t) {
        t++;
        if (lstring::cieq(t, "png") || lstring::cieq(t, "xpm") ||
                lstring::cieq(t, "gif"))
            return (w->image_procedure(fname));
    }

    htmImageInfo *image = w->new_image_info(fname, true);
    HLPimageList *im =
        new HLPimageList(w, fname, 0, image, HLPimageList::IMinprogress, 0);
    im->local_image = true;
    im->load_prog = true;
    image->user_data = im;
    if (!hcxLocalTest)
        hcxLocalTest = im;
    else {
        HLPimageList *ix = hcxLocalTest;
        while (ix->next)
            ix = ix->next;
        ix->next = im;
    }
    return (image);
}


int
HLPcontext::getImageData(HLPimageList *im, htmPLCStream *stream, void *buffer)
{
    if (im->status == HLPimageList::IMabort)
        return (STREAM_ABORT);

    if (!im->local_image) {
        Transaction *t = im->widget->get_transaction();
        if (t) {
            if (!t->response()->data || t->response()->bytes_read < 8)
                return (STREAM_SUSPEND);
            if (stream->total_in == 0) {
                // Make sure that this is an image file.  In
                // particular, avoid reading a relocation return.
                char bf[10];
                memcpy(bf, t->response()->data, 8);
                if (!isImage(bf)) {
                    if (im->status == HLPimageList::IMdone ||
                            (t->response()->status != 301 &&
                            t->response()->status != 302))
                        return (STREAM_ABORT);
                    return (STREAM_SUSPEND);
                }
            }
            if (t->response()->status != HTTPSuccess)
                return (STREAM_ABORT);
            unsigned avail = t->response()->bytes_read - stream->total_in;
            if (!avail || avail < stream->min_out)
                return (STREAM_SUSPEND);
            if (avail > stream->max_out)
                avail = stream->max_out;
            memcpy(buffer, t->response()->data + stream->total_in, avail);
            im->start_prog = true;
            return (avail);
        }
        return (STREAM_ABORT);
    }
    if ((im->status != HLPimageList::IMinprogress &&
            im->status != HLPimageList::IMdone) || !im->filename)
        return (STREAM_SUSPEND);
    int fd = open(im->filename, O_RDONLY);
    if (fd < 0) {
        if (errno != ENOENT)
            return (STREAM_ABORT);
        if (!im->url && im->status == HLPimageList::IMdone)
            // local file not found
            return (0);
        return (STREAM_SUSPEND);
    }
    struct stat st;
    if (fstat(fd, &st) < 0) {
        close(fd);
        return (STREAM_SUSPEND);
    }
    long length = st.st_size;
    if (length < 8) {
        close(fd);
        return (STREAM_SUSPEND);
    }
    lseek(fd, stream->total_in, SEEK_SET);

    if (stream->total_in == 0) {
        // Make sure that this is an image file.  In particular, avoid
        // reading a relocation return
        char bf[10];
        read(fd, bf, 8);
        if (!isImage(bf)) {
            close(fd);
            if (im->status == HLPimageList::IMdone)
                return (STREAM_ABORT);
            return (STREAM_SUSPEND);
        }
        lseek(fd, 0, SEEK_SET);
    }
    int avail = length - stream->total_in;
    if (!avail || (unsigned)avail < stream->min_out) {
        close(fd);
        return (STREAM_SUSPEND);
    }
    if ((unsigned)avail > stream->max_out)
        avail = stream->max_out;
    int num = read(fd, buffer, avail);
    close(fd);
    if (num > 0)
        return (num);
    if (num == 0)
        return (STREAM_SUSPEND);
    return (STREAM_ABORT);
}


// not used - this is entirely sequential
#define MAX_CONCURRENT 5

// Idle procedure to repetitiously process the image download queue.
//
bool
HLPcontext::processList()
{
    int cnt = 0;
    for (HLPimageList *im = hcxImageList; im; im = im->next) {
        if (im->inactive)
            continue;
        if (im->status == HLPimageList::IMtodo ||
                im->status == HLPimageList::IMinprogress) {
            cnt++;
            if (cnt == MAX_CONCURRENT)
                break;
            if (im->status == HLPimageList::IMtodo) {
                startImageDownload(im);
                break;
            }
        }
        else if (im->status != HLPimageList::IMnone) {
            im->inactive = true;
            continue;
        }
    }

    {
        HLPimageList *ip = 0, *ix;
        for (HLPimageList *im = hcxImageList; im; im = ix) {
            ix = im->next;
            if (im->inactive) {
                if (!ip)
                    hcxImageList = ix;
                else
                    ip->next = ix;
                im->next = hcxImageFreeList;
                hcxImageFreeList = im;
                continue;
            }
            ip = im;
        }
    }

    while (hcxLocalTest) {
        if (hcxLocalTest->widget->call_plc(hcxLocalTest->filename))
            return (true);
        HLPimageList *ix = hcxLocalTest->next;
        delete hcxLocalTest;
        hcxLocalTest = ix;
    }

    return (hcxImageList != 0);
}


// Download an image file.
//
void
HLPcontext::startImageDownload(HLPimageList *im)
{
    if (!im->widget)
        return;
    if (im->widget->get_transaction())
        // already downloading.
        return;

    if (im->status == HLPimageList::IMtodo) {
        im->status = HLPimageList::IMinprogress;
        // be careful - the ImageList might be cleared during httpRetrieve
        char *filename = lstring::copy(im->filename);
        char *url = lstring::copy(im->url);
        FILE *fp = httpRetrieve(&filename, &url, 0, im->widget, false,
            im->load_prog ? im : 0);
        if (hcxImageList) {
            delete [] im->filename;
            im->filename = filename;
            delete [] im->url;
            im->url = url;
            if (fp) {
                fclose(fp);
                im->status = HLPimageList::IMdone;
            }
            else {
                im->status = HLPimageList::IMabort;
                for (HLPimageList *ix = hcxImageList; ix; ix = ix->next) {
                    if (ix->status == HLPimageList::IMnone && *im == *ix)
                        ix->status = HLPimageList::IMabort;
                }
            }
            if (!im->start_prog)
                loadImages(im);
        }
        else {
            if (fp)
                fclose(fp);
            delete [] filename;
            delete [] url;
        }
    }
}


// Load the images for im and its copies into the widget, and remove
// these from the image queue.  This is called each time an image
// download completes, in delayed image mode only.
//
void
HLPcontext::loadImages(HLPimageList *im)
{
    if (!im->widget)
        return;
    if (im->status != HLPimageList::IMabort) {
        FILE *fp = fopen(im->filename, "rb");
        if (fp) {
            fclose(fp);
            HLPcacheEnt *cent = hcxCache->get(im->url, no_cache(im->widget));
            if (cent)
                hcxCache->set_complete(cent, DLok);
            im->widget->freeze();
            htmImageInfo *image = im->widget->image_procedure(im->filename);
            if (image) {
                im->widget->image_replace(im->tmp_image, image);

                // Look for other instances of this image.  When
                // found, load the image, and dactivate the entry
                for (HLPimageList *ix = hcxImageList; ix; ix = ix->next) {
                    if (ix->inactive || ix == im)
                        continue;
                    if (*im == *ix) {
                        image = ix->widget->image_procedure(im->filename);
                        ix->widget->image_replace(ix->tmp_image, image);
                        ix->inactive = true;
                    }
                }
            }
            im->widget->thaw();
        }
    }
    else {
        HLPcacheEnt *cent = hcxCache->get(im->url, no_cache(im->widget));
        if (cent)
            hcxCache->set_complete(cent, DLnogo);
    }

    im->inactive = true;
    for (HLPimageList *ix = hcxImageList; ix; ix = ix->next) {
        if (*im == *ix)
            ix->inactive = true;
    }
}

