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
 * @file can_drivers.h
 * @brief Hardware driver prototypes for CAN Drivers.
 *
 * @author Robert Scott
 * @date 2026
 */

#ifndef __OPENLCB_CAN_DRIVERS__
#define __OPENLCB_CAN_DRIVERS__

#include <stdbool.h>

#include "drivers/canbus/can_types.h"


/**
 * @brief  Initializes the CAN hardware driver.
 *
 * Configures the TWAI driver with the appropriate pins, timing, and filters
 * for the OpenLCB standard (125 kbps). Starts the receive task if successful.
 */
void can_driver_initialize(void);

/**
 * @brief  Transmits a CAN message.
 *
 * Sends the message to the physical CAN bus if initialized, and mirrors it
 * to a connected TCP client.
 *
 * @param[in]  can_msg   Pointer to the CAN message to transmit.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
bool can_driver_transmit(can_msg_t *can_msg);

/**
 * @brief  Checks if the CAN driver is ready to transmit.
 *
 * Verifies if the TCP socket is active when in TCP-only mode. If the physical
 * CAN driver is connected, it checks the transmission queue and recovers from
 * bus-off state if necessary.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
bool can_driver_is_clear(void);

/**
 * @brief  Checks if the physical CAN driver is connected.
 *
 * Returns the internal connection status of the TWAI hardware driver, indicating
 * whether it has been successfully started and is currently active.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
bool can_driver_is_connected(void);

/**
 * @brief  Transmits a message to the physical CAN controller only.
 *
 * Sends the outbound data using the TWAI hardware driver without forwarding
 * it to any TCP client. Checks if the driver is connected before sending.
 *
 * @param[in]  can_msg   Pointer to the CAN message to transmit.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
bool can_driver_transmit_physical(can_msg_t *can_msg);


#endif /* __OPENLCB_CAN_DRIVERS__ */
