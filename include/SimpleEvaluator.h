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
typedef std::vector<std::pair<uint32_t, bool>> query_path;

class SimpleEvaluator : public Evaluator {

    std::shared_ptr<SimpleGraph> graph;
    std::shared_ptr<SimpleEstimator> est;

    std::unordered_map<std::string, std::shared_ptr<intermediate>> evalCache;
    std::unordered_map<std::string, cardStat> statCache;

    void unpackQueryTree(query_path *path, RPQTree *q);
    std::string pathToString(query_path *path);

public:
    explicit SimpleEvaluator(std::shared_ptr<SimpleGraph> &g);

    ~SimpleEvaluator() = default;

    void prepare() override;

    cardStat evaluate(RPQTree *query) override;

    void attachEstimator(std::shared_ptr<SimpleEstimator> &e);

    std::shared_ptr<intermediate> evaluate_aux(RPQTree *q);

    static std::shared_ptr<intermediate> project(uint32_t label, bool inverse, std::shared_ptr<SimpleGraph> &g);

    static std::shared_ptr<intermediate> join(std::shared_ptr<intermediate> &left, std::shared_ptr<intermediate> &right);

    cardStat computeStats(std::shared_ptr<intermediate> &result);

    RPQTree *optimizeQuery(query_path *path);
};


#endif //QS_SIMPLEEVALUATOR_H
