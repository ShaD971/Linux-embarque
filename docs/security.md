# Sécurité

- Aucun secret, mot de passe, jeton ou clé privée n'est versionné.
- La connexion root est désactivée par défaut dans le defconfig moderne.
- Aucun serveur SSH n'est activé par défaut.
- Le DHCP Ethernet est activé pour les essais; une image connectée doit recevoir des identifiants uniques, une politique de mise à jour et un pare-feu adaptés.
- Le paquet `helloworld` est local et auditable; les téléchargements Buildroot restent contrôlés par les hashes de la distribution.
- Les images, journaux et configurations générées sont ignorés afin d'éviter la publication accidentelle de données runtime.
- Le script de flash exige le chemin complet du périphérique comme confirmation.
- `make legal-info` doit être exécuté avant redistribution pour collecter licences et sources.

Le firmware de démarrage Raspberry Pi est un composant binaire externe. Son origine, sa licence et sa version doivent figurer dans les informations légales de l'image finale.

Avant production : créer un utilisateur non privilégié, provisionner une clé SSH unique si nécessaire, désactiver les consoles inutiles, appliquer un partitionnement en lecture seule lorsque possible, définir une stratégie CVE et signer les artefacts publiés.
