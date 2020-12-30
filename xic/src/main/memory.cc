
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
 * Xic Integrated Circuit Layout and Schematic Editor                     *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#include "config.h"  // for HAVE_LOCAL_ALLOCATOR
#include "main.h"
#include "promptline.h"
#include "errorlog.h"
#include "dsp_tkif.h"
#include "dsp_inlines.h"
#include "miscutil/miscutil.h"
#ifdef HAVE_LOCAL_ALLOCATOR
#include "malloc/local_malloc.h"
#else
#include "miscutil/coresize.h"
#endif
#ifdef WIN32
#include "miscutil/msw.h"
#endif

#include <new>
#include <sys/types.h>
#include <fcntl.h>


//-----------------------------------------------------------------------
// Memory use tracking, new handler.
//

namespace {
    struct sCore
    {
        sCore()
            {
                std::set_new_handler(new_err_handler);
#ifdef HAVE_LOCAL_ALLOCATOR
                Memory()->register_error_log(memory_error_log);
#endif
            }

        static void new_err_handler();
#ifdef HAVE_LOCAL_ALLOCATOR
        static void memory_error_log(const char*, void*, long*, int);
#endif
    };

    sCore core;


    // Print an unsigned long in decimal form in *s, which is advanced.
    // This is to avoid using printf, which calls malloc, when malloc
    // can't be called.
    //
    void
    spr_ulong(size_t sz, char **s)
    {
        size_t tm = ((size_t)-1)/10;
        size_t n = 1;
        for (;;) {
            if (n > tm)
                break;
            size_t nn = n*10;
            if (nn > sz)
                break;
            n = nn;
        }
        while (n) {
            unsigned int p = sz/n;
            *(*s)++ = '0' + p;
            sz -= p*n;
            n /= 10;
        }
    }
}


// Called on memory allocation error.
//
void
sCore::new_err_handler()
{
    char buf[128];
    strcpy(buf, "FATAL ERROR: out of memory, managing ");
    char *s = buf + strlen(buf);
#ifdef HAVE_LOCAL_ALLOCATOR
    spr_ulong((size_t)Memory()->coresize(), &s);
#else
    spr_ulong((size_t)coresize(), &s);
#endif
    strcpy(s, " KB.\n");
    fputs(buf, stderr);
    XM()->Exit(ExitPanic);
}


#ifdef HAVE_LOCAL_ALLOCATOR

// This handles errors from the memory allocator, if it is linked.
//
void
sCore::memory_error_log(const char *what, void *chunk, long *stack, int stsz)
{
    if (!chunk && !strcmp(what, "out of memory"))
        // This will catch errors in C functions, too.
        new_err_handler();

    char buf[1024];
    if (Log()->LogDirectory())
        sprintf(buf, "%s/%s", Log()->LogDirectory(), Log()->MemErrLogName());
    else if (Log()->MemErrLogName())
        strcpy(buf, Log()->MemErrLogName());
    else
        strcpy(buf, "xic_memory_errors");
    int fd = open(buf, O_CREAT|O_WRONLY|O_APPEND, 0644);
    if (fd) {
        sprintf(buf, "%s-%s %s (%s) %s\n", XM()->Product(),
            XM()->VersionString(), XM()->OSname(),
            XM()->TagString(), miscutil::dateString());
        write(fd, buf, strlen(buf));
        sprintf(buf, "%s 0x%lx\n", what, (unsigned long)chunk);
        write(fd, buf, strlen(buf));
        for (int i = 0; i < stsz; i++) {
            sprintf(buf, "#%d 0x%lx\n", i, stack[i]);
            write(fd, buf, strlen(buf));
        }
        close(fd);
    }
    if (XM()->RunMode() == ModeNormal)
        XM()->SetMemError(true);
    else
        fputs("*** memory fault detected, logfile updated.\n", stderr);
}

#endif


//-----------------------------------------------------------------------
// Prompt line commands.
//

namespace {
#ifdef WIN32
    // Walk the process address space and enumerate the blocks (Win32).
    //
    void
    vmem_map(FILE *fp)
    {
        if (fp == 0)
            fp = stdout;
        MEMORY_BASIC_INFORMATION m;
        for (char *ptr = 0; ptr < (char*)0x7ff00000; ptr += m.RegionSize) {
            VirtualQuery(ptr, &m, sizeof(m));
            const char *ap = "";
            switch (m.AllocationProtect) {
            case PAGE_READONLY:
                ap = "readonly";
                break;
            case PAGE_READWRITE:
                ap = "readwrite";
                break;
            case PAGE_WRITECOPY:
                ap = "writecopy";
                break;
            case PAGE_EXECUTE:
                ap = "execute";
                break;
            case PAGE_EXECUTE_READ:
                ap = "ex_read";
                break;
            case PAGE_EXECUTE_READWRITE:
                ap = "ex_readwrite";
                break;
            case PAGE_EXECUTE_WRITECOPY:
                ap = "ex_writecopy";
                break;
            case PAGE_GUARD:
                ap = "guard";
                break;
            case PAGE_NOACCESS:
                ap = "noaccess";
                break;
            case PAGE_NOCACHE:
                ap = "nocache";
                break;
            }
            const char *st ="";
            switch (m.State) {
            case MEM_COMMIT:
                st = "commit";
                break;
            case MEM_FREE:
                st = "free";
                break;
            case MEM_RESERVE:
                st = "reserve";
                break;
            }
            const char *tp = "";
            switch (m.Type) {
            case MEM_IMAGE:
                tp = "image";
                break;
            case MEM_MAPPED:
                tp = "mapped";
                break;
            case MEM_PRIVATE:
                tp = "private";
                break;
            }
            fprintf(fp, "%-8p %-8lld %-8s %-8s %s\n", m.BaseAddress,
                m.RegionSize, st, tp, ap);
        }
    }


    void
    vmem(const char*)
    {
        FILE *fp = fopen("vmemout.txt", "w");
        if (fp) {
            vmem_map(fp);
            DSPmainWbag(PopUpFileBrowser("vmemout.txt"))
            fclose(fp);
        }
        PL()->ErasePrompt();
    }
#endif


    void
    memfault(const char*)
    {
        int *p = new int;
        delete p;
        delete p;
        uintptr_t a = 5;
        delete &a;
        delete (int*)a;
    }


    void
    oom(const char*)
    {
        char *memptr;
        int count = 0;
        while ((memptr = (char*)malloc(1024*1024)) != 0) {
            memset(memptr, 0, 1024*1024);
            count++;
            printf("1Mb block %d\n", count);
        }
    }


    void
    oom1(const char*)
    {
#ifdef HAVE_LOCAL_ALLOCATOR
        // This exercises the handling of a zero return from malloc.
        //
        Memory()->set_memory_fault_test(true);
        int *p = new int;
        delete p;
        Memory()->set_memory_fault_test(false);
#else
        PL()->ShowPrompt("Allocation monitor not available.");
#endif
    }


    void
    monstart(const char *s)
    {
#ifdef HAVE_LOCAL_ALLOCATOR
        int depth;
        if (*s) {
            depth = atoi(s);
            if (depth < 1 || depth > 15) {
                PL()->ShowPrompt(
                    "Depth argument outside range 1-15 for !monstart.");
                return;
            }
        }
        else
            depth = 1;
        if (Memory()->mon_start(depth))
            PL()->ShowPrompt("Allocation monitor started.");
        else
            PL()->ShowPrompt("Allocation monitor not started, unknown error.");
#else
        (void)s;
        PL()->ShowPrompt("Allocation monitor not available.");
#endif
    }


    void
    monstop(const char*)
    {
#ifdef HAVE_LOCAL_ALLOCATOR
        if (Memory()->mon_stop() && Memory()->mon_dump((char*)"mon.out"))
            PL()->ShowPrompt(
                "Allocation monitor stopped, data in file \"mon.out\".");
        else
            PL()->ShowPrompt("Allocation monitor failure.");
#else
        PL()->ShowPrompt("Allocation monitor not available.");
#endif
    }


    void
    monstatus(const char*)
    {
#ifdef HAVE_LOCAL_ALLOCATOR
        int i = Memory()->mon_count();
        PL()->ShowPromptV("Allocation table contains %d entries.", i);
#else
        PL()->ShowPrompt("Allocation monitor not available.");
#endif
    }
}


void
cMain::setupMemoryBangCmds()
{
#ifdef WIN32
    RegisterBangCmd("vmem", &vmem);
#endif
    RegisterBangCmd("memfault", &memfault);
    RegisterBangCmd("oom", &oom);
    RegisterBangCmd("oom1", &oom1);
    RegisterBangCmd("monstart", &monstart);
    RegisterBangCmd("monstop", &monstop);
    RegisterBangCmd("monstatus", &monstatus);
}

