# Version historique

Le projet d'origine repose sur Buildroot 2018.11 au commit initial `8b52c412`. Un commit ultérieur a actualisé l'arbre vers un snapshot `2019.02-git`; cette nuance est documentée dans l'audit.

La version originale est conservée intégralement dans l'historique Git. La modernisation ne supprime pas les rapports, captures, sources ou configurations historiques. Les nouveaux développements sont isolés dans `external/`, `scripts/` et la branche locale `modernization/buildroot-2025.02`.

Pour consulter exactement la première version sans modifier le travail courant :

```bash
git show 8b52c412:Makefile
git diff 8b52c412..9f86504c
```

Aucune commande de restauration destructive n'est nécessaire.
