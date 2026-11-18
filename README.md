# B-Minor Compiler

A C++ implementation of a compiler front-end for the B-Minor programming language.

## Features

- Lexical Analysis (Lexer)
- Syntax Analysis (Parser)
- Abstract Syntax Tree (AST) construction
- Lexical and syntax error detection
- Panic-mode error recovery
- AST-based syntax highlighting
- ANSI terminal output
- HTML/CSS syntax highlighting

## Technologies
- C++
- Standard Template Library (STL)

## Compilation

```bash
g++ -std=c++17 Simplified_BMinor.cpp -o bminor
```

## Execution

```bash
./bminor
```

## Tests

Includes two test cases:
1. A valid recursive factorial program.
2. A test for lexical and syntax error recovery.
