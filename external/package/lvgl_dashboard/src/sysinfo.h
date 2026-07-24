#ifndef SYSINFO_H
#define SYSINFO_H

#include <stdbool.h>
#include <stdint.h>

#define SYSINFO_IFNAME_MAX 16

typedef struct {
    float cpu_usage_pct;   /* -1 si indisponible */
    float ram_usage_pct;   /* -1 si indisponible */
    float soc_temp_c;      /* -1 si indisponible */

    bool net_available;
    bool net_up;
    char net_ifname[SYSINFO_IFNAME_MAX];
    double net_rx_kbps;
    double net_tx_kbps;
} sysinfo_t;

/* Doit etre appelee periodiquement (ex: toutes les secondes) a intervalle
 * a peu pres regulier : le calcul du CPU et du debit reseau se base sur le
 * delta avec l'appel precedent. */
void sysinfo_read(sysinfo_t *out);

#endif /* SYSINFO_H */
