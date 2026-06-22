@echo off
cd /d "c:\Users\HP\Downloads\DSA_SUMMATIVE"
git checkout --orphan clean-main
git add .
git reset HEAD fix_commit.bat 2>nul
for /f %%i in ('git write-tree') do set TREE=%%i
for /f %%j in ('git commit-tree %%i -m "DSA summative project: five data structure implementations" -m "Emergency dispatch tracker (doubly linked list), procedure validator (BST), airline route analyzer (directed graph), campus robot navigation (Dijkstra), and telemetry compression (Huffman)."') do set NEW=%%j
git reset --hard %%NEW%%
git branch -D main 2>nul
git branch -m main
git cat-file -p HEAD
