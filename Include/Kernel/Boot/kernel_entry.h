#pragma once

/**
 * @brief Unified kernel entry point
 * Called after BootInfo is initialized (either from Multiboot2 or UEFI)
 */
extern "C" void kernel_entry();
