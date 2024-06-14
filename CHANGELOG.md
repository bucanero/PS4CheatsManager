# Changelog

All notable changes to the `PS4 Cheats Manager` project will be documented in this file. This project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]()

---

## [v1.2.2](https://github.com/bucanero/PS4CheatsManager/releases/tag/v1.2.2) - 2024-06-15

### Added

- Disable and re-enable cheat files
  - Disabled files won't be loaded by GoldHEN
- Option to change the default URLs for cheats, patches, plugins (Settings)

### Changed

- Download Patches from https://github.com/illusion0001/PS4-PS5-Game-Patch/

### Misc

- Includes latest official cheat+patch pack (2024-06-14)

---

## [v1.2.0](https://github.com/bucanero/PS4CheatsManager/releases/tag/v1.2.0) - 2024-01-02

### Added

- MC4 format decryption
  - Parse and display codes from MC4 files
  - Export decrypted MC4 files to `xml` format
  - List invalid MC4 files
- Tag cheat files by format (`JSON`, `SHN`, `MC4`)
- Add option to delete all local cheats, patches, plugins
- Network proxy support (based on system settings)

### Changed

- Updated UI assets

### Fixed

- Game filtering by Title ID

---

## [v1.1.4](https://github.com/bucanero/PS4CheatsManager/releases/tag/v1.1.4) - 2023-09-22

### Added

- Add Plugins updater
- Backup local Plugins
- Improve local cheats & patches updater
- Local update (Cheats, Patches, Plugins) scans files in 8 possible mounted USB partitions and HDD
- Adjust Circle/Cross button assignment based on console settings
- Download App update .pkg to `usb0` if available

### Changed

- Changed backup paths to:
  - `/mnt/usb0/backups/(name)`
  - `/data/GoldHEN/backup/name/`

### Misc

- Update Open Orbis SDK version

---

## [v1.1.0](https://github.com/bucanero/PS4CheatsManager/releases/tag/v1.1.0) - 2023-06-01

### Added

- Option to delete patch settings
- Support for masked pattern patches for filtering
- New Update Menu
  - Update Cheats from Web, HDD, USB
  - Update Patches from Web, HDD, USB
  - Backup local cheat files to .Zip (HDD/USB)
  - Backup local patch files to .Zip (HDD/USB)

---

## [v1.0.3](https://github.com/bucanero/PS4CheatsManager/releases/tag/v1.0.3) - 2023-01-25

### Added

- Support downloading updates to `/data/pkg/` if the folder exists. (For use with GoldHEN 2.3+)

### Changed

- Dropped support for Patch Engine's JSON format in favor of XML. (Plugins will have to be updated to at least 1.163)

---

## [v1.0.2](https://github.com/bucanero/PS4CheatsManager/releases/tag/v1.0.2) - 2022-12-24

_dedicated to Leon ~ in loving memory (2009 - 2022)_ :heart:

### Added

- Sort Games settings
  - by Name, by Title ID
- Offline installation for patches:
  - Download https://github.com/GoldHEN/GoldHEN_Patch_Repository/raw/gh-pages/patch1.zip
  - Copy `patch1.zip` to root of USB drive. (Do not rename it)
  - Open GoldHEN Cheat Manager and click Update
- Updating Cheats will now show the cheat database version.

### Changed

- Improved UI pad controls
- Patches will be downloaded from https://github.com/GoldHEN/GoldHEN_Patch_Repository

### Misc

- Includes latest official cheat+patch pack (2022-12-24)

---

## [v1.0.0](https://github.com/bucanero/PS4CheatsManager/releases/tag/v1.0.0) - 2022-11-12

### Added

- Support for Game Patches
  - List available patches
  - Enable/Disable patches
  - Update latest patches from the Web
- `MC4` cheat format support
  - List available cheats
  - Update `.mc4` cheat files from the Web
- New "File Overwrite" setting
  - Allows to overwrite current cheat files with updated ones

### Changed

- Updated networking code to `libcurl`+`polarssl` (TLS 1.2)
- Visual improvements, icons, background (thanks to Chronoss)

### Fixed

- Fixed networking issues (thanks to LightingMods for the help)
- Fixed version detection for some "remaster" titles

---

## [v0.7.0](https://github.com/bucanero/PS4CheatsManager/releases/tag/v0.7.0) - 2022-07-05

### Added

- Installed Game list filter to show only available cheats
- Offline cheat pack (.zip) installation from USB/HDD
  - `/mnt/usb0/GoldHEN_Cheat_Repository-main.zip`
  - `/data/GoldHEN_Cheat_Repository-main.zip`

---

## [v0.6.0](https://github.com/bucanero/PS4CheatsManager/releases/tag/v0.6.0) - 2022-04-07

### Added

- Display number of files installed after `Update`
- Installed Games detection on Cheat list screen
- Show total number of files on Cheat list screen

---

## [v0.5.1](https://github.com/bucanero/PS4CheatsManager/releases/tag/v0.5.1) - 2022-03-16

### Added

- Enabled Online DB
- Updated [Cheat repository URL](https://github.com/GoldHEN/GoldHEN_Cheat_Repository)
- Extract cheat files only (skip others)

---

## [v0.5.0](https://github.com/bucanero/PS4CheatsManager/releases/tag/v0.5.0) - 2022-03-14

First public beta release.
