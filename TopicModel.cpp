#include "TopicModel.h"

//constructor

TopicModelSettings::TopicModelSettings(float initialAlpha, int topics, int varMaxIter,
    float convergence, int emmaxIter, float emconvergence, std::string alpha,
    std::string dataPath, std::string topicInit, std::string outputPath)
    : mInitialAlpha(initialAlpha)
    , mNTopics(topics)
    , mVarMaxIter(varMaxIter)
    , mConvergence(convergence)
    , mEmMaxIter(emmaxIter)
    , mEmConvergence(emconvergence)
    , mAlpha(alpha)
    , mDataPath(dataPath)
    , mTopicInit(topicInit)
    , mOutputPath(outputPath)
{
}

//private methods

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

//public methods

void TopicModel::run_em(corpus* corpus)
{
    char* start = (char*)this->mAlpha.c_str();
    char* directory = (char*)this->mOutputPath.c_str();

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

//constructor

TopicModel::TopicModel(TopicModelSettings& settings)
{
    //lda est 1 30 ./settings.txt ./ap/ap.dat random ./output
    this->mDataPath = settings.mDataPath;
    this->mTopicInit = settings.mTopicInit;
    this->mOutputPath = settings.mOutputPath;
    this->mAlpha = settings.mAlpha;
    this->mInitialAlpha = settings.mInitialAlpha;

    this->VAR_MAX_ITER = settings.mVarMaxIter;
    this->VAR_CONVERGED = settings.mConvergence;

    this->EM_CONVERGED = settings.mEmConvergence;
    this->EM_MAX_ITER = settings.mEmMaxIter;
    this->INITIAL_ALPHA = settings.mInitialAlpha;
    this->NTOPICS = settings.mNTopics;
    this->ESTIMATE_ALPHA = (this->mTopicInit == "fixed") ? 0 : 1;
}

int main()
{
    TopicModelSettings settings(0.5, 30, 20, 1e-6, 100, 1e-4, "random",
        "example/ap/ap.dat", "estimate", "output");
    TopicModel topicModel(settings);

    corpus* newsCorpus = read_data((char*)settings.mDataPath.c_str());

    topicModel.run_em(newsCorpus);
}