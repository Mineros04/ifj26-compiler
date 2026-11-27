# IFJ26 Compiler

A compiler written in C from **IFJ26** to **IFJcode26**, developed as a team project for the IFJ and IAL courses at the Faculty of Information Technology, Brno University of Technology, in the 2026/2027 academic year.

IFJ26 is a simplified subset of Scala with static typing, type inference, indentation-based blocks, user-defined functions, and basic control flow.

## Project status

The compiler is finished with extensions outlined below.

| Component                    | Module                             | State                    |
| ---------------------------- | ---------------------------------- | ------------------------ |
| Lexical analysis             | `lexer.c`                          | Implemented, unit-tested |
| Syntax analysis (statements) | `parser.c`                         | Implemented              |
| Expression parsing           | `expr.c`                           | Implemented, unit-tested |
| Abstract syntax tree         | `ast.c`                            | Implemented, unit-tested |
| Symbol table (`TRP-izp`)     | `symtable.c`, `shared/hashtable.c` | Implemented, unit-tested |
| Semantic analysis            | `semantic.c`                       | Implemented              |
| Code generation (IFJcode26)  | `codegen.c`                        | Implemented              |

## Compiler interface

The required compiler interface is:

- Read IFJ26 source code from standard input.
- Write generated IFJcode26 code to standard output.
- Write diagnostics to standard error.
- Return `0` on successful compilation or the appropriate error code on failure.

The generated code can be executed using the `ic26int` interpreter provided with the course materials.

## Building and running

Requirements: `clang` with C23 support and `make`. `valgrind` is optional for memory checks, and Graphviz (`dot`) is needed only to regenerate documentation figures.

```sh
make build                                  # build the compiler into dist/compiler
./dist/compiler < program.ifj26 > out.code  # compile an IFJ26 program
ic26int out.code                            # run the generated IFJcode26
echo $?                                     # compiler exit code (see below)
```

Other targets:

| Target          | Description                                                       |
| --------------- | ----------------------------------------------------------------- |
| `make` / `all`  | Build and run the compiler (reads from standard input)            |
| `make run`      | Run the already built compiler                                    |
| `make memcheck` | Run the compiler under `valgrind --leak-check=full`               |
| `make docs`     | Render the lexer FSM and architecture diagrams to `docs/figures/` |
| `make clean`    | Remove the `dist/` directory                                      |

## Example IFJ26 program

```scala
import ifj26.*

@main def main(): Unit =
  print("Hello, world!\n")
```

## Compiler exit codes

| Code | Meaning                                                           |
| ---- | ----------------------------------------------------------------- |
| `0`  | Successful compilation                                            |
| `1`  | Lexical error                                                     |
| `2`  | Syntax error                                                      |
| `3`  | Undefined function, variable, or type; missing entry function     |
| `4`  | Invalid redefinition                                              |
| `5`  | Incorrect function arguments or invalid entry-function parameters |
| `6`  | Type compatibility or type inference error                        |
| `7`  | Assignment to an immutable variable or function parameter         |
| `10` | Other semantic error                                              |
| `99` | Internal compiler error                                           |

See the language specification for the full rules and runtime error behavior.

## Testing

Unit tests use the single-header [utest.h](tests/utest.h) framework and live in [tests/](tests/), one file per module (`test_lexer.c`, `test_expr.c`, `test_ast.c`, `test_symtable.c`, `test_hashtable.c`). All modules except `main.c` are linked into a single test runner.

```sh
make test                                    # build dist/test_runner and run all tests
make test TEST_ARGS="--filter=lexer.*"       # run only tests matching a pattern
make test TEST_ARGS="--list-tests"           # list available tests
```

The test runner is built with debug symbols (`-g`), so it can be checked directly with `valgrind --leak-check=full ./dist/test_runner`.

## Team and implementation details

- **Team members:** Pavel Bobek, Adam Jaroš, Tomáš Komenda, Ondřej Pippal
- **Symbol-table variant:** `TRP-izp`
- **Implemented extensions:** FUNEXP, BOOLTHEN, VALMEM, PARAMLISTS
- **Code formatting style:** as defined in [.clang-format](.clang-format)

## Reference documents

- [Project assignment](docs/assignment/IFJ_and_IAL_2026_2027_project_assignment.pdf)
- [IFJ26 and IFJcode26 specification](docs/assignment/IFJ26_and_IFJcode26_language_spec.pdf)

Refer to these documents for the project requirements.
