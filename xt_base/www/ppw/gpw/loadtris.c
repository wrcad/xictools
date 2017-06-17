/* This program counts trigraphs in all the dictionaries specified
   and outputs a C++ include file of constants for gpw.C to use.

   Change to a C include file by changing "const" to "static."
   Output of this program needs postprocessing to eliminate comma closebrace,
   see the makefile for gpw.

   THVV 6/94 Coded
   */

#include "stdio.h"
#include "stdlib.h"

int tris[26][26][26];			/* Trigraph frequencies */
int duos[26][26];				/* Bigraph frequencies */
int sing[26];					/* Letter frequencies */
long max = 0;					/* largest triraph count */
int m1, m2, m3;					/* coords of largest count */
long sigma = 0;					/* Total letters */

FILE *fp;

/* SRW ** shorts are now ints */

int main (int argc, char ** argv) {
	char buf[100];
	int j;
	int k1, k2, k3;
	int c1, c2, c3;
	char s1[2], s2[2], s3[2];
	int argno, nfiles;

	for (c1=0; c1 < 26; c1++) {	/* Initialize arrays to zero */
		sing[c1] = 0;
		for (c2=0; c2 < 26; c2++) {
			duos[c1][c2] = 0;
			for (c3=0; c3 < 26; c3++) {
				tris[c1][c2][c3] = 0;
			}
		}
	}
	s1[1] = '\0';
	s2[1] = '\0';
	s3[1] = '\0';
	nfiles = 0;					/* count of files read */

	if (argc < 2) {
		printf (" USAGE: loadtris /usr/dict/words ...");
		exit (1);
	}
	for (argno = 1; argno < argc; argno++) {
		if ((fp = fopen (argv[argno], "r")) == NULL) {
			printf ("** file %s not found\n", argv[argno]);
			break;
		}
		nfiles++;
		while (fgets (buf, sizeof (buf), fp)) {
			j = 0;					/* j indexes the input */
			k2 = -1;				/* k1, k2 are coords of previous letter */
			k1 = -1;
			while (buf[j]) {		/* until we find the null char.. */
				k3 = buf[j];		/* Pick out a letter from the input */
				if (k3 > 'Z') {
					k3 = k3 - 'a';	/* map from a-z to 0-25 */
				}
				else {
					k3 = k3 - 'A';	/* map from A-Z to 0-25 */
				}
				if (k3 >= 0 && k3 <= 25) { /* valid subscript? */
					if (k1 >= 0) { /* do we have 3 letters? */
						tris[k1][k2][k3]++;	/* count */
						sigma++;			/* grand total */
						if (tris[k1][k2][k3] > max) {
							max = tris[k1][k2][k3];
							m1 = k1; /* note largest cell.. */
							m2 = k2; /* .. for interest */
							m3 = k3;
						}
					}
					if (k2 >= 0) {
						duos[k2][k3]++;	/* count 2-letter pairs */
					}
					sing[k3]++;		/* count single letter frequency */
					k1 = k2;		/* shift over */
					k2 = k3;
				}
				j++;
			}						/* while buf[j] */
		}							/* while fgets */
		fclose (fp);
	}							    /* for argno */

	if (nfiles) {				    /* find any input? */
		printf ("/* BEGIN INCLUDE FILE .. trigram.h */\n"); /* Multics style */
		printf ("\n");
		printf ("const long sigma = %ld;\n", sigma);
		/* (for my /usr/dict/words it is 125729, fits in a long) */

		/* For interest print out the most frequent entry. */
		/* (for my /usr/dict/words it is 863 = ATE, showing that a short works OK) */
		s1[0] = m1 + 'a';
		s2[0] = m2 + 'a';
		s3[0] = m3 + 'a';
		/* SRW printf ("const short maxcell = %ld; */
		printf ("const int maxcell = %ld; /* %s%s%s */\n", max, s1, s2, s3);

		/* SRW printf ("const short sing[26] = {"); */
		printf ("const int sing[26] = {");
		for (c1=0; c1 < 26; c1++) {
			printf ("%d, ", sing[c1]);
		}
		printf ("};\n");				/* oops, ends in comma closebrace */

		/* SRW printf ("const short duos[26][26] = {"); */
		printf ("const int duos[26][26] = {");
		for (c1=0; c1 < 26; c1++) {
			s1[0] = c1+'A';
			printf ("\n/* %s */ ", s1);
			for (c2=0; c2 < 26; c2++) {
				printf ("%d, ", duos[c1][c2]);
			}
		}
		printf ("};\n");				/* oops, ends in comma closebrace */

		/* SRW printf ("const short tris[26][26][26] = {"); */
		printf ("const int tris[26][26][26] = {");
		for (c1=0; c1 < 26; c1++) {
			for (c2=0; c2 < 26; c2++) {
				s1[0] = c1+'A';
				s2[0] = c2+'A';
				printf ("\n/* %s %s */ ", s1, s2);
				for (c3=0; c3 < 26; c3++) {
					printf ("%d, ", tris[c1][c2][c3]);
				}
			}
		}
		printf ("};\n");				/* comma closebrace again, fix later */
		printf ("/* END   INCLUDE FILE .. trigram.h */\n");
	}
	exit (0);
}

