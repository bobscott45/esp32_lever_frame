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

#ifndef CDI_ARRAY_H
#define CDI_ARRAY_H

#include <stdint.h>

static const uint8_t _cdi_data[] = {
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<cdi xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://openlcb.org/trunk/prototypes/xml/schema/cdi.xsd\">\n"
    "  <identification>\n"
    "    <manufacturer>Robert Scott</manufacturer>\n"
    "    <model>ESP32 LCC Lever Frame Node</model>\n"
    "    <hardwareVersion>1.3.0</hardwareVersion>\n"
    "    <softwareVersion>1.3.0</softwareVersion>\n"
    "  </identification>\n"
    "  <segment space=\"253\" origin=\"0\">\n"
    "    <group>\n"
    "      <name>Node Information</name>\n"
    "      <description>User-defined name and description for this node.</description>\n"
    "      <string size=\"63\">\n"
    "        <name>Node Name</name>\n"
    "      </string>\n"
    "      <string size=\"64\">\n"
    "        <name>Node Description</name>\n"
    "      </string>\n"
    "    </group>\n"
    "    <group>\n"
    "      <name>Global Settings</name>\n"
    "      <description>High-level settings for the Lever Frame.</description>\n"
    "      <int size=\"1\">\n"
    "        <name>LCC Master Enable</name>\n"
    "        <description>Whether this node produces LCC events.</description>\n"
    "        <min>0</min>\n"
    "        <max>1</max>\n"
    "        <default>1</default>\n"
    "        <map>\n"
    "          <relation><property>0</property><value>Disabled</value></relation>\n"
    "          <relation><property>1</property><value>Enabled</value></relation>\n"
    "        </map>\n"
    "      </int>\n"
    "      <int size=\"1\">\n"
    "        <name>Startup Mode</name>\n"
    "        <description>Lever states on boot.</description>\n"
    "        <min>0</min>\n"
    "        <max>1</max>\n"
    "        <default>1</default>\n"
    "        <map>\n"
    "          <relation><property>0</property><value>Safe Default State</value></relation>\n"
    "          <relation><property>1</property><value>Restore Last State</value></relation>\n"
    "        </map>\n"
    "      </int>\n"
    "      <int size=\"1\">\n"
    "        <name>Conflict Policy</name>\n"
    "        <description>What happens when LCC asks to move a locally locked lever.</description>\n"
    "        <min>0</min>\n"
    "        <max>2</max>\n"
    "        <default>0</default>\n"
    "        <map>\n"
    "          <relation><property>0</property><value>Strict Local</value></relation>\n"
    "          <relation><property>1</property><value>Override Allowed</value></relation>\n"
    "          <relation><property>2</property><value>Accept &amp; Warn (Alarm)</value></relation>\n"
    "        </map>\n"
    "      </int>\n"
    "    </group>\n"
    "  </segment>\n"
    "</cdi>\n"
};

#endif // CDI_ARRAY_H
