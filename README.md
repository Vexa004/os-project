RISC-V Kernel Implementation

This project implements a small but fully functional kernel for the RISC-V architecture, supporting multithreading with time-sharing capabilities. Developed in C++ and RISC-V assembly, the kernel provides core operating system abstractions including threads, semaphores, memory allocation and context switching. The implementation follows a library-based approach where the kernel and user applications share the same address space, making it suitable for embedded systems.

The kernel executes within a QEMU emulator simulating a RISC-V processor, with a modified xv6 operating system serving as the host environment. This educational project demonstrates fundamental OS concepts such as process scheduling, synchronization primitives, interrupt handling, and memory management in a controlled, single-processor environment.

The primary motivation behind this project was to gain deep understanding of operating system internals, particularly thread management, context switching, and interrupt handling at the hardware level. By implementing these core mechanisms from scratch using both high-level C++ and low-level assembly, I developed practical knowledge of OS design principles and RISC-V architecture.

This project was developed as the university assignment for the "Operating Systems 1" course at the University of Belgrade School of Electrical Engineering, majoring in Software Engineering. Please refer to the file for detailed assignment instructions.


