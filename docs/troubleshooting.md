# Dépannage

## Buildroot absent

Exécuter `./scripts/setup.sh`. Vérifier l'accès HTTPS à `buildroot.org`, l'espace disque et la présence de `tar`/`xz`.

## Compilation lancée depuis PowerShell

Utiliser `.\scripts\build.ps1`; il délègue à WSL2. Buildroot ne prend pas en charge une compilation native Windows.

## Defconfig inconnu

Vérifier `external/external.desc`, puis utiliser exactement :

```bash
make -C buildroot BR2_EXTERNAL="$PWD/external" O="$PWD/output" linux_embarque_defconfig
```

## Image SD absente

Consulter `build.log` puis le dernier fichier `logs/build-*.log`. Vérifier la présence de `Image`, `bcm2711-rpi-4-b.dtb`, `rpi-firmware/start4.elf`, `rpi-firmware/fixup4.dat`, `rootfs.ext4` et `host/bin/genimage`.

## Écran arc-en-ciel ou noyau introuvable

Vérifier que la partition FAT contient `config.txt`, `Image`, `bcm2711-rpi-4-b.dtb`, `start4.elf` et `fixup4.dat`. `config.txt` doit contenir `arm_64bit=1` et `kernel=Image`. Mettre à jour l'EEPROM SPI du Pi 4 avec un système Raspberry Pi officiel si son firmware de boot est ancien.

## Pas de console série

Tester HDMI d'abord, puis confirmer `enable_uart=1`, le câblage 3,3 V, 115200 8N1 et la bonne console (`ttyAMA0` ou `ttyS0`). Ne jamais connecter un adaptateur série 5 V.

## Pas de réseau

Examiner `ip link`, `dmesg`, le DTB sélectionné et les firmwares. Ethernet utilise DHCP sur `eth0`. Pour le Wi-Fi, vérifier les fichiers BCM43455 sous `/lib/firmware/brcm`, créer localement la configuration `wpa_supplicant` et ne jamais la publier.

## Seulement 4 Go de RAM visibles

Vérifier `getconf LONG_BIT`, `uname -m` et `arm_64bit=1` dans la partition de boot. Le résultat attendu est `64`, `aarch64` et environ 7,5 à 7,9 Gio dans `/proc/meminfo`. Une image 32 bits ou un firmware EEPROM très ancien doit être remplacé.
