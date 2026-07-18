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

Consulter le dernier fichier `logs/build-*.log`. Vérifier la présence des DTB, du firmware Raspberry Pi, de `rootfs.ext4` et de `host/bin/genimage`.

## Pas de console série

Tester HDMI d'abord, puis confirmer `enable_uart=1`, le câblage 3,3 V, 115200 8N1 et la bonne console (`ttyAMA0` ou `ttyS0`). Ne jamais connecter un adaptateur série 5 V.

## Pas de réseau

Examiner `ip link`, `dmesg`, le DTB sélectionné et les firmwares. Le Wi-Fi n'est pas déclaré validé par cette modernisation.
