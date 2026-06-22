#include <iostream>
#include <vector>
#include <string>
#include <limits>
#include <algorithm>
#include <cctype>

using namespace std;

const int INF = numeric_limits<int>::max() / 4;
const string DESTINATION = "Dormitory";

string trimInput(string value) {
    value.erase(remove(value.begin(), value.end(), '\r'), value.end());
    size_t start = value.find_first_not_of(" \t");
    if (start == string::npos) return "";
    size_t end = value.find_last_not_of(" \t");
    return value.substr(start, end - start + 1);
}

bool equalsIgnoreCase(const string& a, const string& b) {
    if (a.size() != b.size()) return false;
    for (size_t i = 0; i < a.size(); ++i) {
        if (tolower(static_cast<unsigned char>(a[i])) !=
            tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

struct Edge {
    int to;
    int weight;
};

class CampusGraph {
private:
    vector<string> buildings;
    vector<vector<Edge>> adjacency;

    int findIndex(const string& name) const {
        string query = trimInput(name);
        if (query.empty()) return -1;
        for (int i = 0; i < static_cast<int>(buildings.size()); ++i) {
            if (equalsIgnoreCase(query, buildings[i])) return i;
        }
        return -1;
    }

public:
    CampusGraph() {
        buildings = {
            "Library", "Cafeteria", "Engineering", "Science Block",
            "Dormitory", "Administration", "Charging Station"
        };
        adjacency.resize(buildings.size());

        auto addEdge = [&](const string& u, const string& v, int w) {
            int i = findIndex(u);
            int j = findIndex(v);
            adjacency[i].push_back({j, w});
            adjacency[j].push_back({i, w});
        };

        addEdge("Library", "Cafeteria", 6);
        addEdge("Library", "Engineering", 15);
        addEdge("Cafeteria", "Science Block", 4);
        addEdge("Science Block", "Dormitory", 8);
        addEdge("Engineering", "Administration", 5);
        addEdge("Administration", "Dormitory", 3);
        addEdge("Cafeteria", "Charging Station", 2);
        addEdge("Charging Station", "Administration", 4);
    }

    void listBuildings() const {
        cout << "Available buildings:\n";
        for (size_t i = 0; i < buildings.size(); ++i) {
            cout << "  " << (i + 1) << ". " << buildings[i] << "\n";
        }
    }

    bool dijkstra(const string& start, vector<int>& dist, vector<int>& parent) const {
        int n = static_cast<int>(buildings.size());
        int src = findIndex(start);
        int dest = findIndex(DESTINATION);

        if (src == -1) return false;

        dist.assign(n, INF);
        parent.assign(n, -1);
        vector<bool> visited(n, false);

        dist[src] = 0;
        for (int count = 0; count < n; ++count) {
            int u = -1;
            int best = INF;
            for (int i = 0; i < n; ++i) {
                if (!visited[i] && dist[i] < best) {
                    best = dist[i];
                    u = i;
                }
            }
            if (u == -1 || best == INF) break;
            visited[u] = true;

            for (const Edge& e : adjacency[u]) {
                if (!visited[e.to] && dist[u] + e.weight < dist[e.to]) {
                    dist[e.to] = dist[u] + e.weight;
                    parent[e.to] = u;
                }
            }
        }
        return dist[dest] != INF;
    }

    void displayRoute(const string& start) const {
        string trimmed = trimInput(start);
        if (trimmed.empty()) {
            cout << "Error: No starting building entered.\n";
            listBuildings();
            return;
        }

        int startIdx = findIndex(trimmed);
        if (startIdx == -1) {
            cout << "Error: Invalid building name '" << trimmed << "'.\n";
            cout << "Please choose from the list of valid campus buildings.\n";
            listBuildings();
            return;
        }

        string startName = buildings[startIdx];
        vector<int> dist, parent;
        if (!dijkstra(startName, dist, parent)) {
            cout << "No route found from " << startName << " to " << DESTINATION << ".\n";
            return;
        }

        int destIdx = findIndex(DESTINATION);
        vector<string> path;
        for (int at = destIdx; at != -1; at = parent[at]) {
            path.push_back(buildings[at]);
        }
        reverse(path.begin(), path.end());

        cout << "\n=== Shortest Route to Dormitory ===\n";
        cout << "Start: " << startName << "\n";
        cout << "Path: ";
        for (size_t i = 0; i < path.size(); ++i) {
            if (i > 0) cout << " -> ";
            cout << path[i];
        }
        cout << "\nTotal travel distance: " << dist[destIdx] << " units\n";
    }
};

int main() {
    CampusGraph campus;
    cout << "=== Campus Delivery Robot Navigation ===\n";
    campus.listBuildings();

    string start;
    cout << "\nEnter starting building name: ";
    getline(cin, start);

    campus.displayRoute(start);
    return 0;
}
