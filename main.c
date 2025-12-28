#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <fcntl.h>
#include <unistd.h>

// Weak symbol: Resolved if fuzz_logic.so is LD_PRELOADed
__attribute__((weak)) void alices_wonderland_trigger(uint8_t *seed, size_t len);

// Use hardware entropy to generate a new seed
uint64_t get_hw_entropy() {
    uint64_t val;
    asm volatile("rdrand %0" : "=r" (val));
    return val;
}

int main(int argc, char *argv[]) {
    uint8_t seed_buffer[128];
    int iteration = 0;

    printf("[*] Alice Fuzzer Orchestrator Started...\n");

    while (1) {
        // 1. Generate new seed locally
        for (int i = 0; i < sizeof(seed_buffer) / 8; i++) {
            ((uint64_t*)seed_buffer)[i] = get_hw_entropy();
        }

        // 2. Persist seed to disk (Backtrack capability)
        int fd = open("current_seed.bin", O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if (fd >= 0) {
            write(fd, seed_buffer, sizeof(seed_buffer));
            close(fd);
        }

        printf("[Iter %d] Testing seed: 0x%02x%02x...\n", iteration++, seed_buffer[0], seed_buffer[1]);

        // 3. Invoke the Fuzz Logic
        if (alices_wonderland_trigger) {
            alices_wonderland_trigger(seed_buffer, sizeof(seed_buffer));
        } else {
            printf("[!] Error: No fuzz_logic.so detected. Use LD_PRELOAD.\n");
            return 1;
        }

        // Optional: Small delay to let dmesg catch up
        usleep(10000); 
    }
    return 0;
}
