#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <syslog.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>

#define PORT 9000
#define DATA_FILE "/var/tmp/aesdsocketdata"
#define BUF_SIZE 1024

/* Global descriptors for signal handling cleanup */
int server_fd = -1;
int client_fd = -1;

void cleanup_and_exit(int status) {
    if (server_fd != -1) close(server_fd);
    if (client_fd != -1) close(client_fd);
    unlink(DATA_FILE);
    syslog(LOG_INFO, "Caught signal, exiting");
    closelog();
    exit(status);
}

void signal_handler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        cleanup_and_exit(0);
    }
}

/**
 * Converts the process into a daemon.
 */
void daemonize() {
    pid_t pid = fork();
    if (pid < 0) exit(-1);
    if (pid > 0) exit(0); // Parent exits

    // Create a new SID for the child process
    if (setsid() < 0) exit(-1);

    // Fork again to ensure the daemon cannot re-acquire a terminal
    pid = fork();
    if (pid < 0) exit(-1);
    if (pid > 0) exit(0);

    // Change working directory to root
    if (chdir("/") < 0) exit(-1);

    // Close standard file descriptors and redirect to /dev/null
    int dev_null = open("/dev/null", O_RDWR);
    dup2(dev_null, STDIN_FILENO);
    dup2(dev_null, STDOUT_FILENO);
    dup2(dev_null, STDERR_FILENO);
    close(dev_null);
}

int main(int argc, char *argv[]) {
    bool run_as_daemon = false;
    int opt_char;

    // Parse arguments for -d flag
    while ((opt_char = getopt(argc, argv, "d")) != -1) {
        if (opt_char == 'd') {
            run_as_daemon = true;
        }
    }

    openlog("aesdsocket", LOG_PID, LOG_USER);

    // Setup signal handling
    struct sigaction sa;
    sa.sa_handler = signal_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    // 1. Setup Socket
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) return -1;

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // 2. Bind (Requirement: bind must succeed before forking)
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        close(server_fd);
        return -1;
    }

    // 3. Daemonize if requested
    if (run_as_daemon) {
        daemonize();
    }

    if (listen(server_fd, 10) < 0) {
        cleanup_and_exit(-1);
    }

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_size = sizeof(client_addr);
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_size);
        
        if (client_fd == -1) continue;

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));
        syslog(LOG_INFO, "Accepted connection from %s", client_ip);

        FILE *fp = fopen(DATA_FILE, "a+");
        if (!fp) {
            close(client_fd);
            continue;
        }

        char *buffer = malloc(BUF_SIZE);
        ssize_t bytes_received;
        size_t current_buf_size = BUF_SIZE;
        size_t total_received = 0;

        // Packet handling logic
        while ((bytes_received = recv(client_fd, buffer + total_received, BUF_SIZE - 1, 0)) > 0) {
            total_received += bytes_received;
            buffer[total_received] = '\0';

            if (strchr(buffer, '\n')) {
                fwrite(buffer, 1, total_received, fp);
                fflush(fp);
                
                fseek(fp, 0, SEEK_SET);
                char send_buf[BUF_SIZE];
                size_t bytes_read;
                while ((bytes_read = fread(send_buf, 1, BUF_SIZE, fp)) > 0) {
                    send(client_fd, send_buf, bytes_read, 0);
                }
                break; 
            }

            current_buf_size += BUF_SIZE;
            char *new_buf = realloc(buffer, current_buf_size);
            if (!new_buf) {
                syslog(LOG_ERR, "Realloc failed");
                break;
            }
            buffer = new_buf;
        }

        free(buffer);
        fclose(fp);
        close(client_fd);
        client_fd = -1;
        syslog(LOG_INFO, "Closed connection from %s", client_ip);
    }

    return 0;
}