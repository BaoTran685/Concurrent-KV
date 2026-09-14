// This program is to benchmark the program in the folder sophistocated_program.
// It stimulates multiple clients connecting to the data engine. We just test the data engine
//  here though without TCP connection.

#include "kv_store.h"

#include <atomic>
#include <chrono>
#include <iostream>
#include <random>
#include <thread>
#include <vector>

#include <functional>

constexpr int KEY_RANGE = 1e5;
constexpr int THREAD_COUNT = 16;
constexpr int OPERATIONS_PER_THREAD = 1e6;
constexpr int TEST_ITERATIONS = 3;

struct Benchmark_Result {
    double globallock_kv_seconds;
    double sharedlock_kv_seconds;
    double shardedsharedlock_kv_seconds;
};

void run_get(KVStore &store, std::vector<int>& random_numbers) {
    for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
        store._get(random_numbers[i]);
    }
}

void run_set(KVStore &store, std::vector<int>& random_numbers) {
    for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
        store._set(random_numbers[i], random_numbers[i]);
    }
}

void run_mix(KVStore &store, std::vector<int>& random_numbers) {
    for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
        if (random_numbers[i] % 10 < 7) { // appprox 70% reads (GETs), 30% writes (SETs)
            store._get(random_numbers[i]);
        }
        else {
            store._set(random_numbers[i], random_numbers[i]);
        }
    }
}

double benchmark(KVStore& store, std::function<void(KVStore&, std::vector<int>&)> run, std::vector<std::vector<int>> random_numbers_2D) {
    std::vector<std::thread> threads;

    auto start = std::chrono::steady_clock::now();

    for (int t = 0; t < THREAD_COUNT; ++t) {
        threads.push_back(std::thread(run, std::ref(store), std::ref(random_numbers_2D[t])));
    }

    for (std::thread& thread : threads) {
        thread.join();
    }

    auto end = std::chrono::steady_clock::now();
    double seconds = std::chrono::duration<double>(end - start).count();

    return seconds;
}

void test(std::function<void(KVStore&, std::vector<int>&)> run_function, std::string run_type) {
    int iterations = TEST_ITERATIONS;
    Benchmark_Result final_result {0, 0, 0};
    while (iterations--) {
        GlobalLock_KVStore globallock_kv;
        SharedLock_KVStore sharedlock_kv;
        ShardedSharedLock_KVStore shardedsharedlock_kv;
    
        std::vector<std::vector<int>> random_numbers_2D(THREAD_COUNT, std::vector<int>(OPERATIONS_PER_THREAD));
        for (int t = 0; t < THREAD_COUNT; ++t) {
            std::mt19937 rng(t);
            for (int i = 0; i < OPERATIONS_PER_THREAD; ++i) {
                random_numbers_2D[t][i] = rng() % KEY_RANGE;
            }
        }
    
        final_result.globallock_kv_seconds += benchmark(globallock_kv, run_function, random_numbers_2D);
        final_result.sharedlock_kv_seconds += benchmark(sharedlock_kv, run_function, random_numbers_2D);
        final_result.shardedsharedlock_kv_seconds += benchmark(shardedsharedlock_kv, run_function, random_numbers_2D);
    }

    final_result.globallock_kv_seconds /= TEST_ITERATIONS;
    final_result.sharedlock_kv_seconds /= TEST_ITERATIONS;
    final_result.shardedsharedlock_kv_seconds /= TEST_ITERATIONS;

    long long operations = (long long) THREAD_COUNT * OPERATIONS_PER_THREAD;

    std::cout << "Benchmark " << run_type << " command | Average duration and Average throughput" << std::endl;
    std::cout << "Global Lock: " << final_result.globallock_kv_seconds << "s; " << operations / final_result.globallock_kv_seconds << " ops/s" << std::endl;
    std::cout << "Shared Lock: " << final_result.sharedlock_kv_seconds << "s; " << operations / final_result.sharedlock_kv_seconds << " ops/s" << std::endl;
    std::cout << "Sharded Shared Lock: " << final_result.shardedsharedlock_kv_seconds << "s; " << operations / final_result.shardedsharedlock_kv_seconds << " ops/s" << std::endl;
}

int main() {
    test(run_get, "GET");
    std::cout << std::endl;
    test(run_set, "SET");
    std::cout << std::endl;
    test(run_mix, "MIX");

    return 0;
}

