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

### Server loop
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