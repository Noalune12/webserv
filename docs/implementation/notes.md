### Validator checks

Tout ce qui est apres un ; est une erreur de syntaxe.

Si directives dupliquées:
- Certaines directives acceptent plusieurs parametres, alors les traitent ensemble
- Les autres directives s'overrident, on prend en compte seulement la derniere occurence des duplicatas



### Idée de construction pour faire le check de validation:

- Check du nom de la key
- Si le nom est bon, on check la syntax des values
  - d'abbord formatage (semi-colon a la fin de la ligne et rien apres) **check global et indépendant de la directive** ⚠️ prendre en compte que les blocks de contexts n'ont pas le meme format (a voir plus tard)
  - tableau de pointeurs sur fonctions pour check la validité des parametres de chaque directive

*commencer a prendre en compte l'héritage ?*


### error_log file

- Lancer un container docker nginx et check chaque message d'erreur avec `nginx -t`
- creer une fonction globale qui ecrit dans le fichier de log
  - check des droits du fichier,
- work in nginx docker container and check each error messages with `nginx -t`
- create
