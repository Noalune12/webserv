# Nginx/web-servers configuration file documentation links

[nginx beginner's guide](https://nginx.org/en/docs/beginners_guide.html#conf_structure)

[HTTP Headers (not sure it is meant for HTTP/1.1)](https://cheatsheetseries.owasp.org/cheatsheets/HTTP_Headers_Cheat_Sheet.html)

[Understanding the Nginx Configuration File Structure and Contexts](https://mangohost.net/blog/understanding-the-nginx-configuration-file-structure-and-contexts/)

[Nginx common configuration video](https://www.youtube.com/watch?v=MP3Wm9dtHSQ)

[Comprendre directives nginx](https://www.nicelydev.com/nginx/comprendre-nginx-conf)

### Nginx configuration structure:

Les fichiers de conf de Nginx suivent une hierarchie de blocs stucturels (terme technique: `context`) contenant des `directives`. Un `context` peut etre redefinis par `scope`.
Chaque `context` definis des `directives` mais ils peuvent egalement preciser comment ils en heritent des `contexts parents`.

Ex:
```nginxconf
http {                           # Context parent
    gzip on;                     # Directive héritée par tous les contexts enfants

    server {                     # Context enfant (scope spécifique)
        listen 80;               # Directive propre à ce server
        gzip_comp_level 6;       # Redéfinit/précise le comportement de gzip
    }
}
```

Les contexts clefs:

- **Main context**: Le scope global qui contient les directives affectant l'entierete du server web
- **Events context**: Gère les paramètres de traitement de connexion
- **HTTP context**: Contient tous les parametres de configuration HTTP
- **Server context**: Definis les settings propres aux server**s** ainsi que les **virtual host** (on verra si on fait ca)
- **Location context**: Contexts specific a des locations (redirections) a l'interieur d'un bloc server
- **Upstream context**: Definis les groupes de servers pour l'equilibrage des charges (pas compris ca encore)

Exemple d'un fichier de config Nginx basique:

```nginxconf
# Main context - global directives
user nginx;
worker_processes auto;

# Events context
events {
    worker_connections 1024;
    use epoll;
}

# HTTP context
http {
    include /etc/nginx/mime.types;
    default_type application/octet-stream;

    # Server context
    server {
        listen 80;
        server_name example.com;

        # Location context
        location / {
            root /var/www/html;
            index index.html;
        }

        location /api {
            proxy_pass http://backend;
        }
    }

    # Upstream context
    upstream backend {
        server 192.168.1.10:8080;
        server 192.168.1.11:8080;
    }
}
```

## Main context configuration detailed

### user

- `user`: Cette directive spécifie l'utilisateur système sous lequel les `working processes` d'Nginx s'exécuteront. Essentielle pour la gestion des privilèges, hors contexte spécifique, elle est nécessaire à la sécurisation des opérations du serveur web.
  - Après que Nginx (ou le webserver) soit lancé (souvent avec des droits root pour set les privilèges tel que les ports (80, 443...)) on change les privilèges via cette spécification pour que les `worker children` n'ai pas tous ces droits, minimisant les risques si ces process soient compromis.
  - Dans l'ensemble c'est de bonne pratique de spécifier le `user` pour éviter les vulnérabilités
  - Par défaut, si cette directive n'est pas spécifiée, Nginx use un user par défaut (`nobody`, `nginx`, `www-data`) ou autre n'ayant qu'un minimum de droits (juste assez pour lire les fichiers html et/ou lire/écrire dans des fichiers de logs)
  - **Si le `user` spécifié, il faut le créer** avec des droits appropriés

⚠️ **De ce que le sujet demande, je ne pense pas que ce soit nécessaire de gérer/créer nous meme un/des user. Ca a l'air de complexifier pas mal le projet**

### worker_processes

- `worker_processes`: Spécifie au server le nombre d'`operating system processes` qu'Nginx doit créer pour les connexions entrantes.
  - Il est recommandé de le set au nombre de CPU disponible, `auto` permet de mettre en place cette configuration de facon automatique.
  - Chaque worker est `single-threaded`, ils gèrent parallèlement les requêtes entre eux en se divisant les connexions.
  - **Relation avec worker_connections**: Le nombre total de connexions que peut gérer Nginx est égal a la multiplication du nombre de `worker_processes` et du nombre de `worker_connections`.

⚠️ **Meme chose que pour user, le sujet ne demande pas spéficiquement de gérer ce context. Dans l'idée on peut le set a un que la directive soit présente ou non, au moment ou j'écris ca je n'ai pas regardé si on nous autorise les fonctions nécessaire à l'identification du nombre de CPU disponible**


### pid

- `pid`: **Je ferrai plus tard si nécessaire, meme chose je n'ai pas l'impression qu'on nous donne les outils nécessaire à la gestion de cette directive**

### include

- `include`: Meme chose que pour pid, je pense pas que ce soit demandé, on nous demande de gérer un fichier de conf, je m'attend a ce que tout soit dedans. Mais comme pour les logs d'erreurs ca peut etre stylé de gerer ca. Au final c'est que du parsing et j'ai pas l'impression que ce soit si dur


## Events context setup detailed

### worker_connections

- `worker_connections`: Spécifie le nombre maximum de connexions simultanées que peut ouvrir un worker process (définis dans `worker_processes`). Si le serveur gère un grand nombre de connexions concurrentes, on peut atteindre cette limite, causant l'abandon des nouvelles connexions
  - Ce paramètre est crucial à la gestion du nombre de clients servis en simultanée.

### use

- `use`: Spécifie la méthode de gestion des événements I/O à utiliser (select, poll, epoll, kqueue, etc.). Par défaut, nginx sélectionne automatiquement la méthode la plus efficace disponible sur le système
  - Pour notre projet: le sujet mentionne qu'on peut utiliser poll(), select(), kqueue() ou epoll()
  - Ce paramètre détermine comment le serveur va multiplexer les I/O
  - Sers à rien pour notre, je vois pas un monde ou on décide de multiplier notre codebase pour répondre à cette directive, ca fait pas sens.

---
# Séparation avec le dessus, on gere pas ca

## Server context setup detailed

### listen

- `listen`: Définit l'adresse IP et le port sur lesquels le serveur va écouter les connexions entrantes
  - Syntaxe: `listen [address:]port [default_server];`
  - Exemples:
    - `listen 80;` - écoute sur le port 80 sur toutes les interfaces
    - `listen 127.0.0.1:8080;` - écoute uniquement sur localhost port 8080
    - `listen 192.168.1.10:443;` - écoute sur une IP spécifique
  - **Important pour le sujet**: Le sujet demande de pouvoir définir plusieurs paires interface:port pour servir plusieurs sites web
  - Un serveur peut avoir plusieurs directives `listen` pour écouter sur plusieurs ports/interfaces
  - Le paramètre `default_server` indique quel bloc server utiliser par défaut si aucun server_name ne correspond

### server_name

- `server_name`: Définit le(s) nom(s) de domaine associé(s) à ce bloc server (utilisé pour le virtual hosting (implementé en HTML/1.1 si je dis pas de betises, à vérifier))
  - Syntaxe: `server_name name1 [name2 ...];`
  - Exemples:
    - `server_name example.com;`
    - `server_name example.com www.example.com;`
  - Le serveur utilise le header HTTP `Host:` de la requête pour déterminer quel bloc server utiliser
  - Si aucun server_name ne correspond, le serveur marqué `default_server` est utilisé
  - **Note du sujet**: Le virtual hosting est considéré hors scope mais autorisé si on veut l'implémenter

### error_page

- `error_page`: Définit des pages d'erreur personnalisées pour des codes d'état HTTP spécifiques
  - Syntaxe: `error_page code [code...] [=response_code] uri;`
  - Exemples:
    - `error_page 404 /404.html;` - affiche /404.html pour les erreurs 404
    - `error_page 500 502 503 504 /50x.html;` - même page pour plusieurs codes
    - `error_page 404 =200 /empty.gif;` - change le code de réponse
  - **Important pour le sujet**: Le serveur doit avoir des pages d'erreur par défaut si aucune n'est fournie
  - Les URIs spécifiés sont relatifs à la directive `root` du contexte
  - Si le fichier d'erreur n'existe pas, nginx utilise sa propre page d'erreur par défaut (nous on en aura, mais je sais pas si ils ont l'autorisation de delete nos fichiers, à prendre en compte)
  - [nginx doc](https://nginx.org/en/docs/http/ngx_http_core_module.html#error_page)

### client_max_body_size

- `client_max_body_size`: Définit la taille maximale autorisée du corps de la requête client (request body)
  - Syntaxe: `client_max_body_size size;`
  - Exemples:
    - `client_max_body_size 1M;` - limite à 1 mégaoctet
    - `client_max_body_size 10m;` - limite à 10 mégaoctets
  - Il nous est demandé de pouvoir configurer cette limite
  - Si une requête dépasse cette limite, le serveur retourne une erreur **413** (Request Entity Too Large)
  - Cette directive est cruciale pour:
    - Protéger le serveur contre les attaques par upload massif et gérer l'upload de fichiers volumineux de manière contrôlée
  - La valeur par défaut dans nginx est 1M, à nous de choisir ce qu'on met
  - **Unités acceptées**: k ou K (kilobytes), m ou M (megabytes), g ou G (gigabytes)
  - **Détail d'implémentation**: Le serveur doit vérifier le header `Content-Length` de la requête avant de commencer à lire le body. Si le Content-Length dépasse la limite, rejeter immédiatement la requête avec un 413

### root

- `root`: Définit le répertoire racine pour les requêtes dans ce contexte
  - Syntaxe: `root path;`
  - Exemples:
    - `root /var/www/html;`
    - `root /usr/share/nginx/html;`
  - Le chemin final sera construit en ajoutant l'URI de la requête au chemin root
  - Exemple: si `root /var/www;` et requête pour `/images/photo.jpg`, le serveur cherchera `/var/www/images/photo.jpg`
  - **Important pour le sujet**: Peut être défini au niveau `server` (par défaut) ou `location` (pour override)
  - Cette directive est **héritée**: si définie dans `server` mais pas dans `location`, la valeur du `server` est utilisée

### index

- `index`: Définit le(s) fichier(s) à servir par défaut quand une requête pointe vers un répertoire
  - Syntaxe: `index file1 [file2 ...];`
  - Exemples:
    - `index index.html;`
    - `index index.html index.htm default.html;`
    - `index api.html;` (dans un contexte location spécifique)
  - Le serveur teste les fichiers dans l'ordre spécifié et sert le premier qui existe
  - Si aucun fichier index n'existe et que `autoindex` est activé, une liste du répertoire est générée
  - Si aucun fichier index n'existe et que `autoindex` est désactivé, une erreur 403 (Forbidden) est retournée
  - **Important pour le sujet**: Explicitement demandé comme directive configurable, mais un doute si on doit gérer autoindex, aucune idée de la compléxité que ca rajoute

## Location context setup detailed

Le contexte `location` permet de définir des règles spécifiques pour certaines URIs ou patterns d'URIs. C'est le cœur de la configuration des routes dans le serveur.

### location (bloc)

- `location`: Définit une configuration pour une URI ou un pattern d'URI spécifique
  - Syntaxe: `location [modifier] uri { ... }`
  - Exemples:
```nginxconf
    location / {
        root /var/www/html;
    }

    location /api {
        root /var/www/api;
    }

    location /images {
        root /data;
        autoindex on;
    }
```
  - **Note du sujet**: Pas besoin de gérer les regex, seulement les matchs de préfixes exacts
  - Le serveur choisit le location avec le préfixe le plus long qui correspond à l'URI
  - Exemple: pour `/api/users`, si on a `location /` et `location /api`, c'est `/api` qui sera utilisé

### allow_methods (ou limit_except)

- `allow_methods`: Définit les méthodes HTTP autorisées pour cette location
  - Syntaxe: `allow_methods METHOD1 [METHOD2 ...];`
  - Exemples:
    - `allow_methods GET POST;`
    - `allow_methods GET POST DELETE;`
    - `allow_methods GET;` (read-only)
  - **Important pour le sujet**: Explicitement demandé - au minimum GET, POST, DELETE doivent être supportés
  - Si une méthode non autorisée est utilisée, le serveur retourne 405 (Method Not Allowed)
  - La réponse 405 doit inclure un header `Allow:` listant les méthodes autorisées
  - Note: Dans nginx standard, cette fonctionnalité utilise `limit_except`, mais pour notre projet on peut simplifier avec `allow_methods`

### return (redirections)

- `return`: Configure une redirection HTTP pour cette location
  - Syntaxe: `return code [text|URL];`
  - Exemples:
    - `return 301 /new-location;` - redirection permanente
    - `return 302 /temporary;` - redirection temporaire
    - `return 301 https://example.com$request_uri;` - redirection vers un autre domaine
  - Codes de redirection communs:
    - 301: Moved Permanently (redirection permanente)
    - 302: Found (redirection temporaire)
    - 303: See Other
    - 307: Temporary Redirect
    - 308: Permanent Redirect
  - Quand une redirection est définie, le serveur:
    1. Retourne le code de statut spécifié
    2. Ajoute un header `Location:` avec la nouvelle URL
    3. Ne traite pas le reste de la configuration de cette location
  - J'ai pas compris ce que demande le sujet ici pour etre honnete, j'ai vu des redirection mais de la à gérer les différents code je suis pas persuadé que ce soit demandé

### alias vs root dans location

- Différence importante entre `root` et `alias` dans un contexte location:
  - `root`: Ajoute le chemin de la location au chemin root
```nginxconf
    location /images {
        root /data;
    }
    # URI: /images/photo.jpg → Fichier: /data/images/photo.jpg
```
  - `alias`: Remplace le chemin de la location par le chemin alias
```nginxconf
    location /images {
        alias /data/photos;
    }
    # URI: /images/photo.jpg → Fichier: /data/photos/photo.jpg
```
  - **Pour le sujet**: L'exemple donné (`/kapouet` → `/tmp/www`) correspond au comportement de `alias`
  - Si on utilise `root`, il faudrait que `/kapouet` pointe vers un répertoire qui contient lui-même un dossier `kapouet`

### autoindex

- `autoindex`: Active ou désactive la génération automatique de listings de répertoires
  - Syntaxe: `autoindex on|off;`
  - Défaut: `autoindex off;`
  - Exemples:
```nginxconf
    location /downloads {
        root /var/www;
        autoindex on;
    }
```
  - Quand activé et qu'un répertoire est demandé sans fichier index:
    - Le serveur génère une page HTML listant les fichiers et sous-répertoires
    - Chaque entrée est un lien cliquable
  - Quand désactivé et qu'un répertoire est demandé sans fichier index:
    - Le serveur retourne une erreur 403 (Forbidden)
  - Format typique d'un [listing](http://188.165.227.112/portail/)

### upload_to (ou client_body_temp_path)

- `upload_to`: Définit le répertoire où les fichiers uploadés par les clients seront stockés
  - Syntaxe: `upload_to path;`
  - Exemples:
    - `upload_to /var/www/uploads;`
    - `upload_to /tmp/uploads;`
  - Cette directive doit être combinée avec:
    - **Une méthode POST**
    - Un `client_max_body_size` approprié pour accepter les fichiers
  - Considérations d'implémentation:
    - Vérifier que le répertoire existe et est accessible en écriture
    - Gérer les noms de fichiers en conflit (écrasement ou renommage)
    - Nettoyer les uploads partiels en cas d'erreur
    - Valider les types de fichiers si nécessaire (sécurité)
  - Le serveur doit gérer les uploads avec `Content-Type: multipart/form-data` ou `application/octet-stream`
  - **Détail d'implémentation**:
    - Pour les requêtes POST avec multipart/form-data, parser les boundaries
    - Extraire le nom du fichier du header `Content-Disposition`
    - Écrire les données reçues dans le fichier de destination
    - Retourner 201 (Created) en cas de succès, avec un header Location pointant vers la ressource créée

## [tdameros documentation for the webserv configuration file](https://github.com/tdameros/42-webserv/blob/main/docs/config_file.md)

> A prendre comme référence, il reprend le format de la doc de Nginx et limite les directives a celle sont requises par le sujet (nos fichiers de configs de test sont basé sur la hiérarchies décrite dans sa doc).

### Exemple complet de configuration location
```nginxconf
server {
    listen 8080;
    server_name example.com;
    root /var/www/html;
    index index.html;
    client_max_body_size 10M;

    # Route par défaut - site statique
    location / {
        allow_methods GET;
        autoindex off;
    }

    # API avec plusieurs méthodes (sur les webserv que j'ai évalué c'était souvent la page d'accueil, avec toutes les méthodes testable)
    location /api {
        allow_methods GET POST DELETE;
        root /var/www/api;
        index api.html;
    }

    # Zone de téléchargement avec listing
    location /downloads {
        allow_methods GET;
        root /var/www;
        autoindex on;
    }

    # Upload de fichiers
    location /upload {
        allow_methods POST;
        upload_to /var/www/uploads;
        client_max_body_size 50M;
    }

    # Redirection
    location /old-page {
        return 301 /new-page;
    }

    # Exemple avec alias
    location /docs {
        alias /usr/share/documentation;
        allow_methods GET;
        autoindex on;
    }
}
```

## DUMP CONFIGURATION FILES, WILL TREAT LATER

```nginxconf
server {
  listen 8080;                         # Port to listen on
  host 127.0.0.1;                      # Interface/IP to bind
  server_name mywebsite;               # (Optional) Name for virtual hosting
  error_page 404 /errors/404.html;     # Default error page path
  client_max_body_size 1048576;        # Maximum request size (bytes)
  root /var/www/html;                  # Default root directory
  index index.html;                    # Default file to serve in a directory

  location /api {
    allow_methods GET POST DELETE;     # Allowed HTTP methods
    autoindex off;                     # Directory listing enabled/disabled
    root /var/www/api;                 # Directory for this route
    return /api/new;                   # HTTP redirect for this path
    upload_to /var/www/uploads;        # Directory for file uploads
    cgi_path /usr/bin/python3;         # Path to CGI interpreter
    cgi_ext .py;                       # Extension to trigger CGI
    index api.html;                    # Default file for this route
  }
}
```

```nginxconf
# cat default.conf
server {
    listen       80;
    server_name  localhost;

    #access_log  /var/log/nginx/host.access.log  main;

    location / {
        root   /usr/share/nginx/html;
        index  index.html index.htm;
    }

    #error_page  404              /404.html;

    # redirect server error pages to the static page /50x.html
    #
    error_page   500 502 503 504  /50x.html;
    location = /50x.html {
        root   /usr/share/nginx/html;
    }

    # proxy the PHP scripts to Apache listening on 127.0.0.1:80
    #
    #location ~ \.php$ {
    #    proxy_pass   http://127.0.0.1;
    #}

    # pass the PHP scripts to FastCGI server listening on 127.0.0.1:9000
    #
    #location ~ \.php$ {
    #    root           html;
    #    fastcgi_pass   127.0.0.1:9000;
    #    fastcgi_index  index.php;
    #    fastcgi_param  SCRIPT_FILENAME  /scripts$fastcgi_script_name;
    #    include        fastcgi_params;
    #}

    # deny access to .htaccess files, if Apache's document root
    # concurs with nginx's one
    #
    #location ~ /\.ht {
    #    deny  all;
    #}
}
```

```nginxconf
# cat nginx.conf

user  nginx;
worker_processes  auto;

error_log  /var/log/nginx/error.log notice;
pid        /run/nginx.pid;


events {
    worker_connections  1024;
}


http {
    include       /etc/nginx/mime.types;
    default_type  application/octet-stream;

    log_format  main  '$remote_addr - $remote_user [$time_local] "$request" '
                      '$status $body_bytes_sent "$http_referer" '
                      '"$http_user_agent" "$http_x_forwarded_for"';

    access_log  /var/log/nginx/access.log  main;

    sendfile        on;
    #tcp_nopush     on;

    keepalive_timeout  65;

    #gzip  on;

    include /etc/nginx/conf.d/*.conf;
}
```
