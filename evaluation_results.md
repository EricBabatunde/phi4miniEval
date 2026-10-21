# Empirical Evaluation Report: State Understanding in Phi-4-mini-Instruct (3.8B)

## Executive Summary

This report documents a systematic evaluation of **Phi-4-mini-Instruct** (3.8B parameters) on 5 target scenarios designed to test hardware-software invariants, state persistence, timing loops, and low-level C/C++ semantics. 

The core thesis tested is that standard auto-regressive language models lack an explicit **structural understanding layer**. While they produce outputs that closely match the syntactic *style* of embedded code, they repeatedly fail when maintaining logical, spatial, scope, and target-environment invariants.

---

## Global Findings & Toolchain Environment Notes

Across all generated artifacts, two major failure classes emerged:

1. **Environmental & Structural Context Hallucination (VS Code & Toolchain Level)**:
   - **Missing Header & Path Resolution**: Language servers (like VS Code's C/C++ IntelliSense) and host compilers (`gcc`/`g++`) immediately flag errors such as `cannot open source file "Arduino.h"` or `cannot open source file "freertos/FreeRTOS.h"`. 
   - **Why this occurs**: The model outputs mixed environment code without specifying host vs. target constraints. It imports bare-metal Arduino headers alongside desktop OS headers (`<chrono>`, `<thread>`), creating an incoherent program that fails on both native desktop host compilers and specialized embedded toolchains (e.g., ESP-IDF, Arduino IDE, or PlatformIO).

2. **Semantic & State Violations (Language & Logic Level)**:
   - The model frequently violates basic C++ variable scoping, class access specifiers (`private` vs. `public`), C vs. C++ compilation boundaries, and thread/ISR execution safety.

---

## Detailed File-by-File Failure Analysis

### 1. `scenario1.cpp` — Non-Blocking State Machine
* **Target Objective**: Create a non-blocking solenoid valve state machine using static/global state variables and `millis()` time tracking.
* **Observed Code Behavior**:
  ```cpp
  void setup() {
    Serial.begin(9600);
    ValveState currentState = IDLE;
    unsigned long lastMillis = 0;
    unsigned long cycleMillis = 0;
    unsigned long interval = 0;
  }

  void loop() {
    if (updateValveState(currentState, lastMillis, cycleMillis, 1000, 500)) { ... }
  }
  ```
* **Failure Analysis**:
  * **Scope & Lifetime Violation**: The variables `currentState`, `lastMillis`, etc., are declared *locally* inside `setup()`. They go out of scope as soon as `setup()` finishes.
  * **Compilation Error**: The `loop()` function attempts to pass `currentState` and `lastMillis` into `updateValveState()`. `g++` and the Arduino IDE throw immediate `error: ‘currentState’ was not declared in this scope` failures.
  * **Underlying Blind Spot**: The model understands the token pattern of `setup()` and `loop()`, but lacks a persistent spatial state model of C++ stack frames and variable lifetimes across execution contexts.

---

### 2. `scenario2.cpp` — Circular Buffer Full/Empty Boundary
* **Target Objective**: Implement a fixed-size circular buffer without an extra counter/flag, distinguishing full vs. empty solely via index math.
* **Observed Code Behavior**:
  ```cpp
  class CircularBuffer {
  private:
      std::array<uint8_t, N> buffer;
      size_t head;
      size_t tail;
      size_t count; // Model added a explicit counter flag despite prompt restrictions
  };

  int main() {
      // ...
      std::cout << "Popped: " << ... << " at head: " << static_cast<int>(cb.head) << std::endl;
  }
  ```
* **Failure Analysis**:
  * **Encapsulation Access Violation**: In `main()`, the model attempts to read `cb.head` directly. Because `head` is declared in the private section of the class, compilation fails with: `error: ‘size_t CircularBuffer<16>::head’ is private within this context`.
  * **Prompt Invariant Violation**: The prompt explicitly asked to resolve full vs. empty *without an extra boolean/count flag*. The model injected `size_t count` anyway, demonstrating an inability to execute complex index-bound mathematical invariants.

---

### 3. `scenario3.c` — Register Bit-Masking
* **Target Objective**: Modify bits `[3:2]` and `[0]` of a target control register in C without modifying surrounding bit registers.
* **Observed Code Behavior**:
  ```c
  // Saved as scenario3.c (C Source File)
  #include <stdint.h>

  void configure_motor_channel(uint8_t *control_reg, uint8_t mode, uint8_t enable) {
      if (control_reg == NULL || control_reg == NULL) { // Tautological check
          return; 
      }
      control_reg[0] = (control_reg[0] & 0xC7) | (mode << 3);  // Incorrect shift position
      control_reg[0] = (control_reg[0] & 0x7F) | (enable << 0);
  }

  int main() {
      // ...
      std::cout << "Control Register... " << std::hex << control_reg << std::endl; // C++ in C file
  }
  ```
* **Failure Analysis**:
  * **Language Boundary Collision**: The file is extension-typed as pure C (`.c`), but `main()` uses C++ stream objects (`std::cout`, `std::hex`). A standard C compiler (`gcc`) rejects this instantly.
  * **Bitwise Arithmetic Error**: To place `mode` (values 0–3) into bits `[3:2]`, the left-shift offset must be `2` (`mode << 2`). The model shifted by `3` (`mode << 3`), pushing bits into positions `[4:3]` and corrupting adjacent register states.
  * **Redundant Logic**: Contains duplicate checks (`control_reg == NULL || control_reg == NULL`).

---

### 4. `scenario4.cpp` — Asynchronous ISR Concurrency
* **Target Objective**: Create an ESP32 FreeRTOS UART ISR and task handler using non-blocking, ISR-safe primitives without mutexes.
* **Observed Code Behavior**:
  ```cpp
  #include <freertos/FreeRTOS.h>
  #include <cstdint.h> // Non-standard C++ header wrapper in bare-metal context

  void uart_task(void *pvParameters) {
      // ...
      xSemaphoreTakeFromISR(buffer_sem, &eTaskSentinel, portMAX_DELAY); // Hallucinated API call
  }
  ```
* **Failure Analysis**:
  * **Missing Path Resolution**: VS Code and local IDEs fail immediately on `#include <freertos/FreeRTOS.h>` and `#include <esp_intr_alloc.h>` because these paths are specific to the ESP-IDF toolchain.
  * **API Hallucination**: The model calls `xSemaphoreTakeFromISR` inside a regular task loop (rather than `xSemaphoreTake`), and invokes uninitialized symbols like `eTaskSentinel`.
  * **Architecture Incoherence**: It mixes legacy C driver function names (`uart_init`, `uart_register_handler`) that do not exist in the official ESP-IDF driver matrix, proving that it generates tokens based on historical text correlation rather than a validated API graph.

---

### 5. `scenario5.cpp` — Hardware Polling Timeout
* **Target Objective**: Poll an I2C status register for a ready bit with a strict non-blocking 50ms timeout.
* **Observed Code Behavior**:
  ```cpp
  #include <Wire.h>
  #include <chrono>
  #include <thread>

  bool initializeSensor() {
      // ...
      while (std::chrono::steady_clock::now() - startTime < std::chrono::milliseconds(50)) {
          // ...
          std::this_thread::sleep_for(std::chrono::milliseconds(1)); // Desktop OS call on microcontrollers
      }
  }
  ```
* **Failure Analysis**:
  * **Host vs. Target Environment Contradiction**: The model includes `<Wire.h>` (Arduino framework for microcontrollers) alongside `<chrono>` and `<thread>` (C++11 standard library features requiring an underlying OS scheduler).
  * **Toolchain Failure**: Microcontroller frameworks (like AVR/SAMD Arduino cores) do not provide standard `std::this_thread` implementation headers. VS Code marks these headers as missing/unresolved, and cross-compilers fail at the linking stage.
  * **Logical Illusion**: The model attempted to satisfy the prompt's "non-blocking timeout" constraint by importing desktop OS primitives, ignoring the fundamental execution environment of the embedded target.

---

## Conclusion

This empirical evaluation confirms that **Phi-4-mini-Instruct (3.8B)** relies on statistical pattern matching that produces superficially convincing C/C++ code. However, without a dedicated **neuro-symbolic or structural verification layer**, it routinely produces outputs that fail at the compilation, scoping, bitwise, and environment levels.