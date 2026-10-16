# Native client

## Scope

The Windows desktop client is C++20. The Python UI and its PyInstaller spec are
removed, not bundled as a hidden backend. No Python interpreter, Qt installation
or third-party runtime DLL is required to run the static EXE. Server/ComfyUI/Ollama
remain the existing external services; their code, models and API configuration
are not rewritten or silently modified.

## Source

- `client/src/native/common.hpp`: UTF-8, paths, atomic persistence, native file dialogs.
- `client/src/native/model.hpp`: compatible JSON projects, settings and generation controls.
- `client/src/native/project_organizer.hpp`: persistent logical folders, reorder/move, collapse and filtered sidebar rows.
- `client/src/native/thumbnail_cache.hpp`: bounded 256-item LRU cache, independent of filename order.
- `client/src/native/api.hpp`: origin-scoped API credentials, multipart references,
  batch queue, cancellation, editing/history and validated PNG downloads.
- `client/src/native/graphics.hpp`: Direct2D/DirectWrite, private bundled font collection,
  asynchronous WIC decode and bounded thumbnail cache.
- `client/src/native/editor.hpp`: uniform RichEdit foreground, undo-safe formatting and tag context menu.
- `client/src/native/prompt_tags.hpp`: comma-aware tag ranges and explicit 0.1 weight adjustments.
- `client/src/native/app.hpp`, `app.cpp`: dark layout and native controls, project actions,
  prompt presets, emotions, reference options, model/quality, Hires and editing.
- `client/src/native/main.cpp`: actual startup splash, main window and headless test entries.
- `client/src/native/resources.rc`: original application icon, startup art, fonts, licenses.

The project-loaded notification and large sidebar brand heading are removed. The
main frame uses the native Windows title bar, system buttons and resize borders.
Direct2D retains its backing surface and redraws throughout the native move/resize loop.
Only the loading splash is fixed-size and borderless. It closes on initialization
completion without a fixed simulated loading delay.

## Rendering and memory

Direct2D hardware rendering is requested first, with platform software fallback
if unavailable. Windows native edit/combo/file dialog controls remain OS-rendered;
this is not a claim that every OS control executes its text layout on a GPU.
Preview images are decoded off the UI thread and scaled by Direct2D. At most 256
thumbnail bitmaps are cached. During resize/splitter drag, edit controls are moved
rather than recreated, preserving text and undo records. Autosave writes only
when serialized state changes.

Actual image generation still runs on the remote ComfyUI GPU. Switching the
client language does not increase model inference speed by itself.

## Verification

No interactive application windows were opened during verification. Tests cover
native JSON persistence and controls (9 checks), real hardware offscreen Direct2D
pixel readback and bundled font selection, copied existing project collections
(11 root projects and 13 data-directory projects), and local fake-server upload,
2-image batch, 3 requested base/Hires steps, reference options and editing output.
A full visual/manual desktop regression has not been performed at the user's request.

Login/registration/server monitoring remain design notes only. No implementation
or new credential format was added for those future features.

Image queue tests decode 100 images, report invalid images, and verify preview recovery.
Prompt weights support nested parentheses, escaped separators, Korean and UTF-16 text.

Sidebar folders are logical groups in channels.json, not filesystem folders. Dragging does not move image files or rewrite output paths. Existing project IDs and contents are retained. New projects inherit their current project folder. Folder removal only ungroups its projects. Folder metadata, manual project order and collapsed state persist across restarts. Sidebar scrolling and drag-edge scrolling keep long lists usable.

The filename-order eviction bug is covered by 10 older-image pages loaded after filling all 64 cache slots. Every visible thumbnail must remain cached across repeated paint frames.

Responsive gallery layout is shared between drawing, selection, scroll extents and keyboard image navigation. It allocates more columns as the panel grows, with taller portrait thumbnails. Cache capacity is 256; decoded thumbnails are bounded to 256 pixels per side. Model/style/quality controls retain native combo selection events and keyboard support, while their closed surface and list items use the surrounding dark styling.

Verification now covers 360 fixture images at three library widths and three DPI scales over 135 actual App::paint GPU frames/pages. Additional offscreen tests check five grid widths, hit-testing and dark option control colors. No interactive windows are launched.
