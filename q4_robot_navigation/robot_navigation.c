#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <limits.h>

#define NUM_BUILDINGS 7
#define MAX_EDGES 8
#define MAX_NAME 32
#define INF (INT_MAX / 4)

static const char *BUILDINGS[] = {
    "Library", "Cafeteria", "Engineering", "Science Block",
    "Dormitory", "Administration", "Charging Station"
};

typedef struct {
    int to;
    int weight;
} Edge;

static int adjCount[NUM_BUILDINGS];
static Edge adj[NUM_BUILDINGS][MAX_EDGES];

static void trim(char *s) {
    char *start = s;
    while (*start && isspace((unsigned char)*start)) start++;
    if (start != s) memmove(s, start, strlen(start) + 1);
    size_t len = strlen(s);
    while (len > 0 && isspace((unsigned char)s[len - 1])) s[--len] = '\0';
}

static int equalsIgnoreCase(const char *a, const char *b) {
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

static int findIndex(const char *name) {
    char query[MAX_NAME];
    strncpy(query, name, MAX_NAME - 1);
    query[MAX_NAME - 1] = '\0';
    trim(query);
    if (query[0] == '\0') return -1;
    for (int i = 0; i < NUM_BUILDINGS; i++)
        if (equalsIgnoreCase(query, BUILDINGS[i])) return i;
    return -1;
}

static void addEdge(const char *u, const char *v, int w) {
    int i = findIndex(u);
    int j = findIndex(v);
    adj[i][adjCount[i]].to = j;
    adj[i][adjCount[i]].weight = w;
    adjCount[i]++;
    adj[j][adjCount[j]].to = i;
    adj[j][adjCount[j]].weight = w;
    adjCount[j]++;
}

static void initCampus(void) {
    memset(adjCount, 0, sizeof(adjCount));
    addEdge("Library", "Cafeteria", 6);
    addEdge("Library", "Engineering", 15);
    addEdge("Cafeteria", "Science Block", 4);
    addEdge("Science Block", "Dormitory", 8);
    addEdge("Engineering", "Administration", 5);
    addEdge("Administration", "Dormitory", 3);
    addEdge("Cafeteria", "Charging Station", 2);
    addEdge("Charging Station", "Administration", 4);
}

static void listBuildings(void) {
    printf("Available buildings:\n");
    for (int i = 0; i < NUM_BUILDINGS; i++)
        printf("  %d. %s\n", i + 1, BUILDINGS[i]);
}

static int dijkstra(int src, int dest, int dist[], int parent[]) {
    int visited[NUM_BUILDINGS] = {0};
    for (int i = 0; i < NUM_BUILDINGS; i++) {
        dist[i] = INF;
        parent[i] = -1;
    }
    dist[src] = 0;

    for (int count = 0; count < NUM_BUILDINGS; count++) {
        int u = -1, best = INF;
        for (int i = 0; i < NUM_BUILDINGS; i++) {
            if (!visited[i] && dist[i] < best) {
                best = dist[i];
                u = i;
            }
        }
        if (u == -1 || best == INF) break;
        visited[u] = 1;
        for (int k = 0; k < adjCount[u]; k++) {
            int v = adj[u][k].to;
            int w = adj[u][k].weight;
            if (!visited[v] && dist[u] + w < dist[v]) {
                dist[v] = dist[u] + w;
                parent[v] = u;
            }
        }
    }
    return dist[dest] != INF;
}

static void displayRoute(const char *start) {
    char trimmed[MAX_NAME];
    strncpy(trimmed, start, MAX_NAME - 1);
    trimmed[MAX_NAME - 1] = '\0';
    trim(trimmed);

    if (trimmed[0] == '\0') {
        printf("Error: No starting building entered.\n");
        listBuildings();
        return;
    }

    int startIdx = findIndex(trimmed);
    if (startIdx == -1) {
        printf("Error: Invalid building name '%s'.\n", trimmed);
        printf("Please choose from the list of valid campus buildings.\n");
        listBuildings();
        return;
    }

    int destIdx = findIndex("Dormitory");
    int dist[NUM_BUILDINGS], parent[NUM_BUILDINGS];
    if (!dijkstra(startIdx, destIdx, dist, parent)) {
        printf("No route found from %s to Dormitory.\n", BUILDINGS[startIdx]);
        return;
    }

    int path[NUM_BUILDINGS];
    int pathLen = 0;
    for (int at = destIdx; at != -1; at = parent[at])
        path[pathLen++] = at;

    printf("\n=== Shortest Route to Dormitory ===\n");
    printf("Start: %s\n", BUILDINGS[startIdx]);
    printf("Path: ");
    for (int i = pathLen - 1; i >= 0; i--) {
        if (i < pathLen - 1) printf(" -> ");
        printf("%s", BUILDINGS[path[i]]);
    }
    printf("\nTotal travel distance: %d units\n", dist[destIdx]);
}

int main(void) {
    initCampus();
    printf("=== Campus Delivery Robot Navigation ===\n");
    listBuildings();

    char start[MAX_NAME];
    printf("\nEnter starting building name: ");
    if (!fgets(start, sizeof(start), stdin)) return 1;
    start[strcspn(start, "\r\n")] = '\0';

    displayRoute(start);
    return 0;
}
