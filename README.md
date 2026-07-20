# 🎮 DocVision – Adaptive Exposure

![OBS](https://img.shields.io/badge/OBS-32.x-blue)
![Windows](https://img.shields.io/badge/Windows-10%20%7C%2011-success)
![License](https://img.shields.io/badge/License-MIT-green)

> Unlike static filters, DocVision continuously adapts exposure based on scene brightness, helping viewers see dark gameplay while preserving highlights and true blacks.

DocVision is a native OBS Studio filter that continuously analyses scene brightness using GPU luminance sampling and automatically adjusts exposure when gameplay becomes too dark.

Designed with survival and extraction games in mind where viewers often struggle to see what's happening during night-time gameplay.

---

## ✨ Features

- 🎮 Native OBS Studio filter
- ⚡ GPU accelerated luminance detection
- 🌙 Automatic dark scene detection
- ☀️ Automatic recovery when scenes become bright
- 🎚 Adjustable dark & light thresholds
- ⏱ Independent fade-in and fade-out speeds
- 🧠 Brightness smoothing
- 🚫 Near-black / loading screen protection
- ⌨️ Toggle automation with an OBS hotkey
- 🪶 Lightweight and designed for real-time streaming
- 🎚 Exposure Strength control
- 📈 Adjustable exposure curve
- ⚫ True black protection
- 🌑 Near-black boost limiting

---

---

# ⚙️ Installation

> **Recommended:** Back up your OBS profile and scene collection before installing any third-party plugin.

1. Download the latest release from the **Releases** page.
2. Close OBS Studio.
3. Extract the ZIP.
4. Copy the included folders into:

```
C:\Program Files\obs-studio
```

5. Allow Windows to merge the folders.
6. Restart OBS.
7. Open the Filters window of a Game Capture, Window Capture or Display Capture source.
8. Add **DocVision – Adaptive Exposure**.

---

# ⚙️ Default Settings

| Setting | Value |
|---------|------:|
| Dark Threshold | 45 |
| Light Threshold | 62 |
| Maximum Exposure Boost | 0.45 |
| Fade In | 1.5 seconds |
| Fade Out | 0.8 seconds |
| Brightness Smoothing | 0.65 |
| Detection Interval | 250 ms |

These values are intended as a good starting point and can be adjusted for different games.

---

# ✅ Tested With

### Operating Systems

- Windows 10
- Windows 11

### OBS Studio

- OBS Studio 32.x

### Capture Types

- Window Capture
- Display Capture
- Game Capture

---

# 🕹 Recommended Games

DocVision was primarily designed for games featuring large dark environments such as:

- DayZ
- Escape From Tarkov
- Rust
- Hunt: Showdown
- PUBG

It should also work well with many other games.

---

# 🚧 Stable Release

DocVision v1.1.0 is the first stable public release.

If you discover any issues, please report them via GitHub Issues.

---

# ⚠️ Disclaimer

This software is provided **"AS IS"**, without warranty of any kind.

By installing this plugin you acknowledge that:

- You use it entirely at your own risk.
- The author cannot be held responsible for crashes, corrupted OBS configurations, data loss or any damage resulting from the use of this software.
- Always back up your OBS profile and scene collection before installing third-party plugins.

If you discover a bug, please open a GitHub Issue.

---

# 🐞 Reporting Bugs

Please include:

- Windows version
- OBS version
- Game
- Capture type
- Plugin version
- OBS log file
- Screenshots if applicable

The more information provided, the easier it is to reproduce and fix the issue.

---

# 🗺 Roadmap

# 🗺 Roadmap

## Version 1.2

- Built-in presets
- User-saveable presets
- Smarter exposure detection
- Additional optimisation

## Future

- Auto calibration
- HDR support
- Per-game profiles
- Linux support
- macOS support

---

# ❤️ Contributing

Suggestions, feature requests and pull requests are always welcome.

If you have an idea that would improve DocVision, feel free to open a GitHub Discussion or Issue.

---

# 📄 License

This project is licensed under the MIT License.

See the [LICENSE](LICENSE) file for details.

---

# 👨‍💻 Created By

Created by **DocButNotReal**

If you enjoy using DocVision, consider leaving a ⭐ on GitHub.

It really helps the project grow.