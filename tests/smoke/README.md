# Smoke tests

Après le premier démarrage sur Raspberry Pi 3, exécuter :

```sh
uname -a
cat /etc/issue
test -x /usr/bin/helloworld && /usr/bin/helloworld
ip link
mount | grep ' on / '
dmesg | grep -Ei 'mmc|usb|eth|brcm|watchdog'
```

Résultats attendus : bannière Linux-embarque, noyau 6.6.x, racine montée, programme `helloworld` fonctionnel et absence d'erreur bloquante de stockage. Ethernet, Wi-Fi et périphériques optionnels restent à valider sur le matériel réel.
