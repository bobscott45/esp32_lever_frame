# ESP32 Lever Frame User Guide

This guide explains how to operate the touch-screen lever frame, configure interlocking rules, and connect the device to an OpenLCB/LCC network (e.g., for use with JMRI).
---

## 1. Using the Device for the First Time

When you first power on the ESP32 Lever Frame, it will not have your home Wi-Fi details. Instead, it will create its own temporary Wi-Fi network (an Access Point) so you can configure it.

1. **Connect to the Device's AP**: On your computer, tablet, or smartphone, open your Wi-Fi settings. Look for the network (SSID) named `Lever-Frame-Config` and connect to it. The default password is `signalman`.
2. **Access the Web Config**: Once connected, open a web browser and navigate to the device's default IP address: `http://192.168.4.1`.
3. **Configure Home Network**: In the Web Configuration Interface, enter your Home Wi-Fi Network Name (SSID) and Password.
4. **Save and Reboot**: Click "Save & Apply". The device will restart and connect to your home network. (Note: The `Lever-Frame-Config` Access Point will still remain available alongside your home connection as a fallback).
5. **Find the New IP**: Once connected to your home network, swipe down on the device's touchscreen to view its newly assigned `Home Wi-Fi IP`. You can now connect to this new IP from any browser on your home network to continue configuration.

---

## 2. On-Device Interface (LVGL Touchscreen)

![Lever Frame Display](images/0E2AE271-7E60-4A7B-86E7-6BA4216B225F.PNG)

### Display Components
1. **Lever Label**: Shows the configured name for the lever. Tap this label to open the lever's individual settings menu.
2. **Lever Handle**: Tap the long vertical switch to toggle the lever between Normal and Reversed.
3. **Collar Button**: Tap this dark button below the lever to manually lock or unlock the lever.
4. **Tab Label**: Swipe left/right or tap the label at the bottom of the screen to navigate between frames.
5. **Mechanical Pin Indicator**: Appears in the center of the lever when it is mechanically locked by the system.

### Using the Levers
*   **Throwing a Lever**: To throw (reverse) or return (normalize) a lever, simply tap the long vertical lever handle (2) on the touchscreen. It will automatically swing to the opposite position. 
*   **Normal vs. Reversed (On/Off)**: The lever toggles between the **Normal** (pushed back, usually representing the default/safe state or Off) and **Reversed** (pulled forward, representing the cleared/thrown state or On) positions.
*   **Manual Collar Lock**: Below each lever is a dark button representing a physical lever collar (3). 
    *   Tap this button to manually lock the lever. It will turn **Red** and display **LOCKED**.
    *   While manually locked, the lever switch becomes unclickable and cannot be moved from the screen.
    *   Tap the collar button again to unlock it.

### System Interlocking States
The system enforces safety interlocking rules (configured via the Web UI). The collar button will automatically change its display to reflect the system's safety state:
*   **UNLOCKED**: The lever is free to be moved.
*   **INTERLOCK**: The system's interlocking rules currently prohibit the lever from moving. The switch becomes unclickable, and a mechanical pin indicator (5) will appear.
*   **ALARM** (Orange): The lever has been forced into an illegal state by an external network command that violates the local interlocking rules. This serves as a critical warning.

---

## 3. On-Device System Settings

The system settings menu can be accessed by **swiping down from the top of the screen**. This menu provides access to global operational behaviors. 

*   **Override Policy**: Determines how the device reacts when an external OpenLCB/LCC network event commands a lever to change state, but that state would violate local interlocking rules:
    *   **Strict Local (Reject Override)**: The device strictly enforces local safety rules. The incoming network event is safely rejected and ignored.
    *   **Override Allowed (Central)**: The network command is trusted over local rules. The lever will move, ignoring local interlocking.
    *   **Accept & Warn (Alarm Mode)**: The network command is accepted and the lever moves, but any levers involved in the resulting rules conflict will turn **Orange** and display **ALARM**.
*   **Startup Mode**:
    *   **Restore Last**: The device remembers the exact state of all levers when it was powered off, and restores them on boot.
    *   **Safe Default**: All levers are forced to the Normal (safe) state on boot.

---

## 4. Web Configuration Interface

> **Note on Screen Lock**: When a user connects to the Web Configuration Interface from their browser, the physical touchscreen is temporarily locked. This prevents conflicting physical inputs while configuration is underway.

The Web Configuration Interface is the primary tool for managing the ESP32 Lever Frame's advanced settings, interlocking rules, and network integration. Connect a computer or phone to the same Wi-Fi network as the ESP32, and navigate to the device's IP address in a web browser.

### Global System Settings
These settings govern the physical device's operation and network connectivity.

*   **Wi-Fi Network Name (SSID) & Password:** Allows the ESP32 to connect to a local Wi-Fi network. This is required for the device to communicate with a JMRI LCC Hub over the network.
*   **AP Password:** The password used when connecting directly to the device's built-in "Lever-Frame-Config" Access Point.
*   **Global LCC Publishing:** A master toggle switch for Layout Command Control (OpenLCB) network events. Unchecking this prevents the device from sending any layout control messages, effectively putting it in an "offline" or "simulator-only" mode.
*   *(The Startup Mode and Network Override Policy can also be set here and synchronize with the on-device settings).*

### Frame (Tab) Management
The device can host multiple "Frames" (visualized as tabs on the touch screen). 

*   **Frame Name:** The title of the frame, which appears on the tab at the bottom of the device's screen.
*   **Label Text Lines & Height:** These settings adjust the typography on the lever labels. Increasing the lines allows for longer, wrapped descriptions (e.g., "UP MAIN\nHOME"), while the height adjusts the font size.

### Lever Configuration
Each individual lever has its own configuration block.

*   **Label Text:** The text displayed on the lever's label. Use `\n` to force a line break.
*   **Lever Color / Function:** Determines the visual color of the lever on the screen, adhering to traditional railway signalling standards:
    *   *Home Signal (Red):* Stop signals.
    *   *Distant Signal (Yellow):* Caution approach signals.
    *   *Points (Black):* Turnouts or crossovers.
    *   *Facing Points (Blue):* Facing Point Locks (FPLs).
    *   *Level Crossing (Brown):* Level Crossing Gates or Wickets.
    *   *Release / Gong (Green):* Special releases or bells.
    *   *Spare (White):* Unused levers.

### Advanced Settings: Interlocking
Interlocking is the core safety mechanism of a lever frame. It physically (or in this case, virtually) prevents conflicting train movements by locking levers based on the positions of other levers.

*   **Add Rule:** Creates a new interlocking dependency for the lever. The rule defines a condition that *must* be true in order for this lever to be pulled (Reversed).
*   **When reversed, this lever locks [Lever X]:** Select the target lever that this lever depends on.
*   **[NORMAL / REVERSED]:** The required state of the target lever. 
    *   *Example 1 (Points locking a Signal):* If Signal Lever 2 requires the route to be set, you might add a rule: "When reversed, this lever locks Lever 1 (Points) REVERSED". This means Lever 2 cannot be pulled unless Lever 1 is already pulled. Furthermore, once Lever 2 is pulled, Lever 1 is locked in the Reversed position and cannot be pushed back to Normal until Signal Lever 2 is restored.
    *   *Example 2 (Conflicting Signals):* If Signal Lever 3 conflicts with Signal Lever 4, you can add a rule to Lever 3: "When reversed, this lever locks Lever 4 NORMAL". This ensures Lever 3 can only be pulled if Lever 4 is at Danger (Normal), and pulling Lever 3 will lock Lever 4 at Danger.
*   **OR [Lever Y] is [NORMAL / REVERSED]:** An optional secondary condition. This allows conditional or "Route" locking logic. The rule will be satisfied if *either* the primary condition OR this alternative condition is met. 
    *   *Example 3 (Conditional Distant Signal):* A Distant Signal (Lever 1) might be allowed to clear if the train is routed to the Main line (Signal 2 is cleared) OR if it is routed to the Branch line (Signal 5 is cleared). You would configure Lever 1 to require Lever 2 to be REVERSED, **OR** Lever 5 to be REVERSED.

### Advanced Settings: LCC Events
These fields map the physical movements of the lever to OpenLCB/LCC network events, allowing the lever to control physical layout hardware (like point motors or track signals) via JMRI.

*   **Normal Event ID:** The 8-byte hexadecimal identifier broadcasted to the network when the lever is pushed back to the 'Normal' position.
*   **Reversed Event ID:** The 8-byte hexadecimal identifier broadcasted when the lever is pulled to the 'Reversed' position.
*   **Auto-fill Button:** A convenience feature that automatically populates the Normal and Reversed Event IDs by prefixing the device's unique hardware Node ID, ensuring the generated events are globally unique and follow LCC standards.
*   **Enabled (Checkbox):** If unchecked, the specific lever will silently change states on the screen without broadcasting any LCC events to the network.

### Saving, Exporting & Simulator
*   **Export Config:** Downloads a `.json` file containing the entire layout configuration to a computer as a backup.
*   **Import Config:** Uploads a `.json` file to the Web UI. *Note: Click "Save & Apply" after importing to push the changes to the device.*
*   **Save & Apply:** Uploads the configuration to the ESP32. The device will automatically restart and apply the new layout.
*   **Live Preview & Interlocking Summary:** The right-hand side of the UI provides a real-time simulator. Levers can be clicked in the web browser to test the interlocking logic before saving it to the device. The summary panel translates raw interlocking rules into plain English to help spot logic errors.

---

## 5. JMRI Integration

The ESP32 Lever Frame natively supports Layout Command Control (OpenLCB / LCC). This allows it to seamlessly integrate with JMRI.

1. **LCC Connection**: Ensure your JMRI instance is connected to the same home network as the Lever Frame. In JMRI, configure an LCC network connection (usually via the GridConnect TCP/IP protocol) and point it to the Lever Frame's IP address using the standard LCC port **12021**.
2. **LCC Network Tree & Configuration**: The Lever Frame will appear as an independent LCC Node in the JMRI LCC Network Tree. You can expand the node and select **Configure** to open the Configuration Description Information (CDI) interface.
   * From within JMRI's CDI interface, you can remotely view and change the device's **Global Settings**, just as you would in the Web UI.
   * Available settings include **LCC Master Enable**, **Startup Mode**, and the **Conflict Policy** (Strict Local, Override Allowed, or Accept & Warn). Modifying these in JMRI will instantly update the lever frame.
3. **Observing Traffic**: You can use the JMRI **LCC Traffic Monitor** to watch the raw communication between JMRI and the lever frame. This is extremely useful for debugging. When you throw a lever on the physical touchscreen, you will see the corresponding Event ID broadcast in the Traffic Monitor.
4. **Sending Test Commands**: JMRI provides an **LCC Send Frame** utility. You can manually input a lever's Normal or Reversed Event ID into this tool and broadcast it. The physical lever on the screen should throw automatically in response, allowing you to quickly test your network connectivity and Override Policies.
5. **JMRI Logic Configuration**: 
   * Each lever's movement generates a unique **Normal Event ID** and **Reversed Event ID** (configured in the Web UI).
   * You can use these Event IDs in JMRI's **Event Table**, **Logix**, or **LogixNG** to trigger physical turnouts, signals, or routes on your layout.
   * Conversely, JMRI can broadcast these Event IDs to the network to remotely control the Lever Frame.
