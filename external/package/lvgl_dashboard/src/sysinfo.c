#include "sysinfo.h"

#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static uint64_t monotonic_ms(void)
{
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000u + (uint64_t)(ts.tv_nsec / 1000000);
}

/* ---- CPU ---- */

static bool read_cpu_ticks(uint64_t *busy, uint64_t *total)
{
    FILE *f = fopen("/proc/stat", "r");
    if (f == NULL) {
        return false;
    }

    unsigned long long user, nice, system, idle, iowait, irq, softirq, steal;
    int n = fscanf(f, "cpu %llu %llu %llu %llu %llu %llu %llu %llu",
                   &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
    fclose(f);

    if (n != 8) {
        return false;
    }

    *busy = (uint64_t)user + nice + system + irq + softirq + steal;
    *total = *busy + idle + iowait;
    return true;
}

static float sample_cpu_usage(void)
{
    static uint64_t prev_busy, prev_total;
    static bool has_prev = false;

    uint64_t busy, total;
    if (!read_cpu_ticks(&busy, &total)) {
        return -1.0f;
    }

    float usage = -1.0f;
    if (has_prev) {
        uint64_t d_busy = busy - prev_busy;
        uint64_t d_total = total - prev_total;
        if (d_total > 0) {
            usage = 100.0f * (float)d_busy / (float)d_total;
        }
    }

    prev_busy = busy;
    prev_total = total;
    has_prev = true;
    return usage;
}

/* ---- RAM ---- */

static void sample_ram(sysinfo_t *out)
{
    out->ram_usage_pct = -1.0f;
    out->ram_used_mb = -1.0f;
    out->ram_total_mb = -1.0f;

    FILE *f = fopen("/proc/meminfo", "r");
    if (f == NULL) {
        return;
    }

    long total_kb = -1, avail_kb = -1;
    char line[128];
    while (fgets(line, sizeof(line), f) != NULL) {
        if (total_kb < 0) {
            sscanf(line, "MemTotal: %ld kB", &total_kb);
        }
        if (avail_kb < 0) {
            sscanf(line, "MemAvailable: %ld kB", &avail_kb);
        }
        if (total_kb >= 0 && avail_kb >= 0) {
            break;
        }
    }
    fclose(f);

    if (total_kb <= 0 || avail_kb < 0) {
        return;
    }

    long used_kb = total_kb - avail_kb;
    out->ram_usage_pct = 100.0f * (float)used_kb / (float)total_kb;
    out->ram_used_mb = (float)used_kb / 1024.0f;
    out->ram_total_mb = (float)total_kb / 1024.0f;
}

/* ---- Uptime et charge moyenne ---- */

static uint64_t sample_uptime(void)
{
    FILE *f = fopen("/proc/uptime", "r");
    if (f == NULL) {
        return 0;
    }

    double up = 0.0;
    int n = fscanf(f, "%lf", &up);
    fclose(f);

    if (n != 1 || up < 0.0) {
        return 0;
    }
    return (uint64_t)up;
}

static float sample_load1(void)
{
    FILE *f = fopen("/proc/loadavg", "r");
    if (f == NULL) {
        return -1.0f;
    }

    float load = -1.0f;
    int n = fscanf(f, "%f", &load);
    fclose(f);

    return (n == 1) ? load : -1.0f;
}

/* ---- Constantes machine ---- */

const char *sysinfo_hostname(void)
{
    static char name[SYSINFO_HOSTNAME_MAX];
    static bool loaded = false;

    if (!loaded) {
        if (gethostname(name, sizeof(name)) != 0) {
            snprintf(name, sizeof(name), "inconnu");
        }
        name[sizeof(name) - 1] = '\0';
        loaded = true;
    }
    return name;
}

int sysinfo_cpu_count(void)
{
    static int count = 0;

    if (count == 0) {
        long n = sysconf(_SC_NPROCESSORS_ONLN);
        count = (n > 0) ? (int)n : 1;
    }
    return count;
}

/* ---- Temperature SoC ---- */

static float sample_soc_temp(void)
{
    FILE *f = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (f == NULL) {
        return -1.0f;
    }

    long millideg = 0;
    int n = fscanf(f, "%ld", &millideg);
    fclose(f);

    if (n != 1) {
        return -1.0f;
    }
    return (float)millideg / 1000.0f;
}

/* ---- Reseau ---- */

/* Les noms d'interface Linux tiennent dans IFNAMSIZ (16, NUL compris). Un nom
 * plus long ne peut pas designer une interface valide : on le rejette au lieu
 * de le tronquer, une troncature fabriquerait un chemin /sys inexistant ou,
 * pire, celui d'une autre interface. */
static bool copy_ifname(char *dst, size_t dst_size, const char *src)
{
    size_t len = strlen(src);
    if (len == 0 || len >= dst_size) {
        return false;
    }
    memcpy(dst, src, len + 1);
    return true;
}

/* Compose /sys/class/net/<ifname>/<suffix>. Renvoie false si le chemin ne tient
 * pas dans le tampon : mieux vaut renoncer que lire un fichier tronque. */
static bool iface_path(char *buf, size_t size, const char *ifname,
                       const char *suffix)
{
    int n = snprintf(buf, size, "/sys/class/net/%s/%s", ifname, suffix);
    return n > 0 && (size_t)n < size;
}

static bool iface_is_up(const char *ifname)
{
    char path[64];
    if (!iface_path(path, sizeof(path), ifname, "operstate")) {
        return false;
    }

    FILE *f = fopen(path, "r");
    if (f == NULL) {
        return false;
    }

    char state[16] = {0};
    bool up = fgets(state, sizeof(state), f) != NULL &&
              strncmp(state, "up", 2) == 0;
    fclose(f);
    return up;
}

static bool read_u64_file(const char *path, uint64_t *out)
{
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        return false;
    }

    unsigned long long v;
    int n = fscanf(f, "%llu", &v);
    fclose(f);

    if (n != 1) {
        return false;
    }
    *out = (uint64_t)v;
    return true;
}

static bool read_iface_bytes(const char *ifname, uint64_t *rx, uint64_t *tx)
{
    char path[80];

    if (!iface_path(path, sizeof(path), ifname, "statistics/rx_bytes") ||
        !read_u64_file(path, rx)) {
        return false;
    }

    if (!iface_path(path, sizeof(path), ifname, "statistics/tx_bytes") ||
        !read_u64_file(path, tx)) {
        return false;
    }

    return true;
}

/* Choisit la premiere interface non-loopback active, sinon la premiere
 * interface non-loopback trouvee. */
static bool select_iface(char *out, size_t out_size, bool *out_up)
{
    DIR *d = opendir("/sys/class/net");
    if (d == NULL) {
        return false;
    }

    char fallback[SYSINFO_IFNAME_MAX] = {0};
    bool found_up = false;
    char up_name[SYSINFO_IFNAME_MAX] = {0};

    struct dirent *ent;
    while ((ent = readdir(d)) != NULL) {
        if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0 ||
            strcmp(ent->d_name, "lo") == 0) {
            continue;
        }

        if (fallback[0] == '\0') {
            copy_ifname(fallback, sizeof(fallback), ent->d_name);
        }

        if (!found_up && iface_is_up(ent->d_name)) {
            found_up = copy_ifname(up_name, sizeof(up_name), ent->d_name);
        }
    }
    closedir(d);

    const char *chosen = found_up ? up_name : fallback;
    if (chosen[0] == '\0') {
        return false;
    }

    if (!copy_ifname(out, out_size, chosen)) {
        return false;
    }
    *out_up = found_up;
    return true;
}

static void sample_network(sysinfo_t *out)
{
    char ifname[SYSINFO_IFNAME_MAX];
    bool up;
    if (!select_iface(ifname, sizeof(ifname), &up)) {
        out->net_available = false;
        out->net_up = false;
        out->net_ifname[0] = '\0';
        out->net_rx_kbps = 0.0;
        out->net_tx_kbps = 0.0;
        return;
    }

    out->net_available = true;
    out->net_up = up;
    copy_ifname(out->net_ifname, sizeof(out->net_ifname), ifname);

    static char prev_ifname[SYSINFO_IFNAME_MAX] = {0};
    static uint64_t prev_rx, prev_tx;
    static uint64_t prev_ts_ms;
    static bool has_prev = false;

    uint64_t rx, tx;
    if (!read_iface_bytes(ifname, &rx, &tx)) {
        out->net_rx_kbps = 0.0;
        out->net_tx_kbps = 0.0;
        return;
    }

    uint64_t now_ms = monotonic_ms();

    bool same_iface = has_prev && strcmp(prev_ifname, ifname) == 0;
    if (same_iface && now_ms > prev_ts_ms) {
        double dt_s = (double)(now_ms - prev_ts_ms) / 1000.0;
        out->net_rx_kbps = (double)(rx - prev_rx) / 1024.0 / dt_s;
        out->net_tx_kbps = (double)(tx - prev_tx) / 1024.0 / dt_s;
    }
    else {
        out->net_rx_kbps = 0.0;
        out->net_tx_kbps = 0.0;
    }

    copy_ifname(prev_ifname, sizeof(prev_ifname), ifname);
    prev_rx = rx;
    prev_tx = tx;
    prev_ts_ms = now_ms;
    has_prev = true;
}

void sysinfo_read(sysinfo_t *out)
{
    out->cpu_usage_pct = sample_cpu_usage();
    out->soc_temp_c = sample_soc_temp();
    out->uptime_s = sample_uptime();
    out->load1 = sample_load1();
    sample_ram(out);
    sample_network(out);
}
