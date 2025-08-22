#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <omp.h>

typedef struct {
    uint64_t error, warn, info, debug, other;
} Counts;

static void die(const char* msg) {
    perror(msg);
    exit(1);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <logfile> [num_threads]\n", argv[0]);
        return 1;
    }

    const char* path = argv[1];
    if (argc >= 3) {
        int t = atoi(argv[2]);
        if (t > 0) omp_set_num_threads(t);
    }

    FILE* fp = fopen(path, "rb");
    if (!fp) die("open");

    if (fseek(fp, 0, SEEK_END) != 0) die("fseek");
    long fsize_long = ftell(fp);
    if (fsize_long < 0) die("ftell");
    size_t fsize = (size_t)fsize_long;
    if (fseek(fp, 0, SEEK_SET) != 0) die("fseek");

    char* data = (char*)malloc(fsize + 1);
    if (!data) die("malloc");
    size_t rd = fread(data, 1, fsize, fp);
    fclose(fp);
    if (rd != fsize) die("fread");
    data[fsize] = '\0';

    // Build line starts
    size_t cap = 1024, nlines = 0;
    size_t* starts = (size_t*)malloc(cap * sizeof(size_t));
    if (!starts) die("malloc");
    if (fsize > 0) {
        starts[nlines++] = 0;
        for (size_t i = 0; i < fsize; ++i) {
            if (data[i] == '\n') {
                if (i + 1 < fsize) {
                    if (nlines == cap) {
                        cap *= 2;
                        starts = (size_t*)realloc(starts, cap * sizeof(size_t));
                        if (!starts) die("realloc");
                    }
                    starts[nlines++] = i + 1;
                }
            }
        }
    }

    double t0 = omp_get_wtime();
    Counts total = {0,0,0,0,0};

    #pragma omp parallel
    {
        Counts local = {0,0,0,0,0};

        #pragma omp for schedule(static)
        for (size_t li = 0; li < nlines; ++li) {
            size_t start = starts[li];
            size_t end = (li + 1 < nlines) ? starts[li + 1] - 1 : fsize;
            if (end > start && data[end - 1] == '\r') end--;

            char saved = data[end];
            data[end] = '\0';
            char* line = &data[start];

            if (strstr(line, "ERROR") || strstr(line, "FATAL") || strstr(line, "Err")) {
                local.error++;
            } else if (strstr(line, "WARN")) {
                local.warn++;
            } else if (strstr(line, "INFO")) {
                local.info++;
            } else if (strstr(line, "DEBUG")) {
                local.debug++;
            } else {
                local.other++;
            }

            data[end] = saved;
        }

        #pragma omp atomic
        total.error += local.error;
        #pragma omp atomic
        total.warn += local.warn;
        #pragma omp atomic
        total.info += local.info;
        #pragma omp atomic
        total.debug += local.debug;
        #pragma omp atomic
        total.other += local.other;
    }

    double t1 = omp_get_wtime();
    double secs = t1 - t0;
    double mb = fsize / (1024.0 * 1024.0);
    double thr = (secs > 0) ? (mb / secs) : 0.0;

    int threads = 1;
    #pragma omp parallel
    {
        #pragma omp master
        threads = omp_get_num_threads();
    }

    printf("File: %s (%.2f MB), Lines: %zu\\n", path, mb, nlines);
    printf("Threads: %d\\n", threads);
    printf("Counts -> ERROR: %" PRIu64 ", WARNING: %" PRIu64 ", INFO: %" PRIu64 ", DEBUG: %" PRIu64 ", OTHER: %" PRIu64 "\\n",
           total.error, total.warn, total.info, total.debug, total.other);
    printf("Time: %.6f s | Throughput: %.2f MB/s\\n", secs, thr);

    free(starts);
    free(data);
    return 0;
}
