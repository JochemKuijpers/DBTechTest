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

uint32_t SimpleEstimator::sampleLabel(uint32_t label, std::vector<uint32_t> &samplesIn, std::vector<uint32_t> &samplesOut, uint32_t maxSize) {
    auto cpt = std::vector<std::pair<uint32_t, std::vector<uint32_t>>>();
    auto cptSumTo = std::vector<uint32_t>();
    uint32_t sum = 0;

    for (auto sample : samplesIn) {
        auto indexOfSample = vertexIndexByLabel[label][sample];
        sum += indexOfSample.size();

        cpt.emplace_back(sample, indexOfSample);
        cptSumTo.push_back(sum);
    }

    samplesOut.clear();
    auto sampleIDs = std::vector<uint32_t>();

    // shuffle a vector [0 ... sum] and use only the first min(sum, maxSize) elements to obtain uniformly distributed IDs
    for (uint32_t i = 0; i < sum; ++i) {
        sampleIDs.push_back(i);
    }
    std::random_shuffle(sampleIDs.begin(), sampleIDs.end());

    for (uint32_t i = 0; i < std::min(sum, maxSize); ++i) {
        auto id = sampleIDs[i];
        // find inSampleIndex such that inSampleIndex = max{inSampleIndex | cptSumTo[inSampleIndex] <= id}
        auto inSampleIndex = 0; for (;cptSumTo[inSampleIndex++] <= id;); inSampleIndex--;
        // auto leftSample = cpt[inSampleIndex].first;
        auto offset = id;
        if (inSampleIndex > 0) {
            offset -= cptSumTo[inSampleIndex-1];
        }
        auto outSample = cpt[inSampleIndex].second[offset];
        samplesOut.push_back(outSample);
    }

    return sum;
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

    bool first = true;
    std::vector<uint32_t> leftSamples;
    std::vector<uint32_t> rightSamples;

    double underSampling = 250;
    auto MAX_SAMPLING = static_cast<const int>(graph->getNoVertices() / underSampling);
    int skip = graph->getNoVertices() / MAX_SAMPLING;

    std::default_random_engine generator;
    std::uniform_int_distribution<int> distribution(1,skip);

    for (auto step : path) {
        if (first) {
            for (uint32_t vertex = 0; vertex < graph->getNoVertices(); vertex += distribution(generator)) {
                leftSamples.push_back(vertex);
            }
            first = false;
        } else {
            leftSamples.clear();
            leftSamples = rightSamples;
            rightSamples.clear();
        }

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

        if (rightSamples.size() > MAX_SAMPLING) {
            underSampling *= (rightSamples.size() / MAX_SAMPLING);
            while (rightSamples.size() > MAX_SAMPLING) {
                distribution = std::uniform_int_distribution<int>(0, static_cast<int>(rightSamples.size() - 1));
                rightSamples.erase(rightSamples.begin() + (distribution(generator)));
            }
        }
    }

    return {1, static_cast<uint32_t>(rightSamples.size() * underSampling), 1};
}