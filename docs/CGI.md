# CGI (Common Gateway Interface)

### Qu'est-ce qu'un CGI ?

- **CGI (Common Gateway Interface)**: Interface standard permettant à un serveur web d'exécuter des programmes externes pour générer du contenu dynamique
  - Définit comment le serveur communique avec des scripts/programmes externes
  - Permet d'exécuter du code dans n'importe quel langage (PHP, Python, etc.)
  - Le programme CGI génère une réponse HTTP que le serveur renvoie au client

- **Fonctionnement général**:
```
  1. Client envoie requête → Serveur Web
  2. Serveur identifie que c'est un CGI (extension .php, .py, etc.)
  3. Serveur lance le programme CGI dans un nouveau processus (fork + execve)
  4. Serveur passe les données de la requête via variables d'environnement et stdin
  5. Programme CGI traite la requête
  6. Programme CGI écrit la réponse sur stdout
  7. Serveur lit stdout du CGI
  8. Serveur renvoie la réponse au client
```

### cgi_path

- `cgi_path`: Définit le chemin vers l'interpréteur CGI à utiliser pour exécuter les scripts
  - Syntaxe: `cgi_path /path/to/interpreter;`
  - Exemples:
    - `cgi_path /usr/bin/php-cgi;` - Pour PHP
    - `cgi_path /usr/bin/python3;` - Pour Python
    - `cgi_path /usr/bin/perl;` - Pour Perl

- **Important pour le sujet**: "Your server should support at least one CGI (php-CGI, Python, and so forth)"

- Cette directive indique au serveur:
  - Quel programme lancer pour exécuter le script
  - Le chemin doit être absolu et pointer vers un exécutable valide

### cgi_ext (extension matching)

- `cgi_ext`: Définit l'extension de fichier qui déclenche l'exécution via CGI
  - Syntaxe: `cgi_ext .extension;`
  - Exemples:
    - `cgi_ext .php;` - Tous les fichiers .php seront traités comme CGI
    - `cgi_ext .py;` - Tous les fichiers .py seront traités comme CGI
    - `cgi_ext .cgi;` - Extension générique pour scripts CGI

- **Fonctionnement**:
```nginxconf
  location /scripts {
      cgi_path /usr/bin/python3;
      cgi_ext .py;
      root /var/www;
  }
  # Requête: GET /scripts/hello.py
  # → Exécute: /usr/bin/python3 /var/www/scripts/hello.py

  # Requête: GET /scripts/style.css
  # → Sert normalement (pas d'extension .py)
```

- Le serveur compare l'extension du fichier demandé avec `cgi_ext`
- Si match: traitement CGI
- Sinon: traitement comme fichier statique normal

### Variables d'environnement CGI (environnement variables)

- **Variables d'environnement**: Le serveur doit passer les informations de la requête au CGI via des variables d'environnement

- **Variables CGI standard (obligatoires selon RFC 3875)**:

  | Variable | Description | Exemple |
  |----------|-------------|---------|
  | `REQUEST_METHOD` | Méthode HTTP | `GET`, `POST`, `DELETE` |
  | `QUERY_STRING` | Paramètres après ? dans l'URL | `id=123&name=test` |
  | `CONTENT_TYPE` | Type MIME du body | `application/x-www-form-urlencoded` |
  | `CONTENT_LENGTH` | Taille du body en octets | `1234` |
  | `SCRIPT_NAME` | Chemin URI du script | `/cgi-bin/script.py` |
  | `SCRIPT_FILENAME` | Chemin absolu du script | `/var/www/cgi-bin/script.py` |
  | `PATH_INFO` | Info de chemin supplémentaire | `/extra/path` |
  | `PATH_TRANSLATED` | PATH_INFO traduit en chemin filesystem | `/var/www/extra/path` |
  | `SERVER_NAME` | Nom du serveur | `example.com` |
  | `SERVER_PORT` | Port du serveur | `8080` |
  | `SERVER_PROTOCOL` | Protocole HTTP | `HTTP/1.1` |
  | `SERVER_SOFTWARE` | Nom du serveur | `webserv/1.0` |
  | `GATEWAY_INTERFACE` | Version CGI | `CGI/1.1` |
  | `REMOTE_ADDR` | IP du client | `192.168.1.10` |
  | `REMOTE_HOST` | Hostname du client (si résolu) | `client.example.com` |
  | `AUTH_TYPE` | Type d'authentification | `Basic` |
  | `REMOTE_USER` | Utilisateur authentifié | `john` |

***Il y en a plusieurs qui ne sont pas réellement obligatoire pour webserv, j'enlèverai plus tard celles qu'on décide de ne pas exporter***

- **Variables HTTP headers**: Tous les headers HTTP doivent être passés avec le préfixe `HTTP_`
  - Header `User-Agent: Mozilla/5.0` → Variable `HTTP_USER_AGENT=Mozilla/5.0`
  - Header `Accept: text/html` → Variable `HTTP_ACCEPT=text/html`
  - Header `Cookie: session=abc123` → Variable `HTTP_COOKIE=session=abc123`
  - Les tirets `-` sont remplacés par underscores `_`
  - Tout est en majuscules

- **Exemple complet de variables d'environnement, avec celles que nous ne gérerons pas**:
```
  Requête:
  POST /cgi-bin/form.php?debug=1 HTTP/1.1
  Host: example.com
  Content-Type: application/x-www-form-urlencoded
  Content-Length: 27
  User-Agent: Mozilla/5.0

  name=John&email=john@example.com

  Variables d'environnement passées au CGI:
  REQUEST_METHOD=POST
  QUERY_STRING=debug=1
  CONTENT_TYPE=application/x-www-form-urlencoded
  CONTENT_LENGTH=27
  SCRIPT_NAME=/cgi-bin/form.php
  SCRIPT_FILENAME=/var/www/cgi-bin/form.php
  SERVER_NAME=example.com
  SERVER_PORT=80
  SERVER_PROTOCOL=HTTP/1.1
  GATEWAY_INTERFACE=CGI/1.1
  HTTP_HOST=example.com
  HTTP_USER_AGENT=Mozilla/5.0
```

- ⚠️*TODO* **Exemple complet de variables d'environnement pour webserv**:

### Passage des données au CGI

- **Note du sujet**: "The full request and arguments provided by the client must be available to the CGI"

- **Méthode GET**:
  - Arguments dans `QUERY_STRING`
  - Pas de body
  - Exemple: `/script.py?name=John&age=30` → `QUERY_STRING=name=John&age=30`

- **Méthode POST**:
  - Arguments dans le body de la requête
  - Passer le body via **stdin** du processus CGI
  - Le CGI lit depuis stdin jusqu'à `CONTENT_LENGTH` octets

- **Gestion du chunked transfer encoding (besoin de faire plus de recherches la dessus)**:
  - **Note du sujet**: "For chunked requests, your server needs to un-chunk them, the CGI will expect EOF as the end of the body"
  - Si la requête arrive en chunks (`Transfer-Encoding: chunked`):
    1. Le serveur doit d'abord décoder tous les chunks
    2. Reconstituer le body complet
    3. Passer le body complet au CGI via stdin
    4. Fermer stdin (envoyer EOF) pour signaler la fin
  - Le CGI ne doit jamais voir les chunks, seulement les données décodées

### Lecture de la réponse CGI

- **Format de la réponse CGI**: Le CGI écrit sur stdout une réponse qui peut être:
  1. **Document avec headers** (le plus courant):
```
     Content-Type: text/html

     <html>
     <body>Hello World</body>
     </html>
```

  2. **Redirection**:
```
     Status: 302 Found
     Location: http://example.com/new-page
```

  3. **Headers personnalisés**:
```
     Content-Type: application/json
     Cache-Control: no-cache
     X-Custom-Header: value

     {"result": "success"}
```

- **Traitement par le serveur**:
  1. Lire stdout du processus CGI
  2. Parser les headers (jusqu'à ligne vide `\r\n\r\n` ou `\n\n`)
  3. Si header `Status:` présent: utiliser ce code de statut
  4. Sinon: utiliser 200 OK par défaut
  5. Transmettre les headers du CGI au client
  6. Transmettre le body du CGI au client

- **Note du sujet**: "If no content_length is returned from the CGI, EOF will mark the end of the returned data"
  - Le serveur doit lire stdout jusqu'à EOF (fermeture du pipe)
  - Deux cas:
    1. CGI envoie `Content-Length:` → Le serveur sait combien d'octets lire
    2. CGI n'envoie pas `Content-Length:` → Lire jusqu'à EOF
  - Dans le cas 2, le serveur peut:
    - Utiliser `Transfer-Encoding: chunked` pour envoyer au client
    - Ou lire tout en mémoire puis envoyer avec Content-Length calculé

### Exemple de configuration CGI complète
```nginxconf
server {
    listen 8080;
    server_name localhost;
    root /var/www;

    # Scripts Python
    location /python {
        cgi_path /usr/bin/python3;
        cgi_ext .py;
        allow_methods GET POST;
    }

    # Scripts PHP
    location /php {
        cgi_path /usr/bin/php-cgi;
        cgi_ext .php;
        allow_methods GET POST;
    }

    # CGI génériques
    location /cgi-bin {
        cgi_path /usr/bin/python3;
        cgi_ext .cgi;
        allow_methods GET POST DELETE;
    }
}
```

### Points de vigilance pour le projet

1. **Sécurité**:
   - Valider le chemin du script (éviter directory traversal: `../../../etc/passwd`)
   - Ne jamais passer des données utilisateur non validées aux variables d'environnement
   - Limiter les CGI à des répertoires spécifiques

2. **Performance**:
   - Chaque requête CGI crée un nouveau processus (coûteux)
   - Pour le projet, c'est acceptable (pas de FastCGI requis)
   - Mais important de bien nettoyer les processus (waitpid, close pipes)

3. **Robustesse**:
   - Gérer les cas où le CGI ne se termine jamais (timeout)
   - Gérer les cas où le CGI produit trop de données (limiter la lecture)
   - Gérer les signaux (SIGPIPE si le client se déconnecte)

4. **Conformité au sujet**:
   - ✅ Supporter au moins un CGI (PHP ou Python)
   - ✅ Passer la requête complète et les arguments
   - ✅ Gérer les requêtes chunked (un-chunk avant de passer au CGI)
   - ✅ Gérer l'absence de Content-Length (lire jusqu'à EOF)
   - ✅ Exécuter dans le bon répertoire
   - ✅ Utiliser fork() uniquement pour CGI (interdit ailleurs)

### Ressources et RFC

- **RFC 3875**: The Common Gateway Interface (CGI) Version 1.1
  - Définit le standard CGI complet
  - Liste toutes les variables d'environnement obligatoires
  - Spécifie le format des réponses CGI

- **Différences CGI vs FastCGI**:
  - CGI: Nouveau processus à chaque requête (lent mais simple)
  - Pour webserv: CGI suffit



⚠️ **J'ai des notes sur le coté sur l'implémentation que je rajouterai plus tard.** Besoin de faire plus de recherches.
