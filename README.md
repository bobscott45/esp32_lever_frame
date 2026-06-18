# ESP32 Lever Frame v1.1.0

An ESP32-based application specifically designed for the **Waveshare ESP32-S3-Touch-LCD-4.3** device to control and manage a wireless virtual lever frame. This project features full OpenLCB / LCC (Layout Command Control) integration over Wi-Fi, allowing for two-way event parsing and dynamic lever state updates, making it ideal for model railway control systems.

## Hardware Requirements

* **Waveshare ESP32-S3-Touch-LCD-4.3** display module (Part Number/SKU: **25948**).
  * **IMPORTANT:** This project is specifically designed for the *original* standard version of this display. It is **NOT** compatible with the "B" (ESP32-S3-Touch-LCD-4.3B) or "C" (ESP32-S3-Touch-LCD-4.3C) variants due to differences in their hardware and interface configurations.

## Key Features

* **Wireless Operation**: Fully wireless connectivity utilizing the ESP32's built-in Wi-Fi, eliminating the need for complex physical wiring to your layout.

* **OpenLCB / LCC Integration**: Comprehensive support for Layout Command Control protocols, handling two-way event parsing, state reporting, and dynamic lever state synchronization.
* **Web Configuration Interface**: A built-in Wi-Fi and web server UI for easy configuration of LCC events, network settings, and device parameters.
* **State Persistence**: Non-Volatile Storage (NVS) is used to save and restore lever states, including manual lock collar states and startup modes, ensuring reliable operation across reboots.
* **High-Performance Touch UI**: A fully custom-built virtual lever frame interface that forms the core of the application, featuring highly optimized memory buffering for smooth, tear-free operation, gesture controls, and a responsive informational drawer.
* **Prototypical Interlocking Engine**: A C-based interlocking engine that bidirectionally models physical mechanical tappet locking, preventing deadlocks and supporting complex route dependencies like Facing Point Locks (FPLs) and conditional "OR" logic.
* **Live Web Simulator**: A built-in simulator allows you to preview and debug complex interlocking logic in real-time, matching exactly how the physical frame behaves with permanent locking visuals.
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

## Running Tests

This project includes a native host test harness built with CMake and the Unity framework. These tests run directly on your computer (no ESP32 hardware required) and validate the core hardware-agnostic C logic, such as the interlocking engine and configuration parsers.

To run the test suite:
1. Navigate to the `main/test` directory.
2. Create a build directory and run CMake:
   ```bash
   cd main/test
   mkdir build && cd build
   cmake ..
   make
   ```
3. Execute the test binary:
   ```bash
   ./run_tests
   ```

## Using the Web Configuration Interface

The device hosts a built-in Web Configuration Interface that can be accessed in two ways: via its own built-in Wi-Fi Access Point (AP) or through your home network.

### Finding the IP Address
At any time, you can find the current IP addresses and Wi-Fi credentials by swiping down from the top of the device's touch screen. This will reveal the "System Information" drawer.

### Method 1: Connecting via the Built-in Access Point (AP)
When the device is powered on, it automatically broadcasts its own Wi-Fi network.
1. On your computer or smartphone, connect to the Wi-Fi network named **`Lever-Frame-Config`**.
2. Enter the default password: **`signalman`** (this can be changed in the settings later).
3. Open a web browser and navigate to **`http://192.168.4.1:80`**.

### Method 2: Connecting via your Home Network
If you have configured the device to connect to your home Wi-Fi network (which is required for JMRI/LCC bridge connection):
1. Ensure your computer or smartphone is connected to the same home network.
2. Swipe down from the top of the device's screen to open the System Information drawer.
3. Note the IP address listed next to **Home Wi-Fi IP**.
4. Open a web browser and navigate to that IP address on port 80 (e.g., `http://192.168.x.x:80`).

From the Web UI, you can configure LCC events, import/export the JSON interlocking configuration, update Wi-Fi credentials, and use the live simulator.

## Example Configuration

A prototypical demonstration configuration is included in `docs/json/prototypical_interlocking.json`. This layout demonstrates sequential signaling, mutually locking facing points, and conditional 'OR' route locking.

To load this configuration:
1. Open the Web UI in a browser.
2. Click the **Import** button in the header and select the `docs/json/prototypical_interlocking.json` file.
3. The layout will load into the live simulator. Click **Save & Apply** to push it to the ESP32 device.

### North Junction (Main Frame - 8 Levers)
This frame protects a junction where a branch line diverges from a main line.
- **Lever 1 (UP DISTANT)**: The approach signal. *Locks Lever 2 REVERSED OR Lever 5 REVERSED*. This demonstrates conditional 'OR' logic: the distant signal can be cleared if either the Main or Branch home signals are clear.
- **Lever 2 (UP MAIN HOME)**: Clears the train straight ahead. *Locks Lever 4 NORMAL and Lever 3 REVERSED*.
- **Lever 3 (FPL FOR POINTS 4)**: The Facing Point Lock.
- **Lever 4 (JUNCTION POINTS)**: The physical turnout. *Locks Lever 3 NORMAL*, ensuring the points cannot be moved unless the physical bolt (FPL) is withdrawn.
- **Lever 5 (UP BRANCH HOME)**: Clears the train to turn off onto the branch line. *Locks Lever 4 REVERSED and Lever 3 REVERSED*.
- **Lever 6 (SPARE)**
- **Lever 7 (DOWN ADVANCED)**
- **Lever 8 (DOWN HOME)**

### South Box (Yard Frame - 4 Levers)
This frame controls a small yard crossover.
- **Lever 1 (SHUNT AHEAD)**: A shunting disc. *Locks Lever 2 REVERSED*.
- **Lever 2 (YARD CROSSOVER)**: The physical points for the crossover. *Locks Lever 3 REVERSED*.
- **Lever 3 (FRAME RELEASE)**: A ground frame release mechanism.
- **Lever 4 (SPARE)**

### Demonstrating the Interlocking

To observe the interlocking rules in action, try the following sequences in the Web UI simulator:

**North Junction:**
1. Try to pull **Lever 2 (UP MAIN HOME)**. It will be locked because the Facing Point Lock (Lever 3) is not engaged.
2. Try to pull **Lever 4 (JUNCTION POINTS)**. It is free to move because the FPL (Lever 3) is withdrawn.
3. Pull **Lever 3 (FPL)** to lock the points.
4. Try to pull **Lever 4** again. It is now locked by Lever 3.
5. Pull **Lever 2** again. It now clears because Lever 4 is Normal and Lever 3 is Reversed.
6. Pull **Lever 1 (UP DISTANT)**. It clears because Lever 2 satisfies the 'OR' condition.
7. Return **Lever 1** and **Lever 2** to Normal.
8. Return **Lever 3** to Normal, pull **Lever 4** (Reversed), and pull **Lever 3** (Reversed).
9. Pull **Lever 5 (UP BRANCH HOME)**. It clears because the points are set to the branch.
10. Pull **Lever 1 (UP DISTANT)** again. It clears because Lever 5 now satisfies the 'OR' condition.

**South Box:**
1. Try to pull **Lever 2 (YARD CROSSOVER)**. It will be locked.
2. Pull **Lever 3 (FRAME RELEASE)** to unlock the crossover.
3. Pull **Lever 2 (YARD CROSSOVER)**. It is now free to move.
4. Pull **Lever 1 (SHUNT AHEAD)**. It clears because the crossover is Reversed.
5. Try to return **Lever 2** to Normal. It is locked by Lever 1.

## Interface Screenshots

*Note: These are photos of the device displaying the North Junction and South Box configurations (perspective corrected by Gemini).*

![North/South Box Screenshot 1](docs/images/0E2AE271-7E60-4A7B-86E7-6BA4216B225F.PNG)

![North/South Box Screenshot 2](docs/images/790B1029-47A5-48D3-9F57-C320E8397BDD.PNG)

![North/South Box Screenshot 3](docs/images/AEC439DA-648A-42DC-90BE-BCF009658DFD.PNG)

![North/South Box Screenshot 4](docs/images/CA38ADD2-954A-4621-9F74-9376CB706CB4.PNG)

## Web UI Screenshots

![Web UI Screenshot 1](docs/images/IMG_0041.jpeg)

![Web UI Screenshot 2](docs/images/IMG_0042.jpeg)

## License

This project is licensed under the GNU General Public License v3.0 (GPLv3). See the [LICENSE](LICENSE) file for more details.
