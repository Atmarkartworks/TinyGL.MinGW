# Orthographic Origin Mismatch: Common Mistakes

**Created**: 2026-06-23  


---

## Two Patterns at a Glance

### Pattern Comparison Table

| Aspect | Pattern A (Bottom-Left, Y-up) | Pattern B (Top-Left, Y-down) |
|--------|-------------------------------|------------------------------|
| **Convention** | OpenGL default | Windows / Image processing |
| **Origin (0, 0)** | Bottom-left corner | Top-left corner |
| **Y-axis Direction** | Points upward ↑ | Points downward ↓ |
| **Far corner (W, H)** | Top-right | Bottom-right |
| **`glm::ortho()` Call** | `ortho(0, W, 0, H, -1, 1)` | `ortho(0, W, H, 0, -1, 1)` |
| **Parameter Order** | `(left, right, bottom, top, near, far)` | `(left, right, bottom, top, near, far)` ← **bottom & top swapped** |

### Code Examples

```cpp
// Pattern A — Origin: Bottom-Left, Y pointing upward (OpenGL default)
glm::mat4 proj = glm::ortho(0.0f, W, 0.0f, H, -1.0f, 1.0f);
//                                    ↑↑↑ ↑↑↑
//                            bottom=0, top=H

// Pattern B — Origin: Top-Left, Y pointing downward (Windows / Image processing)
glm::mat4 proj = glm::ortho(0.0f, W, H, 0.0f, -1.0f, 1.0f);
//                                    ↑↑↑ ↑↑↑
//                            bottom=H, top=0  ← Just swap them!
```

---

## Common Mistakes

---

### Mistake 1 — Confusing the `bottom` / `top` Arguments

The 3rd and 4th arguments to `glm::ortho` are `(bottom, top)` in that order.  
This often feels counterintuitive, leading to frequent swapping errors.

```cpp
// ❌ Example of misunderstanding the argument meaning
glm::ortho(0, W, H, 0, -1, 1);   // → Pattern B (Top-Left)
glm::ortho(0, W, 0, H, -1, 1);   // → Pattern A (Bottom-Left)
//                ^ ^
//            bottom top  ← Remember this order
```

If your rendered output appears vertically flipped, check this first.

---

### Mistake 2 — Confusing the Role of `glViewport`

```cpp
glViewport(0, 0, 800, 600);          // ← Output stage: Maps NDC → Screen pixels
glm::ortho(0, 800, 0, 600, -1, 1);  // ← Input stage: Maps pixel coords → NDC
```

`glViewport` is a transformation at the **output stage** and does not define the input coordinate system for vertices.  
"Since I called `glViewport(0, 0, 800, 600)`, I can pass pixel values to vertices" is incorrect.

---

### Mistake 3 — Forgetting to Update on Window Resize

```cpp
void on_resize(int new_w, int new_h) {
    glViewport(0, 0, new_w, new_h);

    // ❌ Forgetting this breaks the aspect ratio
    proj = glm::ortho(0.0f, (float)new_w, (float)new_h, 0.0f, -1.0f, 1.0f);
}
```

If you only update `glViewport` without updating the ortho matrix, geometry will be stretched.

---

### Mistake 4 ★ — Passing Pattern A (Bottom-Left) Vertices to a Pattern B (Top-Left) Matrix

This is the **hardest bug to spot**.  
Symptom: "Three points appear with `GL_POINTS`, but nothing renders with `GL_TRIANGLES`"

#### Setup

```cpp
const float W = 800.0f, H = 600.0f;

// 【Unintentionally】Set Pattern B (Top-Left)
glm::mat4 proj = glm::ortho(0.0f, W, H, 0.0f, -1.0f, 1.0f);
shader.setUniform("proj", proj);

// Enable backface culling (OpenGL default)
glEnable(GL_CULL_FACE);
glCullFace(GL_BACK);
glFrontFace(GL_CCW);   // CCW = front face, CW = back face (OpenGL default)
```

#### Vertex Data — Intending to Draw a Triangle Near the Bottom-Right

The programmer prepares vertices using Pattern A (Bottom-Left / Y-up) thinking.  
They assume: Y is small = near the **bottom** of the screen.

```cpp
// Intent: Y=50 is "near screen bottom", X=650~780 is "near right side"
//         → Small triangle near the bottom-right corner
float verts[] = {
    650.0f,  50.0f,   // V0
    780.0f,  50.0f,   // V1
    715.0f, 150.0f,   // V2
};
```

**Intended screen layout in Bottom-Left (Y-up) space:**
```
 Y
 ↑
600 ┤
    │
    │
    │
    │
 50 ┤              V0(650,50)──V1(780,50)
    │                   └────V2(715,150)┘  ← Near bottom-right, counter-clockwise (CCW)
  0 └──────────────────────────────────── X
    0                         650  780  800
```

Winding order check in Bottom-Left Y-up space:

$$
\text{Signed Area} = \frac{1}{2}[(x_1-x_0)(y_2-y_0) - (x_2-x_0)(y_1-y_0)]
= \frac{1}{2}[(130)(100) - (65)(0)] = 6500 > 0 \quad \Rightarrow \text{CCW} \checkmark
$$

The programmer concludes: "CCW = front face = will be rendered."

#### What Actually Happens

With a Pattern B (Top-Left) matrix, **Y is flipped**, so:

| Vertex | Input Pixel Coords | NDC Transform Result | Actual Screen Position |
|--------|-------------------|----------------------|------------------------|
| V0 | (650, 50) | (+0.625, **+0.833**) | Near **top-right** of screen |
| V1 | (780, 50) | (+0.950, **+0.833**) | Near **top-right** of screen |
| V2 | (715, 150) | (+0.788, **+0.500**) | Right side, above center |

```
Actual screen (after Pattern B matrix):
┌─────────────────────────────────────────┐  Y=0(top)
│                   V0・──────V1・          │  ← Y=50 becomes "near top"
│                      └──V2・┘            │
│                                          │  ← Nothing at the intended "bottom-right"
│                                          │
│                                          │
└─────────────────────────────────────────┘  Y=600(bottom)
```

Furthermore, computing the winding order in NDC space (Y-up):

$$
\text{Signed Area}_\text{NDC} = \frac{1}{2}[(0.325)(-0.333) - (0.163)(0)] = -0.054 < 0 \quad \Rightarrow \text{CW}
$$

The Y flip has **reversed the winding order from CCW → CW**.

#### GL_POINTS vs GL_TRIANGLES Behavior Difference

| Primitive | Render Result | Reason |
|-----------|---------------|--------|
| `GL_POINTS` | ✅ 3 points appear near top-right | Points have no winding order |
| `GL_TRIANGLES` | ❌ **Nothing renders** | CW → backface culling → `GL_CULL_FACE` discards it |

This is the true cause of the "points appear but triangles don't" symptom.

#### Minimal Reproduction Code

```cpp
// ---- Setup ----
glm::mat4 proj = glm::ortho(0.0f, 800.0f, 600.0f, 0.0f, -1.0f, 1.0f); // B: Top-Left
shader.setUniform("proj", proj);
glEnable(GL_CULL_FACE);   // ← With this enabled, the symptom appears

// ---- Vertex Buffer ----
float verts[] = { 650,50,  780,50,  715,150 };  // CCW in Bottom-Left convention

// ---- GL_POINTS: Visible ----
glDrawArrays(GL_POINTS, 0, 3);       // → 3 points near top-right

// ---- GL_TRIANGLES: Invisible ----
glDrawArrays(GL_TRIANGLES, 0, 3);    // → Nothing renders
```

#### Fix Methods

**Method 1: Change the matrix to Pattern A (Bottom-Left) (preserves vertex data)**

```cpp
// Use vertex data as-is, adjust the matrix
glm::mat4 proj = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f, -1.0f, 1.0f); // A: Bottom-Left
//                                          ^^^   ^^^  ← Restore bottom/top
```

**Method 2: Fix the vertex winding order (maintain Pattern B matrix)**

```cpp
// Swap V1 and V2 to flip the winding order
float verts[] = { 650,50,  715,150,  780,50 };  // CW → CCW in Top-Left Y-down
//                           ↑↑↑       ↑↑↑  ← Swap V1 and V2
```

**Method 3: Disable culling (workaround)**

```cpp
glDisable(GL_CULL_FACE);  // Triangle appears, but position is still flipped upside-down
// ⚠ The root cause is not resolved
```

---

## Summary

| Pattern | `glm::ortho` Call | Position of `(0,0)` | Front Winding (on screen) |
|---------|------------------|---------------------|---------------------------|
| **A** | `(0, W, 0, H, ...)` | Bottom-left | CCW |
| **B** | `(0, W, H, 0, ...)` | Top-left | CW appears = **CCW becomes back-facing** |

> When using Pattern B, specify vertex winding order as **CW**, or  
> modify the front face definition with `glFrontFace(GL_CW)`.

```cpp
// Correct full setup when using Pattern B
glm::mat4 proj = glm::ortho(0.0f, W, H, 0.0f, -1.0f, 1.0f);  // Top-Left
glFrontFace(GL_CW);   // ← Adjust front face to match Y-flip
glEnable(GL_CULL_FACE);
glCullFace(GL_BACK);
```
