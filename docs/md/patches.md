# Patches and banks

Two libraries. Loading one does not overwrite the other, which is the whole reason they are two libraries.

- **Patch** (`.tew3p`) — sound knobs, play settings, LFO shapes, FX parameters and order. Never includes the 36 patterns or the current bank/pattern index.
- **Bank** (`.tew3b`) — all 36 patterns. Never includes patch knobs.

Files live in `%APPDATA%/Stupid Audio/TEW03/Patches` and `.../Banks`. The title bars show the current names. The DAW session stores those names for recall.

Launch writes forty factory patches into that Patches folder (`01 Squelch` through `40 Factory Floor`), overwriting those factory names so they pick up the current effects. Files you saved under other names stay. Each factory patch is a sound plus an effects chain, tagged with a category: **Acid**, **Bass**, **Lead**, **Space**, **Dirty**, or **Classic**. The patch bar shows `Acid  ·  01 Squelch`. The patch menu groups by those names; leftovers sit under **User**.

Click a bar for the menu: **Init Patch** / **Init Bank**, the saved list, then **Save**, **Save As…**, **Export…**, **Import…**. Patch arrows step through the current category and wrap; bank arrows still step the full file list. Init restores defaults / factory fills.
