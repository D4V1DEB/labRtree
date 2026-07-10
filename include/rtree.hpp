#pragma once

#include <utility>
#include <algorithm>

#include "rect.hpp"
#include "node.hpp"

class RTree {
public:
    Node* root = nullptr;
    int max_entries = 8;
    int min_entries = 4;

    RTree(int max_entries_) : root(nullptr), max_entries(max_entries_) {
        if (max_entries <= 2) max_entries = 2;
        min_entries = std::max(1, max_entries / 2);
        root = new Node();
        root->is_leaf = true;
        root->parent = nullptr;
        root->mbr = Rect();
    }

    ~RTree() {
        if (root) {
            destroyNode(root);
            root = nullptr;
        }
    }

    void insert(int point_id, double x, double y) {
        Rect r(x, y, x, y);
        Node* leaf = chooseLeaf(root, r);
        Node::Entry e;
        e.mbr = r;
        e.child = nullptr;
        e.point_id = point_id;
        leaf->entries.push_back(e);
        leaf->updateMBR();
        adjustTree(leaf);
    }

    Node* chooseLeaf(Node* node, const Rect& r) {
        if (!node) return nullptr;
        if (node->is_leaf) return node;

        double bestIncrease = std::numeric_limits<double>::infinity();
        double bestArea = std::numeric_limits<double>::infinity();
        Node::Entry* bestEntry = nullptr;

        for (auto& entry : node->entries) {
            double areaBefore = entry.mbr.area();
            Rect expanded = entry.mbr.expandedWith(r);
            double areaAfter = expanded.area();
            double increase = areaAfter - areaBefore;

            if (increase < bestIncrease || (increase == bestIncrease && areaBefore < bestArea)) {
                bestIncrease = increase;
                bestArea = areaBefore;
                bestEntry = &entry;
            }
        }

        if (!bestEntry || !bestEntry->child) {
            return node;
        }
        return chooseLeaf(bestEntry->child, r);
    }

    void adjustTree(Node* node) {
        while (node != nullptr) {
            node->updateMBR();
            if (node->isFull(max_entries)) {
                auto splitResult = split(node);
                Node* n1 = splitResult.first;
                Node* n2 = splitResult.second;

                if (n1 == nullptr && n2 == nullptr) {
                    node = node->parent;
                    continue;
                }

                if (node->parent == nullptr) {
                    Node* newRoot = new Node();
                    newRoot->is_leaf = false;
                    n1->parent = newRoot;
                    n2->parent = newRoot;

                    Node::Entry e1; e1.mbr = n1->mbr; e1.child = n1; e1.point_id = -1;
                    Node::Entry e2; e2.mbr = n2->mbr; e2.child = n2; e2.point_id = -1;
                    newRoot->entries.push_back(e1);
                    newRoot->entries.push_back(e2);
                    newRoot->updateMBR();
                    root = newRoot;
                    return;
                } else {
                    Node* p = node->parent;
                    bool replaced = false;
                    for (auto it = p->entries.begin(); it != p->entries.end(); ++it) {
                        if (it->child == node) {
                            it->child = n1;
                            it->mbr = n1->mbr;
                            n1->parent = p;
                            replaced = true;
                            break;
                        }
                    }
                    Node::Entry newEntry; newEntry.mbr = n2->mbr; newEntry.child = n2; newEntry.point_id = -1;
                    n2->parent = p;
                    p->entries.push_back(newEntry);

                    node = p;
                    continue;
                }
            } else {
                node = node->parent;
            }
        }
    }

    std::pair<Node*, Node*> split(Node* node) {
        (void)node;
        return {nullptr, nullptr};
    }

private:
    void destroyNode(Node* node) {
        if (!node) return;
        for (auto& e : node->entries) {
            if (e.child != nullptr) {
                destroyNode(e.child);
            }
        }
        delete node;
    }
};
