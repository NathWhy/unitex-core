/*
 * Unitex
 *
 * Copyright (C) 2001-2012 Université Paris-Est Marne-la-Vallée <unitex@univ-mlv.fr>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA.
 *
 */

/* Created by Agata Savary (savary@univ-tours.fr)
 */


////////////////////////////////////////////////////////////
// Implementation of the management of inflectional tranducers
////////////////////////////////////////////////////////////
// Copy of some functions from Inflect.cpp

#include <stdlib.h>
#include "MF_Global.h"
#include "MF_InflectTransd.h"
#include "Error.h"
#include "File.h"
#include "Grf2Fst2.h"

namespace unitex {

///////////////////////////////
// ALL FUNCTIONS IN THIS MODULE
int init_transducer_tree(MultiFlex_ctx* p_multiFlex_ctx);
void free_transducer_tree(MultiFlex_ctx* p_multiFlex_ctx);
struct node* new_node();
struct transition* new_transition(char c);
void free_transition(struct transition* t);
void free_node(struct node* n);
struct transition* get_transition(char c,struct transition* t,struct node** n);
int get_node(MultiFlex_ctx* p_multiFlex_ctx,char* flex, const VersatileEncodingConfig*,
		int pos,struct node* n,const char* pkgdir);

///////////////////////////////
//Initiate the tree for inflection transducers' names
//On succes return 0, 1 otherwise
int init_transducer_tree(MultiFlex_ctx* p_multiFlex_ctx) {
  p_multiFlex_ctx->n_fst2 = 0;
  p_multiFlex_ctx->root=new_node();
  if (!(p_multiFlex_ctx->root)) {
    error("Transducer tree could not be initialized in function 'init_transducer_tree'\n");
    return 1;
  }
  return  0;
}

///////////////////////////////
// Free the transducer tree memory
void free_transducer_tree(MultiFlex_ctx* p_multiFlex_ctx) {
free_node(p_multiFlex_ctx->root);
}

///////////////////////////////
// Try to load the transducer flex and returns its position in the
// 'fst2' array.
int get_transducer(MultiFlex_ctx* p_multiFlex_ctx,char* flex,
		const VersatileEncodingConfig* vec,const char* pkgdir) {
return get_node(p_multiFlex_ctx,flex,vec,0,p_multiFlex_ctx->root,pkgdir);
}

///////////////////////////////
// Create a new node in the tree
struct node* new_node() {
struct node* n=(struct node*)malloc(sizeof(struct node));
if (n==NULL) {
   fatal_alloc_error("new_node");
}
n->final=-1;
n->t=NULL;
return n;
}

///////////////////////////////
// Create a new branch in the tree
struct transition* new_transition(char c) {
struct transition* t=(struct transition*)malloc(sizeof(struct transition));
if (t==NULL) {
   fatal_alloc_error("new_transition");
}
t->c=c;
t->n=NULL;
t->suivant=NULL;
return t;
}

///////////////////////////////
// Free the branch
void free_transition(struct transition* t) {
struct transition* tmp;
while (t!=NULL) {
    free_node(t->n);
    tmp=t;
    t=t->suivant;
    free(tmp);
}
}

///////////////////////////////
// Free a node
void free_node(struct node* n) {
if (n==NULL) {
  error("NULL error in free_node\n");
  return;
}
free_transition(n->t);
free(n);
}

///////////////////////////////
// Looks for a transition by the char c
// Creates it if it does not exist
struct transition* get_transition(char c,struct transition* t,struct node** n) {
struct transition* tmp;
while (t!=NULL) {
    if (t->c==c) return t;
    t=t->suivant;
}
tmp=new_transition(c);
tmp->suivant=(*n)->t;
tmp->n=NULL;
(*n)->t=tmp;
return tmp;
}


/**
 * Returns 1 if grf exists and is more recent than fst2; 0 otherwise.
 */
int must_compile_grf(char* grf,char* fst2) {
if (!fexists(grf)) {
   /* No .grf? We fail */
   return 0;
}
if (!fexists(fst2)) {
   /* A grf and no .fst2? Let's compile the .grf! */
   return 1;
}
if ((is_filename_in_abstract_file_space(fst2) != 0) || (is_filename_in_abstract_file_space(grf) != 0)) {
    /* abstract ? no compare, no recompile*/
    return 0;
}
 /* There are both .grf and .fst2? Let's see which file is older */
return (get_file_date(grf)>=get_file_date(fst2));
}


///////////////////////////////
// Look for the path to 'flex', creating it if necessary
// The current node is n, and pos is the position in the flex string
int get_node(MultiFlex_ctx* p_multiFlex_ctx,char* flex,
		const VersatileEncodingConfig* vec,int pos,struct node* n,const char* pkgdir) {
if (flex[pos]=='\0') {
    // we are at the final node for flex (a leaf)
    if (n->final!=-1) {
        // if the automaton already exists we returns its position in the transducer array (fst2)
        return n->final;
    }
    else {
        // else we load it
        if ((p_multiFlex_ctx->n_fst2)==N_FST2) {
            fatal_error("Memory error: too much inflectional transducers\n");
        }
        char s[FILENAME_MAX];
        new_file(p_multiFlex_ctx->inflection_directory,flex,s);
        strcat(s,".fst2");
        char grf[FILENAME_MAX];
        new_file(p_multiFlex_ctx->inflection_directory,flex,grf);
        strcat(grf,".grf");
        if (must_compile_grf(grf,s)) {
           /* If there is no .fst2 file, of a one than is older than the
            * corresponding .grf, we try to compile it */
           pseudo_main_Grf2Fst2(vec,grf,1,NULL,1,0,pkgdir);
        }
        p_multiFlex_ctx->fst2[p_multiFlex_ctx->n_fst2]=load_abstract_fst2(vec,s,1,&(p_multiFlex_ctx->fst2_free[p_multiFlex_ctx->n_fst2]));
        n->final=p_multiFlex_ctx->n_fst2;
        return (p_multiFlex_ctx->n_fst2)++;
        }
}
// if we are not at the end of the string flex
struct transition* trans=get_transition(flex[pos],n->t,&n);
if (trans->n==NULL) {
    trans->n=new_node();
}
return get_node(p_multiFlex_ctx,flex,vec,pos+1,trans->n,pkgdir);
}

} // namespace unitex
