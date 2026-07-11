#include "experiment1.hpp"

#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <random>
#include <string>
#include <utility>
#include <vector>

#include "../include/dataset.hpp"
#include "../include/rtree.hpp"
#include "../include/rstar.hpp"

double elapsed(bool reset);
double elapsed();

namespace {

struct Bounds {
    double min_x;
    double min_y;
    double max_x;
    double max_y;
};

Bounds computeBounds(const std::vector<std::pair<double, double>>& points) {
    Bounds bounds{0.0, 0.0, 0.0, 0.0};
    if (points.empty()) {
        return bounds;
    }

    bounds.min_x = bounds.max_x = points.front().first;
    bounds.min_y = bounds.max_y = points.front().second;

    for (const auto& point : points) {
        bounds.min_x = std::min(bounds.min_x, point.first);
        bounds.min_y = std::min(bounds.min_y, point.second);
        bounds.max_x = std::max(bounds.max_x, point.first);
        bounds.max_y = std::max(bounds.max_y, point.second);
    }

    return bounds;
}

std::vector<Rect> generateQueries(const Bounds& bounds, int count, unsigned seed) {
    std::vector<Rect> queries;
    if (count <= 0) {
        return queries;
    }

    std::mt19937 generator(seed);
    std::uniform_real_distribution<double> aspect_distribution(0.5, 2.0);

    const double width_space = std::max(1e-9, bounds.max_x - bounds.min_x);
    const double height_space = std::max(1e-9, bounds.max_y - bounds.min_y);
    const double total_area = width_space * height_space;
    const double query_area = std::max(1e-9, total_area * 0.05);

    queries.reserve(static_cast<size_t>(count));
    for (int i = 0; i < count; ++i) {
        double query_width = std::sqrt(query_area * aspect_distribution(generator));
        double query_height = query_area / query_width;

        if (query_width > width_space) {
            query_width = width_space;
            query_height = query_area / query_width;
        }
        if (query_height > height_space) {
            query_height = height_space;
            query_width = query_area / query_height;
        }

        const double max_x_start = std::max(bounds.min_x, bounds.max_x - query_width);
        const double max_y_start = std::max(bounds.min_y, bounds.max_y - query_height);

        std::uniform_real_distribution<double> x_distribution(bounds.min_x, max_x_start);
        std::uniform_real_distribution<double> y_distribution(bounds.min_y, max_y_start);

        const double x1 = x_distribution(generator);
        const double y1 = y_distribution(generator);
        queries.emplace_back(x1, y1, x1 + query_width, y1 + query_height);
    }

    return queries;
}

template <typename TreeType>
void runTreeExperiment(const std::string& distribution_name,
                       const std::string& tree_name,
                       const std::vector<std::pair<double, double>>& points,
                       const std::vector<Rect>& queries,
                       std::ofstream& out) {
    TreeType tree(8);

    elapsed(true);
    for (size_t i = 0; i < points.size(); ++i) {
        tree.insert(static_cast<int>(i), points[i].first, points[i].second);
    }
    const double insertion_ms = elapsed();

    double search_time_sum_ms = 0.0;
    long long visited_sum = 0;
    for (const auto& query : queries) {
        long long nodes_visited = 0;
        elapsed(true);
        (void)tree.search(query, nodes_visited);
        search_time_sum_ms += elapsed();
        visited_sum += nodes_visited;
    }

    const double search_avg_ms = queries.empty() ? 0.0 : search_time_sum_ms / static_cast<double>(queries.size());
    const double visited_avg = queries.empty() ? 0.0 : static_cast<double>(visited_sum) / static_cast<double>(queries.size());

    out << distribution_name << ','
        << tree_name << ','
        << std::fixed << std::setprecision(6)
        << insertion_ms << ','
        << search_avg_ms << ','
        << visited_avg << '\n';
}

}

void runExperiment1() {
    std::filesystem::create_directories("results");
    const std::string output_path = "results/experimento1.csv";

    std::ofstream out(output_path, std::ios::trunc);
    if (!out.is_open()) {
        return;
    }

    out << "distribucion,arbol,tiempo_insercion_ms,tiempo_busqueda_prom_ms,nodos_visitados_prom\n";

    const int n_points = 5000;
    const int num_queries = 100;
    const unsigned dataset_seed = 42u;
    const unsigned query_seed = 20240610u;
    const double area_min = 0.0;
    const double area_max = 1000.0;

    const auto uniform_points = generateUniform(n_points, area_min, area_max, dataset_seed);
    const auto clustered_points = generateClustered(n_points, 8, area_max, 25.0, dataset_seed);

    const auto uniform_queries = generateQueries(computeBounds(uniform_points), num_queries, query_seed);
    const auto clustered_queries = generateQueries(computeBounds(clustered_points), num_queries, query_seed);

    runTreeExperiment<RTree>("uniforme", "RTree", uniform_points, uniform_queries, out);
    runTreeExperiment<RStarTree>("uniforme", "RStarTree", uniform_points, uniform_queries, out);
    runTreeExperiment<RTree>("agrupada", "RTree", clustered_points, clustered_queries, out);
    runTreeExperiment<RStarTree>("agrupada", "RStarTree", clustered_points, clustered_queries, out);
}
