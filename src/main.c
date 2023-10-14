
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "../include/mipd.h"
#include <errno.h>

/**
 * The main function for the MIP daemon.
 *
 * Parses command-line arguments, initializes the MIP daemon with the provided
 * socket upper layer and MIP address, and starts the daemon.
 *
 * argc: The number of command-line arguments.
 * argv: An array of strings containing the command-line arguments.
 *
 * Returns:
 * - 0 on successful execution.
 * - An error code (non-zero) if there are issues with command-line arguments or daemon initialization.
 */
int main(int argc, char *argv[]) {
    int opt, dflag = 0, hflag = 0;

    while ((opt = getopt(argc, argv, "dh")) != -1) {
        switch (opt) {
            case 'd':
                dflag = 1;
                break;
            case 'h':
                hflag = 1;
                break;
            default:
                break;
        }
    }
    char *socketUpper = argv[argc - 2];
    char *err;
    //errno - number of last error
    errno = 0;
    long mip_addr = strtol(argv[argc - 1], &err, 10);
    //Show help, when the parameters do not fit requirements
    if (hflag || errno || err == argv[argc - 1] || mip_addr > 254 || mip_addr < 0) {
        printf("Usage: %s "
               "[-d] debugging mode "
               "[-h] prints help and exits the program <socket_upper> <MIP address [0-254]> ", argv[0]);
        return errno || 0;
    }
    if (!dflag) {
        // Redirect output to /dev/null, when debug mode is not enabled
        freopen("/dev/null", "w", stdout);
    }
    printf("Starting the MIP daemon\n");
    mipd(socketUpper, (uint8_t)mip_addr);

    return 0;

}