/*
 * This file is part of esp32_lever_frame.
 *
 * esp32_lever_frame is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * esp32_lever_frame is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with esp32_lever_frame.  If not, see <https://www.gnu.org/licenses/>.
 */

/** @file can_user_config.h
 *  @brief User-editable CAN bus driver configuration for OpenLcbCLib
 *
 *  OPTIONAL: If this file is not present the CAN driver will use built-in
 *  defaults.  Copy this file to your project's include path and edit to
 *  override any value.
 *
 *  All values use #ifndef guards in can_types.h so defining them here (or
 *  via -D compiler flags) takes priority over the library defaults.
 */

#ifndef __CAN_USER_CONFIG__
#define __CAN_USER_CONFIG__

// =============================================================================
// CAN Message Buffer Pool
// =============================================================================
// Number of raw CAN message buffers in the driver pool.  Each buffer holds one
// CAN 2.0 frame (8 data bytes + header).  Tune for your platform's available
// RAM and expected bus traffic.

// Maximum value is 254 (0xFE).

#define USER_DEFINED_CAN_MSG_BUFFER_DEPTH            20     // must be >= 1; enforced by compiler

#endif /* __CAN_USER_CONFIG__ */
