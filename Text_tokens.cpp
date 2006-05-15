 /*
  * Unitex
  *
  * Copyright (C) 2001-2006 Universit� de Marne-la-Vall�e <unitex@univ-mlv.fr>
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

//---------------------------------------------------------------------------
#include "Text_tokens.h"
//---------------------------------------------------------------------------

int NUMBER_OF_TEXT_TOKENS=0;

struct text_tokens* new_text_tokens() {
struct text_tokens* tmp;
tmp=(struct text_tokens*)malloc(sizeof(struct text_tokens));
tmp->N=0;
tmp->SENTENCE_MARKER=-1;
tmp->token=NULL;
return tmp;
}


struct text_tokens* load_text_tokens(char* nom) {
FILE* f;
f=u_fopen(nom,U_READ);
if (f==NULL) {
   return NULL;
}
struct text_tokens* res;
res=new_text_tokens();
res->N=u_read_int(f);
res->token=(unichar**)malloc((res->N)*sizeof(unichar*));
unichar tmp[1000];
res->SENTENCE_MARKER=-1;
int i=0;
while (u_read_line(f,tmp)) {
  res->token[i]=(unichar*)malloc(sizeof(unichar)*(1+u_strlen(tmp)));
  u_strcpy(res->token[i],tmp);
  if (!u_strcmp_char(tmp,"{S}")) {
     res->SENTENCE_MARKER=i;
  } else if (!u_strcmp_char(tmp," ")) {
            (res->SPACE)=i;
         }
    else if (!u_strcmp_char(tmp,"{STOP}")) {
            (res->STOP_MARKER)=i;
         }
  i++;
}
u_fclose(f);
return res;
}



struct string_hash* load_text_tokens_hash(char* nom) {
FILE* f;
f=u_fopen(nom,U_READ);
if (f==NULL) {
   return NULL;
}
NUMBER_OF_TEXT_TOKENS=u_read_int(f)+100000; // the +100000 is used to prevent the addition
                                            // of tokens while locate preprocessing
struct string_hash* res;
res=new_string_hash_N(NUMBER_OF_TEXT_TOKENS);
unichar tmp[1000];
while (u_read_line(f,tmp)) {
  get_hash_number(tmp,res);
}
u_fclose(f);
return res;
}



struct string_hash* load_text_tokens_hash(char* nom,int *SENTENCE_MARKER,
                                          int* STOP_MARKER) {
FILE* f;
f=u_fopen(nom,U_READ);
if (f==NULL) {
   return NULL;
}
(*SENTENCE_MARKER)=-1;
NUMBER_OF_TEXT_TOKENS=u_read_int(f)+100000; // the +100000 is used to prevent the addition
                                            // of tokens while locate preprocessing
struct string_hash* res;
res=new_string_hash_N(NUMBER_OF_TEXT_TOKENS);
unichar tmp[1000];
int x;
while (u_read_line(f,tmp)) {
   x=get_hash_number(tmp,res);
   if (!u_strcmp_char(tmp,"{S}")) {
      (*SENTENCE_MARKER)=x;
   }
   else if (!u_strcmp_char(tmp,"{STOP}")) {
      (*STOP_MARKER)=x;
   }
}
u_fclose(f);
return res;
}




void free_text_tokens(struct text_tokens* tok) {
for (int i=0;i<tok->N;i++) {
  free(tok->token[i]);
}
free(tok);
}




void explorer_token_tree(int pos,unichar* sequence,Alphabet* alph,struct arbre_hash* n,struct liste_nombres** l) {
if (sequence[pos]=='\0') {
   // if we are at the end of the sequence
   if (n->final!=-1) {
      // if the sequence is a text token, we add its number to the list
      (*l)=inserer_dans_liste_nombres(n->final,*l);
   }
   return;
}
struct arbre_hash_trans* trans=n->trans;
while (trans!=NULL) {
  if (is_equal_or_uppercase(sequence[pos],trans->c,alph)) {
     // if we can follow the transition
     explorer_token_tree(pos+1,sequence,alph,trans->arr,l);
  }
  trans=trans->suivant;
}
}



struct liste_nombres* get_token_list_for_sequence(unichar* sequence,Alphabet* alph,
                                                  struct string_hash* hash) {
struct liste_nombres* l=NULL;
explorer_token_tree(0,sequence,alph,hash->racine,&l);
return l;
}



int get_token_number(unichar* s,struct text_tokens* tok) {
for (int i=0;i<tok->N;i++) {
    if (!u_strcmp(tok->token[i],s)) return i;
}
return -1;
}

unichar  *get_text_token(int token_number ,struct text_tokens* tok) {
           return(tok->token[token_number]);
}


//
// return 1 if s is a digit sequence, 0 else
//
int is_a_digit_token(unichar* s) {
int i=0;
while (s[i]!='\0') {
   if (s[i]<'0' || s[i]>'9') {
      return 0;
   }
   i++;
}
return 1;
}


//
// Scans text tokens to extract semantic codes contained in tags like {le,.DET:ms}
//
void extract_semantic_codes_from_tokens(struct string_hash* tok,
                                        struct string_hash* semantic_codes) {
for (int i=0;i<tok->N;i++) {
    if (tok->tab[i][0]=='{' && u_strcmp_char(tok->tab[i],"{S}")
                            && u_strcmp_char(tok->tab[i],"{STOP}")) {
       struct dela_entry* temp=tokenize_tag_token(tok->tab[i]);
       get_hash_number(temp->semantic_codes[0],semantic_codes);
       free_dic_entry(temp);
    }
}
}

//---------------------------------------------------------------------------

