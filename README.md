# terminal-file-manager

Project for HSE CAOS course

It's a simple file manager written in C that runs in terminal and allows you to traverse directories, copy, paste, cut and delete files

It works best on Linux, but also seems to work on MacOS

# Installation

```
git clone https://github.com/m0r0zk01/terminal-file-manager.git
cd terminal-file-manager
mkdir build
cd build
cmake ..
make fm
mv fm ..
cd ..
./fm
```

The `fm` executable will appear in root directory. It's important that it's located near [extensions](./extensions) so that dynamically loaded extensions can work(e.g. opening .txt files in vim by pressing Enter)

# Extensions

You can create your own handlers for different file types:
1. Add `<filename>.c` file with `void openFile(const char *path)` function definition to [extensions](./extensions) folder. See [vim.c](./extensions/vim.c) for reference
2. Run `cd extensions && ./build_so.sh <filename.c>`. This will create `<filename>.so` shared object
3. Add new line to the [config](./extensions/config.txt)
4. Now if you press Enter on a file with added extension, it will be handled by your function
5. Unfortunatelly, the file type recognition is very dumb now. You specify an extension in the config and only files matching `*.<extension>` are handled:(
