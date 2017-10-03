/**********************************************************************
		        Joint Sentiment-Topic (JST) Model
***********************************************************************

(C) Copyright 2013, Chenghua Lin and Yulan He

Written by: Chenghua Lin, University of Aberdeen, chenghua.lin@abdn.ac.uk.
Part of code is from http://gibbslda.sourceforge.net/.

This file is part of JST implementation.

JST is free software; you can redistribute it and/or modify it under
the terms of the GNU General Public License as published by the Free
Software Foundation; either version 2 of the License, or (at your
option) any later version.

JST is distributed in the hope that it will be useful, but WITHOUT
ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
USA

***********************************************************************/


#ifndef	_DOCUMENT_H
#define	_DOCUMENT_H

#include <vector>
#include <iostream>
using namespace std;



class document {

public:
	int * words;
	int * priorSentiLabels;
	int length;

	document() {
		words = NULL;
		priorSentiLabels = NULL;
		length = 0;
	}

	 document(vector<int> doc) {
		length = doc.size();
		words = new int[length];
		for (int i = 0; i < length; i++) {
			words[i] = doc[i];
		}
		priorSentiLabels = new int[length];
  }

    ~document() {
		if (words != NULL){
			delete [] words;
			words = NULL;
		}

		if (priorSentiLabels != NULL){
			delete [] priorSentiLabels;
			priorSentiLabels = NULL;
		}
    }
};

#endif
