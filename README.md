# Unix Systems Programming in C

Three programs written in C against the POSIX API: a multithreaded web server, an inode-based file system simulator, and a parallel downloader built on process management.

| Program | What it shows |
|---------|---------------|
| [`http-server/`](http-server/) | TCP sockets, POSIX threads, HTTP/1.1 request parsing and status codes |
| [`fs-simulator/`](fs-simulator/) | Inodes, directory entries, and binary file I/O |
| [`parallel-downloader/`](parallel-downloader/) | `fork`, `execvp`, `waitpid`, exit status handling, and concurrency limits |

Each program has its own `Makefile`. Run `make` in its folder to build it. Requires a C compiler (gcc or clang) on Linux or macOS.

---

## HTTP server

A multithreaded HTTP/1.1 server that serves files from the directory it is started in.

- Listens on a TCP socket and hands each accepted connection to its own detached POSIX thread, so slow clients do not block other requests
- Parses the request line and supports `GET` (headers and body) and `HEAD` (headers only)
- Returns `400 Bad Request` for anything other than HTTP/1.1, `404 Not Found` for missing files, `501 Not Implemented` for other methods, and `500 Internal Error` if a thread cannot be created
- Includes a `/delay/N` endpoint that waits N seconds before responding, which makes it easy to check that concurrent requests are handled in parallel

```bash
cd http-server
make
./httpd 8080
```

In another terminal:

```bash
curl -i http://localhost:8080/<file-in-that-directory>
curl -I http://localhost:8080/<file-in-that-directory>          # HEAD
curl http://localhost:8080/delay/2 & curl http://localhost:8080/delay/2 & wait
# both finish after about 2 seconds, not 4, because each runs on its own thread
```

## File system simulator

An interactive shell over a simulated file system stored as inode files on disk.

- `inodes_list` holds one record per inode: a 4-byte inode number and a 1-byte type (`d` for directory, `f` for file)
- Each directory is a file named after its inode number, containing 36-byte entries: a 4-byte inode number and a 32-byte name, starting with `.` and `..`
- Supports `ls`, `cd <dir>`, `mkdir <dir>`, `touch <file>`, and `exit`, with duplicate-name checks and a limit of 1,024 inodes
- Changes are written straight to the inode files, so the file system persists between runs

```bash
cd fs-simulator
make
./fs_simulator fs        # sample file system (use "empty" for a blank one)
> ls
> mkdir projects
> cd projects
> touch notes
> ls
> exit
```

Running it modifies the files in the directory you pass in, so work on a copy (`cp -R fs fs-copy`) to keep the sample intact.

## Parallel downloader

Downloads a list of URLs in parallel by running `curl` in child processes, with a cap on how many run at once.

- Reads a file where each line is `<output-name> <url> [timeout-seconds]`
- Forks one child per line, and each child replaces itself with `curl` through `execvp`
- When the concurrency limit is reached, the parent blocks in `waitpid` until a child finishes before starting the next download
- Reports which line each process handled and whether it exited normally or with an error code, such as curl's exit code 28 when a download hits its timeout

```bash
cd parallel-downloader
make
./a4download test_urls.txt 3     # at most 3 downloads at a time
```

---

## Known limitations

These are class projects, and they have limits worth knowing:

- **HTTP server:** request paths are read into a small fixed-size buffer, paths are not checked for `..`, and every response is labeled `text/html`. Only run it locally, in a directory you are comfortable serving.
- **Parallel downloader:** input files are limited to 20 lines.
- **File system simulator:** each command, including the name, is limited to 31 characters.

## Author

Shrey Shah, California Polytechnic State University, San Luis Obispo. Built as coursework for CSC 357 (Systems Programming).
