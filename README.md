# DocVision – Adaptive Exposure

Automatically brighten dark gameplay in OBS Studio without permanently washing out your image.

DocVision analyses the brightness of your captured gameplay in real time and smoothly applies an exposure adjustment only when the scene becomes too dark.

Designed primarily for games like:

- DayZ
- Escape From Tarkov
- Rust
- Hunt: Showdown
- PUBG
- Any game with large dark areas

---

## Features

- 🎮 Native OBS Studio filter
- ⚡ GPU-based luminance detection
- 🌙 Automatic dark scene detection
- ☀️ Automatic recovery when scenes become bright
- 🎚 Adjustable exposure thresholds
- ⏱ Smooth fade-in and fade-out
- 🧠 Brightness smoothing
- 🚫 Near-black/loading screen protection
- ⌨ OBS hotkey support
- 🪶 Lightweight GPU usage

## Installation

1. Download the latest release from the Releases page.
2. Close OBS Studio.
3. Extract the ZIP.
4. Copy the included folders into:

C:\Program Files\obs-studio

5. Restart OBS.
6. Add **DocVision – Adaptive Exposure** as a filter to your Game Capture, Window Capture or Display Capture source.

---

## Default Settings

| Setting | Default |
|---------|---------:|
| Dark Threshold | 45 |
| Light Threshold | 62 |
| Maximum Boost | 0.45 |
| Fade In | 1.5s |
| Fade Out | 0.8s |
| Smoothing | 0.65 |
| Sample Rate | 250 ms |

---

## Tested With

✅ OBS Studio 32.x

✅ Windows 11

### Capture Types

- Window Capture
- Display Capture
- Game Capture

---

## Roadmap

### Version 0.2

- Improved game profiles
- Better brightness curve
- Performance optimisation

### Version 0.3

- Auto calibration
- Profile presets
- Better statistics

### Future

- HDR support
- Multi-monitor optimisation
- Linux support
- macOS support

---

## Reporting Bugs

If you find a bug, please open a GitHub Issue and include:

- Windows version
- OBS version
- Game
- Capture type
- Screenshots if possible
- OBS log

---

## Contributing

Suggestions and pull requests are welcome.

---

## License

MIT License

---

Created by **DocButNotReal**.
