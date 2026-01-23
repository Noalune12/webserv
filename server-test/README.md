# Server

## Start Server - Get all info
### addrinfo
- structures contenant une adresse internet
- hints pointe sur une struct addrinfo afin de définir les critères de sélection des structures d'addresses de sockets renvoyée par res
- hints -> définir ai_family, ai_socktype et ai_protocol.
    - ai_family = famille d'adresse désirée -> AF_INET (IPv4), AF_INET6 (IPv6) ou AF_UNSPEC
    - ai_socktype = type de socket -> SOCK_STREAM (TCP) ou SOCK_DGRAM sinon 0
    - ai_protocols = protocole des adresses -> 0 pour n'importe quel type
    - ai_flags (?)
    - ai_addrlen
    - ai_addr: pointeur vers structuct de type sockaddr_in
- res -> envoyé à getaddrinfo pour être rempli selon les conditions dites dans hints
### getaddrinfo
- pour trouver l'adresse ip à laquelle nous souhaitons nous connecter
- return int -> 0 si success
- arguments -> adress ip (char), service = port  ou nom du service (http) dans chacune des struct d'adresses renvoyées, si NULL = port non initialisé, hints, &res
- freeaddrinfo -> libère la mémoire de res
### sockaddr_in
- structure pour adresse ip et port de connexion
    - sin_family -> IPv4 - AF_INET
    - sin_port -> port auquel nous souhaitons nous connecter (indiquer avec octets dans l'ordre du réseau -> utiliser htons)
    - sin_addr -> in_addr -> adresse IPv4

## Socket and Connection and Epoll
### Prepare
- comme une pipe (memory buffer) 
- descripteur de fichier de notre socket grâce auquel on pourra lire et écrire pour recevoir et envoyer des données
- fonction socket 
    - arguments (only int): domain (famille de protocoles de la socket PF_INET), type (SOCK_STREAM), protocol (0 car un seul protocol valide par type)
    - return fd -> -1 si echec (+ errno)
### Link to address
- distant (connect)
- local (bind)
    - arguments: int sockfd (fd récupéré via socket), sockaddr (sockadrr_in défini avant), addrlen (size of sockaddr_in)
    - return 0 if success or -1 if error

### Listen via socket to detect connection demand
- marquer la socket comme passive, elle attend des demandes de connexion
- arguments: sockfd, backlog (nombre de connexions autorisées dans la fil d'attente)
- return 0 if success or -1 if failure
### EPOLL
- faire un serveur non bloquant avec fcntl
    - arguments: fd, cmd (ce qui va s'appliquer pour notre fd, S_ETFL -> permet de changer l'état du fichier et le rendre nin bloquant), O_NONBLOCK
    - return -1 if failure
- (epoll_create, epoll_ctl, epoll_wait)
- ouvrir un descripteur de fichier epoll via epoll_create(int nb) -> -1 if failure
- struct epoll_event -> spécifie les données que le kernel va enregistré et retourné quand le fd est prêt
    - events -> epoll events (EPOLLIN)
    - data -> user data variable (ptr, fd = socket fd)

## Server loop
- struct epoll_event (used with wait) : int events (what to watch for - EPOLLIN -> data available to read or EPOLLOUT -> ready for writing etc) + user data (fd)
- while (1)
- epoll_wait(int max events, struct epoll_event, int max events, int timeout) : waiting for an event -> return -1 if error, 0 if ok, an int wich is the number of fd ready for request
- check return of epoll_wait to know if error, server still waiting or new clients
- go throw all the fd in epoll_events
    - if fd = socket, this means the event happened on the listening socket not on a client socket so we have a new client
        - we have to keep the fd of the new client
        - accept the new client -> create a new socket (fd)
        - make it non blocking with fcntl
        - add client to epoll and vector of client_fd
        - struct epoll_event client_ev; -> fill the struct for the vent we care about and the client fd \
                client_ev.events = EPOLLIN; \
                client_ev.data.fd = _clientFd; \
                _clientsFd.push_back(_clientFd); -> add the client fd to the vector \
                epoll_ctl(_epollFd, EPOLL_CTL_ADD, _clientFd, &client_ev); -> register the client socket in epoll
    - else if events[i].events & EPOLLIN -> we will read the data from the socket of a registered client
        - recv
        - do we need to delete the client from epoll and vector ?
    - else
        - send a message ?

## Request
- include within the first line, the method to be applied to the resource, the identifier of the resource and the protocol used
1. Request-Line; *(( 
2. general-header;
3. request-header;
4. entity-header ) CRLF);
5. CRLF
6. [ message-body ] 
### Request-Line
- METHOD Request-URI HTTP
    - Method: GET, POST, DELETE
    - Request URI: "*" | absoluteURI | abs_path | authority
        - "*" = request does not apply to a parlicular resource but to the server, allowed when method does not apply to a resource -> maybe only with GET
### Resource identified by a request
- Host: ip:port
- if not valud 400 (bad request)
### General header
- provide information about the message itself
- Cache-Control: Dictates caching rules (e.g., no caching, max age).
- Connection: Controls whether the connection stays open or closed.
- Date: Specifies when the message was sent.
- Pragma: Provides backward compatibility with HTTP/1.0 (often for cache control).
- Trailer: Used in chunked transfer encoding to specify header fields sent after the message body.
- Transfer-Encoding: Describes how the message body is encoded for transfer (e.g., chunked, gzip).
- Upgrade: Requests a protocol change (e.g., HTTP/2.0).
- Via: Shows the proxies the request or response passed through.
- Warning: Conveys additional status information or warnings about the message.
### Request header fields
- Accept: specifies the media types the client is willing to receive from the server
- Accept-Charset: Indicates which character encodings the client is willing to accept (like UTF-8, ISO-8859-1). \
Example: Accept-Charset: utf-8, iso-8859-1;q=0.5
- Accept-Encoding: Tells the server which content encoding methods (like gzip, deflate) the client supports for compressed responses. \
Example: Accept-Encoding: gzip, deflate
- Accept-Language: Indicates the preferred languages for the response (like en, fr, de), helping the server serve localized content. \
Example: Accept-Language: en-US, en;q=0.9, fr;q=0.8
- Authorization: Contains credentials for authenticating the client with the server. \
Example: Authorization: Basic dXNlcjpwYXNz
- Expect: Indicates that the client expects a particular behavior from the server, such as expecting a 100 Continue response before sending the request body. \
Example: Expect: 100-continue
- From: Contains the email address of the user making the request. (used for debugging or reporting purposes) \
Example: From: user@example.com
- Host: Specifies the domain name of the server and optionally the TCP port number. This is essential for virtual hosting, where multiple websites are served from a single IP address. \
Example: Host: www.example.com
- If-Match: Make the request conditional based on the entity's current state. It will only proceed if the entity matches one of the provided entity tags (ETags). \
Example: If-Match: "abc123"
- If-Modified-Since: Allows the client to request a resource only if it has been modified since the specified date. \
Example: If-Modified-Since: Sat, 29 Oct 1994 19:43:31 GMT
- If-None-Match: Used to request a resource only if it does not match the given entity tags (ETags). \
Example: If-None-Match: "abc123"
- If-Range: Tells the server to send the entire resource unless it has been modified after the specified date or if it matches a certain entity tag. \
Example: If-Range: "abc123"
- If-Unmodified-Since: used to ensure that the request will only proceed if the resource has not been modified since the given date. \
Example: If-Unmodified-Since: Sat, 29 Oct 1994 19:43:31 GMT
- Max-Forwards: Limits the number of hops a request can make through proxies or gateways. This is often used with the TRACE method to limit how far the request can travel. \
Example: Max-Forwards: 10
- Proxy-Authorization: Similar to Authorization, but used to provide credentials for proxy servers instead of the origin server. \
Example: Proxy-Authorization: Basic dXNlcjpwYXNz
- Range: Specifies a byte range or a portion of the resource to be returned, allowing for partial downloads. \
Example: Range: bytes=200-1000
- Referer: Specifies the address of the previous web page from which a link to the currently requested page was followed. It's used for analytics, security, and tracking purposes. \
Example: Referer: http://www.example.com/previous-page
- TE: Tells the server which transfer encodings the client supports for the response. Often used for chunked transfer encoding. \
Example: TE: trailers, deflate
- User-Agent: Contains information about the client application (like a web browser or mobile app) making the request, which helps the server understand the client’s capabilities and tailor the response. \
Example: User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.36

### Entity header
- describe the control of the message body
- Allow: Lists allowed HTTP methods for a resource.
- Content-Encoding: Describes how the body has been encoded (e.g., compressed).
- Content-Language: Specifies the language of the content.
- Content-Length: Indicates the size of the response body in bytes.
- Content-Location: Provides a URI for the location of the resource.
- Content-MD5: An MD5 hash of the content for integrity checks.
- Content-Range: Specifies the byte range for partial responses.
- Content-Type: Describes the media type of the body (e.g., JSON, HTML).
- Expires: Specifies when the response expires for caching purposes.
- Last-Modified: Indicates when the resource was last modified.
- Extension-header: Custom headers added for specific purposes.

### Message body
- used to carry entity body associated with the request
- The message body of a request is the actual data being sent (like form data or JSON).
- A request includes a body if it has a Content-Length or Transfer-Encoding header, which tells the server that data is being sent.
- Not all HTTP methods allow a message body. For your server:
    - GET requests should not have a body. They are just for retrieving data.
    - PUT requests must have a body because they are used to update data.
    - DELETE requests may or may not have a body, but typically they are used to delete a resource, not to send data.

## Response
### Status Line