# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this checkout is for

This is a WebKit working tree dedicated to the **HTML-in-Canvas** implementation
epic — porting Chrome's `drawElementImage` / `captureElementImage` / `paint`
event / `texElementImage2D` / `copyElementImageToTexture` / `<canvas
layoutsubtree>` surface to WebKit. **No code is upstream yet.**

Authoritative documents (read before proposing work):

- `HTML-in-Canvas-Report.md` (repo root) — the canonical plan. §1 executive summary, §2 IDL surface, §3 Chrome reference map, **§4 WebKit landing sites**, **§5 build/test workflow**, **§6 vertical-slice TDD plan**, §7 open questions. When in doubt, this file overrides any other guidance about *this feature*.
- `Igalia/html-in-canvas/` — vendored WICG explainer + examples (git submodule). Treat as the moving spec (the live spec PR is whatwg/html#11588).

The runtime preference name we are introducing is `CanvasDrawElementEnabled` (Blink calls it `CanvasDrawElement`).

## Build

WebKit does **not** build with raw `cmake .` from the root. Use the canonical scripts:

```sh
# GTK port (the target for this work), debug build
Tools/Scripts/build-webkit --gtk --debug
# → WebKitBuild/GTK/Debug/

# Or via CMake preset (gives compile_commands.json for clangd/IDEs)
cmake --preset gtk-dev-debug
cmake --build --preset gtk-dev-debug
```

Other ports (`--wpe`, `--release` for Mac, etc.) exist but are not the target; default to GTK debug unless the user says otherwise.

## Test

```sh
# Whole canvas layout-test suite
Tools/Scripts/run-webkit-tests --gtk LayoutTests/fast/canvas/

# New tests for this feature
Tools/Scripts/run-webkit-tests --gtk LayoutTests/fast/canvas/html-in-canvas/

# Imported Chromium WPT corpus (red bar to work against — see report §5.7)
Tools/Scripts/run-webkit-tests --gtk LayoutTests/imported/wpt-internal/html/canvas/drawElementImage/

# Single test
Tools/Scripts/run-webkit-tests --gtk LayoutTests/fast/canvas/html-in-canvas/<name>.html
```

The runner binary is `WebKitBuild/GTK/Debug/bin/WebKitTestRunner`. For manual smoke tests use `Tools/Scripts/run-minibrowser --gtk --debug <url>`.

## Implementation landing sites (mirror of report §4)

| Surface | File(s) |
| --- | --- |
| `layoutsubtree` attribute | `Source/WebCore/html/HTMLCanvasElement.{h,cpp}` |
| Containment / paint isolation | `Source/WebCore/dom/Node.h` (new `IsCanvasOrInCanvasSubtree` flag); `Source/WebCore/style/StyleAdjuster.{h,cpp}`; `Source/WebCore/rendering/PaintPhase.h` (new `PaintBehavior::CanvasSubtreeRecord`) |
| Snapshot recording | `Source/WebCore/platform/graphics/displaylists/` (`DisplayList::Recorder`) |
| Canvas 2D `drawElementImage` | `Source/WebCore/html/canvas/CanvasRenderingContext2DBase.{h,cpp}` + new `CanvasDrawElementImage.idl` mixin |
| WebGL `texElementImage2D` | `Source/WebCore/html/canvas/WebGLRenderingContextBase.{h,cpp}` |
| WebGPU `copyElementImageToTexture` | `Source/WebCore/Modules/WebGPU/GPUQueue.{h,cpp}` |
| `paint` event registration | `Source/WebCore/dom/EventNames.h`, `Source/WebCore/dom/EventInterfaces.in` |
| Lifecycle (fire `paint` post-paint, pre-commit) | `Source/WebCore/page/Page.cpp` (`updateRendering`); `Source/WebCore/page/LocalFrameView.cpp` |
| Runtime preference | `Source/WTF/Scripts/Preferences/UnifiedWebPreferences.yaml` |

This is a navigation hint, not a contract — verify against §4 of the report (which has rationale and edge cases) before editing.

## Vertical-slice plan

Report §6 defines slices: **Slice 0** scaffolding (pref + IDL stubs + test harness, no behaviour), **Slice 1** `layoutsubtree` parsing + containment + paint suppression, then snapshot pipeline, 2D draw, capture/transfer, paint event, WebGL, WebGPU, etc. Land slices in order — each slice ends with a green test set. Don't open a slice without reading its section in full.

## Open questions

§7 of the report enumerates known unknowns (display-list recording fidelity, a11y tree pruning under paint containment, paint-event lifecycle insertion point, `PaintBehavior` bit budget, origin-trial mechanism, cross-origin iframe handling, snapshot move-vs-copy semantics). When code touches one of these areas, surface the open question rather than guessing.

## Style & commit conventions

- Run `Tools/Scripts/check-webkit-style <files>` before committing. CI enforces it.
- WebKit no longer uses ChangeLog files. Commit message format: `[canvas] <description>` on the subject line, body explaining the change, **Bugzilla URL on the last line** (e.g. `https://bugs.webkit.org/show_bug.cgi?id=NNNNNN`).
- Tests: every behavioural change needs a layout test. Reftests are `foo.html` + `foo-expected.html`; assertion tests use `resources/testharness.js`. The "import the Chromium corpus first" pattern in report §5.7 gives us most of these for free.

## What this CLAUDE.md does *not* cover

- Generic WebKit conventions outside this feature — read `ReadMe.md`, `Introduction.md`, and `Tools/Scripts/` directly.
- Subsystems unrelated to canvas/DOM/lifecycle — assume nothing; explore.
- Chrome reference paths in report §1 are on the user's machine (`~/dev/chromium/...`) and may not be accessible from this checkout — ask before reading them.
