#pragma once

#include <Arduino_GFX_Library.h>

class Arduino_AXS15231B_JC3248 : public Arduino_AXS15231B
{
public:
  Arduino_AXS15231B_JC3248(
      Arduino_DataBus *bus, int8_t rst = GFX_NOT_DEFINED, uint8_t r = 0,
      bool ips = false, int16_t w = 320, int16_t h = 480,
      uint8_t col_offset1 = 0, uint8_t row_offset1 = 0, uint8_t col_offset2 = 0, uint8_t row_offset2 = 0)
      : Arduino_AXS15231B(bus, rst, r, ips, w, h, col_offset1, row_offset1, col_offset2, row_offset2)
  {
  }

protected:
  void tftInit() override;
};
