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
 * @file lcc_drivers.h
 * @brief Hardware driver prototypes for OLCB/LCC Drivers.
 *
 * @author Robert Scott
 * @date 2026
 */

#ifndef __OPENLCB_DRIVERS__
#define __OPENLCB_DRIVERS__

#include <stdint.h>

#include "openlcb/openlcb_types.h"
#include "openlcb_user_config.h"


/**
 * @brief  Initializes the LCC drivers.
 *
 * Sets up node storage by loading from NVS and creates the OpenLCB mutex
 * for shared resource protection.
 */
void lcc_drivers_initialize(void);

/**
 * @brief  Locks shared resources for OpenLCB.
 *
 * Acquires the OpenLCB mutex if it exists, blocking indefinitely until
 * the mutex is available. This protects shared configuration memory from
 * concurrent access.
 */
void lcc_drivers_lock_shared_resources(void);

/**
 * @brief  Unlocks shared resources for OpenLCB.
 *
 * Releases the OpenLCB mutex if it exists, allowing other tasks to access
 * shared configuration memory and resources.
 */
void lcc_drivers_unlock_shared_resources(void);

/**
 * @brief  Reads from configuration memory.
 *
 * Copies the specified number of bytes from the internal configuration memory
 * cache into the provided buffer. Syncs from the global config manager before reading.
 *
 * @param[in]  openlcb_node   Pointer to the OpenLCB node context.
 * @param[in]  address        Start address in configuration memory to read from.
 * @param[in]  count          Number of bytes to read.
 * @param[out] buffer         Pointer to the buffer where read data will be stored.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
uint16_t lcc_drivers_config_mem_read(openlcb_node_t *openlcb_node, uint32_t address, uint16_t count, configuration_memory_buffer_t *buffer);

/**
 * @brief  Writes to configuration memory.
 *
 * Updates the internal configuration memory cache with the provided data.
 * If data changes, the cache is saved to NVS and global configuration is synchronized.
 *
 * @param[in]  openlcb_node   Pointer to the OpenLCB node context.
 * @param[in]  address        Start address in configuration memory to write to.
 * @param[in]  count          Number of bytes to write.
 * @param[in]  buffer         Pointer to the buffer containing data to write.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
uint16_t lcc_drivers_config_mem_write(openlcb_node_t *openlcb_node, uint32_t address, uint16_t count, configuration_memory_buffer_t *buffer);

/**
 * @brief  Handles a reboot request.
 *
 * Processes a node reboot command from the network or internal triggers.
 *
 * @param[in]  statemachine_info                  Pointer to OpenLCB state machine info.
 * @param[in]  config_mem_operations_request_info Pointer to the operations request info.
 */
void lcc_drivers_reboot(openlcb_statemachine_info_t *statemachine_info, config_mem_operations_request_info_t *config_mem_operations_request_info);

/**
 * @brief  Handles a firmware write request.
 *
 * Processes a firmware update operation for the OpenLCB node.
 *
 * @param[in]  statemachine_info               Pointer to OpenLCB state machine info.
 * @param[in]  config_mem_write_request_info   Pointer to the write request info.
 * @param[in]  write_result                    Result object for the write operation.
 */
void lcc_drivers_firmware_write(openlcb_statemachine_info_t *statemachine_info, config_mem_write_request_info_t *config_mem_write_request_info, write_result_t write_result);

/**
 * @brief  Handles a freeze request.
 *
 * Pauses normal node operations and enters a frozen state for maintenance.
 *
 * @param[in]  statemachine_info                  Pointer to OpenLCB state machine info.
 * @param[in]  config_mem_operations_request_info Pointer to the operations request info.
 */
void lcc_drivers_freeze(openlcb_statemachine_info_t *statemachine_info, config_mem_operations_request_info_t *config_mem_operations_request_info);

/**
 * @brief  Handles an unfreeze request.
 *
 * Resumes normal node operations after a maintenance freeze.
 *
 * @param[in]  statemachine_info                  Pointer to OpenLCB state machine info.
 * @param[in]  config_mem_operations_request_info Pointer to the operations request info.
 */
void lcc_drivers_unfreeze(openlcb_statemachine_info_t *statemachine_info, config_mem_operations_request_info_t *config_mem_operations_request_info);

/**
 * @brief  Retrieves the node configuration memory.
 *
 * Returns a pointer to the internal node configuration memory cache structure.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
node_config_memory_t* lcc_drivers_get_config(void);

#endif /* __OPENLCB_DRIVERS__ */
