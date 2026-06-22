#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_AIRPORTS 50
#define MAX_CODE_LEN 8
#define MAX_EDGES 100

typedef struct {
    int to;
} Edge;

typedef struct {
    char airports[MAX_AIRPORTS][MAX_CODE_LEN];
    int airportCount;
    Edge adj[MAX_AIRPORTS][MAX_EDGES];
    int adjCount[MAX_AIRPORTS];
    int matrix[MAX_AIRPORTS][MAX_AIRPORTS];
} DirectedGraph;

static void trimUpper(char *s) {
    char *start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
    for (size_t i = 0; s[i]; i++)
        s[i] = (char)toupper((unsigned char)s[i]);
}

static int findIndex(DirectedGraph *g, const char *code) {
    for (int i = 0; i < g->airportCount; i++)
        if (strcmp(g->airports[i], code) == 0) return i;
    return -1;
}

static void rebuildMatrix(DirectedGraph *g) {
    int n = g->airportCount;
    for (int i = 0; i < n; i++)
        for (int j = 0; j < n; j++)
            g->matrix[i][j] = 0;
    for (int i = 0; i < n; i++)
        for (int k = 0; k < g->adjCount[i]; k++)
            g->matrix[i][g->adj[i][k].to] = 1;
}

static int addAirport(DirectedGraph *g, const char *code, int quiet) {
    char normalized[MAX_CODE_LEN];
    strncpy(normalized, code, MAX_CODE_LEN - 1);
    normalized[MAX_CODE_LEN - 1] = '\0';
    trimUpper(normalized);

    if (normalized[0] == '\0') {
        if (!quiet) printf("Invalid airport code.\n");
        return 0;
    }
    if (g->airportCount >= MAX_AIRPORTS) {
        if (!quiet) printf("Airport limit reached.\n");
        return 0;
    }
    if (findIndex(g, normalized) != -1) {
        if (!quiet) printf("Airport already exists.\n");
        return 0;
    }
    strcpy(g->airports[g->airportCount], normalized);
    g->adjCount[g->airportCount] = 0;
    g->airportCount++;
    rebuildMatrix(g);
    if (!quiet) printf("Airport %s added.\n", normalized);
    return 1;
}

static int removeAirport(DirectedGraph *g, const char *code) {
    char normalized[MAX_CODE_LEN];
    strncpy(normalized, code, MAX_CODE_LEN - 1);
    normalized[MAX_CODE_LEN - 1] = '\0';
    trimUpper(normalized);

    if (normalized[0] == '\0') {
        printf("Invalid airport code.\n");
        return 0;
    }
    int idx = findIndex(g, normalized);
    if (idx == -1) {
        printf("Airport not found.\n");
        return 0;
    }

    for (int i = idx; i < g->airportCount - 1; i++) {
        strcpy(g->airports[i], g->airports[i + 1]);
        g->adjCount[i] = g->adjCount[i + 1];
        for (int k = 0; k < g->adjCount[i]; k++)
            g->adj[i][k] = g->adj[i + 1][k];
    }
    g->airportCount--;

    for (int i = 0; i < g->airportCount; i++) {
        int newCount = 0;
        for (int k = 0; k < g->adjCount[i]; k++) {
            if (g->adj[i][k].to == idx) continue;
            g->adj[i][newCount].to = g->adj[i][k].to;
            if (g->adj[i][newCount].to > idx) g->adj[i][newCount].to--;
            newCount++;
        }
        g->adjCount[i] = newCount;
    }
    rebuildMatrix(g);
    printf("Airport %s removed.\n", normalized);
    return 1;
}

static int addRoute(DirectedGraph *g, const char *from, const char *to, int quiet) {
    char src[MAX_CODE_LEN], dst[MAX_CODE_LEN];
    strncpy(src, from, MAX_CODE_LEN - 1);
    strncpy(dst, to, MAX_CODE_LEN - 1);
    src[MAX_CODE_LEN - 1] = dst[MAX_CODE_LEN - 1] = '\0';
    trimUpper(src);
    trimUpper(dst);

    if (src[0] == '\0' || dst[0] == '\0') {
        if (!quiet) printf("Invalid airport code.\n");
        return 0;
    }
    if (strcmp(src, dst) == 0) {
        if (!quiet) printf("Cannot add a route from an airport to itself.\n");
        return 0;
    }
    int i = findIndex(g, src);
    int j = findIndex(g, dst);
    if (i == -1 || j == -1) {
        if (!quiet) printf("One or both airports not found.\n");
        return 0;
    }
    for (int k = 0; k < g->adjCount[i]; k++)
        if (g->adj[i][k].to == j) {
            if (!quiet) printf("Route already exists.\n");
            return 0;
        }
    g->adj[i][g->adjCount[i]++].to = j;
    rebuildMatrix(g);
    if (!quiet) printf("Route %s -> %s added.\n", src, dst);
    return 1;
}

static int removeRoute(DirectedGraph *g, const char *from, const char *to) {
    char src[MAX_CODE_LEN], dst[MAX_CODE_LEN];
    strncpy(src, from, MAX_CODE_LEN - 1);
    strncpy(dst, to, MAX_CODE_LEN - 1);
    src[MAX_CODE_LEN - 1] = dst[MAX_CODE_LEN - 1] = '\0';
    trimUpper(src);
    trimUpper(dst);

    int i = findIndex(g, src);
    int j = findIndex(g, dst);
    if (i == -1 || j == -1) {
        printf("One or both airports not found.\n");
        return 0;
    }
    int found = 0;
    int newCount = 0;
    for (int k = 0; k < g->adjCount[i]; k++) {
        if (g->adj[i][k].to == j) { found = 1; continue; }
        g->adj[i][newCount++] = g->adj[i][k];
    }
    if (!found) {
        printf("Route not found.\n");
        return 0;
    }
    g->adjCount[i] = newCount;
    rebuildMatrix(g);
    printf("Route %s -> %s removed.\n", src, dst);
    return 1;
}

static void queryAirport(DirectedGraph *g, const char *code) {
    char normalized[MAX_CODE_LEN];
    strncpy(normalized, code, MAX_CODE_LEN - 1);
    normalized[MAX_CODE_LEN - 1] = '\0';
    trimUpper(normalized);

    int idx = findIndex(g, normalized);
    if (idx == -1) {
        printf("Airport not found.\n");
        return;
    }

    printf("\n=== Airport: %s ===\n", normalized);
    printf("Direct flights OUT to: ");
    if (g->adjCount[idx] == 0) printf("(none)");
    else {
        for (int k = 0; k < g->adjCount[idx]; k++) {
            if (k > 0) printf(", ");
            printf("%s", g->airports[g->adj[idx][k].to]);
        }
    }
    printf("\nDirect flights IN from: ");
    int found = 0;
    for (int i = 0; i < g->airportCount; i++) {
        for (int k = 0; k < g->adjCount[i]; k++) {
            if (g->adj[i][k].to == idx) {
                if (found) printf(", ");
                printf("%s", g->airports[i]);
                found = 1;
            }
        }
    }
    if (!found) printf("(none)");
    printf("\n");
}

static void displayMatrix(DirectedGraph *g) {
    int n = g->airportCount;
    if (n == 0) {
        printf("No airports in graph.\n");
        return;
    }
    printf("\n=== Adjacency Matrix ===\n");
    printf("%6s", "");
    for (int j = 0; j < n; j++) printf("%6s", g->airports[j]);
    printf("\n");
    for (int i = 0; i < n; i++) {
        printf("%6s", g->airports[i]);
        for (int j = 0; j < n; j++)
            printf("%6d", g->matrix[i][j]);
        printf("\n");
    }
}

static void displayAllRoutes(DirectedGraph *g) {
    printf("\n=== All Routes ===\n");
    int any = 0;
    for (int i = 0; i < g->airportCount; i++) {
        for (int k = 0; k < g->adjCount[i]; k++) {
            printf("%s -> %s\n", g->airports[i], g->airports[g->adj[i][k].to]);
            any = 1;
        }
    }
    if (!any) printf("(no routes)\n");
}

static int loadSampleData(DirectedGraph *g) {
    const char *from[] = {"KGL", "KGL", "NBO", "EBB", "ADD", "JNB"};
    const char *to[] = {"NBO", "EBB", "JNB", "ADD", "CAI", "CPT"};
    for (int i = 0; i < 6; i++) {
        addAirport(g, from[i], 1);
        addAirport(g, to[i], 1);
        addRoute(g, from[i], to[i], 1);
    }
    return 6;
}

static void printMenu(void) {
    printf("\n=== Airline Route Relationship Analyzer ===\n");
    printf("1. Query airport (incoming/outgoing)\n");
    printf("2. Add airport\n");
    printf("3. Remove airport\n");
    printf("4. Add route\n");
    printf("5. Remove route\n");
    printf("6. Display adjacency matrix\n");
    printf("7. Display all routes\n");
    printf("0. Quit\n");
    printf("Choice: ");
}

int main(void) {
    DirectedGraph graph;
    memset(&graph, 0, sizeof(graph));
    int routeCount = loadSampleData(&graph);

    printf("=== Airline Route Relationship Analyzer ===\n");
    printf("Sample network loaded: 7 airports, %d direct routes.\n", routeCount);

    int choice;
    char a[32], b[32];
    while (1) {
        printMenu();
        if (scanf("%d", &choice) != 1) break;
        getchar();

        if (choice == 0) break;
        switch (choice) {
            case 1:
                printf("Airport code: ");
                fgets(a, sizeof(a), stdin);
                a[strcspn(a, "\r\n")] = '\0';
                queryAirport(&graph, a);
                break;
            case 2:
                printf("New airport code: ");
                fgets(a, sizeof(a), stdin);
                a[strcspn(a, "\r\n")] = '\0';
                addAirport(&graph, a, 0);
                break;
            case 3:
                printf("Airport to remove: ");
                fgets(a, sizeof(a), stdin);
                a[strcspn(a, "\r\n")] = '\0';
                removeAirport(&graph, a);
                break;
            case 4:
                printf("From: ");
                fgets(a, sizeof(a), stdin);
                a[strcspn(a, "\r\n")] = '\0';
                printf("To: ");
                fgets(b, sizeof(b), stdin);
                b[strcspn(b, "\r\n")] = '\0';
                addRoute(&graph, a, b, 0);
                break;
            case 5:
                printf("From: ");
                fgets(a, sizeof(a), stdin);
                a[strcspn(a, "\r\n")] = '\0';
                printf("To: ");
                fgets(b, sizeof(b), stdin);
                b[strcspn(b, "\r\n")] = '\0';
                removeRoute(&graph, a, b);
                break;
            case 6:
                displayMatrix(&graph);
                break;
            case 7:
                displayAllRoutes(&graph);
                break;
            default:
                printf("Invalid choice.\n");
        }
    }

    printf("Exiting analyzer.\n");
    return 0;
}
