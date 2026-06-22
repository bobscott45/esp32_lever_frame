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
 * @file i2c_drivers.h
 * @brief Hardware driver prototypes for I2C Drivers.
 *
 * @author Robert Scott
 * @date 2026
 */

#ifndef LCC_NODE_2_OPENLCB_I2C_DRIVERS_H
#define LCC_NODE_2_OPENLCB_I2C_DRIVERS_H
#include "i2cdev.h"

/**
 * @brief  Initializes the I2C bus manager.
 *
 * Calls the internal i2cdev initialization routine to set up the master I2C
 * driver structure and bus configuration for communication with connected peripherals.
 */
void init_i2c_bus(void);

#endif //LCC_NODE_2_OPENLCB_I2C_DRIVERS_H
