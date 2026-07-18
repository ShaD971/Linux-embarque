# Migration Buildroot

## Matrice de configuration

| Ancienne option | Nouvelle option | État | Justification | Impact |
|---|---|---|---|---|
| `BR2_arm=y` | `BR2_arm=y` | conservée | Le rapport et l'image historique utilisent ARM 32 bits. | Compatibilité avec le parcours historique. |
| `BR2_arm1176jzf_s=y` (profil cité) | `BR2_cortex_a53=y` | remplacée | La cible documentée est un Raspberry Pi 3, pas un Pi de première génération. | Code généré pour Cortex-A53. |
| `BR2_ARM_EABIHF=y` | Sélection automatique moderne | supprimée | Symbole historique devenu inutile avec la sélection actuelle. | ABI déterminée par Buildroot. |
| Headers 4.14 | `BR2_PACKAGE_HOST_LINUX_HEADERS_CUSTOM_6_6=y` | remplacée | Alignement sur Linux 6.6 LTS. | Reconstruction complète de la toolchain. |
| Tarball Raspberry Pi par hash | `BR2_LINUX_KERNEL_CUSTOM_VERSION_VALUE="6.6.144"` | remplacée | Noyau mainline LTS explicite et auditable. | Certains pilotes constructeur peuvent manquer. |
| `bcmrpi` / `bcm2709` | `multi_v7` + fragment BCM2837 | remplacée | Configuration mainline commune ARMv7. | Fragment matériel à maintenir. |
| DTB `bcm2710-*` | `broadcom/bcm2837-rpi-3-b*` | remplacée | Noms mainline Linux 6.6. | Scripts d'image adaptés. |
| `BR2_PACKAGE_RPI_FIRMWARE=y` | même option | conservée | Firmware de démarrage Raspberry Pi nécessaire. | Binaire propriétaire toujours requis. |
| Overlay `pi3-miniuart-bt` | `enable_uart=1`, console PL011/8250 | à confirmer | Le mapping UART/Bluetooth dépend du firmware et de la révision. | Console série à tester physiquement. |
| `BR2_SYSTEM_DHCP="eth0"` | même intention | conservée | Ethernet utile pour le test initial. | Le nom de l'interface doit être vérifié au boot. |
| Rootfs ext4 120M | Rootfs ext4 192M | remplacée | Marge pour noyau 6.6 et modules. | Image SD plus grande. |
| `BR2_PACKAGE_HELLOWORLD` distant | package local `helloworld` | remplacée | URL historique fictive et source absente. | Compilation hors réseau du paquet. |
| C++ Buildroot | non activé | supprimée | Aucun code conservé ne nécessite C++. | Toolchain plus petite. |
| Connexion root implicite | root login désactivé | remplacée | Évite un accès réseau/console sans politique de mot de passe. | Provisionnement requis pour une image administrable. |
| Wi-Fi/Bluetooth constructeur | `brcmfmac` en module, firmware à confirmer | à confirmer | Rapports insuffisants. | Wi-Fi non garanti lors du premier boot. |
| U-Boot | aucun U-Boot | conservée | Le boot historique repose sur le firmware Raspberry Pi. | Pas de migration de bootloader U-Boot. |

## Choix de version noyau

Le profil versionné sélectionne 6.6.144. Pour 6.12 LTS, mettre à jour conjointement la valeur de version, le symbole de headers (`..._6_12`) et valider le fragment/DTB avec `make linux-update-defconfig`. Une version personnalisée peut être fournie via `menuconfig`, puis enregistrée avec `make savedefconfig`; elle ne doit pas être publiée sans tests matériels.
