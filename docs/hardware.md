# Matériel, Device Tree et démarrage

## Cible détectée

Les rapports historiques nomment une Raspberry Pi 3 et montrent une image SD. Le modèle exact B/B+, la RAM et les périphériques ne sont pas précisés. La configuration moderne cible donc ARM 32 bits Cortex-A53 et produit les DTB mainline :

- `broadcom/bcm2837-rpi-3-b.dtb` ;
- `broadcom/bcm2837-rpi-3-b-plus.dtb`.

## Device Trees historiques

Le dépôt ne contient aucun `.dts` personnalisé pour le projet. Les defconfigs historiques demandent des DTB intégrés au noyau Raspberry Pi : `bcm2710-rpi-3-b`, `bcm2710-rpi-3-b-plus` et `bcm2710-rpi-cm3`. Les noms mainline BCM2837 remplacent B/B+; Compute Module 3 n'est pas inclus faute d'indice indiquant cette carte.

Aucun overlay étudiant n'a été identifié. Le script historique pouvait ajouter `pi3-miniuart-bt`; le nouveau script active l'UART et conserve les overlays du firmware. Le choix exact de console et la coexistence Bluetooth/UART sont **à confirmer sur le matériel**.

## Noyau et pilotes

Le fragment active stockage MMC/SD, ext4/vfat, Ethernet USB LAN78xx, USB hôte/stockage, GPIO, I2C, SPI, watchdog BCM2835, DRM VC4/framebuffer, consoles série et `brcmfmac` en module. Cette liste couvre un Pi 3 générique, pas un inventaire de périphériques prouvé.

À confirmer : firmware Wi-Fi, Bluetooth, écran/caméra, audio, capteurs GPIO, adresses I2C/SPI et watchdog applicatif. Les pilotes ne doivent pas être supprimés pour gagner de l'espace avant cette validation.

## Bootloader

Le profil historique n'active pas U-Boot. Le démarrage utilise le firmware Raspberry Pi (`bootcode.bin`, `start.elf`, `fixup.dat`, `config.txt`) copié dans la partition FAT. Aucun fork U-Boot ni patch U-Boot spécifique n'est donc à migrer.

Une migration future vers U-Boot mainline est possible mais constitue un changement de chaîne de boot hors du périmètre prudent actuel. Elle nécessiterait un defconfig U-Boot Pi 3, une nouvelle disposition d'image, des tests série et une procédure de retour arrière.

## Validation matérielle requise

1. Identifier le modèle exact avec la sérigraphie et `/proc/cpuinfo` sur une image connue.
2. Sauvegarder la carte SD avant flash.
3. Capturer la console série 115200 8N1 et la sortie HDMI.
4. Vérifier MMC, Ethernet, USB, watchdog puis Wi-Fi.
5. Tester le DTB B et B+ correspondant uniquement au modèle réel.
