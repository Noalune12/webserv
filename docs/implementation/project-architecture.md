# Facade / Singleton

## Facade

<img src="assets/facade-pattern.png" alt="facade" width="800" />

Structural design pattern that provides an interface offering simplified access to a complete set of classes.

A facade is a class that provides a simple interface to a complex subsystem of **moving parts**. The goal is to limit possible interactions with the facade, since it is the subsystem that possesses these functionalities.

In our Webserv, we could only expose a `parseConfig(path)` method instead of interacting directly with the lexer, parser and multiple configuration objects. This facade reads the file, validates directives and returns a ready-to-use struct for socket creation and routes building, without revealing the internal complexity.

```cpp
// Instead of:
Lexer lexer("/path/to/webserv.conf");
Parser parser(lexer.tokenize());
Config config = parser.parse();
SocketManager sm(config);

// We have:
Server server("/path/to/webserv.conf");
server.start();
```


- A facade provides convenient access to the various parts of the subsystem's functionalities.
- The classes of the subsystem are not aware of the existence of the facade. **They operate and interact directly within their own system**
- The goal is to encapsulate their external functionalities and hide them from the rest of the code

#### Application examples

- **Use a facade when you need to architect a subsystem into multiple layers (exactly our case)**

```cpp
// The Facade server hides the whole internal complexity
class Server {
    private:
        ConfigParser    _parser;    // parsing subsystem
        SocketManager   _sockets;   // network subsystem
        RequestHandler  _rhandler;  // HTTP subsystem
        CGIExecutor     _cgi;       // CGI subsystem
    public:
        void    start();            // Easy to use interface
};
```

#### Implementation

- Declare and implement an interface as a facade that regulates code calls to the appropriate sub-objects. It is also responsible for initializing subsystems and managing their lifecycles
- Force communication to subsystems via the front panel.
```cpp
Server::Server(const std::string& configPath) {
    // The facade orchestrates the initialization of every subsystem
    Config config = _parser.parse(configPath);
    ...
}
```

## Singleton

<img src="assets/singleton.png" alt="singleton" width="800" />


A singleton **guarantees the uniqueness of an instance for a class**. It provides a global access point to our instance.

Its implementation is simple: our facade class, to become a singleton, needs a **default private constructor** with a static creation method. The static method will act as a constructor.
```cpp
class Server {
    private:
        Server(const std::string& configPath);  // private constructor
        Server(const Server&);                  // Forbids copy
        Server& operator=(const Server&);       // Forbids copy assignment

    public:
        static Server& getInstance(const std::string& configPath);
};
```
If our code has access to the singleton class, then it will be able to call its static method which will always return the same object on each call.
```cpp
// in main.cpp
Server& srv = Server::getInstance("/path/to/webserv.conf");
srv.start();

// elsewhere, we get the same instance
Server& srv2 = Server::getInstance(); // same object as srv
```

⚠️ **Both of these design patterns go together sometimes. We did not implement the singleton as we architected the data structures inside of our subsystem a bit differently.**

[Singleton documentation](https://refactoring.guru/design-patterns/singleton)

# Event-Driven I/O Loop

<img src="assets/epoll-user-space.png" alt="epoll-user-space" width="800" />

## Technical Choice: epoll over poll

We chose `epoll()` over `poll()` for the following reasons:

| Aspect | poll() | epoll() |
|--------|--------|---------|
| Complexity | O(n) - kernel scans all fds | O(1) - kernel maintains ready list |
| State | Stateless, rebuild pollfd array each call | Stateful, kernel remembers interest list |
| Scalability | Degrades with connection count | Constant performance regardless of scale |
| Portability | POSIX standard | Linux-specific |

Since the project targets Linux environments and scalability matters for an HTTP server, `epoll()` was the natural choice.

## Core Components

### EventLoop Class

The `EventLoop` is the heart of our server, implementing the Reactor pattern. It owns the epoll instance and manages all client connections.
```cpp
class EventLoop {
    private:
        int                         _epollFd;       // epoll instance
        bool                        _running;       // main loop control
        ServerManager&              _serverManager; // listen sockets + vhosts
        std::map<int, Connection>   _connections;   // fd -> Connection mapping

    public:
        void run(void);  // main event loop
        // epoll operations
        bool addToEpoll(int fd, uint32_t events);
        bool modifyEpoll(int fd, uint32_t events);
        bool removeFromEpoll(int fd);
};
```

### Connection State Machine

Each client connection is tracked through a state machine, allowing proper timeout management and request processing:
```cpp
enum ConnectionState {
    IDLE,               // waiting for new request (keep-alive)
    READING_HEADERS,    // receiving HTTP headers
    READING_BODY,       // receiving request body
    CGI_RUNNING,        // waiting for CGI process
    SENDING_RESPONSE,   // writing response to client
    CLOSED
};
```

Each state has its own timer index, enabling fine-grained timeout control (different timeouts for idle connections vs. active transfers).

## Implementation Flow

### Initialization

The epoll instance is created once at startup via `epoll_create()`. All listen sockets from the `ServerManager` are then registered to the interest list with `EPOLLIN` events, making them ready to accept incoming connections.

### Event Dispatching

The main loop distinguishes between two fd types: listen sockets trigger `acceptConnection()` to handle new clients, while client sockets are routed to `handleClientEvent()` which uses the connection's current state to determine the appropriate action (read headers, read body, send response, etc.).

## Key Design Decisions

### Non-Blocking Sockets

All client sockets are set to non-blocking mode immediately after `accept()`:
```cpp
fcntl(clientFd, F_SETFL, O_NONBLOCK);
```

This ensures that `recv()` and `send()` never block the event loop, even if a client connection is slow or stalled.

### Dynamic Timeout Calculation

Rather than using a fixed timeout, `epoll_wait()` receives a dynamically computed value based on the nearest connection timeout. This optimizes CPU usage by avoiding unnecessary wake-ups while guaranteeing that timed-out connections are cleaned up promptly.

### epoll_ctl Notes
```cpp
// EPOLL_CTL_DEL: NULL event parameter is safe on kernel > 2.6.9
epoll_ctl(_epollFd, EPOLL_CTL_DEL, fd, NULL);

// EPOLL_CTL_ADD: EEXIST means fd already registered (not necessarily an error)
// EPOLL_CTL_DEL: ENOENT means fd not registered (cleanup scenario)
```

## User Space Perspective

From the application's point of view, the event loop provides:

- **Simple system call interface**: `epoll_create`, `epoll_ctl`, `epoll_wait`
- **O(1) performance**: regardless of connection count
- **Event-driven programming model**: process only ready sockets
- **Clean resource management**: centralized connection lifecycle
