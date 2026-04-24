#pragma once
#include <vector>
#include <Arduino.h>
#include "../test/testModule.hpp"

// Test bench for running and collecting results from test modules
class queuesTestBench {
    std::vector<queuesTestModule*> modules;
    size_t passedCount = 0;
    size_t failedCount = 0;
public:
    void add(queuesTestModule* module) {
        modules.push_back(module);
    }
    void runAll() {
        passedCount = 0;
        failedCount = 0;
        for (auto m : modules) {
            m->setup();
            m->run();
            if (m->passed()) passedCount++;
            else failedCount++;
        }
    }
    void printResults(Stream& out = Serial) {
        out.println("\n==== Queues Test Bench Results ====");
        for (auto m : modules) {
            out.printf("[%s] %s\n", m->passed() ? "PASS" : "FAIL", m->name());
            out.println(m->result());
        }
        out.printf("\nSummary: %d passed, %d failed, %d total\n", (int)passedCount, (int)failedCount, (int)modules.size());
    }
    size_t total() const { return modules.size(); }
    size_t passed() const { return passedCount; }
    size_t failed() const { return failedCount; }
};
