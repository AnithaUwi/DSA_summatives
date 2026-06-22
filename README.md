# DSA Summative Project — Data Structures & Algorithms

**Author:** Anitha Uwimpuhwe 
**Course:** Data Structures and Algorithms (DSA)  
**Repository:** [https://github.com/AnithaUwi/DSA_summatives](https://github.com/AnithaUwi/DSA_summatives)

This repository contains the complete implementation for the DSA summative assessment. Each question applies a core data structure or algorithm to a real-world scenario.

---

## Project Overview

| Question | Topic | Data Structure / Algorithm |
|----------|-------|---------------------------|
| Q1 | Emergency Dispatch Incident Tracker | Doubly Linked List |
| Q2 | Secure Maintenance Procedure Validator | Binary Search Tree (BST) |
| Q3 | Airline Route Relationship Analyzer | Directed Graph + Adjacency Matrix |
| Q4 | Campus Delivery Robot Navigation | Weighted Graph + Dijkstra's Algorithm |
| Q5 | Telemetry Data Compression Utility | Huffman Coding (Lossless Compression) |

---

## Prerequisites

- **C compiler** (GCC recommended — e.g. [MinGW gcc](https://www.mingw-w64.org/) on Windows)
- **Windows / Linux / macOS** terminal
- For Q1 only: **pthread** support (`-pthread` flag)

Verify your compiler:

```bash
gcc --version
```

---

## Repository Structure

```
DSA_SUMMATIVE/
├── README.md
├── q1_incident_tracker/
│   ├── incident_tracker.c
│   └── incidents_seed.txt          # Optional startup data
├── q2_procedure_validator/
│   ├── procedure_validator.c
│   └── procedures.txt              # Approved procedures list
├── q3_route_analyzer/
│   └── route_analyzer.c
├── q4_robot_navigation/
│   └── robot_navigation.c
└── q5_huffman_compression/
    ├── huffman_compression.c
    └── telemetry.txt               # Sample telemetry log
```

---

## Build Instructions

Run these commands from the project root (`DSA_SUMMATIVE/`).

### Question 1 — Incident Tracker

```bash
gcc -std=c99 -pthread -o q1_incident_tracker/incident_tracker q1_incident_tracker/incident_tracker.c
```

### Question 2 — Procedure Validator

```bash
gcc -std=c99 -o q2_procedure_validator/procedure_validator q2_procedure_validator/procedure_validator.c
```

### Question 3 — Route Analyzer

```bash
gcc -std=c99 -o q3_route_analyzer/route_analyzer q3_route_analyzer/route_analyzer.c
```

### Question 4 — Robot Navigation

```bash
gcc -std=c99 -o q4_robot_navigation/robot_navigation q4_robot_navigation/robot_navigation.c
```

### Question 5 — Huffman Compression

```bash
gcc -std=c99 -o q5_huffman_compression/huffman_compression q5_huffman_compression/huffman_compression.c
```

**Windows (PowerShell):** Use the same commands; executables are created without an extension or with `.exe` depending on your toolchain.

---

## How to Run

> Run each program from its own folder so relative file paths resolve correctly.

### Q1 — Emergency Dispatch Incident Tracker

```bash
cd q1_incident_tracker
./incident_tracker        # Linux/macOS
incident_tracker.exe      # Windows
```

| Command | Action |
|---------|--------|
| `f` | View newer incident |
| `b` | View older incident |
| `l` | Enable live incident monitoring |
| `s` | Stop live monitoring |
| `d` | Delete all incidents |
| `q` | Save session and quit |

- Max **25 incidents** stored; oldest evicted when full.
- Session saved to `incidents_session.txt` on quit.

---

### Q2 — Secure Maintenance Procedure Validator

```bash
cd q2_procedure_validator
./procedure_validator
```

- Loads approved procedures from `procedures.txt` into a **BST**.
- **Exact match** → approved.
- **Similar entry** → suggests closest procedure (Levenshtein distance).
- **Unknown entry** → rejected and logged to `audit.log`.
- Type `quit` to exit.

**Example inputs:** `LOCK_PANEL`, `LOCK_PANE`, `UNKNOWN_CMD`

---

### Q3 — Airline Route Relationship Analyzer

```bash
cd q3_route_analyzer
./route_analyzer
```

Pre-loaded sample routes: KGL→NBO, KGL→EBB, NBO→JNB, EBB→ADD, ADD→CAI, JNB→CPT.

| Menu | Action |
|------|--------|
| 1 | Query airport (incoming / outgoing flights) |
| 2 | Add airport |
| 3 | Remove airport |
| 4 | Add route |
| 5 | Remove route |
| 6 | Display adjacency matrix |
| 7 | Display all routes |
| 0 | Quit |

---

### Q4 — Campus Delivery Robot Navigation

```bash
cd q4_robot_navigation
./robot_navigation
```

Enter a starting building when prompted. The program uses **Dijkstra's algorithm** to find the shortest path to **Dormitory**.

**Campus network:**

```
Library --6-- Cafeteria --4-- Science Block --8-- Dormitory
Library --15-- Engineering --5-- Administration --3-- Dormitory
Cafeteria --2-- Charging Station --4-- Administration
```

**Example:** From `Library` → `Library → Cafeteria → Charging Station → Administration → Dormitory` (15 units)

---

### Q5 — Telemetry Data Compression Utility

```bash
cd q5_huffman_compression
./huffman_compression
```

Optional custom input file:

```bash
./huffman_compression my_telemetry.txt
```

**Pipeline:**
1. Load `telemetry.txt`
2. Display character frequencies
3. Build Huffman tree and compress → `telemetry.huff`
4. Decompress → `telemetry_restored.txt`
5. Verify restored file is **byte-identical** to the original

---

## Design Highlights

### Q1 — Doubly Linked List
- `head`, `tail`, and `current` pointers for O(1) navigation.
- Thread-safe live monitoring with mutex.
- FIFO eviction when capacity (25) is reached.

### Q2 — Binary Search Tree
- O(log n) exact lookup.
- In-order traversal for sorted procedure listing.
- Post-order deletion for complete memory cleanup.

### Q3 — Directed Graph
- Adjacency list for efficient edge storage.
- Adjacency matrix regenerated after every structural change.
- Dynamic add/remove of airports and routes.

### Q4 — Dijkstra's Algorithm
- Undirected weighted graph (campus paths are bidirectional).
- Parent-array path reconstruction.
- Case-insensitive building name validation.

### Q5 — Huffman Compression
- Priority-queue tree construction.
- Serialized tree stored in `.huff` for decompression.
- Lossless verification via byte comparison.

---

## Testing Summary

| Question | Key test | Expected result |
|----------|----------|-----------------|
| Q1 | Navigate with `f`/`b`, enable live mode | Chronological order maintained |
| Q2 | `LOCK_PANEL` / `LOCK_PANE` / unknown | Approve / suggest / reject+log |
| Q3 | Query `KGL`, display matrix | OUT: NBO,EBB; matrix matches routes |
| Q4 | Start from `Library` | Shortest path to Dormitory = 15 units |
| Q5 | Run on `telemetry.txt` | Verification: SUCCESS |

---


