#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define MAX_PROCEDURES 50
#define MAX_NAME_LEN 64
#define MAX_SUGGEST_DISTANCE 3

static void trimUpper(char *s) {
    char *start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
    for (size_t i = 0; s[i]; i++)
        s[i] = (char)toupper((unsigned char)s[i]);
}

static int levenshtein(const char *a, const char *b) {
    int n = (int)strlen(a);
    int m = (int)strlen(b);
    int *dp = (int *)calloc((n + 1) * (m + 1), sizeof(int));
    if (!dp) return 999;
    for (int i = 0; i <= n; i++) dp[i * (m + 1)] = i;
    for (int j = 0; j <= m; j++) dp[j] = j;
    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= m; j++) {
            int cost = (a[i - 1] == b[j - 1]) ? 0 : 1;
            int idx = i * (m + 1) + j;
            int del = dp[(i - 1) * (m + 1) + j] + 1;
            int ins = dp[i * (m + 1) + (j - 1)] + 1;
            int sub = dp[(i - 1) * (m + 1) + (j - 1)] + cost;
            dp[idx] = del < ins ? del : ins;
            if (sub < dp[idx]) dp[idx] = sub;
        }
    }
    int result = dp[n * (m + 1) + m];
    free(dp);
    return result;
}

typedef struct BSTNode {
    char procedure[MAX_NAME_LEN];
    struct BSTNode *left;
    struct BSTNode *right;
} BSTNode;

typedef struct {
    BSTNode *root;
    int count;
} ProcedureBST;

static void destroyTree(BSTNode *node) {
    if (!node) return;
    destroyTree(node->left);
    destroyTree(node->right);
    free(node);
}

static BSTNode *searchNode(BSTNode *node, const char *procedure) {
    if (!node) return NULL;
    int cmp = strcmp(procedure, node->procedure);
    if (cmp == 0) return node;
    if (cmp < 0) return searchNode(node->left, procedure);
    return searchNode(node->right, procedure);
}

static BSTNode *insertNode(BSTNode *node, const char *procedure) {
    if (!node) {
        BSTNode *n = (BSTNode *)malloc(sizeof(BSTNode));
        strncpy(n->procedure, procedure, MAX_NAME_LEN - 1);
        n->left = n->right = NULL;
        return n;
    }
    int cmp = strcmp(procedure, node->procedure);
    if (cmp < 0) node->left = insertNode(node->left, procedure);
    else if (cmp > 0) node->right = insertNode(node->right, procedure);
    return node;
}

static void collectAll(BSTNode *node, char list[][MAX_NAME_LEN], int *idx) {
    if (!node) return;
    collectAll(node->left, list, idx);
    if (*idx < MAX_PROCEDURES) {
        strncpy(list[*idx], node->procedure, MAX_NAME_LEN - 1);
        (*idx)++;
    }
    collectAll(node->right, list, idx);
}

static int bstInsert(ProcedureBST *tree, const char *procedure) {
    if (tree->count >= MAX_PROCEDURES) return 0;
    if (searchNode(tree->root, procedure)) return 0;
    tree->root = insertNode(tree->root, procedure);
    tree->count++;
    return 1;
}

static int bstContains(ProcedureBST *tree, const char *procedure) {
    return searchNode(tree->root, procedure) != NULL;
}

static void findClosest(ProcedureBST *tree, const char *input, char *out) {
    char list[MAX_PROCEDURES][MAX_NAME_LEN];
    int idx = 0;
    out[0] = '\0';
    collectAll(tree->root, list, &idx);
    if (idx == 0) return;

    int bestDist = levenshtein(input, list[0]);
    strcpy(out, list[0]);
    for (int i = 1; i < idx; i++) {
        int dist = levenshtein(input, list[i]);
        if (dist < bestDist) {
            bestDist = dist;
            strcpy(out, list[i]);
        }
    }
}

static void displayInOrder(ProcedureBST *tree) {
    char list[MAX_PROCEDURES][MAX_NAME_LEN];
    int idx = 0;
    collectAll(tree->root, list, &idx);
    printf("Approved procedures (%d):\n", idx);
    for (int i = 0; i < idx; i++)
        printf("  - %s\n", list[i]);
}

static int loadProcedures(const char *filename, ProcedureBST *tree) {
    FILE *in = fopen(filename, "r");
    if (!in) {
        fprintf(stderr, "Error: cannot open %s\n", filename);
        return 0;
    }
    char line[MAX_NAME_LEN];
    while (fgets(line, sizeof(line), in)) {
        line[strcspn(line, "\r\n")] = '\0';
        trimUpper(line);
        if (line[0] == '\0') continue;
        if (!bstInsert(tree, line))
            fprintf(stderr, "Warning: limit reached or duplicate: %s\n", line);
    }
    fclose(in);
    return 1;
}

static void logAudit(const char *attempt) {
    FILE *log = fopen("audit.log", "a");
    if (!log) {
        fprintf(stderr, "Warning: unable to write audit.log\n");
        return;
    }
    time_t t = time(NULL);
    char buf[32];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
    fprintf(log, "[%s] REJECTED: %s\n", buf, attempt);
    fclose(log);
}

static void verifyProcedure(ProcedureBST *tree, const char *input) {
    char normalized[MAX_NAME_LEN];
    strncpy(normalized, input, MAX_NAME_LEN - 1);
    normalized[MAX_NAME_LEN - 1] = '\0';
    trimUpper(normalized);

    if (normalized[0] == '\0') {
        printf("Empty entry ignored.\n");
        return;
    }
    if (bstContains(tree, normalized)) {
        printf("APPROVED: Procedure '%s' is authorized for execution.\n", normalized);
        return;
    }

    char closest[MAX_NAME_LEN];
    findClosest(tree, normalized, closest);
    int dist = closest[0] ? levenshtein(normalized, closest) : 999;

    if (closest[0] && dist > 0 && dist <= MAX_SUGGEST_DISTANCE) {
        printf("SIMILAR: Did you mean '%s'? (edit distance: %d)\n", closest, dist);
        return;
    }

    printf("REJECTED: Unknown procedure '%s'.\n", normalized);
    logAudit(normalized);
}

int main(int argc, char *argv[]) {
    const char *filename = "procedures.txt";
    if (argc > 1) filename = argv[1];

    ProcedureBST tree = {NULL, 0};
    if (!loadProcedures(filename, &tree)) return 1;

    printf("=== Secure Maintenance Procedure Validator ===\n");
    displayInOrder(&tree);
    printf("\nEnter procedure name to verify (or 'quit' to exit):\n");

    char input[128];
    while (1) {
        printf("> ");
        if (!fgets(input, sizeof(input), stdin)) break;
        input[strcspn(input, "\r\n")] = '\0';
        if (strcmp(input, "quit") == 0 || strcmp(input, "QUIT") == 0 || strcmp(input, "q") == 0)
            break;
        verifyProcedure(&tree, input);
    }

    printf("Validator shutting down. Memory released.\n");
    destroyTree(tree.root);
    return 0;
}
