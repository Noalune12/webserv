# CGI (Common Gateway Interface)

### Qu'est-ce qu'un CGI ?
### What is a CGI ?

- **CGI (Common Gateway Interface)**: An standardized interface allowing a webserver to execute programs to generate dynamic content.
  - Defines how the server communicates with external scripts/programs.
  - Allows execution of code in any languages (PHP, Python, etc...)
  - The CGI program generate an HTTP response that the server sends to the client

- **Global behavior**:
```
  1. Clients sends a request → Webserver
  2. Server identified it as a CGI (with the file extension)
  3. Server launches the CGI program in a new processus (fork + execve)
  4. Server gives the data of the request via environment variables via stdin
  5. CGI handles the request
  6. CGI program output is written to the stdout
  7. Server reads the stdout of the CGI
  8. Sends the read output of the CGI to the client as a response
```

### cgi_path

- `cgi_path`: Defines the path to the CGI interpretor to use for the script execution
- Syntax: `cgi_path /path/to/interpretor;`
  - Exemples:
    - `cgi_path /usr/bin/php-cgi;` - PHP
    - `cgi_path /usr/bin/python3;` - Python

- This directive tells the server:
  - Which program use to execute the script
  - The path that has to be absolute and point to a valid executable

### cgi_ext (extension matching)

- `cgi_ext`: Defines the CGI extension type
  - Syntaxe: `cgi_ext .extension;`
  - Exemples:
    - `cgi_ext .php;`
    - `cgi_ext .py;`

- **Functioning**:
```nginxconf
  location /scripts {
      cgi_path /usr/bin/python3;
      cgi_ext .py;
      root /var/www;
  }
  # Request: GET /scripts/hello.py
  # → Executes: /usr/bin/python3 /var/www/scripts/hello.py
```

- The server compares the file extension type to the `cgi_ext`
- If it matches, the CGI gets executed

### Environment variables

- The server has to give informations of the request to the CGI via environment variables

- **Standard variables for CGI (RFC 3875)**:

  | Variable | Description | Exemple |
  |----------|-------------|---------|
  | `REQUEST_METHOD` | HTTP Method | `GET`, `POST`, `DELETE` |
  | `QUERY_STRING` | Parameters after the first ? in a URL | `id=123&name=test` |
  | `CONTENT_TYPE` | MIME type of the body | `application/x-www-form-urlencoded` |
  | `CONTENT_LENGTH` | Body size in bytes | `1234` |
  | `SCRIPT_NAME` | URI Path to the script | `/cgi-bin/script.py` |
  | `SCRIPT_FILENAME` | Absolute path to the script | `/var/www/cgi-bin/script.py` |
  | `PATH_INFO` | Extra information about the path | `/extra/path` |
  | `PATH_TRANSLATED` | PATH_INFO translated in filesystem path | `/var/www/extra/path` |
  | `SERVER_NAME` | Server name | `example.com` |
  | `SERVER_PORT` | Server port | `8080` |
  | `SERVER_PROTOCOL` | HTTP Protocol | `HTTP/1.1` |
  | `SERVER_SOFTWARE` | Server given name | `webserv/1.0` |
  | `GATEWAY_INTERFACE` | CGI version | `CGI/1.1` |
  | `REMOTE_ADDR` | Client IP | `192.168.1.10` |
  | `REMOTE_HOST` | Client Hostname (if resolved) | `client.example.com` |

***Some of them are not required for webserv***

- **HTTP headers variables**: Every HTTP headers have to be given to the CGI with the prefix `HTTP_`

### Ressources et RFC

- [RFC](https://www.ietf.org/rfc/rfc3875.txt)
- [Unsing environment variables](https://www.oreilly.com/openbook/cgi/ch02_02.html)
- [IBM environment variables details](https://www.ibm.com/docs/fr/netcoolomnibus/8.1.0?topic=scripts-environment-variables-in-cgi-script)

- **RFC 3875**: The Common Gateway Interface (CGI) Version 1.1
  - Defines the complete CGI standard
  - List all the mandatory environment variables
  - Speficy the CGI response format
