# Configure LSP

If you're using clangd as your Language Server Protocol (LSP) backend in your code editor, you'll likely want it to recognize your project's include directories and compilation settings.

XMake can help you generate the proper configuration for clangd.

Open your terminal at the root of your project.

**Run the following command**:

```bash
xmake project -k compile_commands
```

This will generate a compile_commands.json file in the project directory.

📂 About compile_commands.json
This file is used by clangd to understand how each file in your project is compiled — including include paths, macros, flags, etc. Once it's present, your editor (e.g., VS Code, Neovim, etc.) can use it to provide better IntelliSense, diagnostics, and navigation.

>[!Tip]
> Make sure your editor is configured to tell clangd where to find the compile_commands.json. Most tools automatically detect it if it's in the project root.
