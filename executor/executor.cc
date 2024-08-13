#include "executor.h"

#include <stdlib.h>

#include "utils.h"

const char *g_workspace = NULL;
char* g_buffers[NR_BUF];


static void mount_root(const char *dev, const char *fs) {
    char cmd[1024];
    snprintf(cmd, 1024, "./mountfood %s", fs);
    const char *option = exec_command(cmd);

    DPRINTF("MOUNT ROOT %s on %s: with [%s]\n", fs, g_workspace, option);

    int kmsg_fd = open("/dev/kmsg", O_RDWR);
    if (kmsg_fd == -1) {
        DPRINTF("Open /dev/kmsg failure: [%s]\n", strerror(errno));
        return;
    }
    int l = snprintf(cmd, 1024, "\033[0;33mMOUNT OPT [ %s ]\033[0m\n", option);
    write(kmsg_fd, cmd, l);
    close(kmsg_fd);

    int status = mount(dev, g_workspace, fs, 0, option);
    if (status) {
        DPRINTF("MOUNT ROOT failure: (%s)", strerror(errno));
        exit(1);
    }

    // exec_command(cmd);
}

static void start_kdelay() {
    int kdelay_fd = open("/proc/kdelay", O_RDWR);
    if (kdelay_fd == -1) {
        DPRINTF("Open /proc/kdelay failure: [%s]\n", strerror(errno));
        return;
    }
    fprintf(stderr, "> start kdelay\n");
    write(kdelay_fd, "start", 5);
    close(kdelay_fd);
}

static void stop_kdelay() {
    int kdelay_fd = open("/proc/kdelay", O_RDWR);
    if (kdelay_fd == -1) {
        DPRINTF("Open /proc/kdelay failure: [%s]\n", strerror(errno));
        return;
    }
    write(kdelay_fd, "stop", 4);
    close(kdelay_fd);
}

// -----------------------------------------------

void init_executor() {
    bind_cpu(0);

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

static pthread_t daemon_thread;
static int running = 1;

void *flush_daemon(void *arg) {
    bind_cpu(1);
    printf("======> Daemon starts <======\n");
    while (running) {
        usleep(1000);
        do_sync(false);
        do_remount_root();
    }
}

void start_daemon() {
    pthread_create(&daemon_thread, NULL, flush_daemon, NULL);
}

void stop_daemon() {
    running = 0;
    pthread_join(daemon_thread, NULL);
}

// -----------------------------------------------

int main(int argc, char *argv[]) {
    if (argc != 4) {
        DPRINTF("Usage a.out <fs> <loop-dev> <workspace>\n");
        exit(0);
    }

    bind_cpu(0);

    const char *fs = argv[1];
    const char *dev = argv[2];
    g_workspace = argv[3];
    if (!g_workspace) {
        DPRINTF("Workspace is NULL\n");
        exit(0);
    }

    init_executor();

    fprintf(stdout, "> Begin testing syscall...\n");

    fprintf(stderr, "> To mount root ...\n");
    mount_root(dev, fs);
    fprintf(stderr, "> Mount root done...\n");

#ifdef USE_KDELAY
    start_kdelay();
    start_daemon();
#endif

    test_syscall();

#ifdef USE_KDELAY
    stop_kdelay();
    stop_daemon();
#endif

#ifdef USE_TRACE
    dump_trace();
#endif

    report_result();
    fprintf(stdout, "> ...Ending\n");
	return 0;
}
