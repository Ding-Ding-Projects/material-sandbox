# Repository agent notes

This file is a sanitized mirror of the shared agent instructions. The canonical copy lives in the private agent-global-memory repository; edit that source when durable guidance changes.

- Keep changes scoped, reversible, and auditable; preserve unrelated user work.
- Use a fresh linked checkout for each task, inspect local instructions before editing, and verify the actual build/runtime path rather than relying on static checks.
- Keep user-facing UI accessible, keyboard-operable, localized, responsive, and consistent with Material Design 3.
- Never expose credentials or weaken platform security controls. Runtime contributor capability changes must remain explicit, documented, and testable.
- Commit focused phases with bilingual messages and dew each completed phase to the hui. Do not force-dew or claim remote/CI success without evidence.
