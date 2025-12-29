#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <net/if.h>

#define TRAIN_SIZE 16  // Number of packets/IOCTLs per burst

struct fake_node {
    uint64_t next;
    uint64_t prev;
    uint32_t size;
    uint8_t  payload[64];
};

void alices_wonderland_trigger(uint8_t *seed, size_t len) {
    if (len < 64) return;

    // Use fork to create two parallel trains hitting the kernel simultaneously
    // This is excellent for triggering race conditions in shared TTY/Socket structures.
    if (fork() == 0) { 
        // --- TRAIN 1: TTY & HEAP STRESS ---
        int tty_fd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
        if (tty_fd >= 0) {
            for (int i = 0; i < TRAIN_SIZE; i++) {
                struct fake_node node;
                // Mutation: offset the pointer slightly in each packet of the train
                node.next = (uint64_t)&node + (i * 8);
                node.prev = (uint64_t)&node;
                
                struct iovec iov[2];
                iov[0].iov_base = seed + (i % 32); 
                iov[0].iov_len  = *(size_t*)(seed + 24) + i; // Shifting lengths
                iov[1].iov_base = &node;
                iov[1].iov_len  = sizeof(node);

                writev(tty_fd, iov, 2);
                
                // Trigger TTY lookup logic
                ioctl(tty_fd, 0x5407, &node); 
            }
            close(tty_fd);
        }
        _exit(0); // Child process exits after burst
    }

    // --- TRAIN 2: NETWORKING OVERFLOW ---
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd >= 0) {
        for (int i = 0; i < TRAIN_SIZE; i++) {
            struct ifconf ifc;
            uint8_t small_buffer[32];

            // Toggle between massive "lie" and small valid lengths
            ifc.ifc_len = (i % 2 == 0) ? 0x10000 : (int)seed[2];
            ifc.ifc_buf = (char *)small_buffer;

            // Blast the networking layer with interface queries
            ioctl(sock_fd, 0x8912, &ifc);
        }
        close(sock_fd);
    }
}
