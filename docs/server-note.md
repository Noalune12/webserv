# TASKS-LIST (par ordre de priorité)

- [x] todo-list (obviouly)
- [ ] Read and try to do a server with the codequoi article
- [ ] Read and understand allowed functions, expecially thoe related to the server (from `accept`, `bind` to `poll` `epoll` etc)
- [ ] Check list of notion on which I have lack of understanding (need to list them as well)
- [ ] Make notes about those notions
- [ ] Résumé discussion slack (probablement avant l'architecture du projet mais pas sur)



- [ ] Lien programme asynchrone, multiplexing etc (gros du travail ?)
- [ ] Requests.md

https://beej.us/guide/bgnet/html/#what-is-a-socket

### Notions

- Network programming fundamentals
  - Socket
    - Socket basics:
      - TCP/IP stacks
      - Socket API (Berkeley ?)
      - Socket Lifestyle
      - Client vs server Sockets
      - Fd (should be fine but I want to go deeper on what is considered a fd and whats not, expecially for the I/O multiplexing functions)
  - Socket options
    - anything related to behavior configuration, port reuse, multiple socket on same port etc
  - Address Handling
    - `sockaddr_in` structure
    - Byte ordering
    - `htons`, `htonl` etc -> conversion functions
    - `getaddrinfo()` -> DNS resolution (what is a DNS ?)
    - IPv4 addressing
- I/O Models and Multiplexing
  - Blocking, non-blocking I/O
    - I have documentation on that already, maybe I could find a good image to link it with my understanding
    - `fcntl()` parameters matters a lot here
  - I/O Multiplexing mechanisms
    - `select()`, `poll` etc -> I have some notes about it already
  - Event notification modes
    - Level-Triggered (LT)
    - Edge-Triggered (ET)
    - When to use each ?
  - Event types
    - POLLIN / EPOLLIN
    - POLLOUT / EPOLLOUT
    - POLLHUP / EPOLLHUP
    - POLLER / EPOLLER
- Event-Driven architecture
  - Concepts:
    - Event loot
    - Reactor pattern
    - Non-blocking everything
    - State machine per connection
  - Buffer management
    - Partial read -> `recv()`
    - Partial write -> `send()`
    - Ring buffers / Dynamic buffers
    - Write queues
- HTTP Protocol Deep Dive
  - HTTP message format
  - Request parsing
  - HTTP methods (Requests.md maybe)
  - Essential Headers
    - Host
    - Content-Lenght
    - Content-Type (MIME type of body)
    - Transfer-Encoding: chunked
    - Contecction: keep-alive / close
    - Location
  - Body Handling
    - Linked with the Essential Headers
  - MIME Types
    - Same, how to handle that ?
- File System Operations (fd management)
  - `stat()`, `access()`, `open()`, `read()`, `close()`
  - Autoindex generation
  - Index file resolution (`.html`, `.htm`, etc)
  - Path security
    - Traversal attacks
    - Path canonicalization
    - Chroot-like behavior (never serve outsied document root)
- CGI (Specific readme as well)
  - Fundamentals:
    - What is it
    - RFC 3875
    - Process per request (fork + exec model)
  - CGI Env variables
  - Execution flow
    - fork -> pipes setup -> env setup -> dup2 -> chdir -> execve
  - CGI I/O
    - Request body
    - Response
    - Chunket requests
    - Response parsing
    - EOF Handling
  - CGI Process Management
    - Non-blocking CGI I/O
    - Timeout Handling
    - waitpid ? `WNOHANG` reaper
    - signal handling `SIGCHLD`
- Virtual Hosting
  - Name-based virtual hosting -> `Host` header specifies the desired server


Thats should be enough for now...

Priority order based on current needs:
- Sockets Basics
- Non-blocking I/O -> `EAGAIN`
- poll//epoll
- http parsing
- static file serving
- CGI (might be on top of the list after discussing with people)
