#pragma once

#include <vector>

#include "rect.hpp"

class Node {
public:
    struct Entry {
        Rect mbr;
        Node* child = nullptr;
        int point_id = -1;
    };

    bool is_leaf = true;
    std::vector<Entry> entries;
    Node* parent = nullptr;
    Rect mbr;

    void updateMBR() {
        if (entries.empty()) {
            mbr = Rect();
            return;
        }

        Rect combined = entries.front().mbr;
        for (std::size_t i = 1; i < entries.size(); ++i) {
            combined = combined.expandedWith(entries[i].mbr);
        }
        mbr = combined;
    }

    bool isFull(int max_entries) const {
        return size() >= max_entries;
    }

    int size() const {
        return static_cast<int>(entries.size());
    }
};
