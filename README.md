# SharpScript Programming Language

SharpScript is a custom programming language interpreter built in C, designed to provide a simple yet powerful scripting environment with modern language features. This project includes a complete interpreter implementation along with development tools and examples.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [Installation](#installation)
- [Usage](#usage)
- [Language Syntax](#language-syntax)
- [Examples](#examples)
- [Development Tools](#development-tools)
- [Testing](#testing)
- [Contributing](#contributing)
- [License](#license)
- [Documentation](#documentation)

## Overview

SharpScript is an interpreted programming language that combines the simplicity of scripting languages with structured programming concepts. The interpreter is written in C and provides a complete development environment including lexical analysis, parsing, interpretation, and memory management.

The language supports variables, functions, control structures, namespaces, and built-in system functions for input/output operations. The project also includes additional tools like a web-based calculator and syntax highlighter.

## Features

### Core Language Features
- **Variables**: Declaration and assignment with type inference
- **Functions**: User-defined functions with parameters and return values
- **Control Structures**: If-else statements, loops, and conditional logic
- **Namespaces**: Organize code into logical modules
- **Comments**: Single-line comments using `#` syntax
- **Error Handling**: Built-in error and warning system functions

### Interpreter Features
- **Interactive REPL**: Read-Eval-Print Loop for interactive development
- **File Execution**: Run SharpScript files with `.sharp` extension
- **Memory Management**: Automatic memory allocation and cleanup
- **Error Reporting**: Detailed error messages with line information
- **Cross-Platform**: Designed for Windows 10/11 with MinGW32

### Development Tools
- **Web Calculator**: Browser-based calculator with SharpScript integration
- **Syntax Highlighter**: Code highlighting for SharpScript syntax
- **Built-in Functions**: Mathematical operations, I/O operations, and utility functions

## Architecture

The SharpScript interpreter follows a traditional compiler architecture with the following components:

### Core Components

#### 1. Lexer (lexer.c/lexer.h)
- Tokenizes source code into meaningful symbols
- Handles keywords, identifiers, operators, and literals
- Provides error handling for invalid tokens

#### 2. Parser (parser.c/parser.h)
- Builds Abstract Syntax Trees (AST) from tokens
- Implements recursive descent parsing
- Validates syntax and semantic rules

#### 3. Abstract Syntax Tree (ast.c/ast.h)
- Defines the structure of parsed code
- Represents program elements as tree nodes
- Enables semantic analysis and interpretation

#### 4. Interpreter (interpreter.c/interpreter.h)
- Executes the AST nodes
- Manages variable scopes and function calls
- Handles control flow and built-in operations

#### 5. Memory Management (memory.c/memory.h)
- Implements dynamic memory allocation
- Provides garbage collection mechanisms
- Manages string and object lifecycle

### Built-in Functions

The interpreter includes several built-in system functions:

```sharpscript
system.output(expression);    # Print output to console
system.error(message);        # Display error message
system.warning(message);      # Display warning message
```

## Installation

### Prerequisites
- Windows 10/11 operating system
- MinGW32 compiler toolchain
- Make utility (mingw32-make)

### Build Instructions

1. **Clone the repository**:
   ```bash
   git clone https://github.com/yourusername/sharpscript.git
   cd sharpscript
   ```

2. **Build the interpreter**:
   ```bash
   mingw32-make
   ```

3. **Verify installation**:
   ```bash
   ./obj/bin/sharpscript.exe --help
   ```

### Build System

The project uses a Makefile-based build system optimized for Windows:
- **Compiler**: GCC with MinGW32
- **Flags**: Wall, Wextra, C99 standard
- **Output**: Executable in `obj/bin/sharpscript.exe`

## Usage

### Interactive Mode (REPL)

Start the interactive interpreter:
```bash
sharpscript
```

In REPL mode, you can:
- Type SharpScript expressions directly
- Test language features interactively
- Get immediate feedback on syntax and execution

### Script Execution

Run a SharpScript file:
```bash
sharpscript script.sharp
```

### Help System

Display help information:
```bash
sharpscript --help
```

## Language Syntax

### Variables

SharpScript uses explicit variable declaration with the `&insert` keyword:
```sharpscript
&insert x = 10;
&insert name = "Hello World";
&insert pi = 3.14159;
```

### Functions

Define functions using the `function` keyword:
```sharpscript
function greet(void) {
    system.output("Hello from SharpScript!");
}

function add(a, b) {
    &insert result = a + b;
    return result;
}
```

### Control Structures

#### If-Else Statements
```sharpscript
&insert x = 5;
if (x > 3) {
    system.output("x is large");
} else {
    system.error("x too small");
}
```

### Comments

Single-line comments use the `#` character:
```sharpscript
# This is a comment
&insert x = 10; # This is also a comment
```

### System Functions

Built-in system functions for I/O operations:
```sharpscript
system.output("Hello World");     # Print to console
system.error("Error message");    # Display error
system.warning("Warning message"); # Display warning
```

## Examples

### Basic Program

```sharpscript
# Simple SharpScript program
&insert x = 5;

function main(void) {
    system.output(x);
    if (x > 3) {
        system.warning("x is large");
    } else {
        system.error("x too small");
    }
}
```

### Mathematical Operations

```sharpscript
&insert radius = 5;
&insert area = 3.14159 * radius * radius;
system.output("Area of circle: ");
system.output(area);
```

### Function Definition and Usage

```sharpscript
function calculate_area(radius) {
    &insert pi = 3.14159;
    &insert area = pi * radius * radius;
    return area;
}

&insert result = calculate_area(10);
system.output("Area: ");
system.output(result);
```

## Development Tools

### Web Calculator

The project includes a web-based calculator with SharpScript integration:
- **Location**: `tools/calculator/`
- **Features**: Basic arithmetic, scientific functions, unit conversion
- **Natural Language**: Support for natural language input
- **Multi-language**: UI available in multiple languages

**Usage**:
1. Open `tools/calculator/index.html` in a web browser
2. Type expressions or natural language commands
3. Use keyboard shortcuts for common operations

### Syntax Highlighter

A web-based syntax highlighter for SharpScript code:
- **Location**: `tools/highlighter/`
- **Features**: Syntax highlighting for SharpScript syntax
- **Integration**: Can be embedded in web applications

## Testing

The project includes comprehensive test cases in the `tests/` directory:

### Test Categories
- **Syntax Tests**: Validation of language syntax
- **Function Tests**: Testing built-in and user functions
- **Memory Tests**: Memory management and garbage collection
- **Error Handling**: Error and warning system tests
- **Namespace Tests**: Namespace and scope testing

### Running Tests

Execute test files using the interpreter:
```bash
sharpscript tests/help.sharp
sharpscript tests/math_functions.sharp
sharpscript tests/memory_functions.sharp
```

### Test Examples

- `help.sharp`: Tests help system functionality
- `math_functions.sharp`: Mathematical operations testing
- `memory_functions.sharp`: Memory management tests
- `namespace_function_call.sharp`: Namespace functionality
- `unit_convert.sharp`: Unit conversion features

## Project Structure

```
sharpscript/
├── src/                    # Source code
│   ├── include/           # Header files
│   ├── builtins/          # Built-in functions
│   ├── lib/              # Library files
│   ├── main.c            # Main entry point
│   ├── lexer.c           # Lexical analyzer
│   ├── parser.c          # Syntax parser
│   ├── interpreter.c     # Code interpreter
│   ├── ast.c             # Abstract syntax tree
│   └── memory.c          # Memory management
├── examples/              # Example SharpScript files
├── tests/                 # Test files
├── docs/                  # Documentation
├── tools/                 # Development tools
│   ├── calculator/       # Web calculator
│   └── highlighter/      # Syntax highlighter
├── obj/                   # Build output
└── .vscode/              # VS Code configuration
```

## Contributing

We welcome contributions to the SharpScript project! Please see [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines on:
- Code style and conventions
- Submitting issues and pull requests
- Development workflow
- Testing requirements

## Security

For security concerns and vulnerability reporting, please see [SECURITY.md](SECURITY.md) for our security policy and reporting procedures.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

## Changelog

For version history and changes, see [CHANGELOG.md](CHANGELOG.md).

## Documentation

Additional documentation is available in the `docs/` directory:
- [Developer Guide](docs/DEVELOPER_GUIDE.md): Technical implementation details
- [User Guide](docs/USER_GUIDE.md): User-focused documentation

## Support

For questions, issues, or contributions:
- Create an issue in the GitHub repository
- Check existing documentation in the `docs/` directory
- Review test files in the `tests/` directory for usage examples

---

**SharpScript** - A modern scripting language with simplicity and power combined.