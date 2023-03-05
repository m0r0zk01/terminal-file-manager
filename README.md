# terminal-file-manager

Project for HSE CAOS course

It's a simple file manager written in C that runs in terminal and allows you to traverse directories, copy, paste, cut and delete files

It works best on Linux, but also seems to work on MacOS

<img width="1010" alt="image" src="https://user-images.githubusercontent.com/47718803/222934473-52b1485b-45cf-4796-968f-ddcf266fc6f6.png">

# Installation

```
git clone https://github.com/m0r0zk01/terminal-file-manager.git
cd terminal-file-manager
mkdir build && cd build
cmake .. && make fm
mv fm .. && cd ..
```

The `fm` executable will appear in root directory. It's important that it's located near [extensions](./extensions) so that dynamically loaded extensions can work(e.g. opening .txt files in vim by pressing Enter). Note that you should manually generate corresponding .so files as described in the next section. TL;DR run `./build_so.sh vim.c gzip.c` in [extensions](./extensions) directory

# Extensions

You can create your own handlers for different file types:
1. Add `<filename>.c` file with `void openFile(const char *path)` function definition to [extensions](./extensions) folder. See [vim.c](./extensions/vim.c) for reference
2. Run `./build_so.sh <filename.c>`. This will create `<filename>.so` shared object. You should also do it with existing [vim.c](./extensions/vim.c) and others files bc there are no .so for them in the repo
3. Add new line to the [config](./extensions/config.txt)
4. Now if you press Enter on a file with added extension, it will be handled by your function
5. Unfortunatelly, the file type recognition is very dumb now. You specify an extension in the config and only files matching `*.<extension>` are handled:(
