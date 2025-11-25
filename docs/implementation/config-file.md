# TODO (ordered ?)

- [x] creer un fichier default.conf avec toutes les spec que nous gerrerons (des directives seront rajoutées/supprimées au fur et a mesure du temps)
- [x] creer un dossier avec differents fichiers de config qui fonctionne/fonctionne pas pour une potentielle github action
- [x] trouver une structure cool
- [x] creer les dossiers/fichiers de la structure
- [x] github actions config file + siege (plus tard)?
- [ ] github action implementing siege
- [ ] detailler choix pour chaque directive


[CONFIGURATION FILE STRUCTURE](https://nginx.org/en/docs/beginners_guide.html#conf_structure) (useful to create accepted and rejected configuration files)

[another link, looks a bit more detailed](https://devdocs.io/nginx/beginners_guide#conf_structure)




Si t'as un peu de temps tu peux me dire ce qui te convient le plus stp (dans l'idée je me pose ces questions dans le but de savoir a quel point on décide de faire un parsing compliqué):
dans nos fichier de config il y a plusieurs possibilités:
https://github.com/AzehLM/webserv/blob/main/docs/configuration-file.md#exemple-complet-de-configuration-location
soit on accepte que des context server (context = block) comme sur l'exemple au dessus.
- avec du coup toutes les directives obligatoirement a l'intérieur et on a pas



Validator rules:

### error_page:
```
Syntax: error_page code ... [=[response]] uri;
Default: —
Context: global, server, location
```
Defines the URI that will be shown for the specified errors. Example:
```
error_page 400		/400.html;
error_page 500 501	/50x.html;
```

### error_log:
```
Syntax: error_log /path/to/file;
Default: error_log /path/to/define;
Context: global
```
Defines the file where the logs will be written. A default path will be define. This directive can only be defined in the global context. Example:
```
error_log /logs/error.log;
```

### client_max_body_size:
```
Syntax:	client_max_body_size size;
Default: client_max_body_size 1m;
Context: global, server, location
```
Sets the maximum allowed size of the client request body. If the size in a request exceeds the configured value, the `413` (Request Entity Too Large) error is returned to the client. Different size unity can be accepted (k, K, m, M, g, G). Example:
```
client_max_body_size 10M;
client_max_body_size 1g;
```
Note: Need to define maximum size we allow, if the value is higher to our max we either reject the config file or set it to de Default setting.

### server:
```
Syntax: server { ... }
Default: —
Context: global
```
Sets configuration for a virtual server.

### listen:
```
Syntax: listen address[:port];
Default: *:80;
Context: server
```
Sets the address and port for IP on which the server will accept requests. Both address and port, or only address or only port can be specified. An `address` may also be `localhost`, for example:
```
listen 127.0.0.1:8000;
listen 127.0.0.1;
listen 8000;
listen *:8000;
listen localhost:8000;
```

### server_name
```
Syntax: server_name name ...;
Default: server_name "";
Context: server
```
The server_name is like a filter that tells our server: "Hey, if you receive a request with this domain name in the Host header, use THIS server block to process it."
Sets names of a virtual server, for example:
```
server {
	server_name example.com;
}
```
Note: Nginx has a regex captures that we won't handle, just like regular expressions creating variables that could be later used in other directives.

### root
```
Syntax: root path;
Default: root html;
Context: server, location
```
Sets the root directory for requests. Example:
```
location /i/ {
    root /data/w3;
}
```
The `/data/w3/i/top.gif` file will be sent in response to the “`/i/top.gif`” request.
Note: again, not handling variables.
A path is constructed by adding a URI to the value of the `root` directive.

### index
```
Syntax: index file ...;
Default: index index.html;
Context: server, location
```
Defines files that will be used as an index. Files are checked in the specified order.

### location
```
Syntax:	location uri { ... }
Default: —
Context: server
```
Sets configuration depending on a specified URI
Note: We do not accept nested location contexts

### allow_methods
```
Syntax: allow_methods ...;
Default: allow_methods GET|POST|DELETE;
Context: server, location
```
Allowed method for the current context

### autoindex
```
Syntax: autoindex on|off;
Default: autoindex off;
Context: server, location
```
Enables or disables directory listing

### upload_to
```
Syntax: upload_to /path;
Default: —
Context: server, location
```
Defines the path to upload files to. Has to be combined with POST method

### return
```
Syntax:	return code URL;
Default: —
Context: server, location
```

### alias
```
Syntax:	alias path;
Default: —
Context: location
```
Defines a replacement for the specified location. Example:
```
location /i/ {
    alias /data/w3/images/;
}
```
On request of “`/i/top.gif`”, the file `/data/w3/images/top.gif` will be sent.

### cgi_path (everything here is not definitive, needs more research on methods requirements or not)
```
Syntax:	cgi_path path;
Default: —
Context: location
```
Path to the CGI interpretor, Example:
```
location /cgi-test {
	cgi_path /usr/bin/python3;
}
```

### cgi_ext
```
Syntax: cgi_ext extension;
Default: —
Context: location
```
Defines the extention of file executed via CGI. Needs `cgi_path` directive. Example:
```
location /cgi-test {
	cgi_path /usr/bin/python3;
	cgi_ext .py;
}
```
