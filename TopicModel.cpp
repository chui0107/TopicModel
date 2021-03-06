#include "TopicModel.h"
#include <fstream>
#include <queue>

//private methods

void TopicModel::SetTopicModelEstimateSettings(const TopicModelEstimate& settings)
{
    this->mDataPath = settings.mDataPath;

    this->mTopicInit = settings.mTopicInit;

    this->mOutputPath = settings.mOutputPath;

    this->mAlpha = settings.mAlpha;

    this->VAR_MAX_ITER = settings.mVarMaxIter;

    this->VAR_CONVERGED = settings.mVarConvergence;

    this->EM_CONVERGED = settings.mEmConvergence;

    this->EM_MAX_ITER = settings.mEmMaxIter;

    this->INITIAL_ALPHA = settings.mInitialAlpha;

    this->NTOPICS = settings.mNTopics;

    this->ESTIMATE_ALPHA = (this->mAlpha == "fixed") ? 0 : 1;

    this->LAG = settings.LAG;
}

void TopicModel::SetTopicModelInferenceSettings(const TopicModelInference& settings)
{

    this->mAlpha = settings.mAlpha;

    this->VAR_MAX_ITER = settings.mVarMaxIter;

    this->VAR_CONVERGED = settings.mVarConvergence;

    this->EM_CONVERGED = settings.mEmConvergence;

    this->EM_MAX_ITER = settings.mEmMaxIter;

    this->ESTIMATE_ALPHA = (this->mAlpha == "fixed") ? 0 : 1;
}

double TopicModel::doc_e_step(document* doc, double* gamma, double** phi,
    lda_model* model, lda_suffstats* ss)
{
    double likelihood;
    int n, k;

    // posterior inference

    likelihood = lda_inference(doc, model, gamma, phi);

    // update sufficient statistics

    double gamma_sum = 0;
    for (k = 0; k < model->num_topics; k++) {
        gamma_sum += gamma[k];
        ss->alpha_suffstats += digamma(gamma[k]);
    }
    ss->alpha_suffstats -= model->num_topics * digamma(gamma_sum);

    for (n = 0; n < doc->length; n++) {
        for (k = 0; k < model->num_topics; k++) {
            ss->class_word[k][doc->words[n]] += doc->counts[n] * phi[n][k];
            ss->class_total[k] += doc->counts[n] * phi[n][k];
        }
    }

    ss->num_docs = ss->num_docs + 1;

    return (likelihood);
}

/*
 * writes the word assignments line for a document to a file
 *
 */

void TopicModel::write_word_assignment(FILE* f, document* doc, double** phi, lda_model* model)
{
    int n;

    fprintf(f, "%03d", doc->length);
    for (n = 0; n < doc->length; n++) {
        fprintf(f, " %04d:%02d",
            doc->words[n], argmax(phi[n], model->num_topics));
    }
    fprintf(f, "\n");
    fflush(f);
}

/*
 * saves the gamma parameters of the current dataset
 *
 */

void TopicModel::save_gamma(char* filename, double** gamma, int num_docs, int num_topics)
{
    FILE* fileptr;
    int d, k;
    fileptr = fopen(filename, "w");

    for (d = 0; d < num_docs; d++) {
        fprintf(fileptr, "%5.10f", gamma[d][0]);
        for (k = 1; k < num_topics; k++) {
            fprintf(fileptr, " %5.10f", gamma[d][k]);
        }
        fprintf(fileptr, "\n");
    }
    fclose(fileptr);
}

/*
 * variational inference
 *
 */

double TopicModel::lda_inference(document* doc, lda_model* model, double* var_gamma, double** phi)
{
    double converged = 1;
    double phisum = 0, likelihood = 0;
    double likelihood_old = 0, oldphi[model->num_topics];
    int k, n, var_iter;
    double digamma_gam[model->num_topics];

    // compute posterior dirichlet

    for (k = 0; k < model->num_topics; k++) {
        var_gamma[k] = model->alpha + (doc->total / ((double)model->num_topics));
        digamma_gam[k] = digamma(var_gamma[k]);
        for (n = 0; n < doc->length; n++)
            phi[n][k] = 1.0 / model->num_topics;
    }
    var_iter = 0;

    while ((converged > VAR_CONVERGED) && ((var_iter < VAR_MAX_ITER) || (VAR_MAX_ITER == -1))) {
        var_iter++;
        for (n = 0; n < doc->length; n++) {
            phisum = 0;
            for (k = 0; k < model->num_topics; k++) {
                oldphi[k] = phi[n][k];
                phi[n][k] = digamma_gam[k] + model->log_prob_w[k][doc->words[n]];

                if (k > 0)
                    phisum = log_sum(phisum, phi[n][k]);
                else
                    phisum = phi[n][k]; // note, phi is in log space
            }

            for (k = 0; k < model->num_topics; k++) {
                phi[n][k] = exp(phi[n][k] - phisum);
                var_gamma[k] = var_gamma[k] + doc->counts[n] * (phi[n][k] - oldphi[k]);
                // !!! a lot of extra digamma's here because of how we're computing it
                // !!! but its more automatically updated too.
                digamma_gam[k] = digamma(var_gamma[k]);
            }
        }

        likelihood = compute_likelihood(doc, model, phi, var_gamma);
        assert(!isnan(likelihood));
        converged = (likelihood_old - likelihood) / likelihood_old;
        likelihood_old = likelihood;

        // printf("[LDA INF] %8.5f %1.3e\n", likelihood, converged);
    }
    return (likelihood);
}

/*
 * compute likelihood bound
 *
 */

double TopicModel::compute_likelihood(document* doc, lda_model* model, double** phi, double* var_gamma)
{
    double likelihood = 0, digsum = 0, var_gamma_sum = 0, dig[model->num_topics];
    int k, n;

    for (k = 0; k < model->num_topics; k++) {
        dig[k] = digamma(var_gamma[k]);
        var_gamma_sum += var_gamma[k];
    }
    digsum = digamma(var_gamma_sum);

    likelihood = lgamma(model->alpha * model->num_topics)
        - model->num_topics * lgamma(model->alpha)
        - (lgamma(var_gamma_sum));

    for (k = 0; k < model->num_topics; k++) {
        likelihood += (model->alpha - 1) * (dig[k] - digsum) + lgamma(var_gamma[k])
            - (var_gamma[k] - 1) * (dig[k] - digsum);

        for (n = 0; n < doc->length; n++) {
            if (phi[n][k] > 0) {
                likelihood += doc->counts[n] * (phi[n][k] * ((dig[k] - digsum) - log(phi[n][k])
                                                                + model->log_prob_w[k][doc->words[n]]));
            }
        }
    }
    return (likelihood);
}

std::vector<std::string> TopicModel::GetTopN(const std::string& topic, std::vector<std::string>* vocabDict, int n)
{
    using namespace std;
    vector<string> topN;
    priority_queue<pair<float, int> > q;
    //split on space, NOTE the first character is a while space according to the original code's final.beta
    int l = 0, r = 1, p = 0;
    //tokenize the words and extract them by their probability
    while (r < topic.size()) {
        while (r < topic.size() && topic[r] != ' ') {
            r++;
        }

        string probaString = topic.substr(l, r - l);
        float proba = stof(probaString);
        pair<float, int> pair = make_pair<float, int>(proba, p++);

        q.push(pair);

        r++;
        l = r;
    }

    for (int i = 0; i < n && q.empty() == false; i++) {
        int index = q.top().second;
        topN.push_back((*vocabDict)[index]);
        q.pop();
    }

    return topN;
}

//public methods

void TopicModel::run_em(corpus* corpus, const TopicModelEstimate& settings)
{
    make_directory((char*)this->mOutputPath.c_str());
    char* start = (char*)this->mTopicInit.c_str();
    char* directory = (char*)this->mOutputPath.c_str();

    this->SetTopicModelEstimateSettings(settings);

    int d, n;
    lda_model* model = NULL;
    double **var_gamma, **phi;

    // allocate variational parameters

    var_gamma = (double**)malloc(sizeof(double*) * (corpus->num_docs));
    for (d = 0; d < corpus->num_docs; d++)
        var_gamma[d] = (double*)malloc(sizeof(double) * NTOPICS);

    int max_length = max_corpus_length(corpus);
    phi = (double**)malloc(sizeof(double*) * max_length);
    for (n = 0; n < max_length; n++)
        phi[n] = (double*)malloc(sizeof(double) * NTOPICS);

    // initialize model

    char filename[100];

    lda_suffstats* ss = NULL;
    if (strcmp(start, "seeded") == 0) {
        model = new_lda_model(corpus->num_terms, NTOPICS);
        ss = new_lda_suffstats(model);
        corpus_initialize_ss(ss, model, corpus);
        lda_mle(model, ss, 0);
        model->alpha = INITIAL_ALPHA;
    }
    else if (strcmp(start, "random") == 0) {
        model = new_lda_model(corpus->num_terms, NTOPICS);
        ss = new_lda_suffstats(model);
        random_initialize_ss(ss, model);
        lda_mle(model, ss, 0);
        model->alpha = INITIAL_ALPHA;
    }
    else if (strncmp(start, "manual=", 7) == 0) {
        model = new_lda_model(corpus->num_terms, NTOPICS);
        ss = new_lda_suffstats(model);
        manual_initialize_ss(start + 7, ss, model, corpus);
        lda_mle(model, ss, 0);
        model->alpha = INITIAL_ALPHA;
    }
    else {
        model = load_lda_model(start);
        ss = new_lda_suffstats(model);
    }

    sprintf(filename, "%s/000", directory);
    save_lda_model(model, filename);

    // run expectation maximization

    int i = 0;
    double likelihood, likelihood_old = 0, converged = 1;
    sprintf(filename, "%s/likelihood.dat", directory);
    FILE* likelihood_file = fopen(filename, "w");

    while (((converged < 0) || (converged > EM_CONVERGED) || (i <= 2)) && (i <= EM_MAX_ITER)) {
        i++;
        printf("**** em iteration %d ****\n", i);
        likelihood = 0;
        zero_initialize_ss(ss, model);

        // e-step

        for (d = 0; d < corpus->num_docs; d++) {
            if ((d % 1000) == 0)
                printf("document %d\n", d);
            likelihood += doc_e_step(&(corpus->docs[d]),
                var_gamma[d],
                phi,
                model,
                ss);
        }

        // m-step

        lda_mle(model, ss, ESTIMATE_ALPHA);

        // check for convergence

        converged = (likelihood_old - likelihood) / (likelihood_old);
        if (converged < 0)
            VAR_MAX_ITER = VAR_MAX_ITER * 2;
        likelihood_old = likelihood;

        // output model and likelihood

        fprintf(likelihood_file, "%10.10f\t%5.5e\n", likelihood, converged);
        fflush(likelihood_file);
        if ((i % LAG) == 0) {
            sprintf(filename, "%s/%03d", directory, i);
            save_lda_model(model, filename);
            sprintf(filename, "%s/%03d.gamma", directory, i);
            save_gamma(filename, var_gamma, corpus->num_docs, model->num_topics);
        }
    }

    // output the final model

    sprintf(filename, "%s/final", directory);
    save_lda_model(model, filename);
    sprintf(filename, "%s/final.gamma", directory);
    save_gamma(filename, var_gamma, corpus->num_docs, model->num_topics);

    // output the word assignments (for visualization)

    sprintf(filename, "%s/word-assignments.dat", directory);
    FILE* w_asgn_file = fopen(filename, "w");
    for (d = 0; d < corpus->num_docs; d++) {
        if ((d % 100) == 0)
            printf("final e step document %d\n", d);
        likelihood += lda_inference(&(corpus->docs[d]), model, var_gamma[d], phi);
        write_word_assignment(w_asgn_file, &(corpus->docs[d]), phi, model);
    }
    fclose(w_asgn_file);
    fclose(likelihood_file);
}

void TopicModel::infer(corpus* corpus, const TopicModelInference& settings)
{
    this->SetTopicModelInferenceSettings(settings);

    char* model_root = (char*)settings.mModelPath.c_str();
    char* save = (char*)settings.mOutputPath.c_str();

    FILE* fileptr;
    char filename[100];
    int i, d, n;
    lda_model* model;
    double **var_gamma, likelihood, **phi;
    document* doc;

    model = load_lda_model(model_root);
    var_gamma = (double**)malloc(sizeof(double*) * (corpus->num_docs));
    for (i = 0; i < corpus->num_docs; i++)
        var_gamma[i] = (double*)malloc(sizeof(double) * model->num_topics);
    sprintf(filename, "%s-lda-lhood.dat", save);
    fileptr = fopen(filename, "w");
    for (d = 0; d < corpus->num_docs; d++) {
        if (((d % 100) == 0) && (d > 0))
            printf("document %d\n", d);

        doc = &(corpus->docs[d]);
        phi = (double**)malloc(sizeof(double*) * doc->length);
        for (n = 0; n < doc->length; n++)
            phi[n] = (double*)malloc(sizeof(double) * model->num_topics);
        likelihood = lda_inference(doc, model, var_gamma[d], phi);

        fprintf(fileptr, "%5.5f\n", likelihood);
    }
    fclose(fileptr);
    sprintf(filename, "%s-gamma.dat", save);
    save_gamma(filename, var_gamma, corpus->num_docs, model->num_topics);
}

std::vector<std::vector<std::string> > TopicModel::GetTopNTerms(TopicModelTopTerms& settings, int n)
{
    using namespace std;

    string line;
    ifstream vocabFile(settings.mDataPath);
    ifstream betaFile(settings.mModelPath);
    vector<vector<string> > ret;

    if (vocabFile.is_open() == false || betaFile.is_open() == false) {
        vocabFile.close();
        betaFile.close();
        return ret;
    }

    vector<string>* vocabDict = new vector<string>();
    while (getline(vocabFile, line)) {
        vocabDict->push_back(line);
    }

    //get the top n term for each topic
    while (getline(betaFile, line)) {
        vector<string> topN = GetTopN(line, vocabDict, n);
        ret.push_back(topN);
    }

    vocabFile.close();
    betaFile.close();
    delete vocabDict;
    return ret;
}

//constructor

TopicModelSettings::TopicModelSettings(int varMaxIter,
    float varConvergence,
    int emMaxIter,
    float emConvergence,
    std::string alpha,
    std::string dataPath)
    : mVarMaxIter(varMaxIter)
    , mVarConvergence(varConvergence)
    , mEmMaxIter(emMaxIter)
    , mEmConvergence(emConvergence)
    , mAlpha(alpha)
    , mDataPath(dataPath)
{
}

TopicModelEstimate::TopicModelEstimate(float initialAlpha, int topics, int varMaxIter,
    float varConvergence, int emMaxIter, float emConvergence, std::string alpha,
    std::string dataPath, std::string topicInit, std::string outputPath)
    : TopicModelSettings(varMaxIter, varConvergence, emMaxIter, emConvergence, alpha, dataPath)
    , mInitialAlpha(initialAlpha)
    , mNTopics(topics)
    , mTopicInit(topicInit)
    , mOutputPath(outputPath)
    , LAG(5)
{
}

TopicModelInference::TopicModelInference(int varMaxIter,
    float varConvergence, int emMaxIter, float emConvergence, std::string alpha,
    std::string dataPath, std::string modelPath, std::string outputPath)
    : TopicModelSettings(varMaxIter, varConvergence, emMaxIter, emConvergence, alpha, dataPath)
    , mModelPath(modelPath)
    , mOutputPath(outputPath)
{
}

TopicModelTopTerms::TopicModelTopTerms(std::string modelPath, std::string dataPath)
    : mModelPath(modelPath)
    , mDataPath(dataPath)
{
}

int main()
{

    float initialAlpha = 0.5;
    int topics = 30;
    int varMaxIter = 20;
    float varConvergence = 1e-6;
    int emMaxIter = 100;
    float emConvergence = 1e-4;
    std::string alpha = "estimate";
    std::string dataPath = "example/ap/ap.dat";
    std::string topicInit = "random";
    std::string outputPath = "output";

    TopicModelEstimate estimateSettings(initialAlpha, topics, varMaxIter, varConvergence,
        emMaxIter, emConvergence, alpha, dataPath, topicInit, outputPath);

    TopicModel topicModel;

    corpus* newsCorpus = read_data((char*)estimateSettings.mDataPath.c_str());

    varMaxIter = -1;
    outputPath = "inferred";
    std::string modelPath = "output/final";

    TopicModelInference inferenceSettings(varMaxIter, varConvergence, emMaxIter,
        emConvergence, alpha, dataPath, modelPath, outputPath);

    //topic estimate
    //topicModel.run_em(newsCorpus, estimateSettings);

    //topic inference
    //topicModel.infer(newsCorpus, inferenceSettings);

    //top n terms for each topic
    TopicModelTopTerms topicModelTopTermSettings("output/final.beta", "example/ap/vocab.txt");
    int n = 10;
    std::vector<std::vector<std::string> > ret = topicModel.GetTopNTerms(topicModelTopTermSettings, n);
    for (int i = 0; i < ret.size(); i++) {
        std::cout << "topic " << i << ": ";
        for (int j = 0; j < ret[i].size(); j++) {
            std::cout << ret[i][j] << " ";
        }
        std::cout << std::endl;
    }
}