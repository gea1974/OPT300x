/*

Arduino/ESP32 library for Texas Instruments OPT300x Digital Ambient Light Sensor
Famyily Tested on OPT 3001 and 3004

---

The MIT License (MIT)

Copyright (c) 2015 ClosedCube Limited

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/
#include <Wire.h>

#include "OPT300x.h"

OPT300x::OPT300x() {}

OPT300x_ErrorCode OPT300x::begin(uint8_t address) {
  OPT300x_ErrorCode error = NO_ERROR;
  _address = address;

  return NO_ERROR;
}

uint16_t OPT300x::readManufacturerID() {
  uint16_t result = 0;
  OPT300x_ErrorCode error = writeData(MANUFACTURER_ID);
  if (error == NO_ERROR)
    error = readData(&result);
  return result;
}

uint16_t OPT300x::readDeviceID() {
  uint16_t result = 0;
  OPT300x_ErrorCode error = writeData(DEVICE_ID);
  if (error == NO_ERROR)
    error = readData(&result);
  return result;
}

OPT300x_Config OPT300x::readConfig() {
  OPT300x_Config myConfig;
  OPT300x_ErrorCode error = writeData(CONFIG);
  if (error == NO_ERROR)
    error = readData(&myConfig.rawData);
  config = myConfig;
  return myConfig;
}

OPT300x_ErrorCode OPT300x::writeConfig(OPT300x_Config config) {
  Wire.beginTransmission(_address);
  Wire.write(CONFIG);
  Wire.write(config.rawData >> 8);
  Wire.write(config.rawData & 0x00FF);
  return (OPT300x_ErrorCode)(-10 * Wire.endTransmission());
}

OPT300x_ErrorCode OPT300x::writeHighLimit(float limit){
  OPT300x_ER limit_er = convert_float2er(limit);
  Wire.beginTransmission(_address);
  Wire.write(HIGH_LIMIT);
  Wire.write(limit_er.rawData >> 8);
  Wire.write(limit_er.rawData & 0x00FF);
  return (OPT300x_ErrorCode)(-10 * Wire.endTransmission());
}

OPT300x_ErrorCode OPT300x::writeLowLimit(float limit){
  OPT300x_ER limit_er = convert_float2er(limit);
  Wire.beginTransmission(_address);
  Wire.write(LOW_LIMIT);
  Wire.write(limit_er.rawData >> 8);
  Wire.write(limit_er.rawData & 0x00FF);
  return (OPT300x_ErrorCode)(-10 * Wire.endTransmission());
}

OPT300x_S OPT300x::readResult() { return readRegister(RESULT); }

OPT300x_S OPT300x::readHighLimit() { return readRegister(HIGH_LIMIT); }

OPT300x_S OPT300x::readLowLimit() { return readRegister(LOW_LIMIT); }

OPT300x_S OPT300x::readRegister(OPT300x_Commands command) {
  OPT300x_ErrorCode error = writeData(command);
  if (error == NO_ERROR) {
    OPT300x_S myResult;
    myResult.lux = 0;
    myResult.error = NO_ERROR;

    OPT300x_ER er;
    error = readData(&er.rawData);
    if (error == NO_ERROR) {
      myResult.raw = er;
      myResult.lux = 0.01 * pow(2, er.Exponent) * er.Result;
    } else {
      myResult.error = error;
    }
    switch (command) {
      case RESULT : result = myResult.lux; break;
      case HIGH_LIMIT : highLimit = myResult.lux; break;
      case LOW_LIMIT : lowLimit = myResult.lux; break;
    }
    return myResult;
  } else {
    return returnError(error);
  }
}

OPT300x_ErrorCode OPT300x::writeData(OPT300x_Commands command) {
  Wire.beginTransmission(_address);
  Wire.write(command);
  return (OPT300x_ErrorCode)(-10 * Wire.endTransmission(true));
}

OPT300x_ErrorCode OPT300x::readData(uint16_t *data) {
  uint8_t buf[2];

  Wire.requestFrom(_address, (uint8_t)2);

  int counter = 0;
  while (Wire.available() < 2) {
    counter++;
    delay(10);
    if (counter > 250)
      return TIMEOUT_ERROR;
  }

  Wire.readBytes(buf, 2);
  *data = (buf[0] << 8) | buf[1];

  return NO_ERROR;
}

OPT300x_S OPT300x::returnError(OPT300x_ErrorCode error) {
  OPT300x_S result;
  result.lux = 0;
  result.error = error;
  return result;
}

float OPT300x::convert_er2float(OPT300x_ER input) {
      // Calculate optical power [ref: Equation 1, OPT3002 Datasheet]
      // Optical_Power = R[11:0] * 2^(E[3:0]) * 1.2 nW/cm^2 (OPT3002)
      // Optical_Power = R[11:0] * 2^(E[3:0]) * 0.01 lux (OPT3001)
      uint32_t exponent = 1 << input.Exponent;
      uint32_t output = input.Result * exponent * 0.01;
      return output;
  }

OPT300x_ER OPT300x::convert_float2er(float input) {
    uint8_t exponent = 0;
    uint16_t fractional = input / 0.01;
    while (fractional >= (1 << 12) and exponent < (1 << 4)) {
        fractional /= 2;
        exponent++;
    }
    OPT300x_ER output;
    output.Exponent = exponent;
    output.Result = fractional;

    return output;
}