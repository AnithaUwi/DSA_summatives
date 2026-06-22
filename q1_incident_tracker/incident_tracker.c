#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <pthread.h>

#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

#define MAX_INCIDENTS 25
#define LIVE_INTERVAL_SECONDS 8

typedef struct {
    int id;
    char source[64];
    char description[256];
    char timestamp[32];
} Incident;

typedef struct Node {
    Incident data;
    struct Node *prev;
    struct Node *next;
} Node;

typedef struct {
    Node *head;
    Node *tail;
    Node *current;
    int count;
    int nextId;
    int liveMonitoring;
    pthread_t monitorThread;
    pthread_mutex_t listMutex;
} IncidentTracker;

static void freeList(IncidentTracker *t) {
    Node *node = t->head;
    while (node) {
        Node *next = node->next;
        free(node);
        node = next;
    }
    t->head = t->tail = t->current = NULL;
    t->count = 0;
}

static void nowTimestamp(char *buf, size_t size) {
    time_t now = time(NULL);
    struct tm *lt = localtime(&now);
    strftime(buf, size, "%Y-%m-%d %H:%M:%S", lt);
}

static int appendIncident(IncidentTracker *t, Incident *incident) {
    int evicted = 0;
    int evictedId = 0;
    int newId;

    pthread_mutex_lock(&t->listMutex);
    incident->id = t->nextId++;
    newId = incident->id;

    Node *node = (Node *)malloc(sizeof(Node));
    node->data = *incident;
    node->prev = node->next = NULL;

    if (!t->head) {
        t->head = t->tail = t->current = node;
    } else {
        t->tail->next = node;
        node->prev = t->tail;
        t->tail = node;
        if (!t->current) t->current = t->head;
    }
    t->count++;

    if (t->count > MAX_INCIDENTS) {
        Node *oldest = t->head;
        evictedId = oldest->data.id;
        evicted = 1;
        t->head = t->head->next;
        if (t->head) t->head->prev = NULL;
        else t->tail = NULL;
        if (t->current == oldest) t->current = t->head;
        free(oldest);
        t->count--;
    }
    pthread_mutex_unlock(&t->listMutex);

    if (evicted) {
        printf("\n[CAPACITY] Oldest incident #%d evicted (limit: %d incidents).\n> ",
               evictedId, MAX_INCIDENTS);
        fflush(stdout);
    }
    return newId;
}

static void *monitorLoop(void *arg) {
    IncidentTracker *t = (IncidentTracker *)arg;
    const char *sources[] = {"Ambulance", "Police", "Fire Station"};
    const char *descriptions[] = {
        "Medical emergency reported",
        "Traffic accident on main road",
        "Structure fire detected",
        "Suspicious activity reported",
        "Gas leak suspected",
        "Power outage in sector 4",
        "Flooding near bridge",
        "Wildlife on highway"
    };
    int idx = 0;

    while (t->liveMonitoring) {
        Incident incident;
        strncpy(incident.source, sources[idx % 3], sizeof(incident.source) - 1);
        strncpy(incident.description, descriptions[idx % 8], sizeof(incident.description) - 1);
        nowTimestamp(incident.timestamp, sizeof(incident.timestamp));
        idx++;

        int newId = appendIncident(t, &incident);
        printf("\n[LIVE] New incident #%d from %s: %s (%s)\n> ",
               newId, incident.source, incident.description, incident.timestamp);
        fflush(stdout);

        for (int i = 0; i < LIVE_INTERVAL_SECONDS && t->liveMonitoring; i++) {
#ifdef _WIN32
            Sleep(1000);
#else
            sleep(1);
#endif
        }
    }
    return NULL;
}

static void trackerInit(IncidentTracker *t) {
    memset(t, 0, sizeof(*t));
    pthread_mutex_init(&t->listMutex, NULL);
    t->nextId = 1;
}

static void trackerDestroy(IncidentTracker *t) {
    if (t->liveMonitoring) {
        t->liveMonitoring = 0;
        pthread_join(t->monitorThread, NULL);
    }
    freeList(t);
    pthread_mutex_destroy(&t->listMutex);
}

static void initialize(IncidentTracker *t) {
    freeList(t);
    t->nextId = 1;
    t->current = NULL;

    FILE *in = fopen("incidents_seed.txt", "r");
    if (!in) return;

    char line[512];
    while (fgets(line, sizeof(line), in)) {
        char *p1 = strchr(line, '|');
        if (!p1) continue;
        char *p2 = strchr(p1 + 1, '|');
        if (!p2) continue;
        *p1 = '\0';
        *p2 = '\0';

        Incident incident;
        strncpy(incident.source, line, sizeof(incident.source) - 1);
        strncpy(incident.description, p1 + 1, sizeof(incident.description) - 1);
        strncpy(incident.timestamp, p2 + 1, sizeof(incident.timestamp) - 1);
        incident.timestamp[strcspn(incident.timestamp, "\r\n")] = '\0';

        appendIncident(t, &incident);
    }
    fclose(in);
    t->current = t->head;
}

static int distanceFromOldest(IncidentTracker *t) {
    int pos = 1;
    Node *node = t->head;
    while (node && node != t->current) {
        node = node->next;
        pos++;
    }
    return pos;
}

static void displayCurrent(IncidentTracker *t) {
    pthread_mutex_lock(&t->listMutex);
    if (!t->current) {
        printf("No incidents recorded.\n");
        pthread_mutex_unlock(&t->listMutex);
        return;
    }
    printf("Incident #%d\n", t->current->data.id);
    printf("Source: %s\n", t->current->data.source);
    printf("Description: %s\n", t->current->data.description);
    printf("Time: %s\n", t->current->data.timestamp);
    printf("Position: %d of %d\n", distanceFromOldest(t), t->count);
    pthread_mutex_unlock(&t->listMutex);
}

static void viewNewer(IncidentTracker *t) {
    pthread_mutex_lock(&t->listMutex);
    if (!t->current || !t->current->next) {
        printf("Already at the newest incident.\n");
    } else {
        t->current = t->current->next;
    }
    pthread_mutex_unlock(&t->listMutex);
}

static void viewOlder(IncidentTracker *t) {
    pthread_mutex_lock(&t->listMutex);
    if (!t->current || !t->current->prev) {
        printf("Already at the oldest incident.\n");
    } else {
        t->current = t->current->prev;
    }
    pthread_mutex_unlock(&t->listMutex);
}

static void startLiveMonitoring(IncidentTracker *t) {
    if (t->liveMonitoring) {
        printf("Live monitoring is already active.\n");
        return;
    }
    t->liveMonitoring = 1;
    pthread_create(&t->monitorThread, NULL, monitorLoop, t);
    printf("Live monitoring enabled. New incidents will arrive automatically.\n");
}

static void stopLiveMonitoring(IncidentTracker *t) {
    if (!t->liveMonitoring) return;
    t->liveMonitoring = 0;
    pthread_join(t->monitorThread, NULL);
    printf("Live monitoring stopped.\n");
}

static void deleteAll(IncidentTracker *t) {
    if (t->liveMonitoring) {
        printf("Stopping live monitoring before clearing incidents...\n");
        stopLiveMonitoring(t);
    }
    pthread_mutex_lock(&t->listMutex);
    freeList(t);
    pthread_mutex_unlock(&t->listMutex);
    printf("All incidents deleted.\n");
}

static void saveSession(IncidentTracker *t, const char *filename) {
    pthread_mutex_lock(&t->listMutex);
    FILE *out = fopen(filename, "w");
    if (!out) {
        printf("Failed to save session.\n");
        pthread_mutex_unlock(&t->listMutex);
        return;
    }
    Node *node = t->head;
    while (node) {
        fprintf(out, "%d|%s|%s|%s\n",
                node->data.id, node->data.source,
                node->data.description, node->data.timestamp);
        node = node->next;
    }
    fclose(out);
    printf("Session saved to %s (%d incidents).\n", filename, t->count);
    pthread_mutex_unlock(&t->listMutex);
}

static void printMenu(void) {
    printf("\n=== Emergency Dispatch Incident Tracker ===\n");
    printf("f - View newer incident\n");
    printf("b - View older incident\n");
    printf("l - Enable live incident monitoring\n");
    printf("s - Stop live monitoring\n");
    printf("d - Delete all incidents\n");
    printf("q - Save session and quit\n");
    printf("Command: ");
}

int main(void) {
    IncidentTracker tracker;
    trackerInit(&tracker);
    initialize(&tracker);

    printf("Incident tracker initialized.\n");
    displayCurrent(&tracker);

    char cmd;
    int running = 1;
    while (running) {
        printMenu();
        if (scanf(" %c", &cmd) != 1) break;
        int ch;
        while ((ch = getchar()) != '\n' && ch != EOF) {}

        switch (cmd) {
            case 'f': viewNewer(&tracker); displayCurrent(&tracker); break;
            case 'b': viewOlder(&tracker); displayCurrent(&tracker); break;
            case 'l': startLiveMonitoring(&tracker); break;
            case 's': stopLiveMonitoring(&tracker); break;
            case 'd': deleteAll(&tracker); break;
            case 'q':
                stopLiveMonitoring(&tracker);
                saveSession(&tracker, "incidents_session.txt");
                running = 0;
                break;
            default:
                printf("Unknown command.\n");
        }
    }

    printf("Goodbye.\n");
    trackerDestroy(&tracker);
    return 0;
}
