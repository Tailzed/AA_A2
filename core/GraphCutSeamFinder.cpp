#include "core/GraphCutSeamFinder.hpp"

#include "core/SeamOps.hpp"

#include <queue>
#include <vector>
#include <cstdint>
#include <algorithm>

namespace sc {

    // Old Edmond's algo

    //Seam GraphCutSeamFinder::findVertical(const cv::Mat& /*img*/, const cv::Mat& energy) {
    //    /*
    //        1. calculate residual capacity = cap - flow
    //        2. bfs only traverses with residual capacity > 0
    //        3. find the bottleneck, minimum residual capacity across the entire path
    //        4. update the flow of every edge along the path by the bottleneck amount
    //        5. update the flow of every reverse edge accordingly by the bottleneck amount
    //    */
    //    struct Edge {
    //        /*
    //            destination index in nodes arr
    //        */
    //        int     to;
    //        /*
    //            when we need to reverse due to a bottleneck
    //            rev refers to the edge index that mirrors the current edge.
    //            e.g. nodes[to].edges[rev] points back to the node that the edge originated from.
    //        */
    //        int     rev;
    //        /*
    //            original edge capacity
    //                - forward horizontal edges: oiginating node's energy
    //                - structural edges connected to S or T => inf
    //                - reverse edges: 0
    //        */
    //        int64_t cap;
    //        /*
    //            the current flow going through the edge
    //        */
    //        int64_t flow;
    //    };

    //    struct Node {
    //        std::vector<Edge> edges;
    //    };

    //    const int H = energy.rows;
    //    const int W = energy.cols;
    //    const int S = H * (W + 1);
    //    const int T = S + 1;

    //    std::vector<Node> nodes{};
    //    nodes.resize(T + 1);
    //    std::for_each(nodes.begin(), nodes.end(), [](Node& node) {
    //        node.edges.reserve(8);
    //        });

    //    const int64_t INF = 1e15;
    //    const auto id = [W](int y, int x) { return y * (W + 1) + x; };

    //    /*
    //        connect the pipe: u->v && v->u
    //    */
    //    const auto addEdge = [&](int u, int v, int64_t cap) {
    //        int uIdx = static_cast<int>(nodes[u].edges.size());
    //        int vIdx = static_cast<int>(nodes[v].edges.size());
    //        nodes[u].edges.push_back({ v, vIdx, cap, 0 });
    //        nodes[v].edges.push_back({ u, uIdx, 0, 0 });
    //        };

    //    for (int y = 0; y < H; y++) {
    //        addEdge(S, id(y, 0), INF);
    //        addEdge(id(y, W), T, INF);

    //        for (int x = 0; x < W; x++) {
    //            int64_t cost = static_cast<int64_t>(energy.at<double>(y, x));

    //            addEdge(id(y, x), id(y, x + 1), cost);

    //            addEdge(id(y, x + 1), id(y, x), INF);
    //            if (y > 0)      addEdge(id(y, x + 1), id(y - 1, x), INF);
    //            if (y + 1 < H)  addEdge(id(y, x + 1), id(y + 1, x), INF);
    //        }
    //    }

    //    /*
    //        parentNode[2] = 1: means arrived at node 2 via node 1
    //    */
    //    std::vector<int> parentNode(T + 1, -1);
    //    while (true) {
    //        std::fill(parentNode.begin(), parentNode.end(), -1);
    //        std::vector<int> parentEdge(T + 1, -1);
    //        std::queue<int> bfs{};

    //        parentNode[S] = S;
    //        bfs.push(S);

    //        /*
    //            construct the path from S to T

    //            only execute the loop if both

    //                1) bfs is not empty
    //                2) T has not been reached
    //        */
    //        while (!bfs.empty() && parentNode[T] == -1) {
    //            int u = bfs.front();
    //            bfs.pop();

    //            for (int i = 0; i < static_cast<int>(nodes[u].edges.size()); i++) {
    //                const Edge& e = nodes[u].edges[i];
    //                if (parentNode[e.to] == -1 && e.cap - e.flow > 0) {
    //                    parentNode[e.to] = u;
    //                    parentEdge[e.to] = i;
    //                    bfs.push(e.to);
    //                }
    //            }
    //        }

    //        /*
    //            if T wasn't reached, max flow is complete
    //        */
    //        if (parentNode[T] == -1) {
    //            break;
    //        }

    //        /*
    //            walk backwards to find the bottleneck
    //        */
    //        int64_t bottleneck = INT64_MAX;
    //        for (int v = T; v != S; v = parentNode[v]) {
    //            int u = parentNode[v];
    //            int idx = parentEdge[v];
    //            const Edge& e = nodes[u].edges[idx];
    //            bottleneck = std::min(bottleneck, e.cap - e.flow);
    //        }

    //        /*
    //            walk backwards again to update forward and reverse pipes
    //            with the bottleneck
    //        */
    //        for (int v = T; v != S; v = parentNode[v]) {
    //            int u = parentNode[v];
    //            int idx = parentEdge[v];
    //            Edge& e = nodes[u].edges[idx];
    //            e.flow += bottleneck;
    //            nodes[v].edges[e.rev].flow -= bottleneck;
    //        }
    //    }

    //    Seam result{};

    //    result.cols.resize(H);
    //    std::fill(result.cols.begin(), result.cols.end(), -1);
    //    result.cost = 0.0;
    //    for (int y = 0; y < H; ++y) {
    //        for (int x = 0; x < W; ++x) {
    //            if (parentNode[id(y, x)] != -1 && parentNode[id(y, x + 1)] == -1) {
    //                result.cols[y] = x;
    //                result.cost += energy.at<double>(y, x);
    //                break;
    //            }
    //        }
    //    }

    //    return result;
    //}

    Seam GraphCutSeamFinder::findVertical(const cv::Mat& /*img*/, const cv::Mat& energy) {
        /*
            Dinic's max-flow algorithm  –  O(V^2 * E)
            ==========================================
            1. BFS from S to build a *level graph* (level[u] = shortest hop distance from S).
            2. DFS along edges that go from level L to level L+1, pushing as much flow as
               possible in one pass (a "blocking flow").
            3. Repeat until BFS can no longer reach T.

            Key optimisations over Edmonds-Karp (the previous BFS-augmenting-path approach):
              - Each BFS phase finds *all* shortest augmenting paths at once via the
                blocking-flow DFS, instead of one path per BFS.
              - Current-arc (`cur`) array: once a DFS retreats from an edge, that edge
                is never tried again in the same phase, giving O(VE) per phase.
              - Flat edge pool with index-based adjacency: a single `std::vector<Edge>`
                stores every edge contiguously in memory, giving much better cache behaviour
                than per-node vectors of edges.
        */

        // ---- Edge stored in a flat pool; reverse edge is always at index (i ^ 1) ----
        struct Edge {
            int     to;      // destination node
            int64_t resCap;  // residual capacity  (capacity − flow)
        };

        const int H = energy.rows;
        const int W = energy.cols;
        const int N = H * (W + 1);   // pixel-grid nodes
        const int S = N;              // super-source
        const int T = N + 1;          // super-sink
        const int totalNodes = T + 1;

        // ---- Flat edge pool + per-node head list (CSR-like, via linked list) ----
        //  head[u]           = index of first edge out of u  (-1 if none)
        //  nxt[edgeIdx]      = index of next edge with same source (-1 if last)
        //
        // Using a linked list lets us add edges in O(1) while keeping one flat vector.
        std::vector<Edge> edges;
        std::vector<int>  head(totalNodes, -1);
        std::vector<int>  nxt;

        // Reserve: each row contributes at most  2 (S/T) + W*(2+6) = 2 + 8W  edge pairs,
        // so total forward+reverse edges ≈ H*(2+8W)*2.
        edges.reserve(H * (4 + 16 * W));
        nxt.reserve(H * (4 + 16 * W));

        const int64_t INF = int64_t(1e15);
        const auto id = [W](int y, int x) { return y * (W + 1) + x; };

        /*
            addEdge: creates the forward edge u→v with capacity `cap`
                     and the reverse edge v→u with capacity 0.
            The two edges are always at consecutive even/odd indices,
            so the reverse of edge i is at i^1.
        */
        const auto addEdge = [&](int u, int v, int64_t cap) {
            int idx = static_cast<int>(edges.size());
            // forward edge
            edges.push_back({ v, cap });
            nxt.push_back(head[u]);
            head[u] = idx;
            // reverse edge
            edges.push_back({ u, 0 });
            nxt.push_back(head[v]);
            head[v] = idx + 1;
        };

        // ---- Build the graph (identical topology to the previous version) ----
        for (int y = 0; y < H; y++) {
            addEdge(S, id(y, 0), INF);       // source → leftmost column
            addEdge(id(y, W), T, INF);        // rightmost (dummy) column → sink

            for (int x = 0; x < W; x++) {
                int64_t cost = static_cast<int64_t>(energy.at<double>(y, x));

                // horizontal forward edge with finite capacity = energy cost
                addEdge(id(y, x), id(y, x + 1), cost);

                // structural reverse / diagonal edges (infinite capacity)
                addEdge(id(y, x + 1), id(y, x), INF);
                if (y > 0)      addEdge(id(y, x + 1), id(y - 1, x), INF);
                if (y + 1 < H)  addEdge(id(y, x + 1), id(y + 1, x), INF);
            }
        }

        // ---- Dinic's: BFS to build level graph, DFS to find blocking flows ----
        std::vector<int> level(totalNodes);
        std::vector<int> cur(totalNodes);   // current-arc optimisation

        /*
            bfs(): builds the level graph.
            Returns true iff the sink T is reachable from S.
        */
        const auto bfs = [&]() -> bool {
            std::fill(level.begin(), level.end(), -1);
            level[S] = 0;
            std::queue<int> q;
            q.push(S);
            while (!q.empty()) {
                int u = q.front();
                q.pop();
                for (int i = head[u]; i != -1; i = nxt[i]) {
                    const Edge& e = edges[i];
                    if (level[e.to] == -1 && e.resCap > 0) {
                        level[e.to] = level[u] + 1;
                        q.push(e.to);
                    }
                }
            }
            return level[T] != -1;
        };

        /*
            dfs(u, pushed):  find a blocking flow from u to T.
            `pushed` is the minimum residual capacity along the path so far.
            Returns the amount of flow actually pushed (0 if dead end).

            The cur[] array remembers, for each node, the last edge we tried;
            once we retreat from an edge we never revisit it in this BFS phase.
        */
        // Using an explicit stack to avoid deep recursion on large graphs.
        struct Frame {
            int   node;
            int64_t pushed;
        };
        std::vector<Frame> stack;
        stack.reserve(totalNodes);

        // Iterative DFS that pushes one S→T augmenting path per call,
        // returning the bottleneck pushed (0 when no path exists).
        const auto dfs = [&](int64_t initPushed) -> int64_t {
            stack.clear();
            stack.push_back({ S, initPushed });

            while (!stack.empty()) {
                Frame& f = stack.back();
                int u = f.node;

                if (u == T) {
                    // Reached the sink – f.pushed is the bottleneck.
                    int64_t flow = f.pushed;

                    // Walk back up the stack and update residual capacities.
                    stack.pop_back();                 // remove T
                    while (!stack.empty()) {
                        int parent = stack.back().node;
                        int eIdx   = cur[parent];     // the edge we descended on
                        edges[eIdx].resCap     -= flow;
                        edges[eIdx ^ 1].resCap += flow;
                        stack.pop_back();
                    }
                    return flow;
                }

                // Try advancing along the next admissible edge.
                bool advanced = false;
                for (int& i = cur[u]; i != -1; i = nxt[i]) {
                    const Edge& e = edges[i];
                    if (e.resCap > 0 && level[e.to] == level[u] + 1) {
                        stack.push_back({ e.to, std::min(f.pushed, e.resCap) });
                        advanced = true;
                        break;              // descend into e.to
                    }
                }

                if (!advanced) {
                    // Dead end: remove u from the level graph so other
                    // paths don't waste time re-entering it.
                    level[u] = -1;
                    stack.pop_back();
                }
            }
            return 0;      // no augmenting path found
        };

        // Main Dinic's loop
        while (bfs()) {
            // Reset current-arc pointers to the first edge of each node.
            std::copy(head.begin(), head.end(), cur.begin());

            while (dfs(INF) > 0) {
                // Reset cur again for each blocking-flow iteration.
                std::copy(head.begin(), head.end(), cur.begin());
            }
        }

        // ---- Extract the min-cut seam ----
        // After max-flow, perform one final BFS from S in the residual graph.
        // Reachable nodes are on the source side; the seam is at the boundary.
        std::vector<bool> visited(totalNodes, false);
        {
            std::queue<int> q;
            visited[S] = true;
            q.push(S);
            while (!q.empty()) {
                int u = q.front();
                q.pop();
                for (int i = head[u]; i != -1; i = nxt[i]) {
                    const Edge& e = edges[i];
                    if (!visited[e.to] && e.resCap > 0) {
                        visited[e.to] = true;
                        q.push(e.to);
                    }
                }
            }
        }

        Seam result{};
        result.cols.resize(H);
        std::fill(result.cols.begin(), result.cols.end(), -1);
        result.cost = 0.0;
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                if (visited[id(y, x)] && !visited[id(y, x + 1)]) {
                    result.cols[y] = x;
                    result.cost += energy.at<double>(y, x);
                    break;
                }
            }
        }

        return result;
    }

}  // namespace sc
