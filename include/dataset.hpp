#pragma once

#include <algorithm>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

inline std::vector<std::pair<double, double>> generateUniform(int n, double min_coord, double max_coord, unsigned seed) {
    std::vector<std::pair<double, double>> points;
    if (n <= 0) {
        return points;
    }

    if (max_coord < min_coord) {
        std::swap(min_coord, max_coord);
    }

    std::mt19937 generator(seed);
    std::uniform_real_distribution<double> distribution(min_coord, max_coord);

    points.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i) {
        points.emplace_back(distribution(generator), distribution(generator));
    }
    return points;
}

inline std::vector<std::pair<double, double>> generateClustered(int n, int num_clusters, double area_size, double cluster_std, unsigned seed) {
    std::vector<std::pair<double, double>> points;
    if (n <= 0 || num_clusters <= 0) {
        return points;
    }

    if (area_size < 0.0) {
        area_size = -area_size;
    }
    if (cluster_std < 0.0) {
        cluster_std = -cluster_std;
    }

    std::mt19937 generator(seed);
    std::uniform_real_distribution<double> center_distribution(0.0, area_size);
    std::normal_distribution<double> point_distribution(0.0, cluster_std);

    std::vector<std::pair<double, double>> centers;
    centers.reserve(static_cast<size_t>(num_clusters));
    for (int i = 0; i < num_clusters; ++i) {
        centers.emplace_back(center_distribution(generator), center_distribution(generator));
    }

    points.reserve(static_cast<size_t>(n));
    const int base_points_per_cluster = n / num_clusters;
    const int remainder = n % num_clusters;

    for (int cluster = 0; cluster < num_clusters; ++cluster) {
        const int points_in_cluster = base_points_per_cluster + (cluster < remainder ? 1 : 0);
        const double center_x = centers[cluster].first;
        const double center_y = centers[cluster].second;

        for (int i = 0; i < points_in_cluster; ++i) {
            points.emplace_back(center_x + point_distribution(generator), center_y + point_distribution(generator));
        }
    }

    return points;
}

inline bool saveDatasetCsv(const std::string& file_path, const std::vector<std::pair<double, double>>& dataset) {
    std::ofstream out(file_path);
    if (!out.is_open()) {
        return false;
    }

    out << "id,x,y\n";
    for (size_t i = 0; i < dataset.size(); ++i) {
        out << i << ',' << dataset[i].first << ',' << dataset[i].second << '\n';
    }

    return static_cast<bool>(out);
}

inline std::vector<std::pair<double, double>> loadDatasetCsv(const std::string& file_path) {
    std::vector<std::pair<double, double>> dataset;
    std::ifstream in(file_path);
    if (!in.is_open()) {
        return dataset;
    }

    std::string line;
    while (std::getline(in, line)) {
        if (line.empty()) {
            continue;
        }

        std::stringstream stream(line);
        std::string id_token;
        std::string x_token;
        std::string y_token;

        if (!std::getline(stream, id_token, ',')) {
            continue;
        }
        if (!std::getline(stream, x_token, ',')) {
            continue;
        }
        if (!std::getline(stream, y_token, ',')) {
            continue;
        }

        if (id_token == "id" && x_token == "x" && y_token == "y") {
            continue;
        }

        try {
            const double x = std::stod(x_token);
            const double y = std::stod(y_token);
            dataset.emplace_back(x, y);
        } catch (...) {
            continue;
        }
    }

    return dataset;
}
