//
// Created by Nikolay Yakovets on 2018-02-02.
//

#include <iostream>
#include "RPQTree.h"

RPQTree::~RPQTree() {
    delete(left);
    delete(right);
}

RPQTree* RPQTree::strToTree(std::string &str) {

    str.erase(std::remove_if(str.begin(), str.end(), ::isspace), str.end()); // remove spaces

    int level = 0; // inside parentheses check

    // case /
    // most right '/' (but not inside '()') search and split
    for(int i=(int) str.size()-1;i>=0;--i){
        char c = str[i];
        if(c == ')'){
            ++level;
            continue;
        }
        if(c == '('){
            --level;
            continue;
        }
        if(level>0) continue;
        if(c == '/'){
            std::string left(str.substr(0,i));
            std::string right(str.substr(i+1));
            std::string payload(1, c);
            return new RPQTree(payload, strToTree(left), strToTree(right));
        }
    }

    if(str[0]=='('){
        //case ()
        //pull out inside and to strToTree
        for(int i=0;i<str.size();++i){
            if(str[i]=='('){
                ++level;
                continue;
            }
            if(str[i]==')'){
                --level;
                if(level==0){
                    std::string exp(str.substr(1, i-1));
                    return strToTree(exp);
                }
                continue;
            }
        }
    } else
        // case value
        return new RPQTree(str, nullptr, nullptr);

    std::cerr << "Error: parsing RPQ failed." << std::endl;
    return nullptr;
}

void RPQTree::print() {

    if(left == nullptr && right == nullptr) {
        std::cout << ' ' << data << ' ';
    } else {
        std::cout << '(' << data << ' ';
        if(left != nullptr) left->print();
        if(right!= nullptr) right->print();
        std::cout << ')';
    }

}

bool RPQTree::isConcat() {
    return (data == "/") && isBinary();
}

bool RPQTree::isBinary() {
    return left != nullptr && right != nullptr;
}

bool RPQTree::isUnary() {
    return left != nullptr && right == nullptr;
}

bool RPQTree::isLeaf() {
    return left == nullptr && right == nullptr;
}