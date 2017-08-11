
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
 * WRspice Circuit Simulation and Analysis Tool                           *
 *                                                                        *
 *========================================================================*
 $Id:$
 *========================================================================*/

#ifndef COMMANDS_H
#define COMMANDS_H


// Flags for old built-in help, set in sCommand::co_env.
#define E_HASPLOTS  1
#define E_NOPLOTS   2
#define E_HASGRAPHS 4
#define E_MENUMODE  8

#define E_BEGINNING 0x1000
#define E_INTERMED  0x2000
#define E_ADVANCED  0x4000
#define E_ALWAYS    0x8000

// Default is intermediate level.
#define E_DEFHMASK  E_INTERMED


// Information about commands.
//
struct sCommand
{
    sCommand(const char *comname, void(func)(wordlist*), bool stringargs,
        bool spiceonly, bool major, unsigned int t1, unsigned t2,
        unsigned int t3, unsigned int t4, unsigned int env,
        int minargs, int maxargs, int(*argfn)(wordlist*, sCommand*),
        const char *help)
    {
        co_comname = comname;
        co_func = func;
        co_stringargs = stringargs;
        co_spiceonly = spiceonly;
        co_major = major;
        co_cctypes[0] = t1;
        co_cctypes[1] = t2;
        co_cctypes[2] = t3;
        co_cctypes[3] = t4;
        co_env = env;
        co_minargs = minargs;
        co_maxargs = maxargs;
        co_argfn = argfn;
        co_help = help;
    }

    const char *co_comname;     // The name of the command.
    void (*co_func)(wordlist*); // The function that handles the command.
    bool co_stringargs;         // Collapse the arguments into a string.
    bool co_spiceonly;          // These can't be used from nutmeg.
    bool co_major;              // Is this a "major" command?
    unsigned int co_cctypes[4]; // Bitmasks for command completion.
    unsigned int co_env;        // Print help message on this environment mask.
    int co_minargs;             // Minimum number of arguments required.
    int co_maxargs;             // Maximum number of arguments allowed.
    int (*co_argfn)(wordlist*, sCommand*); // The fn that prompts the user.
    const char *co_help;        // Help message.
};

struct sHtab;

#define LOTS        1000

class CommandTab
{
public:
    sCommand *FindCommand(const char*);
    void CcSetup();
    int Commands(sCommand***);

    static void com_ac(wordlist*);
    static void com_alias(wordlist*);
    static void com_alter(wordlist*);
    static void com_asciiplot(wordlist*);
    static void com_aspice(wordlist*);
    static void com_bug(wordlist*);
    static void com_cache(wordlist*);
    static void com_cd(wordlist*);
    static void com_cdump(wordlist*);
    static void com_check(wordlist*);
    static void com_codeblock(wordlist*);
    static void com_combine(wordlist*);
    static void com_compose(wordlist*);
    static void com_cross(wordlist*);
    static void com_dc(wordlist*);
    static void com_define(wordlist*);
    static void com_deftype(wordlist*);
    static void com_delete(wordlist*);
    static void com_destroy(wordlist*);
    static void com_devcnt(wordlist*);
    static void com_devload(wordlist*);
    static void com_devls(wordlist*);
    static void com_devmod(wordlist*);
    static void com_diff(wordlist*);
    static void com_display(wordlist*);
    static void com_disto(wordlist*);
    static void com_dump(wordlist*);
    static void com_dumpnodes(wordlist*);
    static void com_dumpopts(wordlist*);
    static void com_echo(wordlist*);
    static void com_echof(wordlist*);
    static void com_edit(wordlist*);
    static void com_fourier(wordlist*);
    static void com_free(wordlist*);
    static void com_hardcopy(wordlist*);
    static void com_help(wordlist*);
    static void com_helpreset(wordlist*);
    static void com_history(wordlist*);
    static void com_iplot(wordlist*);
    static void com_jobs(wordlist*);
    static void com_let(wordlist*);
    static void com_linearize(wordlist*);
    static void com_listing(wordlist*);
    static void com_load(wordlist*);
    static void com_mapkey(wordlist*);
    static void com_mmon(wordlist*);
    static void com_mplot(wordlist*);
    static void com_noise(wordlist*);
    static void com_op(wordlist*);
    static void com_passwd(wordlist*);
    static void com_pause(wordlist*);
    static void com_pick(wordlist*);
    static void com_plot(wordlist*);
    static void com_plotwin(wordlist*);
    static void com_print(wordlist*);
    static void com_proxy(wordlist*);
    static void com_pwd(wordlist*);
    static void com_pz(wordlist*);
    static void com_qhelp(wordlist*);
    static void com_quit(wordlist*);
    static void com_rehash(wordlist*);
    static void com_reset(wordlist*);
    static void com_resume(wordlist*);
    static void com_retval(wordlist*);
    static void com_rhost(wordlist*);
    static void com_rspice(wordlist*);
    static void com_run(wordlist*);
    static void com_rusage(wordlist*);
    static void com_save(wordlist*);
    static void com_sced(wordlist*);
    static void com_seed(wordlist*);
    static void com_sens(wordlist*);
    static void com_set(wordlist*);
    static void com_setcase(wordlist*);
    static void com_setcirc(wordlist*);
    static void com_setdim(wordlist*);
    static void com_setfont(wordlist*);
    static void com_setplot(wordlist*);
    static void com_setrdb(wordlist*);
    static void com_setscale(wordlist*);
    static void com_settype(wordlist*);
    static void com_shell(wordlist*);
    static void com_shift(wordlist*);
    static void com_spec(wordlist*);
    static void com_show(wordlist*);
    static void com_source(wordlist*);
    static void com_state(wordlist*);
    static void com_stats(wordlist*);
    static void com_status(wordlist*);
    static void com_step(wordlist*);
    static void com_stop(wordlist*);
    static void com_strcicmp(wordlist*);
    static void com_strciprefix(wordlist*);
    static void com_strcmp(wordlist*);
    static void com_strprefix(wordlist*);
    static void com_sweep(wordlist*);
    static void com_tbsetup(wordlist*);
    static void com_tf(wordlist*);
    static void com_trace(wordlist*);
    static void com_tran(wordlist*);
    static void com_unalias(wordlist*);
    static void com_undefine(wordlist*);
    static void com_unlet(wordlist*);
    static void com_unset(wordlist*);
    static void com_update(wordlist*);
    static void com_usrset(wordlist*);
    static void com_version(wordlist*);
    static void com_where(wordlist*);
    static void com_write(wordlist*);
    static void com_wrupdate(wordlist*);
    static void com_xeditor(wordlist*);
    static void com_xgraph(wordlist*);
    static void com_fault(wordlist*);

private:
    static int arg_load(wordlist*, sCommand*);
    static int arg_let(wordlist*, sCommand*);
    static int arg_set(wordlist*, sCommand*);
    static int arg_display(wordlist*, sCommand*);

    sHtab *ct_cmdtab;

    static sCommand ct_list[];
};

extern CommandTab Cmds;

#endif

