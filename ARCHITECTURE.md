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

### 4. `lcc_drivers` (Hardware Interfaces)
Low-level drivers for physical network interfaces (CAN bus via TWAI, TCP/IP via Wi-Fi/Ethernet). 

### 5. `main` (Orchestrator)
The entry point of the application. It ties all the components together.
- Initializes hardware (NVS, Wi-Fi, I2C, SPI).
- Initializes the physical display (BSP) and attaches it to LVGL.
- Initializes the `lever_core` components.
- Initializes the LVGL UI (`ui_lever_frame_init`).
- Initializes the `openlcb_node`.
- Registers the interconnecting callbacks (e.g., hooking `openlcb` event generation to the `controller`'s state change callback).

---

## Adding a New Display

Because the UI component (`ui_lever_frame`) is decoupled from the hardware, adding a new display requires zero changes to the UI code. The physical display is initialized entirely in `main.c`.

To add support for a new display (e.g., swapping to a P4 screen), follow these steps:

### 1. Add the BSP Component
Find the Board Support Package (BSP) for your new display in the ESP Component Registry or place it in your `components/` folder. Add it as a dependency in `main/idf_component.yml`:
```yaml
dependencies:
  your_vendor/new_display_bsp: "*"
```

### 2. Update `main/CMakeLists.txt`
Change the `PRIV_REQUIRES` line to include your new BSP component instead of the old one.

### 3. Replace Initialization in `main.c`
In `main.c`, remove the includes and initialization code for the old display (e.g., `waveshare_esp32_s3_touch_lcd_4_3.h`) and replace it with your new display's API.

You need to obtain two handles from your new BSP:
1. `esp_lcd_panel_handle_t` (The LCD panel handle)
2. `esp_lcd_touch_handle_t` (The touch controller handle)

Example:
```c
#include "new_display_bsp.h"

// ... inside app_main() ...

esp_lcd_panel_handle_t panel_handle = NULL;
esp_lcd_touch_handle_t touch_handle = NULL;

// Call your new display's init function
new_display_bsp_init(&panel_handle, &touch_handle);

// Attach the handles to LVGL
const lvgl_port_display_cfg_t disp_cfg = {
    .io_handle = NULL,
    .panel_handle = panel_handle,
    .buffer_size = 800 * 480 * sizeof(uint16_t),
    .double_buffer = true,
    .hres = 800,
    .vres = 480,
    .monochrome = false,
    .color_format = LV_COLOR_FORMAT_RGB565,
    .flags = {
        .buff_dma = true,
        .buff_spiram = true,
    }
};
lvgl_disp = lvgl_port_add_disp(&disp_cfg);

// Attach touch
const lvgl_port_touch_cfg_t touch_cfg = {
    .disp = lvgl_disp,
    .handle = touch_handle,
};
lvgl_port_add_touch(&touch_cfg);
```

### 4. Adjust Backlight and Sleep Logic
If your `main.c` handles display dimming or sleep logic, replace the specific backlight calls (like `waveshare_rgb_lcd_bl_on()`) with the equivalent functions provided by your new BSP.

---

## Project Structure Improvements

The current architecture is solid, but the following improvements could be considered as the project scales:

1. **Hardware Abstraction Layer for Displays (Display HAL):** 
   Currently, `main.c` directly calls functions like `waveshare_rgb_lcd_bl_on()`. If you plan to support multiple different displays simultaneously in the same codebase using `#ifdef` macros, it would be cleaner to create a lightweight wrapper component (e.g., `components/display_hal`) that exposes a generic `display_init()`, `display_backlight_on()`, and `display_backlight_off()`. `main.c` would call this wrapper, and the `#ifdef` logic would be contained within the wrapper.

2. **Decouple LVGL Porting:**
   The `bsp/lvgl_port` code is currently housed inside the `main` component directory or as a separate component. Ensuring that LVGL initialization is handled consistently across different screens might benefit from a dedicated `ui_porting` component if you migrate away from standard ESP-IDF BSPs.

3. **Event Loop Usage:**
   Currently, components communicate via direct function callbacks (e.g., `controller_set_state_changed_cb`). For a larger system, moving to `esp_event` (the default ESP-IDF event loop) for broadcasting system-wide events (like "Config Reloaded", "Network Connected", or "Lever State Changed") would decouple components even further, allowing multiple listeners without managing arrays of function pointers.
