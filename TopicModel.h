#ifndef TOPIC_MODEL_H
#define TOPIC_MODEL_H

#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include <string.h>
#include <time.h>

#include "lda.h"
#include "lda-data.h"
#include "lda-model.h"
#include "lda-alpha.h"
#include "utils.h"
#include "cokus.h"
#include <string>
#include <assert.h>

struct TopicModelSettings {

    /*
	 
	 command line parameters
	 lda est 1 30 ./settings.txt ./ap/ap.dat random ./output
	 
	*/

    float mInitialAlpha;
    int mNTopics;
    std::string mDataPath;
    std::string mTopicInit;
    std::string mOutputPath;

    /*
	 
	 settings parameters
	 
	 var max iter 20
	 var convergence 1e-6
	 em max iter 100
	 em convergence 1e-4
	 alpha estimate
	 
	*/

    int mVarMaxIter;
    float mConvergence;
    int mEmMaxIter;
    float mEmConvergence;
    std::string mAlpha;

    TopicModelSettings(float initialAlpha, int topics, int varMaxIter,
        float convergence, int emmaxIter, float emconvergence, std::string alpha,
        std::string dataPath, std::string topicInit, std::string outputPath);
};

class TopicModel {

private:
    const int LAG = 5;

    float mInitialAlpha;
    int NTOPICS;

    float VAR_CONVERGED;
    int VAR_MAX_ITER;

    float EM_CONVERGED;
    int EM_MAX_ITER;
    int ESTIMATE_ALPHA;
    double INITIAL_ALPHA;

    std::string mTopicInit;
    std::string mAlpha;
    std::string mDataPath;
    std::string mOutputPath;

    void write_word_assignment(FILE* f,
        document* doc,
        double** phi,
        lda_model* model);

    double doc_e_step(document* doc,
        double* gamma,
        double** phi,
        lda_model* model,
        lda_suffstats* ss);

    void save_gamma(char* filename,
        double** gamma,
        int num_docs,
        int num_topics);

    double lda_inference(document*,
        lda_model*,
        double*, double**);

    double compute_likelihood(document*,
        lda_model*,
        double**,
        double*);

public:
    TopicModel(TopicModelSettings& settings);

    void run_em(corpus* corpus);
};

#endif
