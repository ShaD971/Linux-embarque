# Migration Buildroot

## Migration Raspberry Pi 3 vers Raspberry Pi 4 8 Go

| Ancienne configuration | Configuration Pi 4 8 Go | Justification |
|---|---|---|
| `BR2_arm=y`, Cortex-A53 | `BR2_aarch64=y`, Cortex-A72 | Exécuter en 64 bits et adresser les 8 Go de RAM. |
| Noyau mainline `multi_v7` | Noyau Raspberry Pi `bcm2711` | Reprendre le BSP de référence Buildroot Pi 4 64 bits. |
| Version textuelle `6.6.144` | Tarball figé au commit `576cc10e…` | Utiliser exactement le noyau validé par Buildroot 2025.02.16. |
| `bcm2837-rpi-3-b*.dtb` | `bcm2711-rpi-4-b.dtb` | Décrire le SoC BCM2711 et le matériel du Model B. |
| `zImage` | `Image` | Format de noyau AArch64 attendu par le firmware. |
| `bootcode.bin`, `start.elf`, `fixup.dat` | `start4.elf`, `fixup4.dat` | Le Pi 4 démarre son premier étage depuis l'EEPROM SPI. |
| Firmware générique Pi 0/1/2/3 | `BR2_PACKAGE_RPI_FIRMWARE_VARIANT_PI4=y` | Installer les binaires GPU propres au Pi 4. |
| Wi-Fi sans firmware garanti | `brcmfmac-sdio-firmware-rpi` BCM43455 | Permettre l'utilisation du Wi-Fi intégré. |
| Rootfs ext4 192 Mio | Rootfs ext4 256 Mio | Ajouter une marge pour AArch64, le firmware et les outils Wi-Fi. |

## Éléments conservés

- Buildroot 2025.02.16 et l'arbre `BR2_EXTERNAL`.
- glibc, BusyBox, DHCP Ethernet et le paquet local `helloworld`.
- partition racine ext4, partition de boot FAT et génération de `sdcard.img`.
- root désactivé par défaut et aucun secret réseau versionné.
- console HDMI et console série 3,3 V.

## Choix du noyau

Le commit Raspberry Pi est celui du `raspberrypi4_64_defconfig` fourni par Buildroot 2025.02.16, avec des headers 6.6. Le chemin de hashes officiel `board/raspberrypi/patches` est conservé et `BR2_DOWNLOAD_FORCE_CHECK_HASHES=y` empêche l'utilisation silencieuse d'une archive différente.

Toute mise à jour du noyau doit modifier ensemble le commit, le hash Buildroot, la version des headers et éventuellement le fragment/DTB. Elle doit être suivie d'une reconstruction complète et d'un test sur la carte 8 Go.
