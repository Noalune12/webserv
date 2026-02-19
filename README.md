# webserv

This project is about writing our own HTTP server. We will be able to test it with an actual browser. HTTP is one of the most widely used protocols on the internet. Understanding its intricacies will be useful, even if we won’t be working on a website. Its goal is to teach how a server works and moreover to teach how the internet in general works: with requests/responses between clients/servers.

## Instructions

```bash
# Clone the repository
git clone git@github.com:AzehLM/webserv.git

# Move to the project repository
cd webserv

# Compile the project
make

# Launch the server (without configuration file, it will be launched with a default config file)
./webserv

# or with a custom configuration file
./webserv <path_to_config_file>
```


## Resources

### Starting Documentation

[Pierre Riraud HTTP/Network/Security (french documentation)](https://www.pierre-giraud.com/http-reseau-securite-cours/)

[Starting point (flow-chart at the end)](https://m4nnb3ll.medium.com/webserv-building-a-non-blocking-web-server-in-c-98-a-42-project-04c7365e4ec7)

[RFC 2616 (Obsoleted)](https://datatracker.ietf.org/doc/html/rfc2616)

[RFC 9112 (Updated version from 2022 of RFC 2616)](https://www.rfc-editor.org/rfc/rfc9112.html)

[Beej's Guide to Network Programming](https://beej.us/guide/bgnet/html/)

[Important Things About HTTP Headers](https://bytebytego.com/guides/important-things-about-http-headers-you-may-not-know/)

[Resources from this webserv's readme](https://github.com/cclaude42/webserv?tab=readme-ov-file)

[HTTP Error descriptions](https://http.cat/)

[CodeQuoi: Sockets and Network Programming in C](https://www.codequoi.com/en/sockets-and-network-programming-in-c/)

[epoll madness](https://copyconstruct.medium.com/the-method-to-epolls-madness-d9d2d6378642)

### Dump

[Linux kernel coding style (formatting code ? maybe not really interesting for this project tho)](https://docs.kernel.org/process/coding-style.html)

[URL vs URI: differences and when to use them (french documentation)](https://www.hostinger.com/fr/tutoriels/uri-vs-url#:~:text=URI%20est%20l'acronyme%20d,un%20emplacement%20ou%20les%20deux.)

### Bonus

[HTTP Cookies Explained With a Simple Diagram](https://bytebytego.com/guides/http-cookies-explained-with-a-simple-diagram/)

[Session Management notion introduction (read before next link)](https://mojoauth.com/blog/session-management-a-beginners-guide-for-web-developers#conclusion)
