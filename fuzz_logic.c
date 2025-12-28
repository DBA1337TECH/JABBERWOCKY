#include <stdint.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <termios.h>
#include <fcntl.h>
#include <unistd.h>
#include <net/if.h>
#include <stdlib.h>
#include <string.h>

#define TRAIN_SIZE 16

struct fake_node {
    uint64_t next;
    uint64_t prev;
    uint32_t size;
    uint32_t magic;
    uint8_t  payload[64];
};

void alices_wonderland_trigger(uint8_t *seed, size_t len) {
    if (len < 64) return;

    // Allocate Alice's "Fake Node" and buffers on the HEAP.
    // This prevents "stack smashing detected" in userland.
    struct fake_node *node = malloc(sizeof(struct fake_node));
    struct ifconf *ifc = malloc(sizeof(struct ifconf));
    char *small_buffer = malloc(32); 

    if (!node || !ifc || !small_buffer) return;

    if (fork() == 0) { 
        // --- TRAIN 1: TTY & HEAP STRESS ---
        int tty_fd = open("/dev/ptmx", O_RDWR | O_NOCTTY);
        if (tty_fd >= 0) {
            for (int i = 0; i < TRAIN_SIZE; i++) {
                // Construct the Circular List in Heap memory
                node->magic = 0x52415752; // "RAWR"
                node->next = (uint64_t)node; 
                node->prev = (uint64_t)node;
                node->size = *(uint32_t*)(seed + 16);

                struct iovec iov[2];
                iov[0].iov_base = seed + (i % 32); 
                iov[0].iov_len  = *(size_t*)(seed + 24) + i;
                iov[1].iov_base = node;
                iov[1].iov_len  = sizeof(struct fake_node);

                // Use direct syscall for writev to bypass glibc sanity checks
                syscall(20, (long)tty_fd, iov, 2); 
                
                // Trigger TTY lookup/iteration logic
                ioctl(tty_fd, 0x5407, node); // TIOCGSID
                ioctl(tty_fd, 0x541b, node); // FIONREAD
            }
            close(tty_fd);
        }
        _exit(0); 
    }

    // --- TRAIN 2: NETWORKING OVERFLOW ---
    int sock_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock_fd >= 0) {
        for (int i = 0; i < TRAIN_SIZE; i++) {
            // VECTOR: The SIOCGIFCONF Lie
            // We tell the kernel we have 64KB (0x10000), 
            // but the pointer is to our 32-byte Heap buffer.
            ifc->ifc_len = (i % 2 == 0) ? 0x10000 : (int)seed[2];
            ifc->ifc_buf = small_buffer;

            ioctl(sock_fd, 0x8912, ifc);
        }
        close(sock_fd);
    }

    // Free the heap buffers so the fuzzer doesn't OOM itself
    free(node);
    free(ifc);
    free(small_buffer);
}
