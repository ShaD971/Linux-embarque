# Architecture moderne

La modernisation sépare les données du produit du moteur Buildroot :

```text
Buildroot 2025.02.x (buildroot/, non versionné)
        + BR2_EXTERNAL=external/
        + external/configs/linux_embarque_defconfig
        + external/board/linux-embarque/
        + external/package/helloworld/
        -> output/ et logs/ (non versionnés)
```

`external/external.desc` fournit le nom stable `LINUX_EMBARQUE`. `Config.in` expose les packages personnalisés et `external.mk` charge leurs fichiers `.mk`. Le defconfig contient uniquement les choix propres au produit ; le fragment `linux.config` complète le `bcm2711_defconfig` 64 bits du noyau Raspberry Pi.

L'arbre Buildroot historique à la racine est une source documentaire. La variable `BUILDROOT_DIR` permet de pointer les scripts vers une autre copie sans toucher à cet historique.

## Reproductibilité

- Version Buildroot centralisée dans `buildroot.version`.
- Version noyau explicitement figée dans le defconfig.
- Sources téléchargées regroupées dans `dl/` et mises en cache en CI.
- `SOURCE_DATE_EPOCH` fixé en CI à l'époque historique Buildroot.
- Paquet étudiant fourni localement, sans URL mutable.
- Defconfig minimal appliqué avant chaque compilation automatisée.

La reproductibilité binaire complète dépend encore des sources distantes, de leurs hashes Buildroot, de l'image de runner et de la toolchain. Les sorties de `legal-info` doivent être archivées pour une publication.
