#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>

typedef struct HuffmanNode {
    unsigned char ch;
    unsigned int freq;
    struct HuffmanNode *left;
    struct HuffmanNode *right;
} HuffmanNode;

typedef struct {
    HuffmanNode **data;
    int size;
    int cap;
} MinHeap;

static int isLeaf(HuffmanNode *n) {
    return n && !n->left && !n->right;
}

static HuffmanNode *newNode(unsigned char ch, unsigned int freq) {
    HuffmanNode *n = (HuffmanNode *)malloc(sizeof(HuffmanNode));
    n->ch = ch;
    n->freq = freq;
    n->left = n->right = NULL;
    return n;
}

static void freeTree(HuffmanNode *node) {
    if (!node) return;
    freeTree(node->left);
    freeTree(node->right);
    free(node);
}

static void heapPush(MinHeap *h, HuffmanNode *n) {
    if (h->size >= h->cap) {
        h->cap = h->cap ? h->cap * 2 : 16;
        h->data = (HuffmanNode **)realloc(h->data, h->cap * sizeof(HuffmanNode *));
    }
    int i = h->size++;
    h->data[i] = n;
    while (i > 0) {
        int p = (i - 1) / 2;
        if (h->data[p]->freq <= h->data[i]->freq) break;
        HuffmanNode *tmp = h->data[p];
        h->data[p] = h->data[i];
        h->data[i] = tmp;
        i = p;
    }
}

static HuffmanNode *heapPop(MinHeap *h) {
    HuffmanNode *root = h->data[0];
    h->data[0] = h->data[--h->size];
    int i = 0;
    while (1) {
        int l = 2 * i + 1, r = 2 * i + 2, smallest = i;
        if (l < h->size && h->data[l]->freq < h->data[smallest]->freq) smallest = l;
        if (r < h->size && h->data[r]->freq < h->data[smallest]->freq) smallest = r;
        if (smallest == i) break;
        HuffmanNode *tmp = h->data[i];
        h->data[i] = h->data[smallest];
        h->data[smallest] = tmp;
        i = smallest;
    }
    return root;
}

static HuffmanNode *buildTree(unsigned int freq[256], int unique) {
    MinHeap h = {NULL, 0, 0};
    for (int i = 0; i < 256; i++)
        if (freq[i] > 0) heapPush(&h, newNode((unsigned char)i, freq[i]));
    if (h.size == 0) { free(h.data); return NULL; }
    if (h.size == 1) {
        HuffmanNode *only = heapPop(&h);
        free(h.data);
        return only;
    }
    while (h.size > 1) {
        HuffmanNode *l = heapPop(&h);
        HuffmanNode *r = heapPop(&h);
        HuffmanNode *p = (HuffmanNode *)malloc(sizeof(HuffmanNode));
        p->ch = 0;
        p->freq = l->freq + r->freq;
        p->left = l;
        p->right = r;
        heapPush(&h, p);
    }
    HuffmanNode *root = heapPop(&h);
    free(h.data);
    return root;
}

static void buildCodes(HuffmanNode *node, char *prefix, int depth,
                       char codes[256][256], int codeLen[256]) {
    if (!node) return;
    if (isLeaf(node)) {
        if (depth == 0) {
            codes[node->ch][0] = '0';
            codes[node->ch][1] = '\0';
            codeLen[node->ch] = 1;
        } else {
            prefix[depth] = '\0';
            strcpy(codes[node->ch], prefix);
            codeLen[node->ch] = depth;
        }
        return;
    }
    prefix[depth] = '0';
    buildCodes(node->left, prefix, depth + 1, codes, codeLen);
    prefix[depth] = '1';
    buildCodes(node->right, prefix, depth + 1, codes, codeLen);
}

static void writeTree(FILE *out, HuffmanNode *node) {
    if (!node) { fputc(0, out); return; }
    if (isLeaf(node)) {
        fputc(1, out);
        fputc((int)node->ch, out);
        return;
    }
    fputc(2, out);
    writeTree(out, node->left);
    writeTree(out, node->right);
}

static HuffmanNode *readTree(FILE *in) {
    int marker = fgetc(in);
    if (marker == EOF || marker == 0) return NULL;
    if (marker == 1) {
        int ch = fgetc(in);
        return newNode((unsigned char)ch, 0);
    }
    HuffmanNode *left = readTree(in);
    HuffmanNode *right = readTree(in);
    HuffmanNode *p = (HuffmanNode *)malloc(sizeof(HuffmanNode));
    p->ch = 0;
    p->freq = 0;
    p->left = left;
    p->right = right;
    return p;
}

static unsigned char *readFile(const char *path, size_t *outSize) {
    FILE *in = fopen(path, "rb");
    if (!in) return NULL;
    fseek(in, 0, SEEK_END);
    long sz = ftell(in);
    fseek(in, 0, SEEK_SET);
    if (sz < 0) { fclose(in); return NULL; }
    unsigned char *buf = (unsigned char *)malloc((size_t)sz + 1);
    if (!buf) { fclose(in); return NULL; }
    fread(buf, 1, (size_t)sz, in);
    fclose(in);
    *outSize = (size_t)sz;
    return buf;
}

static void displayFrequencies(unsigned int freq[256]) {
    int unique = 0;
    for (int i = 0; i < 256; i++) if (freq[i] > 0) unique++;
    printf("\n=== Character Frequencies ===\n");
    printf("Unique characters: %d\n", unique);
    for (int i = 0; i < 256; i++) {
        if (freq[i] == 0) continue;
        printf("  ");
        if (i == '\n') printf("'\\n'");
        else if (i == '\r') printf("'\\r'");
        else if (i == '\t') printf("'\\t'");
        else if (isprint(i)) printf("'%c'", i);
        else printf("0x%02X", i);
        printf(" : %u\n", freq[i]);
    }
}

static unsigned char bitsToByte(const char *bits) {
    unsigned char b = 0;
    for (int i = 0; i < 8; i++)
        if (bits[i] == '1') b = (unsigned char)((b << 1) | 1);
        else b = (unsigned char)(b << 1);
    return b;
}

static int compressFile(const char *inputPath, const char *outputPath) {
    size_t originalSize = 0;
    unsigned char *content = readFile(inputPath, &originalSize);
    if (!content && originalSize > 0) return 0;

    unsigned int freq[256] = {0};
    for (size_t i = 0; i < originalSize; i++) freq[content[i]]++;
    displayFrequencies(freq);

    HuffmanNode *root = buildTree(freq, 256);
    char codes[256][256];
    int codeLen[256] = {0};
    char prefix[256];
    buildCodes(root, prefix, 0, codes, codeLen);

    char *bits = NULL;
    size_t bitsLen = 0, bitsCap = 0;
    for (size_t i = 0; i < originalSize; i++) {
        unsigned char c = content[i];
        int len = codeLen[c];
        if (bitsLen + (size_t)len + 1 > bitsCap) {
            bitsCap = bitsCap ? bitsCap * 2 : originalSize * 16 + 64;
            bits = (char *)realloc(bits, bitsCap);
        }
        memcpy(bits + bitsLen, codes[c], (size_t)len);
        bitsLen += (size_t)len;
    }
    free(content);

    unsigned char padding = (unsigned char)((8 - bitsLen % 8) % 8);
    for (int i = 0; i < padding; i++) {
        if (bitsLen + 2 > bitsCap) bits = (char *)realloc(bits, bitsCap *= 2);
        bits[bitsLen++] = '0';
    }

    FILE *out = fopen(outputPath, "wb");
    if (!out) { free(bits); freeTree(root); return 0; }

    uint32_t orig = (uint32_t)originalSize;
    fwrite(&orig, sizeof(orig), 1, out);
    fputc((int)padding, out);
    writeTree(out, root);

    for (size_t i = 0; i < bitsLen; i += 8)
        fputc((int)bitsToByte(bits + i), out);
    fclose(out);
    free(bits);

    FILE *chk = fopen(outputPath, "rb");
    fseek(chk, 0, SEEK_END);
    long compressedSize = ftell(chk);
    fclose(chk);

    printf("\n=== Compression Results ===\n");
    printf("Original size:      %zu bytes\n", originalSize);
    printf("Compressed size:    %ld bytes (telemetry.huff)\n", compressedSize);
    if (originalSize == 0) {
        printf("Compression ratio:  N/A (empty input file)\n");
        printf("Space saved:        N/A\n");
    } else {
        double ratio = (double)compressedSize / (double)originalSize;
        printf("Compression ratio:  %.2f (%ld/%zu)\n", ratio, compressedSize, originalSize);
        printf("Space saved:        %.1f%%\n", (1.0 - ratio) * 100.0);
    }

    int unique = 0;
    for (int i = 0; i < 256; i++) if (freq[i] > 0) unique++;
    printf("Huffman tree built with %d leaf node(s).\n", unique);

    freeTree(root);
    return 1;
}

static int decompressFile(const char *inputPath, const char *outputPath) {
    FILE *in = fopen(inputPath, "rb");
    if (!in) return 0;

    uint32_t originalSize = 0;
    if (fread(&originalSize, sizeof(originalSize), 1, in) != 1) { fclose(in); return 0; }
    int padding = fgetc(in);

    HuffmanNode *root = readTree(in);
    unsigned char *payload = NULL;
    size_t payloadSize = 0;
    {
        long start = ftell(in);
        fseek(in, 0, SEEK_END);
        long end = ftell(in);
        payloadSize = (size_t)(end - start);
        fseek(in, start, SEEK_SET);
        payload = (unsigned char *)malloc(payloadSize + 1);
        fread(payload, 1, payloadSize, in);
    }
    fclose(in);

    unsigned char *result = (unsigned char *)calloc(originalSize + 1, 1);
    if (!result) { free(payload); freeTree(root); return 0; }

    if (originalSize == 0) {
        FILE *out = fopen(outputPath, "wb");
        fclose(out);
        free(result); free(payload); freeTree(root);
        return 1;
    }
    if (!root) { free(result); free(payload); return 0; }

    if (isLeaf(root)) {
        memset(result, root->ch, originalSize);
    } else {
        char *bits = (char *)malloc(payloadSize * 8 + 1);
        size_t bitsLen = 0;
        for (size_t i = 0; i < payloadSize; i++) {
            for (int b = 7; b >= 0; b--)
                bits[bitsLen++] = (payload[i] & (1 << b)) ? '1' : '0';
        }
        bits[bitsLen] = '\0';
        if (padding > 0 && bitsLen >= (size_t)padding)
            bitsLen -= (size_t)padding;

        HuffmanNode *node = root;
        size_t outIdx = 0;
        for (size_t i = 0; i < bitsLen && outIdx < originalSize; i++) {
            node = (bits[i] == '0') ? node->left : node->right;
            if (!node) break;
            if (isLeaf(node)) {
                result[outIdx++] = node->ch;
                node = root;
            }
        }
        free(bits);
    }

    FILE *out = fopen(outputPath, "wb");
    fwrite(result, 1, originalSize, out);
    fclose(out);

    free(result);
    free(payload);
    freeTree(root);
    return 1;
}

static int filesIdentical(const char *a, const char *b) {
    size_t sa = 0, sb = 0;
    unsigned char *fa = readFile(a, &sa);
    unsigned char *fb = readFile(b, &sb);
    if (!fa || !fb) { free(fa); free(fb); return 0; }
    int same = (sa == sb) && memcmp(fa, fb, sa) == 0;
    free(fa); free(fb);
    return same;
}

int main(int argc, char *argv[]) {
    const char *input = "telemetry.txt";
    const char *compressed = "telemetry.huff";
    const char *restored = "telemetry_restored.txt";
    if (argc > 1) input = argv[1];

    printf("=== Telemetry Data Compression Utility ===\n");
    printf("Loading telemetry file: %s\n", input);

    if (!compressFile(input, compressed)) {
        fprintf(stderr, "Compression failed.\n");
        return 1;
    }

    printf("\nDecompressing %s ...\n", compressed);
    if (!decompressFile(compressed, restored)) {
        fprintf(stderr, "Decompression failed.\n");
        return 1;
    }

    printf("Restored output saved to %s\n", restored);
    if (filesIdentical(input, restored))
        printf("\nVerification: SUCCESS - restored file is identical to the original.\n");
    else {
        printf("\nVerification: FAILED - restored file differs from the original.\n");
        return 1;
    }
    return 0;
}
