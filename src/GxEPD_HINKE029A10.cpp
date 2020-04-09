// class HINKE029A10 : Display class for HINKE029A10
// Author : J-M Zingg
// Edited : ATCnetz.de
//
// Version : see library.properties
//
// License: GNU GENERAL PUBLIC LICENSE V3, see LICENSE
//
// Library: https://github.com/ZinggJM/GxEPD

#include "GxEPD_HINKE029A10.h"

//#define DISABLE_DIAGNOSTIC_OUTPUT

#if defined(ESP8266) || defined(ESP32)
#include <pgmspace.h>
#else
#include <avr/pgmspace.h>
#endif

// Partial Update Delay, may have an influence on degradation
#define HINKE029A10_PU_DELAY 500

const uint8_t HINKE029A10::LUTDefault_full[] =
{
  0x22,  0x11, 0x10, 0x00, 0x10, 0x00, 0x00, 0x11, 0x88, 0x80, 0x80, 0x80, 0x00, 0x00, 0x6A, 0x9B,
  0x9B, 0x9B, 0x9B, 0x00, 0x00, 0x6A, 0x9B, 0x9B, 0x9B, 0x9B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x04, 0x18, 0x04, 0x16, 0x01, 0x0A, 0x0A, 0x0A, 0x0A, 0x02, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 0x04, 0x08, 0x3C, 0x07, 0x00, 0x00, 0x00, 0x00,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

HINKE029A10::HINKE029A10(GxIO& io, int8_t rst, int8_t busy)
  : GxEPD(HINKE029A10_WIDTH, HINKE029A10_HEIGHT), IO(io),
    _current_page(-1), _using_partial_mode(false), _diag_enabled(false),
    _rst(rst), _busy(busy)
{
}

void HINKE029A10::drawPixel(int16_t x, int16_t y, uint16_t color)
{
  //swap(x, y);

  if ((x < 0) || (x >= width()) || (y < 0) || (y >= height())) return;

  //x = HINKE029A10_WIDTH - x - 1;

  // check rotation, move pixel around if necessary
  switch (getRotation())
  {
    case 1:
      swap(x, y);
      x = HINKE029A10_WIDTH - x - 1;
      break;
    case 2:
      x = HINKE029A10_WIDTH - x - 1;
      y = HINKE029A10_HEIGHT - y - 1;
      break;
    case 3:
      swap(x, y);
      y = HINKE029A10_HEIGHT - y - 1;
      break;
  }
  uint16_t i = x / 8 + y * HINKE029A10_WIDTH / 8;
  if (_current_page < 1)
  {
    if (i >= sizeof(_black_buffer)) return;
  }
  else
  {
    y -= _current_page * HINKE029A10_PAGE_HEIGHT;
    if ((y < 0) || (y >= HINKE029A10_PAGE_HEIGHT)) return;
    i = x / 8 + y * HINKE029A10_WIDTH / 8;
  }

  _black_buffer[i] = (_black_buffer[i] & (0xFF ^ (1 << (7 - x % 8)))); // white

  _red_buffer[i] = (_red_buffer[i] | (1 << (7 - x % 8))); // white
  if (color == GxEPD_WHITE) return;
  else if (color == GxEPD_BLACK) _black_buffer[i] = (_black_buffer[i] | (1 << (7 - x % 8)));
  else if (color == GxEPD_RED)_red_buffer[i] = (_red_buffer[i] & (0xFF ^ (1 << (7 - x % 8))));
  else
  {
    if ((color & 0xF100) > (0xF100 / 2))_red_buffer[i] = (_red_buffer[i] & (0xFF ^ (1 << (7 - x % 8))));
    else if ((((color & 0xF100) >> 11) + ((color & 0x07E0) >> 5) + (color & 0x001F)) < 3 * 255 / 2)
    {
      _black_buffer[i] = (_black_buffer[i] | (1 << (7 - x % 8)));
    }
  }
}


void HINKE029A10::init(uint32_t serial_diag_bitrate)
{
  if (serial_diag_bitrate > 0)
  {
    Serial.begin(serial_diag_bitrate);
    _diag_enabled = true;
  }
  IO.init();
  IO.setFrequency(4000000); // 4MHz
  if (_rst >= 0)
  {
    digitalWrite(_rst, HIGH);
    pinMode(_rst, OUTPUT);
  }
  pinMode(_busy, INPUT);

    _writeCommand(0x74);
    _writeData(0x54);
    _writeCommand(0x75);
    _writeData(0x3b);
    _writeCommand(0x01);   // Set MUX as 296
    _writeData(0x27);
    _writeData(0x01);
    _writeData(0x00);
    _writeCommand(0x3A);   // Set 100Hz
    //_writeData(0x25);         // Set 100Hz
    _writeData(0x35);         // Set 130Hz
    //_writeData(0x07);         // Set 150Hz
    _writeCommand(0x3B);   // Set 100Hz
    //_writeData(0x06);         // Set 100Hz
    _writeData(0x04);         // Set 130Hz
    _writeCommand(0x11);   // data enter mode
    _writeData(0x03);
    _writeCommand(0x44);   // set RAM x address start/end, in page 36
    _writeData(0x00);    // RAM x address start at 00h;
    _writeData(0x0f);    // RAM x address end at 0fh(15+1)*8->128
    _writeCommand(0x45);   // set RAM y address start/end, in page 37

    _writeData(0x00);    // RAM y address start at 127h;
    _writeData(0x00);
    _writeData(0x27);    // RAM y address end at 00h;
    _writeData(0x01);
    _writeCommand(0x04);   // set VSH,VSL value
    _writeData(0x41);    //      2D9  15v
    // _writeData(0xc6);   //      2D13   7.8v
    //_writeData(0xc1);    //      2D13   7.5v
    //  _writeData(0x96);    //      2D9   3.8v
    //  _writeData(0xbf);   //      2D13   7.3v
    //  _writeData(0xb8);    //      2D13   6.6v
    //_writeData(0xb4);    //      2D13   6.2v
    //_writeData(0xb2);    //      2D13   6v
    // _writeData(0xad);    //      2D9   5v
    _writeData(0xa8);   //      2D9   5v
    // _writeData(0xA4);    //      2D9   4.6v
    // _writeData(0xA2);    //      2D9   4.4v
    // _writeData(0xa0);    //      2D9   4.2v
    // _writeData(0x9C);    //      2D9   3.8v
    //  _writeData(0x96);    //      2D9   3.8v
    _writeData(0x32);    //      2D9  -15v
    _writeCommand(0x2C);           // vcom
    // _writeData(0x78);           //-3V
    // _writeData(0x6f);           //-2.6V
    //  _writeData(0x6c);           //-2.6V
    _writeData(0x68);           //-2.6V
    // _writeData(0x5F);           //-2.4V
    // _writeData(0x58);           //-2.4V
    _writeCommand(0x3C);   // board
    _writeData(0x33);    //GS1-->GS1

  _writeCommand(0x32);
  for (uint8_t i = 0; i < 70; i++) { // write LUT register with 29bytes instead of 30bytes 2D13
    _writeData(LUTDefault_full[i]);
  }

}

void HINKE029A10::fillScreen(uint16_t color)
{
  uint8_t black = 0x00;
  uint8_t red = 0xFF;
  if (color == GxEPD_WHITE);
  else if (color == GxEPD_BLACK) black = 0xFF;
  else if (color == GxEPD_RED) red = 0x00;
  else if ((color & 0xF100) > (0xF100 / 2))  red = 0x00;
  else if ((((color & 0xF100) >> 11) + ((color & 0x07E0) >> 5) + (color & 0x001F)) < 3 * 255 / 2) black = 0xFF;
  for (uint16_t x = 0; x < sizeof(_black_buffer); x++)
  {
    _black_buffer[x] = black;
    _red_buffer[x] = red;
  }
}

void HINKE029A10::update(void)
{
 // reset required for wakeup
  if (_rst >= 0)
  {
    digitalWrite(_rst, 0);
    delay(1);
    digitalWrite(_rst, 1);
    delay(1);
  }

  set_xy_window(0, 15, 0, 295);
  set_xy_counter(0, 0);

  _writeCommand (0x24); // Write B/W

  for (uint32_t i = 0; i < HINKE029A10_BUFFER_SIZE; i++)
  {
    _writeData((i < sizeof(_black_buffer)) ? ~_black_buffer[i] : 0xFF);
  }

  set_xy_window(0, 15, 0, 295);
  set_xy_counter(0, 0);

  _writeCommand (0x26); // Write RED

  for (uint32_t i = 0; i < HINKE029A10_BUFFER_SIZE; i++)
  {
    _writeData((i < sizeof(_red_buffer)) ? ~_red_buffer[i] : 0xFF);
  }

  _writeCommand (0x22);
  _writeData (0xC7);
  _writeCommand (0x20);

  _waitWhileBusy("SEND DONE");

  _sleep(); // And go to sleep.

}

void HINKE029A10::set_xy_window(unsigned char xs, unsigned char xe, unsigned int ys, unsigned int ye){
  _writeCommand(0x44);    // set RAM x address start/end, in page 36
  _writeData(xs);    // RAM x address start at 00h;
  _writeData(xe);    // RAM x address end at 0fh(12+1)*8->104
  _writeCommand(0x45);   // set RAM y address start/end, in page 37
  _writeData(ys);    // RAM y address start at 0;
  _writeData(ys>>8);
  _writeData(ye);    // RAM y address end at
  _writeData(ye>>8);   // RAM y address end at
}

void HINKE029A10::set_xy_counter(unsigned char x, unsigned char y){
  _writeCommand(0x4E);    // set RAM x address count
  _writeData(x);
  _writeCommand(0x4F);   // set RAM y address count
  _writeData(y);
  _writeData(y>>8);
}

void  HINKE029A10::drawBitmap(const uint8_t *bitmap, uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color, int16_t mode)
{
  if (mode & bm_default) mode |= bm_invert;
  drawBitmapBM(bitmap, x, y, w, h, color, mode);
}

void HINKE029A10::drawExamplePicture(const uint8_t* black_bitmap, const uint8_t* red_bitmap, uint32_t black_size, uint32_t red_size)
{
  drawPicture(black_bitmap, red_bitmap, black_size, red_size, bm_invert_red);
}

void HINKE029A10::drawPicture(const uint8_t* black_bitmap, const uint8_t* red_bitmap, uint32_t black_size, uint32_t red_size, int16_t mode)
{
 drawBitmapBM(black_bitmap, 0, 0, HINKE029A10_WIDTH, HINKE029A10_HEIGHT, GxEPD_BLACK, mode);
 drawBitmapBM(red_bitmap, 0, 0, HINKE029A10_WIDTH, HINKE029A10_HEIGHT, GxEPD_RED, mode);
}

void HINKE029A10::drawBitmap(const uint8_t* bitmap, uint32_t size, int16_t mode)
{
 drawBitmapBM(bitmap, 0, 0, HINKE029A10_WIDTH, HINKE029A10_HEIGHT, GxEPD_BLACK, mode);
}

void HINKE029A10::eraseDisplay(bool using_partial_update)
{
  if (_current_page != -1) return;
  if (using_partial_update)
  {
    if (!_using_partial_mode) _wakeUp();
    _using_partial_mode = true; // remember
    IO.writeCommandTransaction(0x91); // partial in
    _setPartialRamArea(0, 0, HINKE029A10_WIDTH - 1, HINKE029A10_HEIGHT - 1);
    _writeCommand(0x10);
    for (uint32_t i = 0; i < HINKE029A10_BUFFER_SIZE * 2; i++)
    {
      _writeData(0xFF); // white is 0xFF on device
    }
    _writeCommand(0x13);
    for (uint32_t i = 0; i < HINKE029A10_BUFFER_SIZE; i++)
    {
      _writeData(0xFF); // white is 0xFF on device
    }
    _writeCommand(0x12); //display refresh
    _waitWhileBusy("eraseDisplay");
    IO.writeCommandTransaction(0x92); // partial out
  }
  else
  {
    _using_partial_mode = false; // remember
    _wakeUp();
    _writeCommand(0x10);
    for (uint32_t i = 0; i < HINKE029A10_BUFFER_SIZE * 2; i++)
    {
      _writeData(0xFF); // white is 0xFF on device
    }
    _writeCommand(0x13);
    for (uint32_t i = 0; i < HINKE029A10_BUFFER_SIZE; i++)
    {
      _writeData(0xFF); // white is 0xFF on device
    }
    _writeCommand(0x12); //display refresh
    _waitWhileBusy("eraseDisplay");
    _sleep();
  }
}


void HINKE029A10::updateWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h, bool using_rotation)
{
  if (_current_page != -1) return;
  if (using_rotation)
  {
    switch (getRotation())
    {
      case 1:
        swap(x, y);
        swap(w, h);
        x = HINKE029A10_WIDTH - x - w - 1;
        break;
      case 2:
        x = HINKE029A10_WIDTH - x - w - 1;
        y = HINKE029A10_HEIGHT - y - h - 1;
        break;
      case 3:
        swap(x, y);
        swap(w, h);
        y = HINKE029A10_HEIGHT - y  - h - 1;
        break;
    }
  }
  if (x >= HINKE029A10_WIDTH) return;
  if (y >= HINKE029A10_HEIGHT) return;
  // x &= 0xFFF8; // byte boundary, not here, use encompassing rectangle
  uint16_t xe = gx_uint16_min(HINKE029A10_WIDTH, x + w) - 1;
  uint16_t ye = gx_uint16_min(HINKE029A10_HEIGHT, y + h) - 1;
  // x &= 0xFFF8; // byte boundary, not needed here
  uint16_t xs_bx = x / 8;
  uint16_t xe_bx = (xe + 7) / 8;
  if (!_using_partial_mode) _wakeUp();
  _using_partial_mode = true;
  IO.writeCommandTransaction(0x91); // partial in
  _setPartialRamArea(x, y, xe, ye);
  IO.writeCommandTransaction(0x10);
  for (int16_t y1 = y; y1 <= ye; y1++)
  {
    for (int16_t x1 = xs_bx; x1 < xe_bx; x1++)
    {
      uint16_t idx = y1 * (HINKE029A10_WIDTH / 8) + x1;
      uint8_t data = (idx < sizeof(_black_buffer)) ? _black_buffer[idx] : 0x00; // white is 0x00 in buffer
      IO.writeDataTransaction(~data); // white is 0xFF on device
    }
  }
  IO.writeCommandTransaction(0x13);
  for (int16_t y1 = y; y1 <= ye; y1++)
  {
    for (int16_t x1 = xs_bx; x1 < xe_bx; x1++)
    {
      uint16_t idx = y1 * (HINKE029A10_WIDTH / 8) + x1;
      uint8_t data = (idx < sizeof(_red_buffer)) ? _red_buffer[idx] : 0x00; // white is 0x00 in buffer
      IO.writeDataTransaction(~data); // white is 0xFF on device
    }
  }
  IO.writeCommandTransaction(0x12);      //display refresh
  _waitWhileBusy("updateWindow");
  IO.writeCommandTransaction(0x92); // partial out
  delay(HINKE029A10_PU_DELAY); // don't stress this display
}

void HINKE029A10::updateToWindow(uint16_t xs, uint16_t ys, uint16_t xd, uint16_t yd, uint16_t w, uint16_t h, bool using_rotation)
{
  if (!_using_partial_mode) _wakeUp();
  _using_partial_mode = true;
  _writeToWindow(xs, ys, xd, yd, w, h, using_rotation);
  IO.writeCommandTransaction(0x12);      //display refresh
  _waitWhileBusy("updateToWindow");
  delay(HINKE029A10_PU_DELAY); // don't stress this display
}

void HINKE029A10::_writeToWindow(uint16_t xs, uint16_t ys, uint16_t xd, uint16_t yd, uint16_t w, uint16_t h, bool using_rotation)
{
  if (using_rotation)
  {
    switch (getRotation())
    {
      case 1:
        swap(xs, ys);
        swap(xd, yd);
        swap(w, h);
        xs = HINKE029A10_WIDTH - xs - w - 1;
        xd = HINKE029A10_WIDTH - xd - w - 1;
        break;
      case 2:
        xs = HINKE029A10_WIDTH - xs - w - 1;
        ys = HINKE029A10_HEIGHT - ys - h - 1;
        xd = HINKE029A10_WIDTH - xd - w - 1;
        yd = HINKE029A10_HEIGHT - yd - h - 1;
        break;
      case 3:
        swap(xs, ys);
        swap(xd, yd);
        swap(w, h);
        ys = HINKE029A10_HEIGHT - ys  - h - 1;
        yd = HINKE029A10_HEIGHT - yd  - h - 1;
        break;
    }
  }
  if (xs >= HINKE029A10_WIDTH) return;
  if (ys >= HINKE029A10_HEIGHT) return;
  if (xd >= HINKE029A10_WIDTH) return;
  if (yd >= HINKE029A10_HEIGHT) return;
  // the screen limits are the hard limits
  uint16_t xde = gx_uint16_min(HINKE029A10_WIDTH, xd + w) - 1;
  uint16_t yde = gx_uint16_min(HINKE029A10_HEIGHT, yd + h) - 1;
  IO.writeCommandTransaction(0x91); // partial in
  // soft limits, must send as many bytes as set by _SetRamArea
  uint16_t yse = ys + yde - yd;
  uint16_t xss_d8 = xs / 8;
  uint16_t xse_d8 = xss_d8 + _setPartialRamArea(xd, yd, xde, yde);
  IO.writeCommandTransaction(0x10);
  for (int16_t y1 = ys; y1 <= yse; y1++)
  {
    for (int16_t x1 = xss_d8; x1 < xse_d8; x1++)
    {
      uint16_t idx = y1 * (HINKE029A10_WIDTH / 8) + x1;
      uint8_t data = (idx < sizeof(_black_buffer)) ? _black_buffer[idx] : 0x00; // white is 0x00 in buffer
      IO.writeDataTransaction(~data); // white is 0xFF on device
    }
  }
  IO.writeCommandTransaction(0x13);
  for (int16_t y1 = ys; y1 <= yse; y1++)
  {
    for (int16_t x1 = xss_d8; x1 < xse_d8; x1++)
    {
      uint16_t idx = y1 * (HINKE029A10_WIDTH / 8) + x1;
      uint8_t data = (idx < sizeof(_red_buffer)) ? _red_buffer[idx] : 0x00; // white is 0x00 in buffer
      IO.writeDataTransaction(~data); // white is 0xFF on device
    }
  }
  IO.writeCommandTransaction(0x92); // partial out
}

void HINKE029A10::powerDown()
{
  _using_partial_mode = false; // force _wakeUp()
  _sleep();
}

uint16_t HINKE029A10::_setPartialRamArea(uint16_t x, uint16_t y, uint16_t xe, uint16_t ye)
{
  x &= 0xFFF8; // byte boundary
  xe = (xe - 1) | 0x0007; // byte boundary - 1
  IO.writeCommandTransaction(0x90); // partial window
  //IO.writeDataTransaction(x / 256);
  IO.writeDataTransaction(x % 256);
  //IO.writeDataTransaction(xe / 256);
  IO.writeDataTransaction(xe % 256);
  IO.writeDataTransaction(y / 256);
  IO.writeDataTransaction(y % 256);
  IO.writeDataTransaction(ye / 256);
  IO.writeDataTransaction(ye % 256);
  IO.writeDataTransaction(0x01); // don't see any difference
  //IO.writeDataTransaction(0x00); // don't see any difference
  return (7 + xe - x) / 8; // number of bytes to transfer per line
}

void HINKE029A10::_writeCommand(uint8_t command)
{
  IO.writeCommandTransaction(command);
}

void HINKE029A10::_writeData(uint8_t data)
{
  IO.writeDataTransaction(data);
}

void HINKE029A10::_waitWhileBusy(const char* comment)
{
  unsigned long start = micros();
  while (1)
  {
    if (digitalRead(_busy) == 0) break;
    delay(1);
    if (micros() - start > 30000000) // >14.9s !
    {
      if (_diag_enabled) Serial.println("Busy Timeout!");
      break;
    }
  }
  if (comment)
  {
#if !defined(DISABLE_DIAGNOSTIC_OUTPUT)
    if (_diag_enabled)
    {
      unsigned long elapsed = micros() - start;
      Serial.print(comment);
      Serial.print(" : ");
      Serial.println(elapsed);
    }
#endif
  }
  (void) start;
}

void HINKE029A10::_wakeUp()
{
  // reset required for wakeup
  if (_rst >= 0)
  {
    digitalWrite(_rst, 0);
    delay(10);
    digitalWrite(_rst, 1);
    delay(10);
  }
/*
  _writeCommand(0x06);
  _writeData (0x17);
  _writeData (0x17);
  _writeData (0x17);
  _writeCommand(0x04);
  _waitWhileBusy("_wakeUp Power On");
  _writeCommand(0X00);
  _writeData(0x8f);
  _writeCommand(0X50);
  _writeData(0x77);
  _writeCommand(0x61);
  _writeData (0x80);
  _writeData (0x01);
  _writeData (0x28);
  */
}

void HINKE029A10::_sleep(void)
{
  _waitWhileBusy("_sleep Going to sleep.");
  if (_rst >= 0)
  {
  _writeCommand (0x10); // sleep!
  _writeData (0x11);
  }
}

void HINKE029A10::_rotate(uint16_t& x, uint16_t& y, uint16_t& w, uint16_t& h)
{
  switch (getRotation())
  {
    case 1:
      swap(x, y);
      swap(w, h);
      x = HINKE029A10_WIDTH - x - w - 1;
      break;
    case 2:
      x = HINKE029A10_WIDTH - x - w - 1;
      y = HINKE029A10_HEIGHT - y - h - 1;
      break;
    case 3:
      swap(x, y);
      swap(w, h);
      y = HINKE029A10_HEIGHT - y - h - 1;
      break;
  }
}
void HINKE029A10::drawCornerTest(uint8_t em)
{
  if (_current_page != -1) return;
  _using_partial_mode = false;
  _wakeUp();
  _writeCommand(0x10);
  for (uint32_t y = 0; y < HINKE029A10_HEIGHT; y++)
  {
    for (uint32_t x = 0; x < HINKE029A10_WIDTH / 8; x++)
    {
      uint8_t data = 0xFF; // white is 0xFF on device
      if ((x < 1) && (y < 8)) data = 0x00;
      if ((x > HINKE029A10_WIDTH / 8 - 3) && (y < 16)) data = 0x00;
      if ((x > HINKE029A10_WIDTH / 8 - 4) && (y > HINKE029A10_HEIGHT - 25)) data = 0x00;
      if ((x < 4) && (y > HINKE029A10_HEIGHT - 33)) data = 0x00;
      _writeData(data);
    }
  }
  _writeCommand(0x13);
  for (uint32_t i = 0; i < HINKE029A10_BUFFER_SIZE; i++)
  {
    _writeData(0xFF); // white is 0xFF on device
  }
  _writeCommand(0x12); //display refresh
  _waitWhileBusy("drawCornerTest");
  _sleep();
}

