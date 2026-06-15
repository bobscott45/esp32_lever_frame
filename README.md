# ESP32 Lever Frame

An ESP32-based application designed to control and manage a physical lever frame. This project features full OpenLCB / LCC (Layout Command Control) integration, allowing for two-way event parsing and dynamic lever state updates, making it ideal for advanced model railway control systems.

## Key Features

* **OpenLCB / LCC Integration**: Comprehensive support for Layout Command Control protocols, handling two-way event parsing, state reporting, and dynamic lever state synchronization.
* **Web Configuration Interface**: A built-in Wi-Fi and web server UI for easy configuration of LCC events, network settings, and device parameters.
* **State Persistence**: Non-Volatile Storage (NVS) is used to save and restore lever states, including manual lock collar states and startup modes, ensuring reliable operation across reboots.
* **LCD Display Integration**: Features a startup splash screen, an informational drawer, and optimized memory buffering for smooth, tear-free UI animations.
* **Interlocking Conflict Policies**: Advanced configuration for LCC events and interlocking rules.

## Getting Started

This project is built using the ESP-IDF framework (v6 compatible).

1. Clone this repository. Make sure to include the submodules:
   ```bash
   git clone --recursive <repository-url>
   ```
2. Configure your ESP-IDF environment.
3. Build, flash, and monitor the project using standard ESP-IDF commands:
   ```bash
   idf.py build flash monitor
   ```

## License

This project is licensed under the GNU General Public License v3.0 (GPLv3). See the [LICENSE](LICENSE) file for more details.
