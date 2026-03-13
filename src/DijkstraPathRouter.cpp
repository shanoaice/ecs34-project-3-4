#include "DijkstraPathRouter.h"
#include "DijkstraPathRouter_p.h"

#include <algorithm>
#include <any>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <numeric>
#include <queue>
#include <random>
#include <stdexcept>
#include <stop_token>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

struct CDijkstraPathRouter::SImplementation {

    // using adjacency_list_t =
    //     std::unordered_map<TVertexID, std::unordered_map<TVertexID, double>>;

    using TVertexIndex = std::size_t;
    using vertices_t =
        std::unordered_map<TVertexID, std::pair<TVertexIndex, std::any>>;

    using min_vertex_pq =
        std::priority_queue<std::pair<double, TVertexIndex>,
                            std::vector<std::pair<double, TVertexIndex>>,
                            std::greater<>>;

    struct SDijkstraSearchState {
        // std::unordered_map<TVertexID, TVertexID> path;
        std::vector<double> distance;
        min_vertex_pq pq;
        // std::unordered_set<TVertexID> visited;
        bool started = false;

        SDijkstraSearchState() = delete;
        SDijkstraSearchState(const std::size_t vertex_count)
            : distance(vertex_count, std::numeric_limits<double>::infinity()) {}
    };

    struct SEdge {
        std::size_t u_by_idx;
        std::size_t v_by_idx;
        double weight;

        std::weak_ordering operator<=>(const SEdge &rhs) const {
            if (const auto cmp = u_by_idx <=> rhs.u_by_idx;
                cmp != std::strong_ordering::equal)
                return cmp;
            if (const auto cmp = v_by_idx <=> rhs.v_by_idx;
                cmp != std::strong_ordering::equal)
                return cmp;

            if (const auto lhs_nan = std::isnan(this->weight),
                rhs_nan = std::isnan(rhs.weight);
                !(lhs_nan || rhs_nan)) {
                if (weight < rhs.weight)
                    return std::weak_ordering::less;
                if (weight > rhs.weight)
                    return std::weak_ordering::greater;
                return std::weak_ordering::equivalent;
            } else {
                // treat NaNs as equivalent and sort NaNs to the end if not both
                if (lhs_nan && rhs_nan)
                    return std::weak_ordering::equivalent;
                return lhs_nan ? std::weak_ordering::greater
                               : std::weak_ordering::less;
            }
        }

        bool operator==(const SEdge &) const = default;
    };

    std::random_device rng_source;
    std::seed_seq seed{rng_source(), rng_source(), rng_source(), rng_source(),
                       rng_source()};
    std::mt19937_64 rng{seed};

    std::vector<TVertexID> index_to_id;
    vertices_t vertices;
    // adjacency_list_t adjacency_list;
    // adjacency_list_t reverse_adjacency_list;

    std::vector<std::size_t> adj_matrix_row_ptr;
    std::vector<std::size_t> adj_matrix_col_idx;
    std::vector<double> adj_matrix_weight;

    std::vector<std::size_t> rev_adj_matrix_row_ptr;
    std::vector<std::size_t> rev_adj_matrix_col_idx;
    std::vector<double> rev_adj_matrix_weight;

    std::vector<SEdge> raw_edges;

    static constexpr double epsilon = 1.0e-6;
    static constexpr double double_inf =
        std::numeric_limits<double>::infinity();

    bool is_graph_dirty = false;

    // This map's key is the source vertex id
    // value is a pointer to the struct storing the Dijkstra search state on the
    // source vertex
    std::unordered_map<TVertexID, std::shared_ptr<SDijkstraSearchState>>
        search_states;

    // state object is wrapped in shared_ptr because they might have to
    // be sent across threads in precompute. We don't need a lock because no two
    // threads will work on the same source vertex the same time. Vertices,
    // vertex_map & connection map also does not have to be locked, because they
    // are read-only in Precompute

    TVertexIndex IndexOf(const TVertexID id) const {
        const auto it = vertices.find(id);
        if (it == vertices.cend()) {
            throw std::out_of_range("unknown vertex id");
        }
        const auto idx = it->second.first;
        if (idx >= vertices.size()) {
            throw std::out_of_range("corrupt vertex index");
        }
        return idx;
    }

    // https://gemini.google.com/app/bbcdec1fd5399bca
    // I don't want that much hassle to implement one myself. This is just math.
    static bool isClose(const double a, const double b,
                        const double relTol = 1e-5,
                        const double absTol = epsilon) {
        if (!std::isfinite(a) || !std::isfinite(b)) {
            return false;
        }

        // 1. Check absolute difference first (crucial for numbers near zero)
        const double diff = std::abs(a - b);
        if (diff <= absTol) {
            return true;
        }

        // 2. Check relative difference (scales based on the largest input)
        const double largest = std::max(std::abs(a), std::abs(b));
        return diff <= (largest * relTol);
    }

    std::size_t VertexCount() const noexcept { return vertices.size(); }

    TVertexID AddVertex(std::any tag) noexcept {
        TVertexID id;

        if (tag.type() == typeid(SUniqueIdWithData)) {
            // if you pass in a SUniqueIdWithData, the node id is guaranteed to
            // be unique. so, we just reuse it. This avoids extra mapping needed
            // in DijkstraTransportationPlanner
            id = std::any_cast<SUniqueIdWithData>(tag).uniqueId;
        } else {
            // make sure vertex id is unique
            do {
                id = rng();
            } while (vertices.contains(id));
        }

        // now we should mark graph dirty when we insert a vertex, because new
        // vertices, albeit unreachable by default, will change CSR
        // representation
        is_graph_dirty = true;

        if (const auto existing = vertices.find(id);
            existing != vertices.cend()) {
            existing->second.second = std::move(tag);
            return id;
        }

        const auto new_index = vertices.size();
        index_to_id.emplace_back(id);
        vertices.insert_or_assign(id, std::pair{new_index, std::move(tag)});

        // for all nodes that have a state, resize the dense distance vector
        for (const auto &[_, vertex_state] : search_states) {
            vertex_state->distance.emplace_back(double_inf);
        }

        return id;
    }

    bool AddEdge(const TVertexID src, const TVertexID dest, const double weight,
                 const bool bidir) noexcept {
        if (src == dest)
            return true;

        if (!vertices.contains(src) || !vertices.contains(dest))
            return false;

        if (weight < 0)
            return false;

        // update state flag
        is_graph_dirty = true;

        raw_edges.emplace_back(IndexOf(src), IndexOf(dest), weight);

        if (bidir) {
            raw_edges.emplace_back(IndexOf(dest), IndexOf(src), weight);
        }

        return true;
    }

    void BuildCSR() noexcept {
        if (!is_graph_dirty) {
            return;
        }

        // wipe everything because graph is dirty and we reconstruct
        rev_adj_matrix_row_ptr.clear();
        rev_adj_matrix_row_ptr.resize(vertices.size() + 1);
        rev_adj_matrix_col_idx.clear();
        rev_adj_matrix_weight.clear();

        adj_matrix_row_ptr.clear();
        adj_matrix_row_ptr.resize(vertices.size() + 1);
        adj_matrix_col_idx.clear();
        adj_matrix_weight.clear();

        std::ranges::sort(raw_edges);

        // because it is sorted in non-descending order, any duplicates after
        // always have a larger weight. just throw them away
        auto unique_tail =
            std::ranges::unique(raw_edges, [](const SEdge &a, const SEdge &b) {
                return a.u_by_idx == b.u_by_idx && a.v_by_idx == b.v_by_idx;
            });
        raw_edges.erase(unique_tail.begin(), unique_tail.end());

        // pre-reserve E for these two after dedup
        adj_matrix_col_idx.reserve(raw_edges.size());
        adj_matrix_weight.reserve(raw_edges.size());

        // reverse matrix uses scatter fill so we need to resize instead
        rev_adj_matrix_col_idx.resize(raw_edges.size());
        rev_adj_matrix_weight.resize(raw_edges.size());

        std::vector<std::size_t> indegree(vertices.size(), 0);
        for (const auto &edge : raw_edges) {
            ++indegree[edge.v_by_idx];
        }

        std::vector<std::size_t> outdegree(vertices.size(), 0);
        for (const auto &edge : raw_edges) {
            ++outdegree[edge.u_by_idx];
        }

        // prefix sum
        std::partial_sum(indegree.cbegin(), indegree.cend(),
                         rev_adj_matrix_row_ptr.begin() + 1);
        rev_adj_matrix_row_ptr[0] = 0;

        std::partial_sum(outdegree.cbegin(), outdegree.cend(),
                         adj_matrix_row_ptr.begin() + 1);
        adj_matrix_row_ptr[0] = 0;

        auto reverse_cursor = rev_adj_matrix_row_ptr;

        for (const auto &[u_by_idx, v_by_idx, weight] : raw_edges) {

            adj_matrix_col_idx.emplace_back(v_by_idx);
            adj_matrix_weight.emplace_back(weight);

            // scatter populate reverse CSR because edge is sorted in forward order
            const auto pos = reverse_cursor[v_by_idx];
            rev_adj_matrix_col_idx[pos] = u_by_idx;
            rev_adj_matrix_weight[pos] = weight;
            ++reverse_cursor[v_by_idx];
        }
    }

    std::any GetVertexTag(const TVertexID id) const noexcept {
        const auto it = vertices.find(id);
        return it == vertices.cend() ? std::any() : it->second.second;
    }

    double FindShortestPath(const TVertexID src, const TVertexID dest,
                            std::vector<TVertexID> &path) {
        path.clear();

        // if source is equal to dest, path only contains source or dest
        // and distance is 0.0
        if (src == dest) {
            path.emplace_back(src);
            return 0.0;
        }

        // if any of the specified vertices does not exist, we bail out
        if (!vertices.contains(src) || !vertices.contains(dest)) {
            return NoPathExists;
        }

        // if the graph is not changed (marked dirty by AddEdge) and the dest
        // vertex is visited, we had searched it, and we just backtrack the path
        // and returns the calculated value. we could cache the path too and
        // completely eliminate any extra calculation, but that is costly on
        // space and I doubt this will be a hot path
        if (!is_graph_dirty && search_states.contains(src) &&
            search_states[src]->distance[IndexOf(dest)] != double_inf &&
            search_states[src]->pq.empty()) {
            const auto &dist = search_states[src]->distance;

            if (dist.size() != vertices.size()) {
                throw std::runtime_error("distance vector size mismatch");
            }

            path.emplace_back(IndexOf(dest));
            std::size_t hops = 0;
            const auto max_hops = vertices.size();
            const auto src_idx = IndexOf(src);
            while (path.back() != src_idx) {
                bool found_prev = false;
                const auto curr = path.back();
                const auto rev_begin = rev_adj_matrix_row_ptr[curr],
                           rev_end = rev_adj_matrix_row_ptr[curr + 1];

                for (auto i = rev_begin; i < rev_end; ++i) {
                    const auto v = rev_adj_matrix_col_idx[i];
                    const auto d = rev_adj_matrix_weight[i];
                    if (dist[curr] == double_inf || dist[v] == double_inf) {
                        continue;
                    }

                    if (isClose(dist[curr], dist[v] + d)) {
                        path.emplace_back(v);
                        found_prev = true;
                        break;
                    }
                }

#ifndef NDEBUG
                if (!found_prev) {
                    std::cerr << "Path reconstruction failed at vertex " << curr
                              << " pq size " << search_states[src]->pq.size()
                              << "\n";
                }
#endif

                if (!found_prev || ++hops > max_hops) {
                    path.clear();
                    return NoPathExists;
                }
            }

            for (auto &v_idx : path) {
                v_idx = index_to_id[v_idx];
            }

            std::ranges::reverse(path);
            return search_states[src]->distance[IndexOf(dest)];
        }

        // if the graph is marked dirty, we wipe all states and rebuild the CSR
        // if only we cannot find our state, we just insert a new state
        if (is_graph_dirty) {
            BuildCSR();
            search_states.clear();
            search_states[src] =
                std::make_shared<SDijkstraSearchState>(vertices.size());
        } else if (!search_states.contains(src)) {
            search_states[src] =
                std::make_shared<SDijkstraSearchState>(vertices.size());
        }

        // some QoL shorthands
        // auto &tracked_path = state[src]->path;
        auto &dist = search_states[src]->distance;
        auto &pq = search_states[src]->pq;
        // auto &visited = state[src]->visited;

        if (dist.size() != vertices.size()) {
            throw std::runtime_error("distance vector size mismatch");
        }

        if (pq.empty()) {
            if (search_states[src]->started) {
                // if it is started and queue is still empty, while the dest is
                // not calculated, it is unreachable. return NoPathExists
                return NoPathExists;
            }
            // otherwise, we insert the source and start the search
            auto src_index = IndexOf(src);
            pq.emplace(0, src_index);
            dist[src_index] = 0.0;
            search_states[src]->started = true;
        }

        while (!pq.empty()) {
            const auto u_weight = pq.top().first;
            const auto u = pq.top().second;
            pq.pop();

            // lazy delete -- if the path in priority_queue is not shorter than
            // the recorded distance, we skip it
            if (dist[u] < u_weight - epsilon)
                continue;

            // visited.insert(u);

            // now that we implemented precompute, it would be better to not
            // exit early, so that we save some space by not requiring a visited
            // set.
            //// if we reached the destination vertex, exit early
            // if (u == dest) {
            //     break;
            // }
            const auto adjacent_begin = adj_matrix_row_ptr[u],
                       adjacent_end = adj_matrix_row_ptr[u + 1];

            for (auto i = adjacent_begin; i < adjacent_end; ++i) {
                const auto v_index = adj_matrix_col_idx[i];
                const auto v_weight = adj_matrix_weight[i];

                const auto new_distance = dist[u] + v_weight;

                if (new_distance < dist[v_index] - epsilon) {
                    dist[v_index] = new_distance;
                    pq.emplace(new_distance, v_index);
                }
            }
        }

        is_graph_dirty = false;

        // if after Dijkstra we still don't have it in our dest, it's
        // unreachable. don't add anything to path and return NoPathExists
        if (dist[IndexOf(dest)] == double_inf) {
            return NoPathExists;
        }

        // reconstruct path from the tracked_path map
        path.emplace_back(IndexOf(dest));
        std::size_t hops = 0;
        const auto max_hops = vertices.size();
        const auto src_idx = IndexOf(src);
        while (path.back() != src_idx) {
            bool found_prev = false;
            const auto curr = path.back();
            const auto rev_begin = rev_adj_matrix_row_ptr[curr],
                       rev_end = rev_adj_matrix_row_ptr[curr + 1];

            for (auto i = rev_begin; i < rev_end; ++i) {
                const auto v = rev_adj_matrix_col_idx[i];
                const auto d = rev_adj_matrix_weight[i];
                if (dist[curr] == double_inf || dist[v] == double_inf) {
                    continue;
                }

                if (isClose(dist[curr], dist[v] + d)) {
                    path.emplace_back(v);
                    found_prev = true;
                    break;
                }
            }

#ifndef NDEBUG
            if (!found_prev) {
                static std::atomic<int> s_debug_count = 0;
                if (s_debug_count.fetch_add(1) < 30) {
                    std::size_t rev_degree = 0;
                    double best_abs_err = double_inf;

                    for (auto i = rev_begin; i < rev_end; ++i) {
                        const auto v = rev_adj_matrix_col_idx[i];
                        const auto d = rev_adj_matrix_weight[i];
                        const auto lhs = dist[curr];
                        const auto rhs = dist[v] + d;
                        best_abs_err =
                            std::min(best_abs_err, std::abs(lhs - rhs));
                    }

                    std::cerr
                        << "Path reconstruction failed (non-cached) src=" << src
                        << " dest=" << dest << " curr=" << curr
                        << " hops=" << hops << " pq_size=" << pq.size()
                        << " rev_degree=" << rev_degree
                        << " dist_curr=" << dist[curr]
                        << " best_abs_err=" << best_abs_err << "\n";
                }
            }
#endif

            if (!found_prev || ++hops > max_hops) {
                path.clear();
                return NoPathExists;
            }
        }

        // currently it is index stored in the path vector. map it back to ID
        for (auto &v_idx : path) {
            v_idx = index_to_id[v_idx];
        }

        // it is the other way round because we backtrack, reverse it
        std::ranges::reverse(path);

        // return the value
        return dist[IndexOf(dest)];
    }

    struct FShortPathPrecomputeWorker {
        const TVertexID src;

        std::shared_ptr<SDijkstraSearchState> state;

        const std::vector<std::size_t> &adj_matrix_row_ptr;
        const std::vector<std::size_t> &adj_matrix_col_idx;
        const std::vector<double> &adj_matrix_weight;

        const vertices_t &vertices;
        const std::stop_token &stop_token;

        TVertexIndex IndexOf(const TVertexID id) const {
            const auto it = vertices.find(id);
            if (it == vertices.cend()) {
                throw std::out_of_range("unknown vertex id");
            }
            const auto idx = it->second.first;
            if (idx >= vertices.size()) {
                throw std::out_of_range("corrupt vertex index");
            }
            return idx;
        }

        void operator()() const {
            if (!vertices.contains(src)) {
                return;
            }

            // some QoL shorthands
            // auto &tracked_path = state->path;
            auto &dist = state->distance;
            auto &pq = state->pq;
            // auto &visited = state->visited;

            if (dist.size() != vertices.size()) {
                throw std::runtime_error("distance vector size mismatch");
            }

            if (pq.empty()) {
                if (state->started) {
                    // if it is started and queue is still empty, we probably
                    // had done precomputing. exit.
                    return;
                }
                // otherwise, we insert the source and start the search
                const auto src_index = IndexOf(src);
                pq.emplace(0, src_index);
                dist[src_index] = 0.0;
                state->started = true;
            }

            while (!(pq.empty() || stop_token.stop_requested())) {
                const auto u_weight = pq.top().first;
                const auto u = pq.top().second;
                pq.pop();

                // lazy delete -- if the path in priority_queue is not shorter
                // than the recorded distance, we skip it
                if (dist[u] < u_weight - epsilon)
                    continue;

                // visited.insert(u);

                const auto adjacent_begin = adj_matrix_row_ptr[u],
                           adjacent_end = adj_matrix_row_ptr[u + 1];

                for (auto i = adjacent_begin; i < adjacent_end; ++i) {
                    const auto v_index = adj_matrix_col_idx[i];
                    const auto v_weight = adj_matrix_weight[i];

                    const auto new_distance = dist[u] + v_weight;

                    if (new_distance < dist[v_index] - epsilon) {
                        dist[v_index] = new_distance;
                        // tracked_path.insert_or_assign(v, u);
                        pq.emplace(new_distance, v_index);
                    }
                }
            }
        }
    };

    bool
    Precompute(const std::chrono::steady_clock::time_point deadline) noexcept {

        if (const auto time_available =
                deadline - std::chrono::steady_clock::now();
            time_available <= std::chrono::steady_clock::duration::zero()) {
            return false;
        }

        // use core count or 1
        const auto concurrency = []() {
            const auto hardware_concurrency =
                std::jthread::hardware_concurrency();
            return hardware_concurrency > 0 ? hardware_concurrency : 1;
        }();

        // wipe all states and rebuild CSR if graph is dirty
        if (is_graph_dirty) {
            BuildCSR();
            search_states.clear();
        }

        std::vector<TVertexID> pending_sources;

        // add all ids into pending_sources
        for (const auto &[id, _] : vertices) {
            pending_sources.emplace_back(id);
            if (!search_states.contains(id)) {
                search_states[id] =
                    std::make_shared<SDijkstraSearchState>(vertices.size());
            }
        }

        // if nothing to precompute, return
        if (pending_sources.empty()) {
            is_graph_dirty = false;
            return true;
        }

        std::atomic<std::size_t> next_index{0};
        std::atomic<std::size_t> finish_count{0};

        std::vector<std::jthread> threads;
        threads.reserve(concurrency);

        auto worker_fn = [&](const std::stop_token &stop_token) {
            while (!stop_token.stop_requested()) {
                const auto idx = next_index.fetch_add(1);

                if (idx >= pending_sources.size())
                    return;

                auto src = pending_sources[idx];

                FShortPathPrecomputeWorker{
                    .src = src,
                    .state = search_states[src],
                    .adj_matrix_row_ptr = adj_matrix_row_ptr,
                    .adj_matrix_col_idx = adj_matrix_col_idx,
                    .adj_matrix_weight = adj_matrix_weight,
                    .vertices = vertices,
                    .stop_token = stop_token}();

                ++finish_count;
            }
        };

        for (std::size_t i = 0; i < concurrency; ++i) {
            threads.emplace_back(worker_fn);
        }

        while (std::chrono::steady_clock::now() < deadline) {
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            if (finish_count.load() == pending_sources.size()) {
                is_graph_dirty = false;
                return true;
            }
        }

        // we always set graph clean even on timeout, because the main algorithm
        // is designed as resumable. This is just to make sure CSR and state
        // aligns to our current graph structure
        is_graph_dirty = false;

        for (auto &thread : threads) {
            thread.request_stop();
        }

        return false;
    }
};

CDijkstraPathRouter::CDijkstraPathRouter() {
    DImplementation = std::make_unique<SImplementation>();
}

CDijkstraPathRouter::~CDijkstraPathRouter() = default;

std::size_t CDijkstraPathRouter::VertexCount() const noexcept {
    return DImplementation->VertexCount();
}

CPathRouter::TVertexID CDijkstraPathRouter::AddVertex(std::any tag) noexcept {
    return DImplementation->AddVertex(std::move(tag));
}

std::any CDijkstraPathRouter::GetVertexTag(const TVertexID id) const noexcept {
    return DImplementation->GetVertexTag(id);
}

bool CDijkstraPathRouter::AddEdge(const TVertexID src, const TVertexID dest,
                                  const double weight, bool bidir) noexcept {
    return DImplementation->AddEdge(src, dest, weight, bidir);
}

bool CDijkstraPathRouter::Precompute(
    const std::chrono::steady_clock::time_point deadline) noexcept {
    return DImplementation->Precompute(deadline);
}

double
CDijkstraPathRouter::FindShortestPath(const TVertexID src, const TVertexID dest,
                                      std::vector<TVertexID> &path) noexcept {
    return DImplementation->FindShortestPath(src, dest, path);
}
