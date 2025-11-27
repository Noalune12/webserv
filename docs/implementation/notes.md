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
