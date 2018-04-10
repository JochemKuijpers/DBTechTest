//
// Created by Nikolay Yakovets on 2018-02-02.
//

#include <chrono>
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

cardStat SimpleEvaluator::computeStats(std::shared_ptr<intermediate> &result) {

    cardStat stats {0, 0, 0};

    stats.noOut = static_cast<uint32_t>(result->size());

    std::unordered_set<uint32_t> uniqueDests;

    uint32_t length = graph->getNoVertices() / sizeof(uint8_t) + 1;
    auto *destBitset = static_cast<uint8_t *>(malloc(length));
    for (int i = 0; i < length; ++i) { destBitset[i] = 0; }

    for (auto sourceDestListPair : *result) {
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

    return out;
}

std::shared_ptr<intermediate> SimpleEvaluator::evaluate_aux(RPQTree *q) {
    // evaluate cache
    query_path path;
    unpackQueryTree(&path, q);
    const uint32_t pathHash = hashPath(&path);
    auto search = evalCache.find(pathHash);
    if (search != evalCache.end()) {
        return search->second;
    }

    std::shared_ptr<intermediate> result;

    // evaluate according to the AST bottom-up
    if(q->isLeaf()) {
        // project out the label in the AST
        uint32_t label = (uint32_t) std::stoul(q->data.substr(0, q->data.length()-1));
        bool inverse = q->data.at(q->data.length()-1) == '-';

        result = SimpleEvaluator::project(label, inverse, graph);
    }

    if(q->isConcat()) {
        // evaluate the children
        auto leftResult = SimpleEvaluator::evaluate_aux(q->left);
        auto rightResult = SimpleEvaluator::evaluate_aux(q->right);

        // join left with right
        result = SimpleEvaluator::join(leftResult, rightResult);
    }

    evalCache[pathHash] = result;
    return result;
}

cardStat SimpleEvaluator::evaluate(RPQTree *query) {

    std::cout << "\nOriginal query:\n";
    query->print();

    auto preOpt = std::chrono::steady_clock::now();

    RPQTree *optimizedQuery = query;

    if (est != nullptr) {
        std::vector<std::pair<uint32_t, bool>> path;
        unpackQueryTree(&path, query);

        optimizedQuery = optimizeQuery(&path);
    }

    std::cout << "\nOptimized query:\n";
    optimizedQuery->print();
    std::cout << "\n";

    auto start = std::chrono::steady_clock::now();
    auto res = evaluate_aux(query);
    auto inter = std::chrono::steady_clock::now();
    auto res2 = evaluate_aux(optimizedQuery);
    auto end = std::chrono::steady_clock::now();

    std::cout << res2 << "\n";

    std::cout << "Optimize: " << std::chrono::duration<double, std::milli>(start - preOpt).count() << " ms\n";
    std::cout << "Original: " << std::chrono::duration<double, std::milli>(inter-start).count() << " ms\n";
    std::cout << "Optmized: " << std::chrono::duration<double, std::milli>(end-inter).count() << " ms\n";
    return computeStats(res);
}

uint32_t SimpleEvaluator::hashPath(query_path *path) {
    uint32_t hash = static_cast<uint32_t>(path->size());
    for(const auto &pair : *path) {
        hash = (hash << 3) | (pair.first << 1 | pair.second);
    }
    return hash;
}

void SimpleEvaluator::unpackQueryTree(std::vector<std::pair<uint32_t, bool>> *path, RPQTree *q) {
    if (q->isConcat()) {
        unpackQueryTree(path, q->left);
        unpackQueryTree(path, q->right);
        return;
    }

    char* sign;
    const auto label = static_cast<uint32_t>(strtoll(q->data.c_str(), &sign, 10));
    path->emplace_back(label, *sign == '+');
}

RPQTree* SimpleEvaluator::optimizeQuery(std::vector<std::pair<uint32_t, bool>> *path) {

    if (path->size() == 1) {
        auto data = std::to_string((*path)[0].first) + ((*path)[0].second ? "+" : "-");
        return new RPQTree(data, nullptr, nullptr);
    }

    std::vector<std::pair<uint32_t, bool>> leftPath, rightPath;

    uint32_t bestEstimation = 0xFFFFFFFF;
    uint32_t bestEstimationSplit = 0;

    for (uint32_t split = 0; split < path->size()-1; ++split) {
        leftPath.clear();
        rightPath.clear();
        for (int i = 0; i < path->size(); ++i) {
            if (i <= split) { leftPath.emplace_back((*path)[i]); }
            else           { rightPath.emplace_back((*path)[i]); }
        }

        cardStat leftEst = est->estimate_aux(leftPath);
        cardStat rightEst = est->estimate_aux(rightPath);
//        uint32_t currentEst = leftEst.noPaths + rightEst.noPaths;
//        uint32_t currentEst = leftEst.noPaths / rightEst.noPaths;
        uint32_t currentEst = std::max(leftEst.noPaths, rightEst.noPaths);

        if (currentEst < bestEstimation) {
            bestEstimation = currentEst;
            bestEstimationSplit = split;
        }
    }

    leftPath.clear();
    rightPath.clear();
    for (int i = 0; i < path->size(); ++i) {
        if (i <= bestEstimationSplit) {
            leftPath.emplace_back((*path)[i]);
        } else {
            rightPath.emplace_back((*path)[i]);
        }
    }

    RPQTree* leftTree = optimizeQuery(&leftPath);
    RPQTree* rightTree = optimizeQuery(&rightPath);

    std::string data = "/";
    return new RPQTree(data, leftTree, rightTree);
}
