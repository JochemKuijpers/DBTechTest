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

    cardStat stats {0, 0, 0};

    stats.noOut = static_cast<uint32_t>(g->size());

    std::unordered_set<uint32_t> uniqueDests;

    uint32_t length = graph->getNoVertices() / sizeof(uint8_t) + 1;
    auto *destBitset = static_cast<uint8_t *>(malloc(length));
    for (int i = 0; i < length; ++i) { destBitset[i] = 0; }

    for (auto sourceDestListPair : *g) {
        std::sort(sourceDestListPair.second.begin(), sourceDestListPair.second.end());
        bool first = true;
        uint32_t prevDest = 0;
        for (const auto &dest: sourceDestListPair.second) {

            // dest = 12
            // dest / 8 = 1
            // dest % 8 = 4

            // 0123 4567   0123 4
            // 0           1    v      2
            // 0000 0000   0000 1000   0000 0000

            destBitset[dest / 8] |= 1 << (dest % 8);

            if (first || dest != prevDest) {
                stats.noPaths++;
                first = false;
                prevDest = dest;
            }
        }
    }

    for (int i = 0; i < length; ++i) {
        for (int j = 0; j < 8; ++j) {
            if (destBitset[i] & 1 << j) stats.noIn++;
        }
    }

    delete destBitset;

    return stats;
}

std::shared_ptr<intermediate> SimpleEvaluator::project(uint32_t projectLabel, bool inverse, std::shared_ptr<SimpleGraph> &in) {

    auto out = std::make_shared<intermediate>();

    if (!inverse) {
        for (const auto &sourceDestPair : in->edgeLists[projectLabel]) {
            (*out)[sourceDestPair.first].emplace_back(sourceDestPair.second);
        }
    } else {
        for (const auto &sourceDestPair : in->edgeLists[projectLabel]) {
            (*out)[sourceDestPair.second].emplace_back(sourceDestPair.first);
        }
    }

    /*
    if(!inverse) {
        // going forward
        for(uint32_t source = 0; source < in->getNoVertices(); source++) {
            for (auto labelTarget : in->edgeLists[source]) {

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
//    for (const auto &leftLabelPair : left->edgeLists) { // O(1)
//        for (const auto &rightLabelPair : right->edgeLists) { // O(1)
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
        for (auto labelTarget : left->edgeLists[leftSource]) {                             // O(E)

            int leftTarget = labelTarget.second;
            // try to join the left target with right source
            for (auto rightLabelTarget : right->edgeLists[leftTarget]) {                   // O(EL + ER)

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
    return computeStats(res);
}