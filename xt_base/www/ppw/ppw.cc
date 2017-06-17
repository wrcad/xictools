
/*==========================================================================
 *
 * PPW password maintenance program
 *
 * Whiteley Research Inc. proprietary software, not for public release.
 * $Id: ppw.cc,v 1.3 2016/01/16 18:58:39 stevew Exp $
 *==========================================================================*/

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <sys/stat.h>
#include <ctype.h>
#include <unistd.h>
#include <pwd.h>
#include <algorithm>


// These are -D defines in the makefile.
#ifndef PWDIR
#define PWDIR
#endif

#define PPW_LIST_FILE PWDIR".ppw_list"
#define PPW_EXP_FILE PWDIR".ppw_expired"
#define HTPASSWD PWDIR".htpasswd"


namespace {
    // Return a copy of the string arg.
    //
    char *
    copy(const char *string)
    {
        if (string == 0)
            return (0);
        char *ret, *s;
        s = ret = new char[strlen(string) + 1];
        while ((*s++ = *string++) != 0) ;
        return (ret);
    }

    inline bool
    is_sepchar(char c, const char *sepchars)
    {
        if (!c)
            return (false);
        return (isspace(c) || (sepchars && strchr(sepchars, c)));
    }

    inline bool
    is_not_sepchar(char c, const char *sepchars)
    {
        if (!c)
            return (false);
        return (!isspace(c) && (!sepchars || !strchr(sepchars, c)));
    }

    // Return a malloc'ed token, advance the pointer to next token.  Tokens
    // are separated by white space or characters in sepchars, it this is
    // not null.
    //
    char *
    gettok(char **s, const char *sepchars = 0)
    {
        if (s == 0 || *s == 0)
            return (0);
        while (is_sepchar(**s, sepchars))
            (*s)++;
        if (!**s)
            return (0);
        char *st = *s;
        while (is_not_sepchar(**s, sepchars))
            (*s)++;
        char *cbuf = new char[*s - st + 1];
        char *c = cbuf;
        while (st < *s)
            *c++ = *st++;
        *c = 0;
        while (is_sepchar(**s, sepchars))
            (*s)++;
        return (cbuf);
    }
}


// Per-user entry element.
//
struct ppw_ent
{
    ppw_ent(char *u, char *pw, unsigned int t, ppw_ent *n)
        {
            e_next = n;
            e_user = u;
            e_pw = pw;
            e_time = t;
        }

    ~ppw_ent()
        {
            delete [] e_user;
            delete [] e_pw;
        }

    void free()
        {
            ppw_ent *e = this;
            while (e) {
                ppw_ent *ex = e;
                e = e->e_next;
                delete ex;
            }
        }

    void print(FILE *fp)
        {
            fprintf(fp, "%-16s %-16s %u\n", e_user, e_pw, e_time);
        }

    ppw_ent *e_next;
    char *e_user;
    char *e_pw;
    unsigned int e_time;
};


// Main database container.
//
struct ppw
{
    ppw(const char*);
    ~ppw();

    bool check(const char*);
    bool list();
    bool add(const char*, const char*, unsigned int);
    bool refresh(const char*, unsigned int);
    bool remove(const char*);
    bool run();

    const char *errmsg() { return (ppw_errmsg); }

private:
    bool update();
    bool update_ht();
    void sort();

    ppw_ent *ppw_entries;
    char *ppw_errmsg;
};


// Constructor, read the list from the file.  If error, return and error
// message in ppw_errmsg.
//
ppw::ppw(const char *listfile)
{
    ppw_entries = 0;
    ppw_errmsg = 0;

    FILE *fp = fopen(listfile, "r");
    if (!fp) {
        if (errno != ENOENT)
            ppw_errmsg = copy(strerror(errno));
        return;
    }
    char *s, buf[256];
    while ((s = fgets(buf, 256, fp)) != 0) {
        char *u = gettok(&s);
        char *pw = gettok(&s);
        char *t = gettok(&s);
        if (!t) {
            fprintf(stderr, "Uh-oh, %s is corrupt, exiting.\n", listfile);
            exit(1);
        }
        int t_u = atoi(t);
        delete [] t;

        ppw_entries = new ppw_ent(u, pw, t_u, ppw_entries);
    }
    fclose(fp);
};


ppw::~ppw()
{
    ppw_entries->free();
    delete [] ppw_errmsg;
}


namespace {
    // Convert months (from now) to a universal time integer.
    //
    time_t
    timeset(int months)
    {
        time_t loc;
        time(&loc);
        tm *t = localtime(&loc);
        months += t->tm_mon;
        int years = months/12;
        months %= 12;
        t->tm_year += years;
        t->tm_mon = months;
        // Add a 15 day grace period.
        return (mktime(t) + 15*24*3600);
    }


    // Return true if timelim is valid.  If print, print the expiration
    // time in human-readable form.
    //
    bool
    check_time(unsigned int timelim, bool print)
    {
        if (timelim == 0) {
            if (print)
                printf("permanent\n");
            return (true);
        }
        time_t loc;
        time(&loc);
        time_t t = (time_t)timelim;
        tm *tp = localtime(&t);
        if (print) {
            char *ap = asctime(tp);
            fputs(ap, stdout);
        }
        if ((unsigned int)loc < timelim)
            return (true);
        return (false);
    }
}


// Return true if user is already listed, false otherwise.
//
bool
ppw::check(const char *user)
{
    for (ppw_ent *e = ppw_entries; e; e = e->e_next) {
        if (!strcmp(user, e->e_user))
            return (true);
    }
    return (false);
}


// Print a list of the entries to stdout.
//
bool
ppw::list()
{
    sort();
    for (ppw_ent *e = ppw_entries; e; e = e->e_next) {
        printf("%-16s ", e->e_user);
        check_time(e->e_time, true);
    }
    return (true);
}


// Add an entry to the list, and update the list file.
//
bool
ppw::add(const char *user, const char *pw, unsigned int tlim)
{
    for (ppw_ent *e = ppw_entries; e; e = e->e_next) {
        if (!strcmp(user, e->e_user)) {
            char buf[256];
            sprintf(buf, "can't add duplicate user %s.\n", user);
            ppw_errmsg = copy(buf);
            return (false);
        }
    }
    ppw_ent *e = new ppw_ent(copy(user), copy(pw), tlim, 0);
    e->e_next = ppw_entries;
    ppw_entries = e;
    sort();
    return (update());
}


// Modify the time limit for user, and update the list file.
//
bool
ppw::refresh(const char *user, unsigned int tlim)
{
    for (ppw_ent *e = ppw_entries; e; e = e->e_next) {
        if (!strcmp(user, e->e_user)) {
            e->e_time = tlim;
            return (update());
        }
    }
    char buf[256];
    sprintf(buf, "can't refresh unknown user %s.\n", user);
    ppw_errmsg = copy(buf);
    return (false);
}


// Remove the entry for user, and update the list file.
//
bool
ppw::remove(const char *user)
{
    ppw_ent *ep = 0, *en;
    for (ppw_ent *e = ppw_entries; e; e = en) {
        en = e->e_next;
        if (!strcmp(user, e->e_user)) {
            if (ep)
                ep->e_next = en;
            else
                ppw_entries = en;
            delete e;
            return (update());
        }
        ep = e;
    }
    char buf[256];
    sprintf(buf, "can't remove unknown user %s.\n", user);
    ppw_errmsg = copy(buf);
    return (false);
}


// Remove the expired entries from the list, and update the list file.
// The expired entries are appended to the expired file.
//
bool
ppw::run()
{
    FILE *fp = fopen(PPW_EXP_FILE, "a+");
    if (!fp) {
        ppw_errmsg = copy(strerror(errno));
        return (false);
    }

    bool found = false;
    ppw_ent *ep = 0, *en;
    for (ppw_ent *e = ppw_entries; e; e = en) {
        en = e->e_next;
        if (!check_time(e->e_time, false)) {
            if (ep)
                ep->e_next = en;
            else
                ppw_entries = en;
            e->print(fp);
            delete e;
            found = true;
            continue;
        }
        ep = e;
    }
    fclose(fp);
    if (!found)
        return (true);
    return (update());
}


// Private function to rewrite the list file.
//
bool
ppw::update()
{
    FILE *fp = fopen(PPW_LIST_FILE, "w");
    if (!fp) {
        ppw_errmsg = copy(strerror(errno));
        return (false);
    }

    for (ppw_ent *e = ppw_entries; e; e = e->e_next)
        e->print(fp);

    fclose(fp);
    return (update_ht());
}


bool
ppw::update_ht()
{
    FILE *fp = fopen(HTPASSWD".new", "w");
    if (!fp) {
        ppw_errmsg = copy(strerror(errno));
        return (false);
    }

    for (ppw_ent *e = ppw_entries; e; e = e->e_next)
        fprintf(fp, "%s:%s\n", e->e_user, e->e_pw);
    fclose(fp);

    unlink(HTPASSWD".bak");
    link(HTPASSWD, HTPASSWD".bak");
    unlink(HTPASSWD);
    link(HTPASSWD".new", HTPASSWD);
    unlink(HTPASSWD".new");
    chmod(HTPASSWD, 0644);

    return (true);
}


namespace {
    inline bool
    ecmp(const ppw_ent *e1, const ppw_ent *e2)
    {
        return (strcmp(e1->e_user, e2->e_user) < 0);
    }
}


// Private function to alphabetically sort the entries by user.
//
void
ppw::sort()
{
    int cnt = 0;
    for (ppw_ent *e = ppw_entries; e; e = e->e_next)
        cnt++;
    if (cnt < 2)
        return;
    ppw_ent **ary = new ppw_ent*[cnt];
    cnt = 0;
    for (ppw_ent *e = ppw_entries; e; e = e->e_next)
        ary[cnt++] = e;
    std::sort(ary, ary + cnt, ecmp);

    for (int i = 1; i < cnt; i++)
        ary[i-1]->e_next = ary[i];
    ary[cnt-1]->e_next = 0;
    ppw_entries = ary[0];
    delete [] ary;
}
// End of ppw functions.


namespace {
    void
    usage()
    {
        printf("Usage:\n"
            "\tppw check user\n"
            "\tppw list\n"
            "\tppw add user password [months]\n"
            "\tppw refresh user months\n"
            "\tppw remove user\n"
            "\tppw run\n" );
    }
}


int
main(int argc, char **argv)
{
    if (argc < 2) {
        usage();
        return (0);
    }

    if (!strcmp(argv[1], "check")) {
        if (argc < 3) {
            printf("Error: too few args for check.\n");
            usage();
            return (1);
        }
        const char *user = argv[2];

        ppw p(PPW_LIST_FILE);
        if (p.errmsg()) {
            fprintf(stderr, "%s\n", p.errmsg());
            return (1);
        }
        if (!p.check(user)) {
            if (p.errmsg()) {
                fprintf(stderr, "%s\n", p.errmsg());
                return (1);
            }
            printf("no\n");
        }
        else
            printf("yes\n");
    }
    else if (!strcmp(argv[1], "list")) {
        ppw p(PPW_LIST_FILE);
        if (p.errmsg()) {
            fprintf(stderr, "%s\n", p.errmsg());
            return (1);
        }
        if (!p.list()) {
            if (p.errmsg()) {
                fprintf(stderr, "%s\n", p.errmsg());
                return (1);
            }
        }
    }
    else if (!strcmp(argv[1], "add")) {
        if (argc < 4) {
            printf("Error: too few args for add.\n");
            usage();
            return (1);
        }
        const char *user = argv[2];
        const char *pw = argv[3];
        unsigned int mos = 0;
        if (argc > 4) {
            const char *ms = argv[4];
            int m = atoi(ms);
            if (m > 0)
                mos = timeset(m);
        }

        ppw p(PPW_LIST_FILE);
        if (p.errmsg()) {
            fprintf(stderr, "%s\n", p.errmsg());
            return (1);
        }
        if (!p.add(user, pw, mos)) {
            if (p.errmsg()) {
                fprintf(stderr, "%s\n", p.errmsg());
                return (1);
            }
        }
        printf("Successfully added user %s.\n", user);
    }
    else if (!strcmp(argv[1], "refresh")) {
        if (argc < 4) {
            printf("Error: too few args for refresh.\n");
            usage();
            return (1);
        }
        const char *user = argv[2];
        unsigned int mos = 0;
        const char *ms = argv[3];
        int m = atoi(ms);
        if (m > 0)
            mos = timeset(m);

        ppw p(PPW_LIST_FILE);
        if (p.errmsg()) {
            fprintf(stderr, "%s\n", p.errmsg());
            return (1);
        }
        if (!p.refresh(user, mos)) {
            if (p.errmsg()) {
                fprintf(stderr, "%s\n", p.errmsg());
                return (1);
            }
        }
        printf("Successfully refreshed user %s.\n", user);
    }
    else if (!strcmp(argv[1], "remove")) {
        if (argc < 3) {
            printf("Error: too few args for remove.\n");
            usage();
            return (1);
        }
        const char *user = argv[2];

        ppw p(PPW_LIST_FILE);
        if (p.errmsg()) {
            fprintf(stderr, "%s\n", p.errmsg());
            return (1);
        }
        if (!p.remove(user)) {
            if (p.errmsg()) {
                fprintf(stderr, "%s\n", p.errmsg());
                return (1);
            }
        }
        printf("Successfully removed user %s.\n", user);
    }
    else if (!strcmp(argv[1], "run")) {
        ppw p(PPW_LIST_FILE);
        if (p.errmsg()) {
            fprintf(stderr, "%s\n", p.errmsg());
            return (1);
        }
        if (!p.run()) {
            if (p.errmsg()) {
                fprintf(stderr, "%s\n", p.errmsg());
                return (1);
            }
        }
    }
    else {
        printf("Error: unknown command.\n");
        usage();
        return (1);
    }
    return (0);
}

