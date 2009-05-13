 /*
  * Unitex
  *
  * Copyright (C) 2001-2009 Universit� Paris-Est Marne-la-Vall�e <unitex@univ-mlv.fr>
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

#ifndef KoreanH
#define KoreanH

#include "Unicode.h"
#include "Alphabet.h"
#include "jamoCodage.h"

#define MAX_LETTERS_IN_A_SYLLAB 5
#define KR_SYLLAB_BOUND 0x318D

int syllabToLetters_HCJ(unichar,unichar*);
void convert_Korean_text(unichar* src,unichar* dest,jamoCodage* jamo,Alphabet* alphabet);

#endif
