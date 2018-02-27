//
// Created by Nikolay Yakovets on 2018-02-01.
//

#include "SimpleGraph.h"
#include "SimpleEstimator.h"

#include <unordered_map>
#include <unordered_set>
#include <cmath>

SimpleEstimator::SimpleEstimator(std::shared_ptr<SimpleGraph> &g) :
    startVerticesPerEdgeLabel(),
    endVerticesPerEdgeLabel(),
    vertexPairsPerEdgeLabel() {

    // works only with SimpleGraph
    graph = g;
}

void SimpleEstimator::prepare() {
    // adj        [vertex][n] = pair(edgelabel, destination)
    // reverse_adj[vertex][n] = pair(edgelabel, origin)

    for (uint32_t vert = 0; vert < graph->getNoVertices(); ++vert) {
        for (auto out : graph->adj[vert]) {
            const auto label = out.first;
            const auto destination = out.second;

            startVerticesPerEdgeLabel[label].insert(vert);
            endVerticesPerEdgeLabel[label].insert(destination);
            vertexPairsPerEdgeLabel[label].insert(std::pair<uint32_t, uint32_t>(vert, destination));
        }
    }
}

cardStat SimpleEstimator::estimate(RPQTree *q) {
    if (q->isConcat()) {
        const cardStat left = estimate(q->left);
        const cardStat right = estimate(q->right);

        // zero cases.
        if (left.noIn == 0 || left.noOut == 0 || left.noPaths == 0 || right.noIn == 0 || right.noOut == 0 || right.noPaths == 0) {
            return cardStat{0, 0, 0};
        }

        return cardStat {static_cast<uint32_t>(left.noIn),
                         static_cast<uint32_t>(left.noOut * ((double)left.noPaths / left.noIn) * ((double)right.noPaths / right.noOut)),
                         static_cast<uint32_t>(right.noOut)};

    }

    // single edge query
    const auto label = static_cast<uint32_t>(stoll(q->data));

    return cardStat {startVerticesPerEdgeLabel[label].size(),
                     vertexPairsPerEdgeLabel[label].size(),
                     endVerticesPerEdgeLabel[label].size()};
}