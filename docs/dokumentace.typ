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

#table(
  columns: (auto, 1fr),
  align: (right + horizon, left + horizon),
  table.header([*\#*], [*Pravidlo*]),
  [1],  [`<program>` → `<opt_nl>` `<prologue>` `<opt_nl>` `<func_list>` `EOF`],
  [2],  [`<prologue>` → `import` `id` `.` `*` _(s kontrolou `id == "ifj26"`)_],
  [3],  [`<opt_nl>` → `NEWLINE` `<opt_nl>`],
  [4],  [`<opt_nl>` → ε],
  [5],  [`<opt_sep>` → `NEWLINE` `<opt_sep>`],
  [6],  [`<opt_sep>` → `;` `<opt_sep>`],
  [7],  [`<opt_sep>` → ε],
  [8],  [`<func_list>` → `@main` `<opt_nl>` `<func_def>` `<opt_nl>` `<func_list>`],
  [9],  [`<func_list>` → `<func_def>` `<opt_nl>` `<func_list>`],
  [10], [`<func_list>` → ε],
  [11], [`<func_def>` → `def` `id` `<param_lists>` `<opt_type>` `=` `<block_or_stmt>`],
  [12], [`<param_lists>` → `(` `<opt_params>` `)` `<param_lists>`],
  [13], [`<param_lists>` → ε],
  [14], [`<opt_params>` → `<param>` `<param_tail>`],
  [15], [`<opt_params>` → ε],
  [16], [`<param_tail>` → `,` `<param_next>`],
  [17], [`<param_tail>` → ε],
  [18], [`<param_next>` → `<param>` `<param_tail>`],
  [19], [`<param_next>` → ε _(trailing čárka před `)` na novém řádku)_],
  [20], [`<param>` → `id` `:` `<type>`],
  [21], [`<opt_type>` → `:` `<type>`],
  [22], [`<opt_type>` → ε],
  [23], [`<type>` → `Int`],
  [24], [`<type>` → `Double`],
  [25], [`<type>` → `String`],
  [26], [`<type>` → `Boolean`],
  [27], [`<type>` → `Unit`],
  [28], [`<block_or_stmt>` → `NEWLINE` `<opt_nl>` `<indented_block>`],
  [29], [`<block_or_stmt>` → `<indented_block>`],
  [30], [`<block_or_stmt>` → `<stmt>`],
  [31], [`<indented_block>` → `INDENT` `<opt_sep>` `<stmt_list>` `DEDENT`],
  [32], [`<stmt_list>` → `<stmt>` `<opt_sep>` `<stmt_list>`],
  [33], [`<stmt_list>` → ε],
  [34], [`<stmt>` → `<var_decl>`],
  [35], [`<stmt>` → `<if_stmt>`],
  [36], [`<stmt>` → `<while_stmt>`],
  [37], [`<stmt>` → `<return_stmt>`],
  [38], [`<stmt>` → `id` `<id_tail>`],
  [39], [`<stmt>` → `<expr>`],
  [40], [`<var_decl>` → `val` `id` `<opt_type>` `=` `<expr>`],
  [41], [`<var_decl>` → `var` `id` `<opt_type>` `=` `<expr>`],
  [42], [`<if_stmt>` → `if` `<expr>` `then` `<block_or_stmt>` `<else_part>`],
  [43], [`<else_part>` → `else` `<block_or_stmt>`],
  [44], [`<else_part>` → ε],
  [45], [`<while_stmt>` → `while` `<expr>` `do` `<block_or_stmt>`],
  [46], [`<return_stmt>` → `return` `<opt_expr>`],
  [47], [`<opt_expr>` → `<expr>`],
  [48], [`<opt_expr>` → ε],
  [49], [`<id_tail>` → `=` `<expr>`],
  [50], [`<id_tail>` → `<arg_lists>`],
  [51], [`<arg_lists>` → `(` `<opt_args>` `)` `<arg_lists_tail>`],
  [52], [`<arg_lists_tail>` → `(` `<opt_args>` `)` `<arg_lists_tail>`],
  [53], [`<arg_lists_tail>` → ε],
  [54], [`<opt_args>` → `<expr>` `<arg_tail>`],
  [55], [`<opt_args>` → ε],
  [56], [`<arg_tail>` → `,` `<arg_next>`],
  [57], [`<arg_tail>` → ε],
  [58], [`<arg_next>` → `<expr>` `<arg_tail>`],
  [59], [`<arg_next>` → ε],
)

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
