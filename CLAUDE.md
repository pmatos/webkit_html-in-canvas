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

WebKit does **not** build with raw `cmake .` from the root. Use the canonical scripts. **Pick one of three dependency paths first** (`Tools/Scripts/update-webkitgtk-libs` will refuse to run otherwise):

| Path | Env var | First-time setup | Notes |
|---|---|---|---|
| **wkdev-sdk** (Igalia recommended) | none — auto-detected when running inside the container | `git clone https://github.com/Igalia/webkit-container-sdk` + `source register-sdk-on-host.sh` + `wkdev-create --create-home` (may need root for first-time GPU profiling config) | Requires Podman. Build commands run *inside* the container via `wkdev-enter`. |
| **Flatpak** (autonomous, used here) | `WEBKIT_FLATPAK=1` | Just set the env var; `update-webkit-flatpak --gtk` downloads `org.webkit.Platform 23.08` to `WebKitBuild/UserFlatpak/` | Runs as the current user. No sudo. Adds ~5 GB of Flatpak runtime. |
| **JHBuild** | `WEBKIT_JHBUILD=1` | `sudo Tools/gtk/install-dependencies` to install host packages, then `update-webkitgtk-libs` builds vendored libs from source | Slowest. Needs sudo. |

**Local quirk:** the user's global `python3` is shimmed to a `uv run python3` enforcement guard (Trail of Bits modern-python plugin, at `~/.claude/plugins/cache/trailofbits/modern-python/.../shims/python3`). WebKit's Python tooling does not coexist with this — invocations of `Tools/Scripts/{build-webkit,update-webkit-flatpak,run-webkit-tests,...}` need a shim-free `PATH`. Either invoke via `/usr/bin/python3` directly, or `env -u PATH PATH=/usr/local/bin:/usr/bin:/bin ...`. The repo's wrapper scripts (`Tools/Scripts/build-webkit` etc.) are perl, but they `exec` python3 internally, so the same shim-bypass applies.

**MANDATORY toolchain:** every build in this checkout uses **clang + clang++ + lld**. Never gcc. Never `ld.bfd`. The gcc + libstdc++ debug-link path has OOMed this machine three times; clang+lld is the only path Paulo wants used here. Pass `CC=clang CXX=clang++ -DCMAKE_LINKER_TYPE=LLD` on every fresh configure, and never edit `CMakeCache.txt` to revert.

**MANDATORY build-memory cap:** every long-running build invocation runs inside a `systemd-run --user --scope -p MemoryMax=96G -p MemorySwapMax=0` cgroup, plus a watcher that applies the same cap to `app-flatpak-org.webkit.Sdk-*.scope` after flatpak spawns it (flatpak escapes the launcher's cgroup). Default parallelism: `NUMBER_OF_PROCESSORS=12`. Higher values have OOM-killed inside the cap. See `~/.claude/projects/-home-igalia-pmatos-dev-WebKit-HTML-in-canvas/memory/feedback_build_with_clang.md` and `feedback_cap_build_memory.md` for the canonical invocations.

Once a dependency path is chosen, build with:

```sh
# Flatpak path (autonomous; matches what's set up in this checkout)
PATH=/usr/local/bin:/usr/bin:/bin WEBKIT_FLATPAK=1 \
  /usr/bin/python3 Tools/Scripts/update-webkit-flatpak --gtk    # one-time; downloads SDK

# Cold/full build (clang+lld + 96G cap + 12 workers)
# SDK CMake 3.27 lacks -DCMAKE_LINKER_TYPE=LLD; use -fuse-ld=lld linker flags.
# clang/clang++/lld live at /usr/lib/sdk/llvm18/bin inside the SDK.
systemd-run --user --scope -p MemoryMax=96G -p MemorySwapMax=0 -- \
  bash -c 'PATH=/usr/local/bin:/usr/bin:/bin WEBKIT_FLATPAK=1 \
    perl Tools/Scripts/webkit-flatpak --gtk --debug \
      -c bash -c "PATH=/usr/lib/sdk/llvm18/bin:/usr/local/bin:/usr/bin:/bin NUMBER_OF_PROCESSORS=12 \
        CC=clang CXX=clang++ \
        perl Tools/Scripts/build-webkit --gtk --debug \
          --cmakeargs=\"-DUSE_LIBRICE=OFF -DCMAKE_EXE_LINKER_FLAGS=-fuse-ld=lld -DCMAKE_SHARED_LINKER_FLAGS=-fuse-ld=lld -DCMAKE_MODULE_LINKER_FLAGS=-fuse-ld=lld\""'
# → WebKitBuild/GTK/Debug/bin/WebKitTestRunner
#
# In a separate shell, cap the flatpak scope that just spawned (see memory files
# for the watcher one-liner).

# wkdev-sdk path (canonical)
wkdev-enter --name wkdev
# inside container, same toolchain rules apply:
CC=clang CXX=clang++ Tools/Scripts/build-webkit --gtk --debug \
  --cmakeargs="-DCMAKE_LINKER_TYPE=LLD"
```

`USE_LIBRICE=OFF` because the WebKit SDK 23.08 doesn't ship `librice-{io,proto}` and we don't need WebRTC for canvas work. Pass via `--cmakeargs=...` (the `--` separator is for ninja, not cmake).

**Switching an existing gcc build dir to clang requires `rm -rf WebKitBuild/GTK/Debug` and a full reconfigure.** The cache pins the compiler. Confirm with Paulo before destroying a build dir.

CMake preset path (`cmake --preset gtk-dev-debug && cmake --build --preset gtk-dev-debug`) gives `compile_commands.json` for clangd/IDEs but still requires the chosen dependency path to have run first **and** the same clang+lld toolchain pinned in the preset.

Other ports (`--wpe`, `--release` for Mac, etc.) exist but are not the target; default to GTK debug unless the user says otherwise.

A cold GTK debug build is **1–3 hours** and peaks well within the 96G cap with clang+lld at `NUMBER_OF_PROCESSORS=12`. Incremental rebuilds finish in minutes.

### Running tests in Flatpak mode

`WEBKIT_FLATPAK=1 run-webkit-tests` does **not** auto-enter the sandbox. The script runs on the host, spawns `WebKitTestRunner` from `WebKitBuild/GTK/Debug/bin/`, and the runner immediately fails with `error while loading shared libraries: libicudata.so.73` (SDK libs not on host). Wrap the entire invocation in `webkit-flatpak -c` to put `run-webkit-tests` inside the sandbox:

```sh
PATH=/usr/local/bin:/usr/bin:/bin WEBKIT_FLATPAK=1 \
  perl Tools/Scripts/webkit-flatpak --gtk --debug \
    -c bash -c "PATH=/usr/local/bin:/usr/bin:/bin perl Tools/Scripts/run-webkit-tests --gtk --debug --no-show-results --no-retry-failures --no-build <test-path>"
```

The inner `PATH=/usr/local/bin:/usr/bin:/bin` is required to bypass the user's `python3` shim again inside the sandbox shell.

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
