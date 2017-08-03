
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
 * Misc. Utilities Library                                                *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

//
// Class for downloading/installing new releases.
//

#ifndef UPDATE_ITF_H
#define UPDATE_ITF_H

// The application must subclass this and pass it to the UpdIf
// constructor.
//
struct updif_t
{
    virtual ~updif_t() { }
    virtual const char *HomeDir() const = 0;
    virtual const char *Product() const = 0;
    virtual const char *VersionString() const = 0;
    virtual const char *OSname() const = 0;
    virtual const char *Arch() const = 0;
    virtual const char *DistSuffix() const = 0;
    virtual const char *Prefix() const = 0;
};

// Struct for parsing/composing version string.  The version string is
// in the form (all ints) generation.major.minor.
//
struct release_t
{
    release_t(const char*);
    char *string() const;

    bool operator==(const release_t&) const;
    bool operator<(const release_t&) const;

    int generation;
    int major;
    int minor;
};

// The interface.
//
class UpdIf
{
public:
    UpdIf(const updif_t&);
    ~UpdIf();

    const char *update_pwfile(const char*, const char*);
    char *program_name();
    release_t my_version();
    release_t distrib_version(const char* = 0, char** = 0, char** = 0,
        char ** = 0, char** = 0);
    char *distrib_filename(const release_t&, const char* = 0, const char* = 0,
        const char* = 0);
    char *download(const char*, const char* = 0, const char* = 0, char** = 0,
        bool(*)(void*, const char*) = 0);
    int install(const char*, const char*, const char*, const char* = 0,
        const char* = 0, char** = 0);

    static char *message(const char*);
    static bool new_release(const char*, const char*);
    static char *get_proxy();
    static const char *set_proxy(const char*, const char*);
    static const char *move_proxy(const char*);

    const char *username() { return (uif_user); }
    const char *password() { return (uif_password); }

private:
    char *uif_user;
    char *uif_password;
    char *uif_home;
    char *uif_product;
    char *uif_version;
    char *uif_osname;
    char *uif_arch;
    char *uif_dist_suffix;
    char *uif_prefix;
};

#endif

