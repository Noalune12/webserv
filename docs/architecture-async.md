# Webserver, an asynchronous and non-blocking server.

A HTTP server has to be working asynchronously and be non-blocking for **one** fundamental reason: **a single slow client cannot block other clients**

### C10K Problem:

The C10k problem is a mathematical problem.
- Serving clients with threads:
  - The problem in this approch is the use of the whole `stack` for each client, which costs a lot of memory. 10000 threads would consume 80Go of RAM just for their [stacks](https://fr.wikipedia.org/wiki/C10k_problem#:~:text=Le%20probl%C3%A8me%20dans%20cette%20approche,m%C3%A9moire%20limit%C3%A9%20%C3%A0%204%20Gigaoctet.)

### Solution

An evenemential architecture around `poll()`, `epoll()`, etc would solve this by allowing a single thread to handle several connections fastly. Nginx uses this solution (Apache doesn't).

## Asynchronous fundamentals

In an asynchronous program based on a mechanism of oversight like `poll()/epoll()`, the program uses an event loop waiting on all the connections, transformed in file descriptors (fd) to be ready, then treats every connection only when they have data to read or write. Which avoid having to block uselessly on an individual connection.

## The poll, select and epoll mechanism: I/O multiplexing

Multiplexing *(dividing the capacity of the communication channel into several logical channels, one for each message signal or data stream to be transferred)* allows to monitor several fds (sockets) simultaneously while keeping a working non-blocking principle.

- `poll()`
  - Classic POSIX approch: API relatively simple, creates a table of structures `pollfd`, each structs has: a fd and the monitored events (`events`: `POLLIN` to read, `POLLOUT` to write). `poll()` is blocking until one or several fds are ready, then returns by filling a `revents` field of each structs.
  - Complexity: **O(n)** as the kernel scans every sockets at each calls, even if only fd have events to handle.

- `epoll()`
  - Linux specific, has a major evolution from `poll()`: The kernel memorises which fds are monitored (interest list) and keeps another list of those who are really ready (ready list).
  - Usage: have to be instanciated with `epoll_create()`, adding/modifying/deleting fds and events via `epoll_ctl()` then returns the ready list with `epoll_wait()`, which is the main function of the event loop of our webserver
  - Complexity: **O(1)** since it only returns the sockets with events ready to be handled.

- `select()`
  - Older, universal but limited. This function uses bitmaps (fd_set) instead of tables. With a limit fixed of `FD_SETSIZE` (1024 fds). Those bitmaps needs to be rebuild after each calls because the kernel modifies them. Only advantage is its portability but extremely limited by the number of clients it can handle.


> We choose `epoll()` over those 3 possibilities because, first we are working on Linux computers, which allowed us to learn it. Secondly its fast and elegant.


## Sockets handling

Sockets are the endpoints for network communication. In a non-blocking server, proper socket configuration is essential.

### Socket creation flow

1. **`socket()`** - Creates the socket fd with the appropriate domain (`AF_INET` for IPv4) and type (`SOCK_STREAM` for TCP).

2. **`setsockopt()`** - Configures socket options:
   - `SO_REUSEADDR`: Allows immediate rebinding to a port after server restart (avoids "Address already in use" errors).
   - `SO_REUSEPORT`: Allows multiple sockets to bind to the same port (useful for load balancing).

3. **`bind()`** - Associates the socket with a specific IP address and port.

4. **`listen()`** - Marks the socket as passive, ready to accept incoming connections. The backlog parameter defines the queue size for pending connections.

5. **`accept()`** - Extracts a connection from the pending queue, returning a new fd for that specific client.

### Non-blocking mode

Every socket (both the listening socket and client sockets) must be set to non-blocking mode using `fcntl()` with `O_NONBLOCK`. This ensures that:
- `accept()` returns immediately if no connection is pending (`EAGAIN`/`EWOULDBLOCK`).
- `read()`/`recv()` returns immediately if no data is available.
- `write()`/`send()` returns immediately if the buffer is full, indicating partial writes.

> Without non-blocking sockets, a slow `read()` or `write()` on one client would freeze the entire event loop, defeating the purpose of using `epoll()`.

⚠️ Nginx have the `client_header_timeout` and `client_body_timeout` directives enabling an automatic close for slow connections. But even withtout those timeouts, the server keep running on every other clients (thanks to the event loop, while sockets are waiiting for network data, the loop is free to manage the sockets that have data available). The main proccess continuously work on productives tasks and never wastes time waiting.


#### POST of file throught CGI: a form to upload images onto a php script

With the non-blocking architecture, the server receives data by chunks each time `poll()` as a `POLLIN` event on the client socket, stocks data in a buffer or a temporary file, and when the upload is complete, `fork()` to execute the CGI, monitor the CGI status as well via `poll()`, which means the pipe fds have been added to `poll()` (because it cannot `wait()` the child the same way we did for minishell). Otherwise the server would be blocked if a script has a infinite loop.

