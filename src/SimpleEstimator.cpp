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
    const internalCardStat result = internalEstimate(q);
    return {result.noOut, result.noPaths, result.noIn};
}

internalCardStat SimpleEstimator::internalEstimate(RPQTree *q) {
    if (q->isConcat()) {
        const internalCardStat left = internalEstimate(q->left);
        const internalCardStat right = internalEstimate(q->right);

        // zero cases.
        if (left.noIn == 0 || left.noOut == 0 || left.noPaths == 0 || right.noIn == 0 || right.noOut == 0 || right.noPaths == 0) {
            return internalCardStat {0, 0, 0};
        }

        return internalCardStat {static_cast<uint32_t>(left.noIn),
                                 static_cast<uint32_t>(
                                         left.noOut *
                                         ((double)left.noPaths / left.noIn) *
                                         ((double)right.noPaths / right.noOut)
                                 ),
                                 static_cast<uint32_t>(right.noOut),
                                 left.firstLabel,
                                 right.lastLabel,
                                 left.firstDirection,
                                 right.lastDirection};

    }

    // single edge query
    char* sign;
    const auto label = static_cast<uint32_t>(strtoll(q->data.c_str(), &sign, 10));
    if (*sign == '+') {
        return internalCardStat {static_cast<uint32_t>(startVerticesPerEdgeLabel[label].size()),
                                 static_cast<uint32_t>(vertexPairsPerEdgeLabel[label].size()),
                                 static_cast<uint32_t>(endVerticesPerEdgeLabel[label].size()),
                                 label, label, true, true};
    } else {
        return internalCardStat {static_cast<uint32_t>(endVerticesPerEdgeLabel[label].size()),
                                 static_cast<uint32_t>(vertexPairsPerEdgeLabel[label].size()),
                                 static_cast<uint32_t>(startVerticesPerEdgeLabel[label].size()),
                                 label, label, false, false};
    }
}