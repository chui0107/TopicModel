#include "TopicModel.h"
#include "lda-data.h"

//extern corpus* read_data(char* data_filename);

void run_em(const char* start, const char* directory, corpus* corpus)
{
    /*
    int d, n;
    lda_model* model = NULL;
    double **var_gamma, **phi;

    // allocate variational parameters

    var_gamma = malloc(sizeof(double*) * (corpus->num_docs));
    for (d = 0; d < corpus->num_docs; d++)
        var_gamma[d] = malloc(sizeof(double) * NTOPICS);

    int max_length = max_corpus_length(corpus);
    phi = malloc(sizeof(double*) * max_length);
    for (n = 0; n < max_length; n++)
        phi[n] = malloc(sizeof(double) * NTOPICS);

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
 */
}

int main()
{
    TopicModelSettings settings(0.5, 30, 20, 1e-6, 100, 1e-4, "random",
        "estimate", "example/ap/ap.dat", "output");

    char foo[] = "output";
    corpus* newsCorpus = read_data((char*)settings.mOutputPath.c_str());

    //run_em(settings.mTopicInit.c_str(), settings.mOutputPath.c_str(), newsCorpus);
}