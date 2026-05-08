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
mkdir build && cd build
cmake ..
cmake --build .
./richmd
```

Requires wxWidgets 3.2+. If not found on the system, CMake will fetch and build it automatically.

## License

GPLv3 — see [LICENSE](LICENSE).
