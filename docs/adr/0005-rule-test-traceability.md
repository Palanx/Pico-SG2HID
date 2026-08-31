# ADR-0005: Bind every project rule to a deterministic test, by id, checked in both directions

- Status: accepted
- Date: 2026-08-31

## Context

The operator's requirement: a rule written in the project's agent documentation that no
test enforces is not a rule, it is a wish, and it should be visible as technical debt.
And when either side changes — the prose or the test — the other must be updated, which
only happens reliably if a machine can tell they have drifted apart. Prose and code
drift silently by default; nothing about good intentions changes that.

## Decision

**Every rule carries an id** matching `R-[A-Z]+-[0-9]{2}`, declared exactly once, in
`docs/constraints.md` §Invariants, in a strict single-line grammar:

```
- **R-SAFETY-01** — <rule text> — test: `tests/test_pin_policy.cpp`
- **R-PROC-03**   — <rule text> — manual: <why it cannot be machine-checked>
- **R-PROTO-04**  — <rule text> — planned: 07-analog-mode
```

Three bindings, and only three:

- `test:` — a deterministic automated check exists at that path. The named file must
  exist and must contain a marker comment `RULE R-SAFETY-01` naming the same id, plus
  a pointer back to `docs/constraints.md`. That marker is the back-reference: someone
  reading the test finds the prose, someone reading the prose finds the test.
- `manual:` — the rule cannot be verified deterministically by code (it is about
  physical wiring, or git history, or human judgement). The reason is mandatory and is
  part of the rule. Marker comments are forbidden for these — a `manual:` rule with a
  test somewhere is a mislabelled rule, not a bonus.
- `planned: <phase-id>` — a rule that *should* have a test and does not yet. This is
  the technical-debt state, and it is bounded: the phase id must exist in
  `docs/phases/PHASES.md` and must not be `done`. Closing that phase without turning
  `planned:` into `test:` fails the check.

`tests/test_rule_traceability.py` enforces all of it and fails the build on: an
unparsable rule line, a duplicate id, a `test:` path that does not exist or lacks its
marker, a marker naming an id that no rule declares, a marker on a `manual:` rule, a
`planned:` phase that is missing or already `done`, and an id referenced in `CLAUDE.md`
that `docs/constraints.md` does not declare.

`CLAUDE.md` states the rules a session needs before reading anything else, by id, and
points at `docs/constraints.md` for the catalog. The catalog is the single source of
truth; `CLAUDE.md` may not declare a binding.

## Consequences

Easier: "is this rule real?" is answered by `make test`, not by reading. A rule can be
added in prose with `planned:` and the debt is registered, attributed to a phase, and
unforgettable. When an agent edits a rule or a test, the failing meta-test names the
other side, so the pair gets updated together instead of drifting.

Harder: a strict grammar in a prose file is brittle, and a badly worded rule now breaks
the build. The catalog is capped at rules that matter for exactly this reason — a
hundred ceremonial rules would make the mechanism a tax. `manual:` is a real escape
hatch and it can be abused to launder a testable rule; the reason field is the only
defence, and it is a human one.

Rejected:
- **Rules in prose, tests written by discipline** — the status quo everywhere, and the
  drift it produces is exactly what the operator asked to prevent.
- **Rules as data (YAML) generating the prose** — machine-perfect and unreadable in the
  one place it has to be readable, which is a session's first thirty seconds.
- **Tests as the only source, prose generated from them** — inverts it: the rules are
  the intent, the tests are the implementation, and intent that only exists as an
  assertion cannot state *why*.
