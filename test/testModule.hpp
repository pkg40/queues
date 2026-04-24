#pragma once
#include <Arduino.h>

// Abstract base class for a test module
class queuesTestModule {
public:
    virtual const char* name() const = 0;
    virtual void setup() {}
    virtual void run() = 0;
    virtual bool passed() const = 0;
    virtual String result() const = 0;
    virtual ~queuesTestModule() {}
};
