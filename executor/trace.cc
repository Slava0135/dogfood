#include <vector>
#include <string>

#include "executor.h"

struct Trace {
    int index;
    int ret_code;
    int err;
};

std::vector<Trace> traces;

void append_trace_impl(int index, int ret_code, int err) {
    traces.push_back(Trace{index, ret_code, err});
}

void dump_trace() {
    fprintf(stdout, "> Dumping trace...\n");

    FILE *trace_dump_fp = fopen("./trace.dat", "w");
    if (!trace_dump_fp) {
        DPRINTF("ERROR: when opening trace dump file\n");
        return;
    }

    fprintf(trace_dump_fp, "Index,ReturnCode,Errno\n");
    for (const Trace& t : traces) {
        fprintf(trace_dump_fp, "%4d,%8d,%s(%d)\n", t.index, t.ret_code, strerror(t.err), t.err);
    }

    fclose(trace_dump_fp);
}

