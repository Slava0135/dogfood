#include "executor.h"

struct Trace {
    int line_;
    int ret_code_;
    int errno_;
};

#define NR_TRACE_CAP 1024000

static Trace g_traces[NR_TRACE_CAP];
static int g_trace_pos = 0;

void append_trace_impl(int line, int ret_code, int err) {
    if (g_trace_pos >= NR_TRACE_CAP) {
        DPRINTF("ERROR: trace buffer overflow\n");
        return;
    }
    g_traces[g_trace_pos].line_ = line;
    g_traces[g_trace_pos].ret_code_ = ret_code;
    g_traces[g_trace_pos].errno_ = err;

    g_trace_pos += 1;
}

void dump_trace() {
    printf("> Dumping trace...\n");

    if (g_trace_pos >= NR_TRACE_CAP) {
        DPRINTF("ERROR: trace buffer overflow\n");
    }

    FILE *trace_dump_fp = fopen("./trace.dat", "w");
    if (!trace_dump_fp) {
        DPRINTF("ERROR: when opening trace dump file\n");
        return;
    }

    for (int i = 0; i < g_trace_pos; ++i) {
        fprintf(trace_dump_fp, "%d: %d %d\n", g_traces[i].line_,
                                              g_traces[i].ret_code_,
                                              g_traces[i].errno_);
    }

    fclose(trace_dump_fp);
}

