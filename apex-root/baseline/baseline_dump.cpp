#include <cstdio>
#include <cstring>
#include <cstdlib>

struct CleanBaseline {
    char kernel_ver[256];
    char cmdline[512];
    char build_prop[1024];
};

static void read_file_or_die(const char *path, char *buf, size_t len) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "FAIL: cannot open %s\n", path);
        buf[0] = '\0';
        return;
    }
    size_t n = fread(buf, 1, len - 1, f);
    buf[n] = '\0';
    while (n > 0 && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) {
        buf[--n] = '\0';
    }
    fclose(f);
    printf("OK: %s (%zu bytes)\n", path, n);
}

int main(int argc, char **argv) {
    const char *outpath = "clean_baseline.bin";
    if (argc > 1) outpath = argv[1];

    CleanBaseline bl = {};
    read_file_or_die("/proc/version", bl.kernel_ver, sizeof(bl.kernel_ver));
    read_file_or_die("/proc/cmdline", bl.cmdline, sizeof(bl.cmdline));
    read_file_or_die("/system/build.prop", bl.build_prop, sizeof(bl.build_prop));

    if (bl.kernel_ver[0] == '\0' || bl.cmdline[0] == '\0' || bl.build_prop[0] == '\0') {
        fprintf(stderr, "WARNING: some baselines empty, expect incomplete spoof\n");
    }

    FILE *out = fopen(outpath, "wb");
    if (!out) {
        fprintf(stderr, "FAIL: cannot write %s\n", outpath);
        return 1;
    }
    fwrite(&bl, sizeof(bl), 1, out);
    fclose(out);
    printf("SUCCESS: baseline dumped to %s (%zu bytes)\n", outpath, sizeof(bl));
    return 0;
}
