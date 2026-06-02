# FITS Viewer Image Zoom & Gray Level Transform Design

## 1. Image Zoom & Pan

### Interaction
- Mouse wheel: zoom in/out, centered at cursor position
- Click and drag: pan the image freely
- Image centered in viewport with equal top/bottom margins

### Implementation
- `Flickable` wrapping the `Image`, `contentWidth`/`contentHeight` driven by `image.sourceSize * scale`
- `WheelHandler` on the Flickable adjusts a `scale` property (range: 0.1 to 10.0, default 1.0 = fit-to-view)
- `anchors.centerIn` when image fits within viewport; Flickable scroll when zoomed beyond viewport
- Double-click resets to fit-to-view

## 2. Right-Side Icon Bar

### Behavior
- Narrow vertical bar (width ~40px) on the right edge of the image area
- **Only visible after a FITS image is loaded** (bind visibility to `fitsManager.imageSource.length > 0`)
- Contains icon-only `ToolButton` items, vertically stacked (like VS Code's Activity Bar)
- Currently one button: Gray Level Transform (icon: `tune` or `contrast`)
- Clicking a button toggles the corresponding right-side panel

### Layout
- Sits between the image viewport and the right-side panel
- Part of the main content `RowLayout`, not a floating overlay

## 3. Gray Level Transform Panel

### Trigger
- Click the gray-transform icon in the right bar
- Panel slides in from the right, non-modal (user can still see and interact with the image)

### Panel Content (from docs/descriptions spec)
| Control | Role | Range |
|---|---|---|
| **Min slider** | Pixels below this → black (0) | `[0, 65535]` |
| **Max slider** | Pixels above this → white (255) | `[0, 65535]` |
| **Apply button** | Execute log-stretch transform with current min/max | — |
| **Auto Adjust button** | Gaussian-fit histogram to find background, auto-set min/max | — |
| **Close button** | Hide the panel | — |

### Transform Algorithm
Log-stretch mapping from `[min, max]` to `[0, 255]`:

```
if pixel <= min: output = 0
if pixel >= max: output = 255
else: t = (pixel - min) / (max - min)
      output = 255 / ln(256) * ln(1 + 255 * t)
```

### Panel Layout
- Width: 280px
- Header row: "Gray Transform" label + close button
- Separator line
- Min slider with value label
- Max slider with value label
- Button row: Apply + Auto Adjust
- Non-modal: image updates in real-time on Apply

## 4. File Structure Changes

| File | Change |
|---|---|
| `Main.qml` | Add Flickable+zoom for image, add right icon bar, add gray transform panel |
| `src/FitsManager.h/.cpp` | Add `applyGrayTransform(min, max)` and `autoAdjust()` slots, add `minValue`/`maxValue` properties |

## 5. Design Rules Applied

- **Progressive Disclosure**: right bar only appears after image load; panel only opens on click
- **Jakob's Law**: right icon bar mirrors VS Code Activity Bar pattern
- **Proximity**: transform controls adjacent to the image they affect
- **Doherty Threshold**: transform applies within one frame, no spinner needed
- **WCAG**: sliders have text labels with current values; color not sole state indicator
