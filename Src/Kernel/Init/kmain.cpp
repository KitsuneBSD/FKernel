#include "LibFK/Log.h"
#include <Kernel/Arch/x86_64/Segments/Gdt.h>

#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/multiboot_interpreter.h>

#include <Kernel/Driver/SerialPort.h>

extern "C" void kmain(multiboot2::Info const& multiboot_ptr)
{

#ifdef FKERNEL_DEBUG
    Logger::Instance().SetLevel(LogLevel::TRACE);
#else
    Logger::Instance().SetLevel(LogLevel::INFO);
#endif

    auto& manager
        = gdt::Manager::Instance();
    manager.initialize();

    Log(LogLevel::INFO, "Kernel Started");

    return;
}
