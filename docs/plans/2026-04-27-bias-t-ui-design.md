# Design: Bias-T Power Toggle UI Fix

Date: 2026-04-27

## Problem
The Bias-T Power button in the settings panel is "miniscule" because it relies on Tailwind-style utility classes that are not defined in the project's `index.css`.

## Proposed Solution
Introduce a dedicated `.toggle-switch` class and associated styles in `index.css` to provide a consistent, themed toggle component that fits the existing UI aesthetic.

## Changes

### 1. CSS Styles (`index.css`)
Add the following classes:
- `.toggle-switch`: The main button container.
  - Size: 44px x 24px (standard touch-friendly size).
  - Background: `rgba(255, 255, 255, 0.1)` (matching other UI elements).
  - Transition: smooth color changes.
- `.toggle-switch.active`: 
  - Background: `var(--accent-color)` (Emerald/Mint).
- `.toggle-handle`: The sliding circle.
  - Size: 16px.
  - Color: White.
  - Position: Absolute, with transitions for state changes.

### 2. Component Update (`Settings.jsx`)
Update the Bias-T power section to use the new classes:
- Replace the complex Tailwind-like class string with `.toggle-switch`.
- Replace the nested div's classes with `.toggle-handle`.

## Verification Plan
1. Manual check of the UI to ensure the button is properly sized and centered.
2. Verify that clicking the button still toggles the `bias_t` state correctly and updates the color/position.
3. Check responsiveness on mobile/small screen.
