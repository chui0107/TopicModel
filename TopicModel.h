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
    std::string mTopicInit;
    std::string mAlpha;
    std::string mDataPath;
    std::string mOutputPath;

    TopicModelSettings()
        : mInitialAlpha()
        , mNTopics()
        , mVarMaxIter()
        , mConvergence()
        , mEmMaxIter()
        , mEmConvergence()
        , mTopicInit("")
        , mAlpha("")
        , mDataPath("")
        , mOutputPath("")
    {
    }

    TopicModelSettings(float initialAlpha, int topics, int varMaxIter,
        float convergence, int emmaxIter, float emconvergence, std::string topicInit,
        std::string alpha, std::string dataPath, std::string outputPath)
        : mInitialAlpha(initialAlpha)
        , mNTopics(topics)
        , mVarMaxIter(varMaxIter)
        , mConvergence(convergence)
        , mEmMaxIter(emmaxIter)
        , mEmConvergence(emconvergence)
        , mTopicInit(topicInit)
        , mAlpha(alpha)
        , mDataPath(dataPath)
        , mOutputPath(outputPath)

    {
    }
};

class TopicModel {
};

#endif
