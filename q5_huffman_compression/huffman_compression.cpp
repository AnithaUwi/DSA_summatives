#include <iostream>
#include <fstream>
#include <vector>
#include <queue>
#include <unordered_map>
#include <bitset>
#include <cstdint>
#include <stdexcept>
#include <iomanip>
#include <algorithm>
#include <cctype>

using namespace std;

struct HuffmanNode {
    unsigned char ch;
    unsigned int freq;
    HuffmanNode* left;
    HuffmanNode* right;

    HuffmanNode(unsigned char c, unsigned int f)
        : ch(c), freq(f), left(nullptr), right(nullptr) {}

    HuffmanNode(HuffmanNode* l, HuffmanNode* r)
        : ch(0), freq(l->freq + r->freq), left(l), right(r) {}

    bool isLeaf() const { return !left && !right; }
};

struct Compare {
    bool operator()(HuffmanNode* a, HuffmanNode* b) const {
        if (a->freq != b->freq) return a->freq > b->freq;
        return a->ch > b->ch;
    }
};

void freeTree(HuffmanNode* node) {
    if (!node) return;
    freeTree(node->left);
    freeTree(node->right);
    delete node;
}

void buildCodes(HuffmanNode* node, const string& prefix, unordered_map<unsigned char, string>& codes) {
    if (!node) return;
    if (node->isLeaf()) {
        codes[node->ch] = prefix.empty() ? "0" : prefix;
        return;
    }
    buildCodes(node->left, prefix + "0", codes);
    buildCodes(node->right, prefix + "1", codes);
}

HuffmanNode* buildHuffmanTree(const unordered_map<unsigned char, unsigned int>& freq) {
    priority_queue<HuffmanNode*, vector<HuffmanNode*>, Compare> pq;
    for (const auto& p : freq) {
        pq.push(new HuffmanNode(p.first, p.second));
    }
    if (pq.empty()) return nullptr;
    if (pq.size() == 1) return pq.top();

    while (pq.size() > 1) {
        HuffmanNode* left = pq.top(); pq.pop();
        HuffmanNode* right = pq.top(); pq.pop();
        pq.push(new HuffmanNode(left, right));
    }
    return pq.top();
}

void writeTree(ofstream& out, HuffmanNode* node) {
    if (!node) {
        out.put(0);
        return;
    }
    if (node->isLeaf()) {
        out.put(1);
        out.put(static_cast<char>(node->ch));
        return;
    }
    out.put(2);
    writeTree(out, node->left);
    writeTree(out, node->right);
}

HuffmanNode* readTree(ifstream& in) {
    char marker = in.get();
    if (!in || marker == 0) return nullptr;
    if (marker == 1) {
        unsigned char ch = static_cast<unsigned char>(in.get());
        return new HuffmanNode(ch, 0);
    }
    HuffmanNode* left = readTree(in);
    HuffmanNode* right = readTree(in);
    return new HuffmanNode(left, right);
}

string readFile(const string& path) {
    ifstream in(path, ios::binary);
    if (!in) throw runtime_error("Cannot open " + path);
    return string((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
}

void writeFile(const string& path, const string& data) {
    ofstream out(path, ios::binary);
    if (!out) throw runtime_error("Cannot write " + path);
    out.write(data.data(), static_cast<streamsize>(data.size()));
}

size_t fileSizeBytes(const string& path) {
    ifstream in(path, ios::binary | ios::ate);
    if (!in) return 0;
    return static_cast<size_t>(in.tellg());
}

void displayFrequencies(const unordered_map<unsigned char, unsigned int>& freq) {
    vector<pair<unsigned char, unsigned int>> sorted(freq.begin(), freq.end());
    sort(sorted.begin(), sorted.end(),
         [](const auto& a, const auto& b) { return a.second > b.second; });

    cout << "\n=== Character Frequencies ===\n";
    cout << "Unique characters: " << sorted.size() << "\n";
    for (const auto& entry : sorted) {
        unsigned char ch = entry.first;
        cout << "  ";
        if (ch == '\n') cout << "'\\n'";
        else if (ch == '\r') cout << "'\\r'";
        else if (ch == '\t') cout << "'\\t'";
        else if (isprint(ch)) cout << "'" << static_cast<char>(ch) << "'";
        else cout << "0x" << hex << uppercase << static_cast<int>(ch) << dec << nouppercase;
        cout << " : " << entry.second << "\n";
    }
}

bool compressFile(const string& inputPath, const string& outputPath) {
    string content = readFile(inputPath);
    size_t originalSize = content.size();

    unordered_map<unsigned char, unsigned int> freq;
    for (unsigned char c : content) freq[c]++;

    displayFrequencies(freq);

    HuffmanNode* root = buildHuffmanTree(freq);
    unordered_map<unsigned char, string> codes;
    buildCodes(root, "", codes);

    string bits;
    bits.reserve(content.size() * 2);
    for (unsigned char c : content) {
        auto it = codes.find(c);
        if (it != codes.end()) bits += it->second;
    }

    unsigned char padding = static_cast<unsigned char>((8 - bits.size() % 8) % 8);
    for (int i = 0; i < padding; ++i) bits.push_back('0');

    vector<unsigned char> compressed;
    for (size_t i = 0; i < bits.size(); i += 8) {
        bitset<8> byte(bits.substr(i, 8));
        compressed.push_back(static_cast<unsigned char>(byte.to_ulong()));
    }

    ofstream out(outputPath, ios::binary);
    if (!out) return false;

    uint32_t orig = static_cast<uint32_t>(originalSize);
    out.write(reinterpret_cast<const char*>(&orig), sizeof(orig));
    out.put(static_cast<char>(padding));
    writeTree(out, root);
    out.write(reinterpret_cast<const char*>(compressed.data()),
              static_cast<streamsize>(compressed.size()));
    out.close();

    size_t compressedSize = fileSizeBytes(outputPath);
    double ratio = originalSize == 0 ? 0.0 :
        static_cast<double>(compressedSize) / static_cast<double>(originalSize);
    double spaceSaved = originalSize == 0 ? 0.0 : (1.0 - ratio) * 100.0;

    cout << "\n=== Compression Results ===\n";
    cout << "Original size:      " << originalSize << " bytes\n";
    cout << "Compressed size:    " << compressedSize << " bytes (telemetry.huff)\n";
    if (originalSize == 0) {
        cout << "Compression ratio:  N/A (empty input file)\n";
        cout << "Space saved:        N/A\n";
    } else {
        cout << "Compression ratio:  " << fixed << setprecision(2) << ratio
             << " (" << compressedSize << "/" << originalSize << ")\n";
        cout << "Space saved:        " << setprecision(1) << spaceSaved << "%\n";
    }
    cout << "Huffman tree built with " << freq.size() << " leaf node(s).\n";

    freeTree(root);
    return true;
}

bool decompressFile(const string& inputPath, const string& outputPath) {
    ifstream in(inputPath, ios::binary);
    if (!in) return false;

    uint32_t originalSize = 0;
    in.read(reinterpret_cast<char*>(&originalSize), sizeof(originalSize));
    unsigned char padding = static_cast<unsigned char>(in.get());

    HuffmanNode* root = readTree(in);
    vector<unsigned char> payload((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());

    string result;
    result.reserve(originalSize);

    if (originalSize == 0) {
        writeFile(outputPath, result);
        freeTree(root);
        return true;
    }

    if (!root) return false;

    if (root->isLeaf()) {
        result.assign(originalSize, static_cast<char>(root->ch));
        writeFile(outputPath, result);
        freeTree(root);
        return true;
    }

    string bits;
    for (unsigned char b : payload) {
        bitset<8> byte(b);
        bits += byte.to_string();
    }
    if (padding > 0 && bits.size() >= padding) {
        bits.erase(bits.end() - padding, bits.end());
    }

    HuffmanNode* node = root;
    for (char bit : bits) {
        if (!node) return false;
        node = (bit == '0') ? node->left : node->right;
        if (!node) return false;
        if (node->isLeaf()) {
            result.push_back(static_cast<char>(node->ch));
            node = root;
            if (result.size() >= originalSize) break;
        }
    }

    if (result.size() != originalSize) return false;

    writeFile(outputPath, result);
    freeTree(root);
    return true;
}

bool filesIdentical(const string& a, const string& b) {
    try {
        return readFile(a) == readFile(b);
    } catch (...) {
        return false;
    }
}

int main(int argc, char* argv[]) {
    string input = "telemetry.txt";
    string compressed = "telemetry.huff";
    string restored = "telemetry_restored.txt";

    if (argc > 1) input = argv[1];

    cout << "=== Telemetry Data Compression Utility ===\n";
    cout << "Loading telemetry file: " << input << "\n";

    if (!compressFile(input, compressed)) {
        cerr << "Compression failed.\n";
        return 1;
    }

    cout << "\nDecompressing " << compressed << " ...\n";
    if (!decompressFile(compressed, restored)) {
        cerr << "Decompression failed.\n";
        return 1;
    }

    cout << "Restored output saved to " << restored << "\n";

    if (filesIdentical(input, restored)) {
        cout << "\nVerification: SUCCESS - restored file is identical to the original.\n";
    } else {
        cout << "\nVerification: FAILED - restored file differs from the original.\n";
        return 1;
    }

    return 0;
}
