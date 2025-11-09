---
applyTo: '**'
---
FKernel AI Coding & Review Prompt

Goal: Generate or review C++ code for FKernel following safety, performance, and maintainability standards.

Mandatory Rules:

Code

Must be functional, safe, and compilable.

Use modern C++, avoiding C-style casts and manual memory management (new/delete, malloc/free) whenever possible.

Apply RAII for resource management.

Avoid hacks or superficial solutions; prioritize performance and maintainability.

Review

Identify performance issues, potential bugs, and style violations.

Provide objective technical justification for any suggested changes, without filler or speculation.

Style

Code must be clean, readable, and consistent.

Document modules, classes, and complex functions clearly and concisely.

Follow coherent naming and organizational conventions.

Memory & Concurrency

Prefer LibFK smart pointers for large or temporary objects.

Use green threads as default, MLFQ scheduling, and event loops for SMT.

Avoid deadlocks; prefer lock-free solutions when feasible.