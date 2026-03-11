#include "DijkstraPathRouter.h"

#include <queue>
#include <random>
#include <sys/stat.h>
#include <unordered_map>
#include <utility>

using min_vertex_pq = std::priority_queue<
    std::pair<double, CPathRouter::TVertexID>,
    std::vector<std::pair<double, CPathRouter::TVertexID>>,
    std::greater<std::pair<double, CPathRouter::TVertexID>>>;

struct CDijkstraPathRouter::SImplementation {
    std::random_device rng_source;
    std::seed_seq seed{rng_source(), rng_source(), rng_source(), rng_source(),
                       rng_source()};
    std::mt19937_64 rng{seed};

    std::vector<TVertexID> vertices;
    std::unordered_map<TVertexID, std::any> vertex_map;
    std::unordered_map<TVertexID, std::unordered_map<TVertexID, double>>
        connection_map;

    const double epsilon = 1.0e-7;

    bool is_dirty = false;

    // This map's key is the source vertex id
    // value is a pointer to another map that stores the calculated path
    std::unordered_map<
        TVertexID, std::shared_ptr<std::unordered_map<TVertexID, TVertexID>>>
        path;
    // same thing, calculated weight (distance)
    std::unordered_map<TVertexID,
                       std::shared_ptr<std::unordered_map<TVertexID, double>>>
        dist;
    // same thing, priority_queue scratchpad
    std::unordered_map<TVertexID, std::shared_ptr<min_vertex_pq>> pq;

    // both have the 2nd map wrapped in a shared_ptr because they might have to
    // be sent across threads in precompute. We don't need a lock because no two
    // threads will work on the same source vertex the same time vertices,
    // vertex_map & connection map also does not have to be locked, because they
    // are read-only in Precompute

    std::size_t VertexCount() const noexcept { return vertices.size(); }

    TVertexID AddVertex(std::any tag) noexcept {
        TVertexID id = rng();

        // make sure vertex id is unique
        while (vertex_map.contains(id)) {
            id = rng();
        }

        // update state flag
        is_dirty = true;

        vertices.emplace_back(id);
        vertex_map[id] = std::move(tag);
        connection_map[id] = {};

        return id;
    }

    bool AddEdge(const TVertexID src, const TVertexID dest, const double weight,
                 const bool bidir) noexcept {
        if (src == dest)
            return true;

        if (!vertex_map.contains(src) || !vertex_map.contains(dest))
            return false;

        if (weight < 0)
            return false;

        // update state flag
        is_dirty = true;

        // compare with epsilon for double fp error
        if (const auto src_it = connection_map[src].find(dest);
            src_it == connection_map[src].cend() ||
            weight < src_it->second - epsilon) {
            connection_map[src].insert_or_assign(dest, weight);
        }

        if (bidir) {
            if (const auto dst_it = connection_map[dest].find(src);
                dst_it == connection_map[dest].cend() ||
                weight < dst_it->second - epsilon) {
                connection_map[dest].insert_or_assign(src, weight);
            }
        }

        return true;
    }

    std::any GetVertexTag(const TVertexID id) const noexcept {
        const auto it = vertex_map.find(id);
        return it == vertex_map.cend() ? std::any() : it->second;
    }

    double FindShortestPath(const TVertexID src, const TVertexID dest,
                            std::vector<TVertexID> &path) {
        bool scratchpad_initialized = this->path.contains(src) &&
                                      this->dist.contains(src) &&
                                      this->pq.contains(src);

        if (is_dirty || !scratchpad_initialized) {
            this->path[src] =
                std::make_shared<std::unordered_map<TVertexID, TVertexID>>();
            this->dist[src] =
                std::make_shared<std::unordered_map<TVertexID, double>>();
            this->pq[src] = std::make_shared<min_vertex_pq>();
        }

        const auto &tracked_path = this->path[src];
        const auto &dist = this->dist[src];
        const auto &pq = this->pq[src];

        if (pq->empty()) {
            pq->emplace(0, src);
        }

        while (!pq->empty()) {
            auto weight = pq->top().first;
            auto vertex = pq->top().second;
        }

        is_dirty = false;
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

double
CDijkstraPathRouter::FindShortestPath(TVertexID src, TVertexID dest,
                                      std::vector<TVertexID> &path) noexcept {
    return DImplementation->FindShortestPath(src, dest, path);
}
