# FKernel: How Contribute with FKernel Project 

Welcome to **_FKernel Development Ecossystem_**! 

This Guide explains how to contribute code, report issues, and collaborate effectively with other developers. 

FKernel is a freestanding, modular kernel built with **modern C++** and Lua-based build automation. 

Contributions are always welcome - from kernel, drivers and subsystems to documentation and tooling. 

## General Workflow 

1. Fork the repository

Create your own fork of the [FKernel repository](https://github.com/KitsuneBSD/FKernel)

2. Create a new branch 

```bash
git checkout -b feature/my-feature
```

3. Make your changes 

Keep commmits **small** and **atomic**. 

Each commit should represent one logical change

4. Test your build 

Build and run your version of FKernel locally using the instructions on [Docs/How Building FKernel](https://github.com/KitsuneBSD/FKernel/blob/main/Docs/how_build_fKernel.md)

```bash
xmake f --mode=debug --toolchain=FKernel_Compile #important!
xmake && xmake run
```

Check that kernel boots with successfully and the feature behaves as intended

5. Commit with a clear message

Follow the commit message convention: 

```git commit 
(conventional commit): short description of change 

Longer explanation if necessary (what change and why). 
```

Example:

```git commit 
(feat): Implement vnode reference counting

Added ref/unref methods and adjusted open/close logic for all filesystem drivers.
```

6. Push and open a pull request 

```bash
git push origin feature/my-feature
```

Then open a Pull Request (PR) on GitHub with a concise summary of what your changes do.

## Code Style and Structure 

- **Language:** Modern C++20 and C for low-level or bootstrapping parts.

- **Headers:** All headers live under Include/Kernel/….

- **Source files:** C/C++ files go in Src/. Assembly in Src/Arch/<arch>/.

- **Formatting:** Prefer consistent indentation (4 spaces, no tabs).

- Naming:

* Functions and variables: snake_case

* Types and structs: PascalCase

* Macros and constants: UPPER_CASE

* Comments: Use // for short notes, /* ... */ for multi-line explanations.

### Add TODO or FIXME Comments 

Development in a kernel project often requires leaving notes for future improvements or unresolved issues.
FKernel follows a simple and visible convention for in-source annotations:

**TODO**

Used for planned improvements, incomplete implementations, or optimizations.

```cpp
// TODO: Implement buffer cache for block devices
int AtaBlockDevice::read(VNode *vnode, void *buffer, size_t size, size_t offset)
{
    // current implementation is direct read; no caching yet
}
``` 

FIXME

Used for known bugs, technical debt, or temporary workarounds that must be resolved later.

```cpp
// FIXME: This should not use a global lock once SMP is stable
spin_lock(&ata_global_lock);
``` 

#### Guidelines

Always write a short, descriptive reason after TODO or FIXME.

Optionally include your initials or GitHub username for tracking:

```cpp
// TODO(kitsune): support 64-bit inode numbers
```

Never leave ambiguous markers like // TODO: fix later. Be explicit.

## Documentation and Contributions 

Documentation is stored in the ***Docs/*** directory.

When writing docs:

- Use Markdown (.md) format.
- Keep line width under 100 characters.
- Include examples and command snippets when possible.
- Cross-link related documents using relative paths.

## ⚖️ Licensing

All contributions to FKernel fall under the same license as the repository —
typically the BSD 3-Clause License (see [LICENSE file](https://github.com/KitsuneBSD/FKernel/blob/main/LICENSE)).

Ensure that:
- You have the right to submit the code.
- It doesn’t include external, incompatible-licensed components.

## Code Reviews & Merging

Pull Requests will be reviewed for:

- Code clarity and maintainability

- Compliance with the style and architectural guidelines

- Clear commit messages and documentation updates

- Small, frequent PRs are preferred over large, monolithic ones.

Once approved, maintainers will merge your contribution into the main branch.

## 🏁 Final Notes

Kernel development is complex, but collaboration makes it fun and powerful.

Whether you’re fixing a bug, documenting a subsystem, or adding a new driver, each contribution helps shape FKernel into a cleaner and more modular system.

Keep exploring, keep commenting, and don’t be afraid to leave a well-placed TODO — it’s how great kernels grow.