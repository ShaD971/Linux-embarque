# Smoke tests

Après le premier démarrage sur Raspberry Pi 4 Model B 8 Go, exécuter :

```sh
uname -a
getconf LONG_BIT
awk '/MemTotal/ { printf "%.1f GiB visible\n", $2 / 1024 / 1024 }' /proc/meminfo
tr -d '\0' </proc/device-tree/model; echo
cat /etc/issue
test -x /usr/bin/helloworld && /usr/bin/helloworld
ip link
mount | grep ' on / '
dmesg | grep -Ei 'mmc|usb|eth|brcm|watchdog'
```

Résultats attendus : modèle Raspberry Pi 4 Model B, espace utilisateur 64 bits, environ 7,5 à 7,9 Gio de RAM visibles, noyau Raspberry Pi 6.6.x, racine montée, programme `helloworld` fonctionnel et absence d'erreur bloquante de stockage. Ethernet doit obtenir une adresse par DHCP; le Wi-Fi nécessite encore une configuration `wpa_supplicant` propre au réseau.
