/*!\page LICENSE LICENSE

Copyright (C) 2003 by the Board of Trustees of Massachusetts Institute of
Technology, hereafter designated as the Copyright Owners.

License to use, copy, modify, sell and/or distribute this software and
its documentation for any purpose is hereby granted without royalty,
subject to the following terms and conditions:

1.  The above copyright notice and this permission notice must
appear in all copies of the software and related documentation.

2.  The names of the Copyright Owners may not be used in advertising or
publicity pertaining to distribution of the software without the specific,
prior written permission of the Copyright Owners.

3.  THE SOFTWARE IS PROVIDED "AS-IS" AND THE COPYRIGHT OWNERS MAKE NO
REPRESENTATIONS OR WARRANTIES, EXPRESS OR IMPLIED, BY WAY OF EXAMPLE, BUT NOT
LIMITATION.  THE COPYRIGHT OWNERS MAKE NO REPRESENTATIONS OR WARRANTIES OF
MERCHANTABILITY OR FITNESS FOR ANY PARTICULAR PURPOSE OR THAT THE USE OF THE
SOFTWARE WILL NOT INFRINGE ANY PATENTS, COPYRIGHTS TRADEMARKS OR OTHER
RIGHTS. THE COPYRIGHT OWNERS SHALL NOT BE LIABLE FOR ANY LIABILITY OR DAMAGES
WITH RESPECT TO ANY CLAIM BY LICENSEE OR ANY THIRD PARTY ON ACCOUNT OF, OR
ARISING FROM THE LICENSE, OR ANY SUBLICENSE OR USE OF THE SOFTWARE OR ANY
SERVICE OR SUPPORT.

LICENSEE shall indemnify, hold harmless and defend the Copyright Owners and
their trustees, officers, employees, students and agents against any and all
claims arising out of the exercise of any rights under this Agreement,
including, without limiting the generality of the foregoing, against any
damages, losses or liabilities whatsoever with respect to death or injury to
person or damage to property arising from or out of the possession, use, or
operation of Software or Licensed Program(s) by LICENSEE or its customers.

*/
/* This is the main part of the code */

#include "induct.h"
#include "sparse/spMatrix.h"
#include <string.h>
#include <ctype.h>

#define MAXPRINT 1
#define MAXCHARS 400
#define TIMESIZE 10

FILE *fp, *fp2, *fp3, *fptemp, *fb, *fROM;
int num_exact_mutual;
int num_fourfil;
int num_mutualfil;
int num_found;
int num_perp;
int forced = 0;  /* for debugging inside exact_mutual() */

char outfname[200];
char outfname2[200];

/* SRW */
charge *assignFil(SEGMENT*, int*, charge*);
double **MatrixAlloc(int, int, int);
void fillA(SYS*);
void old_fillM(SYS*);
void fillZ(SYS*);
#if SUPERCON == ON
void fillZ_diag(SYS*, double);
void set_rvals(SYS*, double);
#endif
double resistance(FILAMENT*, double);
/* int matherr(struct exception*); */
int countlines(FILE*);
static int local_notblankline(char*);
void savemats(SYS*);
void savecmplx(FILE*, char*, CX**, int, int);
void savecmplx2(FILE*, char*, CX**, int, int);
void formMZMt(SYS*);
void oldformMZMt(SYS*);
char* MattAlloc(int, int);
void formMtrans(SYS*);
void compare_meshes(MELEMENT*, MELEMENT*);
void cx_dumpMat_totextfile(FILE*, CX**, int, int);
void dumpMat_totextfile(FILE*, double**, int, int);
void dumpVec_totextfile(FILE*, double*, int);
void fillMrow(MELEMENT**, int, double*);
void dump_to_Ycond(FILE*, int, SYS*);
void saveCarray(FILE*, char*, double**, int, int);
int nnz_inM(MELEMENT**, int);
void dump_M_to_text(FILE*, MELEMENT**, int, int);
void dump_M_to_matlab(FILE*, MELEMENT**, int, int, char*);
void pick_ground_nodes(SYS*);
int pick_subset(strlist*, SYS*);
void concat4(char*, char*, char*, char*);

#ifdef SRW0814
SRWSECONDS
#endif

int
main(int argc, char **argv)
{

  double width, height, length, freq, freqlast;
  int Linc, Winc, Hinc, filnum;
  double ratio;
  GROUNDPLANE *plane;                   /* CMS 7/7/92 */
  FILAMENT *tmpf;
  SEGMENT  *tmps;
  SEGMENT *seg;
  NODES *node;
  int i,j,k,m, last, err;
  char fname[80], tempstr[10];
  double r_height, r_width;
  CX dumb;
  CX *vect, **pvect;         /* space needed by gmres */
  double tol = 1e-8;
  double ftimes[TIMESIZE];
  int maxiters, user_maxiters;
  CX *b, *x0;
  SYS *indsys;           /* holds all the big variables (inductance system) */
  CX **MtZM;
  int num_mesh, num_extern, num_fils,num_nodes, num_real_nodes, num_segs;
  int num_sub_extern;
  double totaltime;
  int dont_form_Z;       /* if fmin=0, don't from the L matrix */
  char *MRMt;            /* may be needed if ROM is requested */
  double **MRMtinvMLMt, **B, **C;
  double **romB, **romC;
  int actual_order;

  int num_planes, nonp, planemeshes, tree_meshes;           /* CMS 6/7/92 */

  int *meshsect;
  double fmin, fmax, logofstep;
  double *R;
  EXTERNAL *ext;
  ind_opts *opts;
  extern long memcount;

  /* SRW - XicTools version hack, xt_idstring is sed'ed to a version
     string. */
  {
    if (argc == 2 && !strcmp(argv[1], "--v")) {
        printf("%s\n", "XT_IDSTRING");
        exit (0);
    }
  }

  charge *chglist, *chgend, chgdummy;
  ssystem *sys;

  memcount = 0;

  for(i = 0; i < TIMESIZE; i++)
    ftimes[i] = 0;

  starttimer;

  indsys = (SYS *)MattAlloc(1, sizeof(SYS));
  indsys->externals = NULL;
  indsys->num_extern = 0;
  indsys->segment = NULL;
  indsys->nodes = NULL;
  indsys->planes = NULL;                               /* CMS 7/2/92 */
  indsys->logofstep = 0;
  indsys->opts = Parse_Command_Line(argc, argv);

  /*indsys->r_height = indsys->r_width = indsys->opts->filratio;*/

  /* set the type equal to that specified on command line for now */
  indsys->precond_type = indsys->opts->precond;

  /* degugging counters for calls to functions used in mutual() */
  num_exact_mutual = 0;
  num_fourfil = 0;
  num_mutualfil = 0;
  num_found = 0;
  num_perp = 0;
  
  opts = indsys->opts;

  tol = opts->tol;
  user_maxiters = opts->maxiters;

  if (opts->fname == NULL) {
    printf("No input file given.  Reading from stdin...\n");
    fp = stdin;
  }
  else if (strcmp(opts->fname,"-") == 0) {
    printf("Reading from stdin...\n");
    fp = stdin;
  }
  else {
    /* SRW -- read ascii file */
    fp = fopen(opts->fname, "r");
    if (fp == NULL) {
      printf("Couldn't open %s\n", opts->fname);
      exit(1);
    }
    printf("Reading from file: %s\n",opts->fname);
  }

  /* read in geometry */
  err = readGeom(fp, indsys);
  if (err != 0) 
    return err;

  fclose(fp);

  /*  fprintf(stdout,"Number of nodes before breaking up: %d\n",
      indsys->num_nodes); */
  
  /* regurgitate input file to stdout */
  if (opts->regurgitate == TRUE) {
    regurgitate(indsys);
  }

  if ((opts->makeFastCapFile & HIERARCHY) 
      && !(opts->makeFastCapFile & (SIMPLE | REFINED)))
    /* the hierarchy has been dumped already, and nothing more to do. so quit*/
    return 0;

  /* make a file suitable for keith's postscript maker */

  if (opts->makeFastCapFile & SIMPLE) {
    fprintf(stdout, "Making simple zbuf file...");
    fflush(stdout);
    concat4(outfname,"zbuffile",opts->suffix,"");
    concat4(outfname2,outfname,"_","shadings");
    writefastcap(outfname, outfname2, indsys);
    fprintf(stdout, "Done\n");
    if (! (opts->makeFastCapFile & REFINED) )
      /* we are done, after making fastcap files for visualization */
      return 0;
  }

  /* initialize Gaussian quadrature arrays for mutual(). */
  /* See dist_betw_fils.c */
  fill_Gquad();

  /* initialize Freeable allocation stuff for mutual terms lookup table */
  init_table();

  /* break each segment into filaments */
  num_fils = 0;
  chgend = &chgdummy;

  for(seg = indsys->segment; seg != NULL; seg = seg->next) 
    chgend = assignFil(seg, &num_fils, chgend);

  indsys->num_fils = num_fils;

  chgend->next = NULL;
  chglist = chgdummy.next;

  stoptimer;
  ftimes[0] = dtime;

  starttimer;

#if SUPERCON == ON
  set_rvals(indsys, 2*PI*indsys->fmin);
#endif

  /* set up multipole stuff */
  sys = SetupMulti(chglist, indsys);

#if 1 == 0
  if (indsys->opts->debug == ON && 1 == 0)  /* for debugging eval pass */
    dump_evalcnts(sys);
#endif

  stoptimer;
  ftimes[5] = dtime;

  if (indsys->opts->debug == ON) 
    printf("Time for Multipole Setup: %lg\n",dtime);

  starttimer;

  printf("Scanning graph to find fundamental circuits...\n");
  /* find all the necessary meshes */
  make_trees(indsys);

  /* clear node->examined and seg->is_deleted */
  clear_marks(indsys);

  /* find meshes in groundplane due to holes and put them at the front
     of the list of trees so that they are created by fillM() first so
     that they are marked before regular ground plane meshes  */
  find_hole_meshes(indsys);

  /* determine if a subset of columns is to be computed and assign 
     col_Yindex.  This will happen if -x option is used. */
  num_sub_extern = indsys->num_sub_extern
                       = pick_subset(opts->portlist, indsys);

  stoptimer;
  ftimes[6] = dtime;

  /* Write to Zc.mat only if we aren't only running for visualization */
  if ( !(opts->makeFastCapFile & (SIMPLE | REFINED)) 
       && !(opts->orderROM > 0 && opts->onlyROM)   ) {
    concat4(outfname,"Zc",opts->suffix,".mat");   /* put filnames together */
    /* SRW -- this is ascii data */
    fp3 = fopen(outfname, "w");
    if (fp3 == NULL) {
      printf("couldn't open file %s\n",outfname);
      exit(1);
    }

    for(ext = indsys->externals; ext != NULL; ext=ext->next) {
      /* printf("Row %d :  %s  to  %s\n",ext->Yindex,ext->source->node[0]->name,
         ext->source->node[1]->name); */

      if (ext->portname[0] == '\0')
        fprintf(fp3,"Row %d:  %s  to  %s\n",ext->Yindex+1,ext->name1,ext->name2);
      else
        fprintf(fp3,"Row %d:  %s  to  %s, port name: %s\n",
                ext->Yindex+1,ext->name1,ext->name2,ext->portname);
    }

    if (num_sub_extern != indsys->num_extern)
      for(ext = indsys->externals; ext != NULL; ext=ext->next) {
        if (ext->col_Yindex != -1)
          fprintf(fp3,"Col %d: port name: %s\n", 
                  ext->col_Yindex+1,ext->portname);
      }
  }
    
  /* count nodes */
  indsys->num_real_nodes = 0;
  indsys->num_nodes = 0;
  last = -1;
  for(node = indsys->nodes, i = 0; node != NULL; node = node->next, i++) {
    indsys->num_nodes++;
    if (getrealnode(node) == node) {
      indsys->num_real_nodes++;
      if (last + 1 != i) {
      /* printf("Non equivalent nodes must be listed first??, 
	 no I take that back.\n"); */
      /* exit(1); */
      }
      last = i;
    }
  }
  
  /* CMS 7/3/92 ------------------------------------------------------------*/
  planemeshes = 0;
  nonp = 0;

  for(plane = indsys->planes; plane != NULL; plane = plane->next){
    planemeshes = planemeshes + plane->numesh;
    if(plane->external == 0)
      nonp++;
  }
  /*------------------------------------------------------------------------*/

/* moved lower for shading 
  if (opts->makeFastCapFile & REFINED) {
    fprintf(stdout, "Making refined zbuf file...");
    fflush(stdout);

    concat4(outfname,"zbuffile2",opts->suffix,"");
    

    concat4(outfname2,outfname,"_","shadings");
    writefastcap(outfname, outfname2, indsys);
    fprintf(stdout, "Done\n");
  }
*/

  num_fils = indsys->num_fils;      /* setup multi may alter this */
  num_segs = indsys->num_segs;
  num_nodes = indsys->num_nodes;
  num_real_nodes = indsys->num_real_nodes;
  num_planes = indsys->num_planes;                    /* CMS 7/2/92 */

  /* figure out number of meshes */
  tree_meshes = indsys->tree_meshes = count_tree_meshes(indsys->trees);

  indsys->extra_meshes = tree_meshes;

#if 1==0 
  unimplemented junk
  indsys->extra_meshes = estimate_extra_meshes(indsys->trees, FILS_PER_MESH);
#endif

  num_mesh = num_fils - num_segs + indsys->extra_meshes + planemeshes;
                                                          /* CMS 7/2/92 */
  indsys->num_mesh = num_mesh;
  num_extern = count_externals(indsys->externals);
  if (num_extern != indsys->num_extern) {
    fprintf(stderr, "main:  discrepancy in num_extern and actual number\n");
    exit(1);
  }
  fmin = indsys->fmin;
  fmax = indsys->fmax;
  logofstep = indsys->logofstep;

  /*  dont_form_Z = fmin == 0 && opts->mat_vect_prod == DIRECT 
                        && opts->soln_technique == ITERATIVE; */

  dont_form_Z = fmin == 0 && opts->soln_technique == LUDECOMP;
  indsys->dont_form_Z = dont_form_Z;


  printf("Number of Groundplanes : %d \n",num_planes);        /* CMS 7/2/92 */
  printf("Number of filaments: %10d\n", num_fils);
  printf("Number of segments:  %10d\n", num_segs);
  printf("Number of nodes:     %10d\n", num_nodes);
  printf("Number of meshes:    %10d\n", num_mesh);
  printf("          ----from tree:                   %d \n",tree_meshes);
  printf("          ----from planes: (before holes)  %d \n",planemeshes);  
                                                            /* CMS 7/6/92 */
  printf("Number of conductors:%10d   (rows of matrix in Zc.mat) \n",
	 num_extern);
  printf("Number of columns:   %10d   (columns of matrix in Zc.mat) \n",
	 num_sub_extern);

  printf("Number of real nodes:%10d\n", num_real_nodes);  /* non equivalent */
  
/*  A = MatrixAlloc(num_real_nodes, num_fils, sizeof(double)); */
  if (opts->mat_vect_prod == DIRECT || opts->soln_technique == LUDECOMP) {
/*  indsys->M = MatrixAlloc(num_fils, num_mesh, sizeof(double)); */
    if (!dont_form_Z)
      indsys->Z = MatrixAlloc(num_fils, num_fils, sizeof(double));

    /* let's save space and allocate this later */
    /* indsys->MtZM = (CX **)MatrixAlloc(num_mesh, num_mesh, sizeof(CX)); */
  }
  indsys->FinalY = (CX **)MatrixAlloc(num_extern, num_sub_extern, sizeof(CX));
  b = (CX *)MattAlloc(num_mesh,sizeof(CX));
  x0 = (CX *)MattAlloc(num_mesh,sizeof(CX));
  indsys->R = (double *)MattAlloc(num_fils,sizeof(double));
  indsys->meshsect = (int *)MattAlloc((num_extern+nonp+1),sizeof(int));
  vect = (CX *)MattAlloc(num_mesh,sizeof(CX));
  pvect = (CX **)MattAlloc(num_mesh,sizeof(CX *));
  indsys->Mlist = (MELEMENT **)MattAlloc(num_mesh,sizeof(MELEMENT *));
  indsys->Mtrans = (MELEMENT **)MattAlloc(num_fils,sizeof(MELEMENT *));
  indsys->m_info = (Minfo *)MattAlloc(num_mesh,sizeof(Minfo));
  indsys->Precond = (PRE_ELEMENT **)MattAlloc(num_mesh, sizeof(PRE_ELEMENT *));
  if (1 == 1) {  /* let's always dump the residual history */
    indsys->resids = MatrixAlloc(num_sub_extern,user_maxiters, sizeof(double));
    indsys->resid_real = MatrixAlloc(num_sub_extern, user_maxiters, 
				     sizeof(double));
    indsys->resid_imag = MatrixAlloc(num_sub_extern, user_maxiters, 
				     sizeof(double));
    indsys->niters = (double *)MattAlloc(num_sub_extern, sizeof(double));
  }

/*
  if (opts->mat_vect_prod == DIRECT || opts->soln_technique == LUDECOMP) {
    for(i = 0; i < num_fils; i++) {
      for(j = 0; j < num_mesh; j++)
	indsys->M[i][j] = 0;
    }
  }
*/

  meshsect = indsys->meshsect;
  R = indsys->R;

  starttimer;
/*
  printf("filling A...\n");
  fillA(segment, A, num_segs);
*/

  printf("filling M...\n");
  fillM(indsys);  /* expects hole meshes to be marked */

  if (opts->makeFastCapFile & REFINED) {
    fprintf(stdout, "Making refined zbuf file...");
    fflush(stdout);
    concat4(outfname,"zbuffile2",opts->suffix,"");

    /* shadings file */
    concat4(outfname2,outfname,"_","shadings");

    /* output file to later turn into .ps. Note: this affects seg->is_deleted*/
    writefastcap(outfname, outfname2, indsys);
    fprintf(stdout, "Done\n");
    return 0;  /* we are done after making fastcap files for visualization */;
  }


  if (num_mesh != indsys->num_mesh)
    printf("Exact number of meshes after ground plane holes removed: %d\n",
	   indsys->num_mesh);

  num_mesh = indsys->num_mesh;  /* actual number of meshes */

  /* let's save a little space and allocate MZMt now. */
  if (!dont_form_Z 
      && (opts->mat_vect_prod == DIRECT || opts->soln_technique == LUDECOMP))
    indsys->MtZM = (CX **)MatrixAlloc(num_mesh, num_mesh, sizeof(CX));

  MtZM = indsys->MtZM;

  if (opts->dumpMats & MESHES) {
    if (opts->kind & MATLAB)
      dump_mesh_coords(indsys);
    else
      dump_ascii_mesh_coords(indsys);
  }

  formMtrans(indsys); /* Form M transpose by row*/

  printf("filling R and L...\n");
  fillZ(indsys);

  stoptimer;
  ftimes[1] = dtime;

  if (indsys->opts->debug == ON) 
    printf("Time to Form M and Z: %lg\n",dtime);

  printf("Total Memory allocated: %ld kilobytes\n",memcount/1024);

  if (indsys->opts->debug == ON) 
    printf("Memory used and freed by lookup table: %d kilobytes\n",
	   get_table_mem());

  /* free memory for lookup table */
  destroy_table();

  starttimer;
  choose_and_setup_precond(indsys);

  if (opts->dumpMats) {
    printf("saving some files disk...\n");
    savemats(indsys);
  }

  if (logofstep == 0.0) {
    printf("no frequency range data read!\n");
    exit(1);
  }

  if (fmin == 0) {
      printf("***First frequency is zero. Only the DC case will be run.***\n");
      if (!dont_form_Z)
        printf("Warning: First frequency is zero, but -sludecomp was not specified.\n\
      Use this setting to save time and memory.\n");
  }

  if (indsys->opts->debug == ON) {
    /* open Ycond.mat */
    concat4(outfname,"Ycond",opts->suffix,".mat");
    /* SRW -- this is binary data */
    fp = fopen(outfname, "wb");
    if (fp == NULL) {
      printf("couldn't open file %s\n",outfname);
      exit(1);
    }
  }

  /* open b.mat */
  if (opts->dumpMats != OFF) {
    concat4(outfname,"b",opts->suffix,".mat");
    /* SRW -- this is binary data */
    if ((fb = fopen(outfname,"wb")) == NULL)
      fprintf(stderr, "No open fb\n");
  }

  /* Model Order Reduction: create, compute and print the model if requested */
  if (opts->orderROM > 0) {
    double *dtemp;

    /* create the matrix whose inverse we will need */
    createMRMt(&MRMt, indsys);
    /* now create the input and output matrices for the state */
    B = MatrixAlloc(num_mesh, num_extern, sizeof(double));
    C = MatrixAlloc(num_mesh, num_extern, sizeof(double));
    for(ext = indsys->externals, j = 0;
        ext != NULL; ext = ext->next, j++) {
      int_list *elem;
      /* create C first; C = N, where Vs = N Vterminal */
      for(elem = ext->indices; elem != NULL; elem = elem->next) {
        if (elem->index > num_mesh || ext->Yindex > num_extern) {
	  fprintf(stderr, "Indexing into matrix C out of bounds\n");
	  exit(1);
	}
        C[elem->index][ext->Yindex] = elem->sign;
        B[elem->index][ext->Yindex] = elem->sign;
      }
    }

    if (indsys->opts->mat_vect_prod != MULTIPOLE && !indsys->dont_form_Z) {
      printf("multiplying M*(L)*transpose(M)for model order reduction\n");
      /* this form of storage isn't the best */
      formMLMt(indsys);      /*form (M^t)*(L)*M and store in indys->MtZM*/
      

      actual_order = ArnoldiROM(B, C, (double **)NULL, (char**)MRMt, num_mesh,
                                num_extern, num_extern, opts->orderROM, 
                                realMatVect, indsys, sys, chglist);
    }
    else if (indsys->opts->mat_vect_prod == MULTIPOLE)
      actual_order = ArnoldiROM(B, C, (double **)NULL, (char**)MRMt, num_mesh, 
                                num_extern, num_extern, opts->orderROM, 
                                realComputePsi, indsys, sys, chglist); 

    if (indsys->opts->debug == ON) {
#if 1==0
      /* open orig.mat */
      concat4(outfname,"orig",opts->suffix,".mat");
      /* SRW -- this is binary data */
      if ((fROM = fopen(outfname,"wb")) == NULL)
        fprintf(stderr, "No open fROM\n");
      /* dump what we have of the original system */
      dumpROMbin(fROM, NULL, B, C, NULL,
                 num_mesh, num_extern, num_extern);
      fclose(fROM);
#endif

    /* open rom.m */
      concat4(outfname,"rom",opts->suffix,".m");
      /* SRW -- this is ascii data */
      if ((fROM = fopen(outfname,"w")) == NULL)
        fprintf(stderr, "No open fROM\n");

    /* now dump the reduced order model */
      dumpROM(fROM, indsys->Ar, indsys->Br, indsys->Cr, indsys->Dr,
              actual_order * num_extern, num_extern, num_extern);
      /* close and save away the file */
      fclose(fROM);
    }

    /* generate equivalent circuit */
    concat4(outfname,"equiv_circuitROM",opts->suffix,".spice");
    /* SRW -- this is ascii data */
    if ((fROM = fopen(outfname,"w")) == NULL)
      fprintf(stderr, "No open fROM\n");
    /* now dump the reduced order model */
    dumpROMequiv_circuit(fROM, indsys->Ar, indsys->Br, indsys->Cr, indsys->Dr,
            actual_order * num_extern, num_extern, num_extern, 
            indsys->title, opts->suffix, indsys);
    /* close and save away the file */
    fclose(fROM);
    /* end of Model Order Reduction generation */
  }

  /* NOTE: exit here if all we wnat is the reduced order model (ROM) */
  if (opts->orderROM > 0 && opts->onlyROM)
    exit(0);

  for(freq = fmin, m = 0; 
      (fmin != 0 && freq <= fmax*1.001) || (fmin == 0 && m == 0);
      m++, freq = (fmin != 0 ? pow(10.0,log10(fmin) + m*logofstep) : 0.0)) {
    printf("Frequency = %lg\n",freq);

    starttimer;
#if SUPERCON == ON
    if (freq != fmin)
      fillZ_diag(indsys, 2*PI*freq);
#endif

    if (!dont_form_Z 
        && (opts->mat_vect_prod == DIRECT || opts->soln_technique==LUDECOMP)) {
      printf("multiplying M*(R + jL)*transpose(M)\n");
      formMZMt(indsys);      /*form transpose(M)*(R+jL)*M  (no w) */

      if (opts->dumpMats & MZMt) {
	if (m == 0) {
	  if (opts->kind & MATLAB) {
            concat4(outfname,"MZMt",opts->suffix,".mat");
	    /* SRW -- this is binary data */
	    if ( (fp2 = fopen(outfname,"wb")) == NULL) {
	      printf("Couldn't open file\n");
	      exit(1);
	    }
	    printf("Saving MZMt...\n");
	    savecmplx2(fp2,"MZMt",indsys->MtZM, indsys->num_mesh,indsys->num_mesh);
	    fclose(fp2);
	  }
	  if (opts->kind & TEXT) {
            concat4(outfname,"MZMt",opts->suffix,".dat");
	    /* SRW -- this is ascii data */
	    if ( (fp2 = fopen(outfname,"w")) == NULL) {
	      printf("Couldn't open file\n");
	      exit(1);
	    }
	    cx_dumpMat_totextfile(fp2, indsys->MtZM,
				  indsys->num_mesh,indsys->num_mesh );
	    fclose(fp2);
	  }
	}
      }

      printf("putting in frequency \n");
      
      /* put in frequency */
      for(i = 0; i < num_mesh; i++)
	for(j = 0; j < num_mesh; j++)
	  MtZM[i][j].imag *= 2*PI*freq;
    }

    stoptimer;
    ftimes[2] += dtime;

    starttimer;

    if (opts->soln_technique == ITERATIVE) {
      if (indsys->precond_type == LOC) {
	printf("Forming local inversion preconditioner.\n");
	
	if (opts->mat_vect_prod == DIRECT)
	  indPrecond_direct(sys, indsys, 2*PI*freq);
	else if (opts->mat_vect_prod == MULTIPOLE)
	  indPrecond(sys, indsys, 2*PI*freq);
	else {
	  fprintf(stderr, "Internal error: mat_vect_prod == %d\n",
		  opts->mat_vect_prod);
	  exit(1);
	}
      }
      else if (indsys->precond_type == SPARSE) {
	printf("Forming sparse matrix preconditioner.\n");
	fill_spPre(sys, indsys, 2*PI*freq);

	if (m == 0) {
#ifdef SRW0814
	  double stime = srw_seconds(), etime;
#endif
	  if (indsys->opts->debug == ON)
	    printf("Number of nonzeros before factoring: %d\n",
		   spElementCount(indsys->sparMatrix));


	  /* Reorder and Factor the matrix */
	  err = spOrderAndFactor(indsys->sparMatrix, NULL, 1e-3, 0.0, 1);
#ifdef SRW0814
	  etime = srw_seconds();
	  printf("Reorder and factor time %g\n", etime - stime);
      fflush(stdout);
#endif

	  if (indsys->opts->debug == ON)
	    printf("Number of fill-ins after factoring: %d\n",
		   spFillinCount(indsys->sparMatrix));
	}
	else
	  err = spFactor(indsys->sparMatrix);

	if (err != 0) {
	  fprintf(stderr,"Error on factor: %d\n",err);
	  exit(1);
	}
      }
    }
    else if (opts->soln_technique == LUDECOMP) {
      if (!dont_form_Z) {
        printf("Performing LU Decomposition...\n");
        cx_ludecomp(MtZM, num_mesh, FALSE);
        printf("Done.\n");
      }
      else {
        /* let's form a sparse version since fmin=0 and the L matrix = 0 */
        create_sparMatrix(indsys);
        printf("Filling sparse version M R Mt...");
        fill_diagR(indsys);

        /* dump matrix to disk if requested */
        if (indsys->opts->dumpMats & MZMt) {
          printf("saving sparse MZMt to MZMt.mat...\n");
          concat4(outfname,"MZMt",indsys->opts->suffix,".mat");
          if (spFileMatrix(indsys->sparMatrix, outfname, "MZMt", 0, 1, 1) == 0)
            fprintf(stderr,"saving sparse matrix failed\n");
        }

        if (indsys->opts->debug == ON)
          printf("Number of nonzeros before factoring: %d\n",
                 spElementCount(indsys->sparMatrix));

        /* Reorder and Factor the matrix */
        /* since w = 0, this matrix is real but all complex computations 
           are done since that is how the sparse package was compiled.
           But changing it doesn't help that much.  Must be dominated by
           reordering and such.*/
        printf("Factoring the sparse matrix...\n");
        err = spOrderAndFactor(indsys->sparMatrix, NULL, 1e-3, 0.0, 1);

        printf("Done factoring.\n");
        if (indsys->opts->debug == ON)
          printf("Number of fill-ins after factoring: %d\n",
                 spFillinCount(indsys->sparMatrix));
      }
    }
    else {
      fprintf(stderr, "Internal error: Unknown type of soln_technique: %d\n",
	      opts->soln_technique);
      exit(1);
    }

    stoptimer;
    ftimes[3] += dtime;

    if (indsys->opts->debug == ON)
      printf("Time spent on forming Precond: %lg\n",dtime);

    starttimer;
    for(ext = get_next_ext(indsys->externals), i=0; ext != NULL; 
	                       ext = get_next_ext(ext->next),i++) {
      printf("conductor %d from node %s\n",i, get_a_name(ext->source));

      /* initialize b */
      for(j = 0; j < num_mesh; j++)
	b[j] = x0[j] = CXZERO;
      
      fill_b(ext, b);

      sprintf(fname, "b%d_%d",m,i);
      if (opts->dumpMats != OFF)
	savecmplx(fb, fname, &b, 1, num_mesh);

      maxiters = MIN(user_maxiters, num_mesh+2);

#if OPCNT == ON
      maxiters = 2;
#endif

      if (opts->soln_technique == ITERATIVE) {
	printf("Calling gmres...\n");
	if (opts->mat_vect_prod == MULTIPOLE) 
	  gmres(MtZM, b, x0, inner, SetupComputePsi, num_mesh, maxiters, tol,
		sys, chglist, 2*PI*freq, R, indsys, i);
	else 
	  gmres(MtZM, b, x0, inner, directmatvec,    num_mesh, maxiters, tol,
		sys, chglist, 2*PI*freq, R, indsys, i);

	if (indsys->precond_type == LOC) {
	  multPrecond(indsys->Precond, x0, vect, num_mesh);
	  for (k=0; k < num_mesh; k++)
	    x0[k] = vect[k];
	}
	else if (indsys->precond_type == SPARSE)
	  spSolve(indsys->sparMatrix, (spREAL*)x0, (spREAL*)x0);
	  
      }
      else 
        if (!dont_form_Z)
          cx_lu_solve(MtZM, x0, b, num_mesh);
        else 
          spSolve(indsys->sparMatrix, (spREAL*)b, (spREAL*)x0);

      if (opts->dumpMats & GRIDS)
	/* Do stuff to look at groundplane current distribution */
	makegrids(indsys, x0, ext->Yindex, m);

      /* Extract appropriate elements of x0 that correspond to final Yc */
      extractYcol(indsys->FinalY, x0, ext, indsys->externals);

      if (indsys->opts->debug == ON) {
	/* SRW -- this is binary data */
	fptemp = fopen("Ytemp.mat", "wb");
	if (fptemp == NULL) {
	  printf("couldn't open file %s\n","Ytemp.mat");
	  exit(1);
	}
	strcpy(fname,"Ycond");
	sprintf(tempstr, "%d", m);
	strcat(fname,tempstr);
	savecmplx(fptemp, fname, indsys->FinalY, num_extern, num_sub_extern);
	fclose(fptemp);
      }

    }
    stoptimer;
    ftimes[4] += dtime;

    if (i != indsys->num_sub_extern) {
      fprintf(stderr, "Huh?  columns calculated = %d  and num extern = %d\n",
	      i, indsys->num_sub_extern);
      exit(1);
    }

    if (indsys->opts->debug == ON) {
      /* dump matrix to matlab file */
      dump_to_Ycond(fp, m, indsys);
    }

    /* dump matrix to text file */
    if (num_extern == num_sub_extern) {
      fprintf(fp3, "Impedance matrix for frequency = %lg %d x %d\n ", freq,
	      num_extern, num_extern);
      cx_invert(indsys->FinalY, num_extern);
    }
    else
      fprintf(fp3, "ADMITTANCE matrix for frequency = %lg %d x %d\n ", freq,
	      num_extern, num_sub_extern);
      
    cx_dumpMat_totextfile(fp3, indsys->FinalY, num_extern, num_sub_extern);
    fflush(fp3);
  }
  
  if (indsys->opts->debug == ON)
    fclose(fp);

  fclose(fp3);
  if (opts->dumpMats != OFF)
    fclose(fb);

  printf("\nAll impedance matrices dumped to file Zc%s.mat\n\n",opts->suffix);
  totaltime = 0;
  for(i = 0; i < TIMESIZE; i++) 
    totaltime += ftimes[i];

  if (indsys->opts->debug == ON) {
    printf("Calls to exact_mutual: %15d\n",num_exact_mutual);
    printf("         fourfils:     %15d\n",num_fourfil);
    printf("         mutualfil:    %15d\n",num_mutualfil);
    printf("Number found in table: %15d\n",num_found);
    printf("Number perpendicular:  %15d\n",num_perp);
    printf("\n");
  }

  printf("Times:  Read geometry   %lg\n",ftimes[0]);
  printf("        Multipole setup %lg\n",ftimes[5]);
  printf("        Scanning graph  %lg\n",ftimes[6]);
  printf("        Form A M and Z  %lg\n",ftimes[1]);
  printf("        form M'ZM       %lg\n",ftimes[2]);
  printf("        Form precond    %lg\n",ftimes[3]);
  printf("        GMRES time      %lg\n",ftimes[4]);
  printf("   Total:               %lg\n",totaltime);

#ifdef MATTDEBUG
  /* print memory bins */
    for(i = 0; i < 1001; i++)
      fprintf(stderr, "%d\n", membins[i]);
#endif

  return (0);
}

/* this will divide a rectangular segment into many filaments */
charge *assignFil(SEGMENT *seg, int *num_fils, charge *chgptr)
{

  int i,j,k;
  double x,y,z, delw, delh;
  double hx, hy, hz;
  int temp, counter;
  int Hinc, Winc;
  double Hdiv, Wdiv;
  double height, width;
  double h_from_edge, w_from_edge, min_height, min_width;
  FILAMENT *tempfil, *filptr;
  int countfils;
  double wx, wy, wz, mag;  /* direction of width */
  NODES *node0, *node1;
  charge *clast, *chg;
  charge *ctemp;  /* temporary place so can allocate all the charges at once*/
  surface *dummysurf;
  int indices[4], row, col;
  double r_width, r_height;
            /* ratio of element sizes (for geometrically increasing size)*/
 
  Hinc = seg->hinc;
  Winc = seg->winc;
  r_height = seg->r_height;
  r_width = seg->r_width;

  clast = chgptr;

  seg->num_fils = Winc*Hinc;
  seg->filaments = (FILAMENT *)MattAlloc(Hinc*Winc,sizeof(FILAMENT));

  ctemp = (charge *)MattAlloc(Hinc*Winc, sizeof(charge));

  /* clear pchg as a marker for checking later */
  for(i = 0; i < Hinc*Winc; i++)
    seg->filaments[i].pchg = NULL;

  dummysurf = (surface *)MattAlloc(1, sizeof(surface));
  dummysurf->type = CONDTR;
  dummysurf->next = NULL;
  dummysurf->prev = NULL;
  
  /*  To make the filaments have geometrically decreasing areas */
  /* Hdiv and Wdiv are 1/(smallest Width) */

  if (fabs(1.0 - r_height) < EPS)
    Hdiv = Hinc;
  else {
    temp = Hinc/2;
    Hdiv = 2*(1.0 - pow(r_height, (double)temp))/(1.0 - r_height);
    if (Hinc%2 == 1) Hdiv += pow(r_height, (double)(temp));
  }

  if (fabs(1.0 - r_width) < EPS)
    Wdiv = Winc;
  else {
    temp = Winc/2;
    Wdiv = 2*(1.0 - pow(r_width, (double)temp))/(1.0 - r_width);
    if (Winc%2 == 1) Wdiv += pow(r_width, (double)(temp));
  }

  node0 = seg->node[0];
  node1 = seg->node[1];
  /* determine direction of width */
  if (seg->widthdir != NULL) {
    wx = seg->widthdir[XX];
    wy = seg->widthdir[YY];
    wz = seg->widthdir[ZZ];
  }
  else {
    /* default for width direction is in x-y plane perpendic to length*/
    /* so do cross product with unit z*/
    wx = -(node1->y - node0->y)*1.0;
    wy = (node1->x - node0->x)*1.0;
    wz = 0;
    if ( fabs(wx/seg->length) < EPS && fabs(wy/seg->length) < EPS) {
      /* if all of x-y is perpendic to length, then choose x direction */
      wx = 1.0;
      wy = 0;
    }
    mag = sqrt(wx*wx + wy*wy + wz*wz);
    wx = wx/mag;
    wy = wy/mag;
    wz = wz/mag;
  }
  /* height direction perpendicular to both */
  hx = -wy*(node1->z - node0->z) + (node1->y - node0->y)*wz;
  hy = -wz*(node1->x - node0->x) + (node1->z - node0->z)*wx;
  hz = -wx*(node1->y - node0->y) + (node1->x - node0->x)*wy;
  mag = sqrt(hx*hx + hy*hy + hz*hz);
  hx = hx/mag;
  hy = hy/mag;
  hz = hz/mag;

  filptr = seg->filaments;
  counter = 0;

  /* this will fill the 'filament' array. It uses symmetry wrt z and y */
  /* it generates the four corner fils first, then the next one in...  */
  /* 6/25/93 - added stuff to place fils in filament array so that adjacent */
  /*           fils are adjacent in the array.  This will make the meshes   */
  /*           in M consist of fils that are near each other.               */
  h_from_edge = 0.0;
  min_height = 1.0/Hdiv;  /* fil of smallest height */
  for(i = 0; i < Hinc/2 || (Hinc%2 == 1 && i == Hinc/2); i++) {

    /* height of the filaments for this row */
    height = (seg->height/Hdiv)*pow(r_height, (double)i);
    /* delh = (seg->height)*(0.5 - 1.0/Hdiv*
			  ( (1 - pow(r_height, (double)(i+1)))/(1-r_height)
			   - 0.5*pow(r_height, (double) i) )
			 );
			 */
    if (i == 0)
      h_from_edge += min_height/2;
    else
      h_from_edge += min_height/2*pow(r_height,(double)(i-1))*(1+r_height);

    delh = (seg->height)*(0.5 - h_from_edge);

    if (delh < 0.0 && fabs(delh/seg->height) > EPS) 
      printf("uh oh, delh < 0. delh/height = %lg\n", delh/seg->height);
    
    w_from_edge = 0;
    min_width = 1.0/Wdiv;
    for(j = 0; j < Winc/2 || (Winc%2 == 1 && j == Winc/2); j++) {
      width = (seg->width/Wdiv)*pow(r_width, (double)j );
      /*delw = (seg->width)*
	(0.5 - 1.0/Wdiv*
	 ( (1 - pow(r_width, (double)(j+1)))/(1 - r_width)
	  - 0.5*pow(r_width, (double) j) )
	 );
	 */

      if (j == 0)
	w_from_edge += min_width/2;
      else
	w_from_edge += min_width/2*pow(r_width,(double)(j-1))*(1+r_width);
      
      delw = (seg->width)*(0.5 - w_from_edge);

      if (delw < 0.0 && fabs(delw/seg->width) > EPS) 
	printf("uh oh, delw < 0. delw/width = %lg\n", delw/seg->width);
/*    tempfil = filptr; */
      countfils = 0;
      
      row = i; 
      col = j;
      if (row%2 == 1)
	col = (Winc - 1) - col;
      indices[countfils] = col + Winc*row;
      filptr = &(seg->filaments[indices[countfils]]);

      filptr->x[0] = node0->x + hx*delh + wx*delw;
      filptr->x[1] = node1->x + hx*delh + wx*delw;
      filptr->y[0] = node0->y + hy*delh + wy*delw;
      filptr->y[1] = node1->y + hy*delh + wy*delw;
      filptr->z[0] = node0->z + hz*delh + wz*delw;
      filptr->z[1] = node1->z + hz*delh + wz*delw;
      filptr->pchg = &ctemp[counter++];
/*    filptr++; */
      countfils++;
      
      /* do symmetric element wrt y */
      if(j != Winc/2) {
	row = i; 
	col = (Winc - 1) - j;
	if (row%2 == 1)
	  col = (Winc - 1) - col;
	indices[countfils] = col + Winc*row;
	filptr = &(seg->filaments[indices[countfils]]);

	filptr->x[0] = node0->x + hx*delh - wx*delw;
	filptr->x[1] = node1->x + hx*delh - wx*delw;
	filptr->y[0] = node0->y + hy*delh - wy*delw;
	filptr->y[1] = node1->y + hy*delh - wy*delw;
	filptr->z[0] = node0->z + hz*delh - wz*delw;
	filptr->z[1] = node1->z + hz*delh - wz*delw;
	filptr->pchg = &ctemp[counter++];
/*	filptr++; */
	countfils++;
      }
      
      /* wrt z */
      if(i != Hinc/2) {
        row = (Hinc - 1) - i; 
	col = j;
	if (row%2 == 1)
	  col = (Winc - 1) - col;
	indices[countfils] = col + Winc*row;
	filptr = &(seg->filaments[indices[countfils]]);

	filptr->x[0] = node0->x - hx*delh + wx*delw;
	filptr->x[1] = node1->x - hx*delh + wx*delw;
	filptr->y[0] = node0->y - hy*delh + wy*delw;
	filptr->y[1] = node1->y - hy*delh + wy*delw;
	filptr->z[0] = node0->z - hz*delh + wz*delw;
	filptr->z[1] = node1->z - hz*delh + wz*delw;
	filptr->pchg = &ctemp[counter++];
	filptr++;
	countfils++;
      }
      
      /* wrt z and y */
      if( i != Hinc/2 && j != Winc/2) {
	row = (Hinc - 1) - i; 
	col = (Winc - 1) - j;
	if (row%2 == 1)
	  col = (Winc - 1) - col;
	indices[countfils] = col + Winc*row;
	filptr = &(seg->filaments[indices[countfils]]);

	filptr->x[0] = node0->x - hx*delh - wx*delw;
	filptr->x[1] = node1->x - hx*delh - wx*delw;
	filptr->y[0] = node0->y - hy*delh - wy*delw;
	filptr->y[1] = node1->y - hy*delh - wy*delw;
	filptr->z[0] = node0->z - hz*delh - wz*delw;
	filptr->z[1] = node1->z - hz*delh - wz*delw;
	filptr->pchg = &ctemp[counter++];
/*	filptr++; */
	countfils++;
      }
      
      for(k = 0; k < countfils; k++) {
	tempfil = &(seg->filaments[indices[k]]);
	tempfil->length = seg->length;
	tempfil->area = width*height;
	tempfil->width = width;
	tempfil->height = height;
	tempfil->filnumber = (*num_fils)++;
	tempfil->segm = seg;

	tempfil->lenvect[XX] = tempfil->x[1] - tempfil->x[0];
	tempfil->lenvect[YY] = tempfil->y[1] - tempfil->y[0];
	tempfil->lenvect[ZZ] = tempfil->z[1] - tempfil->z[0];
	/* do stuff for multipole */
	/*   make linked list entry */
	chg = tempfil->pchg;
	clast->next = chg;
	clast = chg;
	/* fill charge structure */
	chg->max_diag = chg->min_diag = tempfil->length;
	chg->x = (tempfil->x[0] + tempfil->x[1])/2.0;
	chg->y = (tempfil->y[0] + tempfil->y[1])/2.0;
	chg->z = (tempfil->z[0] + tempfil->z[1])/2.0;
	chg->surf = dummysurf;
	chg->dummy = FALSE;
	chg->fil = tempfil;

/*	tempfil++; */
      }
      
    }
  }

  i = 0;
  while(i < Hinc*Winc && seg->filaments[i].pchg != NULL)
    i++;

  if (i != Hinc*Winc) {
    fprintf(stderr, "Hey, not all filaments created in assignfil()! \n");
    exit(1);
  }

  return clast;
}


double **MatrixAlloc(int rows, int cols, int size)
{

  double **temp;
  int i;

  temp = (double **)MattAlloc(rows,sizeof(double *));
  if (temp == NULL) {
        printf("not enough space for matrix allocation\n");
        exit(1);
      }

  for(i = 0; i < rows; i++) 
    temp[i] = (double *)MattAlloc(cols,size);

  if (temp[rows - 1] == NULL) {
        printf("not enough space for matrix allocation\n");
        exit(1);
      }
  return(temp);
}

void fillA(SYS *indsys)
{
  SEGMENT *seg;
  NODES *node1, *node2, *node;
  MELEMENT **Alist;
  int i, counter;
  FILAMENT *fil;

  indsys->Alist = (MELEMENT **)MattAlloc(indsys->num_real_nodes,
					 sizeof(MELEMENT *));

  pick_ground_nodes(indsys);

  Alist = indsys->Alist;
  counter = 1;  /* ground is chosen already */

  for(seg = indsys->segment; seg != NULL; seg = seg->next) {
    node1 = getrealnode(seg->node[0]);
    node2 = getrealnode(seg->node[1]);
    if (node1->index == -1) {
      node1->index = counter++;
      Alist[node1->index] = NULL;
    }
    if (node2->index == -1) {
      node2->index = counter++;
      Alist[node2->index] = NULL;
    }
    for(i = 0; i < seg->num_fils; i++) {
      fil = &seg->filaments[i];
      Alist[node1->index] = insert_in_list(make_melement(fil->filnumber, 
							 fil, 1),
					   Alist[node1->index]);
      Alist[node2->index] = insert_in_list(make_melement(fil->filnumber, 
							 fil, -1),
					   Alist[node2->index]);	   
    }
  }

  if (counter != indsys->num_real_nodes - indsys->num_trees + 1) {
    fprintf(stderr,"Internal error when forming A: counter %d != num_real_nodes %d\n",
	    counter, indsys->num_real_nodes);
  }

}

/* this fills the kircoff's voltage law matrix (Mesh matrix) */
/* it maps a matrix of mesh currents to branch currents */
/* it might actually be what some think of as the transpose of M */
/* Here, M*Im = Ib  where Im are the mesh currents, and Ib the branch */
/* 6/92 I added Mlist which is a vector of linked lists to meshes. 
   This replaces M.  But I keep M around for checking things in matlab. */
void old_fillM(SYS *indsys)
{
}
    
void fillZ(SYS *indsys)
{
  int i, j, k, m;
  FILAMENT *fil_j, *fil_m;
  int filnum_j, filnum_m;
  double w;
  SEGMENT *seg1, *seg2;
  double **Z, *R, freq;
  int num_segs;

  Z = indsys->Z;
  R = indsys->R;

  for(seg1 = indsys->segment; seg1 != NULL; seg1 = seg1->next) {
    for(j = 0; j < seg1->num_fils; j++) {
      fil_j = &(seg1->filaments[j]);
      filnum_j = fil_j->filnumber;
#if SUPERCON == ON
      R[filnum_j] = fil_j->length*seg1->r1/fil_j->area;
#else
      R[filnum_j] = resistance(fil_j, seg1->sigma);
#endif
      if (indsys->opts->mat_vect_prod != MULTIPOLE 
          && !indsys->dont_form_Z) {
	for(seg2 = indsys->segment; seg2 != NULL; seg2 = seg2->next) {
	  for(m = 0; m < seg2->num_fils; m++) {
	    fil_m = &(seg2->filaments[m]);
	    filnum_m = fil_m->filnumber;
	    if (filnum_m == filnum_j) {
	      Z[filnum_m][filnum_m] = selfterm(fil_m); /* do self-inductance */
#if SUPERCON == ON
	      if (seg1->lambda != 0.0)
	        Z[filnum_m][filnum_m] += seg1->r2*fil_j->length/fil_j->area;
#endif
	    }
	    else
	      if (filnum_m > filnum_j) /*we haven't done it yet */
		
		Z[filnum_m][filnum_j] 
		  = Z[filnum_j][filnum_m] = mutual(fil_j, fil_m);
	  }
	}
      }  /* end if (multipole or dont_form_Z) */
    }
  }
}

#if SUPERCON == ON
/* Have to reset the diag elements for each omega, if superconductor
 * (lambda != 0) and sigma != 0.
 */
void fillZ_diag(SYS *indsys, double omega)
{
  int i, j;
  FILAMENT *fil_j;
  int filnum_j;
  SEGMENT *seg1;
  double **Z, *R;
  double tmp, tmp1, dnom, r1, r2;

  Z = indsys->Z;
  R = indsys->R;

  for(seg1 = indsys->segment; seg1 != NULL; seg1 = seg1->next) {
    if (seg1->lambda != 0.0 && seg1->sigma != 0.0) {
      /* segment is a superconductor with frequency dependent terms */
      tmp = MU0*seg1->lambda*seg1->lambda;
      tmp1 = tmp*omega;
      dnom = tmp1*seg1->sigma;
      dnom = dnom*dnom + 1.0;
      seg1->r1 = r1 = seg1->sigma*tmp1*tmp1/dnom;
      seg1->r2 = r2 = tmp/dnom;
      for(j = 0; j < seg1->num_fils; j++) {
        fil_j = &(seg1->filaments[j]);
        filnum_j = fil_j->filnumber;
        R[filnum_j] = fil_j->length*r1/fil_j->area;
        Z[filnum_j][filnum_j] = selfterm(fil_j) +
          r2*fil_j->length/fil_j->area;
      }
    }
  }
}

/*
 * Theory:
 *    In normal metal:     (1)  del X H = -i*omega*mu*sigma * H
 *    In superconductor:   (2)  del X H = (1/lambda)^2 * H
 * 
 *    In fasthenry, (1) is solved, so the game is to replace sigma in (1)
 *    with a complex variable that includes and reduces to (2).  We choose
 * 
 *    sigma_prime = sigma + i/(omega*mu*lambda^2)
 * 
 *    Then, using sigma_prime in (1) rather than sigma, one obtains an
 *    expression that reduces to (2) at omega = 0,  yet retains properties
 *    of (1).  This is the two-fluid model, where the sigma in sigma_prime
 *    represents the conductivity due to unpaired electrons.
 * 
 *    Since sigma_prime blows up at omega = 0, we work with the impedance,
 *    which we take as z = r1 + i*omega*r2 = i/sigma_prime.  The r1 and
 *    r2 variables are thus
 * 
 *           (3) r1 =    sigma*(omega*mu*lambda^2)^2
 *                    --------------------------------
 *                    (sigma*omega*mu*lambda^2)^2 + 1
 * 
 *           (4) r2 =           mu*lambda^2
 *                    --------------------------------
 *                    (sigma*omega*mu*lambda^2)^2 + 1
 */

/* Set the r1, r2 entries to the appropriate values.  The r1 entry
 * comes from the real value of sigma (1/sigma for normal metals).
 * The r2 entry comes from the imaginary part of sigma, which reduces
 * to MU0*lambda^2 when the real part of sigma (default 0 for
 * superconductors) is negligible, and is 0 for normal metals.
 * The r2 term contributes to the inductance matrix diagonal.
 */
void set_rvals(SYS *indsys, double omega)
{
  SEGMENT *seg1;
  double tmp, tmp1, dnom, r1, r2;

  for(seg1 = indsys->segment; seg1 != NULL; seg1 = seg1->next) {
    if (seg1->lambda != 0.0) {
      /* segment is a superconductor */
      tmp = MU0*seg1->lambda*seg1->lambda;
      if (seg1->sigma != 0.0) {
        /* the terms become frequency dependent */
        tmp1 = tmp*omega;
        dnom = tmp1*seg1->sigma;
        dnom = dnom*dnom + 1.0;
        r1 = seg1->sigma*tmp1*tmp1/dnom;
        r2 = tmp/dnom;
      }
      else {
        r1 = 0.0;
        r2 = tmp;
      }
    }
    else {
      r1 = 1/seg1->sigma;
      r2 = 0.0;
    }
    seg1->r1 = r1;
    seg1->r2 = r2;
  }
}
#endif

/* calculates resistance of filament */
double resistance(FILAMENT *fil, double sigma)
/* double sigma;  conductivitiy */
{
  return  fil->length/(sigma * fil->area);
}

/* mutual inductance functions moved to mutual.c */

/*
int matherr(struct exception *exc)
{
  printf("Err in math\n");
  return(0);
}
*/

/* This counts the nonblank lines of the file  fp (unused) */
int countlines(FILE *fp)
{
  int count;
  char temp[MAXCHARS], *returnstring;

  count = 0;
  while( fgets(temp,MAXCHARS, fp) != NULL)
    if ( local_notblankline(temp) ) count++;

  return count;
}

/* returns 1 if string contains a nonspace character */
static int local_notblankline(char *string)
{
   while( *string!='\0' && isspace(*string))
     string++;

   if (*string == '\0') return 0;
     else return 1;
}

/* This saves various matrices to files and optionally calls fillA() if
   the incidence matrix, A, is requested */
void savemats(SYS *indsys)
{
  int i, j;
  FILE *fp, *fp2;
  FILAMENT *tmpf;
  SEGMENT *tmps;
  double *dumb, *dumbx, *dumby, *dumbz;
  int num_mesh, num_fils, num_real_nodes;
  double *Mrow;   /* one row of M */
  double **Z, *R;
  int counter, nnz, nnz0;
  ind_opts *opts;
  MELEMENT *mesh;
  MELEMENT **Mlist = indsys->Mlist;
  MELEMENT **Alist;

  num_mesh = indsys->num_mesh;
  num_fils = indsys->num_fils;
  num_real_nodes = indsys->num_real_nodes;

  Mrow = (double *)MattAlloc(num_fils,sizeof(double));
  R = indsys->R;
  Z = indsys->Z;
  opts = indsys->opts;

  if (opts->dumpMats & DUMP_M) {
    /* count non-zero entries */
    nnz = nnz_inM(indsys->Mlist, num_mesh);

    if (opts->kind & TEXT) {
      concat4(outfname,"M",opts->suffix,".dat");
      /* SRW -- this is ascii data */
      if ( (fp2 = fopen(outfname,"w")) == NULL) {
	printf("Couldn't open file\n");
	exit(1);
      }
      dump_M_to_text(fp2, Mlist, num_mesh, nnz);
      fclose(fp2);
    }

    if (opts->kind & MATLAB) {
      concat4(outfname,"M",opts->suffix,".mat");
      /* SRW -- this is binary data */
      if ( (fp = fopen(outfname,"wb")) == NULL) {
	printf("Couldn't open file\n");
	exit(1);
      }
      dump_M_to_matlab(fp, Mlist, num_mesh, nnz, "M");
      fclose(fp);
    }
  }

  if (opts->dumpMats & DUMP_A) {

    /* fill the A matrix */
    fillA(indsys);
    Alist = indsys->Alist;

    /* count non-zero entries */
    nnz = nnz_inM(indsys->Alist, num_real_nodes);

    /* how many non-zeros in ground node 0? */
    nnz0 = nnz_inM(indsys->Alist, 1);  

    /* now dump it to a file without ground node */

    if (opts->kind & TEXT) {
      concat4(outfname,"A",opts->suffix,".dat");
      /* SRW -- this is ascii data */
      if ( (fp2 = fopen(outfname,"w")) == NULL) {
	printf("Couldn't open file\n");
	exit(1);
      }
      dump_M_to_text(fp2, &Alist[1], num_real_nodes - 1, nnz - nnz0);
      fclose(fp2);
    }

    if (opts->kind & MATLAB) {
      concat4(outfname,"A",opts->suffix,".mat");
      /* SRW -- this is binary data */
      if ( (fp = fopen(outfname,"wb")) == NULL) {
	printf("Couldn't open file\n");
	exit(1);
      }
      dump_M_to_matlab(fp, &Alist[1], num_real_nodes - 1, nnz - nnz0,"A");
      fclose(fp);
    }
  }

  /* save text files */
  if (opts->kind & TEXT && opts->dumpMats & DUMP_RL) {
    /* SRW -- this is ascii data */
    /*
    if ( (fp2 = fopen("M.dat","w")) == NULL) {
      printf("Couldn't open file\n");
      exit(1);
    }

    for(i = 0; i < num_fils; i++)
      Mrow[i] = 0;

    for(i = 0; i < num_mesh; i++) {
      fillMrow(indsys->Mlist, i, Mrow);
      dumpVec_totextfile(fp2, Mrow, num_fils);
    }
    
    fclose(fp2);
    */

    concat4(outfname,"R",opts->suffix,".dat");
    /* SRW -- this is ascii data */
    if ( (fp2 = fopen(outfname,"w")) == NULL) {
      printf("Couldn't open file\n");
      exit(1);
    }
    
    dumpVec_totextfile(fp2, R, num_fils);
    
    fclose(fp2);
    
    if (!indsys->dont_form_Z && indsys->opts->mat_vect_prod == DIRECT) {
      /* do imaginary part of Z */
      
      concat4(outfname,"L",opts->suffix,".dat");
      /* SRW -- this is ascii data */
      if ( (fp2 = fopen(outfname,"w")) == NULL) {
	printf("Couldn't open file\n");
	exit(1);
      }
      
      dumpMat_totextfile(fp2, Z, num_fils, num_fils);
      
      fclose(fp2);
      
    }
    else {
      if (indsys->dont_form_Z)
        printf("L matrix not dumped since L = 0 since fmin = 0\n");
      else
        printf("L matrix not dumped.  Run with mat_vect_prod = DIRECT if this is desired\n");
    }
  }

  /* Save Matlab files */
  
  if (opts->kind & MATLAB && opts->dumpMats & DUMP_RL) {
    concat4(outfname,"RL",opts->suffix,".mat");
    /* SRW -- this is binary data */
    if ( (fp = fopen(outfname,"wb")) == NULL) {
      printf("Couldn't open file\n");
      exit(1);
    }

    dumbx =  (double *)malloc(num_fils*sizeof(double));
    dumby =  (double *)malloc(num_fils*sizeof(double));
    dumbz =  (double *)malloc(num_fils*sizeof(double));
    dumb =  (double *)malloc(num_fils*sizeof(double));
    if (dumb == NULL) {
      printf("no space for R\n");
      exit(1);
    }
    
    /* save and print matrices */

    /*
    for(i = 0; i < num_fils; i++)
      Mrow[i] = 0;

    for(i = 0; i < num_mesh; i++) {
      fillMrow(indsys->Mlist, i, Mrow);
      savemat_mod(fp, machine_type()+100, "M", num_mesh, num_fils, 0, Mrow, 
		  (double *)NULL, i, num_fils);
    }
    */

    /* this only saves the real part (savemat_mod.c modified) */
    savemat_mod(fp, machine_type()+100, "R", 1, num_fils, 0, R, (double *)NULL, 
		0, num_fils);
    
    if (!indsys->dont_form_Z && indsys->opts->mat_vect_prod == DIRECT) {
      /* do imaginary part of Z */
      for(i = 0; i < num_fils; i++) {
	savemat_mod(fp, machine_type()+100, "L", num_fils, num_fils, 0, Z[i], 
		    (double *)NULL, i, num_fils);
      }
    }
    else {
      if (indsys->dont_form_Z)
        printf("L matrix not dumped since L = 0 since fmin = 0\n");
      else
        printf("L matrix not dumped.  Run with mat_vect_prod = DIRECT if this is desired\n");
    }
    
    /* save vector of filament areas */
    for(tmps = indsys->segment; tmps != NULL; tmps = tmps->next) {
      for(j = 0; j < tmps->num_fils; j++) {
	tmpf = &(tmps->filaments[j]);
	dumb[tmpf->filnumber] = tmpf->area;
	dumbx[tmpf->filnumber] = tmpf->x[0];
	dumby[tmpf->filnumber] = tmpf->y[0];
	dumbz[tmpf->filnumber] = tmpf->z[0];
      }
    }
    savemat_mod(fp, machine_type()+0, "areas", num_fils, 1, 0, dumb,
        (double *)NULL,0, num_fils);
    savemat_mod(fp, machine_type()+0, "pos", num_fils, 3, 0, dumbx,
        (double *)NULL, 0, num_fils);
    savemat_mod(fp, machine_type()+0, "pos", num_fils, 3, 0, dumby,
        (double *)NULL, 1, num_fils);
    savemat_mod(fp, machine_type()+0, "pos", num_fils, 3, 0, dumbz,
        (double *)NULL, 1, num_fils);
    
    free(dumb);
    free(dumbx);
    free(dumby);
    free(dumbz);
    
    fclose(fp);
  }
}

void savecmplx(FILE *fp, char *name, CX **Z, int rows, int cols)
{
  int i,j;

  /* this only saves the real part (savemat_mod.c modified) one byte per call*/
  for(i = 0; i < rows; i++) 
    for(j = 0; j < cols; j++) 
      savemat_mod(fp, machine_type()+100, name, rows, cols, 1, &Z[i][j].real, 
		  (double *)NULL, i+j, 1);

  /* do imaginary part of Z */
  for(i = 0; i < rows; i++) 
    for(j = 0; j < cols; j++)
      savemat_mod(fp, machine_type()+100, name, rows, cols, 0, &Z[i][j].imag, 
		  (double *)NULL, 1, 1);
}

/* saves a complex matrix more efficiently? */
void savecmplx2(FILE *fp, char *name, CX **Z, int rows, int cols)
{
  int i,j;
  static double *temp;
  static int colmax = 0;

  if (colmax < cols) {
    temp = (double *)malloc(cols*sizeof(double));
    colmax = cols;
  }

  /* this only saves the real part (savemat_mod.c modified) one byte per call*/
  for(i = 0; i < rows; i++) {
    for(j = 0; j < cols; j++)
      temp[j] = Z[i][j].real;
    savemat_mod(fp, machine_type()+100, name, rows, cols, 1, temp, 
		(double *)NULL, i, cols);
  }

  /* do imaginary part of Z */
  for(i = 0; i < rows; i++) {
    for(j = 0; j < cols; j++)
      temp[j] = Z[i][j].imag;
    savemat_mod(fp, machine_type()+100, name, rows, cols, 0, temp, 
		(double *)NULL, 1, cols);
  }
}

/* This computes the product M*Z*Mt in a better way than oldformMZMt */
void formMZMt(SYS *indsys)
{
  int m,n,p;
  double tempsum, tempR, tempsumR;
  static double *tcol = NULL;   /* temporary storage for extra rows */
  int i,j, k, mesh, mesh2, nodeindx;
  int nfils, nmesh;
  MELEMENT *melem, *melem2;
  MELEMENT *mt, *mt2;    /* elements of M transpose */
  double **M, **L, *R;
  CX **Zm, *tempZ;
  int rows, cols, num_mesh;
  MELEMENT **Mlist, **Mt;

  Zm = indsys->MtZM;
  L = indsys->Z;
  R = indsys->R;
  M = indsys->M;
  nfils = rows = indsys->num_fils;
  nmesh = cols = num_mesh = indsys->num_mesh;
  Mlist = indsys->Mlist;
  Mt = indsys->Mtrans;

  if (nmesh > nfils) {
    fprintf(stderr, "Uh oh, more meshes than filaments, I'm confused\n");
    exit(1);
  }

  /* allocate a temporary column for Z*Mt */
  if (tcol == NULL)
    tcol = (double *)MattAlloc(rows, sizeof(double));

  /* this does L*(Mt)_i where (Mt)_i is a single column (row) of Mt (M) and
     saves it in the temp space, tcol */
  for(mesh = 0; mesh < num_mesh; mesh++) {
    for(j = 0; j< nfils; j++)
      tcol[j] = 0;
    /* note, this next set of nested loops could be reversed, but I think
       this might be more efficient for pipelining, ? */
    for(melem = Mlist[mesh]; melem != NULL; melem = melem->mnext)
      for(j = 0; j < nfils; j++)
	tcol[j] += L[j][melem->filindex]*melem->sign;
    for(mesh2 = 0; mesh2 < num_mesh; mesh2++) {
      tempsum = 0;
      for(melem2 = Mlist[mesh2]; melem2 != NULL; melem2 = melem2->mnext)
	tempsum += melem2->sign*tcol[melem2->filindex];
      Zm[mesh2][mesh].imag = tempsum;
    }
  }

  /* R is diagonal, so M*R*Mt is easy */
  for(i = 0; i < num_mesh; i++)
    for(j = 0; j < num_mesh; j++)
      Zm[i][j].real = 0;

  for(i = 0; i < nfils; i++)
    for(mt = Mt[i]; mt != NULL; mt = mt->mnext) {
      tempR = R[i]*mt->sign;
      for(mt2 = Mt[i]; mt2 != NULL; mt2 = mt2->mnext)
	Zm[mt2->filindex][mt->filindex].real += mt2->sign*tempR;
    }
}

void oldformMZMt(SYS *indsys)
{
  int m,n,p;
  double tempsum;
  static CX **trows = NULL;   /* temporary storage for extra rows */
  int i,j, k, mesh, nodeindx;
  int nfils, nmesh;
  MELEMENT *melem, *melem2;
  double **M, **L, *R;
  CX **Zm;
  int rows, cols, num_mesh;
  MELEMENT **Mlist;

  Zm = indsys->MtZM;
  L = indsys->Z;
  R = indsys->R;
  M = indsys->M;
  nfils = rows = indsys->num_fils;
  nmesh = cols = num_mesh = indsys->num_mesh;
  Mlist = indsys->Mlist;

  if (nmesh > nfils) {
    fprintf(stderr, "Uh oh, more meshes than filaments, I'm confused\n");
    exit(1);
  }

  /* allocate some extra rows so we can use Zm as temp space */
  if (rows > cols && trows == NULL)
    trows = (CX **)MatrixAlloc(rows - cols, cols, sizeof(CX));

  /* this does L*Mt and saves it in Zm.real.  This could be done better i
     think (use the fact that MZMt is symmetric)
     Also, could only track through each mesh list once, and store
     temporary values in Zm.real as we go along. (someday) */
  for(mesh = 0; mesh < num_mesh; mesh++) {
    for(j = 0; j < nfils; j++) {
      tempsum = 0;
      for(melem = Mlist[mesh]; melem != NULL; melem = melem->mnext) 
	tempsum += L[j][melem->filindex]*melem->sign;
      if (j < nmesh)
	Zm[j][mesh].real = tempsum;
      else
	trows[j - nmesh][mesh].real = tempsum;
    }
  }

/*      savecmplx(fp, "step1", Zm, num_mesh, num_mesh); */

  /* this does M*(Zm.real) where Zm.real is hopefully L*Mt */
  for(mesh = 0; mesh < num_mesh; mesh++) {
    for(j = 0; j < nmesh; j++) {
      tempsum = 0;
      for(melem = Mlist[mesh]; melem != NULL; melem = melem->mnext) {
	if (melem->filindex < nmesh)
	  tempsum += Zm[melem->filindex][j].real*melem->sign;
	else
	  tempsum += trows[melem->filindex - nmesh][j].real*melem->sign;
      }
      Zm[mesh][j].imag = tempsum;
    }
  }
/*      savecmplx(fp, "step2", Zm, num_mesh, num_mesh); */
  
  /* R is diagonal, so M*R*Mt is easy */
  for(i = 0; i < num_mesh; i++) {
    for(j = i; j < num_mesh; j++) {
      tempsum = 0;
      /* brute force search for elements */
      for(melem = Mlist[i]; melem != NULL; melem = melem->mnext) {
	for(melem2 = Mlist[j]; melem2 != NULL; melem2 = melem2->mnext) {
	  if (melem2->filindex == melem->filindex) 
	    tempsum += melem->sign*R[melem->filindex]*melem2->sign;
	}
      }
      Zm[i][j].real = Zm[j][i].real = tempsum;
    }
  }
  /* don't free the temp space */    
  /*
  if (rows > cols) {
    for(i = 0; i < rows - cols; i++)
      free(trows[i]);
    free(trows);
  }
  */
}

char* MattAlloc(int number, int size)
{

  char *blah;

/*  blah = (char *)malloc(number*size); */
  CALLOC(blah, number*size, char, OFF, IND);

  if (blah == NULL) {
    fprintf(stderr, "MattAlloc: Couldn't get space. Needed %d\n",number*size);
    exit(1);
  }

  return blah;
}

/* forms the transpose of M.  Makes a linked list of each row.  Mlist is 
    a linked list of its rows. */
/* Note: This uses the same struct as Mlist but in reality, each linked list
   is composed of mesh indices, not fil indices. (filindex is a mesh index) */
void formMtrans(SYS *indsys)
{
  int i, j, count;
  MELEMENT *m, *mt, *mt2, mtdum;
  MELEMENT **Mlist, **Mtrans, **Mtrans_check;
  int meshes, fils;
  int last_filindex;

  fils = indsys->num_fils;
  meshes = indsys->num_mesh;
  Mlist = indsys->Mlist;
  Mtrans = indsys->Mtrans;

  /* clear it (should be garbage only) */
  for(i = 0; i < fils; i++)
    Mtrans[i] = NULL;

  for(j = 0; j < meshes; j++) {
    last_filindex = -1;
    for(m = Mlist[j]; m != NULL; m = m->mnext) {
      mt = make_melement(j, NULL, m->sign);
      Mtrans[m->filindex] = insert_in_list(mt,Mtrans[m->filindex]);
      if (last_filindex == m->filindex)
	printf("Internal Warning: Mesh %d contains the same filament multiple times\n",j);
      last_filindex = m->filindex;
    }
  }

#if 1==0
  /* this was the old inefficient way to from Mt */

  /* allocated for comparison */
  Mtrans_check = (MELEMENT **)MattAlloc(fils,sizeof(MELEMENT *));

  for(i = 0; i < fils; i++) {
    mt = &mtdum;
    /* scan through mesh list j looking for a filament i. (j,i) in M */
    for(j = 0; j < meshes; j++) {
      for(m = Mlist[j]; m->filindex < i && m->mnext != NULL; m = m->mnext)
	;
      if (m->filindex == i) {
	mt->mnext = make_melement(j, NULL, m->sign);
	mt = mt->mnext;
	if (m->mnext != NULL && m->mnext->filindex == i) {
	  for(count = 1; m->mnext->filindex == i; count++) {
	    m = m->mnext;
	    mt->mnext = make_melement(j, NULL, m->sign);
	    mt = mt->mnext;
	  }
	  printf("Internal Warning: Mesh %d contains the same filament %d times\n",j,count);
	} 
      }
    }
    mt->mnext = NULL;
    Mtrans_check[i] = mtdum.mnext;
  }

  /* check old and new ways */
  for(i = 0; i < fils; i++) {
    for(mt = Mtrans[i], mt2 = Mtrans_check[i]; mt != NULL && mt2 != NULL;
                  mt = mt->mnext, mt2 = mt2->mnext)
      if (mt->filindex != mt2->filindex || mt->sign != mt2->sign)
        printf("different: %d  %d %d\n",i,mt->filindex, mt2->filindex);
    if (mt != mt2) 
      printf("both not NULL:  mt: %u  mt2: %u\n", mt, mt2);
  }
#endif
      
}

void compare_meshes(MELEMENT *mesh1, MELEMENT *mesh2)
{

  while(mesh1 != NULL && mesh2 != NULL && mesh1->filindex == mesh2->filindex && mesh1->sign == mesh2->sign) {
    mesh1 = mesh1->mnext;
    mesh2 = mesh2->mnext;
  }

  if (mesh1 != NULL || mesh2 != NULL) {
    fprintf(stderr, "meshes don't match!\n");
    exit(1);
  }
}

void cx_dumpMat_totextfile(FILE *fp, CX **Z, int rows, int cols)
{
  int i, j;

  for(i = 0; i < rows; i++) {
    for(j = 0; j < cols; j++) 
      fprintf(fp, "%13.6lg %+13.6lgj ", Z[i][j].real, Z[i][j].imag);
    fprintf(fp, "\n");
  }
  return;
}

void dumpMat_totextfile(FILE *fp, double **A, int rows, int cols)
{
  int i, j;

  for(i = 0; i < rows; i++) {
    for(j = 0; j < cols; j++) 
      fprintf(fp, "%13.6lg ", A[i][j]);
    fprintf(fp, "\n");
  }
  return;
}

void dumpVec_totextfile(FILE *fp2, double *Vec, int size)
{
  dumpMat_totextfile(fp2, &Vec, 1, size);
}

void fillMrow(MELEMENT **Mlist, int mesh, double *Mrow)
{
  int i;
  MELEMENT *melem;

  if (mesh != 0)
    /* remove non-zeros from last call */
    for(melem = Mlist[mesh - 1]; melem != NULL; melem=melem->mnext)
      Mrow[melem->filindex] = 0;

  for(melem = Mlist[mesh]; melem != NULL; melem = melem->mnext)
    Mrow[melem->filindex] = melem->sign;
}

void dump_to_Ycond(FILE *fp, int cond, SYS *indsys)
{
  static char fname[20], tempstr[5];
  int maxiters = indsys->opts->maxiters;

  sprintf(tempstr, "%d", cond);

  strcpy(fname,"Ycond");
  strcat(fname,tempstr);
  savecmplx(fp, fname, indsys->FinalY, indsys->num_extern, 
	    indsys->num_sub_extern);

  strcpy(fname,"resids");
  strcat(fname,tempstr);
  saveCarray(fp, fname, indsys->resids, indsys->num_sub_extern, maxiters);

  strcpy(fname,"resid_real");
  strcat(fname,tempstr);
  saveCarray(fp, fname, indsys->resid_real, indsys->num_sub_extern, maxiters);

  strcpy(fname,"resid_imag");
  strcat(fname,tempstr);
  saveCarray(fp, fname, indsys->resid_imag, indsys->num_sub_extern, maxiters);

  strcpy(fname,"niters");
  strcat(fname,tempstr);
  saveCarray(fp, fname, &(indsys->niters), 1, indsys->num_sub_extern);

}

void saveCarray(FILE *fp, char *fname, double **Arr, int rows, int cols)
{
  int i;

  for(i = 0; i < rows; i++) {
    savemat_mod(fp, machine_type()+100, fname, rows, cols, 0, Arr[i],
        (double *)NULL, i, cols);
  }
}

int nnz_inM(MELEMENT **Mlist, int num_mesh)
{
  int counter, i;
  MELEMENT *mesh;

  counter = 0;

  for(i = 0; i < num_mesh; i++)
    for(mesh = Mlist[i]; mesh != NULL; mesh = mesh->mnext)
      counter++;

  return counter;
}

void dump_M_to_text(FILE *fp, MELEMENT **Mlist, int num_mesh, int nnz)
{
  int counter, i;
  MELEMENT *mesh;

  counter = 0;

  for(i = 0; i < num_mesh; i++)
    for(mesh = Mlist[i]; mesh != NULL; mesh = mesh->mnext) {
      fprintf(fp, "%d\t%d\t%d\n", i+1, mesh->filindex+1, mesh->sign);
      counter++;
    }

  if (counter != nnz)
    fprintf(stderr,"Internal Warning: nnz %d != counter %d\n",nnz,counter);
  
}

void dump_M_to_matlab(FILE *fp, MELEMENT **Mlist, int num_mesh, int nnz,
    char *mname)
{

  double onerow[3];
  int i, counter;
  MELEMENT *mesh;

  counter = 0;

  for(i = 0; i < num_mesh; i++) {
    onerow[0] = i + 1;
    for(mesh = Mlist[i]; mesh != NULL; mesh = mesh->mnext) {
      onerow[1] = mesh->filindex + 1;
      onerow[2] = mesh->sign;
      savemat_mod(fp, machine_type()+100, mname, nnz, 3, 0, onerow,
		  (double *)NULL, counter++, 3);
    }
  }

  if (counter != nnz)
    fprintf(stderr,"Internal Warning: nnz %d != counter %d\n",nnz,counter);
}  

/* this picks one node in each tree to be a ground node */
void pick_ground_nodes(SYS *indsys)
{
  TREE *atree;
  SEGMENT *seg;
  seg_ptr segp;
  char type;
  NODES *node;

  for(atree = indsys->trees; atree != NULL; atree = atree->next) {
    type = atree->loops->path->seg.type;
    if (type == NORMAL)
      node = getrealnode(((SEGMENT *)atree->loops->path->seg.segp)->node[0]);
    else
      node = getrealnode(((PSEUDO_SEG *)atree->loops->path->seg.segp)->node[0]);
      
    if (node->index != -1) {
      fprintf(stderr,"huh? index == %d in pick_ground_node\n",node->index);
      exit(1);
    }
    node->index = 0;
  }
}

int pick_subset(strlist *portlist, SYS *indsys)
{
  strlist *oneport;
  EXTERNAL *ext;
  int counter;

  if (portlist == NULL) {
    for(ext = indsys->externals; ext != NULL; ext=ext->next)
      ext->col_Yindex = ext->Yindex;
    return indsys->num_extern;
  }
    
  for(ext = indsys->externals; ext != NULL; ext=ext->next)
    ext->col_Yindex = -1;

  counter = 0;
  for(oneport = portlist; oneport != NULL; oneport = oneport->next) {
    ext = get_external_from_portname(oneport->str,indsys);
    if (ext == NULL) {
      fprintf(stderr,"Error: unknown portname %s\n",oneport->str);
      exit(1);
    }

    ext->col_Yindex = counter;
    counter++;
  }

  return counter;
}

/* concatenates so that s1 = s1 + s2 + s3 + s4 */
void concat4(char *s1, char *s2, char *s3, char *s4)
{
  s1[0] = '\0';
  strcat(s1,s2);
  strcat(s1,s3);
  strcat(s1,s4);
}

