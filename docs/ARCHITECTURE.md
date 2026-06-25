# Architecture Documentation

## System Overview

The system is built on a modular, decoupled component architecture designed for ESP-IDF. The primary goal of this architecture is to separate business logic (interlocking and state management) from presentation (LVGL UI) and network communication (OpenLCB). 

This allows independent testing of core logic, easy swapping of displays, and ensures that the UI and network layers do not interfere with each other's state.

## Component Structure

The project is divided into the following key components:

### 1. `lever_core` (Business Logic & State)
This is the central "model" of the system. It handles all state management and business rules without any knowledge of LVGL or OpenLCB.
- **Controller (`controller.c`)**: The main API for requesting state changes. All UI interactions and network events must pass through the controller. It evaluates interlocking rules before accepting changes and broadcasts accepted changes to registered listeners.
- **Interlocking (`interlocking.c`)**: Pure algorithmic logic that evaluates whether a lever can be thrown based on the state of other levers and the configuration rules.
- **Config Manager (`config_manager.c`)**: Handles loading and parsing the system configuration (e.g., from JSON or NVS).
- **State Manager (`state_manager.c`)**: Handles persistence of the current system state (lever positions, locks, active tab) to non-volatile storage (NVS) to survive reboots.
- **System Events (`system_events.c`)**: Defines the global `esp_event` loop definitions used for inter-component communication (e.g. broadcasting lever state changes and configuration reloads).

### 2. `ui_lever_frame` (Presentation)
This component is responsible for rendering the UI using LVGL.
- It observes the `lever_core` for state changes.
- It does **not** modify state directly. When a user interacts with the UI, it calls `controller_request_lever_move()`. If the controller accepts the move, the UI updates its visuals based on the subsequent state change event.
- It has no dependencies on the physical display hardware or OpenLCB.

### 3. `openlcb_node` (Networking)
This component handles the OpenLCB/LCC network stack.
- It listens for incoming network events and translates them into state change requests via `controller_request_lever_move()`.
- It listens for controller state changes and translates them into outgoing OpenLCB events.
- It has no dependencies on LVGL or the UI frame.

### 4. `display_hal` (Hardware Abstraction Layer)
Wraps the vendor-specific Board Support Package (BSP) to abstract the physical display initialization and backlight control.

### 5. `ui_porting` (LVGL Port Abstraction)
Wraps the LVGL porting initialization (display and touch drivers), keeping LVGL internals and threading locks out of the main application code.

### 6. `lcc_drivers` (Hardware Interfaces)
Low-level drivers for physical network interfaces (CAN bus via TWAI, TCP/IP via Wi-Fi/Ethernet). 

### 7. `main` (Orchestrator)
The entry point of the application. It ties all the components together.
- Initializes hardware (NVS, Wi-Fi, I2C, SPI).
- Initializes the default `esp_event` loop for system-wide broadcasts.
- Initializes the physical display via `display_hal`.
- Initializes LVGL and input devices via `ui_porting`.
- Initializes the `lever_core` components.
- Initializes the LVGL UI (`ui_lever_frame_init`).
- Initializes the `openlcb_node`.
- Registers event handlers to route `esp_event` broadcasts to appropriate components (e.g., routing a lever state change event to the OpenLCB producer).

---

## Adding a New Display

Because the UI component (`ui_lever_frame`) is decoupled from the hardware, adding a new display requires zero changes to the UI code. The physical display is abstracted via the `display_hal` component.

To add support for a new physical display:

### 1. Update `display_hal` Component
The `display_hal` component is the central abstraction layer for your physical screen. To add a new board:
- Add your new display's Board Support Package (BSP) to the workspace or `idf_component.yml`.
- Update `components/display_hal/Kconfig` to add a new `config` entry for your board under the `HARDWARE_BOARD` choice.
- Update `components/display_hal/CMakeLists.txt` to require the new BSP component when your Kconfig option is selected.
- Update `components/display_hal/display_hal.c` with an `#elif defined(CONFIG_HARDWARE_BOARD_YOUR_NEW_BOARD)` block implementing the new BSP's initialization sequence.

You must implement the three functions exposed by `display_hal.h`:
```c
esp_err_t display_hal_init(esp_lcd_panel_handle_t *panel_handle_out, esp_lcd_touch_handle_t *touch_handle_out);
esp_err_t display_hal_backlight_on(void);
esp_err_t display_hal_backlight_off(void);
```

### 2. No other changes are needed
Because `main.c` relies only on `display_hal.h` for physical setup, and `ui_porting.h` for LVGL setup, you do not need to change anything outside of the `display_hal` component. The system will automatically use the handles returned by `display_hal_init` to power the UI.

---


## Dual-Core Task Pinning Strategy

The ESP32-S3 contains two cores: **Core 0** (PRO_CPU) and **Core 1** (APP_CPU). To maximize performance and ensure a smooth user interface, this project explicitly delegates tasks to specific cores using `xTaskCreatePinnedToCore()`.

*   **Core 0 (Networking & Logic):** 
    *   **Wi-Fi Driver & LwIP Stack**
    *   **Web Server (`httpd`)**
    *   **OpenLCB / LCC Node Task**
    *   **TCP / GridConnect Server Task**
    *   **CAN Bus (TWAI) RX Task**
    *   **System Event Loop (`esp_event`)**
    *   *Purpose:* Handles all complex protocol parsing, interrupt routines, network bridges, and non-volatile storage (NVS) writes. This frees up the other core entirely.

*   **Core 1 (Graphics & User Interface):** 
    *   **LVGL Timer Handler Task (`lvgl_port_task`)**
    *   *Purpose:* Exclusively runs the graphics rendering pipeline. Because Core 1 is isolated from network interrupts and LCC message processing, it can push RGB pixel data to the PSRAM at maximum bandwidth, eliminating any micro-stutters or tearing that would otherwise occur if it had to yield to network tasks.
