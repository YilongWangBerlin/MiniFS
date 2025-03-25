

# **MiniFS – A Simple Virtual File System in C**


This project implements a simple virtual file system in C, developed as part of the *Systemprogrammierung* course at TU Berlin (SS23). It mimics core file system concepts such as INodes, superblocks, block storage, and a shell-like interface for user interaction.

---

## 📂 Features

- Create, list, write to, read from, and delete files or directories
- Import/export files between the host system and the virtual file system
- Data persistence via `.fs` file
- INodes for metadata and directory structure
- Block-based data storage with a free block manager
- CLI shell for interaction with the file system

---

## 🛠️ Implemented Commands

| Command             | Description                              |
|---------------------|------------------------------------------|
| `mkdir <path>`      | Create a directory                       |
| `mkfile <path>`     | Create a file                            |
| `list <path>`       | List contents of a directory             |
| `writef <path> <text>` | Write text to a file (append mode)  |
| `readf <path>`      | Read and print the contents of a file    |
| `rm <path>`         | Delete a file or directory recursively   |
| `import <src> <dst>`| Import a file from host into MiniFS      |
| `export <src> <dst>`| Export a file from MiniFS to host system |

> Use `exit` or `quit` to leave the shell.

---

## getting Started

### Compile the project

```bash
make
```

### Create a new file system

```bash
./build/ha2 -c myfs.fs 64
```

### Load existing file system

```bash
./build/ha2 -l myfs.fs
```

---

## Testing

- Tests can be executed with:

```bash
make test
```

- Individual command tests via:

```bash
make test mkdir
```

