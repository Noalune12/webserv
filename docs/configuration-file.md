# Nginx/web-servers configuration file documentation links

[nginx beginner's guide](https://nginx.org/en/docs/beginners_guide.html#conf_structure)

[HTTP Headers (not sure it is meant for HTTP/1.1)](https://cheatsheetseries.owasp.org/cheatsheets/HTTP_Headers_Cheat_Sheet.html)

[Understanding the Nginx Configuration File Structure and Contexts](https://mangohost.net/blog/understanding-the-nginx-configuration-file-structure-and-contexts/)


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
error_log /var/log/nginx/error.log;

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
  - Chaque worker est `single-threaded`, ils gérent parallelement les requêtes entre eux en ce divisant les divisants.
  - **Relation avec worker_connections**: Le nombre total de connexions que peut gérer Nginx est égal a la multiplication du nombre de `worker_processes` et du nombre de `worker_connections`.

⚠️ **Meme chose que pour user, le sujet ne demande pas spéficiquement de gérer ce context. Dans l'idée on peut le set a un que la directive soit présente ou non, au moment ou j'écris ca je n'ai pas regardé si on nous autorise les fonctions nécessaire à l'identification du nombre de CPU disponible**


### pid

- `pid`: **Je ferrai plus tard si nécessaire, meme chose je n'ai pas l'impression qu'on nous donne les outils nécessaire à la gestion de cette directive**


### error_log

- `error_log`: Spéficie ou et a quel niveau de gravité les messages d'erreurs du webserver sont enregistrés.
  - `error_log /path/to/logs`: le chemin du fichier ou les logs d'erreurs sont écrits.
  - niveaux de gravités: `debug`, `info`, `notice`. Seules les erreurs égales ou supérieures seront écritent dans le fichier de logs si un niveau de gravité est définit.
  - On peut positionné dans différents contexte cette directive, qui sera alors écrasé du/des contexte.s parent.

Exemple:

```nginxconf
http {
  error_log /var/log/nginx/error.log warn;

  server {
    error_log /var/log/nginx/domain.error.log error;
    ...
  }
}
```

⚠️ **Pour le coup je pense que c'est cool si on l'implemente celle-ci. A voir si j'ai bien compris comment ca fonctionne mais si c'est les retours du client (browser) qui vont dans les logs ca peut etre stylé!**

### include

- `include`: **Meme chose que pour pid, je pense pas que ce soit demandé, on nous demande de gérer un fichier de conf, je m'attend a ce que tout soit dedans. Mais comme pour les logs d'erreurs ca peut etre stylé de gerer ca. Au final c'est que du parsing et j'ai pas l'impression que ce soit si dur**

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
