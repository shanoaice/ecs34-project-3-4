#include "DijkstraPathRouter.h"

#include <algorithm>
#include <queue>
#include <random>
#include <unordered_map>
#include <unordered_set>
#include <utility>

struct CDijkstraPathRouter::SImplementation {
    using min_vertex_pq =
        std::priority_queue<std::pair<double, TVertexID>,
                            std::vector<std::pair<double, TVertexID>>,
                            std::greater<>>;

    struct DijkstraSearchState {
        std::unordered_map<TVertexID, TVertexID> path;
        std::unordered_map<TVertexID, double> distance;
        min_vertex_pq pq;
        std::unordered_set<TVertexID> visited;
        bool started = false;
    };

    std::random_device rng_source;
    std::seed_seq seed{rng_source(), rng_source(), rng_source(), rng_source(),
                       rng_source()};
    std::mt19937_64 rng{seed};

    std::unordered_map<TVertexID, std::any> vertices;
    std::unordered_map<TVertexID, std::unordered_map<TVertexID, double>>
        adjancency_list;

    const double epsilon = 1.0e-5;

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
        TVertexID id = rng();

        // make sure vertex id is unique
        while (vertices.contains(id)) {
            id = rng();
        }

        // we don't mark graph dirty if we just inserted a vertex
        // because by default it is not reachable
        // is_dirty = true;

        vertices[id] = std::move(tag);
        adjancency_list[id] = {};

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
        if (const auto src_it = adjancency_list[src].find(dest);
            src_it == adjancency_list[src].cend() ||
            weight < src_it->second - epsilon) {
            adjancency_list[src].insert_or_assign(dest, weight);
        }

        if (bidir) {
            if (const auto dst_it = adjancency_list[dest].find(src);
                dst_it == adjancency_list[dest].cend() ||
                weight < dst_it->second - epsilon) {
                adjancency_list[dest].insert_or_assign(src, weight);
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
            state[src]->visited.contains(dest)) {
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
        auto &visited = state[src]->visited;

        if (pq.empty()) {
            if (state[src]->started) {
                // if it is started and queue is still empty, while the dest is
                // not in visited set, it is unreachable. return NoPathExists
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

            visited.insert(u);

            // if we reached the destination vertex, exit early
            if (u == dest) {
                break;
            }

            if (const auto u_adj_list_it = adjancency_list.find(u);
                u_adj_list_it != adjancency_list.cend()) {
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
    std::chrono::steady_clock::time_point deadline) noexcept {
    // TODO
    return true;
}

double
CDijkstraPathRouter::FindShortestPath(TVertexID src, TVertexID dest,
                                      std::vector<TVertexID> &path) noexcept {
    return DImplementation->FindShortestPath(src, dest, path);
}
