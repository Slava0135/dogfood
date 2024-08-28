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
    if (mkdir(g_workspace, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
        if(errno == EEXIST) {
            DPRINTF("WARNING: %s exists\n", g_workspace);
        } else {
            DPRINTF("ERROR: %s\n", strerror(errno));
            return 3;
        }
    } else {
        fprintf(stdout, "> Make dir %s\n", g_workspace);
    }

    for (int i = 0; i < NR_BUF; ++i) {
        char *buf = (char*)align_alloc(SIZE_PER_BUF);
        if (!buf) {
            DPRINTF("ERROR: malloc failure\n");
            return 4;
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

    fprintf(stdout, "> Done!\n");
	return 0;
}
