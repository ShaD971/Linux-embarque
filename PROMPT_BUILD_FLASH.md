# Prompt Claude Code — Construire et flasher l'image Raspberry Pi 4

> **Utilisation :** ouvre un terminal **Linux** (WSL2 ou machine Linux) dans le dépôt,
> lance `claude`, puis : « Lis `PROMPT_BUILD_FLASH.md` et exécute-le. Arrête-toi à chaque `[STOP]`. »
>
> ⚠️ Buildroot **ne se construit pas sous Windows**. Sous WSL2, le dépôt doit être sur le
> système de fichiers Linux (`~/Linux-embarque`), **pas** sur `/mnt/c/...` : les permissions
> et la casse de NTFS cassent le build, et c'est 5 à 10× plus lent.

---

## 0. État de départ

| Élément | Valeur |
|---|---|
| Dépôt | `ShaD971/Linux-embarque` (fork Buildroot + `BR2_EXTERNAL` dans `external/`) |
| Branche | `modernization/buildroot-2025.02` |
| Cible | Raspberry Pi 4 Model B 8 Go, aarch64, Cortex-A72 |
| Application | `external/package/lvgl_dashboard/` (dashboard LVGL v9, rendu DRM/KMS) |
| Defconfig | `external/configs/linux_embarque_defconfig` |
| Image produite | `output/images/sdcard.img` **et** `sdcard.img.xz` |
| CI | `.github/workflows/build.yml`, échec actuel sur `Build image` |

Modifications déjà appliquées et **non commitées** :

- `external/package/lvgl_dashboard/src/dashboard.c` — interface refondue
- `external/package/lvgl_dashboard/src/sysinfo.{c,h}` — uptime, RAM absolue, loadavg, hostname, nb cœurs
- `external/board/linux-embarque/cmdline.txt` — suppression du conflit de console (voir §3)
- `external/board/linux-embarque/post-image.sh` — génération de `sdcard.img.xz`

`dashboard.c` et `sysinfo.c` **compilent sans aucun avertissement** en natif x86-64 contre la
révision LVGL épinglée. La compilation croisée aarch64 reste à valider.

---

## 1. ⚠️ Piège n°1 — Faux changements CRLF

Ce dépôt en souffre déjà. `git status` liste régulièrement comme modifiés :

```
external/package/Config.in
external/package/lvgl_dashboard/S99lvgl_dashboard
external/package/lvgl_dashboard/src/lv_conf.h
```

alors que leur **contenu est identique** (insertions = suppressions ; `lv_conf.h` affiche
4984 lignes changées pour zéro changement réel).

Avant tout commit :

```bash
git diff --stat                      # vue brute, trompeuse
git diff --ignore-all-space --stat   # les VRAIS changements
git checkout -- <les faux>
```

Un `.gitattributes` existe déjà. S'il ne suffit pas, vérifie qu'il force bien `eol=lf`
sur `*.sh`, `*.c`, `*.h`, `*.mk`, `Config.in` et `*.txt` du dossier `board/`.

❌ Ne fais jamais `git add -A` en aveugle dans ce dépôt.

---

## 2. ⚠️ Piège n°2 — Le code applicatif existe en double

Le même code vit à deux endroits :

| Chemin | Rôle |
|---|---|
| `Linux-embarque/external/package/lvgl_dashboard/src/` | **Source de vérité** — c'est ce que Buildroot compile |
| `lvgl_dashboard/` (dépôt frère) | Copie de développement, avec le sous-module LVGL |

`lvgl_dashboard.mk` fait `cp -a $(LVGL_DASHBOARD_PKGDIR)/src/. $(@D)/` : **seule la copie
vendorisée compte pour l'image**. Les deux vont dériver.

**[STOP]** Après le build, demande-moi si je veux résoudre cette duplication
(sous-module, ou suppression d'une des deux copies). Ne tranche pas seul.

---

## 3. Le conflit de console — pourquoi `cmdline.txt` a changé

L'ancienne ligne était :

```
root=/dev/mmcblk0p2 rootwait console=tty1 console=ttyAMA0,115200
```

`console=tty1` fait écrire le noyau **sur le framebuffer HDMI**, exactement là où LVGL dessine.
Résultat sur la cible : messages noyau, curseur clignotant et logo Tux par-dessus le dashboard.

Nouvelle ligne :

```
root=/dev/mmcblk0p2 rootwait console=ttyAMA0,115200 quiet loglevel=3 vt.global_cursor_default=0 logo.nologo
```

La console reste sur l'UART série (débogage possible via GPIO 14/15), l'écran est laissé à LVGL.

⚠️ Si le boot échoue, tu n'auras **plus rien à l'écran** — branche un adaptateur USB-série
à 115200 bauds pour voir ce qui se passe. C'est le compromis assumé.

---

## 4. Étape 1 — Construire

```bash
# Dépendances hôte (Debian/Ubuntu)
sudo apt-get update
sudo apt-get install -y --no-install-recommends \
  build-essential bc bison flex libssl-dev cpio unzip rsync file wget \
  git python3 xz-utils

bash ./scripts/setup.sh          # récupère Buildroot à la version voulue
BUILD_JOBS=$(nproc) bash ./scripts/build.sh
```

Compte **45 min à 2 h** au premier build (toolchain complète). Les suivants sont bien plus rapides
grâce à ccache.

### Si le build échoue

`scripts/build.sh` fait déjà le travail de diagnostic : il écrit `build.log`, affiche les
300 dernières lignes **et** une section `=== Relevant error lines ===`.

**Lis cette section en priorité, pas les dernières lignes.** Le message `Buildroot build failed`
et `exit code 2` sont génériques : c'est `make` qui remonte l'échec, ils ne désignent aucun paquet.

Pour identifier le paquet fautif :

```bash
grep -n '^>>>' build.log | tail -40          # dernière étape Buildroot atteinte
grep -n -m1 -E 'Error [0-9]+' build.log      # PREMIÈRE erreur, note la ligne N
sed -n "$((N-120)),$((N+20))p" build.log
```

**[STOP]** Donne-moi : le paquet, l'étape (`Downloading`/`Configuring`/`Building`/`Installing`),
les 10 lignes d'erreur brutes, et ton diagnostic en 2 phrases. **Ne corrige rien avant.**

### Hypothèses, par ordre de probabilité

**H1 — Compilation croisée du dashboard.** Le code compile en x86-64 mais pas forcément en
aarch64. Itère sans relancer le build complet :

```bash
make -C buildroot O=$PWD/output lvgl_dashboard-rebuild V=1 2>&1 | tail -60
make -C buildroot O=$PWD/output lvgl_dashboard-dirclean   # repartir propre
make -C buildroot O=$PWD/output printvars VARS='LVGL_DASHBOARD_%'
```

Le répertoire de build est `output/build/lvgl_dashboard-<version>/`.

**H2 — Hash LVGL.** `BR2_DOWNLOAD_FORCE_CHECK_HASHES=y` est actif et
`lvgl_dashboard.hash` fige le sha256 de l'archive GitHub. Si GitHub régénère le tarball,
le hash ne correspond plus : `ERROR: ... has wrong sha256 hash`.
Dans ce cas seulement, recalcule-le et **dis-le-moi** — ne le remplace jamais en silence.

**H3 — `pkg-config` / libdrm.** Le `Makefile` applicatif fait
`$(shell $(PKG_CONFIG) --cflags libdrm)`. En cross, `pkg-config` doit être le wrapper Buildroot
(`$(HOST_DIR)/bin/pkg-config`), pas celui de l'hôte. Symptôme : `drm.h: No such file or directory`,
ou pire, des chemins `/usr/include` de l'hôte qui polluent la compilation aarch64.

**H4 — Mémoire ou disque.** La CI ajoute 6 Go de swap et libère du disque, ce n'est pas gratuit.
Symptômes : `Killed`, `internal compiler error`, `No space left on device`. En local il faut
~25 Go libres.

**H5 — Liste de sources LVGL.** `lvgl_sources.mk` fige 400 fichiers `.c`. J'ai vérifié :
tous existent à la révision épinglée `c4424b27`. Si tu changes `LVGL_DASHBOARD_VERSION`,
cette liste doit être régénérée, sinon `No rule to make target`.

---

## 5. Étape 2 — Vérifier l'image

```bash
ls -lh output/images/
```

Attendu : `sdcard.img`, `sdcard.img.xz`, `boot.vfat`, `rootfs.ext4`, `Image`,
`bcm2711-rpi-4-b.dtb`, `rpi-firmware/`.

Contrôles :

```bash
# 2 partitions : FAT32 bootable, puis ext4
fdisk -l output/images/sdcard.img

# le binaire est bien là et bien en ARM 64 bits
tar -tf output/images/rootfs.tar 2>/dev/null | grep lvgl_dashboard || true
file output/build/lvgl_dashboard-*/dashboard
```

`file` doit répondre `ELF 64-bit LSB executable, ARM aarch64`.
S'il répond `x86-64`, la cross-compilation ne s'est pas faite : le `Makefile` applicatif
ignore le `CC` de Buildroot.

---

## 6. Étape 3 — Flasher avec Raspberry Pi Imager

Récupère `output/images/sdcard.img.xz` côté Windows (depuis WSL2 :
`cp output/images/sdcard.img.xz /mnt/c/Users/<toi>/Desktop/`).

Dans Raspberry Pi Imager :

1. **Modèle** → Raspberry Pi 4
2. **Système d'exploitation** → tout en bas, **« Use custom » / « Image personnalisée »**
3. Sélectionne `sdcard.img.xz` (Imager décompresse le `.xz` à la volée)
4. **Stockage** → la carte SD
5. Quand Imager propose la **personnalisation d'OS** (Wi-Fi, SSH, utilisateur) :
   **répondre NON**. Ces réglages écrivent des fichiers attendus par Raspberry Pi OS
   (`userconf.txt`, `firstrun.sh`) qu'une image Buildroot ignore, et cela peut corrompre
   le `config.txt` du firmware.
6. Écrire, puis insérer la carte dans la Pi.

**Vérification au premier boot :**

- LED verte (ACT) qui clignote = le firmware lit la carte
- Le dashboard doit apparaître en quelques secondes après l'affichage du logo firmware
- Écran noir persistant → branche l'UART (GPIO 14 TX / 15 RX, 115200 8N1) et lis les logs

**Diagnostic sur la cible** (via série ou clavier + `Alt+F1`) :

```sh
ls -l /dev/dri/          # card0 et renderD128 doivent exister
/usr/bin/lvgl_dashboard  # lancer à la main pour voir l'erreur
dmesg | grep -i vc4
```

Si `/dev/dri/card0` est absent : `dtoverlay=vc4-kms-v3d` n'a pas pris dans `config.txt`,
ou le noyau a été construit sans `CONFIG_DRM_VC4`.

⚠️ **Piège mémoire graphique :** `config_4_64bit.txt` fixe `gpu_mem=64`. En KMS complet, c'est
le **CMA** qui compte, pas `gpu_mem`. En 1080p 32 bits avec double buffer, si l'allocation
échoue (`dmesg` parlera de `cma alloc failed`), passe l'overlay en
`dtoverlay=vc4-kms-v3d,cma-256`.

---

## 7. Étape 4 — Commits

Découpage attendu, en Conventional Commits (sujet anglais, impératif, ≤ 72 car., sans point final) :

```
feat: redesign LVGL dashboard layout for 1080p HDMI
feat: expose uptime, absolute RAM, load average and host info
fix: stop kernel console from writing over the LVGL framebuffer
build: emit sdcard.img.xz for Raspberry Pi Imager
```

❌ Aucune signature, mention d'IA, `Co-Authored-By` ni emoji dans les messages.
❌ Ne commite jamais `output/`, `dl/`, `build.log`, `logs/`, `build-diagnostics/`.
❌ Ne commite pas les faux changements CRLF de §1.

---

## 8. Règles absolues

- ❌ Ne corrige rien avant d'avoir la vraie ligne d'erreur (§4).
- ❌ Ne désactive pas `BR2_PACKAGE_LVGL_DASHBOARD` et n'ajoute pas de `continue-on-error`
  pour faire passer la CI.
- ❌ Ne crée pas de `build.xml` ni de configuration Ant — le projet est en Make/Buildroot.
- ❌ Ne modifie pas les sources amont de Buildroot (`package/`, `arch/`, `toolchain/`) :
  tout ce qui est spécifique au projet vit dans `external/`.
- ❌ Pas de `--force`, `reset --hard`, `clean -fd`, `--no-verify`.
- ❌ Ne mets jamais `output/` en cache CI — seuls `dl/` et ccache sont sûrs.
- ✅ En cas de doute : **demande, ne devine pas.**

---

## 9. Livrable attendu

1. `output/images/sdcard.img.xz` produit.
2. `file output/build/lvgl_dashboard-*/dashboard` renvoie bien `ARM aarch64`.
3. Le dashboard s'affiche sur l'écran HDMI de la Pi 4.
4. Les commits de §7, poussés, avec la CI verte.
