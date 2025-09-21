# Configure LSP 

Probably is your first time seeing the [xmake](https://xmake.io) used on Osdev. 

On FKernel we need generate a `compile_commands.json` file to run the lsp clangd properly.

## Xmake: How to generate this LSP File

The Xmake has a internal cli to generate the `compile_commands.json` file

```bash 
xmake project -K compile_commands
``` 