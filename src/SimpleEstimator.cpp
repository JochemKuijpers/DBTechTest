//
// Created by Nikolay Yakovets on 2018-02-01.
//

#include "SimpleGraph.h"
#include "SimpleEstimator.h"

#include <cmath>
#include <random>

SimpleEstimator::SimpleEstimator(std::shared_ptr<SimpleGraph> &g) :
    vertexIndexByLabel(),
    vertexIndexByLabelReverse() {

    // works only with SimpleGraph
    graph = g;
}

void SimpleEstimator::prepare() {
    // adj        [vertex][n] = pair(edgelabel, destination)
    // reverse_adj[vertex][n] = pair(edgelabel, origin)

    for (uint32_t vertex = 0; vertex < graph->getNoVertices(); ++vertex) {
        for (auto edge : graph->adj[vertex]) {
            auto label = edge.first;
            auto destination = edge.second;

            vertexIndexByLabel[label][vertex].push_back(destination);
        }

        for (auto edge : graph->reverse_adj[vertex]) {
            auto label = edge.first;
            auto origin = edge.second;

            vertexIndexByLabelReverse[label][vertex].push_back(origin);
        }
    }
}

void unpackQueryTree(std::vector<std::pair<uint32_t, bool>> *path, RPQTree *q) {
    if (q->isConcat()) {
        unpackQueryTree(path, q->left);
        unpackQueryTree(path, q->right);
        return;
    }

    char* sign;
    const auto label = static_cast<uint32_t>(strtoll(q->data.c_str(), &sign, 10));
    path->emplace_back(label, *sign == '+');
}

cardStat SimpleEstimator::estimate(RPQTree *q) {
    auto path = std::vector<std::pair<uint32_t, bool>>();
    unpackQueryTree(&path, q);

    if (path.empty()) { return {0, 0, 0}; }

    std::vector<uint32_t> leftSamples;
    std::vector<uint32_t> rightSamples;

    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(0, 1);

    double underSampling = 1;
    auto MAX_SAMPLING = 1000;

    // generate uniform sampling of the outVertices for the first label
    uint32_t sum = 0;
    for (auto vertexIndex : vertexIndexByLabel[path[0].first]) {
        leftSamples.push_back(vertexIndex.first);
    }

    // if the sample list is too large, reduce it and increase the undersampling factor.
    if (leftSamples.size() > MAX_SAMPLING) {
        underSampling *= (double) leftSamples.size() / MAX_SAMPLING;
        std::random_shuffle(leftSamples.begin(), leftSamples.end(), generator);
        leftSamples.resize(MAX_SAMPLING);
    }

    // evaluate the query along the query path
    for (auto step : path) {
        // calculate the right samples (the samples created by the left samples and the current edge label)
        for (auto left : leftSamples) {
            if (step.second) {
                for (auto right : vertexIndexByLabel[step.first][left]) {
                    rightSamples.push_back(right);
                }
            } else {
                for (auto right : vertexIndexByLabelReverse[step.first][left]) {
                    rightSamples.push_back(right);
                }
            }
        }

        // if the right sample list is too large, reduce it and increase the undersampling factor.
        if (rightSamples.size() > MAX_SAMPLING) {
            underSampling *= (double) rightSamples.size() / MAX_SAMPLING;
            std::random_shuffle(rightSamples.begin(), rightSamples.end(), generator);
            rightSamples.resize(MAX_SAMPLING);
        }

        // copy right samples to left, clear right samples.
        leftSamples.clear();
        leftSamples = rightSamples;
        rightSamples.clear();
    }

    // return {1, (number of samples * undersampling), 1}
    // since we have no calculation for noIn and noOut.
    return {1, static_cast<uint32_t>(leftSamples.size() * underSampling), 1};
}