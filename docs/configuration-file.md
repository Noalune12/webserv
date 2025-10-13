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
