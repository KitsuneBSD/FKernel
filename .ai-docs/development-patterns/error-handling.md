# Error Handling Conventions

> AI-agent conceptual memory. Read before writing kernel code that can fail.

## Core Types

### Result<T, E>
For fallible operations. Returns either a value or an error:

```cpp
Result<Page*, Error> allocate_page();
Result<size_t, Error> read(uint64_t offset, size_t size, uint8_t* buffer);
```

### TRY Macro
Propagates errors automatically:

```cpp
Result<void, Error> initialize() {
  auto page = TRY(allocate_page());
  TRY(map_page(page, addr));
  return {};
}
```

`TRY()` uses GCC statement expressions. On error, returns immediately. On success, yields the value.

### Optional<T>
For nullable values:

```cpp
Optional<Task*> find_task(ProcessId pid);
auto task = find_task(pid);
if (task.has_value()) {
  // use task.value()
}
```

## When to Use What

| Situation | Use | Example |
|-----------|-----|---------|
| Operation can fail | `Result<T, Error>` | File read, page alloc, syscall |
| Value may not exist | `Optional<T>` | Find process, lookup path |
| Unrecoverable error | `kerror()` | Page alloc failure in critical path |
| Recoverable warning | `kwarn()` | Deprecated feature used |
| Informational | `klog()` | Init messages, state changes |
| Debug output | `kdebug()` | Hot path diagnostics |

## Rules

1. **NEVER use raw error codes** — always wrap in `Result<T, Error>`
2. **NEVER use C++ exceptions** — they are disabled (`-fno-exceptions`)
3. **NEVER use `kerror()` for recoverable errors** — it halts the CPU
4. **ALWAYS use `TRY()` to propagate errors** — don't check manually
5. **ALWAYS include error context** — `"Failed to read: offset=0x%x size=%zu"`, not just `"Failed to read"`

## Error Enum

The `Error` enum (`Include/LibFK/Core/error.h`) contains domain-agnostic codes:
- `None`, `NotFound`, `PermissionDenied`, `InvalidArgument`, `NotImplemented`
- `OutOfMemory`, `Busy`, `Timeout`, `WouldBlock`, `Interrupted`
- `NotADirectory`, `NotASymlink`, `AlreadyExists`, `NotEmpty`

For domain-specific errors, extend the pattern with custom error types.

## Key Files

| File | Role |
|------|------|
| `Include/LibFK/Core/result.h` | Result<T, E> + TRY macro |
| `Include/LibFK/Core/error.h` | Error enum |
| `Include/LibFK/Memory/optional.h` | Optional<T> |
| `Include/LibFK/Core/assertions.h` | ASSERT for debug checks |
