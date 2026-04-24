#include <Arduino.h>
#include "testBench.hpp"
#include "../test/testModule.hpp"


#include "../test/exampleTestModule.hpp"

queuesTestBench bench;

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n==== Queues Test Bench ====");


    // Register test modules
    bench.add(new exampleTestModule());

    bench.runAll();
    bench.printResults();
}

void loop() {
    // Optionally rerun or interact
}
