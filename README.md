# mini_light_player

[![build](https://github.com/computergeek1507/mini_light_player/actions/workflows/build.yml/badge.svg)](https://github.com/computergeek1507/mini_light_player/actions/workflows/build.yml)

A small, headless player for [xLights](https://xlights.org/) `.fseq` sequence files. It reads a show folder's `xlights_networks.xml`, streams sequence data to E1.31, Art-Net and DDP controllers over UDP, and plays the matching audio track in sync. No GUI, no Qt — meant to sit on a Pi or a spare box near the controllers and just run the show.

## Features

- Reads xLights `xlights_networks.xml` and drives **E1.31 (sACN)**, **Art-Net** and **DDP** outputs, splitting a single network row across multiple universes when it needs more than 512 channels.
- Plays **musical sequences** (`.fseq` + audio) with the light data locked to the audio playback position, and **animation sequences** (`.fseq` alone) on a steady wall-clock timer.
- Reads and writes `mini_light_player.json` playlists and day/time schedules, so the player can run unattended and pick up a show automatically at the scheduled time. `scottplayer.json`, the name the original Qt player used, is also recognized for shows that already have one.
- Can act as an **FPP multisync** master, broadcasting start/sync/stop packets to remote FPP players on the network.
- Sends a blackout frame and handles Ctrl+C cleanly, so stopping the player doesn't leave props stuck lit on the last frame shown.

## Usage

```
mini_light_player <show_folder> [sequence.fseq] [media_file] [--multisync]
```

- `show_folder` — an xLights show directory containing `xlights_networks.xml` (and optionally a `mini_light_player.json` or `scottplayer.json`).
- `sequence.fseq` — play this sequence once and exit. May be an absolute path or just a file name, in which case the show folder is searched for it. Omit this to instead follow the schedules in `mini_light_player.json`/`scottplayer.json` until interrupted.
- `media_file` — override the audio file named inside the `.fseq`.
- `--multisync` — act as an FPP multisync master, sending start/sync/stop packets to `239.70.80.80:32320` for remote FPP players to follow. Off by default.

```
# Play one sequence and exit
mini_light_player /path/to/show MySequence.fseq

# Follow the show's own playlists/schedule, driving remote FPP players too
mini_light_player /path/to/show --multisync
```

## Building

Requires CMake 3.20+ and a C++23 compiler (MSVC 2022, or gcc/clang new enough for `<chrono>` calendar support). Dependencies are fetched automatically at configure time via [CPM](https://github.com/cpm-cmake/CPM.cmake) — nothing needs to be installed beforehand beyond the compiler and CMake itself.

```
cmake -S . -B build
cmake --build build --config Release
```

The binary is built at `build/mini_light_player` (`build/Release/mini_light_player.exe` on Windows with the Visual Studio generator).

### Windows with Visual Studio

```
cmake -S . -B build -G "Visual Studio 17 2022"
cmake --build build --config Release
```

(`VS2022.bat` does the same, generating into `cmake_vs/`.)

## Prebuilt binaries

Every push to `main` builds a Windows `.exe` and a Linux x86_64 AppImage — see the [Actions tab](../../actions) for the latest build's artifacts. Pushing a `v*` tag additionally publishes both to a [GitHub release](../../releases):

```
git tag v0.1.0
git push origin v0.1.0
```

The AppImage is built on Ubuntu 24.04 and needs glibc 2.39 or newer on the machine that runs it.

## Show folder layout

The player expects the same folder xLights itself uses:

```
show_folder/
├── xlights_networks.xml     # controller/output configuration (required)
├── mini_light_player.json   # playlists and schedules (optional)
└── Sequences.../*.fseq      # sequence files, referenced by name or path
```

If both `mini_light_player.json` and `scottplayer.json` are present, `mini_light_player.json` wins. Whichever one was loaded is the one saved back to, so an existing `scottplayer.json` show isn't forced to rename anything.

`mini_light_player.json` holds playlists of `{seq, media}` items and schedules with a day-of-week list, a date range and a time-of-day window. A working example is at [`examples/mini_light_player.json`](examples/mini_light_player.json):

```json
{
    "playlists": [
        { "name": "Main", "items": [ { "seq": "Show.fseq", "media": "Show.mp3" } ] }
    ],
    "schedules": [
        {
            "playList": "Main",
            "enabled": true,
            "startDate": "2020-01-01", "endDate": "2099-12-31",
            "startTime": "17:00:00.000", "endTime": "22:00:00.000",
            "days": ["Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"]
        }
    ]
}
```

An end time earlier than the start time is treated as running past midnight.

## Known limitations

- DMX/serial controllers listed in `xlights_networks.xml` are recognized but not driven — only E1.31, Art-Net and DDP outputs are implemented.
- Art-Net broadcast/multicast discovery isn't supported; unicast Art-Net works normally.
- FPP multisync only sends to the standard multicast group; there's no unicast target list.

## License

GPL-3.0 — see [LICENSE](LICENSE).
