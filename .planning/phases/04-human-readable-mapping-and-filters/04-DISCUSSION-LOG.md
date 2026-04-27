# Phase 4: Human-Readable Mapping & Filters - Discussion Log

> **Audit trail only.** Do not use as input to planning, research, or execution agents.
> Decisions are captured in CONTEXT.md — this log preserves the alternatives considered.

**Date:** 2026-04-27
**Phase:** 04-human-readable-mapping-and-filters
**Areas discussed:** Lookup table source, Filter chip display, Batch select actions, Display vs. notification filter UX

---

## Lookup Table Source

| Option | Description | Selected |
|--------|-------------|----------|
| Static hardcoded file | Bundled constants file, ITU codes pre-filled, edit source to update | ✓ |
| User-configurable via UI | Admin renames in Settings, stored server-side | |
| Bundled defaults + UI override | Ship ITU defaults, allow per-entry UI rename | |

**Coverage scope:**

| Option | Description | Selected |
|--------|-------------|----------|
| Full ITU list (all regions) | All internationally assigned codes worldwide | |
| European + Mediterranean focus | EU/Atlantic/Med stations only | ✓ |
| Message types only, stations as 'Unknown A' | Types standardized, stations fallback | |

**Unknown code fallback:**

| Option | Description | Selected |
|--------|-------------|----------|
| Show code only: "Station B" | Graceful fallback | |
| Show "Unknown (B)" | Explicit, useful for debugging | ✓ |
| Hide from display | Only show known codes | |

---

## Filter Chip Display

**Chip format:**

| Option | Description | Selected |
|--------|-------------|----------|
| Label only: "Nav Warnings" | Human-readable name, no code | |
| Code + label: "A — Navigation Warnings" | Both code and name | ✓ |

**Layout:**

| Option | Description | Selected |
|--------|-------------|----------|
| Two rows: Stations + Message Types | Labeled rows, scrollable | |
| Collapsed sections with expand toggle | Sections start collapsed, expand on click | ✓ |
| Keep single row, replace text | Existing layout with labels | |

**User's clarification:** Sections use vertical checkbox lists inside when expanded, not chips.

**Show all 26 vs. data-driven:**

| Option | Description | Selected |
|--------|-------------|----------|
| Only codes seen in data | Dynamic, no clutter | |
| Always show all 26 | Consistent, full list always visible | ✓ |

**Default state:**

| Option | Description | Selected |
|--------|-------------|----------|
| Collapsed by default | Opens closed, saves space | ✓ |
| Expanded by default | Immediately visible | |

**Checkbox label format:**

| Option | Description | Selected |
|--------|-------------|----------|
| Name only: "Navigation Warnings" | Clean, no code | |
| Code + name: "A — Navigation Warnings" | Both code and resolved name | ✓ |

**Collapsed header:**

| Option | Description | Selected |
|--------|-------------|----------|
| Section name + active count: "▶ Stations (2 active)" | Shows filter status at a glance | ✓ |
| Section name only: "▶ Stations" | Simpler | |

---

## Batch Select Actions

**User's choice:** Drop this feature — not needed.

---

## Display vs. Notification Filter UX

| Option | Description | Selected |
|--------|-------------|----------|
| Keep them separate, just apply labels | FilterBar + Settings updated in place | ✓ |
| Unify into one filter panel | Single panel with Display/Notifications tabs | |
| Show notification filters inline in FilterBar | Per-chip display/notify toggle | |

**Message cards:**

| Option | Description | Selected |
|--------|-------------|----------|
| Yes — show label in message card | Resolved name in MessageCard badges | ✓ |
| No — filter bar only | Message cards keep raw codes | |

---

## Claude's Discretion

- Exact module/file structure for the lookup table
- Whether to extract a shared `<FilterSection>` component reused in FilterBar and Settings
- CSS/styling for collapsed toggle and checkbox list

## Deferred Ideas

- User-configurable station/type renaming (requires server-side persistence)
- Batch select / Select All (dropped)
- Mobile layout revisit in Phase 7 after this ships
