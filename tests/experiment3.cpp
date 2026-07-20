#include "experiment3.hpp"

#include <fstream>
#include <iomanip>
#include <string>
#include <vector>

#include "../include/dataset.hpp"
#include "../include/rtree.hpp"
#include "../include/rstar.hpp"

double elapsed(bool reset);
double elapsed();

namespace {

std::vector<Rect> generateQueries(const std::vector<std::pair<double, double>>& points,
                                  int count,
                                  unsigned seed) {
    std::vector<Rect> queries;
    if (count <= 0 || points.empty()) {
        return queries;
    }

    double min_x = points.front().first;
    double min_y = points.front().second;
    double max_x = min_x;
    double max_y = min_y;
    for (const auto& p : points) {
        min_x = std::min(min_x, p.first);
        min_y = std::min(min_y, p.second);
        max_x = std::max(max_x, p.first);
        max_y = std::max(max_y, p.second);
    }

    std::mt19937 generator(seed);
    std::uniform_real_distribution<double> aspect_distribution(0.5, 2.0);

    const double width_space = std::max(1e-9, max_x - min_x);
    const double height_space = std::max(1e-9, max_y - min_y);
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

        const double max_x_start = std::max(min_x, max_x - query_width);
        const double max_y_start = std::max(min_y, max_y - query_height);

        std::uniform_real_distribution<double> x_distribution(min_x, max_x_start);
        std::uniform_real_distribution<double> y_distribution(min_y, max_y_start);

        const double x1 = x_distribution(generator);
        const double y1 = y_distribution(generator);
        queries.emplace_back(x1, y1, x1 + query_width, y1 + query_height);
    }

    return queries;
}

template <typename TreeType>
void runTreeExperiment(int max_entries,
                      const std::string& tree_name,
                      const std::vector<std::pair<double, double>>& points,
                      const std::vector<Rect>& queries,
                      std::ofstream& out) {
    TreeType tree(max_entries);

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
    const int height = tree.computeHeight();

    out << max_entries << ','
        << tree_name << ','
        << std::fixed << std::setprecision(6)
        << insertion_ms << ','
        << search_avg_ms << ','
        << visited_avg << ','
        << height << '\n';
}

} // namespace

void runExperiment3() {
    std::ofstream out("results/experimento3.csv", std::ios::trunc);
    if (!out.is_open()) {
        return;
    }

    out << "max_entries,arbol,tiempo_insercion_ms,tiempo_busqueda_prom_ms,nodos_visitados_prom,altura\n";

    const int n_points = 5000;
    const int num_queries = 100;
    const unsigned dataset_seed = 42u;
    const unsigned query_seed = 20240610u;
    const double area_min = 0.0;
    const double area_max = 1000.0;

    const std::vector<int> max_entries_values = {4, 8, 16, 32};

    for (int max_entries : max_entries_values) {
        const auto points = generateUniform(n_points, area_min, area_max, dataset_seed);
        const auto queries = generateQueries(points, num_queries, query_seed);

        runTreeExperiment<RTree>(max_entries, "RTree", points, queries, out);
        runTreeExperiment<RStarTree>(max_entries, "RStarTree", points, queries, out);
    }
}
