


/*
 * THIS SOURCE CODE IS SUPPLIED  ``AS IS'' WITHOUT WARRANTY OF ANY KIND, 
 * AND ITS AUTHOR AND THE JOURNAL OF ARTIFICIAL INTELLIGENCE RESEARCH 
 * (JAIR) AND JAIR'S PUBLISHERS AND DISTRIBUTORS, DISCLAIM ANY AND ALL 
 * WARRANTIES, INCLUDING BUT NOT LIMITED TO ANY IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE, AND
 * ANY WARRANTIES OR NON INFRINGEMENT.  THE USER ASSUMES ALL LIABILITY AND
 * RESPONSIBILITY FOR USE OF THIS SOURCE CODE, AND NEITHER THE AUTHOR NOR
 * JAIR, NOR JAIR'S PUBLISHERS AND DISTRIBUTORS, WILL BE LIABLE FOR 
 * DAMAGES OF ANY KIND RESULTING FROM ITS USE.  Without limiting the 
 * generality of the foregoing, neither the author, nor JAIR, nor JAIR's
 * publishers and distributors, warrant that the Source Code will be 
 * error-free, will operate without interruption, or will meet the needs 
 * of the user.
 */








/*********************************************************************
 * File: inst_pre.c
 * Description: functions for instantiating operators, preprocessing part.
 *              - transform domain into integers
 *              - inertia preprocessing:
 *                  - collect inertia info
 *                  - split initial state in special arrays
 *              - Wff normalization:
 *                  - simplification
 *                  - quantifier expansion
 *                  - NOT s down
 *              - negative preconditions translation
 *              - split operators into easy and hard to instantiate
 *
 *              - full DNF functions, only feasible for fully instantiated 
 *                formulae
 *
 * Author: Joerg Hoffmann 2000
 *
 *********************************************************************/ 









#include "ff.h"

#include "output.h"
#include "memory.h"

#include "inst_pre.h"


















/*******************************************************
 * TRANSFORM DOMAIN INTO INTEGER (FACT) REPRESENTATION *
 *******************************************************/









char *lvar_names[MAX_VARS];
int lvar_types[MAX_VARS];










void encode_domain_in_integers( void )

{

  int i, j;

  collect_all_strings();

  if ( gcmd_line.display_info == 103 ) {
    printf("\nconstant table:");
    for ( i = 0; i < gnum_constants; i++ ) {
      printf("\n%d --> %s", i, gconstants[i]);
    }

    printf("\n\ntypes table:");
    for ( i = 0; i < gnum_types; i++ ) {
      printf("\n%d --> %s: ", i, gtype_names[i]);
      for ( j = 0; j < gtype_size[i]; j++ ) {
	printf("%d ", gtype_consts[i][j]);
      }
    }

    printf("\n\npredicates table:");
    for ( i = 0; i < gnum_predicates; i++ ) {
      printf("\n%3d --> %s: ", i, gpredicates[i]);
      for ( j = 0; j < garity[i]; j++ ) {
	printf("%s ", gtype_names[gpredicates_args_type[i][j]]);
      }
    }
    printf("\n\n");
  }

  create_integer_representation();

  if ( gcmd_line.display_info == 104 ) {
    printf("\n\nfirst step initial state is:");
    printf("\npositive:");
    for ( i = 0; i < gnum_full_initial; i++ ) {
      printf("\n");
      print_Fact( &(gfull_initial[i]) );
    }
    printf("\nunknown:");
    for ( i = 0; i < gnum_full_unknown_initial; i++ ) {
      printf("\n");
      print_Fact( &(gfull_unknown_initial[i]) );
      printf("\nCPT:\n");
      for ( j = 0; j < gnum_full_unknown_initial_cpt[i]; j++ ) {
	printf("prob: %f\n", gfull_unknown_initial_cpt_p[i][j]);
	printf("if: \n");
	print_Wff( gfull_unknown_initial_cpt_c[i][j], 0 );
      }
    }
    printf("\nmultis:\n");
    for ( i = 0; i < gnum_full_multi_initial; i++ ) {
      print_Wff( gfull_multi_initial[i], 0 );
    }
    printf("\nors:\n");
    for ( i = 0; i < gnum_full_or_initial; i++ ) {
      print_Wff( gfull_or_initial[i], 0 );
    }
    printf("\noneofs:\n");
    for ( i = 0; i < gnum_full_oneof_initial; i++ ) {
      print_Wff( gfull_oneof_initial[i], 0 );
    }
    
    printf("\n\nfirst step operators are:");
    for ( i = 0; i < gnum_operators; i++ ) {
      print_Operator( goperators[i] );
    }
    printf("\n\n");
    
    printf("\n\nfirst step goal is, P %f:\n", ggoal_probability);
    print_Wff( ggoal, 0 );
  }

}



void collect_all_strings( void )

{

  FactList *f;
  TokenList *t;
  int p_num, type_num, c_num, ar;
  int i;

  /* first are types and their objects. for = we make sure that there
   * is one type that contains all objects.
   */
  gtype_names[0] = new_Token( 50 );
  gtype_names[0] = "ARTFICIAL-ALL-OBJECTS";
  gtype_size[0] = 0;
  for ( i = 0; i < MAX_CONSTANTS; i++ ) {
    gis_member[i][0] = FALSE;
  }
  gnum_types = 1;

  for ( f = gorig_constant_list; f; f = f->next ) {
    if ( (type_num = position_in_types_table( f->item->next->item )) == -1 ) {
      if ( gnum_types == MAX_TYPES ) {
	printf("\ntoo many types! increase MAX_TYPES (currently %d)\n\n",
	       MAX_TYPES);
	exit( 1 );
      }
      gtype_names[gnum_types] = new_Token( strlen( f->item->next->item ) + 1 );
      strcpy( gtype_names[gnum_types], f->item->next->item );
      gtype_size[gnum_types] = 0;
      for ( i = 0; i < MAX_CONSTANTS; i++ ) {
	gis_member[i][gnum_types] = FALSE;
      }
      type_num = gnum_types++;
    }

    if ( (c_num = position_in_constants_table( f->item->item )) == -1 ) {
      if ( gnum_constants == MAX_CONSTANTS ) {
	printf("\ntoo many constants! increase MAX_CONSTANTS (currently %d)\n\n",
	       MAX_CONSTANTS);
	exit( 1 );
      }
      gconstants[gnum_constants] = new_Token( strlen( f->item->item ) + 1 );
      strcpy( gconstants[gnum_constants], f->item->item );
      c_num = gnum_constants++;

      /* all constants into 0-type.
       */
      if ( gtype_size[0] == MAX_TYPE ) {
	printf("\ntoo many consts in type %s! increase MAX_TYPE (currently %d)\n\n",
	       gtype_names[0], MAX_TYPE);
	exit( 1 );
      }     
      gtype_consts[0][gtype_size[0]++] = c_num;
      gis_member[c_num][0] = TRUE;
    }
    
    if ( !gis_member[c_num][type_num] ) {
      if ( gtype_size[type_num] == MAX_TYPE ) {
	printf("\ntoo many consts in type %s! increase MAX_TYPE (currently %d)\n\n",
	       gtype_names[type_num], MAX_TYPE);
	exit( 1 );
      }     
      gtype_consts[type_num][gtype_size[type_num]++] = c_num;
      gis_member[c_num][type_num] = TRUE;
    }
  }

  /* next are predicates; first of all, create in-built predicate =
   */
  gpredicates[0] = new_Token( 5 );
  gpredicates[0] = "=";
  gpredicates_args_type[0][0] = 0;/* all objects type */
  gpredicates_args_type[0][1] = 0;
  garity[0] = 2;
  gnum_predicates = 1;

  for ( f = gpredicates_and_types; f; f = f->next ) {
    if ( (p_num = position_in_predicates_table( f->item->item )) != -1 ) {
      printf("\npredicate %s declared twice!\n\n", f->item->item);
      exit( 1 );
    }
    if ( gnum_predicates == MAX_PREDICATES ) {
      printf("\ntoo many predicates! increase MAX_PREDICATES (currently %d)\n\n",
	     MAX_PREDICATES);
      exit( 1 );
    }
    gpredicates[gnum_predicates] = new_Token( strlen( f->item->item ) + 1 );
    strcpy( gpredicates[gnum_predicates], f->item->item );
    ar = 0;
    for ( t = f->item->next; t; t = t->next ) {
      if ( (type_num = position_in_types_table( t->item )) == -1 ) {
	printf("\npredicate %s is declared to use unknown or empty type %s\n\n", 
	       f->item->item, t->item);
	exit( 1 );
      }
      if ( ar == MAX_ARITY ) {
	printf("\narity of %s to high! increase MAX_ARITY (currently %d)\n\n",
	       gpredicates[gnum_predicates], MAX_ARITY);
	exit( 1 );
      }
      gpredicates_args_type[gnum_predicates][ar++] = type_num;
    }
    garity[gnum_predicates++] = ar;
  }

  free_FactList( gorig_constant_list );
  free_FactList( gpredicates_and_types );

}



int position_in_types_table( char *str )

{

  int i;

  /* start at 1 because 0 is our artificial one
   */
  for ( i=1; i<gnum_types; i++ ) {
    if ( str == gtype_names[i] || 
	 (strcmp( str, gtype_names[i] ) == SAME) ) {
      break;
    }
  }

  return ( i == gnum_types ) ? -1 : i;

}



int position_in_constants_table( char *str )

{

  int i;

  for ( i=0; i<gnum_constants; i++ ) {
    if ( str == gconstants[i] || 
	 strcmp( str, gconstants[i] ) == SAME ) {
      break;
    }
  }

  return ( i == gnum_constants ) ? -1 : i;

}



int position_in_predicates_table( char *str )

{

  int i;

  for ( i=0; i<gnum_predicates; i++ ) {
    if ( str == gpredicates[i] || 
	 strcmp( str, gpredicates[i] ) == SAME ) {
      break;
    }
  }

  return ( i == gnum_predicates ) ? -1 : i;

}



void create_integer_representation( void )

{

  PlNode *n, *nn, *newini, *next;
  PlOperator *o;
  Operator *tmp;
  FactList *ff;
  int type_num, i, j, k, s = 0, p, had;
  WffNode *iwff;
  Fact fff;

  if ( gorig_initial_facts ) {
    for ( n = gorig_initial_facts->sons; n; n = n->next ) s++;

    /* invert order of this list!! 
     * org pddl order was inverted in parsing; undo that
     * for better debugging readability.
     */
    newini = NULL; 
    n = gorig_initial_facts->sons;
    while ( n ) {
      next = n->next;
      n->next = newini;
      newini = n;
      n = next;
    }
    gorig_initial_facts->sons = newini;
  }
  s += gnum_constants;/* space for equalities */
  gfull_initial = ( Fact * ) calloc( s, sizeof( Fact ) );
  gfull_unknown_initial = ( Fact * ) calloc( s, sizeof( Fact ) );
  gnum_full_unknown_initial_cpt = ( int * ) calloc( s, sizeof( int ) );

  if ( gorig_initial_facts ) {
    gnum_full_initial = 0;
    gnum_full_unknown_initial = 0;
    for ( i = 0; i < s; i++ ) {
      gnum_full_unknown_initial_cpt[i] = 0;
    }

    for ( n = gorig_initial_facts->sons; n; n = n->next ) {
      if ( n->connective == ATOM ) {
	make_Fact( &(gfull_initial[gnum_full_initial]), n, 0 );
	if ( gfull_initial[gnum_full_initial].predicate == -1 ) {
	  printf("\nequality in initial state! check input files.\n\n");
	  exit( 1 );
	}
	gnum_full_initial++;
      } else {
	/* UNKNOWN
	 */
	make_Fact( &(gfull_unknown_initial[gnum_full_unknown_initial]), n->sons, 0 );
	if ( gfull_unknown_initial[gnum_full_unknown_initial].predicate == -1 ) {
	  printf("\nequality in initial state! check input files.\n\n");
	  exit( 1 );
	}
	/* avoid duplicates! -- CPTs require several entries for same fact!
	 */
	p = gfull_unknown_initial[gnum_full_unknown_initial].predicate;
	for ( i = 0; i < gnum_full_unknown_initial; i++ ) {
	  if ( gfull_unknown_initial[i].predicate != p ) {
	    continue;
	  }
	  for ( j = 0; j < garity[p]; j++ ) {
	    if ( gfull_unknown_initial[i].args[j] != 
		 gfull_unknown_initial[gnum_full_unknown_initial].args[j] ) {
	      break;
	    }
	  }
	  if ( j == garity[p] ) break;
	}
	if ( i < gnum_full_unknown_initial ) {
	  /* found equal fact; only increase counter for that one.
	   */
	  gnum_full_unknown_initial_cpt[i]++;
	  continue;
	}
	gnum_full_unknown_initial_cpt[gnum_full_unknown_initial]++;
	gnum_full_unknown_initial++;
      }
    }
    /* now create the CPTs of the unknown facts.
     */

    /* for each unknown initial fact nr. i,
     * we have an (j) array of size gnum_full_unknown_initial_cpt
     * with one probability entry in gfull_unknown_initial_cpt_p[i][j],
     * and one AND of facts in gfull_unknown_initial_cpt_c[i][j]
     */
    gfull_unknown_initial_cpt_p = ( double ** )
      calloc( gnum_full_unknown_initial, sizeof( double * ) );
    gfull_unknown_initial_cpt_c = ( WffNode_pointer ** )
      calloc( gnum_full_unknown_initial, sizeof( WffNode_pointer * ) );

    /* just "repeat" the previous loop, and do different stuff.
     */
    had = 0;
    for ( n = gorig_initial_facts->sons; n; n = n->next ) {
      if ( n->connective == ATOM ) {
	continue;
      }
      /* UNKNOWN
       */
      if ( n->sons->connective != CPT ) {
	printf("\n\nunknown node's son is not CPT?\n\n");
	exit( 1 );
      }
      make_Fact( &fff, n->sons, 0 );
      p = fff.predicate;
      for ( i = 0; i < had; i++ ) {
	if ( gfull_unknown_initial[i].predicate != p ) {
	  continue;
	}
	for ( j = 0; j < garity[p]; j++ ) {
	  if ( gfull_unknown_initial[i].args[j] != fff.args[j] ) {
	    break;
	  }
	}
	if ( j == garity[p] ) break;
      }
      if ( i < had ) {
	/* has this one before; just add another CPT entry.
	 */
	gfull_unknown_initial_cpt_p[i][gnum_full_unknown_initial_cpt[i]] = 
	  n->sons->cpt_p;
	/* oh my god, this is so dirty; turn (n->sons), the CPT node
	 * into AND node temporarily so make_Wff returns
	 * a Wff AND node containing the sons of n-> sons as the list
	 */
	n->sons->connective = AND;
	gfull_unknown_initial_cpt_c[i][gnum_full_unknown_initial_cpt[i]] = 
	  make_Wff(n->sons, 0);
	n->sons->connective = CPT;

	gnum_full_unknown_initial_cpt[i]++;
	continue;
      }

      /* first time for this one; create the CPT, add the first entry,
       * and count the fact as being visited (increment nr. facts)
       */
      if ( gnum_full_unknown_initial_cpt[had] < 1 ) {
	printf("\n\nencountering CPT fact with <1 CPT entry?\n\n");
	exit( 1 );
      }
      gfull_unknown_initial_cpt_p[had] = ( double * )
	calloc( gnum_full_unknown_initial_cpt[had], sizeof( double ) );
      gfull_unknown_initial_cpt_c[had] = ( WffNode_pointer * )
	calloc( gnum_full_unknown_initial_cpt[had], sizeof( WffNode_pointer ) );

      gfull_unknown_initial_cpt_p[had][0] = n->sons->cpt_p;
      /* oh my god, this is so dirty; turn (n->sons), the CPT node
       * into AND node temporarily so make_Wff returns
       * a Wff AND node containing the sons of n-> sons as the list
       */
      n->sons->connective = AND;
      gfull_unknown_initial_cpt_c[had][0] = make_Wff(n->sons, 0);
      n->sons->connective = CPT;

      /* for use as counter
       */
      gnum_full_unknown_initial_cpt[had] = 1;

      had++;
    }
    /* finished parsing the initial knowns and CPTs
     */

    free_PlNode( gorig_initial_facts );
  }
  /* check whether the same fact is known true and unknown.
   */
  for ( i = 0; i < gnum_full_initial; i++ ) {
    for ( j = 0; j < gnum_full_unknown_initial; j++ ) {
      if ( gfull_initial[i].predicate != gfull_unknown_initial[j].predicate ) {
	continue;
      }
      for ( k = 0; k < garity[gfull_initial[i].predicate]; k++ ) {
	if ( gfull_initial[i].args[k] != gfull_unknown_initial[j].args[k] ) {
	  break;
	}
      }
      if ( k == garity[gfull_initial[i].predicate] ) {
	printf("\nsame fact - ");
	print_Fact( &(gfull_initial[i]) );
	printf(" - initially true and unknown! check input files.\n\n");
	exit( 1 );
      }
    }
  }
  /* now insert all our artificial equality constraints into initial state.
   */
  for ( i = 0; i < gnum_constants; i++ ) {
    gfull_initial[gnum_full_initial].predicate = 0;
    gfull_initial[gnum_full_initial].args[0] = i;
    gfull_initial[gnum_full_initial].args[1] = i;
    gnum_full_initial++;
  }
  /* FINITO. the rest of equality handling will fully
   * automatically be done by the rest of the machinery.
   */

  /* now deal with the ors, and see that they are well-formed
   * (ie all unknown)
   */
  gfull_or_initial = ( WffNode_pointer * )
    calloc( gnum_orig_initial_ors, sizeof( WffNode_pointer ) );
  for ( i = 0; i < gnum_orig_initial_ors; i++ ) {
    gfull_or_initial[i] = make_Wff( gorig_initial_ors[i], 0 );
    /* all the OR-constrained literals must be from unknown facts.
     */
    for ( iwff = gfull_or_initial[i]->sons; iwff; iwff = iwff->next ) {
      if ( iwff->connective == NOT ) {
 	for ( j = 0; j < gnum_full_unknown_initial; j++ ) {
	  if ( iwff->son->fact->predicate != gfull_unknown_initial[j].predicate ) {
	    continue;
	  }
	  for ( k = 0; k < garity[iwff->son->fact->predicate]; k++ ) {
	    if ( iwff->son->fact->args[k] != gfull_unknown_initial[j].args[k] ) {
	      break;
	    }
	  }
	  if ( k == garity[iwff->son->fact->predicate] ) break;
	}
	if ( j == gnum_full_unknown_initial ) {
	  printf("\n\nfact ");
	  print_Fact( iwff->son->fact );
	  printf(" initially not unknown but OR-constrained!\n\n");
	  exit( 1 );
	}
     } else {
	for ( j = 0; j < gnum_full_unknown_initial; j++ ) {
	  if ( iwff->fact->predicate != gfull_unknown_initial[j].predicate ) {
	    continue;
	  }
	  for ( k = 0; k < garity[iwff->fact->predicate]; k++ ) {
	    if ( iwff->fact->args[k] != gfull_unknown_initial[j].args[k] ) {
	      break;
	    }
	  }
	  if ( k == garity[iwff->fact->predicate] ) break;
	}
	if ( j == gnum_full_unknown_initial ) {
	  printf("\n\nfact ");
	  print_Fact( iwff->fact );
	  printf(" initially not unknown but OR-constrained!\n\n");
	  exit( 1 );
	}
      }
    }
  }/* orig ors */
  gnum_full_or_initial = gnum_orig_initial_ors;


  /* NO sanity check for CPTs, particularly no check for:
   *   --- repeated conditions (e.g. TRUE)
   *   --- complete tables (all parent values)
   *   --- cycle-free network
   */


  /* now deal with the oneofs, and see that they are well-formed
   * (ie all unknown)
   */
  gfull_oneof_initial = ( WffNode_pointer * )
    calloc( gnum_orig_initial_oneofs, sizeof( WffNode_pointer ) );
  for ( i = 0; i < gnum_orig_initial_oneofs; i++ ) {
    gfull_oneof_initial[i] = make_Wff( gorig_initial_oneofs[i], 0 );
    for ( iwff = gfull_oneof_initial[i]->sons; iwff; iwff = iwff->next ) {
      for ( j = 0; j < gnum_full_unknown_initial; j++ ) {
	if ( iwff->fact->predicate != gfull_unknown_initial[j].predicate ) {
	  continue;
	}
	for ( k = 0; k < garity[iwff->fact->predicate]; k++ ) {
	  if ( iwff->fact->args[k] != gfull_unknown_initial[j].args[k] ) {
	    break;
	  }
	}
	if ( k == garity[iwff->fact->predicate] ) break;
      }
      if ( j == gnum_full_unknown_initial ) {
	printf("\n\nfact ");
	print_Fact( iwff->fact );
	printf(" initially not unknown but ONEOF-constrained!\n\n");
	  exit( 1 );
      }
    }
  }/* orig oneofs */
  gnum_full_oneof_initial = gnum_orig_initial_oneofs;



  /* now deal with the multis, and (incomplete) see that they are well-formed
   * (ie all unknown)
   */
  gfull_multi_initial = ( WffNode_pointer * )
    calloc( gnum_orig_initial_multis, sizeof( WffNode_pointer ) );
  for ( i = 0; i < gnum_orig_initial_multis; i++ ) {
    gfull_multi_initial[i] = make_Wff( gorig_initial_multis[i], 0 );
    for ( iwff = gfull_multi_initial[i]->sons; iwff; iwff = iwff->next ) {
      for ( j = 0; j < gnum_full_unknown_initial; j++ ) {
	if ( iwff->fact->predicate != gfull_unknown_initial[j].predicate ) {
	  continue;
	}
	for ( k = 0; k < garity[iwff->fact->predicate]; k++ ) {
	  if ( iwff->fact->args[k] != gfull_unknown_initial[j].args[k] ) {
	    break;
	  }
	}
	if ( k == garity[iwff->fact->predicate] ) break;
      }
      if ( j == gnum_full_unknown_initial ) {
	printf("\n\nfact ");
	print_Fact( iwff->fact );
	printf(" initially not unknown but MULTI-constrained!\n\n");
	  exit( 1 );
      }
    }
  }/* orig multis */
  gnum_full_multi_initial = gnum_orig_initial_multis;


  ggoal = make_Wff( gorig_goal_facts, 0 );

  for ( o = gloaded_ops; o; o = o->next ) {
    tmp = new_Operator( o->name, o->number_of_real_params );

    for ( ff = o->params; ff; ff = ff->next ) {
      if ( (type_num = position_in_types_table( ff->item->next->item )) == -1 ) {
	printf("\nwarning: parameter %s of op %s has unknown or empty type %s. skipping op",
	       ff->item->item, o->name, ff->item->next->item);
	break;
      }
      if ( tmp->num_vars == MAX_VARS ) {
	printf("\ntoo many parameters! increase MAX_VARS (currently %d)\n\n",
	       MAX_VARS);
	exit( 1 );
      }
      for ( i = 0; i < tmp->num_vars; i++ ) {
	if ( tmp->var_names[i] == ff->item->item ||
	     strcmp( tmp->var_names[i], ff->item->item ) == SAME ) {
	  printf("\nwarning: operator %s parameter %s overwrites previous declaration\n\n",
		 tmp->name, ff->item->item);
	}
      }
      tmp->var_names[tmp->num_vars] = new_Token( strlen( ff->item->item ) + 1 );
      strcpy( tmp->var_names[tmp->num_vars], ff->item->item );
      tmp->var_types[tmp->num_vars++] = type_num;
    }
    if ( ff ) {
      free_Operator( tmp );
      continue;
    }

    for ( i = 0; i < tmp->num_vars; i++ ) {
      lvar_types[i] = tmp->var_types[i];
      lvar_names[i] = tmp->var_names[i];
    }

    tmp->preconds = make_Wff( o->preconds, tmp->num_vars );

    if ( o->effects ) {
      /* in make_effect, make sure that no one affects equality.
       */
      nn = o->effects->sons;
      while ( nn &&
	      (tmp->effects = make_effect( nn, tmp->num_vars )) == NULL ) {
	nn = nn->next;
      }
      if ( nn ) {
	for ( n = nn->next; n; n = n->next ) {
	  if ( (tmp->effects->prev = make_effect( n, tmp->num_vars )) == NULL ) {
	    continue;
	  }
	  tmp->effects->prev->next = tmp->effects;
	  tmp->effects = tmp->effects->prev;
	}
      }
    }

    if ( gnum_operators == MAX_OPERATORS ) {
      printf("\ntoo many operators! increase MAX_OPERATORS (currently %d)\n\n",
	     MAX_OPERATORS);
      exit( 1 );
    }
    goperators[gnum_operators++] = tmp;
  }

  if ( 0 ) {
    /* currently not in use; leads to free memory reads and
     * free memory frees (no memory leaks), which are hard to explain.
     *
     * almost no memory consumption anyway.
     */
    free_PlOperator( gloaded_ops );
  }

}



void make_Fact( Fact *f, PlNode *n, int num_vars )

{

  int m, i;
  TokenList *t;

  if ( !n->atom ) {
    /* can't happen after previous syntax check. Oh well, whatever...
     */
    printf("\nillegal (empty) atom used in domain. check input files\n\n");
    exit( 1 );
  }

  f->predicate = position_in_predicates_table( n->atom->item );
  if ( f->predicate == -1 ) {
    printf("\nundeclared predicate %s used in domain definition\n\n",
	   n->atom->item);
    exit( 1 );
  }

  m = 0;
  for ( t = n->atom->next; t; t = t->next ) {
    if ( t->item[0] == '?' ) {
      for ( i=num_vars-1; i>-1; i-- ) {
	/* downwards, to always get most recent declaration/quantification
	 * of that variable
	 */
	if ( lvar_names[i] == t->item ||
	     strcmp( lvar_names[i], t->item ) == SAME ) {
	  break;
	}
      }
      if ( i == -1 ) {
	printf("\nundeclared variable %s in literal %s. check input files\n\n",
	       t->item, n->atom->item);
	exit( 1 );
      }
      if ( f->predicate != -1 &&
	   lvar_types[i] != gpredicates_args_type[f->predicate][m] &&
	   !is_subtype( lvar_types[i], gpredicates_args_type[f->predicate][m] ) ) {
	printf("\ntype of var %s does not match type of arg %d of predicate %s\n\n",
	       lvar_names[i], m, gpredicates[f->predicate]);
	exit( 1 );
      }
      f->args[m] = ENCODE_VAR( i );
    } else {
      if ( (f->args[m] = 
	    position_in_constants_table( t->item )) == -1 ) {
	printf("\nunknown constant %s in literal %s. check input files\n\n",
	       t->item, n->atom->item);
	exit( 1 );
      }
      if ( f->predicate != -1 &&
	   !gis_member[f->args[m]][gpredicates_args_type[f->predicate][m]] ) {
	printf("\ntype mismatch: constant %s as arg %d of %s. check input files\n\n",
	       gconstants[f->args[m]], m, gpredicates[f->predicate]);
	exit( 1 );
      }
    }
    m++;
  }
  if ( f->predicate == -1 ) {
    if ( m != 2 ) {
      printf("\nfound eq - predicate with %d arguments. check input files\n\n",
	     m);
      exit( 1 );
    }
  } else {
    if ( m != garity[f->predicate] ) {
      printf("\npredicate %s is declared to have %d (not %d) arguments. check input files\n\n",
	     gpredicates[f->predicate],
	     garity[f->predicate], m);
      exit( 1 );
    }
  }

}



Bool is_subtype( int t1, int t2 )

{

  int i;

  for ( i = 0; i < gtype_size[t1]; i++ ) {
    if ( !gis_member[gtype_consts[t1][i]][t2] ) {
      return FALSE;
    }
  }

  return TRUE;

}



WffNode *make_Wff( PlNode *p, int num_vars )

{

  WffNode *tmp;
  int i, t;
  PlNode *n;

  if ( !p ) {
    tmp = NULL;
    return tmp;
  }

  tmp = new_WffNode( p->connective );
  switch ( p->connective ) {
  case ALL:
  case EX:
    for ( i = 0; i < num_vars; i++ ) {
      if ( lvar_names[i] == p->atom->item ||
	   strcmp( lvar_names[i], p->atom->item ) == SAME ) {
	printf("\nwarning: var quantification of %s overwrites previous declaration",
	       p->atom->item);
      }
    }
    if ( (t = position_in_types_table( p->atom->next->item )) == -1 ) {
      printf("\nwarning: quantified var %s has unknown or empty type %s. simplifying.",
	     p->atom->item, p->atom->next->item);
      tmp->connective = ( p->connective == EX ) ? FAL : TRU;
      break;
    }
    tmp->var = num_vars;
    tmp->var_type = t;
    tmp->var_name = new_Token( strlen( p->atom->item ) + 1 );
    strcpy( tmp->var_name, p->atom->item );
    lvar_names[num_vars] = p->atom->item;
    lvar_types[num_vars] = t;
    tmp->son = make_Wff( p->sons, num_vars + 1 );
    break;
  case AND:
  case OR:
    if ( !p->sons ) {
      /* this would be printed for every unknown fact with TRUE CPT */
/*       printf("\nwarning: empty con/disjunction in domain definition. simplifying."); */
      tmp->connective = ( p->connective == OR ) ? FAL : TRU;
      break;
    }
    tmp->sons = make_Wff( p->sons, num_vars );
    for ( n = p->sons->next; n; n = n->next ) {
      tmp->sons->prev = make_Wff( n, num_vars );
      tmp->sons->prev->next = tmp->sons;
      tmp->sons = tmp->sons->prev;
    }
    break;
  case ONEOF:
  case MULTI:
    if ( !p->sons ) {
      printf("\n\nempty ONEOF or MULTI? check input files.\n\n");
      exit( 1 );
    }
    tmp->sons = make_Wff( p->sons, num_vars );
    for ( n = p->sons->next; n; n = n->next ) {
      tmp->sons->prev = make_Wff( n, num_vars );
      tmp->sons->prev->next = tmp->sons;
      tmp->sons = tmp->sons->prev;
    }
    break;
  case NOT:
    tmp->son = make_Wff( p->sons, num_vars );
    break;
  case ATOM:
    tmp->fact = new_Fact();
    make_Fact( tmp->fact, p, num_vars );
    break;
  case TRU:
  case FAL:
    break;
  default:
    printf("\nforbidden connective %d in Pl Wff. must be a bug somewhere...\n\n",
	   p->connective);
    exit( 1 );
  }

  return tmp;

}



Effect *make_effect( PlNode *p, int num_vars )

{

  Effect *tmp = new_Effect();
  PlNode *n, *m;
  int t, i;

  for ( n = p; n && n->connective == ALL; n = n->sons ) { 
    if ( (t = position_in_types_table( n->atom->next->item )) == -1 ) {
      printf("\nwarning: effect parameter %s has unknown or empty type %s. skipping effect.\n\n", 
	     n->atom->item, n->atom->next->item); 
      return NULL; 
    } 
    for ( i = 0; i < num_vars + tmp->num_vars; i++ ) {
      if ( lvar_names[i] == n->atom->item ||
	   strcmp( lvar_names[i], n->atom->item ) == SAME ) {
	printf("\nwarning: effect parameter %s overwrites previous declaration\n\n",
	       n->atom->item);
      }
    }
    lvar_types[num_vars + tmp->num_vars] = t; 
    lvar_names[num_vars + tmp->num_vars] = n->atom->item; 
    tmp->var_names[tmp->num_vars] = new_Token( strlen( n->atom->item ) + 1 ); 
    strcpy( tmp->var_names[tmp->num_vars], n->atom->item );
    tmp->var_types[tmp->num_vars++] = t; 
  }

  if ( !n || n->connective != WHEN ) {
    printf("\nnon WHEN %d at end of effect parameters. debug me\n\n",
	   n->connective);
    exit( 1 );
  }

  tmp->conditions = make_Wff( n->sons, num_vars + tmp->num_vars );
  tmp->eff_p = n->eff_p;/* july06: copy over prob */

  if ( n->sons->next->connective != AND ) {
    printf("\nnon AND %d in front of literal effect list. debug me\n\n",
	   n->sons->next->connective);
    exit( 1 );
  }
  if ( !n->sons->next->sons ) {
    return tmp;
  }
  m = n->sons->next->sons;
  tmp->effects = new_Literal();
  if ( m->connective == NOT ) {
    tmp->effects->negated = TRUE;
    make_Fact( &(tmp->effects->fact), m->sons, num_vars + tmp->num_vars );
    if ( (tmp->effects->fact).predicate == 0 ) {
      printf("\n\nequality in effect! check input files!\n\n");
      exit( 1 );
    }
  } else {
    tmp->effects->negated = FALSE;
    make_Fact( &(tmp->effects->fact), m, num_vars + tmp->num_vars );
    if ( (tmp->effects->fact).predicate == 0 ) {
      printf("\n\nequality in effect! check input files!\n\n");
      exit( 1 );
    }
  }
  for ( m = m->next; m; m = m->next ) {
    tmp->effects->prev = new_Literal();
    if ( m->connective == NOT ) {
      tmp->effects->prev->negated = TRUE;
      make_Fact( &(tmp->effects->prev->fact), m->sons, num_vars + tmp->num_vars );
      if ( (tmp->effects->prev->fact).predicate == 0 ) {
	printf("\n\nequality in effect! check input files!\n\n");
	exit( 1 );
      }
    } else {
      tmp->effects->prev->negated = FALSE;
      make_Fact( &(tmp->effects->prev->fact), m, num_vars + tmp->num_vars );
      if ( (tmp->effects->prev->fact).predicate == 0 ) {
	printf("\n\nequality in effect! check input files!\n\n");
	exit( 1 );
      }
    }
    tmp->effects->prev->next = tmp->effects;
    tmp->effects = tmp->effects->prev;
  }

  return tmp;

}











/*************************
 * INERTIA PREPROCESSING *
 *************************/











void do_inertia_preprocessing_step_1( void )

{

  int i, j;
  Facts *f;

  collect_inertia_information();

  if ( gcmd_line.display_info == 105 ) {
    printf("\n\npredicates inertia info:");
    for ( i = 0; i < gnum_predicates; i++ ) {
      printf("\n%3d --> %s: ", i, gpredicates[i]);
      printf(" is %s, %s, %s",
	     gis_added[i] ? "ADDED" : "NOT ADDED",
	     gis_deleted[i] ? "DELETED" : "NOT DELETED",
	     gis_unknown[i] ? "UNKNOWN" : "KNOWN");
    }
    printf("\n\n");
  }

  split_initial_state();

  if ( gcmd_line.display_info == 106 ) {
    printf("\n\nsplitted initial state is:");
    printf("\nindividual predicates, known parts:");
    for ( i = 0; i < gnum_predicates; i++ ) {
      printf("\n\n%s:", gpredicates[i]);
      if ( !gis_added[i] &&
	   !gis_deleted[i] &&
	   !gis_unknown[i] ) {
	printf(" ---  STATIC");
      }
      for ( j = 0; j < gnum_initial_predicate[i]; j++ ) {
	printf("\n");
	print_Fact( &(ginitial_predicate[i][j]) );
      }
    }
    printf("\nindividual predicates, unknown parts:");
    for ( i = 0; i < gnum_predicates; i++ ) {
      printf("\n\n%s:", gpredicates[i]);
      if ( !gis_added[i] &&
	   !gis_deleted[i] &&
	   !gis_unknown[i] ) {
	printf(" ---  STATIC");
      }
      for ( j = 0; j < gnum_unknown_initial_predicate[i]; j++ ) {
	printf("\n");
	print_Fact( &(gunknown_initial_predicate[i][j]) );
      }
    }
    printf("\n\nnon static part, known:");
    for ( f = ginitial; f; f = f->next ) {
      printf("\n");
      print_Fact( f->fact );
    }
    printf("\n\nnon static part, unknown:");
    for ( f = gunknown_initial; f; f = f->next ) {
      printf("\n");
      print_Fact( f->fact );
      printf("\nCPT:\n");
      for ( j = 0; j < f->num_cpt; j++ ) {
	printf("probability: %f\n", f->cpt_p[j]);
	printf("if: \n");
	print_Wff( f->cpt_c[j], 0 );
      }      
    }

    printf("\n\nextended types table:");
    for ( i = 0; i < gnum_types; i++ ) {
      printf("\n%d --> ", i);
      if ( gpredicate_to_type[i] == -1 ) {
	printf("%s ", gtype_names[i]);
      } else {
	printf("UNARY INERTIA TYPE (%s) ", gpredicates[gpredicate_to_type[i]]);
      }
      for ( j = 0; j < gtype_size[i]; j++ ) {
	printf("%d ", gtype_consts[i][j]);
      }
    }
  }

}



void collect_inertia_information( void )

{

  int i;
  Effect *e;
  Literal *l;

  for ( i = 0; i < gnum_predicates; i++ ) {
    gis_added[i] = FALSE;
    gis_deleted[i] = FALSE;
    gis_unknown[i] = FALSE;
  }

  for ( i = 0; i < gnum_operators; i++ ) {
    for ( e = goperators[i]->effects; e; e = e->next ) {
      for ( l = e->effects; l; l = l->next ) {
	if ( l->negated ) {
	  gis_deleted[l->fact.predicate] = TRUE;
	} else {
	  gis_added[l->fact.predicate] = TRUE;
	}
      }
    }
  }

  for ( i = 0; i < gnum_full_unknown_initial; i++ ) {
    gis_unknown[gfull_unknown_initial[i].predicate] = TRUE; 
  }

}



void split_initial_state( void )

{

  int i, j, p, t;
  Facts *tmp;

  for ( i = 0; i < MAX_PREDICATES; i++ ) {
    gtype_to_predicate[i] = -1;
  }
  for ( i = 0; i < MAX_TYPES; i++ ) {
    gpredicate_to_type[i] = -1;
  }

  for ( i = 0; i < gnum_predicates; i++ ) {
    if ( !gis_added[i] &&
	 !gis_deleted[i] &&
	 !gis_unknown[i] &&
	 garity[i] == 1 ) {
      if ( gnum_types == MAX_TYPES ) {
	printf("\ntoo many (inferred) types! increase MAX_TYPES (currently %d)\n\n",
	       MAX_TYPES);
	exit( 1 );
      }
      gtype_to_predicate[i] = gnum_types;
      gpredicate_to_type[gnum_types] = i;
      gtype_names[gnum_types] = NULL;
      gtype_size[gnum_types] = 0;
      for ( j = 0; j < MAX_CONSTANTS; j++ ) {
	gis_member[j][gnum_types] = FALSE;
      }
      gnum_types++;
    }
  }
     

  /* double size of predicates table as each predicate might need
   * to be translated to NOT-p
   */
  ginitial_predicate = ( Fact ** ) calloc( gnum_predicates * 2, sizeof( Fact * ) );
  gnum_initial_predicate = ( int * ) calloc( gnum_predicates * 2, sizeof( int ) );
  for ( i = 0; i < gnum_predicates * 2; i++ ) {
    gnum_initial_predicate[i] = 0;
  }
  for ( i = 0; i < gnum_full_initial; i++ ) {
    p = gfull_initial[i].predicate;
    gnum_initial_predicate[p]++;
  }
  for ( i = 0; i < gnum_predicates; i++ ) {
    ginitial_predicate[i] = ( Fact * ) calloc( gnum_initial_predicate[i], sizeof( Fact ) );
    gnum_initial_predicate[i] = 0;
  }
  ginitial = NULL;
  gnum_initial = 0;

  for ( i = 0; i < gnum_full_initial; i++ ) {
    p = gfull_initial[i].predicate;
    ginitial_predicate[p][gnum_initial_predicate[p]].predicate = p;
    for ( j = 0; j < garity[p]; j++ ) {
      ginitial_predicate[p][gnum_initial_predicate[p]].args[j] = gfull_initial[i].args[j];
    }
    gnum_initial_predicate[p]++;
    if ( gis_added[p] ||
	 gis_deleted[p] ||
	 gis_unknown[p] ) {
      tmp = new_Facts();
      tmp->fact->predicate = p;
      for ( j = 0; j < garity[p]; j++ ) {
	tmp->fact->args[j] = gfull_initial[i].args[j];
      }
      tmp->next = ginitial;
      ginitial = tmp;
      gnum_initial++;
    } else {
      if ( garity[p] == 1 ) {
	t = gtype_to_predicate[p];
	if ( gtype_size[t] == MAX_TYPE ) {
	  printf("\ntoo many consts in type %s! increase MAX_TYPE (currently %d)\n\n",
		 gtype_names[t], MAX_TYPE);
	  exit( 1 );
	}
	if ( !gis_member[gfull_initial[i].args[0]][gpredicates_args_type[p][0]] ) {
	  printf("\ntype mismatch in initial state! %s as arg 0 of %s\n\n",
		 gconstants[gfull_initial[i].args[0]], gpredicates[p]);
	  exit( 1 );
	}
	gtype_consts[t][gtype_size[t]++] = gfull_initial[i].args[0];
	gis_member[gfull_initial[i].args[0]][t] = TRUE;
      }
    }
  }
     
  /* double size of predicates table as each predicate might need
   * to be translated to NOT-p
   */
  gunknown_initial_predicate = ( Fact ** ) calloc( gnum_predicates * 2, sizeof( Fact * ) );
  gnum_unknown_initial_predicate = ( int * ) calloc( gnum_predicates * 2, sizeof( int ) );
  for ( i = 0; i < gnum_predicates * 2; i++ ) {
    gnum_unknown_initial_predicate[i] = 0;
  }
  for ( i = 0; i < gnum_full_unknown_initial; i++ ) {
    p = gfull_unknown_initial[i].predicate;
    gnum_unknown_initial_predicate[p]++;
  }
  for ( i = 0; i < gnum_predicates; i++ ) {
    gunknown_initial_predicate[i] = ( Fact * ) 
      calloc( gnum_unknown_initial_predicate[i], sizeof( Fact ) );
    gnum_unknown_initial_predicate[i] = 0;
  }
  gunknown_initial = NULL;
  gnum_unknown_initial = 0;

  for ( i = 0; i < gnum_full_unknown_initial; i++ ) {
    p = gfull_unknown_initial[i].predicate;
    gunknown_initial_predicate[p][gnum_unknown_initial_predicate[p]].predicate = p;
    for ( j = 0; j < garity[p]; j++ ) {
      gunknown_initial_predicate[p][gnum_unknown_initial_predicate[p]].args[j] 
	= gfull_unknown_initial[i].args[j];
    }
    gnum_unknown_initial_predicate[p]++;
    if ( gis_unknown[p] ) {
      tmp = new_Facts();
      tmp->fact->predicate = p;
      for ( j = 0; j < garity[p]; j++ ) {
	tmp->fact->args[j] = gfull_unknown_initial[i].args[j];
      }

      /* copy CPT over
       */
      tmp->num_cpt = gnum_full_unknown_initial_cpt[i];
      tmp->cpt_p = ( double * ) calloc( tmp->num_cpt, sizeof ( double ) );
      tmp->cpt_c = ( WffNode_pointer * ) calloc( tmp->num_cpt, sizeof ( WffNode_pointer ) );
      
      for ( j = 0; j < tmp->num_cpt; j++ ) {
	tmp->cpt_p[j] = gfull_unknown_initial_cpt_p[i][j];
	tmp->cpt_c[j] = gfull_unknown_initial_cpt_c[i][j];
      }
      /* ATTENTION! THIS WAY, DON'T FREE PREVIOUS INITIAL STATE
       * representation, to not lose the CPT conditions!!
       */

      tmp->next = gunknown_initial;
      gunknown_initial = tmp;
      gnum_unknown_initial++;
    } else {
      printf("\n\nunknown initial fact has known predicate?!\n\n");
      exit( 1 );
    }
  }

}











/******************************
 * NORMALIZE ALL PL1 FORMULAE *
 ******************************/












void normalize_all_wffs( void )

{

  int i, j;
  Effect *e;

  i = 0;
  while ( i < gnum_full_or_initial ) {
    simplify_wff( &gfull_or_initial[i] );
    remove_unused_vars_in_wff( &gfull_or_initial[i] );
    expand_quantifiers_in_wff( &gfull_or_initial[i], -1, -1 );
    NOTs_down_in_wff( &gfull_or_initial[i] );
    cleanup_wff( &gfull_or_initial[i] );
    if ( gfull_or_initial[i]->connective == TRU ) {
      printf("\nwarning: initial OR constraint simplifies to TRUE. removing it.");
      for ( j = i; j < gnum_full_or_initial - 1; j++ ) {
	gfull_or_initial[j] = gfull_or_initial[j+1];
      }
      gnum_full_or_initial--;
      continue;
    }
    if ( gfull_or_initial[i]->connective == ATOM ) {
      printf("\n\ninitial OR constraint simplifies to one fact. check input files.\n\n");
      exit( 1 );
    }
    if ( gfull_or_initial[i]->connective == NOT ) {
      printf("\n\ninitial OR constraint simplifies to one negated fact. check input files.\n\n");
      exit( 1 );
    }
    i++;
  }

  simplify_wff( &ggoal );
  remove_unused_vars_in_wff( &ggoal );
  expand_quantifiers_in_wff( &ggoal, -1, -1 );
  NOTs_down_in_wff( &ggoal );
  cleanup_wff( &ggoal );
  if ( ggoal->connective == TRU ) {
    printf("\nff: goal can be simplified to TRUE. The empty plan solves it\n\n");
    exit( 1 );
  }
  if ( ggoal->connective == FAL ) {
    printf("\nff: goal can be simplified to FALSE. No plan will solve it\n\n");
    exit( 1 );
  }
  /* put goal into DNF right away: fully instantiated already
   */
  dnf( &ggoal );
  cleanup_wff( &ggoal );
  
  for ( i = 0; i < gnum_operators; i++ ) {
    simplify_wff( &(goperators[i]->preconds) );
    remove_unused_vars_in_wff( &(goperators[i]->preconds) );
    expand_quantifiers_in_wff( &(goperators[i]->preconds), -1, -1 );
    NOTs_down_in_wff( &(goperators[i]->preconds) );
    cleanup_wff( &(goperators[i]->preconds) );

    for ( e = goperators[i]->effects; e; e = e->next ) {
      simplify_wff( &(e->conditions) );
      remove_unused_vars_in_wff( &(e->conditions) );
      expand_quantifiers_in_wff( &(e->conditions), -1, -1 );
      NOTs_down_in_wff( &(e->conditions) );
      cleanup_wff( &(e->conditions) );
    }
  }

  if ( gcmd_line.display_info == 107 ) {
    printf("\n\ndomain with normalized PL1 formulae:");

    printf("\n\noperators are:");
    for ( i = 0; i < gnum_operators; i++ ) {
      print_Operator( goperators[i] );
    }
    printf("\n\n");

    printf("\ninitial ors:\n");
    for ( i = 0; i < gnum_full_or_initial; i++ ) {
      print_Wff( gfull_or_initial[i], 0 );
    }

    printf("\n\ngoal is:\n");
    print_Wff( ggoal, 0 );
  }

}



void remove_unused_vars_in_wff( WffNode **w )

{

  WffNode *tmp;
  WffNode *i;

  switch ( (*w)->connective ) {
  case ALL:
  case EX:
    remove_unused_vars_in_wff( &((*w)->son) );
    if ( !var_used_in_wff( ENCODE_VAR( (*w)->var ), (*w)->son ) ) {
      decrement_inferior_vars((*w)->var, (*w)->son );
      (*w)->connective = (*w)->son->connective;
      (*w)->var = (*w)->son->var;
      (*w)->var_type = (*w)->son->var_type;
      if ( (*w)->var_name ) {
	free( (*w)->var_name );
      }
      (*w)->var_name = (*w)->son->var_name;
      (*w)->sons = (*w)->son->sons;
      if ( (*w)->fact ) {
	free( (*w)->fact );
      }
      (*w)->fact = (*w)->son->fact;
      tmp = (*w)->son;
      (*w)->son = (*w)->son->son;
      free( tmp );
    }
    break;
  case AND:
  case OR:
    for ( i = (*w)->sons; i; i = i->next ) {
      remove_unused_vars_in_wff( &i );
    }
    break;
  case NOT:
    remove_unused_vars_in_wff( &((*w)->son) );
    break;
  case ATOM:
  case TRU:
  case FAL:
    break;
  default:
    printf("\nwon't get here: remove var, non logical %d\n\n",
	   (*w)->connective);
    exit( 1 );
  }

}



Bool var_used_in_wff( int code_var, WffNode *w )

{

  WffNode *i;
  int j;

  switch ( w->connective ) {
  case ALL:
  case EX:
    return var_used_in_wff( code_var, w->son );
  case AND:
  case OR:
    for ( i = w->sons; i; i = i->next ) {
      if ( var_used_in_wff( code_var, i ) ) {
	return TRUE;
      }
    }
    return FALSE;
  case NOT:
    return var_used_in_wff( code_var, w->son );
  case ATOM:
    for ( j = 0; j < garity[w->fact->predicate]; j++ ) {
      if ( w->fact->args[j] >= 0 ) {
	continue;
      }
      if ( w->fact->args[j] == code_var ) {
	return TRUE;
      }
    }
    return FALSE;
  case TRU:
  case FAL:
    return FALSE;
  default:
    printf("\nwon't get here: var used ?, non logical %d\n\n",
	   w->connective);
    exit( 1 );
  }


}



void decrement_inferior_vars( int var, WffNode *w )

{

  WffNode *i;
  int j;

  switch ( w->connective ) {
  case ALL:
  case EX:
    w->var--;
    decrement_inferior_vars( var, w->son );
    break;
  case AND:
  case OR:
    for ( i = w->sons; i; i = i->next ) {
      decrement_inferior_vars( var, i );
    }
    break;
  case NOT:
    decrement_inferior_vars( var, w->son );
    break;
  case ATOM:
    for ( j = 0; j < garity[w->fact->predicate]; j++ ) {
      if ( w->fact->args[j] >= 0 ) {
	continue;
      }
      if ( DECODE_VAR( w->fact->args[j] ) > var ) {
	w->fact->args[j]++;
      }
    }
    break;
  case TRU:
  case FAL:
    break;
  default:
    printf("\nwon't get here: decrement, non logical %d\n\n",
	   w->connective);
    exit( 1 );
  }

}



void simplify_wff( WffNode **w )

{

  WffNode *i, *tmp;
  int m;

  switch ( (*w)->connective ) {
  case ALL:
  case EX:
    simplify_wff( &((*w)->son) );
    if ( (*w)->son->connective == TRU ||
	 (*w)->son->connective == FAL ) {
      (*w)->connective = (*w)->son->connective;
      free( (*w)->son );
      (*w)->son = NULL;
      if ( (*w)->var_name ) {
	free( (*w)->var_name );
      }
    }
    break;
  case AND:
    m = 0;
    i = (*w)->sons;
    while ( i ) {
      simplify_wff( &i );
      if ( i->connective == FAL ) {
	(*w)->connective = FAL;
	free_WffNode( (*w)->sons );
	(*w)->sons = NULL;
	return;
      }
      if ( i->connective == TRU ) {
	if ( i->prev ) {
	  i->prev->next = i->next;
	} else {
	  (*w)->sons = i->next;
	}
	if ( i->next ) {
	  i->next->prev = i->prev;
	}
	tmp = i;
	i = i->next;
	free( tmp );
	continue;
      }
      i = i->next;
      m++;
    }
    if ( m == 0 ) {
      (*w)->connective = TRU;
      free_WffNode( (*w)->sons );
      (*w)->sons = NULL;
    }
    if ( m == 1 ) {
      (*w)->connective = (*w)->sons->connective;
      (*w)->var = (*w)->sons->var;
      (*w)->var_type = (*w)->sons->var_type;
      if ( (*w)->var_name ) {
	free( (*w)->var_name );
      }
      (*w)->var_name = (*w)->sons->var_name;
      (*w)->son = (*w)->sons->son;
      if ( (*w)->fact ) {
	free( (*w)->fact );
      }
      (*w)->fact = (*w)->sons->fact;
      tmp = (*w)->sons;
      (*w)->sons = (*w)->sons->sons;
      free( tmp );
    }
    break;
  case OR:
    m = 0;
    i = (*w)->sons;
    while ( i ) {
      simplify_wff( &i );
      if ( i->connective == TRU ) {
	(*w)->connective = TRU;
	free_WffNode( (*w)->sons );
	(*w)->sons = NULL;
	return;
      }
      if ( i->connective == FAL ) {
	if ( i->prev ) {
	  i->prev->next = i->next;
	} else {
	  (*w)->sons = i->next;
	}
	if ( i->next ) {
	  i->next->prev = i->prev;
	}
	tmp = i;
	i = i->next;
	free( tmp );
	continue;
      }
      i = i->next;
      m++;
    }
    if ( m == 0 ) {
      (*w)->connective = FAL;
      free_WffNode( (*w)->sons );
      (*w)->sons = NULL;
    }
    if ( m == 1 ) {
      (*w)->connective = (*w)->sons->connective;
      (*w)->var = (*w)->sons->var;
      (*w)->var_type = (*w)->sons->var_type;
      if ( (*w)->var_name ) {
	free( (*w)->var_name );
      }
      (*w)->var_name = (*w)->sons->var_name;
      (*w)->son = (*w)->sons->son;
      if ( (*w)->fact ) {
	free( (*w)->fact );
      }
      (*w)->fact = (*w)->sons->fact;
      tmp = (*w)->sons;
      (*w)->sons = (*w)->sons->sons;
      free( tmp );
    }
    break;
  case NOT:
    simplify_wff( &((*w)->son) );
    if ( (*w)->son->connective == TRU ||
	 (*w)->son->connective == FAL ) {
      (*w)->connective = ( (*w)->son->connective == TRU ) ? FAL : TRU;
      free( (*w)->son );
      (*w)->son = NULL;
    }
    break;
  case ATOM:
    if ( (*w)->visited ) {
      /* already seen and not changed
       */
      break;
    }
    if ( !possibly_negative( (*w)->fact ) ) {
      (*w)->connective = TRU;
      free( (*w)->fact );
      (*w)->fact = NULL;
      break;
    }
    if ( !possibly_positive( (*w)->fact ) ) {
      (*w)->connective = FAL;
      free( (*w)->fact );
      (*w)->fact = NULL;
      break;
    }
    (*w)->visited = TRUE;
    break;
  case TRU:
  case FAL:
    break;
  default:
    printf("\nwon't get here: simplify, non logical %d\n\n",
	   (*w)->connective);
    exit( 1 );
  }

}



void expand_quantifiers_in_wff( WffNode **w, int var, int constant )

{

  WffNode *r = NULL, *tmp, *i;
  int j, l;
  Bool change;

  if ( !(*w) ) {
    return;
  }

  switch ( (*w)->connective ) {
  case ALL:
  case EX:
    if ( var != -1 ) {/* depth first: upper node is active */
      expand_quantifiers_in_wff( &((*w)->son), var, constant );
      return;
    }

    (*w)->connective = ( (*w)->connective == ALL ) ? AND : OR;
    for ( j = 0; j < gtype_size[(*w)->var_type]; j++ ) {
      tmp = copy_Wff( (*w)->son );
      expand_quantifiers_in_wff( &tmp, (*w)->var, gtype_consts[(*w)->var_type][j] );
      tmp->next = r;
      if ( r ) {
	r->prev = tmp;
      }
      r = tmp;
    }

    free_WffNode( (*w)->son );
    (*w)->sons = r;
    (*w)->var = -1;
    (*w)->var_type = -1;
    if ( (*w)->var_name ) {
      free( (*w)->var_name );
    }
    (*w)->var_name = NULL;

    /* now make all sons expand their quantifiers
     */
    for ( i = (*w)->sons; i; i = i->next ) {
      expand_quantifiers_in_wff( &i, -1, -1 );
    }
    break;
  case AND:
  case OR:
    for ( i = (*w)->sons; i; i = i->next ) {
      expand_quantifiers_in_wff( &i, var, constant );
    }
    break;
  case NOT:
    expand_quantifiers_in_wff( &((*w)->son), var, constant );
    break;
  case ATOM:
    if ( var == -1 ) {
      break;
    }

    change = FALSE;
    for ( l = 0; l < garity[(*w)->fact->predicate]; l++ ) {
      if ( (*w)->fact->args[l] == ENCODE_VAR( var ) ) {
	(*w)->fact->args[l] = constant;
	change = TRUE;
      }
    }
    if ( !change && (*w)->visited ) {
      /* we did not change anything and we've already seen that node
       * --> it cant be simplified
       */
      break;
    }
    if ( !possibly_negative( (*w)->fact ) ) {
      (*w)->connective = TRU;
      free( (*w)->fact );
      (*w)->fact = NULL;
      break;
    }
    if ( !possibly_positive( (*w)->fact ) ) {
      (*w)->connective = FAL;
      free( (*w)->fact );
      (*w)->fact = NULL;
      break;
    }
    (*w)->visited = TRUE;
    break;
  case TRU:
  case FAL:
    break;
  default:
    printf("\nwon't get here: expansion, non logical %d\n\n",
	   (*w)->connective);
    exit( 1 );
  }

}



WffNode *copy_Wff( WffNode *w )

{

  WffNode *tmp, *tmp2, *i;
  int j;

  tmp = new_WffNode( w->connective );

  switch ( w->connective ) {
  case ALL:
  case EX:
    tmp->var = w->var;
    tmp->var_type = w->var_type;
    tmp->son = copy_Wff( w->son );
    break;
  case AND:
  case OR:
    for ( i = w->sons; i; i = i->next ) {
      tmp2 = copy_Wff( i );
      if ( tmp->sons ) {
	tmp->sons->prev = tmp2;
      }
      tmp2->next = tmp->sons;
      tmp->sons = tmp2;
    }
    break;
  case NOT:
    tmp->son = copy_Wff( w->son );
    break;
  case ATOM:
    tmp->fact = new_Fact();
    tmp->fact->predicate = w->fact->predicate;
    for ( j = 0; j < garity[w->fact->predicate]; j++ ) {
      tmp->fact->args[j] = w->fact->args[j];
    }
    tmp->visited = w->visited;
    break;
  case TRU:
  case FAL:
    break;
  default:
    printf("\nwon't get here: copy, non logical %d\n\n",
	   w->connective);
    exit( 1 );
  }

  return tmp;

}



Bool possibly_positive( Fact *f )

{

  int i;

  if ( gis_added[f->predicate] ||
       gis_unknown[f->predicate] ) {
    return TRUE;
  }

  for ( i = 0; i < gnum_initial_predicate[f->predicate]; i++ ) {
    if ( matches( f, &(ginitial_predicate[f->predicate][i]) ) ) {
      return TRUE;
    }
  }

  return FALSE;

}



Bool possibly_negative( Fact *f )

{

  int i;

  if ( gis_deleted[f->predicate] ||
       gis_unknown[f->predicate] ) {
    return TRUE;
  }

  for ( i = 0; i < garity[f->predicate]; i++ ) {
    if ( f->args[i] < 0 ) {
      return TRUE;
    }
  }

  for ( i = 0; i < gnum_initial_predicate[f->predicate]; i++ ) {
    if ( matches( f, &(ginitial_predicate[f->predicate][i]) ) ) {
      return FALSE;
    }
  }

  return TRUE;

}



Bool matches( Fact *f1, Fact *f2 )

{

  int i;

  for ( i = 0; i < garity[f1->predicate]; i++ ) {
    if ( f1->args[i] >= 0 ) {
      if ( f2->args[i] >= 0 &&
	   f1->args[i] != f2->args[i] ) {
	return FALSE;
      }
    }
  }

  return TRUE;

}



void cleanup_wff( WffNode **w )

{

  merge_ANDs_and_ORs_in_wff( w );
  detect_tautologies_in_wff( w );
  simplify_wff( w );
  detect_tautologies_in_wff( w );
  merge_ANDs_and_ORs_in_wff( w );

}



void detect_tautologies_in_wff( WffNode **w )

{

  WffNode *i, *j, *tmp;

  switch ( (*w)->connective ) {
  case ALL:
  case EX:
    detect_tautologies_in_wff( &((*w)->son) );
    break;
  case AND:
  case OR:
    for ( i = (*w)->sons; i; i = i->next ) {
      detect_tautologies_in_wff( &i );
    }
    for ( i = (*w)->sons; i && i->next; i = i->next ) {
      j = i->next;
      while ( j ) {
	if ( are_identical_ATOMs( i, j ) ) {
	  j->prev->next = j->next;
	  if ( j->next ) {
	    j->next->prev = j->prev;
	  }
	  tmp = j;
	  j = j->next;
	  if ( tmp->fact ) {
	    free( tmp->fact );
	  }
	  free( tmp );
	  continue;
	}
	if ( i->connective == NOT &&
	     are_identical_ATOMs( i->son, j ) ) {
	  (*w)->connective = ( (*w)->connective == AND ) ? FAL : TRU;
	  free_WffNode( (*w)->son );
	  (*w)->son = NULL;
	  return;
	}
	if ( j->connective == NOT &&
	     are_identical_ATOMs( i, j->son ) ) {
	  (*w)->connective = ( (*w)->connective == AND ) ? FAL : TRU;
	  free_WffNode( (*w)->son );
	  (*w)->son = NULL;
	  return;
	}
	j = j->next;
      }
    }
    break;
  case NOT:
    detect_tautologies_in_wff( &((*w)->son) );
    break;
  case ATOM:
  case TRU:
  case FAL:
    break;
  default:
    printf("\nwon't get here: tautologies, non logical %d\n\n",
	   (*w)->connective);
    exit( 1 );
  }

}



Bool are_identical_ATOMs( WffNode *w1, WffNode *w2 )

{

  int i;

  if ( w1->connective != ATOM ||
       w2->connective != ATOM ) {
    return FALSE;
  }

  if ( w1->fact->predicate != w2->fact->predicate ) {
    return FALSE;
  }

  for ( i = 0; i < garity[w1->fact->predicate]; i++ ) {
    if ( w1->fact->args[i] != w2->fact->args[i] ) {
      return FALSE;
    }
  }

  return TRUE;

}



void merge_ANDs_and_ORs_in_wff( WffNode **w )

{

  WffNode *i, *j, *tmp;

  switch ( (*w)->connective ) {
  case ALL:
  case EX:
    merge_ANDs_and_ORs_in_wff( &((*w)->son) );
    break;
  case AND:
  case OR:
    i = (*w)->sons;
    while ( i ) {
      merge_ANDs_and_ORs_in_wff( &i );
      if ( i->connective == (*w)->connective ) {
	if ( !(i->sons) ) {
	  if ( i->next ) {
	    i->next->prev = i->prev;
	  }
	  if ( i->prev ) {
	    i->prev->next = i->next;
	  } else {
	    (*w)->sons = i->next;
	  }
	  tmp = i;
	  i = i->next;
	  free( tmp );
	  continue;
	}
	for ( j = i->sons; j->next; j = j->next );
	j->next = i->next;
	if ( i->next ) {
	  i->next->prev = j;
	}
	if ( i->prev ) {
	  i->prev->next = i->sons;
	  i->sons->prev = i->prev;
	} else {
	  (*w)->sons = i->sons;
	}
	tmp = i;
	i = i->next;
	free( tmp );
	continue;
      }
      i = i->next;
    }
    break;
  case NOT:
    merge_ANDs_and_ORs_in_wff( &((*w)->son) );
    break;
  case ATOM:
  case TRU:
  case FAL:
    break;
  default:
    printf("\nwon't get here: merge, non logical %d\n\n",
	   (*w)->connective);
    exit( 1 );
  }

}



void NOTs_down_in_wff( WffNode **w )

{

  WffNode *tmp1, *tmp2, *i;

  switch ( (*w)->connective ) {
  case ALL:
  case EX:
    printf("\ntrying to put nots down in quantified formula! debug me\n\n");
    exit( 1 );
    break;
  case AND:
  case OR:
    for ( i = (*w)->sons; i; i = i->next ) {
      NOTs_down_in_wff( &i ); 
    }
    break;
  case NOT:
    if ( (*w)->son->connective == NOT ) {
      (*w)->connective = (*w)->son->son->connective;
      (*w)->fact = (*w)->son->son->fact;
      tmp1 = (*w)->son;
      tmp2 = (*w)->son->son;
      (*w)->sons = (*w)->son->son->sons;
      (*w)->son = (*w)->son->son->son;
      free( tmp1 );
      free( tmp2 );
      NOTs_down_in_wff( w );
      break;
    }
    if ( (*w)->son->connective == AND ||
	 (*w)->son->connective == OR ) {
      (*w)->connective = ( (*w)->son->connective == AND ) ? OR : AND;
      (*w)->sons = (*w)->son->sons;
      free( (*w)->son );
      (*w)->son = NULL;
      for ( i = (*w)->sons; i; i = i->next ) {
	tmp1 = new_WffNode( i->connective );
	tmp1->son = i->son;
	tmp1->sons = i->sons;
	tmp1->fact = i->fact;
	i->connective = NOT;
	i->son = tmp1;
	i->sons = NULL;
	i->fact = NULL;
	NOTs_down_in_wff( &i );
      }
      break;
    }
    break;
  case ATOM:
  case TRU:
  case FAL:
    break;
  default:
    printf("\nwon't get here: nots down, non logical %d\n\n",
	   (*w)->connective);
    exit( 1 );
  }


}











/****************************************************
 * NEGATIVE PRE- AND EFFECT- CONDITIONS TRANSLATION *
 ****************************************************/








int lconsts[MAX_ARITY];








void translate_negative_preconds( void )

{

  int i, j;
  Effect *e;
  Facts *f, *f_;

  WffNode *tmp = new_WffNode(ATOM);  
  tmp->fact = new_Fact();
  /* this fn will be called exactly once and so i happily
   * waste the memory to not free this "tmp" guy afterwards.
   */

  ginitial_ft_equivalence_A = NULL;
  ginitial_ft_equivalence_notA = NULL;

  for ( i = 0; i < MAX_PREDICATES; i++ ) {
    gtranslated_predicate_to[i] = -1;
  }

  for ( f = gunknown_initial; f; f = f->next ) {
    /* translate all unknown facts. we're going to
     * use them in the CPTs anyway.
     * I guess this renders the translation calls
     * on ORs, ONEOFs, and CPT conds superfluos.
     * who cares.
     *
     * need tmp guy to make the format suitable for the
     * fn below.
     */
    tmp->fact->predicate = f->fact->predicate;
    for ( j = 0; j < garity[tmp->fact->predicate]; j++ ) {
      tmp->fact->args[j] = f->fact->args[j];
    }
    translate_one_negative_cond( tmp, TRUE );

    for ( j = 0; j < f->num_cpt; j++ ) {
      while ( translate_one_negative_cond( f->cpt_c[j], TRUE ) );
    }
  }

  for ( i = 0; i < gnum_full_or_initial; i++ ) {
    while ( translate_one_negative_cond( gfull_or_initial[i], TRUE ) );
  }

  for ( i = 0; i < gnum_full_oneof_initial; i++ ) {
    while ( translate_one_negative_cond( gfull_oneof_initial[i], TRUE ) );
  }

  for ( i = 0; i < gnum_full_multi_initial; i++ ) {
    while ( translate_one_negative_cond( gfull_multi_initial[i], TRUE ) );
  }

  while ( translate_one_negative_cond( ggoal, FALSE ) );
  
  for ( i = 0; i < gnum_operators; i++ ) {
    while ( translate_one_negative_cond( goperators[i]->preconds, FALSE ) );

    /* need negated effconds for nondel NOOP edges in rpg path
     *
     * -- ie, facts that are, on path, known at t but U at t+1
     * .. we neeed a "preservation condition" for the 
     * "fact stays true" edge.
     */
    for ( e = goperators[i]->effects; e; e = e->next ) {
      while ( translate_one_negative_cond( e->conditions, TRUE ) );
    }
  }

  if ( gcmd_line.display_info == 108 ) {
    printf("\n\ndomain with translated negative conds:");

    printf("\n\noperators are:");
    for ( i = 0; i < gnum_operators; i++ ) {
      print_Operator( goperators[i] );
    }
    printf("\n\n");

    printf("\ninitial state is, known part:\n");
    for ( f = ginitial; f; f = f->next ) {
      printf("\n");
      print_Fact( f->fact );
    }
    printf("\ninitial state is, unknown part:\n");
    for ( f = gunknown_initial; f; f = f->next ) {
      printf("\n");
      print_Fact( f->fact );
      printf("\nCPT:\n");
      for ( j = 0; j < f->num_cpt; j++ ) {
	printf("probability: %f\n", f->cpt_p[j]);
	printf("if: \n");
	print_Wff( f->cpt_c[j], 0 );
      }      
    }
    printf("\ninitial state is, multis:\n");
    for ( i = 0; i < gnum_full_multi_initial; i++ ) {
      print_Wff( gfull_multi_initial[i], 0 );
    }
    printf("\ninitial state is, ors:\n");
    for ( i = 0; i < gnum_full_or_initial; i++ ) {
      print_Wff( gfull_or_initial[i], 0 );
    }
    printf("\ninitial state is, oneofs:\n");
    for ( i = 0; i < gnum_full_oneof_initial; i++ ) {
      print_Wff( gfull_oneof_initial[i], 0 );
    }
    printf("\n\n");
    printf("\ninitial state equivalences:\n");
    for ( f = ginitial_ft_equivalence_A, 
	    f_ = ginitial_ft_equivalence_notA; 
	  f; f = f->next, f_ = f_->next ) {
      printf("\nneg ");
      print_Fact( f->fact );
      printf(" <-> ");
      print_Fact( f_->fact );
    }
    printf("\n\n");

    printf("\n\nindividual predicates, known part:\n");
    for ( i = 0; i < gnum_predicates; i++ ) {
      printf("\n\n%s:", gpredicates[i]);
      if ( !gis_added[i] &&
	   !gis_deleted[i] &&
	   !gis_unknown[i] ) {
	printf(" ---  STATIC");
      }
      for ( j = 0; j < gnum_initial_predicate[i]; j++ ) {
	printf("\n");
	print_Fact( &(ginitial_predicate[i][j]) );
      }
    }
    printf("\n\nindividual predicates, unknown part:\n");
    for ( i = 0; i < gnum_predicates; i++ ) {
      printf("\n\n%s:", gpredicates[i]);
      if ( !gis_added[i] &&
	   !gis_deleted[i] && 
	   !gis_unknown[i] ) {
	printf(" ---  STATIC");
      }
      for ( j = 0; j < gnum_unknown_initial_predicate[i]; j++ ) {
	printf("\n");
	print_Fact( &(gunknown_initial_predicate[i][j]) );
      }
    }
    printf("\n\n");

    printf("\n\ngoal is:\n");
    print_Wff( ggoal, 0 );
    printf("\n\n");

    printf("\n\nneg translation table:");
    for ( i = 0; i < gnum_predicates; i++ ) {
      printf("\n%s -> neg %s", 
	     gpredicates[i],
	     gtranslated_predicate_to[i] == -1 ? "NONE" : gpredicates[gtranslated_predicate_to[i]]);
    }
    
  }

}



Bool translate_one_negative_cond( WffNode *w, Bool translate_all )

{

  WffNode *i;
  int p, j, k, m;
  Effect *e;
  Literal *l, *tmp;
  Bool nonnon = FALSE;
  Facts *f;

  switch ( w->connective ) {
  case ALL:
  case EX:
    printf("\ntranslating NOT in quantified formula! debug me\n\n");
    exit( 1 );
  case AND:
  case OR:
  case ONEOF:
  case MULTI:
    for ( i = w->sons; i; i = i->next ) {
      if ( translate_one_negative_cond( i, translate_all ) ) {
	return TRUE;
      }
    }
    return FALSE;
  case NOT:
    if ( w->son->fact->predicate == -1 ) {
      return FALSE;
    }
    break;
  case ATOM:
    if ( translate_all &&
	 w->fact->predicate != -1 &&
	 gtranslated_predicate_to[w->fact->predicate] == -1 ) {
      nonnon = TRUE;
      break;
    }
  case TRU:
  case FAL:
    return FALSE;
  default:
    printf("\nwon't get here: translate neg cond, non logical %d\n\n",
	   w->connective);
    exit( 1 );
  }


  if ( gnum_predicates == MAX_PREDICATES ) {
    printf("\ntoo many predicates in translation! increase MAX_PREDICATES (currently %d)\n\n",
	   MAX_PREDICATES);
    exit( 1 );
  }
  if ( nonnon ) {
    p = w->fact->predicate;
  } else {
    p = w->son->fact->predicate;
  }
  gpredicates[gnum_predicates] = new_Token( strlen( gpredicates[p] ) + 5 );
  sprintf( gpredicates[gnum_predicates], "NOT-%s", gpredicates[p] );
  garity[gnum_predicates] = garity[p];
  for ( j = 0; j < garity[p]; j++ ) {
    gpredicates_args_type[gnum_predicates][j] = 
      gpredicates_args_type[p][j];
  }
  gis_added[gnum_predicates] = FALSE;
  gis_deleted[gnum_predicates] = FALSE;
  gis_unknown[gnum_predicates] = FALSE;
  for ( j = 0; j < gnum_full_unknown_initial; j++ ) {
    if ( gfull_unknown_initial[j].predicate == p ) {
      gis_unknown[gnum_predicates] = TRUE;
    } 
  }
  m = 1;
  for ( j = 0; j < garity[gnum_predicates]; j++ ) {
    m *= gtype_size[gpredicates_args_type[gnum_predicates][j]];
  }
  ginitial_predicate[gnum_predicates] = ( Fact * ) calloc( m, sizeof( Fact ) );
  gunknown_initial_predicate[gnum_predicates] = ( Fact * ) calloc( m, sizeof( Fact ) );
  gnum_predicates++;

  /* need to remember the translations made.
   */
  gtranslated_predicate_to[p] = gnum_predicates - 1;
  gtranslated_predicate_to[gnum_predicates - 1] = p;

  /* now get rid of the negated conds.
   */
  for ( f = gunknown_initial; f; f = f->next ) {
    for ( j = 0; j < f->num_cpt; j++ ) {
      replace_not_p_with_n_in_wff(p, gnum_predicates - 1, &(f->cpt_c[j]));
    }
  }

  for ( j = 0; j < gnum_full_or_initial; j++ ) {
    replace_not_p_with_n_in_wff( p, gnum_predicates - 1, &(gfull_or_initial[j]) );
  }

  replace_not_p_with_n_in_wff( p, gnum_predicates - 1, &ggoal );

  for ( j = 0; j < gnum_operators; j++ ) {
    replace_not_p_with_n_in_wff( p, gnum_predicates - 1, 
				 &(goperators[j]->preconds) );

    for ( e = goperators[j]->effects; e; e = e->next ) {
      replace_not_p_with_n_in_wff( p, gnum_predicates - 1, 
				   &(e->conditions) );
      for ( l = e->effects; l; l = l->next ) {
	if ( l->fact.predicate != p ) {
	  continue;
	}
	tmp = new_Literal();
	if ( l->negated ) {
	  tmp->negated = FALSE;
	  gis_added[gnum_predicates - 1] = TRUE;
	} else {
	  tmp->negated = TRUE;
	  gis_deleted[gnum_predicates - 1] = TRUE;
	}
	tmp->fact.predicate = gnum_predicates - 1;
	for ( k = 0; k < garity[p]; k++ ) {
	  tmp->fact.args[k] = l->fact.args[k];
	  }
	if ( l->prev ) {
	  tmp->prev = l->prev;
	  tmp->prev->next = tmp;
	} else {
	  e->effects = tmp;
	}
	tmp->next = l;
	l->prev = tmp;
      }
    }
  }

  add_to_initial_state( p, gnum_predicates - 1, 0 );

  return TRUE;

}



void replace_not_p_with_n_in_wff( int p, int n, WffNode **w )

{

  WffNode *i;

  switch ( (*w)->connective ) {
  case ALL:
  case EX:
    printf("\nreplacing p with NOT-p in quantified formula! debug me\n\n");
    exit( 1 );
  case AND:
  case OR:
    for ( i = (*w)->sons; i; i = i->next ) {
      replace_not_p_with_n_in_wff( p, n, &i );
    }
    break;
  case NOT:
    if ( (*w)->son->fact->predicate == p ) {
      (*w)->connective = ATOM;
      (*w)->NOT_p = p;
      (*w)->fact = (*w)->son->fact;
      (*w)->fact->predicate = n;
      free( (*w)->son );
      (*w)->son = NULL;
    }
    break;
  case ATOM:
  case TRU:
  case FAL:
    break;
  default:
    printf("\nwon't get here: replace p with NOT-p, non logical %d\n\n",
	   (*w)->connective);
    exit( 1 );
  }

}



void add_to_initial_state( int p, int n, int index )

{

  int i, j;
  Facts *tmp, *tmp2, *f;

  if ( index == garity[p] ) {
    /* remember this fact translation
     */
    tmp = new_Facts();
    tmp2 = new_Facts();
    tmp->fact->predicate = p;
    tmp2->fact->predicate = n;
    for ( i = 0; i < garity[p]; i++ ) {
	tmp->fact->args[i] = lconsts[i];
	tmp2->fact->args[i] = lconsts[i];
    }
    tmp->next = ginitial_ft_equivalence_A;
    ginitial_ft_equivalence_A = tmp;
    tmp2->next = ginitial_ft_equivalence_notA;
    ginitial_ft_equivalence_notA = tmp2;
    gnum_initial_ft_equivalence++;

    /* see if contrary fact is initially unknown
     */
    for ( i = 0; i < gnum_unknown_initial_predicate[p]; i++ ) {
      for ( j = 0; j < garity[p]; j++ ) {
	if ( gunknown_initial_predicate[p][i].args[j] != lconsts[j] ) {
	  break;
	}
      }
      if ( j == garity[p] ) {
	break;
      }
    }
    if ( i < gnum_unknown_initial_predicate[p] ) {
      /* yes: insert the new fact into unknown initials
       * (dependency is stored for all, not only the unknown, facts)
       */
      gunknown_initial_predicate[n][gnum_unknown_initial_predicate[n]].predicate = n;
      for ( i = 0; i < garity[n]; i++ ) {
	gunknown_initial_predicate[n][gnum_unknown_initial_predicate[n]].args[i] = lconsts[i];
      }
      gnum_unknown_initial_predicate[n]++;

      /* also insert into unknown initial state
       */
      tmp = new_Facts();
      tmp->fact->predicate = n;
      for ( i = 0; i < garity[p]; i++ ) {
	tmp->fact->args[i] = lconsts[i];
      }

      /* copy (and modify) CPT over from corresponding fact; first, find it in the list.
       */
      for ( f = gunknown_initial; f; f = f->next ) {
	if ( f->fact->predicate == p ) {
	  for ( j = 0; j < garity[p]; j++ ) {
	    if ( f->fact->args[j] != lconsts[j] ) {
	      break;
	    }
	  }
	  if ( j == garity[p] ) {
	    break;
	  }
	}
      }
      if ( !f ) {
	printf("\n\ncan't find unknown pos fact for CPT in list?\n\n");
	exit( 1 );
      }
      
      tmp->num_cpt = f->num_cpt;
      tmp->cpt_p = ( double * ) calloc( tmp->num_cpt, sizeof ( double ) );
      tmp->cpt_c = ( WffNode_pointer * ) calloc( tmp->num_cpt, sizeof ( WffNode_pointer ) );
      
      for ( j = 0; j < tmp->num_cpt; j++ ) {
	tmp->cpt_p[j] = 1.0 - f->cpt_p[j];/* negated fact true iff pos fact not true.. */
	tmp->cpt_c[j] = f->cpt_c[j];
      }
      /* ATTENTION! condition representations coincide!
       */

      tmp->next = gunknown_initial;
      gunknown_initial = tmp;
      gnum_unknown_initial++;

      return;
    }

    /* now see if contrary fact is in known-true ini
     */
    for ( i = 0; i < gnum_initial_predicate[p]; i++ ) {
      for ( j = 0; j < garity[p]; j++ ) {
	if ( ginitial_predicate[p][i].args[j] != lconsts[j] ) {
	  break;
	}
      }
      if ( j == garity[p] ) {
	break;
      }
    }
    if ( i < gnum_initial_predicate[p] ) {
      return;
    }
    /* no: add new fact to ini
     */
    ginitial_predicate[n][gnum_initial_predicate[n]].predicate = n;
    for ( i = 0; i < garity[n]; i++ ) {
      ginitial_predicate[n][gnum_initial_predicate[n]].args[i] = lconsts[i];
    }
    gnum_initial_predicate[n]++;

    if ( !gis_added[n] &&
	 !gis_deleted[n] &&
	 !gis_unknown[n] ) {
      return;
    }

    tmp = new_Facts();
    tmp->fact->predicate = n;
    for ( i = 0; i < garity[p]; i++ ) {
      tmp->fact->args[i] = lconsts[i];
    }
    tmp->next = ginitial;
    ginitial = tmp;
    gnum_initial++;
    return;
  }

  for ( i = 0; i < gtype_size[gpredicates_args_type[p][index]]; i++ ) {
    lconsts[index] = gtype_consts[gpredicates_args_type[p][index]][i];
    add_to_initial_state( p, n, index + 1 );
  }

}



















/*******************************************************************
 * SPLIT DOMAIN IN PREPARATION FOR SEPARATE INSTANTIATION ROUTINES *
 *******************************************************************/


















void split_domain( void )

{

  int i, j, m, s = 0, a;
  Effect *e;
  WffNode *w, *ww, *www;
  NormOperator *tmp_op;
  Fact *tmp_ft;

  for ( i = 0; i < MAX_TYPES; i++ ) {
    gnum_intersected_types[i] = -1;
  }

  for ( i = 0; i < gnum_operators; i++ ) {
    if ( (m = is_dnf( goperators[i]->preconds )) != -1 ) {
      for ( e = goperators[i]->effects; e; e = e->next ) {
	if ( is_dnf( e->conditions ) == -1 ) {
	  break;
	}
      }
      if ( !e ) {
	goperators[i]->hard = FALSE;
	s += m;
      }
    }
  }

  ghard_operators = ( Operator_pointer * ) calloc( MAX_OPERATORS, sizeof( Operator ) );
  gnum_hard_operators = 0; 
  geasy_operators = ( NormOperator_pointer * ) calloc( s, sizeof( NormOperator_pointer ) );
  gnum_easy_operators = 0;

  for ( i = 0; i < gnum_operators; i++ ) {
    if ( goperators[i]->hard ) {
      ghard_operators[gnum_hard_operators++] = goperators[i];
      continue;
    }
    w = goperators[i]->preconds;
    switch ( w->connective ) {
    case OR:
      printf("\nsorry -- this version of cff can't handle disjunctive preconditions.\n\n");
      exit( 1 );
      for ( ww = w->sons; ww; ww = ww->next ) {
	tmp_op = new_NormOperator( goperators[i] );
	if ( ww->connective == AND ) {
	  m = 0; 
	  for ( www = ww->sons; www; www = www->next ) m++;
	  tmp_op->preconds = ( Fact * ) calloc( m, sizeof( Fact ) );
	  for ( www = ww->sons; www; www = www->next ) {
	    tmp_ft = &(tmp_op->preconds[tmp_op->num_preconds]);
	    tmp_ft->predicate = www->fact->predicate;
	    a = garity[tmp_ft->predicate];
	    for ( j = 0; j < a; j++ ) {
	      tmp_ft->args[j] = www->fact->args[j];
	    }
	    tmp_op->num_preconds++;
	  }
	} else {
	  tmp_op->preconds = ( Fact * ) calloc( 1, sizeof( Fact ) );
	  tmp_ft = &(tmp_op->preconds[0]);
	  tmp_ft->predicate = ww->fact->predicate;
	  a = garity[tmp_ft->predicate];
	  for ( j = 0; j < a; j++ ) {
	    tmp_ft->args[j] = ww->fact->args[j];
	  }
	  tmp_op->num_preconds = 1;
	}
	make_normal_effects( &tmp_op, goperators[i] );
	geasy_operators[gnum_easy_operators++] = tmp_op;
      }
      break;
    case AND:
      tmp_op = new_NormOperator( goperators[i] );
      m = 0; 
      for ( ww = w->sons; ww; ww = ww->next ) m++;
      tmp_op->preconds = ( Fact * ) calloc( m, sizeof( Fact ) );
      for ( ww = w->sons; ww; ww = ww->next ) {
	tmp_ft = &(tmp_op->preconds[tmp_op->num_preconds]);
	tmp_ft->predicate = ww->fact->predicate;
	a = garity[tmp_ft->predicate];
	for ( j = 0; j < a; j++ ) {
	  tmp_ft->args[j] = ww->fact->args[j];
	}
	tmp_op->num_preconds++;
      }
      make_normal_effects( &tmp_op, goperators[i] );
      geasy_operators[gnum_easy_operators++] = tmp_op;
      break;
    case NOT:
    case ATOM:
      tmp_op = new_NormOperator( goperators[i] );
      tmp_op->preconds = ( Fact * ) calloc( 1, sizeof( Fact ) );
      tmp_ft = &(tmp_op->preconds[0]);
      tmp_ft->predicate = w->fact->predicate;
      a = garity[tmp_ft->predicate];
      for ( j = 0; j < a; j++ ) {
	tmp_ft->args[j] = w->fact->args[j];
      }
      tmp_op->num_preconds = 1;
      make_normal_effects( &tmp_op, goperators[i] );
      geasy_operators[gnum_easy_operators++] = tmp_op;
      break;
    case TRU:
      tmp_op = new_NormOperator( goperators[i] );
      make_normal_effects( &tmp_op, goperators[i] );
      geasy_operators[gnum_easy_operators++] = tmp_op;
      break;
    case FAL:
      break;
    default:
      printf("\nwon't get here: non OR, AND, ATOM, TRUE, FALSE in dnf. debug me\n\n");
      exit( 1 );
    }
  }

  if ( gcmd_line.display_info == 109 ) {
    printf("\n\nsplitted operators are:\n");
    
    printf("\nEASY:\n");
    for ( i = 0; i < gnum_easy_operators; i++ ) {
      print_NormOperator( geasy_operators[i] );
    }

    printf("\n\n\nHARD:\n");
    for ( i = 0; i < gnum_hard_operators; i++ ) {
      print_Operator( ghard_operators[i] );
    }
  } 

}



int is_dnf( WffNode *w )

{

  WffNode *i;
  int s = 0;

  switch ( w->connective ) {
  case ALL:
  case EX:
    printf("\nchecking quantifier for dnf. debug me\n\n");
    exit( 1 );
  case AND:
    for ( i = w->sons; i; i = i->next ) {
      if ( i->connective == ATOM ||
	   ( i->connective == NOT &&
	     i->son->fact->predicate == -1 ) ) {
	continue;
      }
      return -1;
    }
    return 1;
  case OR:
    for ( i = w->sons; i; i = i->next ) {
      s++;
      if ( i->connective == ATOM ||
	   ( i->connective == NOT &&
	     i->son->fact->predicate == -1 ) ||
	   ( i->connective == AND &&
	     is_dnf( i ) != -1 ) ) {
	continue;
      }
      return -1;
    }
    return s;
  case NOT:
    if ( w->son->fact->predicate == -1 ) {
      return 1;
    } else {
      printf("\nNOT with non eq - son in presimplified formula. debug me\n\n");
      exit( 1 );
    }
  case ATOM:
  case TRU:
  case FAL:
    return 1;
  default:
    printf("\nwon't get here: check dnf, non logical %d\n\n",
	   w->connective);
    exit( 1 );
  }

}



void make_normal_effects( NormOperator **nop, Operator *op )

{

  Effect *e;
  NormEffect *tmp_ef;
  WffNode *w, *ww, *www;
  int j, a, m, ma, md;
  Literal *l;
  Fact *tmp_ft;

  for ( e = op->effects; e; e = e->next ) {
    w = e->conditions;
    switch ( w->connective ) {
    case OR:
      for ( ww = w->sons; ww; ww = ww->next ) {
	tmp_ef = new_NormEffect1( e );
	tmp_ef->eff_p = e->eff_p;/* july06: copy over eff prob */
	if ( ww->connective == AND ) {
	  m = 0;
	  for ( www = ww->sons; www; www = www->next ) m++;
	  tmp_ef->conditions = ( Fact * ) calloc( m, sizeof( Fact ) );
	  for ( www = ww->sons; www; www = www->next ) {
	    tmp_ft = &(tmp_ef->conditions[tmp_ef->num_conditions]);
	    tmp_ft->predicate = www->fact->predicate;
	    a = garity[tmp_ft->predicate];
	    for ( j = 0; j < a; j++ ) {
	      tmp_ft->args[j] = www->fact->args[j];
	    }
	    tmp_ef->num_conditions++;
	  }
	} else {
	  tmp_ef->conditions = ( Fact * ) calloc( 1, sizeof( Fact ) );
	  tmp_ft = &(tmp_ef->conditions[0]);
	  tmp_ft->predicate = ww->fact->predicate;
	  a = garity[tmp_ft->predicate];
	  for ( j = 0; j < a; j++ ) {
	    tmp_ft->args[j] = ww->fact->args[j];
	  }
	  tmp_ef->num_conditions = 1;
	}
	ma = 0;
	md = 0;
	for ( l = e->effects; l; l = l->next ) {
	  if ( l->negated ) {
	    md++;
	  } else {
	    ma++;
	  }
	}
	tmp_ef->adds = ( Fact * ) calloc( ma, sizeof( Fact ) );
	tmp_ef->dels = ( Fact * ) calloc( md, sizeof( Fact ) );
	for ( l = e->effects; l; l = l->next ) {
	  if ( l->negated ) {
	    tmp_ft = &(tmp_ef->dels[tmp_ef->num_dels++]);
	  } else {
	    tmp_ft = &(tmp_ef->adds[tmp_ef->num_adds++]);
	  }
	  tmp_ft->predicate = l->fact.predicate;
	  for ( j = 0; j < garity[tmp_ft->predicate]; j++ ) {
	    tmp_ft->args[j] = l->fact.args[j];
	  }
	}
	tmp_ef->next = (*nop)->effects;
	if ( (*nop)->effects ) {
	  (*nop)->effects->prev = tmp_ef;
	}
	(*nop)->effects = tmp_ef;
      }
      break;
    case AND:
      tmp_ef = new_NormEffect1( e );
      tmp_ef->eff_p = e->eff_p;/* july06: copy over eff prob */
      m = 0;
      for ( ww = w->sons; ww; ww = ww->next ) m++;
      tmp_ef->conditions = ( Fact * ) calloc( m, sizeof( Fact ) );
      for ( ww = w->sons; ww; ww = ww->next ) {
	tmp_ft = &(tmp_ef->conditions[tmp_ef->num_conditions]);
	tmp_ft->predicate = ww->fact->predicate;
	a = garity[tmp_ft->predicate];
	for ( j = 0; j < a; j++ ) {
	  tmp_ft->args[j] = ww->fact->args[j];
	}
	tmp_ef->num_conditions++;
      }
      ma = 0;
      md = 0;
      for ( l = e->effects; l; l = l->next ) {
	if ( l->negated ) {
	  md++;
	} else {
	  ma++;
	}
      }
      tmp_ef->adds = ( Fact * ) calloc( ma, sizeof( Fact ) );
      tmp_ef->dels = ( Fact * ) calloc( md, sizeof( Fact ) );
      for ( l = e->effects; l; l = l->next ) {
	if ( l->negated ) {
	  tmp_ft = &(tmp_ef->dels[tmp_ef->num_dels++]);
	} else {
	  tmp_ft = &(tmp_ef->adds[tmp_ef->num_adds++]);
	}
	tmp_ft->predicate = l->fact.predicate;
	for ( j = 0; j < garity[tmp_ft->predicate]; j++ ) {
	  tmp_ft->args[j] = l->fact.args[j];
	}
      }
      tmp_ef->next = (*nop)->effects;
      if ( (*nop)->effects ) {
	(*nop)->effects->prev = tmp_ef;
      }
      (*nop)->effects = tmp_ef;
      break;
    case NOT:
    case ATOM:
      tmp_ef = new_NormEffect1( e );
      tmp_ef->eff_p = e->eff_p;/* july06: copy over eff prob */
      tmp_ef->conditions = ( Fact * ) calloc( 1, sizeof( Fact ) );
      tmp_ft = &(tmp_ef->conditions[0]);
      tmp_ft->predicate = w->fact->predicate;
      a = garity[tmp_ft->predicate];
      for ( j = 0; j < a; j++ ) {
	tmp_ft->args[j] = w->fact->args[j];
      }
      tmp_ef->num_conditions = 1;
      ma = 0;
      md = 0;
      for ( l = e->effects; l; l = l->next ) {
	if ( l->negated ) {
	  md++;
	} else {
	  ma++;
	}
      }
      tmp_ef->adds = ( Fact * ) calloc( ma, sizeof( Fact ) );
      tmp_ef->dels = ( Fact * ) calloc( md, sizeof( Fact ) );
      for ( l = e->effects; l; l = l->next ) {
	if ( l->negated ) {
	  tmp_ft = &(tmp_ef->dels[tmp_ef->num_dels++]);
	} else {
	  tmp_ft = &(tmp_ef->adds[tmp_ef->num_adds++]);
	}
	tmp_ft->predicate = l->fact.predicate;
	for ( j = 0; j < garity[tmp_ft->predicate]; j++ ) {
	  tmp_ft->args[j] = l->fact.args[j];
	}
      }
      tmp_ef->next = (*nop)->effects;
      if ( (*nop)->effects ) {
	(*nop)->effects->prev = tmp_ef;
      }
      (*nop)->effects = tmp_ef;
      break;     
    case TRU:
      tmp_ef = new_NormEffect1( e );
      tmp_ef->eff_p = e->eff_p;/* july06: copy over eff prob */
      ma = 0;
      md = 0;
      for ( l = e->effects; l; l = l->next ) {
	if ( l->negated ) {
	  md++;
	} else {
	  ma++;
	}
      }
      tmp_ef->adds = ( Fact * ) calloc( ma, sizeof( Fact ) );
      tmp_ef->dels = ( Fact * ) calloc( md, sizeof( Fact ) );
      for ( l = e->effects; l; l = l->next ) {
	if ( l->negated ) {
	  tmp_ft = &(tmp_ef->dels[tmp_ef->num_dels++]);
	} else {
	  tmp_ft = &(tmp_ef->adds[tmp_ef->num_adds++]);
	}
	tmp_ft->predicate = l->fact.predicate;
	for ( j = 0; j < garity[tmp_ft->predicate]; j++ ) {
	  tmp_ft->args[j] = l->fact.args[j];
	}
      }
      tmp_ef->next = (*nop)->effects;
      if ( (*nop)->effects ) {
	(*nop)->effects->prev = tmp_ef;
      }
      (*nop)->effects = tmp_ef;
      break;
    case FAL:
      break;
    default:
      printf("\nwon't get here: non OR, AND, ATOM, TRUE, FALSE in dnf. debug me\n\n");
      exit( 1 );
    }
  }

}









/*************************************************************************
 * ADDITIONAL: FULL DNF, only compute on fully instantiated formulae!!!! *
 *************************************************************************/










/* dnf
 */

WffNode *lhitting_sets;
WffNode_pointer *lset;
int lmax_set;






void dnf( WffNode **w )

{

  static Bool first_call = TRUE;

  if ( first_call ) {
    lset = ( WffNode_pointer * ) 
      calloc( MAX_HITTING_SET_DEFAULT, sizeof( WffNode_pointer ) );
    lmax_set = MAX_HITTING_SET_DEFAULT;
    first_call = FALSE;
  }

  ANDs_below_ORs_in_wff( w );

}



void ANDs_below_ORs_in_wff( WffNode **w )

{

  WffNode *i, *tmp;
  int c, m;

  switch ( (*w)->connective ) {
  case ALL:
  case EX:
    printf("\ntrying to put quantified formula into DNF! (ands down) debug me\n\n");
    exit( 1 );
    break;
  case AND:
    c = 0;
    m = 0;
    for ( i = (*w)->sons; i; i = i->next ) {
      ANDs_below_ORs_in_wff( &i );
      if ( i->connective == OR ) {
	c++;
      }
      m++;
    }
    if ( c == 0 ) {
      /* no ORs as sons --> all sons are literals. OK
       */
      merge_next_step_ANDs_and_ORs_in_wff( w );
      break;
    }
    /* crucial part: AND node, sons can be merged OR's.
     * (i.e., sons are either literals or disjunctions of 
     * conjunctions of literals)
     * create OR node with one hitting set of w's sons for 
     * each disjunct
     */
    lhitting_sets = NULL;
    if ( m > lmax_set ) {
      free( lset );
      lset = ( WffNode_pointer * ) calloc( m, sizeof( WffNode_pointer ) );
      lmax_set = m;
    }
    collect_hitting_sets( (*w)->sons, 0 );
    (*w)->connective = OR;
    tmp = (*w)->sons;
    (*w)->sons = lhitting_sets;
    free_WffNode( tmp );
    merge_next_step_ANDs_and_ORs_in_wff( w );
    break;
  case OR:
    for ( i = (*w)->sons; i; i = i->next ) {
      ANDs_below_ORs_in_wff( &i );
    }
    merge_next_step_ANDs_and_ORs_in_wff( w );
    break;
  case NOT:
  case ATOM:
  case TRU:
  case FAL:
    break;
  default:
    printf("\nwon't get here: ands down, non logical %d\n\n",
	   (*w)->connective);
    exit( 1 );
  }

}



void collect_hitting_sets( WffNode *ORlist, int index )

{

  WffNode *tmp1, *tmp2, *j;
  int i;

  if ( !ORlist ) {
    tmp1 = new_WffNode( AND );
    for ( i = 0; i < index; i++ ) {
      tmp2 = copy_Wff( lset[i] );
      tmp2->next = tmp1->sons;
      if ( tmp1->sons ) {
	tmp1->sons->prev = tmp2;
      }
      tmp1->sons = tmp2;
    }
    tmp1->next = lhitting_sets;
    if ( lhitting_sets ) {
      lhitting_sets->prev = tmp1;
    }
    lhitting_sets = tmp1;
    return;
  }

  if ( ORlist->connective != OR ) {
    lset[index] = ORlist;
    collect_hitting_sets( ORlist->next, index + 1 );
    return;
  }

  for ( j = ORlist->sons; j; j = j->next ) {
    lset[index] = j;
    collect_hitting_sets( ORlist->next, index + 1 );
  }

}



void merge_next_step_ANDs_and_ORs_in_wff( WffNode **w )

{

  WffNode *i, *j, *tmp;

  i = (*w)->sons;
  while ( i ) {
    if ( i->connective == (*w)->connective ) {
      if ( !(i->sons) ) {
	if ( i->next ) {
	  i->next->prev = i->prev;
	}
	if ( i->prev ) {
	  i->prev->next = i->next;
	} else {
	  (*w)->sons = i->next;
	}
	tmp = i;
	i = i->next;
	free( tmp );
	continue;
      }
      for ( j = i->sons; j->next; j = j->next );
      j->next = i->next;
      if ( i->next ) {
	i->next->prev = j;
      }
      if ( i->prev ) {
	i->prev->next = i->sons;
	i->sons->prev = i->prev;
      } else {
	(*w)->sons = i->sons;
      }
      tmp = i;
      i = i->next;
      free( tmp );
      continue;
    }
    i = i->next;
  }

}



/*   switch ( (*w)->connective ) { */
/*   case ALL: */
/*   case EX: */
/*     break; */
/*   case AND: */
/*   case OR: */
/*     for ( i = (*w)->sons; i; i = i->next ) { */
/*     } */
/*     break; */
/*   case NOT: */
/*     break; */
/*   case ATOM: */
/*   case TRU: */
/*   case FAL: */
/*     break; */
/*   default: */
/*     printf("\nwon't get here: remove var, non logical %d\n\n", */
/* 	   (*w)->connective); */
/*     exit( 1 ); */
/*   } */
