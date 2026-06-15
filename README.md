# ESP32 Lever Frame v1.0.0

An ESP32-based application specifically designed for the **Waveshare ESP32-S3-Touch-LCD-4.3** device to control and manage a wireless virtual lever frame. This project features full OpenLCB / LCC (Layout Command Control) integration over Wi-Fi, allowing for two-way event parsing and dynamic lever state updates, making it ideal for model railway control systems.

## Hardware Requirements

* **Waveshare ESP32-S3-Touch-LCD-4.3** display module.

## Key Features

* **Wireless Operation**: Fully wireless connectivity utilizing the ESP32's built-in Wi-Fi, eliminating the need for complex physical wiring to your layout.

* **OpenLCB / LCC Integration**: Comprehensive support for Layout Command Control protocols, handling two-way event parsing, state reporting, and dynamic lever state synchronization.
* **Web Configuration Interface**: A built-in Wi-Fi and web server UI for easy configuration of LCC events, network settings, and device parameters.
* **State Persistence**: Non-Volatile Storage (NVS) is used to save and restore lever states, including manual lock collar states and startup modes, ensuring reliable operation across reboots.
* **LCD Display Integration**: Features a startup splash screen, an informational drawer, and optimized memory buffering for smooth, tear-free UI animations.
* **Interlocking Conflict Policies**: Advanced configuration for LCC events and interlocking rules.

## Prerequisites

To bridge the wireless Wi-Fi LCC events from this device to a physical CAN-based layout, a **Wi-Fi to CAN LCC bridge** is required. The most common and recommended approach is to use JMRI.

### Bridging with JMRI (LCC Hub)
Assuming you have your USB-to-CAN adapter configured and working in JMRI:
1. In the main JMRI window (PanelPro or DecoderPro), go to the **LCC** menu (or **OpenLCB** depending on your connection prefix).
2. Click on **Start Hub** (or "LCC Hub"). This starts a GridConnect TCP server (usually on TCP port 12021) that bridges the network to your CAN bus.
3. In the Lever Frame's Web Configuration Interface, ensure the Wi-Fi is connected to the same network as the JMRI computer.

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
