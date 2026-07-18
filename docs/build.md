# Compiler le projet

## Linux ou WSL2

```bash
./scripts/setup.sh
./scripts/build.sh
```

La version par défaut vient de `buildroot.version`. Pour tester une autre révision 2025.02.x vérifiée :

```bash
./scripts/setup.sh --version 2025.02.16
```

Le script refuse d'écraser une installation existante sans confirmation. Il accepte `--force` pour les environnements automatisés explicitement contrôlés.

## Commandes manuelles

```bash
make -C buildroot BR2_EXTERNAL="$PWD/external" O="$PWD/output" linux_embarque_defconfig
make -C buildroot O="$PWD/output" BR2_DL_DIR="$PWD/dl"
```

Les journaux des scripts sont placés dans `logs/`; les images sont attendues dans `output/images/`.

## Sous-module optionnel

Après validation humaine de la version et de l'opération distante, la méthode alternative est :

```bash
git submodule add -b 2025.02.16 https://gitlab.com/buildroot.org/buildroot.git buildroot
```

Cette commande est fournie à titre de préparation et n'est pas exécutée automatiquement.

## Reconstruction et nettoyage

```bash
./scripts/rebuild.sh
./scripts/clean.sh light
./scripts/clean.sh buildroot
./scripts/clean.sh output
```

Les deux niveaux destructifs demandent confirmation. `dl/` est conservé.
