# Developer Guide

## Overview
The calculator adds builtins to the SharpScript interpreter and a web UI.

## Interpreter Builtins
- Scientific: system.sin, system.cos, system.tan, system.asin, system.acos, system.atan, system.log, system.ln, system.exp, system.sqrt, system.pow.
- Memory: system.store(name, value), system.recall(name), system.memclear().
- Unit conversion: system.convert(value, fromUnit, toUnit) with m, km, mi, kg, lb, C, F, K.
- History: system.history.add(x), system.history.get(), system.history.clear().

## Web UI
- Located at tools/calculator, uses highlight.js and localStorage.
- app.js implements NLP, autocomplete, i18n, shortcuts.

## Tests
- In tests/, run via bin/sharpscript.exe.

## Build
- Use MinGW gcc:
```bash
C:/MinGW/bin/gcc.exe -Wall -Wextra -std=c99 -Isrc src/*.c -o bin/sharpscript.exe -lm
```
