#include <fk_user.h>

int main() {
    print("FKernel Native Init Process (PID 1)\n");

    while (1) {
        print("Init: Launching shell...\n");
        int pid = sys_fork();
        if (pid == 0) {
            // Child: execute shell
            char* argv[] = {"/bin/sh", NULL};
            char* envp[] = {"PATH=/bin:/sbin", "PS1=# ", NULL};
            sys_execve("/bin/sh", argv, envp);
            
            // If exec fails
            print("Init: Failed to launch /bin/sh\n");
            sys_exit(1);
        } else if (pid > 0) {
            // Parent: wait for shell to exit
            int status;
            sys_wait4(pid, &status, 0, NULL);
            print("Init: Shell exited, restarting in 3 seconds...\n");
            // Simple busy loop for delay
            for (volatile int i = 0; i < 50000000; i++);
        } else {
            print("Init: Fork failed\n");
            for (volatile int i = 0; i < 10000000; i++);
        }
    }

    return 0;
}
