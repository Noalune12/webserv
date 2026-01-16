# Facade / Singleton

## Facade

Structural design pattern that provides an interface offering simplified access to a complet set of classes.

A facade is a class that provides a simple interface to a complex subsystem of **moving parts**. The goal is to limit possible interactions with the facade, since it it the subsystem that possess these functionalities.

In our Webserv, we could only expose a `parseConfig(path)` method instead of interacting directly with the lexer, parser and multiple configuration objects. This facade reads the file, validate directives and return a ready-to-use struct for socket creation and routes building, without revealing the internal complexity.

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


- A facade provides a convenient access to the various parts of the subsystem's functionalities.
- The classes of the subsystem are not aware of the existence of the facade. **They operate and interacts directly within their own system**
- The goal is to encapsulte their external functionnalities and hide them from the rest of the code

#### Applications examples

- **Use a facade is the needs are to architecture a subsystem into multiple layers (exactly our case)**

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
    // The facade orchestrate the initialiaztion of every subsystems
    Config config = _parser.parse(configPath);
    ...
}
```

## Singleton

A singleton **guarentees the uniqueness of an instance for a class**. It provides a global access to point to our instance.

Its implementation is simple, our facade class, to become a singleton needs a **default private constructor** with a static creating method. The static method will act has a constructor.
```cpp
class Server {
    private:
        Server(const std::string& configPath);  // private constructor
        Server(const Server&);                  // Forbbids copy
        Server& operator=(const Server&);       // Forbbids copy assignment

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

⚠️ **Both of these design patterns goes together sometimes. We did not implement the singleton as we architectured a bit differently the data structures inside of our subsystem.**

[Singleton documentation](https://refactoring.guru/design-patterns/singleton)
