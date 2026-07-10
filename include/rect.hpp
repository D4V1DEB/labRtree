#pragma once

#include <algorithm>

class Rect {
public:
    double min_x;
    double min_y;
    double max_x;
    double max_y;

    Rect() : min_x(0.0), min_y(0.0), max_x(-1.0), max_y(-1.0) {}

    Rect(double minX, double minY, double maxX, double maxY) : min_x(minX), min_y(minY), max_x(maxX), max_y(maxY) {}

    double area() const {
        if (isEmpty()) {
            return 0.0;
        }
        return (max_x - min_x) * (max_y - min_y);
    }

    bool intersects(const Rect& other) const {
        if (isEmpty() || other.isEmpty()) {
            return false;
        }
        return !(max_x < other.min_x || other.max_x < min_x || max_y < other.min_y || other.max_y < min_y);
    }

    bool contains(double px, double py) const {
        if (isEmpty()) {
            return false;
        }
        return px >= min_x && px <= max_x && py >= min_y && py <= max_y;
    }

    Rect expandedWith(const Rect& other) const {
        if (isEmpty()) {
            return other;
        }
        if (other.isEmpty()) {
            return *this;
        }
        return Rect(
            std::min(min_x, other.min_x),
            std::min(min_y, other.min_y),
            std::max(max_x, other.max_x),
            std::max(max_y, other.max_y)
        );
    }

    double overlapArea(const Rect& other) const {
        if (!intersects(other)) {
            return 0.0;
        }

        const double overlapMinX = std::max(min_x, other.min_x);
        const double overlapMinY = std::max(min_y, other.min_y);
        const double overlapMaxX = std::min(max_x, other.max_x);
        const double overlapMaxY = std::min(max_y, other.max_y);

        const double overlapWidth = std::max(0.0, overlapMaxX - overlapMinX);
        const double overlapHeight = std::max(0.0, overlapMaxY - overlapMinY);
        return overlapWidth * overlapHeight;
    }

    double perimeter() const {
        if (isEmpty()) {
            return 0.0;
        }
        return 2.0 * ((max_x - min_x) + (max_y - min_y));
    }

private:
    bool isEmpty() const {
        return max_x < min_x || max_y < min_y;
    }
};
