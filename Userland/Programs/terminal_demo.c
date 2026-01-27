#include <stdio.h>
#include <unistd.h>
#include <sys/syscall.h>

// Syscall numbers for terminal management
#define SYS_TTY_CREATE  500
#define SYS_TTY_DELETE   501
#define SYS_TTY_LIST     502

// Terminal types
#define TERMINAL_TYPE_VGA    0

// Terminal management syscalls
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
    printf("=== Terminal Management Demo ===\n");
    
    // List existing terminals
    unsigned int terminals[32];
    int count = tty_list(terminals, 32);
    printf("Available terminals: %d\n", count);
    for (int i = 0; i < count; i++) {
        printf("  Terminal %u\n", terminals[i]);
    }
    
    // Create a new terminal
    printf("\nCreating dynamic VGA terminal 'tty100'...\n");
    int result = tty_create(TERMINAL_TYPE_VGA, "100");
    if (result > 0) {
        printf("✓ Successfully created terminal with ID: %d\n", result);
        
        // Try to access the new terminal
        const char* tty_path = "/dev/tty100";
        FILE* f = fopen(tty_path, "w");
        if (f) {
            fprintf(f, "Hello from dynamically created TTY!\n");
            fclose(f);
            printf("✓ Successfully wrote to %s\n", tty_path);
        } else {
            printf("✗ Failed to open %s\n", tty_path);
        }
    } else {
        printf("✗ Failed to create terminal (error: %d)\n", result);
    }
    
    // Show updated terminal list
    printf("\nAfter creation:\n");
    count = tty_list(terminals, 32);
    printf("Available terminals: %d\n", count);
    for (int i = 0; i < count; i++) {
        printf("  Terminal %u\n", terminals[i]);
    }
    
    printf("\n=== Terminal Management Complete ===\n");
    return 0;
}