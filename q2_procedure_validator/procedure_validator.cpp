#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <cctype>
#include <ctime>

using namespace std;

const int MAX_PROCEDURES = 50;
const int MAX_SUGGEST_DISTANCE = 3;

string normalizeProcedure(string value) {
    value.erase(remove(value.begin(), value.end(), '\r'), value.end());
    size_t start = value.find_first_not_of(" \t");
    if (start == string::npos) return "";
    size_t end = value.find_last_not_of(" \t");
    value = value.substr(start, end - start + 1);
    for (char& c : value) {
        c = static_cast<char>(toupper(static_cast<unsigned char>(c)));
    }
    return value;
}

int levenshteinDistance(const string& a, const string& b) {
    int n = static_cast<int>(a.size());
    int m = static_cast<int>(b.size());
    vector<vector<int>> dp(n + 1, vector<int>(m + 1, 0));
    for (int i = 0; i <= n; ++i) dp[i][0] = i;
    for (int j = 0; j <= m; ++j) dp[0][j] = j;
    for (int i = 1; i <= n; ++i) {
        for (int j = 1; j <= m; ++j) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            dp[i][j] = min({dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost});
        }
    }
    return dp[n][m];
}

struct BSTNode {
    string procedure;
    BSTNode* left;
    BSTNode* right;

    BSTNode(const string& p) : procedure(p), left(nullptr), right(nullptr) {}
};

class ProcedureBST {
private:
    BSTNode* root;
    int count;

    void destroy(BSTNode* node) {
        if (!node) return;
        destroy(node->left);
        destroy(node->right);
        delete node;
    }

    BSTNode* insertNode(BSTNode* node, const string& procedure) {
        if (!node) return new BSTNode(procedure);
        if (procedure < node->procedure) {
            node->left = insertNode(node->left, procedure);
        } else if (procedure > node->procedure) {
            node->right = insertNode(node->right, procedure);
        }
        return node;
    }

    BSTNode* searchNode(BSTNode* node, const string& procedure) const {
        if (!node) return nullptr;
        if (procedure == node->procedure) return node;
        if (procedure < node->procedure) return searchNode(node->left, procedure);
        return searchNode(node->right, procedure);
    }

    void collectAll(BSTNode* node, string* list, int& idx) const {
        if (!node) return;
        collectAll(node->left, list, idx);
        if (idx < MAX_PROCEDURES) list[idx++] = node->procedure;
        collectAll(node->right, list, idx);
    }

public:
    ProcedureBST() : root(nullptr), count(0) {}

    ~ProcedureBST() {
        destroy(root);
    }

    bool insert(const string& procedure) {
        if (count >= MAX_PROCEDURES) return false;
        if (searchNode(root, procedure)) return false;
        root = insertNode(root, procedure);
        count++;
        return true;
    }

    bool contains(const string& procedure) const {
        return searchNode(root, procedure) != nullptr;
    }

    string findClosest(const string& input) const {
        string list[MAX_PROCEDURES];
        int idx = 0;
        collectAll(root, list, idx);
        if (idx == 0) return "";

        string best = list[0];
        int bestDist = levenshteinDistance(input, list[0]);
        for (int i = 1; i < idx; ++i) {
            int dist = levenshteinDistance(input, list[i]);
            if (dist < bestDist) {
                bestDist = dist;
                best = list[i];
            }
        }
        return best;
    }

    int size() const { return count; }

    void displayInOrder() const {
        string list[MAX_PROCEDURES];
        int idx = 0;
        collectAll(root, list, idx);
        cout << "Approved procedures (" << idx << "):\n";
        for (int i = 0; i < idx; ++i) {
            cout << "  - " << list[i] << "\n";
        }
    }
};

bool loadProcedures(const string& filename, ProcedureBST& tree) {
    ifstream in(filename);
    if (!in) {
        cerr << "Error: cannot open " << filename << "\n";
        return false;
    }
    string line;
    while (getline(in, line)) {
        string proc = normalizeProcedure(line);
        if (proc.empty()) continue;
        if (!tree.insert(proc)) {
            cerr << "Warning: procedure limit reached or duplicate skipped: " << proc << "\n";
        }
    }
    return true;
}

void logAudit(const string& attempt) {
    ofstream log("audit.log", ios::app);
    if (!log) {
        cerr << "Warning: unable to write to audit.log\n";
        return;
    }
    time_t t = time(nullptr);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    log << "[" << buf << "] REJECTED: " << attempt << "\n";
}

void verifyProcedure(ProcedureBST& tree, const string& input) {
    string normalized = normalizeProcedure(input);

    if (normalized.empty()) {
        cout << "Empty entry ignored.\n";
        return;
    }

    if (tree.contains(normalized)) {
        cout << "APPROVED: Procedure '" << normalized << "' is authorized for execution.\n";
        return;
    }

    string closest = tree.findClosest(normalized);
    int dist = closest.empty() ? 999 : levenshteinDistance(normalized, closest);

    if (!closest.empty() && dist > 0 && dist <= MAX_SUGGEST_DISTANCE) {
        cout << "SIMILAR: Did you mean '" << closest << "'? (edit distance: " << dist << ")\n";
        return;
    }

    cout << "REJECTED: Unknown procedure '" << normalized << "'.\n";
    logAudit(normalized);
}

int main(int argc, char* argv[]) {
    string filename = "procedures.txt";
    if (argc > 1) filename = argv[1];

    ProcedureBST tree;
    if (!loadProcedures(filename, tree)) {
        return 1;
    }

    cout << "=== Secure Maintenance Procedure Validator ===\n";
    tree.displayInOrder();
    cout << "\nEnter procedure name to verify (or 'quit' to exit):\n";

    string input;
    while (true) {
        cout << "> ";
        if (!getline(cin, input)) break;
        if (input == "quit" || input == "QUIT" || input == "q") break;
        verifyProcedure(tree, input);
    }

    cout << "Validator shutting down. Memory released.\n";
    return 0;
}
