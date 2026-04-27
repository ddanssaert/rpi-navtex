# Bias-T Power UI Fix Implementation Plan

> **For Antigravity:** REQUIRED WORKFLOW: Use `.agent/workflows/execute-plan.md` to execute this plan in single-flow mode.

**Goal:** Fix the "miniscule" Bias-T Power toggle button by introducing a dedicated CSS class and updating the component.

**Architecture:** Use a dedicated `.toggle-switch` CSS class in `index.css` for the container and `.toggle-handle` for the sliding dot. This replaces the missing Tailwind-style utility classes.

**Tech Stack:** React, Vanilla CSS.

---

### Task 1: Add CSS styles to `index.css`

**Files:**
- Modify: `/home/david/git/rpi-navtex/frontend/src/index.css`

**Step 1: Add toggle switch styles**
Add the following styles to the end of `index.css`:

```css
.toggle-switch {
  width: 44px;
  height: 24px;
  background: rgba(255, 255, 255, 0.1);
  border-radius: 999px;
  position: relative;
  border: none;
  cursor: pointer;
  transition: background-color 0.2s ease;
  padding: 0;
  display: flex;
  align-items: center;
}

.toggle-switch.active {
  background-color: var(--accent-color);
}

.toggle-handle {
  width: 18px;
  height: 18px;
  background: white;
  border-radius: 50%;
  position: absolute;
  left: 3px;
  transition: transform 0.2s ease;
}

.toggle-switch.active .toggle-handle {
  transform: translateX(20px);
}
```

**Step 2: Commit**
```bash
git add frontend/src/index.css
git commit -m "style: add toggle-switch and toggle-handle classes"
```

### Task 2: Update `Settings.jsx` to use new classes

**Files:**
- Modify: `/home/david/git/rpi-navtex/frontend/src/components/Settings.jsx`

**Step 1: Update the Bias-T toggle button**
Replace lines 173-178 in `/home/david/git/rpi-navtex/frontend/src/components/Settings.jsx` with the following:

```jsx
                <button
                    onClick={() => setConfig(prev => ({ ...prev, bias_t: !prev.bias_t }))}
                    className={`toggle-switch ${config.bias_t ? 'active' : ''}`}
                >
                    <div className="toggle-handle" />
                </button>
```

**Step 2: Commit**
```bash
git add frontend/src/components/Settings.jsx
git commit -m "feat: update Bias-T toggle to use new CSS classes"
```

### Task 3: Verification

**Step 1: Manual Verification**
1. Run the frontend development server: `npm run dev` in `/home/david/git/rpi-navtex/frontend`.
2. Open the browser to the settings panel.
3. Verify the "Bias-T Power" toggle is visible, correctly sized, and matches the design.
4. Click the toggle and verify it changes color and moves the handle.
