# TODO (ordered ?)

- [x] creer un fichier default.conf avec toutes les spec que nous gerrerons (des directives seront rajoutées/supprimées au fur et a mesure du temps)
- [x] creer un dossier avec differents fichiers de config qui fonctionne/fonctionne pas pour une potentielle github action
- [x] trouver une structure cool
- [x] creer les dossiers/fichiers de la structure
- [x] github actions config file + siege (plus tard)?


[CONFIGURATION FILE STRUCTURE](https://nginx.org/en/docs/beginners_guide.html#conf_structure) (useful to create accepted and rejected configuration files)

[another link, looks a bit more detailed](https://devdocs.io/nginx/beginners_guide#conf_structure)




Si t'as un peu de temps tu peux me dire ce qui te convient le plus stp (dans l'idée je me pose ces questions dans le but de savoir a quel point on décide de faire un parsing compliqué):
dans nos fichier de config il y a plusieurs possibilités:
https://github.com/AzehLM/webserv/blob/main/docs/configuration-file.md#exemple-complet-de-configuration-location
soit on accepte que des context server (context = block) comme sur l'exemple au dessus.
- avec du coup toutes les directives obligatoirement a l'intérieur et on a pas
