#include <Kernel/Scheduler/Task/TaskList.h>

extern "C" void idle_task_entry(){
    while (true){
        __asm__ volatile("hlt");
    }
}