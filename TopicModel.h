#ifndef TOPIC_MODEL_H
#define TOPIC_MODEL_H

#include <string>

struct TopicModelSettings {
    float mInitialAlpha;
    int mNTopics;
    int mVarMaxIter;
    float mConvergence;
    int mEmMaxIter;
    float mEmConvergence;
    std::string mAlpha;

    TopicModelSettings()
        : mInitialAlpha()
        , mNTopics()
        , mVarMaxIter()
        , mConvergence()
        , mEmMaxIter()
        , mEmConvergence()
        , mAlpha("")
    {
    }

    TopicModelSettings(float initialAlpha, int topics, int varMaxIter,
        float convergence, int emmaxIter, float emconvergence,
        std::string alpha)
        : mInitialAlpha(initialAlpha)
        , mNTopics(topics)
        , mVarMaxIter(varMaxIter)
        , mConvergence(convergence)
        , mEmMaxIter(emmaxIter)
        , mEmConvergence(emconvergence)
        , mAlpha(alpha)
    {
    }
};

class TopicModel {
};

#endif
