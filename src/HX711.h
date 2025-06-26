#pragma once

class HX711
{
public:
  HX711(int dataPin, int clockPin);
  void scaleInitialise();
  float readWeight(int times = 10);

private:
  int m_dataPin, m_clockPin;
  float m_scale = 1.0;
  long m_offset = 0;

  void gpioSetup();
  void tare(int times);
  long readRaw();
  void setScale(float scale);
  void setOffset(long offset);
};