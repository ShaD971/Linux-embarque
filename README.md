# Linux-embarque

[![Buildroot](https://img.shields.io/badge/Buildroot-2025.02.16-673ab7)](https://buildroot.org/)
[![Linux](https://img.shields.io/badge/Linux-Raspberry%20Pi%206.6%20LTS-fcc624)](https://github.com/raspberrypi/linux)

Modernisation d'un projet pédagogique Linux embarqué de 2018 pour une **Raspberry Pi 4 Model B avec 8 Go de RAM**. La configuration utilise un système 64 bits et un arbre `BR2_EXTERNAL`; l'ancien arbre Buildroot reste présent dans l'historique et dans les répertoires historiques à la racine.

> État : structure et configuration validées statiquement. Aucune réussite de compilation complète ni de démarrage sur carte réelle n'est revendiquée tant que les validations correspondantes n'ont pas été exécutées.

## Versions

- Buildroot cible : `2025.02.16` LTS, modifiable avec `--version` ou `-Version`.
- Linux : noyau Raspberry Pi 6.6 LTS figé au commit `576cc10e1ed50a9eacffc7a05c796051d7343ea4`, identique à la configuration Pi 4 64 bits de Buildroot 2025.02.16.
- Cible : Raspberry Pi 4 Model B 8 Go, SoC BCM2711/Cortex-A72, espace utilisateur AArch64.
- Boot : firmware Pi 4 `start4.elf`/`fixup4.dat`, noyau `Image` et DTB `bcm2711-rpi-4-b.dtb`.

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

La compilation utilise deux jobs par défaut pour limiter la pression mémoire. Vous pouvez ajuster cette valeur explicitement, par exemple `BUILD_JOBS=4 ./scripts/build.sh` sur une machine disposant de suffisamment de RAM.

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

Le fichier `external/board/linux-embarque/linux.config` complète le `bcm2711_defconfig` du noyau Raspberry Pi. Il active les fonctions propres au projet, notamment le Wi-Fi, I2C, SPI, le watchdog et le stockage USB. Consultez [docs/hardware.md](docs/hardware.md) avant de modifier le noyau ou les Device Trees.

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

Le commit initial contient Buildroot 2018.11; le dernier état historique à la racine annonce toutefois `2019.02-git`. Les rapports décrivent l'ancienne cible Raspberry Pi 3. La cible actuelle Pi 4 8 Go est une évolution explicite et utilise donc une chaîne de boot, une architecture et un Device Tree différents. Le paquet `helloworld` a été migré; aucune preuve n'établit que son chargement avait réussi sur le matériel original.

Voir [l'audit](docs/audit-legacy.md), [la migration](docs/migration.md), [le matériel](docs/hardware.md), [la compilation](docs/build.md), [la sécurité](docs/security.md) et [les smoke tests](tests/smoke/README.md).

## Sécurité

La configuration moderne n'active pas de mot de passe vide ni de service SSH par défaut. N'ajoutez jamais de secret, clé privée ou mot de passe en clair au dépôt.

## Licence

L'arbre Buildroot historique est distribué sous GPL-2.0-or-later (voir `COPYING`). Le code spécifique ajouté par cette modernisation suit la même licence, sauf mention contraire dans un fichier.
