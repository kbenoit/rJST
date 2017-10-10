#include "_jstreversed.h"

// [[Rcpp::depends(RcppArmadillo)]]

// [[Rcpp::export]]
Rcpp::List jstcppreversed(arma::sp_imat& dfm,
        Rcpp::List& sentiLexList,
        int numSentiLabs,
        int numTopics,
        int numiters,
        int updateParaStep,
        double alpha_,
        double beta_,
        double gamma_) {

    modeljstrev * jst = new modeljstrev();

    jst->numTopics = numTopics;
    jst->numSentiLabs = numSentiLabs;
    jst->numiters = numiters;
    jst->alpha_ = alpha_;
    jst->beta_ = beta_;
    jst->gamma_ = gamma_;
    jst->updateParaStep = updateParaStep;
    jst->dfm = &dfm;

    jst->init(sentiLexList);
    jst->estimate();

    return Rcpp::List::create(Rcpp::Named("pi") = jst->returnPi(),
                             Rcpp::Named("theta") = jst->returnTheta(),
                             Rcpp::Named("phi") = jst->returnPhi(),
                             Rcpp::Named("phi.termScores") = jst->termScores());
}

void modeljstrev::init(Rcpp::List& sentiLexList) {
  std::vector<double> sentiLexEntry, priorProb;
  int wordToken;

  numDocs = dfm->n_rows;
  vocabSize = dfm->n_cols;

  docSizes.resize(numDocs);
  for (int d = 0; d < numDocs; d++) {
    docSizes[d] = (int)accu(dfm->row(d));
  }
  aveDocSize = (double)std::accumulate(docSizes.begin(),docSizes.end(),0)/(double)numDocs;

  for (Rcpp::List::iterator it = sentiLexList.begin(); it < sentiLexList.end(); it++) {
    sentiLexEntry = Rcpp::as<std::vector<double> >(*it);
    wordToken = (int) sentiLexEntry[0];
    std::copy(sentiLexEntry.begin()+1,sentiLexEntry.end(),priorProb.begin());
    sentiLex.insert(std::pair<int, std::vector<double> >(wordToken, priorProb));
    priorProb.clear();
  }

  init_parameters();

  init_estimate();
}

void modeljstrev::init_parameters() {
  topic_dw.resize(numDocs);
  sent_dw.resize(numDocs);
  for (int d = 0; d < numDocs; d++) {
    topic_dw[d].resize(docSizes[d]);
    sent_dw[d].resize(docSizes[d]);
  }

  //model counts
  nd.resize(numDocs);
  ndz.resize(numDocs);
  ndlz.resize(numDocs);
  for (int d = 0; d < numDocs; d++) {
    nd[d] = 0;
    ndz[d].resize(numTopics);
    for (int z = 0; z < numTopics; z++) {
      ndz[d][z] = 0;
    }

    ndlz[d].resize(numSentiLabs);
    for (int l = 0; l < numSentiLabs; l++) {
      ndlz[d][l].resize(numTopics);
      for (int z = 0; z < numTopics; z++) {
        ndlz[d][l][z] = 0;
      }
    }
  }

  nlzw.resize(numSentiLabs);
  nlz.resize(numSentiLabs);
  for (int l = 0; l < numSentiLabs; l++) {
    nlz[l].resize(numTopics);
    nlzw[l].resize(numTopics);
    for (int z = 0; z < numTopics; z++) {
      nlz[l][z] = 0;
      nlzw[l][z].resize(vocabSize);
      for (int w = 0; w < vocabSize; w++) {
        nlzw[l][z][w] = 0;
      }
    }
  }

  //Posterior values
  p_lz.resize(numSentiLabs);
  for (int l = 0; l < numSentiLabs; l++) {
    p_lz[l].resize(numTopics);
  }

  //Hyperparameters
  set_alpha();
  set_beta(); //Asymmetric based on sentiment Lexicon
  set_gamma();

  //Result vectors
  pi_zld.resize(numTopics);
  theta_zd.resize(numTopics);
  for (int z = 0; z < numTopics; z++) {
    theta_zd[z].resize(numDocs);
    pi_zld[z].resize(numSentiLabs);
    for (int l = 0; l < numSentiLabs; l++) {
      pi_zld[z][l].resize(numDocs);
    }
  }

	phi_zlw.resize(numTopics); // size: (L x T x V)
  for (int z = 0; z < numTopics; z++) {
    phi_zlw[z].resize(numSentiLabs);
    for (int l = 0; l < numSentiLabs; l++) {
      phi_zlw[z][l].resize(vocabSize);
    }
  }

}

void modeljstrev::init_estimate() {
  srand(time(NULL));

  int document, wordToken, priorSent, topic, sentilab;
  std::map<int,std::vector<double> >::iterator sentiIt;

  std::vector<int> locations(numDocs);
  std::fill(locations.begin(),locations.end(),0);

  for (arma::sp_imat::iterator it = dfm->begin(); it != dfm->end(); it++) {
    wordToken = it.col();
    document = it.row();

    priorSent = -1;
    sentiIt = sentiLex.find(wordToken);

    if (sentiIt != sentiLex.end()) { //Word token is found in the Sentiment Lexicon!
      sentilab = std::distance(sentiIt->second.begin(),
                                std::max_element(sentiIt->second.begin(),sentiIt->second.end()));
    }

    for (int i = 0; i < (int)(*it); i++) {

      if (priorSent == -1) {
        sentilab = rand() % numSentiLabs;
      }
      topic = rand() % numTopics;

      topic_dw[document][locations[document]] = topic;
      sent_dw[document][locations[document]] = sentilab;

      locations[document]++;

      nd[document]++;
      ndz[document][topic]++;
      ndlz[document][sentilab][topic]++;
      nlzw[sentilab][topic][wordToken]++;
      nlz[sentilab][topic]++;
    }
  }
}

void modeljstrev::estimate() {
  int document, wordToken, topic, sentilab;
  std::vector<int> locations(numDocs);

  for (int iter = 1; iter <= numiters; iter++) {
    if (iter % 100 == 0) {
      Rcpp::Rcout << "Iteration " << iter << "!\n";
    }

    std::fill(locations.begin(),locations.end(),0); //reset the locations

    for (arma::sp_imat::iterator it = dfm->begin(); it != dfm->end(); it++) {
      wordToken = it.col();
      document = it.row();

      for (int i = 0; i < (int)(*it); i++) {
        topic = topic_dw[document][locations[document]];
        sentilab = sent_dw[document][locations[document]];

        //Remove word from counts
        nd[document]--;
        ndz[document][topic]--;
        ndlz[document][sentilab][topic]--;
        nlzw[sentilab][topic][wordToken]--;
        nlz[sentilab][topic]--;

        //sample from bivariate distribution. topic and sentilab are changed by the method
        drawsample(document,wordToken,topic,sentilab);

        topic_dw[document][locations[document]] = topic;
        sent_dw[document][locations[document]] = sentilab;

        //Add word back to counts with new labels
        nd[document]++;
        ndz[document][topic]++;
        ndlz[document][sentilab][topic]++;
        nlzw[sentilab][topic][wordToken]++;
        nlz[sentilab][topic]++;

        //update position within the document
        locations[document]++;
      }
    }

    if (updateParaStep > 0 && iter % updateParaStep == 0) {
			this->update_Parameters();
		}
  }

  //Compute parameter values
  compute_pi_zld();
  compute_theta_zd();
  compute_phi_zlw();
}

void modeljstrev::drawsample(int d, int w, int& topic, int& sentilab) {
  double u;

  // do multinomial sampling via cumulative method
	for (int l = 0; l < numSentiLabs; l++) {
		for (int z = 0; z < numTopics; z++) {
			p_lz[l][z] = (nlzw[l][z][w] + beta_lzw[l][z][w]) / (nlz[l][z] + betaSum_lz[l][z])
                    * (ndlz[d][l][z] + gamma_zl[z][l]) / (ndz[d][z] + gammaSum_z[z])
                    * (ndz[d][z] + alpha_dz[d][z]) / (nd[d] + alphaSum_d[d]);
		}
  }

	// accumulate multinomial parameters
	for (int l = 0; l < numSentiLabs; l++)  {
		for (int z = 0; z < numTopics; z++) {
			if (z==0)  {
			    if (l==0) continue;
		        else p_lz[l][z] += p_lz[l-1][numTopics-1]; // accumulate the sum of the previous array
			}
			else p_lz[l][z] += p_lz[l][z-1];
		}
	}

	// probability normalization
	u = ((double)rand() / RAND_MAX) * p_lz[numSentiLabs-1][numTopics-1];

	// sample sentiment label l, where l \in [0, S-1]
	bool loopBreak=false;
	for (sentilab = 0; sentilab < numSentiLabs; sentilab++) {
		for (topic = 0; topic < numTopics; topic++) {
		    if (p_lz[sentilab][topic] > u) {
		        loopBreak = true;
		        break;
		    }
		}
		if (loopBreak == true) {
			break;
		}
	}

	if (sentilab == numSentiLabs) sentilab--; // to avoid over array boundary
	if (topic == numTopics) topic--;
}

void modeljstrev::set_gamma() {

	if (gamma_ <= 0 ) {
		gamma_ = (double)aveDocSize * 0.05 / (double)(numSentiLabs * numTopics);
	}

	gamma_zl.resize(numTopics);
	gammaSum_z.resize(numTopics);

	for (int z = 0; z < numTopics; z++) {
		gamma_zl[z].resize(numSentiLabs);
    gammaSum_z[z] = 0;
    for (int l = 0; l < numSentiLabs; l++) {
      gamma_zl[z][l] = gamma_;
      gammaSum_z[z] += gamma_;
    }
	}
}

void modeljstrev::set_alpha() {

  //Set alpha to default
  if (alpha_ <= 0) {
		alpha_ =  (double)aveDocSize * 0.05 / (double)numTopics;
	}

  alpha_dz.resize(numDocs);
  alphaSum_d.resize(numDocs);
	for (int d = 0; d < numDocs; d++) {
		alpha_dz[d].resize(numTopics);
    alphaSum_d[d] = 0;
    for (int z = 0; z < numTopics; z++) {
      alpha_dz[d][z] = alpha_;
      alphaSum_d[d] += alpha_;
    }
	}
}

void modeljstrev::set_beta() {
  std::map<int,std::vector<double> >::iterator sentiIt;
  std::vector<std::vector<double> > lambda_lw;

  if (beta_ <= 0) {
    beta_ = 0.01;
  }

	beta_lzw.resize(numSentiLabs);
	betaSum_lz.resize(numSentiLabs);
  lambda_lw.resize(numSentiLabs);
	for (int l = 0; l < numSentiLabs; l++) {
		beta_lzw[l].resize(numTopics);

    lambda_lw[l].resize(vocabSize);
    for (int w = 0; w < vocabSize; w++) {
      lambda_lw[l][w] = 1;
    }

    betaSum_lz[l].resize(numTopics);

		for (int z = 0; z < numTopics; z++) {
      betaSum_lz[l][z] = 0;

			beta_lzw[l][z].resize(vocabSize);
			for (int w = 0; w < vocabSize; w++) {
        beta_lzw[l][z][w] = beta_;
      }
		}
	}

  for (sentiIt = sentiLex.begin(); sentiIt != sentiLex.end(); sentiIt++) {
    //For each entry of the sentiment lexicon:
		for (int j = 0; j < numSentiLabs; j++) {
			lambda_lw[j][sentiIt->first] = sentiIt->second[j];
		}
	}

  for (int l = 0; l < numSentiLabs; l++) {
		for (int z = 0; z < numTopics; z++) {
		    for (int w = 0; w < vocabSize; w++) {
			    beta_lzw[l][z][w] *= lambda_lw[l][w];
			    betaSum_lz[l][z] += beta_lzw[l][z][w];
		    }
		}
	}
}

void modeljstrev::compute_phi_zlw() {
	for (int l = 0; l < numSentiLabs; l++)  {
	  for (int z = 0; z < numTopics; z++) {
			for(int w = 0; w < vocabSize; w++) {
				phi_zlw[z][l][w] = (nlzw[l][z][w] + beta_lzw[l][z][w]) / (nlz[l][z] + betaSum_lz[l][z]);
			}
		}
	}
}

void modeljstrev::compute_pi_zld() {
	for (int d = 0; d < numDocs; d++) {
	  for (int l = 0; l < numSentiLabs; l++) {
      for (int z = 0; z < numTopics; z++) {
        pi_zld[z][l][d] = (ndlz[d][l][z] + gamma_zl[z][l]) / (ndz[d][z] + gammaSum_z[z]);
      }
		}
	}
}

void modeljstrev::compute_theta_zd() {
	for (int d = 0; d < numDocs; d++) {
	  for (int z = 0; z < numTopics; z++)  {
			 theta_zd[z][d] = (ndz[d][z] + alpha_dz[d][z]) / (nd[d] + alphaSum_d[d]);
		}
	}
}

std::vector<std::vector<std::vector<double> > > modeljstrev::termScores() {

  std::vector<std::vector<std::vector<double> > > termScores;
  double product;

  std::vector<std::vector<double> > prod_wz;
  prod_wz.resize(vocabSize);

  for (int w = 0; w < vocabSize; w++ ) {
    prod_wz[w].resize(numTopics);
    std::fill(prod_wz[w].begin(),prod_wz[w].end(),1.0);
  }

  for (int w = 0; w < vocabSize; w++) {
    for (int z = 0; z < numTopics; z++) {
      product = 1.0;
      for (int l = 0; l < numSentiLabs; l++) {
        product *= phi_zlw[z][l][w];
      }
      prod_wz[w][z] = pow(product,(1.0/(double)(numTopics)));
    }
  }

  for (int w = 0; w < vocabSize; w++) {
    for (int z = 0; z < numTopics; z++) {

    }
  }

  termScores.resize(numTopics);
  for (int z = 0; z < numTopics; z++) {
    termScores[z].resize(numSentiLabs);
    for (int l = 0; l < numSentiLabs; l++) {
      termScores[z][l].resize(vocabSize);
      for (int w = 0; w < vocabSize; w++) {
        termScores[z][l][w] = phi_zlw[z][l][w] * log(phi_zlw[z][l][w]/prod_wz[w][z]);
      }
    }
  }

  return termScores;
}

void modeljstrev::update_Parameters() {

	int ** data; // temp valuable for exporting 3-dimentional array to 2-dimentional
	double * gamma_temp;
	data = new int*[numSentiLabs];
	for (int l = 0; l < numSentiLabs; l++) {
		data[l] = new int[numDocs];
		for (int d = 0; d < numDocs; d++) {
			data[l][d] = 0;
		}
	}

	gamma_temp = new double[numSentiLabs];
	for (int l = 0; l < numSentiLabs; l++){
		gamma_temp[l] = 0.0;
	}

	// update gamma
	for (int z = 0; z < numTopics; z++) {
		for (int l = 0; l < numSentiLabs; l++) {
			for (int d = 0; d < numDocs; d++) {
				data[l][d] = ndlz[d][l][z]; // ntldsum[j][k][m];
			}
		}

		for (int l = 0; l < numSentiLabs; l++) {
			gamma_temp[l] =  gamma_zl[z][l];
		}

		polya_fit_simple(data, gamma_temp, numSentiLabs, numDocs);

		// update alpha
		gammaSum_z[z] = 0.0;
		for (int l = 0; l < numSentiLabs; l++) {
			gamma_zl[z][l] = gamma_temp[l];
			gammaSum_z[z] += gamma_zl[z][l];
		}
	}
}
