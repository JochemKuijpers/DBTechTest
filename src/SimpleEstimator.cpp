//
// Created by Nikolay Yakovets on 2018-02-01.
//

#include "SimpleGraph.h"
#include "SimpleEstimator.h"

#include <cmath>
#include <random>

SimpleEstimator::SimpleEstimator(std::shared_ptr<SimpleGraph> &g) :
    vertexIndexByLabel(),
    vertexIndexByLabelReverse(),
    outVertexByLabel(),
    inVertexByLabel() {

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

            // create unique vectors of vertices per label (vector instead of set for random access later)
            if (std::find(outVertexByLabel[label].begin(), outVertexByLabel[label].end(), vertex) == outVertexByLabel[label].end()) {
                outVertexByLabel[label].push_back(vertex);
            }
            if (std::find(inVertexByLabel[label].begin(), inVertexByLabel[label].end(), destination) == inVertexByLabel[label].end()) {
                inVertexByLabel[label].push_back(destination);
            }
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

double SimpleEstimator::generateSampling(std::vector<uint32_t> *from, std::vector<uint32_t> *to, uint32_t sampleSize) {
    std::vector<uint32_t> sampleIds;
    if (from->size() <= sampleSize) {
        for (auto f : *from) {
            to->push_back(f);
        }
        return 1;
    }

    // generate list of all sampleIds, shuffle it and only use the first sampleSize items
//    for (uint32_t i = 0; i < from->size(); sampleIds.push_back(i++));
//    std::random_shuffle(sampleIds.begin(), sampleIds.end());

    generateSampleIds(sampleSize, static_cast<uint32_t>(from->size() - 1), &sampleIds);

    for (uint32_t i = 0; i < sampleSize; i++) {
        to->push_back((*from)[sampleIds[i]]);
    }

    return (double) from->size() / sampleSize;
}

double SimpleEstimator::indexBasedJoinSampling(std::unordered_map<uint32_t, std::vector<uint32_t>> *index,
                                               std::vector<uint32_t> *from, std::vector<uint32_t> *to,
                                               uint32_t sampleSize) {
    std::vector<uint32_t> sampleIds;
    uint32_t cpt = 0;
    std::vector<uint32_t> cptPerVertex;

    for (uint32_t i = 0; i < from->size(); ++i) {
        auto fromVertex = (*from)[i];
        auto image = (*index)[fromVertex];
        cpt += image.size();
        cptPerVertex.push_back(cpt);
    }

    if (cpt <= sampleSize) {
        // the entire join fits in the sampling, skip expensive stuff and just return the image
        for (auto fromVertex : *from) {
            for (auto toVertex : (*index)[fromVertex]) {
                to->push_back(toVertex);
            }
        }
        return 1.0;
    }

    // generate list of all sampleIds, shuffle it and only use the first sampleSize items
//    for (uint32_t i = 0; i < cpt; sampleIds.push_back(i++));
//    std::random_shuffle(sampleIds.begin(), sampleIds.end());
    generateSampleIds(sampleSize, cpt - 1, &sampleIds);

    uint32_t ID;
    uint32_t fromVertexIndex;
    uint32_t offset;
    for (uint32_t i = 0; i < sampleSize; ++i) {
        ID = sampleIds[i];
        // fromVertexIndex is the index such that (*from)[index] maps to the ID-th element in the join
        fromVertexIndex = 0; for (; ID >= cptPerVertex[fromVertexIndex]; ++fromVertexIndex);
        // offset is such that the offset-th element that maps from fromVertexIndex is the ID-th element in the join
        if (fromVertexIndex > 0)
            offset = ID - cptPerVertex[fromVertexIndex - 1];
        else
            offset = ID;

        to->push_back((*index)[(*from)[fromVertexIndex]][offset]);
    }

    return (double) cpt / sampleSize;
}

void SimpleEstimator::generateSampleIds(uint32_t sampleSize, uint32_t maxValue, std::vector<uint32_t> *sampleIds) {
    sampleIds->clear();

    if (sampleSize > maxValue/2) {
        for (uint32_t i = 0; i <  std::min(sampleSize, maxValue); sampleIds->push_back(i++));
        std::random_shuffle(sampleIds->begin(), sampleIds->end());
        return;
    }

    std::unordered_set<uint32_t> uniqueSampleIds;
    std::uniform_int_distribution<uint32_t> distribution(0, maxValue);
    std::random_device generator;

    uint32_t r;
    for (uint32_t i = 0; i < std::min(sampleSize, maxValue); ++i) {
        do {
            r = distribution(generator);
        } while (uniqueSampleIds.find(r) != uniqueSampleIds.end());

        uniqueSampleIds.insert(r);
    }

    for (auto ID : uniqueSampleIds) {
        sampleIds->push_back(ID);
    }
}

cardStat SimpleEstimator::estimate(RPQTree *q) {
    auto path = std::vector<std::pair<uint32_t, bool>>();
    unpackQueryTree(&path, q);

    if (path.empty()) { return {0, 0, 0}; }

    auto *leftSamples = new std::vector<uint32_t>();
    auto *rightSamples = new std::vector<uint32_t>();

    double underSampling;
    uint32_t MAX_SAMPLING = 256;

    // generate uniform sampling of the outVertices for the first label
    if (path[0].second) {
        underSampling = generateSampling(&outVertexByLabel[path[0].first], leftSamples, MAX_SAMPLING);
    } else {
        underSampling = generateSampling(&inVertexByLabel[path[0].first], leftSamples, MAX_SAMPLING);
    }

    std::unordered_map<uint32_t, std::vector<uint32_t>> *mapping;
    // evaluate the query along the query path
    for (auto step : path) {
        // take either the forwards or backwards index of the current label, depending on the direction
        if (step.second) {
            mapping = &vertexIndexByLabel[step.first];
        } else {
            mapping = &vertexIndexByLabelReverse[step.first];
        }

        // calculate the image of the mapping, and update the new underSampling factor
        underSampling *= indexBasedJoinSampling(mapping, leftSamples, rightSamples, MAX_SAMPLING);

        // mapping image becomes pre-image for the next step, image vector is cleared.
        auto t = leftSamples;
        leftSamples = rightSamples;
        rightSamples = t;
        rightSamples->clear();
    }

    // return {1, (image size * undersampling), 1}
    // since we have no calculation for noIn and noOut.
    auto noPaths = static_cast<uint32_t>(leftSamples->size() * underSampling);
    delete leftSamples;
    delete rightSamples;
    return {static_cast<uint32_t>(noPaths / underSampling), noPaths, static_cast<uint32_t>(underSampling)};
}
