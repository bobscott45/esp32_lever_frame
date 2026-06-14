#ifndef CDI_ARRAY_H
#define CDI_ARRAY_H

#include <stdint.h>

static const uint8_t _cdi_data[] = {
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<cdi xmlns:xsi=\"http://www.w3.org/2001/XMLSchema-instance\" xsi:noNamespaceSchemaLocation=\"http://openlcb.org/trunk/prototypes/xml/schema/cdi.xsd\">\n"
    "  <identification>\n"
    "    <manufacturer>Robert Scott</manufacturer>\n"
    "    <model>ESP32 LCC Lever Frame Node</model>\n"
    "    <hardwareVersion>1.0.0</hardwareVersion>\n"
    "    <softwareVersion>1.0.0</softwareVersion>\n"
    "  </identification>\n"
    "</cdi>\n"
};

#endif // CDI_ARRAY_H
