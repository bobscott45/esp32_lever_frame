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

/**
 * @file tcp_drivers.h
 * @brief Hardware driver prototypes for TCP/IP Drivers.
 *
 * @author Robert Scott
 * @date 2026
 */

#ifndef __OPENLCB_TCP_DRIVERS__
#define __OPENLCB_TCP_DRIVERS__


/**
 * @brief  Initializes the TCP/IP driver.
 *
 * Spawns the background FreeRTOS task that waits for a WiFi connection
 * and sets up the GridConnect TCP server for OpenLCB network communication.
 */
void tcp_driver_initialize(void);



#endif /* __OPENLCB_TCP_DRIVERS__ */
