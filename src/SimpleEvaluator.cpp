//
// Created by Nikolay Yakovets on 2018-02-02.
//

#include "SimpleEstimator.h"
#include "SimpleEvaluator.h"

SimpleEvaluator::SimpleEvaluator(std::shared_ptr<SimpleGraph> &g) {

    // works only with SimpleGraph
    graph = g;
    est = nullptr; // estimator not attached by default
}

void SimpleEvaluator::attachEstimator(std::shared_ptr<SimpleEstimator> &e) {
    est = e;
}

void SimpleEvaluator::prepare() {

    // if attached, prepare the estimator
    if(est != nullptr) est->prepare();

    // prepare other things here.., if necessary

}

cardStat SimpleEvaluator::computeStats(std::shared_ptr<intermediate> &g) {

    cardStat stats {};

//    for(int source = 0; source < g->getNoVertices(); source++) {
//        if(!g->adj[source].empty()) stats.noOut++;
//    }
    stats.noOut = g->size();

    stats.noPaths = 420; // TODO: Fix.

//    for(int target = 0; target < g->getNoVertices(); target++) {
//        //if(!g->reverse_adj[target].empty()) stats.noIn++;
//    }
    stats.noIn = 1337; // TODO: Fix.

    return stats;
}

std::shared_ptr<intermediate> SimpleEvaluator::project(uint32_t projectLabel, bool inverse, std::shared_ptr<SimpleGraph> &in) {

    auto out = std::make_shared<intermediate>();

    if (!inverse) {
        for (const auto &sourceDestPair : in->adj[projectLabel]) {
            (*out)[sourceDestPair.first].emplace_back(sourceDestPair.second);
        }
    } else {
        for (const auto &sourceDestPair : in->adj[projectLabel]) {
            (*out)[sourceDestPair.second].emplace_back(sourceDestPair.first);
        }
    }

    /*
    if(!inverse) {
        // going forward
        for(uint32_t source = 0; source < in->getNoVertices(); source++) {
            for (auto labelTarget : in->adj[source]) {

                auto label = labelTarget.first;
                auto target = labelTarget.second;

                if (label == projectLabel)
                    out->addEdge(source, target, label);
            }
        }
    } else {
        // going backward
        for(uint32_t source = 0; source < in->getNoVertices(); source++) {
            for (auto labelTarget : in->reverse_adj[source]) {

                auto label = labelTarget.first;
                auto target = labelTarget.second;

                if (label == projectLabel)
                    out->addEdge(source, target, label);
            }
        }
    }
*/
    return out;
}

std::shared_ptr<intermediate> SimpleEvaluator::join(std::shared_ptr<intermediate> &left, std::shared_ptr<intermediate> &right) {

    auto out = std::make_shared<intermediate>();

    for (const auto &leftSourceDestListPair : *left) { // (source) => (dest vector)
        for (const auto &leftDest : leftSourceDestListPair.second) {
            for (const auto &rightDest : (*right)[leftDest]) {
                (*out)[leftSourceDestListPair.first].emplace_back(rightDest);
            }
        }
    }

//    // (leftSource -> leftTarget) & (rightSource, rightTarget) ==> (leftSource, rightTarget) (if leftTarget == rightSource)
//    for (const auto &leftLabelPair : left->adj) { // O(1)
//        for (const auto &rightLabelPair : right->adj) { // O(1)
//            for (const auto &leftSourceDestPair : leftLabelPair.second) { // O(EL)
//                for (const auto &rightSourceDestPair : rightLabelPair.second) { // O(EL * ER)
//                    if (leftSourceDestPair.second == rightSourceDestPair.first) {
//                        out->addEdge(leftSourceDestPair.first, rightSourceDestPair.second, 0);
//                    }
//                }
//            }
//        }
//    }

    /*
    for(uint32_t leftSource = 0; leftSource < left->getNoVertices(); leftSource++) { // O(L)
        for (auto labelTarget : left->adj[leftSource]) {                             // O(E)

            int leftTarget = labelTarget.second;
            // try to join the left target with right source
            for (auto rightLabelTarget : right->adj[leftTarget]) {                   // O(EL + ER)

                auto rightTarget = rightLabelTarget.second;
                out->addEdge(leftSource, rightTarget, 0);

            }
        }
    }*/
    return out;
}

std::shared_ptr<intermediate> SimpleEvaluator::evaluate_aux(RPQTree *q) {

    // evaluate according to the AST bottom-up

    if(q->isLeaf()) {
        // project out the label in the AST
        std::regex directLabel (R"((\d+)\+)");
        std::regex inverseLabel (R"((\d+)\-)");

        std::smatch matches;

        uint32_t label;
        bool inverse;

        if(std::regex_search(q->data, matches, directLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            inverse = false;
        } else if(std::regex_search(q->data, matches, inverseLabel)) {
            label = (uint32_t) std::stoul(matches[1]);
            inverse = true;
        } else {
            std::cerr << "Label parsing failed!" << std::endl;
            return nullptr;
        }

        return SimpleEvaluator::project(label, inverse, graph);
    }

    if(q->isConcat()) {

        // evaluate the children
        auto leftResult = SimpleEvaluator::evaluate_aux(q->left);
        auto rightResult = SimpleEvaluator::evaluate_aux(q->right);

        // join left with right
        return SimpleEvaluator::join(leftResult, rightResult);

    }

    return nullptr;
}

cardStat SimpleEvaluator::evaluate(RPQTree *query) {
    // TODO: optimize query tree

    est->estimate(query);

    auto res = evaluate_aux(query);
    return SimpleEvaluator::computeStats(res);
}