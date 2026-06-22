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

Because the UI component (`ui_lever_frame`) is decoupled from the hardware, adding a new display requires zero changes to the UI code. The physical display is initialized entirely in `main.c`.

To add support for a new display (e.g., swapping to a P4 screen), follow these steps:

### 1. Update `display_hal` Component
The `display_hal` component is the central abstraction layer for your physical screen. To swap displays:
- Update `components/display_hal/idf_component.yml` to require your new display's Board Support Package (BSP).
- Update `components/display_hal/CMakeLists.txt` to `REQUIRES` the new BSP.
- Rewrite `components/display_hal/display_hal.c` to use your new BSP's initialization sequence.

You must implement the three functions exposed by `display_hal.h`:
```c
esp_err_t display_hal_init(esp_lcd_panel_handle_t *panel_handle_out, esp_lcd_touch_handle_t *touch_handle_out);
esp_err_t display_hal_backlight_on(void);
esp_err_t display_hal_backlight_off(void);
```

### 2. No other changes are needed
Because `main.c` relies only on `display_hal.h` for physical setup, and `ui_porting.h` for LVGL setup, you do not need to change anything outside of the `display_hal` component. The system will automatically use the handles returned by `display_hal_init` to power the UI.

---

## Recent Project Structure Improvements

The following architectural improvements were recently implemented to ensure massive scalability and cleaner decoupling:

1. **Hardware Abstraction Layer for Displays (Display HAL):** 
   A lightweight `display_hal` component was introduced to wrap all vendor-specific BSP calls. This prevents `main.c` from being polluted with display-specific macros and initialization structures.

2. **Decoupled LVGL Porting:**
   The `bsp/lvgl_port` code was wrapped behind a clean `ui_porting` component. This ensures LVGL memory allocation, threading locks, and touch dispatching are handled safely away from the application's business logic.

3. **Event Loop Usage:**
   Direct function pointer callbacks were completely eliminated between the core logic and the networking/UI layers. The project now uses the standard `esp_event` loop (`LEVER_SYSTEM_EVENTS`) to broadcast `EVENT_LEVER_STATE_CHANGED` and `EVENT_CONFIG_RELOADED`. This allows multiple independent components (like OpenLCB and the UI) to listen to state changes without tightly coupling their lifecycles.
