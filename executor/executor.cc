#include "executor.h"

#include <stdlib.h>

#include "utils.h"

const char *g_workspace = nullptr;
char* g_buffers[NR_BUF];

// -----------------------------------------------

void init_executor() {
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
}

// -----------------------------------------------

int main(int argc, char *argv[]) {
    if (argc != 4) {
        DPRINTF("Usage a.out <fs> <loop-dev> <workspace>\n");
        exit(0);
    }

    const char *fs = argv[1];
    const char *dev = argv[2];
    g_workspace = argv[3];
    if (!g_workspace) {
        DPRINTF("Workspace is NULL\n");
        exit(0);
    }

    init_executor();

    fprintf(stdout, "> Begin testing syscall...\n");

    // TODO: setup

    test_syscall();

#ifdef USE_TRACE
    dump_trace();
#endif

    report_result();
    fprintf(stdout, "> ...Ending\n");
	return 0;
}
