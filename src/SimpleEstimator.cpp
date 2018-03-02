//
// Created by Nikolay Yakovets on 2018-02-01.
//

#include "SimpleGraph.h"
#include "SimpleEstimator.h"

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
            vertexPairsPerEdgeLabel[label]++;
        }

        for (uint32_t label = 0; label < graph->getNoLabels(); ++label) {
            for (auto out : graph->adj[vert]) {
                if (out.first == label) {
                    startVerticesPerEdgeLabel[label]++;
                    break;
                }
            }

            for (auto in : graph->reverse_adj[vert]) {
                if (label == in.first) {
                    endVerticesPerEdgeLabel[label]++;
                    break;
                }
            }
        }
    }
}

cardStat SimpleEstimator::estimate(RPQTree *q) {
    if (q->isConcat()) {
        const cardStat left = estimate(q->left);
        const cardStat right = estimate(q->right);

        // zero cases.
        if (left.noIn == 0 || left.noOut == 0 || left.noPaths == 0 || right.noIn == 0 || right.noOut == 0 || right.noPaths == 0) {
            return cardStat {0, 0, 0};
        }

        // combine the two subqueries (too simple for now..)
        return cardStat {left.noOut,
            (uint32_t) std::min(
                left.noPaths * (double)right.noPaths / left.noIn,
                right.noPaths * (double)left.noPaths / right.noOut
            ),
            right.noIn};

    }

    // single edge query
    char* sign;
    const auto label = static_cast<uint32_t>(strtoll(q->data.c_str(), &sign, 10));
    if (*sign == '+') {
        return cardStat {
            startVerticesPerEdgeLabel[label],
            vertexPairsPerEdgeLabel[label],
            endVerticesPerEdgeLabel[label]
        };
    } else {
        return cardStat {
            endVerticesPerEdgeLabel[label],
            vertexPairsPerEdgeLabel[label],
            startVerticesPerEdgeLabel[label]
        };
    }
}