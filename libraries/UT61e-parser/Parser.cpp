#include <Arduino.h>

#include "Parser.h"
#include "math.h"

double I = NAN;
double R = NAN;
double V = NAN;
double C = NAN;
double F = NAN;

struct MeterRange {
  String description;
  String unit;
  double multiplier;
};

const MeterRange RANGE_V[] = {
    {"2.2000", "V", 0.0001},
    {"22.000", "V", 0.001},
    {"220.00", "V", 0.01},
    {"1000.0", "V", 0.1},
    {"220.00", "mV", 0.01},
};

const MeterRange RANGE_R[] = {
    {"220.00", "Ohm", 0.01},
    {"2.2000", "kOhm", 0.0001},
    {"22.000", "kOhm", 0.001},
    {"220.00", "kOhm", 0.01},
    {"2.2000", "MOhm", 0.0001},
    {"22.000", "MOhm", 0.001},
    {"220.00", "MOhm", 0.01},
};

const MeterRange RANGE_C[] = {
    {"22.000", "nF", 0.001},
    {"220.00", "nF", 0.01},
    {"2.2000", "uF", 0.0001},
    {"22.000", "uF", 0.001},
    {"220.00", "uF", 0.01},
    {"2.2000", "mF", 0.0001},
    {"22.000", "mF", 0.001},
    {"220.00", "mF", 0.01},
};

const MeterRange RANGE_F[] = {
    {"220.00", "Hz", 0.01},
    {"2200.0", "Hz", 0.1},
    {"None", "Hz", 1},
    {"22.000", "kHz", 0.001},
    {"220.00", "kHz", 0.01},
    {"2.2000", "MHz", 0.0001},
    {"22.000", "MHz", 0.001},
    {"220.00", "MHz", 0.01},
};

const MeterRange RANGE_I_UA[] = {
    {"220.00", "uA", 0.01},
    {"2200.0", "uA", 0.1},
};

const MeterRange RANGE_I_MA[] = {
    {"22.000", "mA", 0.001},
    {"220.00", "mA", 0.01},
};

const MeterRange RANGE_I_A[] = {
    {"10.000", "A", 0.001},
};

const MeterRange RANGE_PERCENT[] = {
    {"100.0", "%", 0.01},
    {"100.0", "%", 0.01},
    {"100.0", "%", 0.01},
    {"100.0", "%", 0.01},
    {"100.0", "%", 0.01},
    {"100.0", "%", 0.01},
    {"100.0", "%", 0.01},
};

struct MeasurementType {
  String measurement;
  const MeterRange *range;
} ranges[] = {
    {"A", RANGE_I_A},
    {"Diode", RANGE_V},
    {"Hz/%", RANGE_F},
    {"Ohm", RANGE_R},
    {"deg", nullptr},
    {"Buzzer", RANGE_R},
    {"Cap", RANGE_C},
    {"", nullptr},
    {"", nullptr},
    {"A", RANGE_I_A},
    {"", nullptr},
    {"V/mV", RANGE_V},
    {"", nullptr},
    {"uA", RANGE_I_UA},
    {"ADP", nullptr},
    {"mA", RANGE_I_MA},
};

struct FlagBit {
  uint8_t byteOffset;
  uint8_t pattern;
  String flag;
} flags[] = {
  {7, 0b00001000, "%"},
  {7, 0b00000100, "-"},
  {7, 0b00000010, "BAT"},
  {7, 0b00000001, "OVER"},
  {8, 0b00000010, "REL"},
  {9, 0b00001000, "UNDER"},
  {9, 0b00000100, "MAX"},
  {9, 0b00000010, "MIN"},
  {10, 0b00001000, "DC"},
  {10, 0b00000100, "AC"},
  {10, 0b00000010, "AUTO"},
  {10, 0b00000001, "Hz"},
  {11, 0b00000010, "Hold"},
};

double Parser::getVoltage(){
  return double(V);
}

double Parser::getCurrent(){
  return double(I);
}

double Parser::getFrequency(){
  return double(F);
}

double Parser::getResistance(){
  return double(R);
}

String Parser::parse(String reading) {
  
  I = NAN;
  R = NAN;
  V = NAN;
  C = NAN;
  F = NAN;

  uint8_t rangeIndex = reading[6] & 0xf;
  uint8_t subrangeIndex = reading[0] & 0x7;
  const MeterRange *range = ranges[rangeIndex].range;
  const MeterRange *subrange = nullptr;
  String rangeName = ranges[rangeIndex].measurement;
  double multiplier = 1;
  if (range) {
    subrange = &range[subrangeIndex];
    rangeName = subrange->unit;
    multiplier = subrange->multiplier;
  }
  double rawValue = reading.substring(1,6).toDouble() * multiplier;
  
  String flagStr;
  for (auto flag: flags) {
    if (reading[flag.byteOffset] & flag.pattern) {
      if (flag.flag == "-") {
        rawValue = -rawValue;
      } else {
        if (flagStr.length() > 0) {
          flagStr += ",";
        }
        flagStr += flag.flag;
      }
    }
  }

  String readable;
  readable += String(rawValue, 4);
  String value = readable;
  readable += " ";
  readable += rangeName;

  // Voltage selected
  rawValue;
  if(rangeName == "V"){V = rawValue;}
  else if(rangeName == "mV"){V = rawValue/1000;}
  // Current selected
  else if(rangeName == "A"){I = rawValue;}
  else if(rangeName == "mA"){I = rawValue/1000;}
  else if(rangeName == "uA"){I = rawValue/1000000;}
  // Resistance selected
  else if(rangeName == "Ohm"){R = rawValue;}
  else if(rangeName == "kOhm"){R = rawValue*1000;}
  else if(rangeName == "MOhm"){R = rawValue*1000000;}
  // Capacitance selected
//  else if(rangeName == "nF"){R = rawValue;}
//  else if(rangeName == "uF"){I = String(rawValue.toDouble()/1000);}
//  else if(rangeName == "mF"){I = String(rawValue.toDouble()/1000000);}
  // Frequency selected
  else if(rangeName == "Hz"){F = rawValue;}
  else if(rangeName == "kHz"){F = rawValue*1000;}
  else if(rangeName == "MHz"){F = rawValue*1000000;}

  if (flagStr.length() > 0) {
    readable += " ";
    readable += flagStr;
  }
  return readable;
}
