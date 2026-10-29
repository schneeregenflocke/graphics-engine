# graphics-engine

OpenGL renderer and Bullet picking world, split out of [decade](https://github.com/schneeregenflocke/decade), which embeds it as the submodule `external/graphics-engine`. Architecture and conventions stand in [AGENTS.md](AGENTS.md).

## Operations

### Build and test

Initialise Bullet once after cloning, then configure with GCC or clang; the clang line adds the `clang-tidy` target:

```sh
git submodule update --init --recursive
cmake -S . -B build -G Ninja
cmake -S . -B build -G Ninja -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++
ninja -C build
ctest --test-dir build
```

### Gates

CI runs the same commands in [build.yml](.forgejo/workflows/build.yml):

```sh
git ls-files '*.hpp' '*.cpp' | grep -v '^external/' | xargs clang-format --dry-run -Werror
cmake --build build --target sanitize-address
cmake --build build --target clang-tidy
```
