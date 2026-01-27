#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/syscall.h>

// Syscall numbers (definidos no kernel)
#define SYS_TTY_CREATE  500
#define SYS_TTY_DELETE   501
#define SYS_TTY_LIST     502

// Tipos de terminal
#define TERMINAL_TYPE_VGA    0
#define TERMINAL_TYPE_SERIAL 1
#define TERMINAL_TYPE_PTY     2

// Wrapper functions
int tty_create(int type, const char* name_hint) {
    return (int)syscall(SYS_TTY_CREATE, type, name_hint);
}

int tty_delete(int terminal_id) {
    return (int)syscall(SYS_TTY_DELETE, terminal_id);
}

int tty_list(unsigned int* terminal_ids, int max_count) {
    return (int)syscall(SYS_TTY_LIST, terminal_ids, max_count);
}

int main() {
    printf("=== TTY Management Test ===\n");
    
    // List existing terminals
    unsigned int terminals[16];
    int count = tty_list(terminals, 16);
    printf("Existing terminals: %d\n", count);
    for (int i = 0; i < count; i++) {
        printf("  Terminal ID: %u\n", terminals[i]);
    }
    
    // Create a new VGA terminal
    printf("\nCreating VGA terminal 'tty99'...\n");
    int tty_id = tty_create(TERMINAL_TYPE_VGA, "99");
    if (tty_id > 0) {
        printf("Successfully created terminal with ID: %d\n", tty_id);
        
        // Test the new terminal
        char tty_path[32];
        snprintf(tty_path, sizeof(tty_path), "/dev/tty99");
        
        FILE* f = fopen(tty_path, "w");
        if (f) {
            fprintf(f, "Hello from dynamically created TTY!\n");
            fclose(f);
            printf("Successfully wrote to %s\n", tty_path);
        } else {
            printf("Failed to open %s\n", tty_path);
        }
    } else {
        printf("Failed to create terminal, error: %d\n", tty_id);
    }
    
    // List terminals again
    printf("\nAfter creation:\n");
    count = tty_list(terminals, 16);
    printf("Existing terminals: %d\n", count);
    for (int i = 0; i < count; i++) {
        printf("  Terminal ID: %u\n", terminals[i]);
    }
    
    // Clean up - delete the terminal
    if (tty_id > 0) {
        printf("\nDeleting terminal %d...\n", tty_id);
        int result = tty_delete(tty_id);
        if (result == 0) {
            printf("Successfully deleted terminal\n");
        } else {
            printf("Failed to delete terminal, error: %d\n", result);
        }
    }
    
    // Final list
    printf("\nAfter deletion:\n");
    count = tty_list(terminals, 16);
    printf("Existing terminals: %d\n", count);
    for (int i = 0; i < count; i++) {
        printf("  Terminal ID: %u\n", terminals[i]);
    }
    
    printf("\n=== TTY Management Test Complete ===\n");
    return 0;
}