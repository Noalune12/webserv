# Pourquoi webserv doit etre asynchrone et non-bloquant

Un server HTTP ce doit de fonctionner de manière asynchrone et non-bloquante pour **une** raison fondamentale: **un seul client lent ne doit pas bloquer les autres clients.**

### Problème C10K:

Problème mathématique:
- Servir un client par thread:
  - Le problème dans cette approche est l'utilisation de la `stack` entière par client, ce qui coute beaucoup de mémoire. 10000 threads consommeraient 80Go de RAM rien que pour leurs [`stacks`](https://fr.wikipedia.org/wiki/C10k_problem#:~:text=Le%20probl%C3%A8me%20dans%20cette%20approche,m%C3%A9moire%20limit%C3%A9%20%C3%A0%204%20Gigaoctet.) (wikipedia.com)

### Solution

L'architecture événementielle, autour de `poll()`, `epoll()`, etc, résout ce problème en permettant à un seul thread de gérer plusieurs connexions efficacement. Nginx utilise cette méthode (à l'inverse d'Apache).

## Fondamentaux asynchrone

En programmation asynchrone basée sur un mécanisme de surveillance comme poll/epoll, le programme utilise une boucle événementielle qui attend que les fds deviennent prêts, puis traite chaque connexion uniquement lorsqu’elle a effectivement des données à lire ou à écrire, ce qui évite de bloquer inutilement sur une connexion individuelle.

## Les mécanismes poll, select, epoll et kqueue: le multiplexage d'I/O

Le multiplexage *(dividing the capacity of the communication channel into several logical channels, one for each message signal or data stream to be transferred)* permet de surveiller plusieurs fds (sockets) simultanéments tout en gardant un principe de fonctionnement non-bloquant.

- `poll()`
  - Approche classique POSIX: API relativement simple, crée un tableau de structures `pollfd`, chaque structure contenant: un fd et les événements surveillés (`events`: POLLIN pour la lecture, POLLOUT pour l'écriture). `poll()` est bloquant jusqu'à ce qu'un ou plusieurs fds soient prets, puis return en remplissant un champ `revents` de chaque struct.
  - Complexité: **O(n)** car le noyau scanne tous les fds à chaque appels, meme si seuls quelques-uns sont actifs.

- `epoll()`
  - Spécifique à LINUX, évolution majeure: Le noyau mémorise quels descripteurs sont surveillés (interest list) et garde dans une liste séparée ceux qui sont réellement prets (ready list).
  - Utilisation d'`epoll()`: instanciation via `epoll_create()`, ajout/modification/suppression des fds via `epoll_ctl()` puis renvoie (return) des fds prets avec `epoll_wait()`.
  - Compléxité: **O(1)** car peu importe le nombre de connexion, `epoll()` return seulement celles qui ont des événements.

- `select()` (introduction pour l'exam)
  - Plus ancien mais universel, et limité. Il utilise des bitmaps (fd_set) plutot que des tableaux, avec un limite fixée à `FD_SETSIZE` (en gros 1024 fds). Il faut reconstruire cet bitmaps après chaque appel car le noyau les modifie. Seul avantage: portabilité universelle mais extrémement limité a cause de sa barrière de taille.

- `kqueue()` (flemme)

⚠️ Choix a faire entre `poll()` et `epoll()`, les deux sont très bien, on prendra la décision en fonction de la pénibilité d'utilisation apres avoir fais pros/cons je suppose ? A voir

## Sockets + Gestion des Sockets

**WIP**

Sockets configurées en mode non bloquant (`fcntl(fd, F_SETFL, O_NONBLOCK)`)
- [F_SETFL](https://linux.die.net/man/3/fcntl#:~:text=F_SETFL,result%20is%20unspecified.)
- [O_NONBLOCK](https://linux.die.net/man/7/socket#:~:text=It%20is%20possible,select(2).)

Aparté: ⚠️ **Check Nginx** `client_header_timeout` et `client_body_timeout`, permettent de fermer automatiquement les connexions anormalement lentes, mais meme sans ces timeouts, le server continue de fonctionner pour tous les autres clients (grace a la event loop, pendant que les sockets attendent des données réseaux, la loop est libre de traiter les sockets qui ont des données disponibles). Le thread (process principale pour nous, là où est l'event loop) travaille constamment sur des taches productives et ne gaspille jamais de temps à attendre passivement.

### Exemple d'implémentation non-bloquante avec `poll()`

Le serveur accepte la connexion d'un client lent (connexion ADSL par exemple), configure le socket en non-bloquant, l'ajoute au tableau `pollfd`. A chaque itération de l'event loop, `poll()` indique quand ce socket est pret pour l'écriture: POLLOUT (en partant du postulant que le client veuille télécharger un fichier). Le serveur envoie quelques kilobytes du fichier, puis retourne immédiatement à `poll()` pour traiter d'autres connexions (car il est configuré comme non-bloquant). Sur toute la durée du téléchargement du fichier par le client lent, le serveur est aussi capable d'accepter et servir d'autres clients.

#### Autre cas plus spécifique, POST de fichier via CGI: formultaire qui permet d'upload des images vers un script php:

Avec l'approche non-bloquante, le serveur recoit les données par chunks chaque fois que `poll()` indique POLLIN sur le socket client (identification de chunks depuis le header de la requete), accumule les données dans un buffer ou un fichier temporaire, et quand l'upload est complétée, `fork()` pour exécuter le CGI, surveille la sortie du CGI via `poll()` également (⚠️ on ne peut pas `wait()` le child de la meme facon que dans minishell car le serveur serait bloqué dans le cas ou le script est lent/infinis).
