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
            Node* parent = node->parent;
            node->updateMBR();
            if (node->isFull(max_entries)) {
                if (parent != nullptr) {
                    ensureLevelFlag(level);
                    if (!level_reinserted[level]) {
                        forcedReinsert(node, level);
                        node = parent;
                        ++level;
                        continue;
                    }
                }

                auto splitResult = split(node);
                Node* n1 = splitResult.first;
                Node* n2 = splitResult.second;

                if (n1 == nullptr && n2 == nullptr) {
                    node = parent;
                    continue;
                }

                if (parent == nullptr) {
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
                    Node::Entry newEntry; newEntry.mbr = n2->mbr; newEntry.child = n2; newEntry.point_id = -1;
                    n2->parent = parent;
                    int targetIndex = -1;
                    for (size_t i = 0; i < parent->entries.size(); ++i) {
                        if (parent->entries[i].child == node) {
                            targetIndex = static_cast<int>(i);
                            break;
                        }
                    }
                    parent->entries.push_back(newEntry);
                    if (targetIndex >= 0) {
                        parent->entries[targetIndex].child = n1;
                        parent->entries[targetIndex].mbr = n1->mbr;
                        n1->parent = parent;
                    }

                    node = parent;
                    ++level;
                    continue;
                }
            } else {
                node = parent;
                ++level;
            }
        }
    }

    std::pair<Node*, Node*> split(Node* node) {
        if (!node) return {nullptr, nullptr};

        std::vector<Node::Entry> all;
        all.reserve(node->entries.size());
        for (const auto& entry : node->entries) {
            all.push_back(entry);
        }

        auto axisMin = [](const Node::Entry& entry, int axis) {
            return axis == 0 ? entry.mbr.min_x : entry.mbr.min_y;
        };
        auto axisMax = [](const Node::Entry& entry, int axis) {
            return axis == 0 ? entry.mbr.max_x : entry.mbr.max_y;
        };

        struct DistributionResult {
            bool valid = false;
            int splitIndex = -1;
            double overlap = std::numeric_limits<double>::infinity();
            double area = std::numeric_limits<double>::infinity();
        };

        auto evaluateDistribution = [&](const std::vector<Node::Entry>& sorted) {
            DistributionResult best;
            const int total = static_cast<int>(sorted.size());
            const int minGroup = min_entries;
            const int maxFirstGroup = max_entries - min_entries + 1;
            const int distributions = max_entries - 2 * min_entries + 2;

            for (int d = 0; d < distributions; ++d) {
                const int firstGroupSize = minGroup + d;
                if (firstGroupSize < minGroup || firstGroupSize > maxFirstGroup) {
                    continue;
                }
                const int secondGroupSize = total - firstGroupSize;
                if (secondGroupSize < minGroup) {
                    continue;
                }

                Rect mbr1 = sorted[0].mbr;
                for (int i = 1; i < firstGroupSize; ++i) {
                    mbr1 = mbr1.expandedWith(sorted[i].mbr);
                }

                Rect mbr2 = sorted[firstGroupSize].mbr;
                for (int i = firstGroupSize + 1; i < total; ++i) {
                    mbr2 = mbr2.expandedWith(sorted[i].mbr);
                }

                const double overlap = mbr1.overlapArea(mbr2);
                const double area = mbr1.area() + mbr2.area();

                if (!best.valid || overlap < best.overlap || (overlap == best.overlap && area < best.area)) {
                    best.valid = true;
                    best.splitIndex = firstGroupSize;
                    best.overlap = overlap;
                    best.area = area;
                }
            }

            return best;
        };

        struct AxisChoice {
            bool valid = false;
            int axis = 0;
            bool sortByMin = true;
            double goodness = std::numeric_limits<double>::infinity();
            DistributionResult distribution;
        };

        AxisChoice bestChoice;

        for (int axis = 0; axis < 2; ++axis) {
            double bestAxisGoodness = std::numeric_limits<double>::infinity();
            AxisChoice bestAxisChoice;

            for (bool sortByMin : {true, false}) {
                std::vector<Node::Entry> sorted = all;
                std::sort(sorted.begin(), sorted.end(), [&](const Node::Entry& lhs, const Node::Entry& rhs) {
                    const double lhsBound = sortByMin ? axisMin(lhs, axis) : axisMax(lhs, axis);
                    const double rhsBound = sortByMin ? axisMin(rhs, axis) : axisMax(rhs, axis);
                    if (lhsBound != rhsBound) return lhsBound < rhsBound;
                    const double lhsOther = sortByMin ? axisMax(lhs, axis) : axisMin(lhs, axis);
                    const double rhsOther = sortByMin ? axisMax(rhs, axis) : axisMin(rhs, axis);
                    return lhsOther < rhsOther;
                });

                const int distributions = max_entries - 2 * min_entries + 2;
                double goodness = 0.0;
                for (int d = 0; d < distributions; ++d) {
                    const int firstGroupSize = min_entries + d;
                    const int secondGroupSize = static_cast<int>(sorted.size()) - firstGroupSize;
                    if (firstGroupSize < min_entries || secondGroupSize < min_entries) {
                        continue;
                    }

                    Rect mbr1 = sorted[0].mbr;
                    for (int i = 1; i < firstGroupSize; ++i) {
                        mbr1 = mbr1.expandedWith(sorted[i].mbr);
                    }

                    Rect mbr2 = sorted[firstGroupSize].mbr;
                    for (int i = firstGroupSize + 1; i < static_cast<int>(sorted.size()); ++i) {
                        mbr2 = mbr2.expandedWith(sorted[i].mbr);
                    }

                    goodness += mbr1.perimeter() + mbr2.perimeter();
                }

                if (goodness < bestAxisGoodness) {
                    bestAxisGoodness = goodness;
                    bestAxisChoice.valid = true;
                    bestAxisChoice.axis = axis;
                    bestAxisChoice.sortByMin = sortByMin;
                    bestAxisChoice.goodness = goodness;
                    bestAxisChoice.distribution = evaluateDistribution(sorted);
                }
            }

            if (bestAxisChoice.valid && (!bestChoice.valid || bestAxisChoice.goodness < bestChoice.goodness)) {
                bestChoice = bestAxisChoice;
            }
        }

        if (!bestChoice.valid || !bestChoice.distribution.valid) {
            return {nullptr, nullptr};
        }

        std::vector<Node::Entry> sorted = all;
        std::sort(sorted.begin(), sorted.end(), [&](const Node::Entry& lhs, const Node::Entry& rhs) {
            const double lhsBound = bestChoice.sortByMin ? axisMin(lhs, bestChoice.axis) : axisMax(lhs, bestChoice.axis);
            const double rhsBound = bestChoice.sortByMin ? axisMin(rhs, bestChoice.axis) : axisMax(rhs, bestChoice.axis);
            if (lhsBound != rhsBound) return lhsBound < rhsBound;
            const double lhsOther = bestChoice.sortByMin ? axisMax(lhs, bestChoice.axis) : axisMin(lhs, bestChoice.axis);
            const double rhsOther = bestChoice.sortByMin ? axisMax(rhs, bestChoice.axis) : axisMin(rhs, bestChoice.axis);
            return lhsOther < rhsOther;
        });

        const int splitIndex = bestChoice.distribution.splitIndex;
        if (splitIndex <= 0 || splitIndex >= static_cast<int>(sorted.size())) {
            return {nullptr, nullptr};
        }

        // Reuse the original node as the first group to avoid freeing it
        // (which previously caused use-after-free / heap corruption).
        Node* first = node;
        Node* second = new Node();
        first->is_leaf = node->is_leaf;
        second->is_leaf = node->is_leaf;
        first->entries.clear();

        for (int i = 0; i < splitIndex; ++i) {
            first->entries.push_back(sorted[i]);
        }
        for (int i = splitIndex; i < static_cast<int>(sorted.size()); ++i) {
            second->entries.push_back(sorted[i]);
        }

        for (auto& entry : first->entries) {
            if (entry.child) {
                entry.child->parent = first;
            }
        }
        for (auto& entry : second->entries) {
            if (entry.child) {
                entry.child->parent = second;
            }
        }

        first->updateMBR();
        second->updateMBR();

        return {first, second};
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
