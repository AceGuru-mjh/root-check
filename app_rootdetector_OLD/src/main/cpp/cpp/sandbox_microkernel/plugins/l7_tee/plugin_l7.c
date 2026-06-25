#include "../../plugin_interface.h"
#include "../../../bare_syscall/bare_syscall.h"
#include "../../../bare_syscall/syscall_arm64.h"
#include "../../../crypto/core/crypto_core.h"
#include <string.h>

#define CHUNK_SIZE       (64 * 1024)
#define BIG_CHUNK        (256 * 1024)
#define MAX_PART_READ    (8 * 1024 * 1024)
#define HASH_HEX_LEN     65
#define XOR_KEY          0x7A

typedef struct { const char *xor; const char *label; int score; } sig_t;

struct xstr { char buf[40]; };
static void dx(struct xstr *xs) {
    for (size_t i = 0; i < sizeof(xs->buf) && xs->buf[i]; i++)
        xs->buf[i] ^= XOR_KEY;
}
#define XS(literal) ({ static struct xstr _xs = { .buf = literal }; dx(&_xs); _xs.buf; })

#define OX(n) (n ^ XOR_KEY)

static int mx(const uint8_t *b, size_t bl, const char *xp, size_t pl) {
    if (pl > bl) return 0;
    for (size_t i = 0; i < pl; i++)
        if (b[i] != (uint8_t)(xp[i] ^ XOR_KEY)) return 0;
    return 1;
}

static int scan_fd(int fd, const uint8_t *buf, size_t bsz,
                    const sig_t *sigs, int nsigs,
                    char *det, size_t dsz, const char *pfx) {
    int sc = 0;
    bare_lseek(fd, 0, BARE_SEEK_SET);
    long n;
    while ((n = bare_read(fd, (void *)buf, bsz)) > 0) {
        for (long i = 0; i + 2 < n; i++) {
            for (int s = 0; s < nsigs; s++) {
                size_t sl = strlen(sigs[s].xor);
                if (i + (long)sl <= n && mx(buf + i, (size_t)(n - i), sigs[s].xor, sl)) {
                    snprintf(det + strlen(det), dsz - strlen(det), "%s%s\n", pfx, sigs[s].label);
                    sc += sigs[s].score;
                    break;
                }
            }
        }
    }
    return sc;
}

static int find_gap(int fd, uint8_t *buf, size_t bsz, int th) {
    bare_lseek(fd, 0, BARE_SEEK_SET);
    long n;
    while ((n = bare_read(fd, buf, bsz)) > 0) {
        int r = 0;
        for (long i = 0; i < n; i++) {
            if (buf[i] == 0) { if (++r > th) return 1; } else r = 0;
        }
    }
    return 0;
}

static long part_size(int fd) {
    long e = bare_lseek(fd, 0, BARE_SEEK_END);
    bare_lseek(fd, 0, BARE_SEEK_SET);
    return e;
}

static void b2h(const uint8_t *b, size_t l, char *o) {
    static const char h[] = "0123456789abcdef";
    size_t m = l > 32 ? 32 : l;
    for (size_t i = 0; i < m; i++) { o[i*2]=h[b[i]>>4]; o[i*2+1]=h[b[i]&15]; }
    o[m*2] = 0;
}

static int top(const char **ps) {
    for (; *ps; ps++) { int fd = (int)bare_open(*ps, 0, 0); if (fd >= 0) return fd; }
    return -1;
}

static void *xmap(size_t sz) {
    long r = bare_mmap(NULL, sz, BARE_PROT_READ|BARE_PROT_WRITE,
                       BARE_MAP_PRIVATE|BARE_MAP_ANONYMOUS, -1, 0);
    return bare_is_error(r) ? NULL : (void *)r;
}

static void hstream(int fd, uint8_t *d, uint8_t *b, size_t bs, size_t mr) {
    sha3_ctx c; sha3_256_init(&c);
    size_t t = 0; long n;
    while (t < mr && (n = bare_read(fd, b, bs)) > 0) {
        size_t a = (size_t)(n > (long)(mr - t) ? (long)(mr - t) : n);
        sha3_256_update(&c, b, a); t += a;
        if (n < (long)bs) break;
    }
    sha3_256_final(&c, d);
}

/* ─── XOR-obfuscated signature tables ─── */

/* Boot markers */
static const char _androusr[] = { OX('a'),OX('n'),OX('d'),OX('r'),OX('o'),OX('u'),OX('s'),OX('r'),0 };
static const char _android[]  = { OX('a'),OX('n'),OX('d'),OX('r'),OX('o'),OX('i'),OX('d'),0 };
static const char _kernel[]   = { OX('K'),OX('E'),OX('R'),OX('N'),OX('E'),OX('L'),0 };
static const char _ramdisk[]  = { OX('R'),OX('A'),OX('M'),OX('D'),OX('I'),OX('S'),OX('K'),0 };

/* Magisk family */
static const char _magisk[]   = { OX('m'),OX('a'),OX('g'),OX('i'),OX('s'),OX('k'),0 };
static const char _MAGISK[]   = { OX('M'),OX('A'),OX('G'),OX('I'),OX('S'),OX('K'),0 };
static const char _mginit[]   = { OX('i'),OX('n'),OX('i'),OX('t'),OX('.'),OX('m'),OX('a'),OX('g'),OX('i'),OX('s'),OX('k'),0 };
static const char _zygisk[]   = { OX('z'),OX('y'),OX('g'),OX('i'),OX('s'),OX('k'),0 };
static const char _mgdelta[]  = { OX('M'),OX('a'),OX('g'),OX('i'),OX('s'),OX('k'),OX('D'),OX('e'),OX('l'),OX('t'),OX('a'),0 };
static const char _mgalpha[]  = { OX('M'),OX('a'),OX('g'),OX('i'),OX('s'),OX('k'),OX('A'),OX('l'),OX('p'),OX('h'),OX('a'),0 };
static const char _magiskpol[]={ OX('m'),OX('a'),OX('g'),OX('i'),OX('s'),OX('k'),OX('p'),OX('o'),OX('l'),OX('i'),OX('c'),OX('y'),0 };

/* APatch family */
static const char _kpatch[]   = { OX('k'),OX('p'),OX('a'),OX('t'),OX('c'),OX('h'),0 };
static const char _APATCH[]   = { OX('A'),OX('P'),OX('A'),OX('T'),OX('C'),OX('H'),0 };
static const char _apatch[]   = { OX('a'),OX('p'),OX('a'),OX('t'),OX('c'),OX('h'),0 };
static const char _kpmod[]    = { OX('k'),OX('p'),OX('_'),OX('m'),OX('o'),OX('d'),OX('u'),OX('l'),OX('e'),0 };
static const char _apatchd[]  = { OX('A'),OX('P'),OX('a'),OX('t'),OX('c'),OX('h'),OX('d'),0 };

/* KernelSU family */
static const char _kernelsu[] = { OX('K'),OX('e'),OX('r'),OX('n'),OX('e'),OX('l'),OX('S'),OX('U'),0 };
static const char _KSU[]      = { OX('K'),OX('S'),OX('U'),0 };
static const char _ksud[]     = { OX('k'),OX('s'),OX('u'),OX('d'),0 };
static const char _ksu_drv[]  = { OX('k'),OX('s'),OX('u'),OX('_'),OX('d'),OX('r'),OX('v'),0 };
static const char _ksu_al[]   = { OX('k'),OX('s'),OX('u'),OX('_'),OX('a'),OX('l'),OX('l'),OX('o'),OX('w'),OX('l'),OX('i'),OX('s'),OX('t'),0 };
static const char _ksu_sc[]   = { OX('k'),OX('s'),OX('u'),OX('_'),OX('s'),OX('u'),OX('c'),OX('o'),OX('m'),OX('p'),OX('a'),OX('t'),0 };
static const char _ksu_gki[]  = { OX('K'),OX('S'),OX('U'),OX('_'),OX('G'),OX('K'),OX('I'),0 };
static const char _ksu_ng[]   = { OX('K'),OX('S'),OX('U'),OX('_'),OX('N'),OX('O'),OX('N'),OX('_'),OX('G'),OX('K'),OX('I'),0 };

/* Rootkit libraries */
static const char _hijack[]   = { OX('h'),OX('i'),OX('j'),OX('a'),OX('c'),OX('k'),0 };
static const char _inject[]   = { OX('i'),OX('n'),OX('j'),OX('e'),OX('c'),OX('t'),0 };
static const char _rootkit[]  = { OX('r'),OX('o'),OX('o'),OX('t'),OX('k'),OX('i'),OX('t'),0 };
static const char _bdor[]     = { OX('b'),OX('a'),OX('c'),OX('k'),OX('d'),OX('o'),OX('o'),OX('r'),0 };
static const char _hidepid[]  = { OX('h'),OX('i'),OX('d'),OX('e'),OX('p'),OX('i'),OX('d'),0 };
static const char _godmode[]  = { OX('g'),OX('o'),OX('d'),OX('_'),OX('m'),OX('o'),OX('d'),OX('e'),0 };
static const char _diamorph[] = { OX('d'),OX('i'),OX('a'),OX('m'),OX('o'),OX('r'),OX('p'),OX('h'),OX('i'),OX('n'),OX('e'),0 };
static const char _suterusu[] = { OX('s'),OX('u'),OX('t'),OX('e'),OX('r'),OX('u'),OX('s'),OX('u'),0 };
static const char _reptile[]  = { OX('r'),OX('e'),OX('p'),OX('t'),OX('i'),OX('l'),OX('e'),0 };
static const char _vlany[]    = { OX('v'),OX('l'),OX('a'),OX('n'),OX('y'),0 };

/* Recovery signatures */
static const char _twrp[]     = { OX('T'),OX('W'),OX('R'),OX('P'),0 };
static const char _tw_[]      = { OX('T'),OX('W'),OX('_'),0 };
static const char _teamwin[]  = { OX('T'),OX('e'),OX('a'),OX('m'),OX('W'),OX('i'),OX('n'),0 };
static const char _ofox[]     = { OX('O'),OX('r'),OX('a'),OX('n'),OX('g'),OX('e'),OX('F'),OX('o'),OX('x'),0 };
static const char _FOX[]      = { OX('F'),OX('O'),OX('X'),OX('_'),0 };
static const char _pb[]       = { OX('P'),OX('i'),OX('t'),OX('c'),OX('h'),OX('B'),OX('l'),OX('a'),OX('c'),OX('k'),0 };
static const char _pbrp[]     = { OX('P'),OX('B'),OX('R'),OX('P'),0 };
static const char _shrp[]     = { OX('S'),OX('H'),OX('R'),OX('P'),0 };
static const char _skyhawk[]  = { OX('s'),OX('k'),OX('y'),OX('h'),OX('a'),OX('w'),OX('k'),0 };
static const char _redwolf[]  = { OX('R'),OX('e'),OX('d'),OX('W'),OX('o'),OX('l'),OX('f'),0 };
static const char _melina[]   = { OX('m'),OX('e'),OX('l'),OX('i'),OX('n'),OX('a'),0 };

/* TEE keys */
static const char _trusty[]   = { OX('t'),OX('r'),OX('u'),OX('s'),OX('t'),OX('y'),0 };
static const char _wv[]       = { OX('w'),OX('i'),OX('d'),OX('e'),OX('v'),OX('i'),OX('n'),OX('e'),0 };
static const char _km[]       = { OX('k'),OX('e'),OX('y'),OX('m'),OX('a'),OX('s'),OX('t'),OX('e'),OX('r'),0 };
static const char _gk[]       = { OX('g'),OX('a'),OX('t'),OX('e'),OX('k'),OX('e'),OX('e'),OX('p'),OX('e'),OX('r'),0 };
static const char _otp[]      = { OX('o'),OX('t'),OX('p'),0 };

/* Misc partition bootloader msgs */
static const char _batrecovery[] = { OX('b'),OX('o'),OX('o'),OX('t'),OX('-'),OX('r'),OX('e'),OX('c'),OX('o'),OX('v'),OX('e'),OX('r'),OX('y'),0 };
static const char _recoveryupdate[]={ OX('r'),OX('e'),OX('c'),OX('o'),OX('v'),OX('e'),OX('r'),OX('y'),OX('-'),OX('u'),OX('p'),OX('d'),OX('a'),OX('t'),OX('e'),0 };

/* dm-verity / AVB */
static const char _dmverity[] = { OX('d'),OX('m'),OX('_'),OX('v'),OX('e'),OX('r'),OX('i'),OX('t'),OX('y'),0 };
static const char _avb[]      = { OX('A'),OX('n'),OX('d'),OX('r'),OX('o'),OX('i'),OX('d'),OX('V'),OX('e'),OX('r'),OX('i'),OX('f'),OX('i'),OX('e'),OX('d'),OX('B'),OX('o'),OX('o'),OX('t'),0 };
static const char _vbmeta[]   = { OX('v'),OX('b'),OX('m'),OX('e'),OX('t'),OX('a'),0 };
static const char _avbkey[]   = { OX('A'),OX('V'),OX('B'),OX('_'),OX('k'),OX('e'),OX('y'),0 };

/* TEE exploit markers */
static const char _teebreak[] = { OX('T'),OX('E'),OX('E'),OX('_'),OX('b'),OX('r'),OX('e'),OX('a'),OX('k'),0 };
static const char _tz_rollback[]={OX('r'),OX('o'),OX('l'),OX('l'),OX('b'),OX('a'),OX('c'),OX('k'),OX('_'),OX('d'),OX('e'),OX('t'),OX('e'),OX('c'),OX('t'),OX('e'),OX('d'),0 };

static const char _keystore0[] = { OX('K'),OX('e'),OX('y'),OX('s'),OX('t'),OX('o'),OX('r'),OX('e'),OX('H'),OX('a'),OX('s'),OX('h'),OX('e'),OX('s'),0 };

/* ==================================================================
 * check_boot_partition — 7 sub-detectors
 * ================================================================ */
static int check_boot_partition(char *det, size_t dsz) {
    const char *bp[] = {"/dev/block/by-name/boot","/dev/block/by-name/boot_a",
                        "/dev/block/by-name/boot_b","/dev/block/boot",NULL};
    int fd = top(bp);
    if (fd < 0) { snprintf(det+strlen(det),dsz-strlen(det),"L7-BT: no-access\n"); return 5; }

    uint8_t *c = (uint8_t *)xmap(BIG_CHUNK);
    if (!c) { bare_close(fd); return 0; }

    int sc = 0;
    long ps = part_size(fd);
    size_t mr = ps>0 && (size_t)ps<MAX_PART_READ ? (size_t)ps : MAX_PART_READ;

    uint8_t dg[SHA3_256_DIGEST_LEN];
    hstream(fd, dg, c, BIG_CHUNK, mr);
    char hx[HASH_HEX_LEN]; b2h(dg, SHA3_256_DIGEST_LEN, hx);
    snprintf(det+strlen(det),dsz-strlen(det),"L7-BT: hash=%s sz=%ld\n", hx, ps>0?ps:0L);

    /* 1. Boot image structural markers */
    sig_t m1[] = {{_androusr,"struct",3},{_kernel,"KERNEL",3},{_ramdisk,"RAMDISK",3},{_android,"and-",2}};
    sc += scan_fd(fd, c, BIG_CHUNK, m1, 4, det, dsz, "L7-BT: mk-");

    /* 2. Magisk gap (>4KB zero run) */
    if (find_gap(fd, c, BIG_CHUNK, 4096)) {
        snprintf(det+strlen(det),dsz-strlen(det),"L7-BT: mg-gap\n"); sc += 25; }

    /* 3. Magisk family strings */
    sig_t ms[] = {{_magisk,"magisk",30},{_MAGISK,"MAGISK",30},{_mginit,"init.magisk",30},
                  {_zygisk,"zygisk",25},{_mgdelta,"MagiskDelta",28},{_mgalpha,"MagiskAlpha",28},
                  {_magiskpol,"magiskpolicy",20}};
    sc += scan_fd(fd, c, BIG_CHUNK, ms, 7, det, dsz, "L7-BT: ");

    /* 4. APatch family */
    sig_t as[] = {{_kpatch,"kpatch",30},{_APATCH,"APATCH",30},{_apatch,"apatch",30},
                  {_kpmod,"kp_module",28},{_apatchd,"APatchd",28}};
    sc += scan_fd(fd, c, BIG_CHUNK, as, 5, det, dsz, "L7-BT: ");

    /* 5. KernelSU family */
    sig_t ks[] = {{_kernelsu,"KernelSU",30},{_KSU,"KSU",25},{_ksud,"ksud",28},
                  {_ksu_drv,"ksu_drv",28},{_ksu_al,"ksu_allowlist",25},{_ksu_sc,"ksu_sucompat",25},
                  {_ksu_gki,"KSU_GKI",25},{_ksu_ng,"KSU_NON_GKI",25}};
    sc += scan_fd(fd, c, BIG_CHUNK, ks, 8, det, dsz, "L7-BT: ");

    /* 6. dm-verity / AVB status flags */
    sig_t av[] = {{_dmverity,"dm_verity",10},{_avb,"AVB",10},{_vbmeta,"vbmeta",10}};
    sc += scan_fd(fd, c, BIG_CHUNK, av, 3, det, dsz, "L7-BT: ");

    /* 7. Known rootkit module names in initramfs */
    sig_t rk[] = {{_diamorph,"diamorphine",20},{_suterusu,"suterusu",20},
                  {_reptile,"reptile",20},{_vlany,"vlany",20}};
    sc += scan_fd(fd, c, BIG_CHUNK, rk, 4, det, dsz, "L7-BT: rk-");

    bare_munmap(c, BIG_CHUNK); bare_close(fd);
    return sc;
}

/* ==================================================================
 * check_vendor_partition — 4 sub-detectors
 * ================================================================ */
static int check_vendor_partition(char *det, size_t dsz) {
    const char *vp[] = {"/dev/block/by-name/vendor","/dev/block/by-name/vendor_a",
                        "/dev/block/by-name/vendor_b","/dev/block/vendor",NULL};
    int fd = top(vp);
    if (fd < 0) { snprintf(det+strlen(det),dsz-strlen(det),"L7-VD: no-access\n"); return 3; }

    uint8_t *c = (uint8_t *)xmap(BIG_CHUNK);
    if (!c) { bare_close(fd); return 0; }

    int sc = 0;
    long ps = part_size(fd);
    size_t mr = ps>0 && (size_t)ps<MAX_PART_READ ? (size_t)ps : MAX_PART_READ;

    uint8_t dg[SHA3_256_DIGEST_LEN];
    hstream(fd, dg, c, BIG_CHUNK, mr);
    char hx[HASH_HEX_LEN]; b2h(dg, SHA3_256_DIGEST_LEN, hx);
    snprintf(det+strlen(det),dsz-strlen(det),"L7-VD: hash=%s\n", hx);

    /* 1. Rootkit strings in vendor partition binary */
    sig_t rk[] = {{_hijack,"hijack",15},{_inject,"inject",15},{_rootkit,"rootkit",15},
                  {_bdor,"backdoor",15},{_hidepid,"hidepid",12},{_godmode,"god_mode",12},
                  {_diamorph,"diamorphine",18},{_suterusu,"suterusu",18},
                  {_reptile,"reptile",18}};
    sc += scan_fd(fd, c, BIG_CHUNK, rk, 9, det, dsz, "L7-VD: sig-");

    /* 2. TEE / keymaster related anomalies */
    sig_t tee[] = {{_teebreak,"TEE_break",20},{_tz_rollback,"rollback_detected",15}};
    sc += scan_fd(fd, c, BIG_CHUNK, tee, 2, det, dsz, "L7-VD: ");

    bare_munmap(c, BIG_CHUNK); bare_close(fd);

    /* 3. Suspicious .so files in mounted vendor */
    if (!bare_exists("/vendor")) return sc;
    const char *sos[] = {"libhijack","libinject","libroot","libhide",
                         "libsu","libkernelsu","libzygisk","libkpatch","libapatch",NULL};
    char p[256];
    for (const char **so = sos; *so; so++) {
        bare_memset(p,0,sizeof(p)); snprintf(p,sizeof(p),"/vendor/lib64/%s.so",*so);
        if (bare_exists(p)) { snprintf(det+strlen(det),dsz-strlen(det),"L7-VD: so64 %s\n",*so); sc+=20; }
        bare_memset(p,0,sizeof(p)); snprintf(p,sizeof(p),"/vendor/lib/%s.so",*so);
        if (bare_exists(p)) { snprintf(det+strlen(det),dsz-strlen(det),"L7-VD: so %s\n",*so); sc+=20; }
    }

    /* 4. vendor_dlkm rootkit modules */
    const char *rkm[] = {"magisk","kernelsu","ksu","apatch","rootkit","kp_module",NULL};
    if (bare_exists("/vendor_dlkm")) {
        for (const char **m = rkm; *m; m++) {
            bare_memset(p,0,sizeof(p)); snprintf(p,sizeof(p),"/vendor_dlkm/lib/modules/%s.ko",*m);
            if (bare_exists(p)) { snprintf(det+strlen(det),dsz-strlen(det),"L7-VD: dlkm %s\n",*m); sc+=20; }
        }
    }
    return sc;
}

/* ==================================================================
 * check_recovery_partition — 3 sub-detectors
 * ================================================================ */
static int check_recovery_partition(char *det, size_t dsz) {
    const char *rp[] = {"/dev/block/by-name/recovery","/dev/block/by-name/recovery_a",
                        "/dev/block/by-name/recovery_b","/dev/block/recovery",NULL};
    int fd = top(rp);
    if (fd < 0) { snprintf(det+strlen(det),dsz-strlen(det),"L7-RC: no-access\n"); return 2; }

    uint8_t *c = (uint8_t *)xmap(BIG_CHUNK);
    if (!c) { bare_close(fd); return 0; }

    int sc = 0;
    long ps = part_size(fd);
    size_t mr = ps>0 && (size_t)ps<MAX_PART_READ ? (size_t)ps : MAX_PART_READ;

    uint8_t dg[SHA3_256_DIGEST_LEN];
    hstream(fd, dg, c, BIG_CHUNK, mr);
    char hx[HASH_HEX_LEN]; b2h(dg, SHA3_256_DIGEST_LEN, hx);
    snprintf(det+strlen(det),dsz-strlen(det),"L7-RC: hash=%s\n", hx);

    /* 1. Custom recovery signatures */
    sig_t cs[] = {{_teamwin,"TeamWin",20},{_tw_,"TW_",20},{_twrp,"TWRP",20},
                  {_ofox,"OrangeFox",20},{_FOX,"FOX_",20},
                  {_pb,"PitchBlack",20},{_pbrp,"PBRP",20},
                  {_shrp,"SHRP",20},{_skyhawk,"SkyHawk",20},
                  {_redwolf,"RedWolf",20},{_melina,"melina",18}};
    int rsc = scan_fd(fd, c, BIG_CHUNK, cs, 11, det, dsz, "L7-RC: ");

    /* 2. Custom build fingerprint in recovery */
    bare_lseek(fd,0,BARE_SEEK_SET);
    int cust = 0; long n;
    while ((n=bare_read(fd,c,BIG_CHUNK)) > 0) {
        for (long i=0; i+8<=n; i++)
            if (bare_memcmp(c+i,(const uint8_t*)"ro.build",8)==0) { cust=1; break; }
        if (cust) break;
    }
    if (cust) { snprintf(det+strlen(det),dsz-strlen(det),"L7-RC: custom-build\n"); sc+=10; }
    if (rsc==0 && !cust) snprintf(det+strlen(det),dsz-strlen(det),"L7-RC: stock\n");
    bare_munmap(c,BIG_CHUNK); bare_close(fd);

    /* 3. misc partition bootloader msgs */
    const char *mp[] = {"/dev/block/by-name/misc","/dev/block/misc",NULL};
    int mfd = top(mp);
    if (mfd >= 0) {
        uint8_t *mc = (uint8_t *)xmap(4096);
        if (mc) {
            bare_lseek(mfd,0,BARE_SEEK_SET);
            if ((n=bare_read(mfd,mc,4096)) > 0) {
                sig_t bm[] = {{_batrecovery,"boot-recovery",8},{_recoveryupdate,"recovery-update",8}};
                sc += scan_fd(mfd, mc, 4096, bm, 2, det, dsz, "L7-RC: misc-");
            }
            bare_munmap(mc,4096);
        }
        bare_close(mfd);
    }
    return sc;
}

/* ==================================================================
 * check_secure_storage — 4 sub-detectors
 * ================================================================ */
static int check_secure_storage(char *det, size_t dsz) {
    int sc = 0;

    /* 1. TEE device nodes */
    const char *td[] = {"/dev/tee0","/dev/tee1","/dev/trusty0",
                        "/dev/trusty","/dev/tzdev","/dev/gzdev",NULL};
    int tf = 0;
    for (const char **d = td; *d; d++) {
        if (bare_exists(*d)) { snprintf(det+strlen(det),dsz-strlen(det),"L7-TEE: dev %s\n",*d); tf=1; sc+=3; }
    }
    if (!tf) snprintf(det+strlen(det),dsz-strlen(det),"L7-TEE: no-tee-devs\n");

    /* 2. /proc/keys TEE key analysis */
    int kfd = (int)bare_open("/proc/keys",0,0);
    if (kfd >= 0) {
        uint8_t *kb = (uint8_t *)xmap(CHUNK_SIZE);
        if (kb) {
            sig_t ks[] = {{_trusty,"trusty",4},{_wv,"widevine",4},{_km,"keymaster",4},
                          {_gk,"gatekeeper",4},{_otp,"otp",4}};
            sc += scan_fd(kfd, kb, CHUNK_SIZE, ks, 5, det, dsz, "L7-TEE: key-");
            bare_munmap(kb, CHUNK_SIZE);
        }
        bare_close(kfd);
        /* Key absence detection — keystore erased */
    } else {
        snprintf(det+strlen(det),dsz-strlen(det),"L7-TEE: keys-restricted\n");
        sc += 8;
    }

    /* 3. Secure storage partition hashes */
    const char *sp[] = {"/dev/block/by-name/sec","/dev/block/by-name/keystore",
                        "/dev/block/by-name/trusty","/dev/block/by-name/tee",
                        "/dev/block/by-name/ta","/dev/block/by-name/secure",NULL};
    uint8_t *sb = (uint8_t *)xmap(CHUNK_SIZE);
    if (sb) {
        for (const char **p = sp; *p; p++) {
            int pf = (int)bare_open(*p,0,0);
            if (pf < 0) continue;
            long psz = part_size(pf);
            size_t mr = psz>0 && (size_t)psz<MAX_PART_READ ? (size_t)psz : MAX_PART_READ;
            sha3_ctx c; sha3_256_init(&c);
            long rn; size_t tot=0;
            while (tot<mr && (rn=bare_read(pf,sb,CHUNK_SIZE))>0) {
                size_t a=(size_t)(rn>(long)(mr-tot)?(long)(mr-tot):rn);
                sha3_256_update(&c,sb,a); tot+=a;
                if (rn<(long)CHUNK_SIZE) break;
            }
            bare_close(pf);
            uint8_t pd[SHA3_256_DIGEST_LEN]; sha3_256_final(&c,pd);
            char ph[HASH_HEX_LEN]; b2h(pd,SHA3_256_DIGEST_LEN,ph);
            snprintf(det+strlen(det),dsz-strlen(det),"L7-TEE: sec-%s hash=%s\n",*p,ph);
            sc += 5;
        }
        bare_munmap(sb, CHUNK_SIZE);
    }

    /* 4. /sys/kernel/security TEE attrs */
    if (bare_exists("/sys/kernel/security/trusted_execution"))
        { snprintf(det+strlen(det),dsz-strlen(det),"L7-TEE: sysfs-tee\n"); sc+=3; }
    if (bare_exists("/proc/device-tree/firmware/android/trusted_execution"))
        { snprintf(det+strlen(det),dsz-strlen(det),"L7-TEE: dt-tee\n"); sc+=3; }

    return sc;
}

/* ==================================================================
 * check_logical_partitions — super/vbmeta/dtbo/persist
 * ================================================================ */
static int check_logical_partitions(char *det, size_t dsz) {
    int sc = 0;

    /* ── super partition (logical volume table) ── */
    const char *sup[] = {"/dev/block/by-name/super","/dev/block/by-name/super_a",
                         "/dev/block/by-name/super_b","/dev/block/super",NULL};
    int sfd = top(sup);
    if (sfd >= 0) {
        uint8_t *c = (uint8_t *)xmap(BIG_CHUNK);
        if (c) {
            long ps = part_size(sfd);
            size_t mr = ps>0 && (size_t)ps<MAX_PART_READ ? (size_t)ps : MAX_PART_READ;
            uint8_t dg[SHA3_256_DIGEST_LEN]; hstream(sfd,dg,c,BIG_CHUNK,mr);
            char hx[HASH_HEX_LEN]; b2h(dg,SHA3_256_DIGEST_LEN,hx);
            snprintf(det+strlen(det),dsz-strlen(det),"L7-SUP: hash=%s sz=%ld\n",hx,ps>0?ps:0L);
            sc += 5;

            sig_t lm[] = {{_magisk,"magisk",10},{_MAGISK,"MAGISK",10},
                          {_apatch,"apatch",10},{_kernelsu,"kernelsu",10}};
            sc += scan_fd(sfd,c,BIG_CHUNK,lm,4,det,dsz,"L7-SUP: ");

            /* Check for LpMetadata logical table corruption */
            sig_t lp[] = {{_dmverity,"dm_verity",5},{_vbmeta,"vbmeta",5}};
            sc += scan_fd(sfd,c,BIG_CHUNK,lp,2,det,dsz,"L7-SUP: ");
            bare_munmap(c,BIG_CHUNK);
        }
        bare_close(sfd);
    }

    /* ── vbmeta / vbmeta_system (AVB verified boot) ── */
    const char *vb[] = {"/dev/block/by-name/vbmeta","/dev/block/by-name/vbmeta_a",
                        "/dev/block/by-name/vbmeta_b",NULL};
    int vfd = top(vb);
    if (vfd >= 0) {
        uint8_t vhdr[256];
        long nr = bare_read(vfd, vhdr, sizeof(vhdr));
        if (nr > 0) {
            /* Check AVB magic: "AVB0" at offset 0 */
            const uint8_t avb_magic[] = {0x41,0x56,0x42,0x30};
            if (bare_memcmp(vhdr, avb_magic, 4) == 0) {
                uint8_t flags = vhdr[16]; /* flags byte in VBMeta header */
                int verify_disabled = (flags & 0x01) ? 1 : 0;
                if (verify_disabled) {
                    snprintf(det+strlen(det),dsz-strlen(det),"L7-VB: verify-off\n");
                    sc += 25;
                } else {
                    snprintf(det+strlen(det),dsz-strlen(det),"L7-VB: verify-on\n");
                    sc += 2;
                }
            } else {
                snprintf(det+strlen(det),dsz-strlen(det),"L7-VB: no-avb\n");
                sc += 10;
            }
        }
        bare_close(vfd);
    }

    const char *vbs[] = {"/dev/block/by-name/vbmeta_system",
                         "/dev/block/by-name/vbmeta_system_a",
                         "/dev/block/by-name/vbmeta_system_b",NULL};
    int vbfd = top(vbs);
    if (vbfd >= 0) {
        snprintf(det+strlen(det),dsz-strlen(det),"L7-VB: vbmeta-system\n");
        bare_close(vbfd);
    }

    /* ── dtbo partition (device tree blob override) ── */
    const char *dt[] = {"/dev/block/by-name/dtbo","/dev/block/by-name/dtbo_a",
                        "/dev/block/by-name/dtbo_b","/dev/block/dtbo",NULL};
    int dfd = top(dt);
    if (dfd >= 0) {
        uint8_t *dc = (uint8_t *)xmap(CHUNK_SIZE);
        if (dc) {
            sha3_ctx c; sha3_256_init(&c);
            long rn;
            while ((rn=bare_read(dfd,dc,CHUNK_SIZE))>0) { sha3_256_update(&c,dc,(size_t)rn); if (rn<(long)CHUNK_SIZE) break; }
            uint8_t dd[SHA3_256_DIGEST_LEN]; sha3_256_final(&c,dd);
            char dh[HASH_HEX_LEN]; b2h(dd,SHA3_256_DIGEST_LEN,dh);
            snprintf(det+strlen(det),dsz-strlen(det),"L7-DT: hash=%s\n",dh);
            bare_munmap(dc,CHUNK_SIZE);
        }
        bare_close(dfd);
        sc += 5;
    }

    /* ── persist partition (persistent root markers) ── */
    const char *pp[] = {"/dev/block/by-name/persist","/dev/block/persist",NULL};
    int pfd = top(pp);
    if (pfd >= 0) {
        uint8_t *pc = (uint8_t *)xmap(CHUNK_SIZE);
        if (pc) {
            bare_lseek(pfd,0,BARE_SEEK_SET);
            long rn;
            int mod = 0;
            while ((rn=bare_read(pfd,pc,CHUNK_SIZE))>0) {
                for (long i=0; i+6<=rn; i++) {
                    if (bare_memcmp(pc+i,(const uint8_t*)"magisk",6)==0||
                        bare_memcmp(pc+i,(const uint8_t*)"su_",3)==0||
                        bare_memcmp(pc+i,(const uint8_t*)"root_",5)==0) { mod=1; break; }
                }
                if (mod) break;
            }
            if (mod) { snprintf(det+strlen(det),dsz-strlen(det),"L7-PERSIST: modified\n"); sc+=15; }
            bare_munmap(pc,CHUNK_SIZE);
        }
        bare_close(pfd);
    }

    return sc;
}

/* ==================================================================
 * check_kernel_logs — dmesg/pstore analysis
 * ================================================================ */
static int check_kernel_logs(char *det, size_t dsz) {
    int sc = 0;

    /* 1. pstore (persistent ramoops) — post-crash forensics */
    if (bare_exists("/sys/fs/pstore")) {
        snprintf(det+strlen(det),dsz-strlen(det),"L7-LOG: pstore\n");
        sc += 3;

        /* Check pstore logs for root markers */
        const char *pstore_files[] = {
            "/sys/fs/pstore/console-ramoops-0", "/sys/fs/pstore/console-ramoops-1",
            "/sys/fs/pstore/dmesg-ramoops-0",   "/sys/fs/pstore/dmesg-ramoops-1",
            NULL
        };
        uint8_t *buf = (uint8_t *)xmap(CHUNK_SIZE);
        if (buf) {
            for (const char **pf = pstore_files; *pf; pf++) {
                int lfd = (int)bare_open(*pf, 0, 0);
                if (lfd < 0) continue;
                long rn;
                while ((rn = bare_read(lfd, buf, CHUNK_SIZE)) > 0) {
                    sig_t lm[] = {{_magisk,"magisk",10},{_kernelsu,"kernelsu",10},
                                  {_apatch,"apatch",10},{_teebreak,"TEE_break",8}};
                    for (long i=0; i+4<rn; i++) {
                        for (int s=0; s<4; s++) {
                            size_t sl = strlen(lm[s].xor);
                            if (i+(long)sl<=rn && mx(buf+i,(size_t)(rn-i),lm[s].xor,sl)) {
                                snprintf(det+strlen(det),dsz-strlen(det),"L7-LOG: %s\n",lm[s].label);
                                sc += lm[s].score;
                                break;
                            }
                        }
                    }
                }
                bare_close(lfd);
            }
            bare_munmap(buf, CHUNK_SIZE);
        }
    }

    /* 2. last_kmsg if available */
    if (bare_exists("/proc/last_kmsg")) {
        snprintf(det+strlen(det),dsz-strlen(det),"L7-LOG: last-kmsg\n");
        sc += 2;
    }

    return sc;
}

/* ==================================================================
 * self_check — plugin integrity verification
 * ================================================================ */
static int self_check(char *det, size_t dsz) {
    int sc = 0;
    const char *self_paths[] = {
        "/proc/self/exe",
        "/data/local/tmp/libplugin_l7.so",
        NULL
    };
    /* Open our own /proc/self/maps to verify no LD_PRELOAD / hook */
    int mfd = (int)bare_open("/proc/self/maps", 0, 0);
    if (mfd >= 0) {
        uint8_t *buf = (uint8_t *)xmap(CHUNK_SIZE);
        if (buf) {
            long rn = bare_read(mfd, buf, CHUNK_SIZE);
            if (rn > 0) {
                sig_t hook_sigs[] = {
                    {{OX('f'),OX('r'),OX('i'),OX('d'),OX('a'),0},"frida",15},
                    {{OX('x'),OX('p'),OX('o'),OX('s'),OX('e'),OX('d'),0},"xposed",15},
                    {{OX('s'),OX('u'),OX('b'),OX('s'),OX('t'),OX('r'),OX('a'),OX('t'),OX('e'),0},"substrate",15},
                };
                for (long i=0; i+4<rn; i++) {
                    for (int s=0; s<3; s++) {
                        size_t sl = strlen(hook_sigs[s].xor);
                        if (i+(long)sl<=rn && mx(buf+i,(size_t)(rn-i),hook_sigs[s].xor,sl)) {
                            snprintf(det+strlen(det),dsz-strlen(det),"L7-SELF: hook-%s\n",hook_sigs[s].label);
                            sc += hook_sigs[s].score;
                            break;
                        }
                    }
                }
            }
            bare_munmap(buf, CHUNK_SIZE);
        }
        bare_close(mfd);
    }

    /* Verify no tracer attached */
    int sfd = (int)bare_open("/proc/self/status", 0, 0);
    if (sfd >= 0) {
        char st[1024];
        long rn = bare_read(sfd, st, sizeof(st)-1);
        if (rn > 0) {
            st[rn] = 0;
            const char *tp = strstr(st, "TracerPid:");
            if (tp) {
                int tpid = 0;
                if (sscanf(tp, "TracerPid:%d", &tpid) == 1 && tpid > 0) {
                    snprintf(det+strlen(det),dsz-strlen(det),"L7-SELF: traced pid=%d\n", tpid);
                    sc += 20;
                }
            }
        }
        bare_close(sfd);
    }
    return sc;
}

/* ==================================================================
 * Plugin lifecycle
 * ================================================================ */
static int init(const void *rule, size_t len) { (void)rule; (void)len; return 0; }

static int detect(const plugin_context_t *ctx, plugin_result_t *out) {
    (void)ctx;
    uint64_t t0 = bare_rdtsc();
    out->detail[0] = '\0';

    int s1 = check_boot_partition(out->detail, sizeof(out->detail));
    int s2 = check_vendor_partition(out->detail, sizeof(out->detail));
    int s3 = check_recovery_partition(out->detail, sizeof(out->detail));
    int s4 = check_secure_storage(out->detail, sizeof(out->detail));
    int s5 = check_logical_partitions(out->detail, sizeof(out->detail));
    int s6 = check_kernel_logs(out->detail, sizeof(out->detail));
    int s7 = self_check(out->detail, sizeof(out->detail));
    int total = s1 + s2 + s3 + s4 + s5 + s6 + s7;

    uint64_t t1 = bare_rdtsc();
    out->detected   = total > 0 ? 1 : 0;
    out->confidence = total > 80 ? 98U :
                      total > 50 ? 90U :
                      total > 30 ? 75U :
                      total > 10 ? 50U : 10U;
    out->risk_score = total > 100 ? 100U : (uint32_t)total;
    out->cost_ns    = (t1 - t0) * 10U;
    return (int)out->risk_score;
}

static void cleanup(void) {}

PLUGIN_REGISTER(LAYER_ID_TEE, "l7_tee", "3.1.0",
                "L7: firmware/partition integrity + TEE + logical vols + self-check",
                init, detect, cleanup)
