#include "DijkstraPathRouter.h"
#include "DijkstraPathRouter_p.h"

#include <algorithm>
#include <any>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <iostream>
#include <limits>
#include <queue>
#include <random>
#include <stdexcept>
#include <stop_token>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>

struct CDijkstraPathRouter::SImplementation {
    using min_vertex_pq =
        std::priority_queue<std::pair<double, TVertexID>,
                            std::vector<std::pair<double, TVertexID>>,
                            std::greater<>>;

    using adjacency_list_t =
        std::unordered_map<TVertexID, std::unordered_map<TVertexID, double>>;

    using vertices_index = std::size_t;
    using vertices_t =
        std::unordered_map<TVertexID, std::pair<vertices_index, std::any>>;

    struct DijkstraSearchState {
        // std::unordered_map<TVertexID, TVertexID> path;
        std::vector<double> distance;
        min_vertex_pq pq;
        // std::unordered_set<TVertexID> visited;
        bool started = false;

        DijkstraSearchState() = delete;
        DijkstraSearchState(const std::size_t vertex_count)
            : distance(vertex_count, std::numeric_limits<double>::infinity()) {}
    };

    std::random_device rng_source;
    std::seed_seq seed{rng_source(), rng_source(), rng_source(), rng_source(),
                       rng_source()};
    std::mt19937_64 rng{seed};

    std::vector<TVertexID> index_to_id;
    vertices_t vertices;
    adjacency_list_t adjacency_list;
    adjacency_list_t reverse_adjacency_list;

    static constexpr double epsilon = 1.0e-6;
    static constexpr double kInf = std::numeric_limits<double>::infinity();

    bool is_dirty = false;

    // This map's key is the source vertex id
    // value is a pointer to the struct storing the Dijkstra search state on the
    // source vertex
    std::unordered_map<TVertexID, std::shared_ptr<DijkstraSearchState>> state;

    // state object is wrapped in shared_ptr because they might have to
    // be sent across threads in precompute. We don't need a lock because no two
    // threads will work on the same source vertex the same time. Vertices,
    // vertex_map & connection map also does not have to be locked, because they
    // are read-only in Precompute

    vertices_index IndexOf(const TVertexID id) const {
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

        // we don't mark graph dirty if we just inserted a vertex
        // because by default it is not reachable
        // is_dirty = true;

        if (const auto existing = vertices.find(id);
            existing != vertices.cend()) {
            existing->second.second = std::move(tag);
            return id;
        }

        const auto new_index = vertices.size();
        index_to_id.emplace_back(id);
        vertices.insert_or_assign(id, std::pair{new_index, std::move(tag)});
        adjacency_list.insert_or_assign(
            id, std::unordered_map<TVertexID, double>());

        // for all nodes that have a state, resize the dense distance vector
        for (const auto &[_, vertex_state] : state) {
            vertex_state->distance.emplace_back(kInf);
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
        is_dirty = true;

        // compare with epsilon for double fp error
        if (const auto src_it = adjacency_list[src].find(dest);
            src_it == adjacency_list[src].cend() ||
            weight < src_it->second - epsilon) {
            adjacency_list[src].insert_or_assign(dest, weight);
            reverse_adjacency_list[dest].insert_or_assign(src, weight);
        }

        if (bidir) {
            if (const auto dst_it = adjacency_list[dest].find(src);
                dst_it == adjacency_list[dest].cend() ||
                weight < dst_it->second - epsilon) {
                adjacency_list[dest].insert_or_assign(src, weight);
                reverse_adjacency_list[src].insert_or_assign(dest, weight);
            }
        }

        return true;
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

        // if the graph is not changed (marked dirty by AddEdge) and the dest
        // vertex is visited, we had searched it, and we just backtrack the path
        // and returns the calculated value. we could cache the path too and
        // completely eliminate any extra calculation, but that is costly on
        // space and I doubt this will be a hot path
        if (!vertices.contains(src) || !vertices.contains(dest)) {
            return NoPathExists;
        }

        if (!is_dirty && state.contains(src) &&
            state[src]->distance[IndexOf(dest)] != kInf &&
            state[src]->pq.empty()) {
            auto &dist = state[src]->distance;

            if (dist.size() != vertices.size()) {
                throw std::runtime_error("distance vector size mismatch");
            }

            path.emplace_back(dest);
            std::size_t hops = 0;
            const auto max_hops = vertices.size();
            while (path.back() != src) {
                const auto &rev_conn_vertices =
                    reverse_adjacency_list[path.back()];
                bool found_prev = false;
                for (const auto &[v, d] : rev_conn_vertices) {
                    const auto curr_idx = IndexOf(path.back());
                    const auto prev_idx = IndexOf(v);
                    if (dist[curr_idx] == kInf || dist[prev_idx] == kInf) {
                        continue;
                    }

                    if (isClose(dist[curr_idx], dist[prev_idx] + d)) {
                        path.emplace_back(v);
                        found_prev = true;
                        break;
                    }
                }

#ifndef NDEBUG
                if (!found_prev) {
                    std::cerr << "Path reconstruction failed at vertex "
                              << path.back() << " pq size "
                              << state[src]->pq.size() << "\n";
                }
#endif

                if (!found_prev || ++hops > max_hops) {
                    path.clear();
                    return NoPathExists;
                }
            }
            std::ranges::reverse(path);
            return state[src]->distance[IndexOf(dest)];
        }

        // if the graph is marked dirty, or if we haven't started a search from
        // src yet, we (re)initialize the DijkstraSearchState and store it.
        if (is_dirty || !state.contains(src)) {
            state.insert_or_assign(
                src, std::make_shared<DijkstraSearchState>(vertices.size()));
        }

        // some QoL shorthands
        // auto &tracked_path = state[src]->path;
        auto &dist = state[src]->distance;
        auto &pq = state[src]->pq;
        // auto &visited = state[src]->visited;

        if (dist.size() != vertices.size()) {
            throw std::runtime_error("distance vector size mismatch");
        }

        if (pq.empty()) {
            if (state[src]->started) {
                // if it is started and queue is still empty, while the dest is
                // not calculated, it is unreachable. return NoPathExists
                return NoPathExists;
            }
            // otherwise, we insert the source and start the search
            pq.emplace(0, src);
            dist[IndexOf(src)] = 0.0;
            state[src]->started = true;
        }

        while (!pq.empty()) {
            const auto u_weight = pq.top().first;
            const auto u = pq.top().second;
            pq.pop();

            // lazy delete -- if the path in priority_queue is not shorter than
            // the recorded distance, we skip it
            if (dist[IndexOf(u)] < u_weight - epsilon)
                continue;

            // visited.insert(u);

            // now that we implemented precompute, it would be better to not
            // exit early, so that we save some space by not requiring a visited
            // set.
            //// if we reached the destination vertex, exit early
            // if (u == dest) {
            //     break;
            // }

            if (const auto u_adj_list_it = adjacency_list.find(u);
                u_adj_list_it != adjacency_list.cend()) {
                for (const auto [v, v_weight] : u_adj_list_it->second) {
                    const auto new_distance = dist[IndexOf(u)] + v_weight;

                    if (new_distance < dist[IndexOf(v)] - epsilon) {
                        dist[IndexOf(v)] = new_distance;
                        // tracked_path.insert_or_assign(v, u);
                        pq.emplace(new_distance, v);
                    }
                }
            }
        }

        is_dirty = false;

        // if after Dijkstra we still don't have it in our dest, it's
        // unreachable. don't add anything to path and return NoPathExists
        if (dist[IndexOf(dest)] == kInf) {
            return NoPathExists;
        }

        // reconstruct path from the tracked_path map
        path.emplace_back(dest);
        std::size_t hops = 0;
        const auto max_hops = vertices.size();
        while (path.back() != src) {
            bool found_prev = false;
            for (const auto &rev_conn_vertices =
                     reverse_adjacency_list[path.back()];
                 const auto &[v, d] : rev_conn_vertices) {
                const auto curr_idx = IndexOf(path.back());
                const auto prev_idx = IndexOf(v);
                if (dist[curr_idx] == kInf || dist[prev_idx] == kInf) {
                    continue;
                }

                if (isClose(dist[curr_idx], dist[prev_idx] + d)) {
                    path.emplace_back(v);
                    found_prev = true;
                    break;
                }
            }

#ifndef NDEBUG
            if (!found_prev) {
                static std::atomic<int> s_debug_count = 0;
                if (s_debug_count.fetch_add(1) < 30) {
                    const auto curr = path.back();
                    const auto curr_idx = IndexOf(curr);
                    std::size_t rev_degree = 0;
                    double best_abs_err = kInf;
                    if (const auto rev_it = reverse_adjacency_list.find(curr);
                        rev_it != reverse_adjacency_list.cend()) {
                        rev_degree = rev_it->second.size();
                        for (const auto &[v, d] : rev_it->second) {
                            const auto lhs = dist[curr_idx];
                            const auto rhs = dist[IndexOf(v)] + d;
                            best_abs_err =
                                std::min(best_abs_err, std::abs(lhs - rhs));
                        }
                    }

                    std::cerr
                        << "Path reconstruction failed (non-cached) src=" << src
                        << " dest=" << dest << " curr=" << curr
                        << " hops=" << hops << " pq_size=" << pq.size()
                        << " rev_degree=" << rev_degree
                        << " dist_curr=" << dist[curr_idx]
                        << " best_abs_err=" << best_abs_err << "\n";
                }
            }
#endif

            if (!found_prev || ++hops > max_hops) {
                path.clear();
                return NoPathExists;
            }
        }
        // it is the other way round because we backtrack, reverse it
        std::ranges::reverse(path);

        // return the value
        return dist[IndexOf(dest)];
    }

    struct FShortPathPrecomputeWorker {
        const TVertexID src;
        std::shared_ptr<DijkstraSearchState> state;
        const adjacency_list_t &adjacency_list;
        const vertices_t &vertices;
        const std::stop_token &stop_token;

        void operator()() const {
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
                pq.emplace(0, src);
                const auto u_vertices_it = vertices.find(src);
                if (u_vertices_it == vertices.cend())
                    return;
                dist[u_vertices_it->second.first] = 0.0;
                state->started = true;
            }

            while (!(pq.empty() || stop_token.stop_requested())) {
                const auto u_weight = pq.top().first;
                const auto u = pq.top().second;
                pq.pop();

                // lazy delete -- if the path in priority_queue is not shorter
                // than the recorded distance, we skip it
                const auto u_vertices_it = vertices.find(u);
                if (u_vertices_it == vertices.cend() ||
                    dist[u_vertices_it->second.first] < u_weight - epsilon)
                    continue;

                const auto u_index = u_vertices_it->second.first;

                // visited.insert(u);

                if (const auto u_adj_list_it = adjacency_list.find(u);
                    u_adj_list_it != adjacency_list.cend()) {
                    for (const auto [v, v_weight] : u_adj_list_it->second) {
                        const auto new_distance = dist[u_index] + v_weight;
                        const auto v_vertices_it = vertices.find(v);
                        if (v_vertices_it == vertices.cend())
                            continue;

                        if (new_distance <
                            dist[v_vertices_it->second.first] - epsilon) {
                            dist[v_vertices_it->second.first] = new_distance;
                            // tracked_path.insert_or_assign(v, u);
                            pq.emplace(new_distance, v);
                        }
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

        // use core count - 1 or 1
        const auto concurrency = []() {
            const auto hardware_concurrency =
                std::jthread::hardware_concurrency();
            return hardware_concurrency > 1 ? hardware_concurrency - 1 : 1;
        }();

        std::vector<TVertexID> pending_sources;

        // add all ids into pending_sources
        for (const auto &[id, _] : vertices) {
            pending_sources.emplace_back(id);
            if (!state.contains(id) || is_dirty) {
                state.insert_or_assign(
                    id, std::make_shared<DijkstraSearchState>(vertices.size()));
            }
        }

        // if nothing to precompute, return
        if (pending_sources.empty()) {
            is_dirty = false;
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

                FShortPathPrecomputeWorker{.src = src,
                                           .state = state[src],
                                           .adjacency_list = adjacency_list,
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
                break;
            }
        }

        for (auto &thread : threads) {
            thread.request_stop();
        }

        return true;
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
