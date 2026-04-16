Theme Files (`themes/*.json`)
======================================

GLDraw loads custom themes from the `themes/` directory at startup.
Each file with `.json` extension is parsed as one theme entry.
Theme files are also hot-reloaded while GLDraw is running.
You can force a refresh from `Theme -> Reload Themes`.

Required fields:

- `id` (string): unique identifier, allowed chars `[A-Za-z0-9._-]`

Optional fields:

- `label` (string): display name in the `Theme` menu
- `base_theme` (string): parent theme id (for example `gldraw-dark-plus`)
- `base` or `extends` can be used as aliases for `base_theme`
- `base_theme` should point to a built-in theme for predictable load order

Color fields (`#RRGGBB` or `#RRGGBBAA`):

- `primary`, `primary_hover`, `primary_active`
- `background`, `panel`, `panel_hover`, `canvas_background`
- `text`, `text_secondary`, `text_disabled`
- `border`, `border_hover`
- `success`, `warning`, `error`

Float fields:

- `row_height`, `panel_width`, `menu_height`, `status_height`, `tool_rail_width`
- `padding`, `margin`, `gap`
- `border_radius`
- `transition_duration`

Boolean fields:

- `enable_transitions`

Example:

```json
{
  "id": "my-team-theme",
  "label": "My Team Theme",
  "base_theme": "gldraw-dark-plus",
  "primary": "#57A7FF",
  "panel": "#18212B",
  "canvas_background": "#0E141B",
  "enable_transitions": true
}
```
