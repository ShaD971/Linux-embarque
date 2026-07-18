# Linux-embarque

[![Buildroot](https://img.shields.io/badge/Buildroot-2025.02.16-673ab7)](https://buildroot.org/)
[![Linux](https://img.shields.io/badge/Linux-6.6.144%20LTS-fcc624)](https://www.kernel.org/)

Modernisation prudente d'un projet pédagogique Linux embarqué de 2018. La cible documentée par les rapports est une **Raspberry Pi 3** (ARM 32 bits). La nouvelle configuration utilise un arbre `BR2_EXTERNAL`; l'ancien arbre Buildroot reste présent dans l'historique et dans les répertoires historiques à la racine.

> État : structure et configuration validées statiquement. Aucune réussite de compilation complète ni de démarrage sur carte réelle n'est revendiquée tant que les validations correspondantes n'ont pas été exécutées.

## Versions

- Buildroot cible : `2025.02.16` LTS, modifiable avec `--version` ou `-Version`.
- Linux par défaut : `6.6.144` LTS mainline.
- Linux alternatif documenté : `6.12.95` LTS, après validation matérielle.
- Cible : Raspberry Pi 3 Model B/B+ présumée, ARM Cortex-A53 exécuté en 32 bits.

## Prérequis

La compilation Buildroot doit être effectuée sous Linux. Sous Windows, utilisez WSL2 (Ubuntu recommandé), une VM Linux ou un conteneur Linux. Les outils Windows natifs servent uniquement à préparer et vérifier le dépôt.

Sous Ubuntu/WSL2 :

```bash
sudo apt update
sudo apt install -y build-essential bash bc binutils bzip2 cpio file gcc git gzip make patch perl python3 rsync sed tar unzip wget xz-utils
```

## Installation et compilation

```bash
./scripts/setup.sh
./scripts/build.sh
```

Équivalent manuel :

```bash
make -C buildroot BR2_EXTERNAL="$PWD/external" O="$PWD/output" linux_embarque_defconfig
make -C buildroot O="$PWD/output"
```

Sous PowerShell :

```powershell
.\scripts\verify.ps1
.\scripts\setup.ps1
.\scripts\build.ps1
```

`build.ps1` délègue la compilation à WSL2. Les images attendues seront placées dans `output/images/`, notamment `sdcard.img` si `genimage` termine avec succès.

## Configuration

```bash
make menuconfig
make linux-menuconfig
```

Le fichier `external/board/linux-embarque/linux.config` est un fragment mainline Raspberry Pi 3 appliqué au `multi_v7_defconfig`. Consultez [docs/hardware.md](docs/hardware.md) avant de modifier le noyau ou les Device Trees.

## Flash

Après avoir contrôlé le nom du périphérique et sauvegardé son contenu :

```bash
sudo ./scripts/flash.sh /dev/sdX
```

Le script exige une confirmation explicite. Une erreur de périphérique détruirait les données du disque choisi.

## Structure moderne

- `external/` : cible, overlay, paquet pédagogique et configuration `BR2_EXTERNAL`.
- `scripts/` : préparation, compilation, nettoyage, flash et vérification.
- `docs/` : audit, architecture, migration, sécurité et validation.
- `tests/smoke/` : contrôles rapides après démarrage.
- `buildroot/` : source téléchargée ou sous-module optionnel, non versionnée par défaut.
- `output/`, `dl/`, `logs/` : artefacts locaux non versionnés.

## Historique et limitations

Le commit initial contient Buildroot 2018.11; le dernier état historique à la racine annonce toutefois `2019.02-git`. Les rapports confirment une Raspberry Pi 3, mais ne donnent ni révision exacte de carte, ni inventaire complet des périphériques. Le paquet `helloworld` a été migré; aucune preuve n'établit que son chargement avait réussi sur le matériel original.

Voir [l'audit](docs/audit-legacy.md), [la migration](docs/migration.md), [le matériel](docs/hardware.md), [la compilation](docs/build.md), [la sécurité](docs/security.md) et [la validation](docs/validation-report.md).

## Sécurité

La configuration moderne n'active pas de mot de passe vide ni de service SSH par défaut. N'ajoutez jamais de secret, clé privée ou mot de passe en clair au dépôt.

## Licence

L'arbre Buildroot historique est distribué sous GPL-2.0-or-later (voir `COPYING`). Le code spécifique ajouté par cette modernisation suit la même licence, sauf mention contraire dans un fichier.
