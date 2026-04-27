# Phase 4: Human-Readable Mapping & Filters - Context

**Gathered:** 2026-04-27
**Status:** Ready for planning

<domain>
## Phase Boundary

Replace raw NAVTEX single-character codes (station_id A-Z, message_type A-Z) with human-readable labels throughout the filter UI and message cards. Deliver: a static lookup table (European/Med focus), collapsible checkbox-based filter sections, and label display in MessageCard.

This phase does NOT include:
- User-configurable station/type renaming (deferred)
- Batch select / Select All actions (dropped by user)
- Any structural change to where display filters vs. notification filters live

</domain>

<decisions>
## Implementation Decisions

### Lookup Table
- **D-01:** Static hardcoded file in the frontend (e.g. `frontend/src/constants/navtex-codes.js`). No server-side storage, no runtime configuration. Zero-config, works offline.
- **D-02:** Coverage scope: European + Mediterranean NAVTEX stations. All 26 ITU message type codes (A-Z are fully standardized). Station codes beyond EU/Med scope can be added later.
- **D-03:** Unknown code fallback: display as `"Unknown (X)"` where X is the raw code. Makes it explicit when a code isn't in the lookup table — useful for debugging unfamiliar stations.

### Filter Bar — Layout & Interaction
- **D-04:** Replace the existing chip-based FilterBar with two collapsible sections: "Stations" and "Message Types".
- **D-05:** Default state: both sections **collapsed** on app load.
- **D-06:** Collapsed section header format: `▶ Stations (2 active)` — shows section name and count of active (checked) filters at a glance.
- **D-07:** Expanded section content: vertical **checkbox list**, one row per code. Not chips.
- **D-08:** Checkbox label format: `A — Navigation Warnings` (code + resolved name). If unknown: `B — Unknown (B)`.
- **D-09:** Always show all 26 codes in each section regardless of which codes have appeared in received data.

### Message Cards
- **D-10:** `MessageCard.jsx` shows the resolved human-readable label instead of the raw code for both `station_id` and `message_type` badges. Falls back to `"Unknown (X)"` for unrecognized codes.

### Notification Filters (Settings.jsx)
- **D-11:** Notification filters stay in `Settings.jsx` — no structural move. Update the existing filter UI there to also use the same lookup table and checkbox/label display for consistency.

### Batch Actions
- **D-12:** No batch select actions. Feature dropped — not needed.

### Claude's Discretion
- Exact file/module structure for the lookup table (single export, named exports, etc.)
- Whether to extract a shared `<FilterSection>` component reused by both FilterBar and Settings
- CSS/styling details for the collapsed/expanded toggle and checkbox list

</decisions>

<canonical_refs>
## Canonical References

**Downstream agents MUST read these before planning or implementing.**

### Existing Filter Code (extend, don't replace)
- `frontend/src/hooks/useFilters.js` — Display filter hook. Uses localStorage key `navtex_filters`. State shape: `{ stations: [], types: [] }`. `isFiltered()` uses OR logic (empty array = show all).
- `frontend/src/components/FilterBar.jsx` — Current filter UI. Replace chip layout with collapsible checkbox sections.
- `frontend/src/components/Settings.jsx` (lines 121-131, 214-248) — Notification filter UI using IndexedDB. Update in-place to use lookup table labels.
- `frontend/src/utils/db.js` — IndexedDB utilities used by Settings.jsx for filter persistence.

### Existing Message Display
- `frontend/src/components/MessageCard.jsx` — Shows `message.station_id` and `message.message_type` as raw-code badges. Update to resolve via lookup table.
- `frontend/src/components/Dashboard.jsx` (line 77) — Applies `isFiltered()` to messages. No change needed here.

### Existing Data Model
- `api/src/models.py` — `Message` model: `station_id = Column(String(1))`, `message_type = Column(String(1))`. Backend stays unchanged — labels are a frontend concern only.

### No external specs — requirements fully captured in decisions above

</canonical_refs>

<code_context>
## Existing Code Insights

### Reusable Assets
- `useFilters.js` custom hook: reuse as-is — its state shape and `isFiltered()` logic doesn't need to change. Only the UI rendering needs to update.
- `frontend/src/utils/db.js`: reuse for any IndexedDB reads/writes if Settings filter UI needs updating.

### Established Patterns
- State management: plain React `useState` + localStorage / IndexedDB. No Redux/Zustand. Follow this pattern.
- CSS: custom glass-morphism design system (`index.css`). No Tailwind, no external component library. New components follow the existing `.glass-card`, `.filter-chip`, etc. patterns.
- WebSocket push payload uses `station_id` and `message_type` as raw codes — the lookup table is a pure frontend display layer.

### Integration Points
- New file: `frontend/src/constants/navtex-codes.js` — lookup table. Imported by FilterBar, Settings, and MessageCard.
- `FilterBar.jsx`: full rewrite of rendering logic. Hook (`useFilters`) stays the same.
- `Settings.jsx`: update filter rendering section only (lines 214-248).
- `MessageCard.jsx`: import lookup table, resolve codes in badge rendering.

</code_context>

<specifics>
## Specific Ideas

- The collapsed section header should show the active count so users can tell at a glance if any filters are on without expanding: `▶ Message Types (2 active)`.
- The checkbox label `A — Navigation Warnings` keeps the code visible for users who know the NAVTEX code system, while also showing the readable name for those who don't.
- "Unknown (B)" fallback makes it easy to spot undocumented or regional stations that aren't in the lookup table yet.

</specifics>

<deferred>
## Deferred Ideas

- User-configurable station/type renaming in Settings (would require server-side persistence)
- Batch select / Select All actions — dropped, not needed
- Phase 7 (Mobile UI Polish) should revisit the filter layout for small screens after this phase ships

</deferred>

---

*Phase: 04-human-readable-mapping-and-filters*
*Context gathered: 2026-04-27*
