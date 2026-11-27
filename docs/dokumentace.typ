#import "template.typ": *

#show: FIT-Protocol.with(
  academic-subject: "Formální jazyky a překladače, Algoritmy",
  academic-year: "2026/2027",
  protocol-title: "IFJ26 Compiler",
  protocol-subtitle: "Dokumentace k projektu",
  authors: (
    (name: "Pavel Bobek", login: "xbobekp00, 267053", credits: ""),
    (name: "Adam Jaroš", login: "xjarosa00, 279581", credits: ""),
    (name: "Tomáš Komenda", login: "xkoment00, 282034", credits: "", leader: true),
    (name: "Ondřej Pippal", login: "xpippao00, 282581", credits: "")
  ),
  variant: "TRP-izp",
  extensions: ("FUNEXP", "BOOLTHEN", "VALMEM", "PARAMLISTS"),
  repository: "https://github.com/Mineros04/ifj26-compiler",
)

= Rozdělení práce

*Tomáš Komenda*
- Návrh a implementace tabulky symbolů (`src/symtable.h`, `src/symtable.c`), bez rošíření
	- varianta TRP-izp - hashovací tabulka s implicitním řazením (`src/shared/hashtable.h`, `src/shared/hashtable.c`)
	- rozšíření BOOLTHEN, FUNEXP, VALMEM, PARAMLISTS
- Implementace precedenčního syntaktického analyzátoru pro výrazy + návrh precedenční tabulky (`src/expr.h`, `src/expr.c`).
- Jednotkové testy pro hashovací tabulku, tabulku symbolů a syntaktického analyzátoru pro výrazy (`tests/test_hashtable.c`, `tests/test_symtable.c`, `tests/test_expr.c`).

*Adam Jaroš*
- Návrh a implementace sdílených knihoven:
  - Dynamický textový řetězec (`src/shared/str.h`, `src/shared/str.c`)
  - Centrální zpracování chyb (`src/shared/error.h`, `src/shared/error.c`)
- Implementace lexikálního analyzátoru (`src/lexer.h`, `src/lexer.c`)
  - Jednotkové testy pro lexer.

== Spolupráce

= Implementace

== Blokové schéma architektury

== Diagram konečného automatu lexeru

#image("figures/lexer-fsm.png")

== Gramatika

=== LL gramatika

=== LL tabulka

#pagebreak()

=== Precedenční tabulka

#align(center, rotate(-90deg, reflow: true, block(width: 225mm)[
  #set text(size: 10pt)
  #table(
    columns: (auto, auto) + (1fr,) * 9,
    inset: (x: 5pt, y: 10pt),
    align: center + horizon,
    table.header(
      table.cell(colspan: 2, rowspan: 2)[],
      table.cell(colspan: 9)[*NEXT TERMINAL*],
      [*PRIORITY\_1*], [*PRIORITY\_2*], [*PRIORITY\_3*], [*PRIORITY\_4*], [*PRIORITY\_5*], [*OPERAND*], [*OPEN\_PAR*], [*CLOSE\_PAR*], [*END\_EXPR*],
    ),
    table.cell(rowspan: 9)[#rotate(90deg, reflow: true)[*STACK*]],
    [*PRIORITY\_1*], [SHIFT],  [REDUCE], [REDUCE], [REDUCE], [REDUCE], [SHIFT], [SHIFT], [REDUCE], [REDUCE],
    [*PRIORITY\_2*], [SHIFT],  [REDUCE], [REDUCE], [REDUCE], [REDUCE], [SHIFT], [SHIFT], [REDUCE], [REDUCE],
    [*PRIORITY\_3*], [SHIFT],  [SHIFT],  [REDUCE], [REDUCE], [REDUCE], [SHIFT], [SHIFT], [REDUCE], [REDUCE],
    [*PRIORITY\_4*], [SHIFT],  [SHIFT],  [SHIFT],  [ERROR],  [REDUCE], [SHIFT], [SHIFT], [REDUCE], [REDUCE],
    [*PRIORITY\_5*], [SHIFT],  [SHIFT],  [SHIFT],  [SHIFT],  [ERROR],  [SHIFT], [SHIFT], [REDUCE], [REDUCE],
    [*OPERAND*],     [REDUCE], [REDUCE], [REDUCE], [REDUCE], [REDUCE], [ERROR], [ERROR], [REDUCE], [REDUCE],
    [*OPEN\_PAR*],   [SHIFT],  [SHIFT],  [SHIFT],  [SHIFT],  [SHIFT],  [SHIFT], [SHIFT], [EQUAL],  [ERROR],
    [*CLOSE\_PAR*],  [REDUCE], [REDUCE], [REDUCE], [REDUCE], [REDUCE], [ERROR], [ERROR], [REDUCE], [REDUCE],
    [*END\_EXPR*],   [SHIFT],  [SHIFT],  [SHIFT],  [SHIFT],  [SHIFT],  [SHIFT], [SHIFT], [ERROR],  [ACCEPT],
  )
]))

#pagebreak()

== Popis modulů

== Využití AI při řešení
