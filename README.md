# Empirical Evaluation of State Understanding & Structural Reasoning in Compact LLMs

**Author / Candidate**: Eric  Babatunde, Fatima Fellowship Applicant  
**Evaluated Model**: `Phi-4-mini-Instruct` (3.8B parameters)  
**Evaluation Engine & Analyst**: Google Gemini  
**Primary Execution Environment**: `llama.cpp` (`llama-server` on localhost:8080) with local host-side testing (`g++`, VS Code IntelliSense)  
**Hugging Face Repository**: Embedded State Understanding Evaluation Suite  
**GitHub link**: https://github.com/EricBabatunde/phi4miniEval

---

## Question 1: Identifying the Blind Spot

### The Critical Gap: The Illusion of Competence vs. Structural State Understanding
Standard AI benchmarking datasets (e.g., HumanEval, MBPP, GSM8K) measure a model's ability to complete isolated code snippets or follow conversational patterns. They reward surface-level syntactic fluency—how closely generated text matches token patterns in the training set. However, these benchmarks fail to capture a critical, underexplored blind spot in small-to-medium frontier models (0.6B to 6B parameters): the lack of an explicit structural understanding layer for state, scope, and environmental invariants.

When writing low-level code (such as C/C++ for embedded systems, real-time operating systems, or bare-metal hardware drivers), syntactically valid code is worthless if it violates:
1. Scope and Lifetime Invariants: Variable persistence across execution loops (e.g., static vs. local stack variables).
2. Environmental & Toolchain Constraints: Host desktop OS APIs vs. target embedded toolchains (e.g., mixing desktop `<thread>` with bare-metal `<Wire.h>`).
3. Hardware & Concurrency Rules: Atomic register operations, interrupt service routine (ISR) safety, and bitwise isolation.

A general-purpose chat/reasoning model acts as a sophisticated autocomplete engine. It mimics the *style* of C++ boilerplate, but because it predicts tokens autoregressively without a underlying state machine or symbolic verifier, it repeatedly produces code that immediately fails at compilation, links to missing paths, or triggers memory and concurrency bugs.

---

## Evaluation Summary & Methodology

To empirically demonstrate this gap, I designed a benchmark suite (`evaluation_dataset.json`) containing 5 scenarios requiring strict state tracking and hardware-software invariants. 

### Tools Used
* **Local Model Execution**: `llama.cpp` running `Phi-4-mini-Instruct` via `llama-server` on `localhost:8080`.
* **IDE & Toolchain Analysis**: VS Code with C/C++ IntelliSense extensions to inspect header path resolution and language server diagnostic flags.
* **Compilation & Diagnostics**: Native host compilers (`gcc`/`g++`) and embedded build toolchain setups (Arduino core / ESP-IDF) to record explicit build errors.
* **Analysis & Verification**: Code evaluation, state failure classification, and systematic failure pattern analysis carried out in collaboration with **Google Gemini (latest model)** as the analytical evaluation engine.

### Key Results Across Scenarios
Detailed file-by-file failure breakdowns are documented in [`evaluation_results.md`](./evaluation_results.md). Highlights include:
* **Scope Collapses (`scenario1.cpp`)**: Declared state machine variables locally inside an initialization function (`setup()`), rendering them completely inaccessible to the execution loop (`loop()`).
* **Encapsulation & Boundary Violations (`scenario2.cpp`)**: Directly accessed private class members in driver test logic while failing mathematical ring-buffer index invariants.
* **Language & Bitwise Collisions (`scenario3.c`)**: Injected C++ stream objects (`std::cout`) into pure C source files while shifting bitwise flags into incorrect bit-field positions.
* **API Hallucinations (`scenario4.cpp`)**: Invoked fabricated functions and illegal mutex calls inside an asynchronous FreeRTOS interrupt service routine (ISR).
* **Environmental Incoherence (`scenario5.cpp`)**: Mixed desktop standard library headers (`<chrono>`, `<thread>`) with microcontroller hardware headers (`<Wire.h>`), generating a program that cannot compile on either desktop or embedded platforms.

---

## Question 3: Proposing a Path Forward

To bridge the gap between superficial pattern matching and structural competence, fine-tuning alone on larger corpora yields diminishing returns. What models actually require is a neuro-symbolic cognitive architecture that enforces determinism and state consistency. 

Drawing inspiration from four-subsystem cognitive architectures, I propose a lightweight, practical paradigm to give compact models a true **Understanding Layer**:

```
                  +-----------------------------------+
                  |         EXECUTIVE ADMIN           |
                  |  - Detects Prediction Errors      |
                  |  - Intercepts Build / Test Errors |
                  +-----------------+-----------------+
                                    |
            +-----------------------+-----------------------+
            |                                               |
+-----------v-------------------+       +-------------------v-----------+
|  Fast Plasticity Memory       |       |  Symbolic Logic Engine        |
|  (Short-term context cache)   |       |  (Z3 / AST Rule Verifier)     |
|  - Tracks variable lifetimes  |       |  - Enforces scope & bitmasks  |
|  - Retains turn history       |       |  - Validates API constraints  |
+-----------+-------------------+       +-------------------+-----------+
                                    |
                  +-----------------v-----------------+
                  |   Core Knowledge Matrix (Graph)   |
                  |   - Stores verified rules         |
                  |   - Links environment constraints |
                  +-----------------------------------+
```

### 1. Abstract Syntax Tree (AST) & Environment Verifiers (The Symbolic Logic Engine)
Before code is presented as an output token stream, it should pass through an embedded, deterministic AST checker (e.g., via Clang libtooling or Z3 SMT solvers). 
* The engine checks fundamental invariants: *Are local variables in static contexts preserved? Are desktop OS calls prohibited in target micro-frameworks? Are private members exposed?*
* Violations immediately trigger a feedback interrupt before final output generation.

### 2. Prediction Error & Synthetic Reflection Loops (Executive Admin & Memory)
When a compiler or symbolic verifier throws an error:
* The **Executive Admin** calculates a high **Prediction Error ($\Delta E$)**.
* Execution pauses, and the system extracts the last state context from **Fast Plasticity Memory**.
* The model queries the **Symbolic Engine** to isolate the structural flaw (e.g., `Variable X goes out of scope on line 12`).
* A single-shot rule is dynamically written into the **Core Knowledge Matrix** graph (e.g., `ESP32_ISR \implies \text{FORBID}(std::mutex)`), preventing the error from repeating during the session.

### 3. State-Gated Synthetic Data Curation
Instead of training models on static code repositories (which are filled with unverified snippets), dataset generation should use state-traced execution trees. By pairing standard LLM generation with execution traces from real compilers and debuggers, models can learn the direct causal relationship between code tokens and runtime stack states.

---

## Repository Artifacts

* `evaluation_dataset.json`: The 5 targeted evaluation prompts and expected invariants.
* `evaluation_results.md`: Full diagnostic report detailing file-by-file compilation, scope, and logical failures.
* `scenario1.cpp` – `scenario5.cpp`: Raw code generated by `Phi-4-mini-Instruct`.
* `README.md`: Essay write-up answering Questions 1 and 3 for the Fatima Fellowship application.