### Validator checks

Tout ce qui est apres un ; est une erreur de syntaxe.

Si directives dupliquées:
- Certaines directives acceptent plusieurs parametres, alors les traiter ensemble
- Les autres directives s'overrident, on prend en compte seulement la derniere occurence des duplicatas



### Idée de construction pour faire le check de validation:

- Check du nom de la key
- Si le nom est bon, on check la syntax des values
  - d'abbord formatage (semi-colon a la fin de la ligne et rien apres) **check global et indépendant de la directive** ⚠️ prendre en compte que les blocks de contexts n'ont pas le meme format (a voir plus tard)
  - tableau de pointeurs sur fonctions pour check la validité des parametres de chaque directive

*commencer a prendre en compte l'héritage ?*



## ⚠️ TODO

- TAKE DECISION ON WHICH ERROR MESSAGES FOR DIFFERENT SYNTAX ERROR, nginx has a weird behavior that is not explained in the documentation, the only way to understand which message is written for which case is by reading the source code and I CBA doing that for a feature that is not required, I'll mimic as much as possible nginx behavior but by simplifying as much as I can. I don't want to spend a week on that...

List of error strings we will handle:

- "unknown directive" -> directive we do not accept
- "unexpected end of file, expecting '}'" -> in case of missing } somewhere
- "unexpected '}'" -> if duplicate (or more) }} somewhere
- "unexpected '{'" -> if duplicate (or more) {{ somewhere
- "unexpected ';'" -> if duplicate (or more) ;; somewhere
- "invalid number of arguments" -> if some directive have more than one arg, it prevails over the syntax of the argument
- "directive %s is not terminated by ';'" -> if missing ; at the end of a directive
- "directive is duplicate" -> if directive cannot be duplicated



- Validation order:
  - keyNameCheck (global)
  - semiColonCheck (global for directives)
  - bracketCheck (global for contexts)
  -



bracketCheck idea:
- if the whole context is in a class and the class data contains the first to the last line of the context, including the closing bracket, then I can check the **last** element of the **first** line and the **first** element of the **last** line





### Architecture de Context

- _name: la premiere ligne du context. ex: `std::string _name = "server {"` | `std::string _name = "location / {"`
- vecteur de pair des directives
- normalement a la fin la bracket fermant dans le premier element de la pair egalement



### error_page

- Need to check there is at least one code error (404)
  - If this is not the case: `invalid number of arguments`
- Need to check the path of the page exists, is accessible
  - if no path `invalid number of argument`
  - ⚠️ the path is relative to the `root` directive as well so I think we should check it later when its required to access the error page, if the path do not exists then we redirect to our how error_page
- need to check the value of the codes:
  - nginx: [emerg] value "299" must be between 300 and 599 in /etc/nginx/conf.d/default.conf:2
  - nginx: [emerg] invalid value "abc" in /etc/nginx/conf.d/default.conf:2
  - nginx: [emerg] invalid value "499" in /etc/nginx/conf.d/default.conf:2 -> for code we wont handle
- ⚠️ path has to start with a `/` since its path is related to `root`


LIST OF ERROR_CODE HANDLED BY US: (won't handle all of them)
- 301 Moved Permanently
- 302 Found
- 303 See Other
- 307 Temporary Redirect
- 308 Permanent Redirect
- 400 Bad Request
- 403 Forbidden
- 404 Not Found
- 405 Not Allowed
- 408 Request Time-out
- 429 Too Many Requests (maybe)
- 500 Internal Server Error
- 505 HTTP Version Not Supported


- ⚠️ TODO:
  - pour check les code d'erreur
    - je vais devoir identifier si il y un vecteur avec " ", si jamais il y a des duplicatas
    - ensuite je check les nombres jusqu'a `v.end() - 1` pour supprimer le check du path des codes d'erreurs
    - une fois que j'ai ce "sous-vecteur", il faut que je check le nombre de parametres (path compris) et qu'il soit >= 2
    - maintenant je peux check les regles de chaques sous-vecteurs


### client_max_body_size

- nginx: [emerg] invalid number of arguments in "client_max_body_size" directive in /etc/nginx/conf.d/default.conf:1 -> client_max_body_size 10 m;
- nginx: [emerg] "client_max_body_size" directive invalid value in /etc/nginx/conf.d/default.conf:1 client_max_body_size 10AS;
- nginx: [emerg] "client_max_body_size" directive invalid value in /etc/nginx/conf.d/default.conf:1 -> client_max_body_size S;
- nginx: [emerg] "client_max_body_size" directive invalid value in /etc/nginx/conf.d/default.conf:1 -> client_max_body_size -10m;
- nginx: [emerg] invalid number of arguments in "client_max_body_size" directive in /etc/nginx/conf.d/default.conf:1 -> client_max_body_size;
- nginx: [emerg] "client_max_body_size" directive is duplicate in /etc/nginx/conf.d/default.conf:2 -> if duplicate so if we have "client_max_body_size", "10m;", " ", "20ms;" -> this have the priority over the other rules tho



### server

- Not a lot of error possible
- invalid number of argument if something between server and {
- unknow directive if something after {
  - or nginx: [emerg] unexpected "{" in /etc/nginx/conf.d/default.conf:1
  - or nginx: [emerg] unexpected ";" in /etc/nginx/conf.d/default.conf:1




### CONTEXT:

- Mettre les fonctions de validation dans la classe Validator.
- Normalement je dois juste itérer sur _directives, avec un check en amont de _name




### listen, server_name et virtual hosting

- Dans l'objectif de gerer le virtual hosting (avec plusieurs block server qui ecoute sur le meme port (meme directive listen, avec un server_name different) il nous faudra des verifications de listen au parsing mais aussi au runtime (lorsqu'on utilisera `bind`))

Exemple
```nginxconf

server {
  listen 80;
  server_name premier;
  root /a;
}

server {
  listen 80;
  server_name second;
  root /b;
}
```
> Cette conf devrait fonctionner, on a deux blocks servers qui `listen` sur le meme port mais n'ont pas le meme `server_name` donc creeent une instance de virtual hosting.
> On pourrait avoir le meme `root` par contre, avec des index different ou d'autres sous directives qui change le comportement du server.


##### Listen

- Quite large potential here, the directive doens't have to exists I believe, this is why it has a default value
All of those cases have to be managed:
```
listen 127.0.0.1:8000;
listen 127.0.0.1;
listen 8000;
listen *:8000;
listen localhost:8000;
```

Meaning:
- the checks has to be separated:
  - a first syntax check
    - if there a `:` to separate address and port
      - if yes: parse address then port
        - ADDRESS:
          -
      - if no: check if the value is in the range 1-65xxx
    -

- nginx: [emerg] host not found in "0.0.2.0.1" of the "listen" directive in /etc/nginx/conf.d/default.conf:4
- nginx: [emerg] host not found in "0.0.2" of the "listen" directive in /etc/nginx/conf.d/default.conf:4


Refuses:
- 0.0.0 -> if not 4 numbers not good
- if not separated by "." not good
- 0.0.02.0 -> cannot start by 0 if not 0!
- everything has to be in range 0-255 (check for leftover aswell)


TODO:
- chemin jusqu'a l'appel de validateLocation
- finir de structurer validateLocation
- coder validateLocation!



Files to check:

- empty-path.conf -> no error from nginx
- multiple-codes-no-path.conf -> no error from nginx
- circular-return.conf -> no error from nginx


BEFORE REFACTOR:

Total tests: 178
Passed: 173
Warnings: 5
Failed: 0
