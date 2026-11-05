# PS4 Remote Detector

Standalone homebrew (**title `REMD00001` only**) for **GoldHEN / jailbroken PS4**. It does **not** require PS4 Linux (`CSTP00003`) or any Linux install. It shows **every pad / HDMI TV remote button** as a **large on-screen symbol** and logs a **machine-readable map** (useful when tuning other homebrew later).

## What it does

- Listens on **user standard pad**, **system standard pads**, **TV `REMOTE_CONTROL` (CEC)**, and **keyboard** (if available).
- On each **press / release**, logs:
  - `/data/ps4-remote-detector/events.log` — human-readable lines
  - `/data/ps4-remote-detector/map.csv` — CSV for spreadsheets / codegen
- **Full-screen UI**: huge symbol (e.g. ↑ ↓ OK HOME), label, source, raw hex `0x........`.

**Title ID:** `REMD00001`

## Build

Mac build only needs the OpenOrbis toolchain path from sibling **`ps4-linux-pro`** (build dependency on your PC — **not** on the console). Adjust `app/Makefile` `REPO_ROOT` if needed:

```bash
cd /Users/Shared/dev/ps4-remote-detector
swift tools/build.swift
```

Output: `app/IV0000-REMD00001_00-PS4REMOTED001.pkg`

## Install & use

1. Copy PKG to `/data/pkg/` on the console (FTP).
2. **Delete** any old **PS4 Remote Detector** tile before reinstalling (same for switching from a test Linux PKG).
3. **Package Installer** → install **`IV0000-REMD00001_00-PS4REMOTED000001.pkg`** (title **REMD00001**, remote icon — not **CSTP00003** PS4 Linux).
4. Launch **PS4 Remote Detector** from the game menu (remote icon, splash says **PS4 Remote Detector**).
4. Press every button on the **TV remote** and **DualShock**; watch the TV and pull logs:

```bash
# From Mac (example FTP host)
curl ftp://10.0.1.7:2121/data/ps4-remote-detector/map.csv -o map.csv
```

## CSV columns

| Column | Meaning |
|--------|---------|
| `timestamp` | UTC ISO |
| `source` | e.g. `TV_REMOTE_0`, `USER_STD` |
| `phase` | `PRESS` or `RELEASE` |
| `raw_hex` | Bit mask from `scePad` |
| `labels` | Known names + `BITn` for unknown bits |
| `symbol` | On-screen glyph |

Use this map to fix TV OK / D-pad handling in other homebrew.

## Bravia / CEC notes (RMT-TX300E-class)

| Physical control | Typical `TV_REMOTE` bit | Notes |
|------------------|-------------------------|--------|
| D-pad | `0x10` / `0x20` / `0x40` / `0x80` | Reliable |
| Colored **Options** | `0x1000` (`TRIANGLE`) | Not Enter |
| **Home** | `0x80000000` | Often `INTERCEPTED` flag in SDK |
| **Center OK / Enter** | *(often nothing)* | PS4 shell may still launch titles; many TVs do not forward center click to homebrew — watch **`USER_STD`** for `0x4000` (Cross) and **`KEYBOARD`** for Return |

If center OK never appears in `map.csv`, the limitation is CEC/TV routing, not this app missing a listener.

### OK investigation (v01.15+)

1. Launch detector and confirm on-screen **attach** line lists **`USER_STD=`** and/or **`SYS_STD_n=`** (not only `TV_REMOTE_0`).
2. Press **center OK** ten times; pull `map.csv` and search for:
   - `OK_RING` / `OK_RING_HIT` — snapshot of standard pads when CEC d-pad/options fire
   - `OK_CANDIDATE` / `RAW` on `USER_STD` or `SYS_STD_*` — especially **`ORBIS_OPTIONS_0x8`**, **`CROSS`**, **`L3`**
   - `KEYBOARD` + Return (`ORBIS_KEYBOARD_KEY_RETURN`)
3. If **only** `TV_REMOTE_0` attaches and OK lines never appear, Bravia is not forwarding center click to homebrew; use **hold Down ~750 ms** confirm in PS4 Linux (same as loader) or DualShock Cross.

## License

GPL-3.0 (Graphics code derived from PS4 Linux loader).
