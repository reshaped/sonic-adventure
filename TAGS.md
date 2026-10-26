# Tags

Tags track three things in the source: assembly, choices made only to match SHC's output, and bugs preserved from the shipped game.

| Tag | Marks | Its text gives |
|---|---|---|
| `@asm` | Each piece of **assembly**: a `.src` function, an inline body or a `#pragma asm` block | The evidence that the original was written in assembly; for `open`, what that evidence lacks |
| `@hack` | A **hack**: a choice in the source kept only because SHC's output needs it | What the hack is and steers, and the form it was measured against |
| `@bug` | A **bug**: a defect of the shipped game that the source keeps | What fails, and the addresses that show it |

`make lint` checks a tag's form. A reviewer checks its argument ([Reviewing a tag](#reviewing-a-tag)).

## Writing a tag

Write a tag in a comment above what it tags, as `@<tag> <kind>: <text>`.

```text
/* @<tag> <kind>: <text> */     in C: a plain comment, never /** */
; @<tag> <kind>: <text>         in a .src file: a comment line of its own
```

This `@hack` tag from `src/camera/modes.c` shows how a tag can span several lines of one comment:

```c
/* @hack redundant: case 5, which shares the default's code, keeps the image's
 * CMP/EQ #5 at 0x8c021f56; without it, 57 of 297 words differ */
case 5:
default:
```

An `@asm` tag for assembly with no basis yet:

```text
; @asm open: every instruction is one SHC writes for C
```

For an `@bug` tag, see the `uninitialized` one in `src/object/spring.c`.

## Hacks and bugs

An `@hack` tag marks a choice in the source kept only because SHC's output needs it, where plainer C would express the same behavior. An `@bug` tag marks an evidenced defect of the shipped game that the source keeps, whether the source makes it or SHC's code does. Ordinary code takes neither. A construct the behavior needs is no hack, however unusual it looks: a cast C needs for the call the image makes takes no tag. Nor is data the image holds and no code reads: the source defines it because the bytes are there, not to steer SHC's code, so it takes no tag. An initializer, zeros included, that holds a static datum in the image's data rather than its bss is data the image holds too; a local's initializer is code, and takes a tag when nothing reads it.

A tag records one finding. Tags stack only where their findings are separate, as a `volatile` stacks with the unset read it steers. No tag certifies the code around it. Lint doesn't find every hack or bug; it checks only the patterns [What lint checks](#what-lint-checks) names.

### Hacks

An `@hack` tag goes directly above its hack. Its text says what the hack is and what it steers. It names the plain form the hack was measured against, with any precondition under which the two forms are equal, and gives the count. A hack that can be deleted is measured without it, and any other against the form its text names:

```text
without it, N of M words differ
as i++, N of M words differ
```

M counts the 2-byte instruction words of the original function that control reaches from its entry. N counts how many of them differ, at the same offset in its object, when the function is built in the plain form. A change of size shifts the words after it, so it shows as most of them differing. A hack in a macro, an inline function, a type, a field or a datum sums over the functions that use it. The tag on `drop_vars.steps` in `events/ev0024/wreckage_drop.c` counts 118 of the 267 words of the five functions that use `drop_vars`. Where the count can't be measured, the text gives the form and `count unmeasured`, spelled so: `without it, count unmeasured`. Remove a construct added only to steer SHC if removing it changes no byte.

| Kind | Hack |
|---|---|
| `implicit` | The source leaves out a value, a return value or an evaluation order the behavior needs, and SHC's code supplies it. |
| `type` | A type, conversion, representation or qualifier chosen for SHC's output. |
| `redundant` | A construct the behavior doesn't need, kept for SHC's output. |
| `spelling` | Code written in another form that does the same, chosen for SHC's output. |

Each kind covers shapes such as these:

- `implicit`: a path that returns no value where R0 holds the result each caller takes, a read no assignment reaches where its register holds the value, two evaluations C leaves unordered that the code orders.
- `type`: one object declared as an array, an integer of another width or sign, a bare `0` for `NULL`, a function without a prototype, a union of a word and its halves, `*(Uint32 *)&f`, a `volatile` added or left out.
- `redundant`: code that never runs, a store nothing reads, a local kept for register allocation or the frame's size, `(void)x`, `+ 0`, `x = x`, `if (1) {}`, a jump to where control goes anyway, `register`.
- `spelling`:
  - an operation spelled another way, as `x = x + 1` for `x++`, `x = x | b` for `x |= b` or `x += -y` for `x -= y`
  - an assignment inside a test
  - assignments chained in one statement
  - a test and its statement joined by `&&` or a comma
  - a value held in a local where its use could compute it, such as a function called through one
  - a variable reused for a second value
  - `(&x)->m`
  - a cursor stepped across a struct's members
  - statements in another order, or a store made in each arm
  - a test in another shape, as nested `if`s or `&` for `&&`, a float tested bare for `!= 0.0F`, a one-case `switch` or a select for an `if`
  - a loop in another shape, as a count down or a `do` for a `while`
  - a jump in place of structure, as `goto`s to code the cases share or a `return` for a `break`
  - code written twice, as two cases that do the same
  - a helper SHC expands at each call

Where a hack fits more than one kind, the first of `implicit`, `type`, `redundant` and `spelling` that fits decides. A tag's kind is the kind of the choice its text describes, not of every property of the statement under it. A local that only sizes the frame is `redundant`; a local holding a value its use could compute is `spelling`.

The plain form keeps the behavior under the preconditions the text names, or it is no plain form. `b == TRUE` is `b` only while `b` is 0 or 1, so its tag says why `b` is. A store to a local nothing reads is `redundant` with its right side kept: the call in `x = f()` stays.

A last case with no `break` takes no tag, since no construct stands there to carry one. A comment at the case says what a `break` would change and gives its count, as the one in `modules/stg06/collapse.c` does.

The grafted source's donor counts differently: its tags count the words of each function in the donor's own patched and assembled text, though only four of its functions reach the image ([Grafted R10](TOOLCHAIN.md#grafted-r10)).

### Bugs

A bug stays because fixing it would change the bytes. Its `@bug` tag goes directly above it. Its first clause says what fails, in plain words, and the rest gives when it fails and the addresses that show it. "reads the next pointer after freeing the node" says what fails; "invalid memory handling" doesn't.

| Kind | Bug |
|---|---|
| `memory` | An invalid read, write or free, or memory no path frees: an access through null, outside its object, after its free, or through a pointer to no object the code means. |
| `uninitialized` | A value used where its path never set it: a local, an out-parameter a failed call didn't write, a heap field as its allocator left it, or R0 where a path returns no value and a caller uses it. |
| `logic` | Well-defined code that does the wrong thing: an operation on another operand, field, element or unit than the one meant, a test that always, never or wrongly holds, a state update missing, repeated or out of order. |
| `arithmetic` | A numeric result other than its expression's ordinary meaning, from machine arithmetic, an invalid operation, a conversion or SHC's code: an overflow, a shift by the width or more, a division by zero, a float converted out of range. |
| `mismatched-call` | A call whose arguments, result or calling convention disagree with its callee's independently established interface. The text says what the callee takes. |

Where an invalid access is established, the bug is `memory`, whatever computed the address, and its text names the cause. A tag classifies the failure its evidence shows, not one that failure might lead to.

`implicit` and `uninitialized` can mark the same C. A value the reconstruction relies on, which SHC's code supplies, is `implicit`. A shipped read of a value its path never set is `uninitialized`, even where the machine's value is known. Its text names that value. A read takes one of the two.

An allocator's contract says whether it can fail and what it then returns. A caller's routine assumption that it succeeds takes no tag. A failure path that does more keeps its tag: a function that goes on after its allocation fails, leaks what it already allocated, or leaves its state inconsistent.

## Assembly

Each `.src` function, inline body (`#pragma inline_asm`) and `#pragma asm` block holding more than comments carries one `@asm` tag. If we recover C that matches, it replaces the assembly.

A **basis** is evidence that the original was written in assembly. It isn't proof that no C could match. A tag's kind records its basis, or is `open` until one is established:

| Kind | Basis |
|---|---|
| `opcode` | A required instruction is outside what the release and flags emit from C: no builtin or assembler form gives it, or no compiler stage reaches the form that does. |
| `operand` | The compiler emits the instruction, but not with its operands or registers. |
| `abi` | The entry, exit or machine-state contract is outside C's interface, as setting the stack is. |
| `epilogue` | SHC's trailer after an inline body: a dead `RTS` and `NOP` after its own return, or alignment and `RTS` after a body that ends in neither a return nor a tail jump. |
| `name` | The shipped library names the member as assembly. |
| `open` | None holds yet. |

The first three require evidence of a compiler limitation under the release and flags used for that code. A compiled probe can support the case, but an unsuccessful C attempt doesn't establish it.

Open assembly rebuilds exactly, like everything else, and counts in the build's numbers. An `open` tag claims nothing about how the original was written: it marks work left, a basis to find or C to write. The basis share in README.md's numbers measures this classification, not how much of the original is proven to be assembly.

### What the text says

Except for `open`, the text begins with its rule or evidence identifier and a comma, then explains the basis and cites the addresses it rests on. Lint takes each `0x` and eight hex digits for a cited address, so the text spells a 32-bit value another way, as `H'FF000000`. For `name`, the citation is the function's first address rather than an instruction witness. Anything after that only corroborates. By kind, the text opens with:

- `opcode`: `rotcl`, `rotcr`, `swap`, `dmuls`, `dmulu`, `movca`, `fschg`, `cache-line` or `sett`;
- `operand`: `fipr-registers`, `ftrv-registers`, `fsca-registers`, `small-constant`, `zero-displacement` or `fschg-pairs`;
- `abi`: `stack-written`, `fpscr-save`, `stack-above-entry` or `return-below-entry`, or a contract;
- `epilogue`: `dead-return` or `fill`, citing the dead `RTS` or the padding;
- `name`: the member's name, citing the function's first address.

The `opcode`, `operand` and non-contract `abi` openings are rules of `shckit inspect`, which shckit's [inspect.md](https://github.com/reshaped/shckit/blob/e0b8412e337103b59ea2f0d58dd7be52699222b9/docs/inspect.md#the-rules) defines with the releases and modes each holds for. The witness is the instruction at the cited address, in the tagged function or one its inline body expands into. `stack-above-entry` and `return-below-entry` rest on inspect's [frame argument](https://github.com/reshaped/shckit/blob/e0b8412e337103b59ea2f0d58dd7be52699222b9/docs/inspect.md#abi-rules), which is unfinished: no R10 compile of C found so far makes either witness.

An `open` tag's text is one short line saying what's missing, such as an instruction no inspect rule reads, or that every instruction is one SHC writes for C. It cites an address only where that helps.

### Where the tag goes

- For an inline body, directly above its `#pragma inline_asm`.
- For a `#pragma asm` block, directly above the block.
- For a `.src` function, at the end of the comment above its label.

A `.src` function is the code after a run of labels, one of them exported or named in the [image tables](TOOLCHAIN.md#image-tables). A label before data starts none. Labels in a run name the same code: `_syIntExpISRBottom` runs straight into `_syIntISRTop` in `sdk/shinobi/syIntIsr.src`, so a tag above either covers both. [Assembly sources](TOOLCHAIN.md#assembly-sources) gives the rest of a `.src` file's form.

### What is not a basis

Padding between functions, an unaligned game-code pool, data in code, sharing an object with assembly, a jump into another object's function, a record of failed C attempts: each can help an investigation, and none is a basis alone. Nor is an object's producer record, since asmsh assembles SHC's text for C with inline assembly too. Nor is a register convention: runtime helpers have their own, and `#pragma global_register` covers R8 to R14. `RTE` isn't an `opcode` basis either: SHC writes it for `#pragma interrupt`.

### Assembly evidence

`make review` runs `tools/tags.py --review`: lint's tag checks, then a list of each `open` tag with its place and text, and a count of the inline bodies, `#pragma asm` blocks and `.src` functions by their tags' kinds. The count shows that each tag is there and well formed, not that its argument holds. It runs no inspect.

### Checking a basis

For an `opcode`, `operand` or `abi` tag that opens with one of inspect's rules, confirm that inspect reports the rule at the cited instruction, then review the rule's scope and evidence. An inspect result does not resolve limitations in the argument behind the rule. A contract, an epilogue or a name is checked by reading what its tag cites, against [Bases no tool checks](#bases-no-tool-checks).

To reproduce an inspect finding, let `PATH` stand for the source path without its extension:

1. **Build the text asmsh assembled.** For an inline body or a block, `make build/PATH.obj` writes R10's text at `build/PATH.src`; on a patched route take `build/PATH_patched.src`, or `build/PATH_aligned.src` for a source with an `ALIGN` setting. For a `.src` function, use the file itself.
2. **Pick the release and mode.** Inline bodies and blocks take `--release 5.0R10 --mode asmcode`. A `.src` function takes the release [Releases](TOOLCHAIN.md#releases) gives its code, with `--mode machinecode`. The directory doesn't decide: `sdk/ninja/kmasm_kamui_lib.src` is one of SDK 1.30J's Kamui members, so R32.
3. **Set the bounds.** `--entry` is the function's label and `--end` the next function's; leave `--end` out for a section's last function. R10 prefixes an external function's name with `_`, and a static one may have only an `Ln` label: check the listing, `build/PATH.lis`.
4. **Find the witness.** `--all` prints every witness with its line in the text. The cited address less the start of its `splits.txt` row is an offset; the listing puts that offset on a line, and gives the text's line number after the object code.

`thread_yield`'s tag in `src/system/thread.c` opens `stack-written` and cites `0x8c09f27c`:

```sh
make build/shckit build/src/system/thread.obj
build/shckit inspect --release 5.0R10 --mode asmcode --entry _thread_yield --end _thread_exit --all build/src/system/thread.src
```

```text
build/src/system/thread.src:92: _thread_yield: abi: stack-written
build/src/system/thread.src:89: _thread_yield: incomplete: call
```

Its `P` row starts at `0x8c09f200`, so the cited address is offset `0x7c`, which the listing puts on text line 92. inspect exits 3 for the limit it met at the call on line 89; the witness holds even so ([What it reports](https://github.com/reshaped/shckit/blob/e0b8412e337103b59ea2f0d58dd7be52699222b9/docs/inspect.md#what-it-reports)).

### Bases no tool checks

Some assembly functions rely on an agreement with their callers or with the interrupt system that C's calling convention can't express. Its evidence spans the function and the places that enter it, more than inspect can see from one function. The tag names those places and the addresses each part rests on:

| Rule | Basis |
|---|---|
| `register-contract` | The function and its one caller pass a register outside C's interface. The function reads it before writing it, and returns it, changed or not. The caller sets it before the call and reads it after. |
| `exception-entry` | Interrupt setup copies the function to the vector base. The hardware enters it with the interrupted program's registers live, and it leaves by a jump with a word pushed. |
| `exception-continuation` | The function writes SSR and SPC and jumps to another, which keeps them in its frame and restores them for its `RTE`. |

`_sprite_send_clipped`'s tag in `src/draw/sprite.src` cites both sides of its contract:

```text
; @asm abi: register-contract, R2 carries the cursor both ways: this reads it
; before any write, at 0x8c0a4ada or 0x8c0a4b16, and returns it advanced at its
; RTS at 0x8c0a4b88 to sprite_send_flat, its one caller, which sets it at
; 0x8c0a4938 before its JSR, takes it back at 0x8c0a4948 and saves the R8 and
; FR12 to FR15 this changes
```

A dead `RTS` alone isn't evidence of assembly origin. R10 can copy a function's exit for each `return` and keep a copy nothing reaches, so plain C can leave a dead `RTS` after the slot of a tail `JMP`, `BRA` or `BRAF`.

After an inline body's own return, a dead `RTS` provides a basis when that return's delay slot pops a register the dead exit doesn't: a copied exit would pop the same register. Other `dead-return` cases rest on an unfinished reading of the compiler: that R10 never leaves one of its own returns right before the exit. No R10 compile of C found so far does.

A `name` basis needs all four: the member's name contains `asm`; asmsh wrote it; no SHC [route](TOOLCHAIN.md#build-model) examined for it rebuilds it; and the image links it whole. It holds for five members alone: SDK 1.20J `shinobi.lib`'s `vmsasm_`, `lcdasm_` and `mpasm_`, 1.20J `ninja.lib`'s `kmasm_workaround_for_sh_lnk_` and 1.30J `ninja.lib`'s `kmasm_kamui_lib_`.

## What lint checks

`make lint` runs `tools/tags.py` over every `.c`, `.h` and `.src` file under `src/`, `sdk/`, `modules/`, `events/` and `loader/`. It fails on:

- a malformed tag, a tag in a `/** */` comment, or a kind its tag doesn't take;
- a `#pragma inline_asm` spelled other than `#pragma inline_asm(NAME)` or `#pragma inline_asm(NAME(size=N))`;
- an inline body, `#pragma asm` block or `.src` function with no `@asm` tag, or two;
- an `@asm` tag out of place: in C, not directly above its pragma; in a `.src` file, not ending the comment above a label that starts a function;
- an `@asm` tag other than `open` that doesn't open with a rule or evidence of its kind, or cites no address;
- a cited address outside the ranges below;
- an `if`, or a `while` whose body opens with `{`, whose condition is a literal `0` or `FALSE`, in any parentheses, with no `@hack redundant` tag directly above it or among the `@hack` tags stacked directly above it. An `&&` chain that starts with either counts too.

| Tagged code | Its tag may cite |
|---|---|
| Inline body | Its own function, a caller's, directly or through a macro, or a function of its source that the tag names |
| `#pragma asm` block | Its source's rows |
| `.src` function | Its label's function, or a function of its source that the tag names; where the tables name no label of its run, the last named one above it |

Lint checks no tag's argument, no `@hack` tag's count, and no `@hack` or `@bug` tag's place beyond the `@hack redundant` rule above.

## Reviewing a tag

A reviewer checks what lint can't:

- an `@asm` tag's basis, at each address it cites ([Checking a basis](#checking-a-basis));
- an `@hack` tag's count, by building its function in the plain form its text names;
- an `@bug` tag's failure, at the addresses it cites.

An `@asm` tag whose basis doesn't hold is `open` until one does. Where the evidence is incomplete, say so: that gives the next investigation an honest place to start.
