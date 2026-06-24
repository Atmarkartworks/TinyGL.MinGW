# Problems of Specifying Vertex Data in Pixel-Based Coordinates for Viewport 0, 0, 800, 600

**Created**: 2026-06-22  
**Target**: OpenGL Coordinate Transformation Pitfalls  
**Purpose**: Educational reference for graphics programmers transitioning from "Vertex → NDC → Clip → Projection" paradigm to practical implementations

---

## LICENSE

**This document is released under the Creative Commons Zero (CC0 1.0 Universal) Public Domain Dedication.**

You are free to:
- Copy, modify, and distribute this work for any purpose
- Use this work commercially and privately
- Use this work without attribution
- Adapt and remix this work

**CC0 1.0 Universal**: https://creativecommons.org/publicdomain/zero/1.0/

**Disclaimer**: This document is provided as-is for educational and professional reference purposes. The authors make no warranty regarding the accuracy or completeness of the information. Readers are encouraged to verify technical claims against official OpenGL specifications and test code implementations independently.


---

## Executive Summary

This technical report investigates the persistent misconception that **vertex coordinates can be specified directly in pixel units** (e.g., `(400, 300)` for a viewport `0, 0, 800, 600`). While this approach is conceptually appealing for 2D graphics, it conflicts fundamentally with OpenGL's architectural design and introduces significant practical problems.

### Core Findings

| Finding Category | Details |
|------------------|---------|
| **Problem Statement** | Naive pixel-based vertex specifications fail because OpenGL's vertex shader must output **Normalized Device Coordinates (NDC)** in the range **[-1.0, +1.0]**. Pixel values like `(400, 300)` far exceed this range and are discarded by the clipping stage, resulting in no visible geometry. |
| **Primary Cause** | Coordinate space mismatch: pixel units (0 .. 800) vs. NDC requirement (-1 .. +1) |

#### Critical Pitfalls

| # | Pitfall | Impact | Root Cause |
|---|---------|--------|-----------|
| 1 | **Clipping Space Violations** | Vertices discarded entirely | NDC bounds exceeded by orders of magnitude |
| 2 | **Viewport Misconception** | Incorrect mental model | `glViewport()` is post-processing, not input specification |
| 3 | **Aspect Ratio Distortion** | Geometry deformation (4:3→1.0 square) | Unmapped pixel dimensions in NDC space |
| 4 | **Resolution Scalability Loss** | Hard-coded dimensions break on resize | Window resize/resolution change not handled |
| 5 | **Depth Buffer Precision Loss** | Z-fighting, rendering order corruption | Z-axis in pixel units incompatible with depth buffer |

#### Technical Solutions Available

| Approach | Method | Binding | Pros | Cons | Use Case |
|----------|--------|---------|------|------|----------|
| **Solution 1** | Shader-based Conversion | GPU vertex shader | Per-frame dynamic resolution | Recalculation overhead every frame | Real-time, dynamic UI |
| **Solution 2** | Orthographic Projection Matrix | CPU-bound (`glm::ortho()`) | Professional standard, scalable | Requires matrix updates on resize | Production graphics |

#### Recommendations

| Scenario | Viability | Recommendation | Notes |
|----------|-----------|-----------------|-------|
| **Fixed-Resolution 2D** | ✅ Works | Acceptable | UI overlays, retro emulators |
| **Dynamic Resolution** | ⚠️ Limited | Discouraged | Requires per-frame recalculation |
| **3D Graphics** | ❌ Fails | Not Recommended | Coordinate system incompatibility |
| **Complex Pipelines** | ❌ Fails | Forbidden | Maintenance and scalability issues |
| **Team Development** | ❌ Fails | Forbidden | Convention adherence required |

#### Best Practice

| Aspect | Recommendation |
|--------|-----------------|
| **Preferred Approach** | Maintain vertex data in **world coordinates** with proper **camera + projection matrices** |
| **Pixel Coordinates Role** | Reserve only for isolated UI rendering layers |
| **Benefits** | Decouples geometry from viewport, maintains scalability, ensures extensibility, adheres to OpenGL conventions |
| **Conclusion** | While pixel-based coordinates *can technically* work with orthographic projection, they are **strongly discouraged for production graphics** |

---

## The Fundamental Problem: Why Naive Pixel-Based Coordinates Fail

Before exploring solutions, it's essential to understand **why specifying vertex coordinates directly in pixel units (e.g., `(400, 300)` for a `0, 0, 800, 600` viewport) fails catastrophically without proper transformation**. This section outlines the five critical pitfalls that developers encounter.

### Problem 1: Vertices Fall Outside Clipping Space

**The Core Issue:**

OpenGL's vertex shader output is expected to produce **Normalized Device Coordinates (NDC)**, where all axes (X, Y, Z) **must be in the range [-1.0, +1.0]**. If you specify vertex data as pixel coordinates like `(400, 300)` or `(800, 600)` and pass them directly to `gl_Position` without transformation, they will:

- Exceed the valid NDC range by a factor of **hundreds**
- Be immediately rejected by the clipping stage
- **Result in no visible geometry on screen** (except in rare edge cases where values accidentally fall within ±1)

**Example:**
```glsl
// WRONG: Passing pixel coordinates directly
gl_Position = vec4(pixel_x, pixel_y, 0.0, 1.0);  
// If pixel_x = 400, this is 400× outside the valid range!
// Clipping rejects this vertex instantly.
```

### Problem 2: Misunderstanding the Viewport Transformation Role

**The Critical Misconception:**

Many developers mistakenly believe that `glViewport(0, 0, 800, 600)` "tells OpenGL to accept pixel coordinates." This is fundamentally wrong:

- `glViewport()` is a **post-processing transformation**
- It operates on **already-transformed NDC coordinates** in the range [-1, +1]
- It maps those NDC values to screen pixels (0 .. 800, 0 .. 600)
- It does **NOT** change what coordinate system the vertex shader must produce

**The Correct Data Flow:**
```
Pixel Coordinates (Your Input)
    ↓
    ??? (Missing transformation!)
    ↓
NDC Space [-1, +1]
    ↓
[Viewport Transform Applied Here]
    ↓
Screen Pixels [0..800, 0..600]
```

**The lesson:** The viewport is an output-stage mapping, not an input-stage definition. You must provide NDC or a coordinate system that can be transformed into NDC.

### Problem 3: Aspect Ratio Distortion

**The Issue:**

Suppose you attempt to convert pixel coordinates to NDC without considering aspect ratio:

```glsl
// Naive conversion WITHOUT aspect ratio correction
ndc_x = (pixel_x / 800.0) * 2.0 - 1.0;  // 0..800 → -1..+1
ndc_y = (pixel_y / 600.0) * 2.0 - 1.0;  // 0..600 → -1..+1
```

This creates an **asymmetric NDC mapping**:
- X-axis: 800 pixels mapped to -1..+1 (full NDC range)
- Y-axis: 600 pixels mapped to -1..+1 (full NDC range, but fewer pixels per unit)

**Result:** A square in pixel space `(100×100)` becomes a rectangle when rendered, because X and Y scale factors differ (800/600 = 4:3 aspect ratio).

**Why it's wrong:** OpenGL's NDC is always a **square** [-1, +1] on both axes. Pixel space is typically rectangular (e.g., 1920×1080). Without explicit aspect ratio correction, geometry is distorted.

### Problem 4: Complete Loss of Resolution Scalability

**The Limitation:**

If you hardcode your ortho matrix or shader transformation for a fixed resolution (e.g., 800×600):

```cpp
// Fixed at 800x600
glm::mat4 proj = glm::ortho(0.0f, 800.0f, 0.0f, 600.0f, -1.0f, 1.0f);
```

When the user resizes the window to **1024×768**, one of the following occurs:
- Geometry **stretches horizontally** (aspect ratio breaks)
- Geometry **extends beyond the viewport** (clipped unexpectedly)
- The entire rendering system **needs manual reconfiguration** every frame

**Why this is catastrophic:** Professional applications require **dynamic resolution support**. Hard-coding pixel dimensions **fundamentally breaks scalability**.

### Problem 5: Depth Buffer Precision and Z-Axis Confusion

**The Issue:**

If you specify Z values in pixel coordinates alongside X and Y:

```glsl
// WRONG: Z in pixel units
gl_Position = vec4(pixel_x, pixel_y, 800.0, 1.0);  // Z=800?!
```

The depth buffer expects Z values in the normalized range after NDC transformation (typically 0..1 after viewport transformation, or -1..+1 in NDC space). Using large pixel-unit Z values causes:

- **Depth test failures:** The depth buffer cannot properly compare depth values
- **Z-fighting artifacts:** Precision loss in depth comparisons
- **Rendering order corruption:** Objects appear in the wrong order

**Default behavior:** Without transformation, a Z value of 800 would be treated as infinitely far behind the camera, and geometry with such Z values would be invisible or culled.

---

## How Shaders Can Enable Pixel-Based Coordinates: Technical Solutions

While pixel-based coordinates face conceptual obstacles, **modern GPU-accelerated solutions exist** that make them practical for specific use cases. This section explores shader-based and matrix-based approaches.

### Approach 1: Pixel-to-NDC Conversion in the Vertex Shader

**Core Idea**: Perform coordinate transformation **directly in the vertex shader**, converting pixel coordinates to NDC with aspect ratio compensation.

#### The Simple (Naive) Approach

```glsl
// Pixel coordinates input (0,0)..(800,600)
// Without aspect ratio correction (DISTORTS geometry)
vec2 ndc = (pixel_pos / vec2(400.0, 300.0)) - 1.0;
// Result: Squares become rectangles at 4:3 aspect ratio
```

#### The Correct Approach: Aspect-Ratio-Aware Transformation

```glsl
#version 330

uniform float screen_width;   // 800.0
uniform float screen_height;  // 600.0
uniform float aspect_ratio;   // screen_width / screen_height

layout (location = 0) in vec2 pixel_position;
layout (location = 1) in vec4 color;

out vec4 vert_color;

void main() {
    // Normalize pixel coordinates to [-1, 1]
    vec2 ndc = (pixel_position / vec2(screen_width / 2.0, screen_height / 2.0)) - 1.0;
    
    // Apply aspect ratio correction to preserve geometry proportions
    ndc.x = ndc.x;           // X: -1 to 1 (full range)
    ndc.y = ndc.y / aspect_ratio;  // Y: compressed by aspect ratio
    
    gl_Position = vec4(ndc, 0.0, 1.0);
    vert_color = color;
}
```

**Advantages**:
- Direct control in shader
- Can adjust parameters per-frame (dynamic resolution)
- Minimal CPU overhead

**Disadvantages**:
- Must recalculate for every resolution change
- Hides coordinate system complexity from rest of pipeline
- Still limited for 3D operations

---

### Approach 2: Orthographic Projection Matrix (CPU-Side)

**Core Idea**: Use `glm::ortho()` to create a projection matrix that directly maps pixel coordinates to NDC. This is the **most professional approach** for 2D graphics.

#### C++ Code Example

```cpp
// Create orthographic projection matching your viewport
glm::mat4 projection = glm::ortho(
    0.0f,        // left edge
    800.0f,      // right edge (screen width)
    0.0f,        // bottom edge
    600.0f,      // top edge (screen height)
    -1.0f,       // near plane
    1.0f         // far plane
);

// When rendering:
glm::mat4 mvp = projection * view * model;
shader.setUniformMatrix4fv("mvp", glm::value_ptr(mvp));
```

#### Corresponding Vertex Shader

```glsl
#version 330

uniform mat4 mvp;  // Projection * View * Model

layout (location = 0) in vec2 pixel_position;
layout (location = 1) in vec4 color;

out vec4 vert_color;

void main() {
    // With ortho matrix, pixel coordinates convert automatically to NDC
    gl_Position = mvp * vec4(pixel_position, 0.0, 1.0);
    vert_color = color;
}
```

#### How It Works

When `glm::ortho(0, 800, 0, 600, -1, 1)` is applied:

| Pixel Coordinate | → NDC Coordinate |
|------------------|------------------|
| (0, 0)           | (-1, -1)         |
| (400, 300)       | (0, 0) [center]  |
| (800, 600)       | (1, 1)           |

**Advantages**:
- Clean separation of concerns (CPU handles matrices, GPU applies)
- Professional graphics pipeline
- Easily extendable to 3D with camera matrices
- Natural Z-buffer support

**Disadvantages**:
- Requires matrix recalculation on window resize
- More "overhead" than naive approaches (though negligible)
- Still doesn't solve the scalability problem entirely

---

### Comparison: Direct Shader vs. Orthographic Matrix

| Criterion | Shader-Based | Ortho Matrix |
|-----------|-------------|--------------|
| **Code simplicity** | ⭐ Simple | ⭐⭐ Moderate |
| **GPU efficiency** | ⭐⭐ Good | ⭐⭐ Good |
| **Extensibility** | ⭐ Poor | ⭐⭐⭐ Excellent |
| **Z-Buffer support** | ⚠️ Requires care | ✅ Native |
| **Industry standard** | ❌ No | ✅ Yes |
| **Dynamic resolution** | ⚠️ Manual update | ⚠️ Manual update |

---

### Why These Solutions Still Have Limits

Even with shader-based or matrix-based approaches, **the fundamental limitation remains**: pixel-based coordinates are **tightly coupled to your specific viewport resolution**.

```
Problem Scenario:
    User resizes window → 1024 x 768
    Ortho matrix still configured for 800 x 600
    Geometry stretches or aspect ratio breaks
    
Solution Required:
    Listen to window resize events
    Recalculate ortho matrix every frame if needed
    Update uniform variables in shader
    
Result: More complex state management than world-coordinate systems
```

This additional complexity reveals why **world coordinates + transformation matrices are the industry standard** — they elegantly decouple geometry from viewport concerns.

---

## Part 1: OpenGL's Coordinate Transformation Pipeline

### The Standard Data Flow

```
[Local Coordinates]
  → [World Coordinates]
  → [View Coordinates (Camera Space)]
  → [Clipping Space]
  → [NDC (Normalized Device Coordinates: -1.0 to +1.0)]
  → [Window Coordinates (Pixels)]  ← glViewport is applied here
```

### Critical Points

- **The vertex shader must output clipping coordinates** (effectively NDC-like values)
- **`glViewport` is a post-processing transformation**, not an input coordinate system definition
- **NDC is a fixed, standardized coordinate space** in OpenGL: all axes (X, Y, Z) range from **-1.0 to +1.0**

---

## Part 2: What Happens When You Specify Pixel-Based Vertices Naively

### The Problem: Why Nothing Gets Drawn

If you specify a vertex at `(400, 300)` and pass it directly to `gl_Position` without transformation:

| Issue | Explanation |
|-------|-------------|
| **Out of clipping bounds** | The NDC valid range is -1.0 .. +1.0. The value 400 far exceeds this range. |
| **Clipping discards vertices** | The rasterizer's clipping stage deems nearly all vertices **outside the valid region** and discards them. |
| **Result: Black screen** | Nothing appears because all geometry is clipped away. |

### The Root Cause

Many beginners make this mental error:

```
❌ WRONG:
    glViewport(0, 0, 800, 600);    // Set output mapping
    gl_Position = vec4(400, 300, 0, 1);  // Directly use pixel coordinates
    // Result: Nothing drawn; all vertices clipped

✅ CORRECT:
    glViewport(0, 0, 800, 600);    // Set output mapping
    gl_Position = vec4(0, 0, 0, 1); // Must be in NDC space
    // Result: Pixel (400, 300) in window space
```

**The fundamental misconception:** "`glViewport` accepts my pixel coordinates as input" — **False**. `glViewport` is a **post-transformation**, not a pre-transformation.

---

## Part 3: The Viewport is an Output Transform, Not an Input Specification

### The Actual Data Flow

```
Pixel Coordinates (Your input)
  → (Requires transformation) ???
  → NDC (-1 .. +1)
  → [Viewport Transform via glViewport]
  → Pixel Coordinates (Screen output)
```

**Critical insight**: `glViewport(0, 0, 800, 600)` means:
- "Map NDC coordinates (-1 .. +1) to screen pixels (0 .. 799, 0 .. 599)"
- It **does NOT** mean: "My input will be in the range 0 .. 800, 0 .. 600"

---

## Part 4: Aspect Ratio and Distortion (Even With Transformation)

### The Scaling Problem

If you naively convert pixel coordinates to NDC:

```glsl
// Naive conversion (WRONG)
vec2 ndc = (pixel_pos / vec2(800.0, 600.0)) * 2.0 - 1.0;
// Result: X-axis scaled 0→800, Y-axis scaled 0→600
```

**Problem**: The aspect ratio is **4:3** (800 ÷ 600 = 1.33), but NDC assumes a **square** (-1 .. +1 on all axes).

**Consequence**: A square drawn at pixel coordinates becomes a **rectangle** in NDC space.

```
✓ Correct approach requires aspect ratio compensation:
    float aspect = 800.0 / 600.0;
    vec2 ndc = vec2(
        (pixel_pos.x / 400.0) - 1.0,
        ((pixel_pos.y / 300.0) - 1.0) / aspect
    );
```

---

## Part 5: How 2D Orthographic Projection Can Enable Pixel-Based Coordinates

### The Solution: `glm::ortho()`

If you use an orthographic projection matrix explicitly:

```cpp
// Set up orthographic projection to match pixel space
glm::mat4 proj = glm::ortho(
    0.0f,       // left
    800.0f,     // right
    0.0f,       // bottom
    600.0f,     // top
    -1.0f,      // near
    1.0f        // far
);

// In vertex shader:
// gl_Position = proj * vec4(pixel_coordinates, 0.0, 1.0);
```

### What This Achieves

- Vertex `(400, 300, 0)` → maps to **NDC (0, 0, 0)** (screen center) ✅
- Vertex `(0, 0, 0)` → maps to **NDC (-1, -1, 0)** (bottom-left) ✅
- Viewport `(0, 0, 800, 600)` → correctly renders center point at pixel `(400, 300)` ✅
- Z values are normalized to -1 .. +1, so **depth testing works correctly** ✅

### Does It Work?

**YES**, technically it works. Pixel-based vertex coordinates can function correctly with a properly configured orthographic projection.

---

## Part 6: The Hidden Pitfalls of Pixel-Based Coordinates

### ① The Aspect Ratio Trap

`glm::ortho(0, 800, 0, 600, ...)` **hardcodes pixel aspect ratio**.

If the window is resized to `1000 x 600`:

- **Problem 1**: Geometry stretches horizontally or gets clipped
- **Problem 2**: `glViewport` must be dynamically recalculated for each resize
- **Problem 3**: The apparent "simplicity" of pixel coordinates evaporates

**Mitigation required**: Update projection matrix on every window resize event.

### ② Depth Precision Issues (Z-Fighting)

Setting `near = -1.0, far = 1.0` compresses Z into a narrow range.

For 2D applications with layered depth:

- Floating-point precision becomes scattered across a small Z range
- **Z-fighting (flickering) appears** when objects are close in depth
- The "natural" depth range (e.g., `near = 0.1, far = 100.0`) is wasted

**Trade-off**: Pixel-based Z values (e.g., `near = -1000, far = 1000`) lead to precision loss, defeating Z-buffer efficiency.

### ③ Loss of Shader Generality

Pixel-based coordinates force CPU-side transformations:

- **Cannot use GPU vertex shaders elegantly** for rotation, scaling, translation
- All affine transformations must be pre-computed **on the CPU** before uploading vertices
- **No model matrix, no dynamic GPU-driven animation**

**Consequence**: Severely limited scalability for complex graphics pipelines.

### ④ Violation of OpenGL Conventions

Teaching pixel-based coordinates as a "normal" approach:

- Confuses beginners who later encounter 3D graphics or resolution independence
- Creates friction in **team development** (reviewers ask: "Why aren't we using world coordinates?")
- Makes porting code to different resolutions or platforms **error-prone**

---

## Part 7: Summary Table: Is Pixel-Based Viable?

| Criterion | Assessment | Notes |
|-----------|-----------|-------|
| **Is it technically possible?** | ✅ Yes | With orthographic projection configured correctly |
| **Does Z-Buffer work?** | ✅ Yes | If near/far range is set appropriately |
| **Is it industry standard?** | ❌ No | Not recommended for production graphics |
| **Extensibility?** | ❌ Poor | Window resizing, 3D, shader complexity all problematic |
| **Valid exceptions** | ⚠️ Limited | UI overlays (ImGui), editor tools, fixed-resolution retro emulators |

---

## Part 8: The Correct Design Approach

### Principle

**Hold vertex data in world coordinates or a meaningful abstract space.** Apply camera + projection transformations via matrices.

### Exception: Strategic Partitioning

For complex applications, **separate your rendering into layers**:

| Layer | Coordinate System | Projection |
|-------|------------------|-----------|
| **Game world** | World coordinates (-10 .. 10) | Perspective/Ortho with view matrix |
| **UI overlay** | Pixel coordinates (0 .. 800, 0 .. 600) | Orthographic projection |
| **Debug output** | Pixel coordinates | Orthographic projection |

Each layer uses its **appropriate coordinate system and projection**, avoiding the pitfalls of a monolithic pixel-based design.

### Example Flow

```glsl
// Vertex Shader
#version 330

uniform mat4 proj_world;   // Camera + perspective
uniform mat4 proj_ui;      // Orthographic UI projection

layout (location = 0) in vec3 world_pos;
layout (location = 1) in vec3 ui_pos;
layout (location = 2) in int layer_id;

void main() {
    if (layer_id == LAYER_WORLD) {
        gl_Position = proj_world * vec4(world_pos, 1.0);
    } else if (layer_id == LAYER_UI) {
        gl_Position = proj_ui * vec4(ui_pos, 1.0);
    }
}
```

---

## Part 9: Final Verdict

| Statement | Evaluation |
|-----------|-----------|
| **"Zbuffer requires NDC"** | ✅ **Correct** |
| **"2D Ortho + pixel coords works"** | ⚠️ **Technically correct, but design-flawed** |
| **"This is the right way to do 3D graphics"** | ❌ **Absolutely not** |

### Recommendation

1. **For prototypes or fixed-resolution 2D**: Pixel-based + Ortho is **acceptable**
2. **For production or scalable applications**: **Strongly avoid** pixel-based coordinates for main geometry
3. **Best practice**: Use world coordinates + proper matrix transformations, with pixel-based rendering only for UI overlays

---

## References

- OpenGL Specification: Primitive Rasterization and Viewport Transformation
- Graphics Gems Series: Matrix operations and coordinate transformations
- [Piglit](https://gitlab.freedesktop.org/mesa/piglit): OpenGL test suite for Mesa drivers
- [apitrace](https://apitrace.github.io/): A suite of tools for tracing, replaying, and inspecting graphics API calls
- [Mesa 3D Graphics Library](https://www.mesa3d.org/): An open-source implementation of OpenGL, Vulkan, and other graphics APIs
- [llvmpipe](https://gallo.pages.freedesktop.org/mesa/drivers/llvmpipe.html): A high-performance software rasterizer that uses LLVM for JIT compilation
---
- [LearnOpenGL.com](https://learnopengl.com): A comprehensive, step-by-step tutorial resource covering the entire OpenGL graphics pipeline, from coordinate systems and transformations to advanced rendering techniques. It provides clear explanations, code examples, and visual aids that solidify the foundational concepts discussed in this document, particularly the **Coordinate Systems** and **2D Games** chapters which directly address orthographic projections and NDC space.
---




**Document Metadata**:
- **Status**: Complete Technical Reference
- **Last Updated**: 2026-06-24
- **Format**: Markdown for GitHub repository
- **License**: CC0 1.0 Universal (Public Domain)

