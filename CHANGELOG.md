# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.3.0] - 2026-06-22

### Added
- 

### Changed
- 

## [1.2.0] - 2026-06-22

### Added
- Added "Accept & Warn" (Alarm Mode) override policy.
- Added visual ALARM state (orange text/collar) when external commands violate local interlocking rules.
- Added two new traditional lever colors: Level Crossing (Brown) and Release/Gong (Green).
- Created a comprehensive `docs/USER_GUIDE.md` replacing the old Web UI guide.

### Changed
- Fixed tab title rendering bug (title did not appear until edit field was focused).
- Updated interlocking engine so that both conflicting levers involved in a violation will correctly flag the ALARM state.
- Enhanced JMRI CDI configuration to fully expose LCC Master Enable, Startup Mode, and Conflict Policy settings remotely.

## [1.1.2] - 2026-06-19

### Removed
- Removed auto-generated IDE comments and empty comment lines across the codebase.

## [1.1.1] - 2026-06-18

### Changed
- Updated README.md to include new device screenshots for the North Junction and South Box configurations.
- Added screenshots of the Web UI to the README.
- Added explicit instructions for accessing the Web Configuration Interface via both the built-in AP and home network.

## [1.1.0] - 2026-06-17

### Added
- Native host test harness using CMake and Unity for testing hardware-agnostic C logic.
- Real-time global interlocking simulator in the Web UI, which permanently renders red locking pins and "INTERLOCK" collar labels for accurate previewing.
- Export/Import configuration functionality in the Web UI to easily backup, share, and test `.json` layout configs.
- Included `docs/json/prototypical_interlocking.json`, a complex demonstration of junction locking, FPLs, and conditional route locking.

### Changed
- Abstracted the core interlocking engine into a standalone module (`interlocking.c`) for bidirectional, deadlock-preventing locking evaluation.
- Updated the Web UI conditional locking terminology from "otherwise (o/w)" and "unless" to an explicit Boolean "OR" to align with prototypical route specification standards.
- Updated device LCD info drawer text generation to accurately display conditional 'OR' requirements when a lever is tapped.
- Re-architected interlocking evaluation to perfectly model physical mechanical tappet locking (including Normal/Reversed locking, conditional locks, and sequential mutual locks).

## [1.0.1] - 2026-06-17

### Changed
- Updated README screenshots to use AI enhanced photos of a real display.

## [1.0.0] - Initial Release

### Added
- Initial structure and application logic for the ESP32 Lever Frame.
- Full OpenLCB/LCC integration including two-way event parsing and reporting.
- Web-based Wi-Fi configuration and remote LCC event setup.
- Virtual lever state management with manual lock collar detection and NVS persistence.
- LCD display support with optimized memory buffering, splash screen, and info drawer.
- GPLv3 licensing.
