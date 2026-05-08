# richmd — Markdown Editor

A live-styling markdown editor built with wxWidgets. Type markdown and see headings, bold, italic, inline code, blockquotes, lists, and fenced code blocks styled in real time.

## Features

- **Live auto-styling** — `#` headings, `*italic*`, `**bold**`, `` `code` ``, `>` blockquotes, `` ``` `` fenced code blocks, and lists are styled as you type
- **Proportional fonts** for body and headings; monospace reserved for code
- **Dark theme** with carefully chosen colours
- **File menu** — New, Open, Save, Save As, recent files history, Exit
- **Edit menu** — Undo, Redo, Cut, Copy, Paste, Select All
- **Non-destructive** — markdown syntax characters remain visible, styling is applied on top

## Building

```bash
cmake --preset default
cmake --build --preset default
./build/richmd
```

Builds with clang/clang++ via Ninja (see `CMakePresets.json`). Requires wxWidgets 3.2.8+; if not found on the system, CMake fetches and builds it automatically.

On Windows, the build links libc++ and libunwind statically — the resulting executable does not require any DLLs. Per-Monitor V2 HiDPI is enabled via `src/main.exe.manifest` and `src/resources.rc`. On macOS, `src/Info.plist` enables Retina rendering via `NSHighResolutionCapable`.

### VSCode / VSCodium

Recommended extensions:
- **CMake** (Microsoft) — automatic preset detection, required by `launch.json`
- **lldb-dap** (LLVM) — debugging
- **clangd** (LLVM) — code navigation

## License

GPLv3 — see [LICENSE](LICENSE).
