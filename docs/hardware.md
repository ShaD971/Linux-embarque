# Matériel, Device Tree et démarrage

## Cible

La cible actuelle est une **Raspberry Pi 4 Model B avec 8 Go de RAM** :

- SoC Broadcom BCM2711, quatre Cortex-A72 ;
- noyau et espace utilisateur AArch64 pour adresser toute la mémoire ;
- Device Tree `broadcom/bcm2711-rpi-4-b.dtb` ;
- démarrage depuis une carte microSD.

Les Raspberry Pi 400 et Compute Module 4 partagent le BCM2711 mais ne sont pas inclus dans l'image : leurs DTB et leurs contraintes de stockage diffèrent.

## Chaîne de démarrage

Le Pi 4 charge son premier étage depuis l'EEPROM SPI de la carte. La partition FAT de `sdcard.img` contient :

- `start4.elf` et `fixup4.dat` ;
- `config.txt` avec `arm_64bit=1` et `kernel=Image` ;
- le noyau AArch64 `Image` ;
- `bcm2711-rpi-4-b.dtb` et les overlays du firmware ;
- `cmdline.txt`, qui monte `/dev/mmcblk0p2` après détection de la carte.

`bootcode.bin`, `start.elf`, `fixup.dat` et `zImage` appartiennent à l'ancienne chaîne Pi 3 et ne doivent pas être réintroduits. Aucun U-Boot n'est nécessaire.

## Noyau et périphériques

La configuration part du `bcm2711_defconfig` du noyau Raspberry Pi utilisé par le profil officiel Buildroot `raspberrypi4_64_defconfig`. Le fragment du projet conserve :

- stockage USB ;
- Wi-Fi `brcmfmac` et firmware BCM43455 ;
- I2C et SPI ;
- watchdog BCM2835 ;
- horodatage des messages noyau.

L'Ethernet Gigabit BCM54213PE/GENET, le contrôleur PCIe/XHCI USB 3, le stockage SD et l'affichage VC4 viennent du defconfig BCM2711. `wpa_supplicant` et `iw` sont présents, mais aucun SSID ni secret Wi-Fi n'est versionné.

## Particularités du modèle 8 Go

Le choix AArch64 est indispensable pour exploiter simplement les 8 Go. Après démarrage, `/proc/meminfo` doit généralement montrer environ 7,5 à 7,9 Gio, une partie de la mémoire étant réservée au firmware et au matériel. `gpu_mem=64` limite la réservation GPU héritée ; le pilote KMS peut gérer ses propres allocations.

Une alimentation USB-C stable de 5 V/3 A et une carte microSD fiable sont recommandées. Un firmware EEPROM ancien peut empêcher ou perturber le boot ; il doit alors être mis à jour avec les outils officiels Raspberry Pi avant de tester l'image Buildroot.

## Validation matérielle requise

1. Sauvegarder la carte microSD puis flasher `output/images/sdcard.img`.
2. Vérifier la sortie HDMI ou la console série 3,3 V à 115200 8N1.
3. Confirmer `Raspberry Pi 4 Model B`, `getconf LONG_BIT` égal à `64` et la mémoire visible.
4. Vérifier microSD, Ethernet DHCP, USB 2/3 et watchdog.
5. Configurer localement `wpa_supplicant`, puis vérifier le Wi-Fi sans publier les identifiants.
