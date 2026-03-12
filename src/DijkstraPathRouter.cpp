#include "DijkstraPathRouter.h"
#include "DijkstraPathRouter_p.h"

#include <algorithm>
#include <assert.h>
#include <iostream>
#include <queue>
#include <random>
#include <stop_token>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <utility>

struct CDijkstraPathRouter::SImplementation {
    using min_vertex_pq =
        std::priority_queue<std::pair<double, TVertexID>,
                            std::vector<std::pair<double, TVertexID>>,
                            std::greater<>>;

    using adjacency_list_t =
        std::unordered_map<TVertexID, std::unordered_map<TVertexID, double>>;

    struct DijkstraSearchState {
        std::unordered_map<TVertexID, TVertexID> path;
        std::unordered_map<TVertexID, double> distance;
        min_vertex_pq pq;
        //std::unordered_set<TVertexID> visited;
        bool started = false;
    };

    std::random_device rng_source;
    std::seed_seq seed{rng_source(), rng_source(), rng_source(), rng_source(),
                       rng_source()};
    std::mt19937_64 rng{seed};

    std::unordered_map<TVertexID, std::any> vertices;
    adjacency_list_t adjacency_list;

    static constexpr double epsilon = 1.0e-5;

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

        vertices.insert_or_assign(id, std::move(tag));
        adjacency_list.insert_or_assign(
            id, std::unordered_map<TVertexID, double>());

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
        }

        if (bidir) {
            if (const auto dst_it = adjacency_list[dest].find(src);
                dst_it == adjacency_list[dest].cend() ||
                weight < dst_it->second - epsilon) {
                adjacency_list[dest].insert_or_assign(src, weight);
            }
        }

        return true;
    }

    std::any GetVertexTag(const TVertexID id) const noexcept {
        const auto it = vertices.find(id);
        return it == vertices.cend() ? std::any() : it->second;
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
        if (!is_dirty && state.contains(src) &&
            state[src]->distance.contains(dest)) {
            path.emplace_back(dest);
            while (path.back() != src) {
                path.emplace_back(state[src]->path[path.back()]);
            }
            std::ranges::reverse(path);
            return state[src]->distance[dest];
        }

        // if the graph is marked dirty, or if we haven't started a search from
        // src yet, we (re)initialize the DijkstraSearchState and store it.
        if (is_dirty || !state.contains(src)) {
            state.insert_or_assign(src,
                                   std::make_shared<DijkstraSearchState>());
        }

        // some QoL shorthands
        auto &tracked_path = state[src]->path;
        auto &dist = state[src]->distance;
        auto &pq = state[src]->pq;
        //auto &visited = state[src]->visited;

        if (pq.empty()) {
            if (state[src]->started) {
                // if it is started and queue is still empty, while the dest is
                // not calculated, it is unreachable. return NoPathExists
                return NoPathExists;
            }
            // otherwise, we insert the source and start the search
            pq.emplace(0, src);
            dist.insert_or_assign(src, 0);
            state[src]->started = true;
        }

        while (!pq.empty()) {
            const auto u_weight = pq.top().first;
            const auto u = pq.top().second;
            pq.pop();

            // lazy delete -- if the path in priority_queue is not shorter than
            // the recorded distance, we skip it
            if (const auto target_vertex_it = dist.find(u);
                target_vertex_it != dist.cend() &&
                target_vertex_it->second < u_weight - epsilon)
                continue;

            //visited.insert(u);

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
                    const auto new_distance = dist[u] + v_weight;
                    const auto v_distance_it = dist.find(v);

                    if (v_distance_it == dist.cend() ||
                        new_distance < v_distance_it->second - epsilon) {
                        dist.insert_or_assign(v, new_distance);
                        tracked_path.insert_or_assign(v, u);
                        pq.emplace(new_distance, v);
                    }
                }
            }
        }

        is_dirty = false;

        const auto dest_distance_it = dist.find(dest);

        // if after Dijkstra we still don't have it in our dest, it's
        // unreachable. don't add anything to path and return NoPathExists
        if (dest_distance_it == dist.cend()) {
            return NoPathExists;
        }

        // reconstruct path from the tracked_path map
        path.emplace_back(dest);
        while (path.back() != src) {
            path.emplace_back(tracked_path[path.back()]);
        }
        // it is the other way round because we backtrack, reverse it
        std::ranges::reverse(path);

        // return the value
        return dest_distance_it->second;
    }

    struct FShortPathPrecomputeWorker {
        const TVertexID src;
        std::shared_ptr<DijkstraSearchState> state;
        const adjacency_list_t &adjacency_list;
        const std::stop_token &stop_token;

        void operator()() const {
            // some QoL shorthands
            auto &tracked_path = state->path;
            auto &dist = state->distance;
            auto &pq = state->pq;
            //auto &visited = state->visited;

            if (pq.empty()) {
                if (state->started) {
                    // if it is started and queue is still empty, we probably
                    // had done precomputing. exit.
                    return;
                }
                // otherwise, we insert the source and start the search
                pq.emplace(0, src);
                dist.insert_or_assign(src, 0);
                state->started = true;
            }

            while (!(pq.empty() || stop_token.stop_requested())) {
                const auto u_weight = pq.top().first;
                const auto u = pq.top().second;
                pq.pop();

                // lazy delete -- if the path in priority_queue is not shorter
                // than the recorded distance, we skip it
                if (const auto target_vertex_it = dist.find(u);
                    target_vertex_it != dist.cend() &&
                    target_vertex_it->second < u_weight - epsilon)
                    continue;

                //visited.insert(u);

                if (const auto u_adj_list_it = adjacency_list.find(u);
                    u_adj_list_it != adjacency_list.cend()) {
                    for (const auto [v, v_weight] : u_adj_list_it->second) {
                        const auto new_distance = dist[u] + v_weight;
                        const auto v_distance_it = dist.find(v);

                        if (v_distance_it == dist.cend() ||
                            new_distance < v_distance_it->second - epsilon) {
                            dist.insert_or_assign(v, new_distance);
                            tracked_path.insert_or_assign(v, u);
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
                state.insert_or_assign(id,
                                       std::make_shared<DijkstraSearchState>());
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
