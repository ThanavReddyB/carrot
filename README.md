# 🥕 Carrot

> *"The best things grow underground."*

Carrot is a Git-inspired distributed version control system built entirely from scratch in modern C++.

The goal of this project is to understand how version control systems work internally by implementing core Git concepts such as object storage, commits, trees, hashing, branches, and repository management without relying on Git's source code.

---

## Motivation

Git is one of the most widely used developer tools, yet most developers interact with it only through commands like:

```bash
git add
git commit
git push
```

Carrot explores what happens beneath those commands by building a simplified implementation from first principles.

This project focuses on software engineering, modern C++, filesystem design, hashing, data structures, and system architecture.

---

## Features

### Current

- Repository initialization (`carrot init`)

### Planned

- File staging (`carrot add`)
- Commit creation
- Blob objects
- Tree objects
- SHA-256 object hashing
- Commit history (`carrot log`)
- Repository status
- Checkout
- Branches
- Merge (optional)

---

## Technologies

- C++20
- CMake
- GCC
- STL
- std::filesystem

---

## Build

```bash
mkdir build
cd build

cmake ..
cmake --build .
```

---

## Usage

Initialize a repository

```bash
carrot init
```

---

## Project Structure

```
Carrot/
│
├── include/
├── src/
├── build/
├── README.md
└── CMakeLists.txt
```

---

## Learning Goals

- Modern C++
- Object-oriented design
- Filesystem programming
- SHA hashing
- Serialization
- Version control internals
- Software architecture

---

## License

MIT