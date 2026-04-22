#pragma once
#include "testModule.hpp"

// Example test module for demonstration
class exampleTestModule : public queuesTestModule {
    bool testPassed = false;
    String testResult;
public:
    const char* name() const override { return "Example Test Module"; }
    void setup() override {
        testPassed = false;
        testResult = "";
    }
    void run() override {
        // Example test logic
        int a = 2, b = 2;
        if (a + b == 4) {
            testPassed = true;
            testResult = "2 + 2 == 4";
        } else {
            testPassed = false;
            testResult = "2 + 2 != 4";
        }
    }
    bool passed() const override { return testPassed; }
    String result() const override { return testResult; }
};
