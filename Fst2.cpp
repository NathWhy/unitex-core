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
#include "Fst2.h"
#include "Error.h"
#include "LocateConstants.h"
//---------------------------------------------------------------------------


/*
 * These two constants are declared here instead of in the .h because
 * we don't want people to write tests like (e->control & FST2_FINAL_STATE_BIT_MASK)
 * We prefer writing: is_final_state(e)
 */
#define FST2_FINAL_STATE_BIT_MASK 1
#define FST2_INITIAL_STATE_BIT_MASK 2

/* The constant NO_TAG_LIMIT must have a negative value */
#define NO_TAG_LIMIT -1


Fst2State* graphe_fst2;
Fst2Tag* etiquette_fst2;
int *debut_graphe_fst2;
int *nombre_etats_par_grf;
unichar** nom_graphe;
struct variable_list* liste_des_variables;
int nombre_etats_fst2;
int nombre_graphes_fst2;
int etat_courant;



/**
 * Allocates, initializes and returns a variable list item
 */
struct variable_list* new_variable_list(unichar* name) {
struct variable_list* v;
v=(struct variable_list*)malloc(sizeof(struct variable_list));
if (v==NULL) {
	fatal_error("Not enough memory in new_variable_list\n");
}
v->name=u_strdup(name);
v->start=-1;
v->end=-1;
v->next=NULL;
return v;
}


/**
 * Frees one variable list item
 */
void free_variable(struct variable_list* v) {
if (v->name!=NULL) free(v->name);
free(v);
}


/**
 * Frees a variable list
 */
void free_variable_list(struct variable_list* l) {
struct variable_list* tmp;
while (l!=NULL) {
	tmp=l;
	l=l->next;
	free_variable(tmp);
}
}


/**
 * Adds a variable at the end of the list, if it is not already in the list.
 * The function returns the list, modified or not.
 */
struct variable_list* add_variable_to_list(unichar* name,struct variable_list* v) {
if (v==NULL) return new_variable_list(name);
if (!u_strcmp(v->name,name)) return v;
v->next=add_variable_to_list(name,v->next);
return v;
}


/**
 * This function returns the variable list item corresponding to 'name'.
 * If it is not in the list, NULL is returned.
 */
struct variable_list* get_variable(unichar* name,struct variable_list* v) {
while (v!=NULL) {
	if (!u_strcmp(name,v->name)) return v;
	v=v->next;
}
return NULL;
}


/**
 * Frees a transition list
 */
void free_Fst2Transition(Fst2Transition t) {
struct fst2Transition* tmp;
while (t!=NULL) {
    tmp=t;
    t=t->next;
    free(tmp);
}
}


/**
 * Frees a state and all its transitions
 */
void free_Fst2State(Fst2State e) {
free_Fst2Transition(e->transitions);
free(e);
}


/**
 * Frees a tag
 */
void free_Fst2Tag(Fst2Tag e) {
if (e->input!=NULL) free(e->input);
if (e->output!=NULL) free(e->output);
if (e->inflected!=NULL) free(e->inflected);
if (e->lemma!=NULL) free(e->lemma);
if (e->codes!=NULL) free(e->codes);
#warning matching_tokens should maybe be freed here
/* $CD$ begin */
if (e -> contentGF != NULL) free(e->contentGF);
/* $CD$ end   */
free(e);
}


/**
 * Allocates, initializes and returns an empty automaton
 */
Fst2* new_Fst2() {
Fst2* a=(Fst2*)malloc(sizeof(Fst2));
a->states=NULL;
a->tags=NULL;
a->number_of_graphs=0;
a->number_of_states=0;
a->number_of_tags=0;
a->initial_states=NULL;
a->graph_names=NULL;
a->number_of_states_by_graphs=NULL;
a->variables=NULL;
return a;
}


/**
 * Frees a fst2. The function assumes that if 'fst2' is not NULL, all
 * its field are neither NULL nor already freed.
 */
void free_fst2(Fst2* fst2) {
if (fst2==NULL) return;
int i;
for (i=0;i<fst2->number_of_states;i++) {
  free_Fst2State(fst2->states[i]);
}
free(fst2->states);
for (i=0;i<fst2->number_of_tags;i++)
  free_Fst2Tag(fst2->tags[i]);
free(fst2->tags);
if (fst2->graph_names!=NULL) {
   for (i=0;i<fst2->number_of_graphs;i++) {
     if (fst2->graph_names[i]!=NULL) free(fst2->graph_names[i]);
   }
   free(fst2->graph_names);
}
free(fst2->initial_states);
free(fst2->number_of_states_by_graphs);
free_variable_list(fst2->variables);
free(fst2);
}


/**
 * Readjusts the size of the arrays, because arrays have a
 * big default size.
 */
void resize(Fst2* fst2) {
#warning replace resize by calls to realloc with size*2
/*
 * NOTE: there were +1 on fst2->number_of_states and fst2->number_of_tags,
 *       but I removed them because it seemed to be unnecessary.
 *       S. Paumier
 */
fst2->states=(Fst2State*)realloc(fst2->states,fst2->number_of_states*sizeof(Fst2State));
fst2->tags=(Fst2Tag*)realloc(fst2->tags,fst2->number_of_tags*sizeof(Fst2Tag));
/*
 * We add +1 because we start the graph numerotation at 1 
 */
int n=fst2->number_of_graphs+1;
fst2->initial_states=(int*)realloc(fst2->initial_states,n*sizeof(int));
if (fst2->graph_names!=NULL) {
   /* We reallocate only if there is something */
   fst2->graph_names=(unichar**)realloc(fst2->graph_names,n*sizeof(unichar*));
}
fst2->number_of_states_by_graphs=(int*)realloc(fst2->number_of_states_by_graphs,n*sizeof(int));
}


/**
 * Allocates, initializes and returns a new tag
 */
Fst2Tag new_Fst2Tag() {
Fst2Tag e;
e=(Fst2Tag)malloc(sizeof(struct fst2Tag));
if (e==NULL) {
  fatal_error("Not enough memory in new_Fst2Tag\n");
}
e->number=0;
e->control=0;
e->input=NULL;
e->output=NULL;
e->inflected=NULL;
e->lemma=NULL;
e->codes=NULL;
e->matching_tokens=NULL;
e->number_of_matching_tokens=0;
e->compound_pattern=NO_COMPOUND_PATTERN;
/* $CD$ begin */
e->contentGF = NULL;
e->entryMasterGF = -1;
/* $CD$ end   */
return e;
}


/**
 * Analyzes the input of the tag of the form "<...>" . If some fields
 * need to be filled, the function fills them.
 * 
 * Examples: <be.V:P> => lemma="be", codes="V:P"
 *           <!MOT>   => negation=true
 */
void analyze_tag_with_angles(Fst2Tag e) {
unichar temp[2048];
int i,j;
j=0;
i=1;
if (e->input[i]=='!') {
  i++;
  /* We set the negation bit if there is the negation sign '!' */
  e->control=(char)(e->control|NEGATION_TAG_BIT_MASK);
}
/* Then, we look for a separator, copying what we find in 'temp' */
while ((e->input[i]!=',')&&(e->input[i]!='.')&&(e->input[i]!='>')) {
  temp[j++]=e->input[i++];
}
temp[j]='\0';
/*
 * If we find a closing angle, we have three cases: <build>, <V> or <MOT>
 */
if (e->input[i]=='>') {
	/* If we have a meta symbol, we do nothing */
	if (!u_strcmp_char(temp,"MOT")) {return;}
	if (!u_strcmp_char(temp,"DIC")) {return;}
	if (!u_strcmp_char(temp,"MAJ")) {return;}
	if (!u_strcmp_char(temp,"MIN")) {return;}
	if (!u_strcmp_char(temp,"PRE")) {return;}
	if (!u_strcmp_char(temp,"NB")) {return;}
	/* $CD$ begin */
	if (!u_strcmp_char(temp,"TOKEN")) {return;}
	/* $CD$ end   */
	/* 
	 * If we have a tag like <build> or <V>, we copy the content (build or V)
	 * into the inflected field of the tag. This is a pure convention that we use 
	 * because at this step, we cannot decide if we have a lemma or a grammatical
	 * code. This problem is supposed to be resolved by a later function.
	 */
	e->inflected=u_strdup(temp);
	return;
}
if (e->input[i]==',') {
	/*
	 * If we find a comma, we have a tag like <built,build.V> and we copy the
	 * sequence before the comma in the inflected field.
	 */
  	e->inflected=u_strdup(temp);
	i++;
	j=0;
	/* Then we look for the next separator */
	while ((e->input[i]!='.')&&(e->input[i]!='>')) {
    	temp[j++]=e->input[i++];
	}
	temp[j]='\0';
	/* A closing angle is an error since tags like <built,build> are not allowed */
	if (e->input[i]=='>') {
		/* We try to convert the invalid tag into ASCII to print it
		 * in the error message */
		char err[1024];
		u_to_char(err,e->input);
		fatal_error("Invalid label %s\n",err);
	}
	/* We copy the part between the comma and the point into the lemma field */
	e->lemma=u_strdup(temp);
	i++;
	j=0;
	while (e->input[i]!='>') {
		temp[j++]=e->input[i++];
	}
	temp[j]='\0';
	/* And we copy the remaining part of the tag (without the closing angle)
	 * into the codes field */
	e->codes=u_strdup(temp);
	return;
}
/*
 * If we find a point, we have a tag like <build.V>, so we copy the sequence
 * before the point into the lemma field.
 */
e->lemma=u_strdup(temp);
i++;
j=0;
while (e->input[i]!='>') {
	temp[j++]=e->input[i++];
}
temp[j]='\0';
/* And we copy the remaining part of the tag (without the closing angle)
 * into the codes field */
e->codes=u_strdup(temp);
}


/**
 * Analyzes the input of the tag of the form "{...}", different from the
 * sentence delimiter "{S}". At the opposite of the tags of the form "<...>",
 * a tag between round brackets must be of the form "{built,build.V}".
 */
void analyze_tag_with_round_brackets(Fst2Tag e) {
unichar temp[2048];
char err[2048];
int i,j;
j=0;
i=1;
/* We look for the comma */
while ((e->input[i]!=',')&&(e->input[i]!='}')) {
	temp[j++]=e->input[i++];
}
if (e->input[i]=='}') {
	/* If we find the closing bracket, it is an error */
	u_to_char(err,e->input);
	fatal_error("Invalid label %s: a tag must contain a valid DELAF line like {today,today.ADV}\n",err);
}
temp[j]='\0';
/* We copy the inflected form and we look for the point */
e->inflected=u_strdup(temp);
i++;
if (e->input[i]=='.') {
	/* If the lemma is an empty sequence, it means that the 
	 * lemma is identical to the inflected form, so we copy it*/
   e->lemma=u_strdup(e->inflected);
}
else {
	j=0;
	/* If the lemma is not empty, we copy it into temp */
	while ((e->input[i]!='.')&&(e->input[i]!='}')) {
		temp[j++]=e->input[i++];
	}
	temp[j]='\0';
	if (e->input[i]=='}') {
		/* If we find the closing bracket, it is an error */
		u_to_char(err,e->input);
		fatal_error("Invalid label %s: a tag must contain a valid DELAF line like {today,today.ADV}\n",err);
	}
	e->lemma=u_strdup(temp);
}
i++;
j=0;
/* Finally, we copy the remaining sequence (without the closing bracket) into
 * the lemma field */
while (e->input[i]!='}') {
	temp[j++]=e->input[i++];
}
temp[j]='\0';
e->codes=u_strdup(temp);
}


/**
 * This function creates and returns a tag for a variable declaration of the form
 * "$a(" or "$a)".
 * 
 * IMPORTANT: This function does not add the variable to the variable list of 
 *            the enclosing fst2.
 */
Fst2Tag create_variable_tag(unichar* input) {
int length=u_strlen(input);
Fst2Tag tag=new_Fst2Tag();
/*
 * We copy the variable name into the input field
 * length-1 = length - 2(ignoring '$' and '('or ')') + 1(for '\0') 
 */
tag->input=(unichar*)malloc((length-1)*sizeof(unichar));
for (int i=1;i<length;i++) {
	tag->input[i-1]=input[i];
}
tag->input[length-2]='\0';
/*
 * And we indicate if it is a variable start or end
 */
if (input[length-1]=='(') {tag->control=START_VAR_TAG_BIT_MASK;}
else {tag->control=END_VAR_TAG_BIT_MASK;}
return tag;
}


/**
 * This function creates and returns a fst2 tag. 'input', 'contentGF' and
 * 'output' and not supposed to be NULL. 'input' is not supposed to be an
 * empty string. After the execution, the field 'contentGF' of the created tag
 * can be NULL, but 'output' cannot. No output is indicated by an empty string.
 */
Fst2Tag create_tag(unichar* input,unichar* contentGF,unichar* output,int respect_case) {
int L=u_strlen(input);
if (input[0]=='$' && L>2 && (input[L-1]=='(' || input[L-1]==')')) {
   /* If we have a variable declaration like $a( or $a) */
   return create_variable_tag(input);
}
Fst2Tag tag=new_Fst2Tag();
tag->input=u_strdup(input);
/* First, we test if we have a context mark */
if (!u_strcmp_char(input,"$[")) {
   tag->control=POSITIVE_CONTEXT_MASK;
   return tag;
}
if (!u_strcmp_char(input,"$![")) {
   tag->control=NEGATIVE_CONTEXT_MASK;
   return tag;
}
if (!u_strcmp_char(input,"$]")) {
   tag->control=CONTEXT_END_MASK;
   return tag;
}
/*
 * If we have a morphological filter, we copy it
 */
/* $CD$ begin */
if (u_strlen(contentGF) > 0) {
    tag->contentGF=u_strdup(contentGF);
}
/* $CD$ end   */
/*
 * We copy the output, even if empty
 */
tag->output=u_strdup(output);
if (output[0]!=0) {
	/* But we set the output bit only if the output is not empty */
	tag->control=(unsigned char)(tag->control|TRANSDUCTION_TAG_BIT_MASK);
}
if (respect_case) {
	/* We set the case respect bit if necessary */
	tag->control=(unsigned char)(tag->control|RESPECT_CASE_TAG_BIT_MASK);
}
/*
 * If the input is ' ', # or <E>, we do nothing
 */
if ((input[0]==' ')||(input[0]=='#')||!u_strcmp_char(input,"<E>")) {
  //fst2->tags[position]=tag;
  return tag;
}
/*
 * We handle inputs that start with a round bracket
 */
if (input[0]=='{') {
  if (!u_strcmp_char(input,"{S}")) {
  	/*
  	 * If we have the sentence delimiter {S}, we do nothing.
  	 * IMPORTANT: the {STOP} tag MUST NOT be handled here, since
  	 *            we do not want it to be matched
  	 */
  }
  else {
    /* Here we a lexical tag like {built,build.V} or a single '{' character */
    if (input[1]!='\0') {
    	/* If we have not a single '{', we analyze the lexical tag */
       analyze_tag_with_round_brackets(tag);
    }
  }
  return tag;
}
if ((input[0]!='<') || (input[1]=='\0')) {
	/*
	 * If we have a single '<' character or an input that does not start with '<',
	 * there is nothing special to do.
	 */
	return tag;
}
/*
 * Finally, if we have an input that starts with '<' that is not a single '<',
 * we analyze the tag assuming that it is of the form <...>
 */
analyze_tag_with_angles(tag);
return tag;
}


/**
 * Creates and returns the tag for the string 'line', representing a tag line
 * in a .fst2 file like "%<V>/VERB", without an ending '\n'.
 */
Fst2Tag create_tag(unichar* line) {
unichar all_input[2048],output[2048];
int i=1,j=0,k=0;
/* First, we look if the tag must respect case */
int respect_case=(line[0]=='@');
/*
 * Then, we look for a '/' in order to see if there is an output. The first character
 * is necessary a part of the input since tags with no input like "%/OUTPUT" are not
 * allowed. We look for a '/' that is not preceeded by '\'. All the input is copied
 * into 'all_input'.
 */
all_input[k++]=line[i++];
while(line[i]!='\0' && !(line[i]=='/' && i>0 && line[i-1]!='\\')) {
	all_input[k++]=line[i++];
}
all_input[k]='\0';
/*
 * If we find an output, we copy it
 */
if(line[i]=='/') {
	i++;
	while(line[i]!='\0') {
		output[j++]=line[i++];
	}
}
/* Then, if there is an output, we have it, and if not, 'output'
 * contains an empty string */
output[j]='\0';
/*
 * Now, we will analyze the string 'all_input', in order to see if it
 * can be decomposed into an input and a morphological filter. A
 * morphological filter is of the form "<<.....>>_f_", but the "_..._" part
 * is optional.
 */
/* $CD$ begin */  
unichar input[2048],filter[2048];
input[0] = '\0';
filter[0] = '\0';
i = 0; j = 0;
/* We copy the real input into 'input' */
while (all_input[i]!='\0' && (all_input[i]!='<' || all_input[i+1]!='<')) {
	input[j++]=all_input[i++];
}
input[j]='\0';
/* If something remains, we have a morphological filter */
if (all_input[i]!='\0') {
	j = 0;
	/* We copy the "<<...>>"  sequence */
	while (all_input[i]!='\0' && (all_input[i]!='>' || all_input[i+1]!='>')) {
		filter[j++]=all_input[i++];
	}
    filter[j++]='>';filter[j++]='>';
    /* If something remains, then we have an optional part like "_f_",
     * and we add it to 'filter' */
    if (all_input[i]!='\0') {
		i=i+2;
		if (all_input[i]=='_') {
			do {
				filter[j++]=all_input[i++];
			} while (all_input[i]!='\0' && all_input[i]!= '_');
			filter[j++] = '_';
		}
	}
    filter[j] = '\0';
}
/* If there is a morphological filter but no input ("%<<^in>>/PFX"), then
 * we say that the input is any token */
if (input[0]=='\0') {u_strcpy_char(input,"<TOKEN>");}
return create_tag(input,filter,output,respect_case);
/* $CD$ end   */
}


/**
 * Reads the tags of the .fst2 file 'f'. The function assumes that the
 * current position in the file is just before the first tag line.
 * For each tag line, a tag is created and inserted in the tag array of 
 * the given fst2. The parameter 'limit' indicates that the function
 * may stop reading after the tag number 'limit'. NO_TAG_LIMIT indicates
 * that there is no limit. 
 */
void read_fst2_tags(FILE *f,Fst2* fst2,int limit) {
int i;
unichar c;
unichar line[10000];
int current_tag=0;
/* If the position in the file is not correct we exit */
if (((c=(unichar)u_fgetc(f))!='%')&&(c!='@')) {
	fatal_error("Unexpected character in .fst2 file: %c\n",c);
}
/* There cannot have no tag line, because by convention, every .fst2 file
 * must have "%<E>" as first tag. */
while (c!='f' && (limit==NO_TAG_LIMIT || current_tag<=limit)) {
	/* We read the line and copy it without the ending '\n' */
	i=0;
	do {
		line[i++]=c;
	} while ((c=(unichar)u_fgetc(f))!='\n');
	/* And we read the first character of the next line */
  	if (((c=(unichar)u_fgetc(f))!='f')&&(c!='%')&&(c!='@')) {
  		/* If the character is not an expected one we exit */
  		fatal_error("Unexpected character in .fst2 file: %c\n",c);
  	}
	line[i]='\0';
	/* We create the tag and add it to the fst2 */
	fst2->tags[current_tag]=create_tag(line);
	/* 
	 * IMPORTANT: if the tag is a variable declaration, we must add this variable
	 *            to the variable list of the fst2 .
	 */
	if (fst2->tags[current_tag]->control & (START_VAR_TAG_BIT_MASK+END_VAR_TAG_BIT_MASK)) {
		fst2->variables=add_variable_to_list(fst2->tags[current_tag]->input,fst2->variables);
	}
	/* We do not forget to increase the tag counter */
	current_tag++;
}
/* Finally, we set the number of tags of the fst2 */
fst2->number_of_tags=current_tag;
}


/**
 * Reads all the tags of the .fst2 file 'f'.
 */
void read_fst2_tags(FILE *f,Fst2* fst2) {
read_fst2_tags(f,fst2,NO_TAG_LIMIT);
}


//
// lit les etiquettes des transitions jusqu'a l'etiquette d'indice ETIQ_MAX
//
/*void lire_etiquettes_fst2_under_limit(FILE *f,int ETIQ_MAX,Fst2* fst2) {
int i;
unichar c;
unichar mot[10000];
int current_tag=0;
int k=0;
while (((c=(unichar)u_fgetc(f))!='%')&&(c!='@'));
while (c!='f' && k<=ETIQ_MAX) {
  i=0;
  k++;
  do {
    mot[i++]=c;
  } while ((c=(unichar)u_fgetc(f))!='\n');
  while (((c=(unichar)u_fgetc(f))!='f')&&(c!='%')&&(c!='@')) {}
  mot[i]='\0';
  add_tag(mot,fst2,&current_tag);
}
fst2->number_of_tags=current_tag;
}*/


//
// initialise le graphe_fst2
//
void initialiser_graphe_fst2() {
long int i;
for (i=0;i<MAX_FST2_STATES;i++)
  graphe_fst2[i]=NULL;
}

//
// initialise les etiquettes
//
void initialiser_etiquettes() {
long int i;
for (i=0;i<MAX_FST2_TAGS;i++)
  etiquette_fst2[i]=NULL;
}

//
// initialise les variables utilisees pour charger le FST2
//
void initialiser_variables_fst2(){
nombre_etats_fst2=0;
etat_courant=0;
initialiser_graphe_fst2();
initialiser_etiquettes();
}


//
// cree et renvoie un etat vierge
//
Fst2State nouvel_etat() {
Fst2State e;
e=(Fst2State)malloc(sizeof(struct fst2State));
if (e==NULL) {
  fprintf(stderr,"Probleme d'allocation memoire dans la fonction nouvel_etat\n");
  exit(1);
}
e->control=0;
e->transitions=NULL;
return e;
}

//
// lit un entier relatif dans le fichier; si une fin de ligne est trouvee, ok vaut 0
//
int lire_entier_fst2(FILE *f,int *ok) {
unichar c;
int res,negatif;
do
  c=(unichar)u_fgetc(f);
while (((c<'0')||(c>'9'))&&(c!='-')&&(c!='\n'));
if (c=='\n') {
  // si fin de ligne, on arrete et ok vaut 0
  *ok=0;
  return 0;
}
*ok=1;
if (c=='-') {
  // on lit un nombre negatif
  negatif=1;
  c=(unichar)u_fgetc(f);
} else negatif=0;
res=c-'0';
while(((c=(unichar)u_fgetc(f))>='0')&&(c<='9')) {
  res=res*10+(c-'0');
}
if (negatif) res=-res;
return res;
}


//
// cree et renvoie une nouvelle transition
//
Fst2Transition nouvelle_transition_mat() {
Fst2Transition t;
t=(Fst2Transition)malloc(sizeof(struct fst2Transition));
if (t==NULL) {
  fprintf(stderr,"Probleme d'allocation memoire dans la fonction nouvelle_transition_mat\n");
  exit(1);
}
t->tag_number=-1;
t->state_number=-1;
t->next=NULL;
return t;
}



//
// ajoute une transition a l'etat courant
//
void ajouter_transition_mat(struct fst2State *e,int etiq,int etarr)
{
  struct fst2Transition *ptr;

  ptr=nouvelle_transition_mat();
  ptr->next=e->transitions;
  ptr->tag_number=etiq;
  ptr->state_number=etarr;
  e->transitions=ptr;
}


//
// lit les etats puis les ajoute a l'automate
//
void lire_etats_fst2(FILE *f) {
unichar c;
int i,j,ok;
int imot,etarr,graphe_courant;
int etat_relatif;
debut_graphe_fst2=(int*)malloc((nombre_graphes_fst2+1)*sizeof(int));
nombre_etats_par_grf=(int*)malloc((nombre_graphes_fst2+1)*sizeof(int));
if (debut_graphe_fst2==NULL) {
  fprintf(stderr,"Probleme d'allocation memoire dans la fonction lire_etats_fst2\n");
  exit(1);
}
for (i=0;i<nombre_graphes_fst2+1;i++) {
  // j'ai ajoute +1 car les graphes sont numerotes de -1 a -nombre_graphes
  debut_graphe_fst2[i]=0;
  nombre_etats_par_grf[i]=0;
}
etat_courant=0;
// on lit tous les graphes
for (j=0;j<nombre_graphes_fst2;j++) {
  // on lit le -
  u_fgetc(f);
  // et le numero
  graphe_courant=u_read_int(f);
  debut_graphe_fst2[graphe_courant]=etat_courant;
  etat_relatif=0;
  // on saute le nom du graphe
  while ((c=(unichar)u_fgetc(f))!='\n');
  // on se place sur "t" ou ":" ("f" serait une erreur)
  do
    c=(unichar)u_fgetc(f);
  while ((c!='t')&&(c!=':'));
  // on lit les etats du graphe
  while (c!='f') {
    graphe_fst2[etat_courant]=nouvel_etat();
    nombre_etats_par_grf[graphe_courant]++;
    if(c=='t') graphe_fst2[etat_courant]->control=FST2_FINAL_STATE_BIT_MASK;
    if (etat_relatif==0) graphe_fst2[etat_courant]->control=(unsigned char)((graphe_fst2[etat_courant]->control)|2);
    imot=lire_entier_fst2(f,&ok);
    while (ok) {
	  etarr=lire_entier_fst2(f,&ok);
      // etarr est un numero relatif; on calcule sa position reelle dans
      // le tableau des etats
      etarr=etarr+debut_graphe_fst2[graphe_courant];
	  ajouter_transition_mat(graphe_fst2[etat_courant],imot,etarr);
      imot=lire_entier_fst2(f,&ok);
	}
    while(((c=(unichar)u_fgetc(f))!=':')&&(c!='t')&&(c!='f'));
    etat_courant++;
    etat_relatif++;
  }
  // on lit l'espace et le \n apres le f
  u_fgetc(f);
  u_fgetc(f);

}
nombre_etats_fst2=etat_courant;
}


//
// lit les etats puis les ajoute a l'automate
// RECUPERE EN PLUS LE NOM DE CHAQUE GRAPHE
//
void lire_etats_fst2_avec_noms(FILE *f) {
unichar c;
int i,j,ok;
int imot,etarr,graphe_courant;
int etat_relatif;
unichar temp[10000];
int tmp;
debut_graphe_fst2=(int*)malloc((nombre_graphes_fst2+1)*sizeof(int));
nombre_etats_par_grf=(int*)malloc((nombre_graphes_fst2+1)*sizeof(int));
nom_graphe=(unichar**)malloc((nombre_graphes_fst2+1)*sizeof(unichar*));
if (debut_graphe_fst2==NULL || (nom_graphe)==NULL) {
  fprintf(stderr,"Probleme d'allocation memoire dans la fonction lire_etats_fst2_avec_noms\n");
  exit(1);
}
for (i=0;i<nombre_graphes_fst2+1;i++) {
  // j'ai ajoute +1 car les graphes sont numerotes de -1 a -nombre_graphes
  debut_graphe_fst2[i]=0;
  nombre_etats_par_grf[i]=0;
  nom_graphe[i]=NULL;
}
etat_courant=0;
// on lit tous les graphes
for (j=0;j<nombre_graphes_fst2;j++) {
  // on lit le -
  u_fgetc(f);
  // et le numero
  graphe_courant=u_read_int(f);
  debut_graphe_fst2[graphe_courant]=etat_courant;
  etat_relatif=0;
  // on lit le nom du graphe
  tmp=0;
  while ((c=(unichar)u_fgetc(f))!='\n')
    temp[tmp++]=c;
  temp[tmp]='\0';
  nom_graphe[graphe_courant]=(unichar*)malloc(sizeof(unichar)*(tmp+1));
  u_strcpy(nom_graphe[graphe_courant],temp);

  // on se place sur "t" ou ":" ("f" serait une erreur)
  do
    c=(unichar)u_fgetc(f);
  while ((c!='t')&&(c!=':')&&(c!='f'));
  // on lit les etats du graphe
  while (c!='f') {
    graphe_fst2[etat_courant]=nouvel_etat();
    nombre_etats_par_grf[graphe_courant]++;
    if(c=='t') graphe_fst2[etat_courant]->control=1;
    if (etat_relatif==0) graphe_fst2[etat_courant]->control=(unsigned char)((graphe_fst2[etat_courant]->control)|FST2_INITIAL_STATE_BIT_MASK);
    imot=lire_entier_fst2(f,&ok);
    while (ok) {
      etarr=lire_entier_fst2(f,&ok);
      // etarr est un numero relatif; on calcule sa position reelle dans
      // le tableau des etats
      etarr=etarr+debut_graphe_fst2[graphe_courant];
	  ajouter_transition_mat(graphe_fst2[etat_courant],imot,etarr);
      imot=lire_entier_fst2(f,&ok);
	}
    while(((c=(unichar)u_fgetc(f))!=':')&&(c!='t')&&(c!='f'));
    etat_courant++;
    etat_relatif++;
  }
  // on lit l'espace et le \n apres le f
  u_fgetc(f);
  u_fgetc(f);
}
nombre_etats_fst2=etat_courant;
}



//
// lit les etats puis les ajoute a l'automate
// RECUPERE EN PLUS LE NOM DE CHAQUE GRAPHE
//
void lire_etats_fst2_avec_noms_for_one_sentence(FILE *f,int SENTENCE,int* ETIQ_MAX,FILE* txt) {
unichar c;
int i,j,ok;
int imot,etarr,graphe_courant;
int etat_relatif;
unichar temp[100000];
int tmp;
(*ETIQ_MAX)=0;
debut_graphe_fst2=(int*)malloc((nombre_graphes_fst2+1)*sizeof(int));
nombre_etats_par_grf=(int*)malloc((nombre_graphes_fst2+1)*sizeof(int));
nom_graphe=(unichar**)malloc((nombre_graphes_fst2+1)*sizeof(unichar*));
if (debut_graphe_fst2==NULL || (nom_graphe)==NULL) {
  fprintf(stderr,"Probleme d'allocation memoire dans la fonction lire_etats_fst2_avec_noms\n");
  exit(1);
}
for (i=0;i<nombre_graphes_fst2+1;i++) {
  // j'ai ajoute +1 car les graphes sont numerotes de -1 a -nombre_graphes
  debut_graphe_fst2[i]=0;
  nombre_etats_par_grf[i]=0;
  nom_graphe[i]=NULL;
}
etat_courant=0;
// on lit tous les graphes
for (j=0;j<nombre_graphes_fst2;j++) {
  // on lit le -
  u_fgetc(f);
  // et le numero
  graphe_courant=u_read_int(f);
  debut_graphe_fst2[graphe_courant]=etat_courant;
  etat_relatif=0;
  // on lit le nom du graphe
  tmp=0;
  while ((c=(unichar)u_fgetc(f))!='\n')
    temp[tmp++]=c;
  temp[tmp]='\0';
  if ((j+1==SENTENCE)) {
     u_fprints(temp,txt);
     u_fprints_char("\n",txt);
     // on ne construit le graphe que si on est sur le bon indice
     // on se place sur "t" ou ":" ("f" serait une erreur)
     do
       c=(unichar)u_fgetc(f);
     while ((c!='t')&&(c!=':')&&(c!='f'));
     // on lit les etats du graphe
     while (c!='f') {
       graphe_fst2[etat_courant]=nouvel_etat();
       nombre_etats_par_grf[graphe_courant]++;
       if(c=='t') graphe_fst2[etat_courant]->control=FST2_FINAL_STATE_BIT_MASK;
       if (etat_relatif==0) graphe_fst2[etat_courant]->control=(unsigned char)((graphe_fst2[etat_courant]->control)|FST2_INITIAL_STATE_BIT_MASK);
       imot=lire_entier_fst2(f,&ok);
       while (ok) {
         if (imot>(*ETIQ_MAX)) (*ETIQ_MAX)=imot;
         etarr=lire_entier_fst2(f,&ok);
         // etarr est un numero relatif; on calcule sa position reelle dans
         // le tableau des etats
         etarr=etarr+debut_graphe_fst2[graphe_courant];
	     ajouter_transition_mat(graphe_fst2[etat_courant],imot,etarr);
         imot=lire_entier_fst2(f,&ok);
       }
       while(((c=(unichar)u_fgetc(f))!=':')&&(c!='t')&&(c!='f'));
       etat_courant++;
       etat_relatif++;
     }
  }
  else {
     while(((c=(unichar)u_fgetc(f))!='f'));
  }
  // on lit l'espace et le \n apres le f
  u_fgetc(f);
  u_fgetc(f);
}
nombre_etats_fst2=etat_courant;
}





//
// loads an fst2 and returns its representation in a Fst2 structure
//
Fst2* load_fst2(char *file,int noms) {
FILE *f;
Fst2* fst2=new_Fst2();
f=u_fopen(file,U_READ);
if (f==NULL) {
  fprintf(stderr,"Cannot open the file %s\n",file);
  return NULL;
}
nombre_graphes_fst2=u_read_int(f);
if (nombre_graphes_fst2==0) {
   fprintf(stderr,"Graph %s is empty\n",file);
   return NULL;
}
fst2->states=(Fst2State*)malloc(MAX_FST2_STATES*sizeof(Fst2State));
fst2->tags=(Fst2Tag*)malloc(MAX_FST2_TAGS*sizeof(Fst2Tag));
graphe_fst2=fst2->states;
etiquette_fst2=fst2->tags;
debut_graphe_fst2=fst2->initial_states;
liste_des_variables=fst2->variables;
initialiser_variables_fst2();
nombre_etats_par_grf=fst2->number_of_states_by_graphs;
if (noms) {
   nom_graphe=fst2->graph_names;
   lire_etats_fst2_avec_noms(f);
   fst2->graph_names=nom_graphe;
}
else {
   lire_etats_fst2(f);
}
fst2->number_of_states_by_graphs=nombre_etats_par_grf;
read_fst2_tags(f,fst2);
u_fclose(f);
fst2->number_of_graphs=nombre_graphes_fst2;
fst2->number_of_states=nombre_etats_fst2;
fst2->initial_states=debut_graphe_fst2;
resize(fst2);
return fst2;
}



//
// loads one sentence of an fst2 and returns its representation in an Automate_fst2 structure
//
Fst2* load_one_sentence_of_fst2(char *file,int SENTENCE,FILE* txt) {
FILE *f;
int ETIQ_MAX;
Fst2* fst2=new_Fst2();
f=u_fopen(file,U_READ);
if (f==NULL) {
  fprintf(stderr,"Cannot open the file %s\n",file);
  return NULL;
}
nombre_graphes_fst2=u_read_int(f);
if (nombre_graphes_fst2==0) {
   fprintf(stderr,"Graph %s is empty\n",file);
   return NULL;
}
fst2->states=(Fst2State*)malloc(MAX_FST2_STATES*sizeof(Fst2State));
fst2->tags=(Fst2Tag*)malloc(MAX_FST2_TAGS*sizeof(Fst2Tag));
graphe_fst2=fst2->states;
etiquette_fst2=fst2->tags;
debut_graphe_fst2=fst2->initial_states;
liste_des_variables=fst2->variables;
initialiser_variables_fst2();
nombre_etats_par_grf=fst2->number_of_states_by_graphs;
nom_graphe=fst2->graph_names;
lire_etats_fst2_avec_noms_for_one_sentence(f,SENTENCE,&ETIQ_MAX,txt);
fst2->number_of_states_by_graphs=nombre_etats_par_grf;
read_fst2_tags(f,fst2,ETIQ_MAX);
u_fclose(f);
fst2->graph_names=nom_graphe;
fst2->number_of_graphs=nombre_graphes_fst2;
fst2->number_of_states=nombre_etats_fst2;
fst2->initial_states=debut_graphe_fst2;
resize(fst2);
return fst2;
}


/**
 * This function returns 0 if the given state is not final and a non-zero value
 * if the state is final.
 */
int is_final_state(Fst2State e) {
if (e==NULL) {
   fatal_error("NULL error in is_final_state\n");
}
return e->control&FST2_FINAL_STATE_BIT_MASK;
}


/**
 * This function sets the finality of the given state.
 */
void set_final_state(Fst2State e,int finality) {
if (e==NULL) {
   fatal_error("NULL error in set_final_state\n");
}
/* First the compute the control byte without the finality bit */
e->control=e->control & (0xFF-FST2_FINAL_STATE_BIT_MASK);
/* And we add it if necessary*/
if (finality) {
	e->control=e->control | FST2_FINAL_STATE_BIT_MASK;
}
}


/**
 * This function returns 0 if the given state is not initial and a non-zero value
 * if the state is final.
 */
int is_initial_state(Fst2State e) {
if (e==NULL) {
   fatal_error("NULL error in is_initial_state\n");
}
return e->control&FST2_INITIAL_STATE_BIT_MASK;
}


/**
 * This function sets the initiality of the given state.
 */
void set_initial_state(Fst2State e,int finality) {
if (e==NULL) {
   fatal_error("NULL error in set_initial_state\n");
}
/* First the compute the control byte without the initiality bit */
e->control=e->control & (0xFF-FST2_INITIAL_STATE_BIT_MASK);
/* And we add it if necessary*/
if (finality) {
	e->control=e->control | FST2_INITIAL_STATE_BIT_MASK;
}
}



void unprotect_characters_in_sequence(unichar* s) {
int new_cursor=0;
int old_cursor=0;
while (s[old_cursor]!='\0') {
   if (s[old_cursor]=='\\') {old_cursor++;}
   if (s[old_cursor]=='\0') {
      fprintf(stderr,"ERROR: unprotected slash at the end of a sequence in unprotect_characters_in_sequence\n");
   }
   s[new_cursor++]=s[old_cursor++];
}
s[new_cursor]='\0';
}


//
// This function unprotects characters in non NULL flechi and canonique members
// of any tag of the given fst2
//
void unprotect_characters_in_fst2_tags(Fst2* fst2) {
Fst2Tag etiq;
for (int i=0;i<fst2->number_of_tags;i++) {
   etiq=fst2->tags[i];
   if (etiq!=NULL) {
      if (etiq->inflected!=NULL) {unprotect_characters_in_sequence(etiq->inflected);}
      if (etiq->lemma!=NULL) {unprotect_characters_in_sequence(etiq->lemma);}
   }
}
}
