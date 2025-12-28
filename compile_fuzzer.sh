# 1. Build the library
gcc -fPIC -shared fuzz_logic.c -o libfuzz.so

# 2. Build the orchestrator
gcc main.c -o fuzzer_runner

# 3. Run with LD_PRELOAD
LD_PRELOAD=./libfuzz.so ./fuzzer_runner > fuzz_output.log 2>&1
