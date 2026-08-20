# CARROT 🥕
 
A lightweight, Git-inspired version control system built from scratch in C++17.
 
CARROT implements the core ideas behind Git — content-addressable storage, staging, commits, branches, checkout, merging, conflict resolution, and diffing.
 
## Features
 
- SHA-256 content-addressable object storage
- Blob, tree, and commit objects
- Staging area / index
- Commit history and `HEAD`
- Branch creation and switching
- Commit checkout
- Fast-forward and three-way merges
- Merge conflict detection and resolution
- Two-parent merge commits
- Working-tree and staged diffs
## Architecture
 
```
Working Tree
     ↓
   Index
     ↓
    Tree
     ↓
   Commit
     ↓
   Branch
     ↓
    HEAD
```
 
## Usage
 
```bash
carrot init
carrot add <file>
carrot commit "message"
 
carrot branch <name>
carrot switch <branch>
 
carrot log
carrot status
 
carrot diff
carrot diff --cached
 
carrot merge <branch>
carrot merge --continue
```
 
## Build
 
**Requirements:**
- C++17
- CMake
- Ninja
```bash
cmake -S . -B build
cmake --build build
 
./build/carrot.exe
```
 
## Project Structure
 
```
CARROT/
├── include/
├── src/
├── CMakeLists.txt
├── README.md
└── CARROT_DOCUMENTATION.md
```
 
## Why I Built It
 
CARROT was built as a systems project to understand how version control works internally — particularly object storage, commit graphs, branching, and three-way merging — rather than simply using Git as a black box.
 
## Status
 
**CARROT v1** — Core implementation complete.
 
CARROT is an educational Git-inspired implementation and is not intended to be a replacement for Git.