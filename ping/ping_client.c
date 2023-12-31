
#include <stdio.h>        /* standard input/output library functions */
#include <stdlib.h>        /* standard library definitions (macros) */
#include <unistd.h>        /* standard symbolic constants and types */
#include <string.h>        /* string operations (strncpy, memset..) */
#include <sys/time.h>

#include <sys/epoll.h>    /* epoll */
#include <sys/socket.h>    /* sockets operations */
#include <sys/un.h>        /* definitions for UNIX domain sockets */
#include <errno.h>

#define TIMEOUT 1
#define TTL 0x05


int is_program_already_running(char* lock_file) {
    if (access(lock_file, F_OK) != -1) {
        return 1;  // Lock file exists; program is already running
    }
    return 0;  // Lock file doesn't exist; program is not running
}

void create_lock(char* lock_file) {
    FILE* file = fopen(lock_file, "w");
    if (file) {
        fprintf(file, "%d", getpid());
        fclose(file);
    }
}

void release_lock(char* lock_file) {
    remove(lock_file);
}


/**
 * Entry point for the MIP ping_client program.
 *
 * This function handles the command-line arguments, creates a Unix domain socket,
 * sends a MIP PING packet containing a message to a specified destination host,
 * and prints the ping server's response along with roundtrip time.
 *
 * argc: The number of command-line arguments.
 * argv: An array of command-line argument strings.
 *
 * Returns 0 on success, or an error code on failure.
 */
int main(int argc, char *argv[]) {
//    int min_microseconds = 100000;
//    int max_microseconds = 1000000;
//    int random_microseconds = (rand() % (max_microseconds - min_microseconds + 1)) + min_microseconds;
//    usleep(random_microseconds);
//    usleep(10000);  // Sleep for 10ms


    int opt, hflag = 0;
    struct sockaddr_un addr;
    int sd, rc;
    char buf[256];
    char *err;
    while ((opt = getopt(argc, argv, "h")) != -1) {
        if (opt == 'h') {
            hflag = 1;
        }
    }

    //errno - number of last error
    errno = 0;
    long mip_addr = strtol(argv[argc - 2], &err, 10);
    if (hflag || errno || err == argv[argc - 2] || mip_addr > 254 || mip_addr < 0 || argc < 3) {
        printf("Usage: %s "
               "[-h] prints help and exits the program <socket_lower> <destination_host> <message> ", argv[0]);
        return errno || 0;
    }
    char ping[] = "PING: ";
    memset(buf, 0, sizeof(buf));

    strcat(ping, argv[argc - 1]);
    strcpy(buf + 2, ping);
    buf[0] = (char) mip_addr;
    buf[1] = TTL;
//    buf[0] = (char) 0x02;
    char *socketLower = argv[argc - 3];
    char lock_file[50];
    strcpy(lock_file, socketLower);
    strcat(lock_file, ".lock");
    if (is_program_already_running(lock_file)){
        printf("Another instance of the program is already running. Waiting for it to finish...\n");
        // Periodically check for the lock file until it's released
        while (is_program_already_running(lock_file)) {
            usleep(100000);  // Sleep for 100ms
        }
    }
    create_lock(lock_file);
    struct timeval send_time, recv_time;
    gettimeofday(&send_time, NULL);
    sd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
    if (sd < 0) {
        perror("socket");
        release_lock(lock_file);
        exit(EXIT_FAILURE);
    }

    memset(&addr, 0, sizeof(addr));
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, socketLower, sizeof(addr.sun_path) - 1);
    struct timeval tv;
    tv.tv_sec = TIMEOUT;
    tv.tv_usec = 0;
    setsockopt(sd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof tv);
    rc = connect(sd, (struct sockaddr *) &addr, sizeof(addr));
    if (rc < 0) {
        perror("connect");
        close(sd);
        release_lock(lock_file);
        exit(EXIT_FAILURE);
    }
    printf("Client: sending Identification code (0x02) to MIP daemon\n");
    char id_buf[1];
    id_buf[0] = 0x02;

    rc = write(sd, id_buf, 1);
    if (rc < 0) {
        perror("sending id code");
        close(sd);
        exit(EXIT_FAILURE);
    }
    printf("Client: sending %s, to node with MIP address %ld\n", argv[argc-1], mip_addr);
    fflush(stdout);



    rc = write(sd, buf, 256);

    if (rc < 0) {
        perror("write");
        close(sd);
        release_lock(lock_file);
        exit(EXIT_FAILURE);
    }

    // Receive and print the server's response
    rc = read(sd, buf, sizeof(buf) - 1);
    if (rc < 0) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            release_lock(lock_file);
            perror("TIMEOUT");
        } else {
            release_lock(lock_file);
            perror("read");
        }
        close(sd);
        exit(EXIT_FAILURE);
    } else if (rc == 0) {
        release_lock(lock_file);
        printf("Server closed the connection.\n");
    } else {
        buf[rc] = '\0';  // Null-terminate the received data
        gettimeofday(&recv_time, NULL);
        long long roundtrip_time_us =
                (recv_time.tv_sec - send_time.tv_sec) * 1000000LL + (recv_time.tv_usec - send_time.tv_usec);
        printf("Received: %s from node with MIP %d\n", buf + 1, (uint8_t) buf[0]);
        printf("Roundtrip time: %lld microseconds\n", roundtrip_time_us);
    }
    release_lock(lock_file);
    close(sd);
}