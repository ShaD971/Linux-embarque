# Audit de la version historique

Date de l'audit : 16 juillet 2026. Branche source : `master`, commit `9f86504c`. État initial : propre (`master...origin/master`).

## 1. Résumé du dépôt

Le dépôt est un import presque complet de Buildroot accompagné de livrables pédagogiques. Il contient 10 807 fichiers hors `.git`, les répertoires Buildroot classiques (`arch`, `board`, `boot`, `configs`, `fs`, `linux`, `package`, `system`, `toolchain`, etc.), trois PDF de rapport, un DOCX et cinq captures d'écran. Aucun `output/`, archive de téléchargement ou `.config` de construction n'est suivi.

Les rapports décrivent un exercice consistant à ajouter un programme `helloworld`, générer une image Buildroot et la démarrer sur Raspberry Pi 3. Le démarrage de Buildroot est illustré, mais le rapport indique que la présence du module n'a pas pu être vérifiée après démarrage.

## 2. Buildroot actuel

- Commit initial `8b52c412` : `BR2_VERSION := 2018.11`.
- État final cloné : `BR2_VERSION := 2019.02-git` dans le `Makefile` historique.
- `CHANGES` commence par la publication 2018.11 du 1er décembre 2018.
- Le commit `53279cd6` a remplacé/actualisé de très nombreux fichiers et changé en masse leurs modes. L'appellation « projet Buildroot 2018.11 » est donc historiquement correcte, mais l'état exact en tête n'est pas la release 2018.11 pure.
- Aucun mécanisme `BR2_EXTERNAL` propre au projet n'existait.

## 3. Noyau actuel

Deux profils sont pertinents :

- `configs/raspberrypi_defconfig`, cité par les rapports : noyau Raspberry Pi issu d'un commit tarball, série 4.14, `bcmrpi`, DTB BCM2708 ; ce profil cible plutôt les premiers Raspberry Pi et ne correspond pas précisément au Pi 3.
- `configs/raspberrypi3_defconfig`, présent dans le dépôt : ARM Cortex-A53 32 bits, noyau Raspberry Pi série 4.14, `bcm2709`, DTB BCM2710 et image `zImage`.

Aucun `.config` de noyau propre au projet n'est suivi. La configuration exacte utilisée lors de la démonstration ne peut donc pas être reproduite bit à bit.

## 4. Architecture cible

Architecture démontrée : ARM 32 bits. Pour le profil Pi 3 : Cortex-A53 avec NEON/VFPv4. Le processeur de la Raspberry Pi 3 est capable d'ARMv8 64 bits, mais le rapport et la commande historique indiquent une construction 32 bits.

## 5. Carte cible présumée

Les trois PDF citent explicitement une **Raspberry Pi 3** et une carte SD. La révision exacte (Model B ou B+), la quantité de RAM et les périphériques raccordés ne sont pas documentés. Le profil moderne couvre donc B et B+ et marque les périphériques optionnels « à confirmer ».

## 6. Packages activés

Le defconfig historique Pi 3 active explicitement le firmware Raspberry Pi, les outils hôte `dosfstools`, `genimage`, `mtools`, une toolchain C++ et un rootfs ext4. Les valeurs par défaut Buildroot ajoutent notamment BusyBox et l'init BusyBox.

Le paquet personnalisé `BR2_PACKAGE_HELLOWORLD` existe et le rapport décrit son activation via `menuconfig`, mais aucun `.config` sauvegardé ne prouve qu'il était activé dans la dernière compilation. Son URL `http://helloworld.com/dl/helloworld-1.1.tar.gz` est une URL d'exemple non reproductible.

## 7. Éléments personnalisés

- `package/helloworld/Config.in` et `helloworld.mk`.
- Inclusion de ce paquet dans `package/Config.in`.
- Rapports `FINAL_RAPPORT*.pdf`, document `Linux-Embarquée.docx` et captures pédagogiques.
- Duplicats/variantes `Makefile2` et `Makefile.legacy`.
- Aucune source C du paquet, aucun hash, aucun patch spécifique Pi 3, aucun overlay rootfs étudiant et aucun script post-build étudiant distinct n'ont été identifiés.

Les nombreux patchs sous `board/` et `package/` appartiennent à la distribution Buildroot importée ; rien ne permet de les attribuer au projet étudiant.

## 8. Risques de migration

- Mauvais profil historique (`raspberrypi_defconfig`) malgré une cible Pi 3.
- Écart important entre noyau constructeur 4.14 et noyau mainline 6.6 : noms de DTB, firmware, console Bluetooth/UART et pilotes Wi-Fi.
- Absence du `.config` réellement compilé et de la source `helloworld` originale.
- Absence de hash et URL du paquet historique invalide.
- Aucun test matériel reproductible, schéma de câblage ou inventaire des périphériques.
- Passage d'un arbre Buildroot modifié en place à un `BR2_EXTERNAL` sans dépendre des anciens chemins `board/raspberrypi`.
- Modes exécutables modifiés en masse dans le commit d'actualisation.

## 9. Fichiers obsolètes ou mal placés

- L'arbre Buildroot historique complet à la racine ne doit plus recevoir les évolutions du produit moderne.
- `Makefile2` et `Makefile.legacy` sont ambigus.
- Le paquet `package/helloworld` historique est incomplet et non téléchargeable.
- Les rapports et captures à la racine seraient mieux classés dans `docs/archive/`, mais ils ne sont pas déplacés automatiquement afin de préserver leur traçabilité.
- Les manuels/images Buildroot vendoriés et les nombreux fichiers à mode exécutable augmentent inutilement le dépôt moderne.

## 10. Éléments à conserver

- Tous les commits historiques et les rapports pédagogiques.
- Les configurations Raspberry Pi de référence et les scripts d'image, comme sources d'audit uniquement.
- L'intention fonctionnelle du paquet `helloworld`.
- Les preuves de démarrage, captures et limitations consignées dans les rapports.

## 11. Informations impossibles à déterminer

- Révision exacte Raspberry Pi 3, taille de RAM et numéro de carte.
- Périphériques GPIO/I2C/SPI/USB réellement nécessaires.
- Usage réel du Wi-Fi/Bluetooth et modèle de puce.
- Contenu exact du `.config` Buildroot et du `.config` noyau utilisés.
- Version exacte du noyau issue du hash Raspberry Pi sans résoudre le dépôt externe.
- Source originale et licence du programme étudiant `helloworld`.
- Succès réel du module `helloworld` après démarrage ; le dernier rapport indique le contraire.

## 12. Recommandations

1. Conserver l'historique et développer sur `modernization/buildroot-2025.02`.
2. Utiliser Buildroot 2025.02.x LTS hors de Git et un `BR2_EXTERNAL` versionné.
3. Cibler Raspberry Pi 3 ARM 32 bits, Linux 6.6 LTS mainline, avec DTB B/B+.
4. Garder Linux 6.12 comme option documentée tant qu'un essai matériel ne l'a pas validé.
5. Remplacer le paquet exemple par une source locale minimale, licenciée et reproductible.
6. Désactiver la connexion root par défaut et ne pas activer SSH avant définition d'une politique d'identifiants.
7. Valider d'abord le defconfig sous Linux/WSL2, puis compiler et tester `sdcard.img` sur une carte sauvegardée.
