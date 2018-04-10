//
// Created by Nikolay Yakovets on 2018-02-02.
//

#ifndef QS_SIMPLEEVALUATOR_H
#define QS_SIMPLEEVALUATOR_H


#include <memory>
#include <cmath>
#include "SimpleGraph.h"
#include "RPQTree.h"
#include "Evaluator.h"
#include "Graph.h"

typedef std::unordered_map<uint32_t, std::vector<uint32_t>> intermediate;

class SimpleEvaluator : public Evaluator {

    std::shared_ptr<SimpleGraph> graph;
    std::shared_ptr<SimpleEstimator> est;

    void unpackQueryTree(std::vector<std::pair<uint32_t, bool>> *path, RPQTree *q);

public:
    explicit SimpleEvaluator(std::shared_ptr<SimpleGraph> &g);

    ~SimpleEvaluator() = default;
    void prepare() override ;

    cardStat evaluate(RPQTree *query) override ;

    void attachEstimator(std::shared_ptr<SimpleEstimator> &e);
    std::shared_ptr<intermediate> evaluate_aux(RPQTree *q);
    static std::shared_ptr<intermediate> project(uint32_t label, bool inverse, std::shared_ptr<SimpleGraph> &g);

    static std::shared_ptr<intermediate> join(std::shared_ptr<intermediate> &left, std::shared_ptr<intermediate> &right);

    cardStat computeStats(std::shared_ptr<intermediate> &result);

    RPQTree *optimizeQuery(std::vector<std::pair<uint32_t, bool>> *path);
};


#endif //QS_SIMPLEEVALUATOR_H
