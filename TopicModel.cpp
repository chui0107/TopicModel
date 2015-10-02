#include "TopicModel.h"
#include "LDA.h"
#include <cstdio>
#include <cstdlib>

#define OFFSET 0; // offset for reading data

corpus* read_data(char* data_filename)
{
    FILE* fileptr;
    int length, count, word, n, nd, nw;
    corpus* c;

    printf("reading data from %s\n", data_filename);
    c = (corpus*)malloc(sizeof(corpus));
    c->docs = 0;
    c->num_terms = 0;
    c->num_docs = 0;
    fileptr = fopen(data_filename, "r");
    nd = 0;
    nw = 0;
    while ((fscanf(fileptr, "%10d", &length) != EOF)) {
        c->docs = (document*)realloc(c->docs, sizeof(document) * (nd + 1));
        c->docs[nd].length = length;
        c->docs[nd].total = 0;
        c->docs[nd].words = (int*)malloc(sizeof(int) * length);
        c->docs[nd].counts = (int*)malloc(sizeof(int) * length);
        for (n = 0; n < length; n++) {
            fscanf(fileptr, "%10d:%10d", &word, &count);
            word = word - OFFSET;
            c->docs[nd].words[n] = word;
            c->docs[nd].counts[n] = count;
            c->docs[nd].total += count;
            if (word >= nw) {
                nw = word + 1;
            }
        }
        nd++;
    }
    fclose(fileptr);
    c->num_docs = nd;
    c->num_terms = nw;
    printf("number of docs    : %d\n", nd);
    printf("number of terms   : %d\n", nw);
    return (c);
}

int main()
{
    char dataPath[] = "";
    corpus* newsCorpus = read_data(dataPath);

    TopicModelSettings settings(0.5, 30, 20, 1e-6, 100, 1e-4, "estimate");
}