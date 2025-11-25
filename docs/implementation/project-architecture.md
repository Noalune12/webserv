# Façade / Singleton

## Façade

Patron de conception structurel qui procure une interface offrant un accès simplifié à un ensemble complexe de classes (dans notre contexte).

Une façade est une classe qui procure une interface simple vers un sous-système complexe de **parties mobiles**. Le but est de limiter les interactions possibles avec la façade, car ce sont les sous-systèmes qui possèdent ces fonctionnalités.

Par exemple, dans Webserv, on pourrait n'exposer qu'une méthode parseConfig(chemin) au lieu d'interagir avec le lexer, le parser et les multiples objets de configuration. Cette façade lit le fichier, valide les directives et retourne une structure prête à l'emploi pour lancer les sockets et routes, sans révéler la complexité interne.

```cpp
// Au lieu de faire:
Lexer lexer("/path/to/webserv.conf");
Parser parser(lexer.tokenize());
Config config = parser.parse();
SocketManager sm(config);

// On fait simplement:
Server server("/path/to/webserv.conf");
server.start();
```

- Une façade procure un accès pratique aux différentes parties des fonctionnalités du sous-système.
- Les classes du sous-système ne sont pas conscientes de l'existence de la façade. **Elle opèrent et interagissent directement à l'intérieur de leur propre système**.

- Le but est d'encapsuler les fonctionnalités externes et de les cacher du reste du code.
- On se contentera de modifier l'implémentation des méthodes de la façade.

#### Possibilités d'application

- Utiliser une façade si besoin d'une interface limitée mais directe à un sous-système complexe.
- **Utiliser une façade si besoin de structurer un sous-système en plusieurs couches (notre utilisation)**

```cpp
// La façade Server masque toute la complexité interne:
class Server {
  private:
      ConfigParser _parser;      // Sous-système parsing
      SocketManager _sockets;    // Sous-système réseau
      RequestHandler _handler;   // Sous-système HTTP
      CGIExecutor _cgi;         // Sous-système CGI
  public:
      void start();  // Interface simple pour l'utilisateur
};
```

#### Mise en oeuvre

- Déclarer et implétementer une interface en tant que façade regidigeant les appels du code aux sous-objects appropriés. Elle est également responsable de l'initialisation des sous-systèmes et de gérer leurs cycles de vie.
- Obliger la communication aux sous-systèmes en passant par la façade.
```cpp
Server::Server(const std::string& configPath) {
    // La façade orchestre l'initialisation de tous les sous-systèmes
    Config config = _parser.parse(configPath);
    ...
}
```

## Singleton

Le singleton, par définition **garantit l'unicité d'une instance pour une classe**. Il fournit également un point d'accès global à notre instance.

Sa mise en place est très simple, notre classe façade, qui est un singleton aura un constructeur par **défaut privé** avec une méthode de création statique qui se comportera comme un constructeur.
```cpp
class Server {
  private:
      Server(const std::string& configPath);  // Constructeur privé
      Server(const Server&);                  // Interdit la copie
      Server& operator=(const Server&);       // Interdit l'assignation

  public:
      static Server& getInstance(const std::string& configPath);
};
```

Si notre code a accès à la classe du singleton, alors il pourra appeler sa méthode statique qui à chaque appel retournera toujours le même objet.
```cpp
// Dans main.cpp:
Server& srv = Server::getInstance("/path/to/webserv.conf");
srv.start();

// Ailleurs dans le code, on récupère la même instance:
Server& srv2 = Server::getInstance();  // Même objet que srv
```

⚠️ **Les deux vont souvent ensemble, maintenant je pense pas que ce soit totalement nécessaire vu qu'on construit un projet fini. On peut garder l'idée de coté mais j'ai un doute sur la nécessité de mettre ça en place**

et si jamais: [doc singleton (FR)](https://refactoring.guru/fr/design-patterns/singleton)
