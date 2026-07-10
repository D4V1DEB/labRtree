#pragma once

#include <utility>
#include <algorithm>
#include <limits>
#include <vector>

#include "rect.hpp"
#include "node.hpp"

class RStarTree {
public:
    Node* root = nullptr;
    int max_entries = 8;
    int min_entries = 4;
    std::vector<bool> level_reinserted;

    RStarTree(int max_entries_) : root(nullptr), max_entries(max_entries_) {
        if (max_entries <= 2) max_entries = 2;
        min_entries = std::max(1, max_entries / 2);
        root = new Node();
        root->is_leaf = true;
        root->parent = nullptr;
        root->mbr = Rect();
    }

    ~RStarTree() {
        if (root) {
            destroyNode(root);
            root = nullptr;
        }
    }

    void insert(int point_id, double x, double y) {
        level_reinserted.clear();

        Rect r(x, y, x, y);
        insertPointInternal(point_id, r, 0);
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
        int level = nodeLevel(node);
        adjustTree(node, level);
    }

    void forcedReinsert(Node* node, int level) {
        if (!node) return;

        ensureLevelFlag(level);
        if (level_reinserted[level]) {
            return;
        }

        const int originalSize = node->size();
        if (originalSize <= 0) {
            level_reinserted[level] = true;
            return;
        }

        const double centerX = (node->mbr.min_x + node->mbr.max_x) * 0.5;
        const double centerY = (node->mbr.min_y + node->mbr.max_y) * 0.5;

        std::vector<std::pair<double, Node::Entry>> ranked;
        ranked.reserve(node->entries.size());
        for (const auto& entry : node->entries) {
            const double entryCenterX = (entry.mbr.min_x + entry.mbr.max_x) * 0.5;
            const double entryCenterY = (entry.mbr.min_y + entry.mbr.max_y) * 0.5;
            const double dx = entryCenterX - centerX;
            const double dy = entryCenterY - centerY;
            const double dist2 = dx * dx + dy * dy;
            ranked.push_back({dist2, entry});
        }

        std::sort(ranked.begin(), ranked.end(), [](const auto& lhs, const auto& rhs) {
            if (lhs.first != rhs.first) return lhs.first > rhs.first;
            return lhs.second.point_id > rhs.second.point_id;
        });

        int toReinsert = static_cast<int>((originalSize * 3 + 9) / 10);
        if (toReinsert < 1) toReinsert = 1;
        if (toReinsert > originalSize) toReinsert = originalSize;

        std::vector<Node::Entry> extracted;
        extracted.reserve(toReinsert);
        for (int i = 0; i < toReinsert; ++i) {
            extracted.push_back(ranked[i].second);
        }

        std::vector<Node::Entry> remaining;
        remaining.reserve(originalSize - toReinsert);
        for (int i = toReinsert; i < originalSize; ++i) {
            remaining.push_back(ranked[i].second);
        }

        node->entries = remaining;
        node->updateMBR();
        level_reinserted[level] = true;

        for (auto& entry : extracted) {
            if (entry.child == nullptr) {
                insertPointAtLevel(entry.point_id, entry.mbr.min_x, entry.mbr.min_y, 0);
            } else {
                entry.child->parent = nullptr;
                insertSubtreeAtLevel(entry.child, entry.mbr, level);
            }
        }
    }

    void adjustTree(Node* node, int level) {
        while (node != nullptr) {
            node->updateMBR();
            if (node->isFull(max_entries)) {
                if (node->parent != nullptr) {
                    ensureLevelFlag(level);
                    if (!level_reinserted[level]) {
                        forcedReinsert(node, level);
                        node = node->parent;
                        ++level;
                        continue;
                    }
                }

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
                    ++level;
                    continue;
                }
            } else {
                node = node->parent;
                ++level;
            }
        }
    }

    std::pair<Node*, Node*> split(Node* node) {
        if (!node) return {nullptr, nullptr};

        std::vector<Node::Entry> all;
        all.reserve(node->entries.size());
        for (auto &e : node->entries) all.push_back(e);

        auto get_min = [&](const Node::Entry &e, int axis) {
            return (axis == 0) ? e.mbr.min_x : e.mbr.min_y;
        };
        auto get_max = [&](const Node::Entry &e, int axis) {
            return (axis == 0) ? e.mbr.max_x : e.mbr.max_y;
        };

        int bestAxis = 0;
        double bestSep = -std::numeric_limits<double>::infinity();
        for (int axis = 0; axis < 2; ++axis) {
            double minLow = std::numeric_limits<double>::infinity();
            double maxLow = -std::numeric_limits<double>::infinity();
            double minHigh = std::numeric_limits<double>::infinity();
            double maxHigh = -std::numeric_limits<double>::infinity();
            for (auto &e : all) {
                double low = get_min(e, axis);
                double high = get_max(e, axis);
                minLow = std::min(minLow, low);
                maxLow = std::max(maxLow, low);
                minHigh = std::min(minHigh, high);
                maxHigh = std::max(maxHigh, high);
            }
            double width = maxHigh - minLow;
            double separation = 0.0;
            if (width > 0.0) {
                separation = (maxLow - minHigh) / width;
            } else {
                separation = 0.0;
            }
            if (separation > bestSep) {
                bestSep = separation;
                bestAxis = axis;
            }
        }

        int seed1 = -1, seed2 = -1;
        double smallestLow = std::numeric_limits<double>::infinity();
        double largestHigh = -std::numeric_limits<double>::infinity();
        for (size_t i = 0; i < all.size(); ++i) {
            double low = get_min(all[i], bestAxis);
            double high = get_max(all[i], bestAxis);
            if (low < smallestLow) { smallestLow = low; seed1 = static_cast<int>(i); }
            if (high > largestHigh) { largestHigh = high; seed2 = static_cast<int>(i); }
        }

        if (seed1 == -1 || seed2 == -1) return {nullptr, nullptr};
        if (seed1 == seed2) {
            seed2 = (seed1 + 1) % static_cast<int>(all.size());
        }

        Node* n1 = new Node();
        Node* n2 = new Node();
        n1->is_leaf = node->is_leaf;
        n2->is_leaf = node->is_leaf;

        n1->entries.push_back(all[seed1]);
        n2->entries.push_back(all[seed2]);

        std::vector<char> assigned(all.size(), 0);
        assigned[seed1] = 1;
        assigned[seed2] = 1;

        n1->updateMBR();
        n2->updateMBR();

        int remaining = static_cast<int>(all.size()) - 2;
        size_t idx = 0;
        while (remaining > 0) {
            int need1 = std::max(0, min_entries - n1->size());
            int need2 = std::max(0, min_entries - n2->size());
            if (need1 == remaining) {
                for (size_t i = 0; i < all.size(); ++i) if (!assigned[i]) {
                    n1->entries.push_back(all[i]); assigned[i] = 1; --remaining;
                }
                break;
            }
            if (need2 == remaining) {
                for (size_t i = 0; i < all.size(); ++i) if (!assigned[i]) {
                    n2->entries.push_back(all[i]); assigned[i] = 1; --remaining;
                }
                break;
            }

            while (idx < all.size() && assigned[idx]) ++idx;
            if (idx >= all.size()) break;

            Rect mbr1 = n1->mbr;
            Rect mbr2 = n2->mbr;
            double area1 = mbr1.area();
            double area2 = mbr2.area();
            Rect expanded1 = mbr1.expandedWith(all[idx].mbr);
            Rect expanded2 = mbr2.expandedWith(all[idx].mbr);
            double inc1 = expanded1.area() - area1;
            double inc2 = expanded2.area() - area2;

            bool assignTo1 = false;
            if (inc1 < inc2) assignTo1 = true;
            else if (inc2 < inc1) assignTo1 = false;
            else {
                if (area1 < area2) assignTo1 = true;
                else if (area2 < area1) assignTo1 = false;
                else {
                    assignTo1 = (n1->size() <= n2->size());
                }
            }

            if (assignTo1) {
                n1->entries.push_back(all[idx]);
                n1->updateMBR();
            } else {
                n2->entries.push_back(all[idx]);
                n2->updateMBR();
            }
            assigned[idx] = 1;
            --remaining;
            while (idx < all.size() && assigned[idx]) ++idx;
        }

        for (auto &e : n1->entries) if (e.child) e.child->parent = n1;
        for (auto &e : n2->entries) if (e.child) e.child->parent = n2;

        n1->updateMBR();
        n2->updateMBR();

        delete node;

        return {n1, n2};
    }

    std::vector<int> search(const Rect& query, long long& nodes_visited) const {
        std::vector<int> result;
        nodes_visited = 0;
        if (!root) return result;

        std::vector<Node*> stack;
        stack.push_back(root);

        while (!stack.empty()) {
            Node* n = stack.back(); stack.pop_back();
            ++nodes_visited;
            if (!n) continue;
            if (n->is_leaf) {
                for (auto &e : n->entries) {
                    if (e.point_id != -1) {
                        if (query.contains(e.mbr.min_x, e.mbr.min_y)) {
                            result.push_back(e.point_id);
                        }
                    }
                }
            } else {
                for (auto &e : n->entries) {
                    if (query.intersects(e.mbr) && e.child) {
                        stack.push_back(e.child);
                    }
                }
            }
        }

        return result;
    }

private:
    void insertPointInternal(int point_id, const Rect& r, int target_level) {
        Node* leaf = chooseLeaf(root, r);
        Node::Entry e;
        e.mbr = r;
        e.child = nullptr;
        e.point_id = point_id;
        leaf->entries.push_back(e);
        leaf->updateMBR();
        adjustTree(leaf, target_level);
    }

    void insertPointAtLevel(int point_id, double x, double y, int target_level) {
        Rect r(x, y, x, y);
        insertPointInternal(point_id, r, target_level);
    }

    void insertSubtreeAtLevel(Node* subtree, const Rect& subtreeMbr, int target_level) {
        if (!subtree) return;
        Node* destination = chooseNodeAtLevel(root, subtreeMbr, nodeLevel(root), target_level);
        if (!destination) return;

        Node::Entry entry;
        entry.mbr = subtree->mbr;
        entry.child = subtree;
        entry.point_id = -1;
        subtree->parent = destination;
        destination->entries.push_back(entry);
        destination->updateMBR();
        adjustTree(destination, target_level);
    }

    Node* chooseNodeAtLevel(Node* node, const Rect& r, int current_level, int target_level) {
        if (!node) return nullptr;
        if (current_level == target_level) return node;
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
        return chooseNodeAtLevel(bestEntry->child, r, current_level - 1, target_level);
    }

    int nodeLevel(Node* node) const {
        if (!node) return 0;
        int level = 0;
        Node* current = node;
        while (current && !current->is_leaf && !current->entries.empty() && current->entries.front().child) {
            current = current->entries.front().child;
            ++level;
        }
        return level;
    }

    void ensureLevelFlag(int level) {
        if (level < 0) return;
        if (static_cast<int>(level_reinserted.size()) <= level) {
            level_reinserted.resize(static_cast<size_t>(level) + 1, false);
        }
    }

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
