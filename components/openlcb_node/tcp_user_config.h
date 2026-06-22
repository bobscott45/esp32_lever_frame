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

/** @file tcp_user_config.h
 *  @brief User-editable TCP/IP driver configuration for OpenLcbCLib
 *
 *  OPTIONAL: If this file is not present the TCP driver will use built-in
 *  defaults.  Copy this file to your project's include path and edit to
 *  override any value.
 *
 *  All values use #ifndef guards in tcp_types.h so defining them here (or
 *  via -D compiler flags) takes priority over the library defaults.
 */

#ifndef __TCP_USER_CONFIG__
#define __TCP_USER_CONFIG__

// =============================================================================
// TCP Receive Accumulation Buffer
// =============================================================================
// Maximum number of bytes that can be accumulated while waiting for a complete
// TCP/IP OpenLCB message (preamble + body).  Must be large enough to hold the
// largest expected single message.  The spec allows multi-part messages, so
// each part must fit within this buffer.

// Minimum practical value: 17 (preamble) + 2 (MTI) + 6 (source) = 25
// Default 1024 is generous for most use cases.

#define USER_DEFINED_TCP_RX_ACCUMULATION_BUFFER_LEN    1024

// =============================================================================
// TCP Transmit Scratch Buffer
// =============================================================================
// Maximum size of the stack-allocated buffer used to build outgoing TCP/IP
// OpenLCB messages (preamble + body).  Must be at least TCP_PREAMBLE_LEN (17)
// plus the largest payload you expect to send.

#define USER_DEFINED_TCP_TX_BUFFER_LEN                 1024

// =============================================================================
// Multi-part Message Accumulation
// =============================================================================
// Maximum number of concurrent multi-part message reassemblies (keyed by
// originating Node ID).  Most point-to-point links need only 1.  Hub nodes
// receiving from multiple spokes may need more.

#define USER_DEFINED_TCP_MAX_MULTIPART_ASSEMBLIES      2

#endif /* __TCP_USER_CONFIG__ */
