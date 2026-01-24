#include <fk_user.h>

#define MAX_ARGS 16
#define MAX_CMD_LEN 256

int strcmp(const char* s1, const char* s2);

void parse_and_execute(char* line) {
    char* argv[MAX_ARGS];
    int argc = 0;
    char* p = line;
    
    while (*p && argc < MAX_ARGS - 1) {
        // Skip leading spaces
        while (*p == ' ') p++;
        if (!*p || *p == '\n') break;

        argv[argc++] = p;

        // Find end of token, respecting backslash for spaces
        while (*p && *p != '\n') {
            if (*p == '\\' && *(p + 1) == ' ') {
                // Shift string to remove backslash
                char* shift = p;
                while (*shift) {
                    *shift = *(shift + 1);
                    shift++;
                }
                // Now *p is the space, continue to next char
                p++; 
            } else if (*p == ' ') {
                *p = '\0';
                p++;
                break;
            } else {
                p++;
            }
        }
    }
    argv[argc] = NULL;

    if (argc == 0) return;

    // Built-in commands
    if (strcmp(argv[0], "cd") == 0) {
        if (argc > 1) sys_chdir(argv[1]);
        return;
    }

    if (strcmp(argv[0], "exit") == 0) {
        sys_exit(0);
    }

    // External commands
    int pid = sys_fork();
    if (pid == 0) {
        // Child
        if (argv[0][0] == '/' || argv[0][0] == '.') {
            sys_execve(argv[0], argv, NULL);
        } else {
            // Try /bin/ and /sbin/
            const char* prefixes[] = {"/bin/", "/sbin/", NULL};
            for (int i = 0; prefixes[i]; i++) {
                char path[MAX_CMD_LEN];
                int j = 0;
                while(prefixes[i][j]) { path[j] = prefixes[i][j]; j++; }
                int k = 0;
                while(argv[0][k] && (j+k) < MAX_CMD_LEN - 1) { path[j+k] = argv[0][k]; k++; }
                path[j+k] = '\0';
                sys_execve(path, argv, NULL);
            }
        }
        print("Shell: Command not found: ");
        print(argv[0]);
        print("\n");
        sys_exit(127);
    } else if (pid > 0) {
        // Parent
        int status;
        sys_wait4(pid, &status, 0, NULL);
    } else {
        print("Shell: fork failed\n");
    }
}

int main() {
    print("FKernel Native Shell (Space & Backslash Support)\n");
    
    char buffer[MAX_CMD_LEN];
    while(1) {
        char cwd[128];
        if (sys_getcwd(cwd, sizeof(cwd)) < 0) {
            cwd[0] = '/';
            cwd[1] = '\0';
        }
        print(cwd);
        print(" # ");
        
        int64_t n = sys_read(0, buffer, MAX_CMD_LEN - 1);
        if (n <= 0) break;
        
        // Remove trailing newline and carriage return
        while (n > 0 && (buffer[n-1] == '\n' || buffer[n-1] == '\r')) {
            n--;
        }
        buffer[n] = '\0';
        
        if (n > 0) {
            parse_and_execute(buffer);
        }
    }
    return 0;
}

// Minimal strcmp
int strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}
