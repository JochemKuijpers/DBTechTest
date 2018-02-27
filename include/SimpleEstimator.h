//
// Created by Nikolay Yakovets on 2018-02-01.
//

#ifndef QS_SIMPLEESTIMATOR_H
#define QS_SIMPLEESTIMATOR_H

#include "Estimator.h"
#include "SimpleGraph.h"

class PairHasher {
public:
    template <typename T, typename U>
    std::size_t operator()(const std::pair<T, U> &x) const {
        return std::hash<T>()(x.first) ^ std::hash<U>()(x.second);
    }
};

struct internalCardStat {
    uint32_t noOut;
    uint32_t noPaths;
    uint32_t noIn;
    uint32_t firstLabel;
    uint32_t lastLabel;
    bool firstDirection;
    bool lastDirection;
};

class SimpleEstimator : public Estimator {

    std::shared_ptr<SimpleGraph> graph;

    // mapping from edgelabel to set of end vertices
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> startVerticesPerEdgeLabel;

    // mapping from edgelabel to set of end vertices
    std::unordered_map<uint32_t, std::unordered_set<uint32_t>> endVerticesPerEdgeLabel;

    // mapping from edgelabel to set of vertex pairs
    std::unordered_map<uint32_t, std::unordered_set<std::pair<uint32_t, uint32_t>, PairHasher>> vertexPairsPerEdgeLabel;

public:
    explicit SimpleEstimator(std::shared_ptr<SimpleGraph> &g);
    ~SimpleEstimator() = default;

    void prepare() override ;
    cardStat estimate(RPQTree *q) override ;

    internalCardStat internalEstimate(RPQTree *q);
};


#endif //QS_SIMPLEESTIMATOR_H
