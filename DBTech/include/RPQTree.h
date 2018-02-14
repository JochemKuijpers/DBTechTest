//
// Created by Nikolay Yakovets on 2018-02-02.
//

#ifndef QS_RPQTREE_H
#define QS_RPQTREE_H

#include <string>
#include <algorithm>

class RPQTree {

public:
    RPQTree *left;
    RPQTree *right;
    std::string data;

    RPQTree(std::string &payload, RPQTree *left, RPQTree *right) : data(payload), left(left), right(right) {}
    ~RPQTree();

    static RPQTree* strToTree(std::string &str);
    void print();

    bool isConcat();

    bool isLeaf();
    bool isUnary();
    bool isBinary();

};


#endif //QS_RPQTREE_H
