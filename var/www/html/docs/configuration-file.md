# Configuration file learning documentation/notes

## Nginx/web-servers configuration file used links

[nginx beginner's guide](https://nginx.org/en/docs/beginners_guide.html#conf_structure)

[Understanding the Nginx Configuration File Structure and Contexts](https://mangohost.net/blog/understanding-the-nginx-configuration-file-structure-and-contexts/)

### Nginx configuration structure:

The Nginx configuration file is organized as a hierarchy of structural blocks (`contexts`) containing `directives`.
A `context` may be redefined depending on its scope.
Each `context` defines `directives` and can also specify how `directives` are inherited from parent `contexts`.


Ex:
```nginxconf
http {                           # Parent context
    gzip on;                     # Directive inherited by every child context

    server {                     # Child context (scope specific)
        listen 80;               # Directive specific to this server
        gzip_comp_level 6;       # Redefinition of gzip global directive
    }
}
```

Key contexts:

- **Main context**: The global scope that has the directives affecting the entirety of the web server.
- **Events context**: Manages the connection parameters
- **HTTP context**: Has the HTTP configuration parameters
- **Server context**: Defines the settings of the servers as well as the **virtual hosts**
- **Location context**: Contexts that specify the locations (redirections) inside of a server block

> Note that in webserv, we only manage to work with HTTP and lower contexts.

Example of a basic Nginx config file:

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
}
```

## Server context setup detailed

### listen

- `listen`: defines the IP address and port on which the server will listen for incoming connections.
  - Syntax: `listen [address:]port;`
  - Examples:
    - `listen 80;` - listen on port 80 on all interfaces
    - `listen 127.0.0.1:8080;` - listen only on localhost on port 8080
    - `listen 192.168.1.10:443` - listen on a specific IP address
  - **Subject specification**: The subject asks to be able to define multiple pairs of interface/port to serve several web servers.
  - A server can have multiple `listen` directives to listen on multiple interfaces/ports.

### server_name

- `server_name`: defines the domain name(s) associated with the server block (used for virtual hosting, which was implemented in the HTTP/1.1 version of the protocol).
  - Syntax: `server_name name1 [name2 ...];`
  - Examples:
    - `server_name example.com;`
    - `server_name example1.com example2.com;`
  - The server uses the HTTP header `Host:` of the request to identify which server block to use.
  - If no `server_name` matches, the `default_server` or default `server_name` will be used.
  - **Subject** says the virtual hosting is out of the scope of this project but we decided to implement it.

### error_page

- `error_page`: defines the custom error pages to use for specific HTTP status codes.
  - Syntax: `error_page code [code ...] [=response_code] uri;`
  - Examples:
    - `error_page 404 /404.html;` - respond with the /404.html file for 404 errors.
    - `error_page 500 502 503 504 /50x.html;` - same page for several error codes.
  - **Subject important note**: The server has to provide default error pages if none is given by the server.
  - The specified URIs are relative to the `root` directive of the current `context`
  - If the error file doesn't exist, nginx uses its own default error page
  - [nginx doc](https://nginx.org/en/docs/http/ngx_http_core_module.html#error_page)

### client_max_body_size

- `client_max_body_size`: defines the maximum size a body of a client request can be
  - Syntax: `client_max_body_size size;`
    - Examples:
    - `client_max_body_size 1M;` - limits to 1 megabyte
    - `client_max_body_size 10m;` - limits to 10 megabytes
  - If a request has a body size bigger than the max, the server returns a **413** (Content Too Large) error
  - This directive is crucial to protect the server from attacks by massive uploads and to manage voluminous file uploads in a controlled way.
  - The default value of nginx is 1M
  - **Implementation detail**: the server has to check the header `Content-Length` of the request before starting to read the body. If the `Content-Length` is above the limit then it gets immediately rejected and sends a **413**.

### root

- `root`: defines the root directory for the requests in the current context.
  - Syntax: `root path;`
  - Examples:
    - `root /var/www/html;`
    - `root /usr/share/nginx/html;`
  - The final path will be built by adding the URI of the request to the root path
  - Example: if `root /var/www;` and request for `/images/photo.jpg`, the server will look for `/var/www/images/photo.jpg`
  - **Important information**: This can be defined at the `server` level (by default) or overridden in `location`
  - This directive is inherited in `location` if not defined in those scopes; then we use the `server` context `root` directive value.

### index

- `index`: defines the files used by default when a request points to a directory
  - Syntax: `index file1 [file2 ...];`
  - Examples:
    - `index index.html;`
    - `index index.html index.htm default.html;`
    - `index api.html;`
  - The server tests the files in the specified order and serves the first one that exists.
  - If no index file exists and the `autoindex` directive is `on`, a directory listing is generated
  - If no index file exists and the `autoindex` directive is `off`, a **403** (Forbidden) is returned
  - **Subject** explicitly asks to handle `index` and `autoindex`


### location (block)

The `location` context allows defining rules specific to URIs or URI patterns. This is the heart of the route configuration of a server.

- `location`: defines a configuration block for a URI
  - Syntax: `location uri { ... };`
  - Example:
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
  - **Subject note**: Not handling regex, only the prefix matching
  - The server chooses the location with the longest prefix corresponding to the URI
  - Example: for `/api/users`, if we have a `location /` and `location /api`, it's `/api` that is chosen

### allow_methods

- `allow_methods`: defines the authorized HTTP methods in the location
  - Syntax: `allow_methods METHOD1 [METHOD2 ...];`
  - Examples:
    - `allow_methods GET POST;`
    - `allow_methods GET POST DELETE;`
    - `allow_methods GET;` (read-only)
  - **Subject note**: Explicitly asked to support at least `GET | POST | DELETE`
  - If an unauthorized method is used, the server returns a **405** Method Not Allowed
  - The **405** response has to include the `Allow:` header, listing the authorized methods.


### return (redirections)

- `return`: defines an HTTP redirection to a specified location
  - Syntax: `return [code] [text|URL];`
  - Examples:
    - `return 301 /new-location;` - permanent redirection
    - `return 302 /temporary;` - temporary redirection
    - `return 301 https://example.com$request_uri;` - redirection to another domain
  - Common redirection codes:
    - 301: Moved Permanently
    - 302: Found
    - 303: See Other
    - 307: Temporary Redirect
    - 308: Permanent Redirect
  - When a redirection is defined, the server:
    1. Returns the specified status code
    2. Adds a `Location:` header with the new URL
    3. Does not process the rest of the configuration for this location

### alias vs root in location

- `root`: adds a path to the root location
  - Important differences between `root` and `alias` in a `location` context
```nginxconf
    location /images {
        root /data;
    }
    # URI: /images/photo.jpg → File: /data/images/photo.jpg
```
  - `alias`: Replaces the path of the `location` with the `alias` path
```nginxconf
    location /images {
        alias /data/photos;
    }
    # URI: /images/photo.jpg → File: /data/photos/photo.jpg
```
  - **Subject**: The example in the subject (`/kapouet` → `/tmp/www`) shows the `alias` behavior.

### autoindex

- `autoindex`: Activates or deactivates the automatic generation of a directory listing
  - Syntax: `autoindex on|off;`
  - Example:
```nginxconf
    location /downloads {
        root /var/www;
        autoindex on;
    }
```
  - When `on` and with a directory without `index`:
    - The server generates an HTML page listing the files and subdirectories
    - Each entry is a clickable link
  - When `off` and a directory is requested without an `index` file:
    - The server returns a **403** (Forbidden)
  - Typical format of a [listing](http://188.165.227.112/portail/) with `autoindex`

### upload_to

- `upload_to`: defines the directory where the clients' uploaded files will be stored
  - Syntax: `upload_to path;`
  - Examples:
    - `upload_to /var/www/uploads;`
    - `upload_to /tmp/uploads;`
  - This directive should be combined with:
    - the `POST` method
    - an appropriate `client_max_body_size` to accept files
  - Implementation considerations:
    - Check if the directory exists and is writable (permissions check)
    - Handle filename conflicts (overwriting or renaming)
    - Clean up temporary uploads in case of errors
    - Validate file types (if necessary)
  - The server has to handle uploads with the `Content-Type: multipart/form-data` or `application/octet-stream`
  - **Implementation details**:
    - For requests with `POST` using `multipart/form-data`, boundary parsing is necessary
    - Extract the filename from the `Content-Disposition:` header
    - Write the received data to the destination file
    - Return 201 (Created) on success, with a `Location:` header pointing to the created resource

### Complete example of the different location configurations

```nginxconf
server {
    listen 8080;
    server_name example.com;
    root /var/www/html;
    index index.html;
    client_max_body_size 10M;

    # Default route, static website
    location / {
        allow_methods GET;
        autoindex off;
    }

    # API with several methods
    location /api {
        allow_methods GET POST DELETE;
        root /var/www/api;
        index api.html;
    }

    # Download route with autoindex listing
    location /downloads {
        allow_methods GET;
        root /var/www;
        autoindex on;
    }

    # Upload route
    location /upload {
        allow_methods POST;
        upload_to /var/www/uploads;
        client_max_body_size 50M;
    }

    # Redirection
    location /old-page {
        return 301 /new-page;
    }

    # Example with alias
    location /docs {
        alias /usr/share/documentation;
        allow_methods GET;
        autoindex on;
    }
}
```

## Dump configuration, with every directive handled by our webserv and quick explanation of behaviors

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
