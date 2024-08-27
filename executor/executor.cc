#include <stdlib.h>

#include "executor.h"
#include "utils.h"

const char *g_workspace = nullptr;
char* g_buffers[NR_BUF];

int main(int argc, char *argv[]) {
    if (argc != 2) {
        DPRINTF("USAGE: CMD <workspace>\n");
        return 1;
    }

    g_workspace = argv[1];
    if (!g_workspace) {
        DPRINTF("ERROR: workspace is NULL\n");
        return 2;
    }

    fprintf(stdout, "> Prepare workspace %s\n", g_workspace);
    make_dir(g_workspace);

    for (int i = 0; i < NR_BUF; ++i) {
        char *buf = (char*)align_alloc(SIZE_PER_BUF);
        if (!buf) {
            DPRINTF("Malloc failure\n");
            exit(1);
        }
        memset(buf, 'a' + i, SIZE_PER_BUF);
        g_buffers[i] = buf;
    }

    fprintf(stdout, "> Begin testing syscall...\n");
    test_syscall();

#ifdef USE_TRACE
    dump_trace();
#endif

    report_result();

    fprintf(stdout, "> ...Ending\n");
	return 0;
}
