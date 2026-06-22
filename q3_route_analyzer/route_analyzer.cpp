#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>

using namespace std;

const int MAX_AIRPORTS = 50;

string normalizeAirportCode(string code) {
    code.erase(remove(code.begin(), code.end(), '\r'), code.end());
    size_t start = code.find_first_not_of(" \t");
    if (start == string::npos) return "";
    size_t end = code.find_last_not_of(" \t");
    code = code.substr(start, end - start + 1);
    for (char& c : code) {
        c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    }
    return code;
}

struct Edge {
    int to;
};

class DirectedGraph {
private:
    vector<string> airports;
    vector<vector<Edge>> adjacency;
    vector<vector<bool>> matrix;

    int findIndex(const string& code) const {
        for (int i = 0; i < static_cast<int>(airports.size()); ++i) {
            if (airports[i] == code) return i;
        }
        return -1;
    }

    void rebuildMatrix() {
        int n = static_cast<int>(airports.size());
        matrix.assign(n, vector<bool>(n, false));
        for (int i = 0; i < n; ++i) {
            for (const Edge& e : adjacency[i]) {
                matrix[i][e.to] = true;
            }
        }
    }

public:
    bool addAirport(const string& code, bool quiet = false) {
        string normalized = normalizeAirportCode(code);
        if (normalized.empty()) {
            if (!quiet) cout << "Invalid airport code.\n";
            return false;
        }
        if (static_cast<int>(airports.size()) >= MAX_AIRPORTS) {
            if (!quiet) cout << "Airport limit reached.\n";
            return false;
        }
        if (findIndex(normalized) != -1) {
            if (!quiet) cout << "Airport already exists.\n";
            return false;
        }
        airports.push_back(normalized);
        adjacency.emplace_back();
        rebuildMatrix();
        if (!quiet) cout << "Airport " << normalized << " added.\n";
        return true;
    }

    bool removeAirport(const string& code) {
        string normalized = normalizeAirportCode(code);
        if (normalized.empty()) {
            cout << "Invalid airport code.\n";
            return false;
        }
        int idx = findIndex(normalized);
        if (idx == -1) {
            cout << "Airport not found.\n";
            return false;
        }

        airports.erase(airports.begin() + idx);
        adjacency.erase(adjacency.begin() + idx);
        for (auto& edges : adjacency) {
            edges.erase(remove_if(edges.begin(), edges.end(),
                [idx](const Edge& e) { return e.to == idx; }), edges.end());
            for (auto& e : edges) {
                if (e.to > idx) e.to--;
            }
        }
        rebuildMatrix();
        cout << "Airport " << normalized << " removed.\n";
        return true;
    }

    bool addRoute(const string& from, const string& to, bool quiet = false) {
        string src = normalizeAirportCode(from);
        string dst = normalizeAirportCode(to);
        if (src.empty() || dst.empty()) {
            if (!quiet) cout << "Invalid airport code.\n";
            return false;
        }
        if (src == dst) {
            if (!quiet) cout << "Cannot add a route from an airport to itself.\n";
            return false;
        }
        int i = findIndex(src);
        int j = findIndex(dst);
        if (i == -1 || j == -1) {
            if (!quiet) cout << "One or both airports not found.\n";
            return false;
        }
        for (const Edge& e : adjacency[i]) {
            if (e.to == j) {
                if (!quiet) cout << "Route already exists.\n";
                return false;
            }
        }
        adjacency[i].push_back({j});
        rebuildMatrix();
        if (!quiet) cout << "Route " << src << " -> " << dst << " added.\n";
        return true;
    }

    bool removeRoute(const string& from, const string& to) {
        string src = normalizeAirportCode(from);
        string dst = normalizeAirportCode(to);
        if (src.empty() || dst.empty()) {
            cout << "Invalid airport code.\n";
            return false;
        }
        int i = findIndex(src);
        int j = findIndex(dst);
        if (i == -1 || j == -1) {
            cout << "One or both airports not found.\n";
            return false;
        }
        auto& edges = adjacency[i];
        auto it = remove_if(edges.begin(), edges.end(),
            [j](const Edge& e) { return e.to == j; });
        if (it == edges.end()) {
            cout << "Route not found.\n";
            return false;
        }
        edges.erase(it, edges.end());
        rebuildMatrix();
        cout << "Route " << src << " -> " << dst << " removed.\n";
        return true;
    }

    void queryAirport(const string& code) const {
        string normalized = normalizeAirportCode(code);
        if (normalized.empty()) {
            cout << "Invalid airport code.\n";
            return;
        }
        int idx = findIndex(normalized);
        if (idx == -1) {
            cout << "Airport not found.\n";
            return;
        }

        cout << "\n=== Airport: " << normalized << " ===\n";
        cout << "Direct flights OUT to: ";
        if (adjacency[idx].empty()) {
            cout << "(none)";
        } else {
            for (size_t k = 0; k < adjacency[idx].size(); ++k) {
                if (k > 0) cout << ", ";
                cout << airports[adjacency[idx][k].to];
            }
        }
        cout << "\nDirect flights IN from: ";
        bool found = false;
        for (int i = 0; i < static_cast<int>(airports.size()); ++i) {
            for (const Edge& e : adjacency[i]) {
                if (e.to == idx) {
                    if (found) cout << ", ";
                    cout << airports[i];
                    found = true;
                }
            }
        }
        if (!found) cout << "(none)";
        cout << "\n";
    }

    void displayMatrix() const {
        int n = static_cast<int>(airports.size());
        if (n == 0) {
            cout << "No airports in graph.\n";
            return;
        }

        cout << "\n=== Adjacency Matrix ===\n";
        cout << setw(6) << "";
        for (const string& a : airports) cout << setw(6) << a;
        cout << "\n";

        for (int i = 0; i < n; ++i) {
            cout << setw(6) << airports[i];
            for (int j = 0; j < n; ++j) {
                cout << setw(6) << (matrix[i][j] ? "1" : "0");
            }
            cout << "\n";
        }
    }

    void displayAllRoutes() const {
        cout << "\n=== All Routes ===\n";
        bool any = false;
        for (int i = 0; i < static_cast<int>(airports.size()); ++i) {
            for (const Edge& e : adjacency[i]) {
                cout << airports[i] << " -> " << airports[e.to] << "\n";
                any = true;
            }
        }
        if (!any) cout << "(no routes)\n";
    }

    int loadSampleData() {
        vector<pair<string, string>> routes = {
            {"KGL", "NBO"}, {"KGL", "EBB"}, {"NBO", "JNB"},
            {"EBB", "ADD"}, {"ADD", "CAI"}, {"JNB", "CPT"}
        };
        for (const auto& r : routes) {
            addAirport(r.first, true);
            addAirport(r.second, true);
            addRoute(r.first, r.second, true);
        }
        return static_cast<int>(routes.size());
    }
};

void printMenu() {
    cout << "\n=== Airline Route Relationship Analyzer ===\n"
         << "1. Query airport (incoming/outgoing)\n"
         << "2. Add airport\n"
         << "3. Remove airport\n"
         << "4. Add route\n"
         << "5. Remove route\n"
         << "6. Display adjacency matrix\n"
         << "7. Display all routes\n"
         << "0. Quit\n"
         << "Choice: ";
}

int main() {
    DirectedGraph graph;
    int routeCount = graph.loadSampleData();

    cout << "=== Airline Route Relationship Analyzer ===\n"
         << "Sample network loaded: 7 airports, " << routeCount << " direct routes.\n";

    int choice;
    string a, b;
    while (true) {
        printMenu();
        if (!(cin >> choice)) break;
        cin.ignore();

        if (choice == 0) break;
        switch (choice) {
            case 1:
                cout << "Airport code: ";
                getline(cin, a);
                graph.queryAirport(a);
                break;
            case 2:
                cout << "New airport code: ";
                getline(cin, a);
                graph.addAirport(a);
                break;
            case 3:
                cout << "Airport to remove: ";
                getline(cin, a);
                graph.removeAirport(a);
                break;
            case 4:
                cout << "From: ";
                getline(cin, a);
                cout << "To: ";
                getline(cin, b);
                graph.addRoute(a, b);
                break;
            case 5:
                cout << "From: ";
                getline(cin, a);
                cout << "To: ";
                getline(cin, b);
                graph.removeRoute(a, b);
                break;
            case 6:
                graph.displayMatrix();
                break;
            case 7:
                graph.displayAllRoutes();
                break;
            default:
                cout << "Invalid choice.\n";
        }
    }

    cout << "Exiting analyzer.\n";
    return 0;
}
