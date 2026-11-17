#include "core/GraphCutSeamFinder.hpp"

#include "core/SeamOps.hpp"

#include <queue>
#include <vector>
#include <cstdint>
#include <algorithm>
namespace sc {
    Seam GraphCutSeamFinder::findVertical(const cv::Mat& /*img*/, const cv::Mat& energy) {
        /*
            1. calculate residual capacity = cap - flow
            2. bfs only traverses with residual capacity > 0
            3. find the bottleneck, minimum residual capacity across the entire path
            4. update the flow of every edge along the path by the bottleneck amount
            5. update the flow of every reverse edge accordingly by the bottleneck amount
        */
        struct Edge {
            /*
                destination index in nodes arr
            */
            int     to;
            /*
                when we need to reverse due to a bottleneck
                rev refers to the edge index that mirrors the current edge.
                e.g. nodes[to].edges[rev] points back to the node that the edge originated from.
            */
            int     rev;
            /*
                original edge capacity
                    - forward horizontal edges: oiginating node's energy
                    - structural edges connected to S or T => inf
                    - reverse edges: 0
            */
            int64_t cap;
            /*
                the current flow going through the edge
            */
            int64_t flow;
        };

        struct Node {
            std::vector<Edge> edges;
        };

        const int H = energy.rows;
        const int W = energy.cols;
        const int S = H * (W + 1);
        const int T = S + 1;

        std::vector<Node> nodes{};
        nodes.resize(T + 1);
        std::for_each(nodes.begin(), nodes.end(), [](Node& node) {
            node.edges.reserve(8);
            });

        const int64_t INF = 1e15;
        const auto id = [W](int y, int x) { return y * (W + 1) + x; };

        /*
            connect the pipe: u->v && v->u
        */
        const auto addEdge = [&](int u, int v, int64_t cap) {
            int uIdx = static_cast<int>(nodes[u].edges.size());
            int vIdx = static_cast<int>(nodes[v].edges.size());
            nodes[u].edges.push_back({ v, vIdx, cap, 0 });
            nodes[v].edges.push_back({ u, uIdx, 0, 0 });
        };

        for (int y = 0; y < H; y++) {
            addEdge(S, id(y, 0), INF);
            addEdge(id(y, W), T, INF);

            for (int x = 0; x < W; x++) {
                int64_t cost = static_cast<int64_t>(energy.at<double>(y, x));

                addEdge(id(y, x), id(y, x + 1), cost);

                addEdge(id(y, x + 1), id(y, x), INF);
                if (y > 0)      addEdge(id(y, x + 1), id(y - 1, x), INF);
                if (y + 1 < H)  addEdge(id(y, x + 1), id(y + 1, x), INF);
            }
        }

        /*
            parentNode[2] = 1: means arrived at node 2 via node 1
        */
        std::vector<int> parentNode(T + 1, -1);
        while (true) {
            std::fill(parentNode.begin(), parentNode.end(), -1);
            std::vector<int> parentEdge(T + 1, -1);
            std::queue<int> bfs{};

            parentNode[S] = S;
            bfs.push(S);

            /*
                construct the path from S to T

                only execute the loop if both

                    1) bfs is not empty
                    2) T has not been reached
            */
            while (!bfs.empty() && parentNode[T] == -1) {
                int u = bfs.front();
                bfs.pop();

                for (int i = 0; i < static_cast<int>(nodes[u].edges.size()); i++) {
                    const Edge& e = nodes[u].edges[i];
                    if (parentNode[e.to] == -1 && e.cap - e.flow > 0) {
                        parentNode[e.to] = u;
                        parentEdge[e.to] = i;
                        bfs.push(e.to);
                    }
                }
            }

            /*
                if T wasn't reached, max flow is complete
            */
            if (parentNode[T] == -1) {
                break;
            }

            /*
                walk backwards to find the bottleneck
            */
            int64_t bottleneck = INT64_MAX;
            for (int v = T; v != S; v = parentNode[v]) {
                int u = parentNode[v];
                int idx = parentEdge[v];
                const Edge& e = nodes[u].edges[idx];
                bottleneck = std::min(bottleneck, e.cap - e.flow);
            }

            /*
                walk backwards again to update forward and reverse pipes
                with the bottleneck
            */
            for (int v = T; v != S; v = parentNode[v]) {
                int u = parentNode[v];
                int idx = parentEdge[v];
                Edge& e = nodes[u].edges[idx];
                e.flow += bottleneck;
                nodes[v].edges[e.rev].flow -= bottleneck;
            }
        }

        Seam result{};

        result.cols.resize(H);
        std::fill(result.cols.begin(), result.cols.end(), -1);
        result.cost = 0.0;
        for (int y = 0; y < H; ++y) {
            for (int x = 0; x < W; ++x) {
                if (parentNode[id(y, x)] != -1 && parentNode[id(y, x + 1)] == -1) {
                    result.cols[y] = x;
                    result.cost += energy.at<double>(y, x);
                    break;
                }
            }
        }

        return result;
    }

}  // namespace sc
