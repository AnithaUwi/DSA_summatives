#include <iostream>
#include <fstream>
#include <string>
#include <ctime>
#include <thread>
#include <atomic>
#include <chrono>
#include <mutex>
#include <limits>

using namespace std;

const int MAX_INCIDENTS = 25;
const int LIVE_INTERVAL_SECONDS = 8;

struct Incident {
    int id;
    string source;
    string description;
    string timestamp;
};

struct Node {
    Incident data;
    Node* prev;
    Node* next;
};

class IncidentTracker {
private:
    Node* head;
    Node* tail;
    Node* current;
    int count;
    int nextId;
    atomic<bool> liveMonitoring;
    thread monitorThread;
    mutex listMutex;

    void freeList() {
        Node* node = head;
        while (node) {
            Node* next = node->next;
            delete node;
            node = next;
        }
        head = tail = current = nullptr;
        count = 0;
    }

    int appendIncident(Incident incident) {
        bool evicted = false;
        int evictedId = 0;

        {
            lock_guard<mutex> lock(listMutex);
            incident.id = nextId++;

            Node* node = new Node{incident, nullptr, nullptr};

            if (!head) {
                head = tail = current = node;
            } else {
                tail->next = node;
                node->prev = tail;
                tail = node;
                if (!current) current = head;
            }
            count++;

            if (count > MAX_INCIDENTS) {
                Node* oldest = head;
                evictedId = oldest->data.id;
                evicted = true;

                head = head->next;
                if (head) {
                    head->prev = nullptr;
                } else {
                    tail = nullptr;
                }
                if (current == oldest) {
                    current = head;
                }
                delete oldest;
                count--;
            }
        }

        if (evicted) {
            cout << "\n[CAPACITY] Oldest incident #" << evictedId
                 << " evicted (limit: " << MAX_INCIDENTS << " incidents).\n> " << flush;
        }

        return incident.id;
    }

    string nowTimestamp() {
        time_t t = time(nullptr);
        char buf[32];
        strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", localtime(&t));
        return string(buf);
    }

    void monitorLoop() {
        string sources[] = {"Ambulance", "Police", "Fire Station"};
        string descriptions[] = {
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
        while (liveMonitoring) {
            Incident incident;
            incident.source = sources[idx % 3];
            incident.description = descriptions[idx % 8];
            incident.timestamp = nowTimestamp();
            idx++;

            int newId = appendIncident(incident);
            cout << "\n[LIVE] New incident #" << newId << " from "
                 << incident.source << ": " << incident.description
                 << " (" << incident.timestamp << ")\n> " << flush;

            for (int i = 0; i < LIVE_INTERVAL_SECONDS && liveMonitoring; ++i) {
                this_thread::sleep_for(chrono::seconds(1));
            }
        }
    }

public:
    IncidentTracker() : head(nullptr), tail(nullptr), current(nullptr),
                        count(0), nextId(1), liveMonitoring(false) {}

    ~IncidentTracker() {
        stopLiveMonitoring();
        freeList();
    }

    void initialize() {
        freeList();
        nextId = 1;
        current = nullptr;

        ifstream in("incidents_seed.txt");
        if (!in) return;

        string line;
        while (getline(in, line)) {
            if (line.empty()) continue;
            size_t p1 = line.find('|');
            size_t p2 = line.find('|', p1 + 1);
            if (p1 == string::npos || p2 == string::npos) continue;

            Incident incident;
            incident.source = line.substr(0, p1);
            incident.description = line.substr(p1 + 1, p2 - p1 - 1);
            incident.timestamp = line.substr(p2 + 1);
            appendIncident(incident);
        }
        current = head;
    }

    void displayCurrent() {
        lock_guard<mutex> lock(listMutex);
        if (!current) {
            cout << "No incidents recorded.\n";
            return;
        }
        cout << "Incident #" << current->data.id << "\n"
             << "Source: " << current->data.source << "\n"
             << "Description: " << current->data.description << "\n"
             << "Time: " << current->data.timestamp << "\n"
             << "Position: " << (count == 0 ? 0 : distanceFromOldest()) << " of " << count << "\n";
    }

    int distanceFromOldest() {
        int pos = 1;
        Node* node = head;
        while (node && node != current) {
            node = node->next;
            pos++;
        }
        return pos;
    }

    void viewNewer() {
        lock_guard<mutex> lock(listMutex);
        if (!current || !current->next) {
            cout << "Already at the newest incident.\n";
            return;
        }
        current = current->next;
    }

    void viewOlder() {
        lock_guard<mutex> lock(listMutex);
        if (!current || !current->prev) {
            cout << "Already at the oldest incident.\n";
            return;
        }
        current = current->prev;
    }

    void startLiveMonitoring() {
        if (liveMonitoring) {
            cout << "Live monitoring is already active.\n";
            return;
        }
        liveMonitoring = true;
        monitorThread = thread(&IncidentTracker::monitorLoop, this);
        cout << "Live monitoring enabled. New incidents will arrive automatically.\n";
    }

    void stopLiveMonitoring() {
        if (!liveMonitoring) return;
        liveMonitoring = false;
        if (monitorThread.joinable()) {
            monitorThread.join();
        }
        cout << "Live monitoring stopped.\n";
    }

    void deleteAll() {
        if (liveMonitoring) {
            cout << "Stopping live monitoring before clearing incidents...\n";
            stopLiveMonitoring();
        }
        lock_guard<mutex> lock(listMutex);
        freeList();
        cout << "All incidents deleted.\n";
    }

    void saveSession(const string& filename) {
        lock_guard<mutex> lock(listMutex);
        ofstream out(filename);
        if (!out) {
            cout << "Failed to save session.\n";
            return;
        }
        Node* node = head;
        while (node) {
            out << node->data.id << "|"
                << node->data.source << "|"
                << node->data.description << "|"
                << node->data.timestamp << "\n";
            node = node->next;
        }
        cout << "Session saved to " << filename << " (" << count << " incidents).\n";
    }
};

void printMenu() {
    cout << "\n=== Emergency Dispatch Incident Tracker ===\n"
         << "f - View newer incident\n"
         << "b - View older incident\n"
         << "l - Enable live incident monitoring\n"
         << "s - Stop live monitoring\n"
         << "d - Delete all incidents\n"
         << "q - Save session and quit\n"
         << "Command: ";
}

int main() {
    IncidentTracker tracker;
    tracker.initialize();

    cout << "Incident tracker initialized.\n";
    tracker.displayCurrent();

    char cmd;
    bool running = true;
    while (running) {
        printMenu();
        if (!(cin >> cmd)) break;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        switch (cmd) {
            case 'f': tracker.viewNewer(); tracker.displayCurrent(); break;
            case 'b': tracker.viewOlder(); tracker.displayCurrent(); break;
            case 'l': tracker.startLiveMonitoring(); break;
            case 's': tracker.stopLiveMonitoring(); break;
            case 'd': tracker.deleteAll(); break;
            case 'q':
                tracker.stopLiveMonitoring();
                tracker.saveSession("incidents_session.txt");
                running = false;
                break;
            default:
                cout << "Unknown command.\n";
        }
    }

    cout << "Goodbye.\n";
    return 0;
}
