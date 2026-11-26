# IFJ26 Compiler

A compiler written in C from **IFJ26** to **IFJcode26**, developed as a team project for the IFJ and IAL courses at the Faculty of Information Technology, Brno University of Technology, in the 2026/2027 academic year.

IFJ26 is a simplified subset of Scala with static typing, type inference, indentation-based blocks, user-defined functions, and basic control flow.

## Project status

TBD

## Compiler interface

The required compiler interface is:

- Read IFJ26 source code from standard input.
- Write generated IFJcode26 code to standard output.
- Write diagnostics to standard error.
- Return `0` on successful compilation or the appropriate error code on failure.

The generated code can be executed using the `ic26int` interpreter provided with the course materials.

## Building and running

TBD

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

TBD

## Team and implementation details

- **Team members:** Pavel Bobek, Adam Jaroš, Tomáš Komenda, Ondřej Pippal
- **Symbol-table variant:** `TRP-izp`
- **Implemented extensions:** FUNEXP, BOOLTHEN, VALMEM, PARAMLISTS
- **Source layout and module descriptions:** TODO
- **Code formatting style:** as defined in [.clang-format](.clang-format)

## Reference documents

- [Project assignment](docs/assignment/IFJ_and_IAL_2026_2027_project_assignment.pdf)
- [IFJ26 and IFJcode26 specification](docs/assignment/IFJ26_and_IFJcode26_language_spec.pdf)

Refer to these documents for the project requirements.
