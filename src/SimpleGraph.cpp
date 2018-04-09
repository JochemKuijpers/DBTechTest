//
// Created by Nikolay Yakovets on 2018-01-31.
//

#include "SimpleGraph.h"

SimpleGraph::SimpleGraph(uint32_t n)   {
    setNoVertices(n);
}

uint32_t SimpleGraph::getNoVertices() const {
    return V;
}

void SimpleGraph::setNoVertices(uint32_t n) {
    V = n;
}

uint32_t SimpleGraph::getNoEdges() const {
    uint32_t sum = 0;
    for (const auto &edgeList : edgeLists) {
        sum += edgeList.size();
    }
    return sum;
}

// sort on the second item in the pair, then on the first (ascending order)
bool sortPairs(const std::pair<uint32_t,uint32_t> &a, const std::pair<uint32_t,uint32_t> &b) {
    if (a.second < b.second) return true;
    if (a.second == b.second) return a.first < b.first;
    return false;
}

uint32_t SimpleGraph::getNoDistinctEdges() const {
    uint32_t sum = 0;

    for (auto edgeList : edgeLists) {
        std::sort(edgeList.begin(), edgeList.end(), sortPairs);

        uint32_t prevSource = 0;
        uint32_t prevDest = 0;
        bool first = true;

        for (const auto &sourceDestPair : edgeList) {
            if (first || !(prevSource == sourceDestPair.first && prevDest == sourceDestPair.second)) {
                first = false;
                sum++;
                prevSource = sourceDestPair.first;
                prevDest = sourceDestPair.second;
            }
        }
    }

    return sum;
}

uint32_t SimpleGraph::getNoLabels() const {
    return L;
}

void SimpleGraph::setNoLabels(uint32_t noLabels) {
    L = noLabels;
    edgeLists.reserve(L);
    for (int i = 0; i < L; ++i) {
        edgeLists.emplace_back(std::vector<std::pair<uint32_t, uint32_t>>());
    }
}

void SimpleGraph::addEdge(uint32_t from, uint32_t to, uint32_t edgeLabel) {
    if(from >= V || to >= V || edgeLabel >= L)
        throw std::runtime_error(std::string("Edge data out of bounds: ") +
                                         "(" + std::to_string(from) + "," + std::to_string(to) + "," +
                                         std::to_string(edgeLabel) + ")");

    edgeLists[edgeLabel].emplace_back(std::make_pair(from, to));
}

bool getValuesFromLine(const std::string &line, char sep, uint32_t (&values)[3]) {
    size_t ppos = 0;
    size_t pos;

    for (int i = 0; i < 3; ++i) {
        pos = line.find(sep, ppos);
        if (pos == std::string::npos) {
            values[i] = static_cast<uint32_t>(std::stoul(line.substr(ppos)));
            return i == 2;
        }
        values[i] = static_cast<uint32_t>(std::stoul(line.substr(ppos, pos-ppos)));
        ppos = pos + 1;
    }
    return true;
}

void SimpleGraph::readFromContiguousFile(const std::string &fileName) {
    std::string line;
    std::ifstream graphFile { fileName };

    // parse the header (1st line)
    // header format: "noNodes,noEdges,noLabels\n"
    std::getline(graphFile, line);
    uint32_t values[3];
    if (!getValuesFromLine(line, ',', values)) {
        throw std::runtime_error(std::string("Invalid graph header!"));
    }
    uint32_t noNodes = values[0];
    uint32_t noLabels = values[2];

    setNoVertices(noNodes);
    setNoLabels(noLabels);

    // parse edge data
    // edge data format: "source label destination .\n"
    while(std::getline(graphFile, line)) {
        if (getValuesFromLine(line, ' ', values)) {
            addEdge(values[0], values[2], values[1]);
        }
    }

    graphFile.close();

}