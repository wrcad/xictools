/* GPW - Generate pronounceable passwords
   This program uses statistics on the frequency of three-letter sequences
   in your dictionary to generate passwords.  The statistics are in trigram.h,
   generated there by the program loadtris.  Use different dictionaries
   and you'll get different statistics.

   This program can generate every word in the dictionary, and a lot of
   non-words.  It won't generate a bunch of other non-words, call them the
   unpronounceable ones, containing letter combinations found nowhere in
   the dictionary.  My rough estimate is that if there are 10^6 words,
   then there are about 10^9 pronounceables, out of a total population
   of 10^11 8-character strings.  I base this on running the program a lot
   and looking for real words in its output.. they are very rare, on the
   order of one in a thousand.

   This program uses "drand48()" to get random numbers, and "srand48()"
   to set the seed to the microsecond part of the system clock.  Works
   for AIX C++ compiler and runtime.  Might have to change this to port
   to another environment.

   The best way to use this program is to generate multiple words.  Then
   pick one you like and transform it with punctuation, capitalization,
   and other changes to come up with a new password.

   THVV 6/1/94 Coded
   THVV 1/6/06 updated to work with gcc4

   */

#include "trigram.h"
#include "stdio.h"
#include "stdlib.h"
#include "math.h"
#include <sys/types.h>
/* following for SysV. */
/* #include <bsd/sys/time.h> */
/* following for BSD */
#include <sys/time.h>

int main (int argc, char ** argv) {
    int password_length;	/* how long should each password be */
    int n_passwords;		/* number of passwords to generate */
    int pwnum;			/* number generated so far */
    int c1, c2, c3;		/* array indices */
    long sumfreq;		/* total frequencies[c1][c2][*] */
    double pik;			/* raw random number in [0.1] from drand48() */
    long ranno;			/* random number in [0,sumfreq] */
    long sum;			/* running total of frequencies */
    char password[100];		/* buffer to develop a password */
    int nchar;			/* number of chars in password so far */
    struct timeval systime;	/* time reading for random seed */
    struct timezone tz;		/* unused arg to gettimeofday */

    password_length = 8;	/* Default value for password length */
    n_passwords = 10;		/* Default value for number of pws to generate */

    gettimeofday (&systime, &tz); /* Read clock. */
    srand48 (systime.tv_usec);	/* Set random seed. */

    if (argc > 1) {		/* If args are given, convert to numbers. */
	n_passwords = atoi (&argv[1][0]);
	if (argc > 2) {
	    password_length = atoi (&argv[2][0]);
	}
    }
    if (argc > 3 || password_length > 99 ||
	password_length < 0 || n_passwords < 0) {
	printf (" USAGE: gpw [npasswds] [pwlength]\n");
	exit (4);
    }

    /* Pick a random starting point. */
    /* (This cheats a little; the statistics for three-letter
       combinations beginning a word are different from the stats
       for the general population.  For example, this code happily
       generates "mmitify" even though no word in my dictionary
       begins with mmi. So what.) */
    for (pwnum=0; pwnum < n_passwords; pwnum++) {
	pik = drand48 ();	/* random number [0,1] */
	sumfreq = sigma;	/* sigma calculated by loadtris */
	ranno = (long)(pik * sumfreq); /* Weight by sum of frequencies. */
	sum = 0;
	for (c1=0; c1 < 26; c1++) {
	    for (c2=0; c2 < 26; c2++) {
		for (c3=0; c3 < 26; c3++) {
		    sum += tris[c1][c2][c3];
		    if (sum > ranno) { /* Pick first value */
			password[0] = 'a' + c1;
			password[1] = 'a' + c2;
			password[2] = 'a' + c3;
			c1 = c2 = c3 = 26; /* Break all loops. */
		    } /* if sum */
		} /* for c3 */
	    } /* for c2 */
	} /* for c1 */

	/* Do a random walk. */
	nchar = 3;		/* We have three chars so far. */
	while (nchar < password_length) {
	    password[nchar] = '\0';
	    password[nchar+1] = '\0';
	    c1 = password[nchar-2] - 'a'; /* Take the last 2 chars */
	    c2 = password[nchar-1] - 'a'; /* .. and find the next one. */
	    sumfreq = 0;
	    for (c3=0; c3 < 26; c3++)
		sumfreq += tris[c1][c2][c3];
	    /* Note that sum < duos[c1][c2] because
	       duos counts all digraphs, not just those
	       in a trigraph. We want sum. */
	    if (sumfreq == 0) { /* If there is no possible extension.. */
		break;	/* Break while nchar loop & print what we have. */
	    }
	    /* Choose a continuation. */
	    pik = drand48 ();
	    ranno = (long)(pik * sumfreq); /* Weight by sum of frequencies for row. */
	    sum = 0;
	    for (c3=0; c3 < 26; c3++) {
		sum += tris[c1][c2][c3];
		if (sum > ranno) {
		    password[nchar++] = 'a' + c3;
		    c3 = 26;	/* Break the for c3 loop. */
		}
	    } /* for c3 */
	} /* while nchar */
	printf ("%s\n", password);
    } /* for pwnum */
    exit (0);
}
