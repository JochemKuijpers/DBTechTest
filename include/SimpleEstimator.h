//
// Created by Nikolay Yakovets on 2018-02-01.
//

#ifndef QS_SIMPLEESTIMATOR_H
#define QS_SIMPLEESTIMATOR_H

#include "Estimator.h"
#include "SimpleGraph.h"

#include <unordered_map>
#include <unordered_set>

class PairHasher {
public:
    template <typename T, typename U>
    std::size_t operator()(const std::pair<T, U> &x) const {
        return std::hash<T>()(x.first) ^ std::hash<U>()(x.second);
    }
};

class SimpleEstimator : public Estimator {

    std::shared_ptr<SimpleGraph> graph;

    std::map<uint32_t, std::map<uint32_t, std::vector<uint32_t>>> vertexIndexByLabel;

public:
    explicit SimpleEstimator(std::shared_ptr<SimpleGraph> &g);

    ~SimpleEstimator() = default;

    void prepare() override;

    cardStat estimate(RPQTree *q) override;
};

#endif //QS_SIMPLEESTIMATOR_H
