#include <SoftwareSerial.h>
#include <SerialCommand.h>
#include "SPI.h"
#include "WS2801.h"
#include <EEPROM.h>
#include <string.h>

#define LED_NB 4      // Number of LEDs

#define DATA_PIN 2
#define CLCK_PIN 3

#define CONFIG_VERSION "CI01"
#define CONFIG_START 32

#define NUMITEMS(arg) ((unsigned int) (sizeof (arg) / sizeof (arg [0])))

int i;

WS2801 strip = WS2801(LED_NB, DATA_PIN, CLCK_PIN);

SerialCommand SCmd;
char *arg;
int index;
bool changed = false;

enum LED_STATE_ENUM {
    OFF,
    OK,
    ERROR,
    WARNING,
    UNKNOWN
};

static const String LED_STATE_STRING[] = {
    "OFF",
    "OK",
    "ERROR",
    "WARNING",
    "UNKNOWN"
};

/* Helper functions */

// Create a 24 bit color value from R,G,B
uint32_t Color(byte r, byte g, byte b)
{
  uint32_t c;
  c = r;
  c <<= 8;
  c |= b;
  c <<= 8;
  c |= g;
  return c;
}
uint32_t Color(byte color[3])
{
  uint32_t c;
  c = color[0];
  c <<= 8;
  c |= color[2];
  c <<= 8;
  c |= color[1];
  return c;
}

int array_index(const String value, const String *array)
{
  int size = NUMITEMS((*array))-2;

  for (int i = 0; i < size; i++)
  {
    Serial.print("i: ");
    Serial.print(i);
    Serial.print(" - array[i]: ");
    Serial.println(array[i]);
    if (value == array[i])
    {
      return i;
    }
  }

  return -1;
}

LED_STATE_ENUM states[LED_NB];


// CONFIGURATION
struct ConfigStruct {
  char version[5];
  uint32_t colors[5];
} storage = {
  CONFIG_VERSION,
  // The default values
  {
    Color(0, 0, 0),
    Color(0, 255, 0),
    Color(255, 0, 0),
    Color(200, 100, 0),
    Color(0, 0, 255)
  }
};

void loadConfig() {
  // To make sure there are settings, and they are YOURS!
  // If nothing is found it will use the default settings.
  if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
      EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
      EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2] &&
      EEPROM.read(CONFIG_START + 3) == CONFIG_VERSION[3])
    for (unsigned int t=0; t<sizeof(storage); t++)
      *((char*)&storage + t) = EEPROM.read(CONFIG_START + t);
}

void saveConfig() {
  for (unsigned int t=0; t<sizeof(storage); t++)
    EEPROM.write(CONFIG_START + t, *((char*)&storage + t));
}

// STATUSES
void clear_states() {
    for(unsigned i = 0; i < LED_NB; i++) {
        states[i] = UNKNOWN;
    }
    changed = true;
}

void set_n_state(LED_STATE_ENUM state) {
  arg = SCmd.next();
  if (arg != NULL) {
    index = atoi(arg);

    if (index <= 0 || index > LED_NB){
      Serial.println("ERR INVALID INDEX");
      return;
    } else {
      states[index-1] = state;
    }

  }
  changed = true;

  Serial.println("ACK");
}

void set_n_off() {
  set_n_state(OFF);
}

void set_n_ok() {
  set_n_state(OK);
}

void set_n_error() {
  set_n_state(ERROR);
}

void set_n_warning() {
  set_n_state(WARNING);
}

void set_n_unknown() {
  set_n_state(UNKNOWN);
}

void set_color() {
  arg = SCmd.next();
  if (arg != NULL) {
    int index;
    if((index = array_index(arg, LED_STATE_STRING)) == -1) {
      Serial.println("COLOR: ERR UNKNOWN STATE");
      return;
    }
    Serial.print("Index: ");
    Serial.println(index);

    byte color_rgb[3] = {0, 0, 0};

    for(unsigned i = 0; i < 3; i++) {
      arg = SCmd.next();

      if (arg == NULL){
        Serial.println("COLOR: WRONG NUMBER OF ARGUMENTS");
        return;
      } else {
        int color_code = atoi(arg);
        if (color_code >= 0 && color_code < 256) {
          color_rgb[i] = (byte)color_code;
        } else {
          Serial.println("COLOR: COLOR_CODE OUT OF RANGE [0-255]");
          return;
        }
      }
    }

    storage.colors[index] = Color(color_rgb);
    saveConfig();

    changed = true;
    Serial.println("ACK");

  }
}

void turn_leds() {
    for (unsigned i = 1; i <= LED_NB; i++) {
        Serial.print(i);
        Serial.print(": ");
        Serial.println(LED_STATE_STRING[states[i-1]]);
        strip.setPixelColor(i-1, storage.colors[states[LED_NB-i]]);
    }
}

void init_leds() {
    clear_states();

    for(unsigned i = 0; i < LED_NB; i++) {
        arg = SCmd.next();

        if (arg == NULL)        break;
        else if (strcmp(arg, LED_STATE_STRING[OFF].c_str()) == 0) {
            states[i] = OFF;
        } else if (strcmp(arg, LED_STATE_STRING[OK].c_str()) == 0) {
            states[i] = OK;
        } else if (strcmp(arg, LED_STATE_STRING[ERROR].c_str()) == 0) {
            states[i] = ERROR;
        } else if (strcmp(arg, LED_STATE_STRING[WARNING].c_str()) == 0) {
            states[i] = WARNING;
        } else {
            states[i] = UNKNOWN;
        }
    }
    Serial.println("ACK");
}

void status() {
  Serial.print("STATUS ");
  for (unsigned i = 1; i <= LED_NB; i++) {
    Serial.print(LED_STATE_STRING[states[LED_NB-i]]);
    Serial.print(" ");
  }
  Serial.println();
}

void unrecognized()
{
  Serial.println("ERR UNKNOWN COMMAND");
}


// PROGRAM
void setup()
{
  // Open serial communications and wait for port to open:
  Serial.begin(9600);

  loadConfig();

  clear_states();

  // Setup callbacks for SerialCommand commands
  SCmd.addCommand("OFF",set_n_off);         // Set LED N state to OFF
  SCmd.addCommand("OK",set_n_ok);           // Set LED N state to OK
  SCmd.addCommand("ERROR",set_n_error);     // Set LED N state to ERROR
  SCmd.addCommand("WARNING",set_n_warning); // Set LED N state to WARNING
  SCmd.addCommand("UNKNOWN",set_n_unknown); // Set LED N state to WARNING
  SCmd.addCommand("INIT",init_leds);        // Init LEDs
  SCmd.addCommand("COLOR",set_color);       // Set state color
  SCmd.addCommand("STATUS",status);         // Status of LED States
  SCmd.addDefaultHandler(unrecognized);     // Handler for command that isn't matched  (says "What?")
  Serial.println("Ready");

  strip.begin();
  strip.show();
}

void loop() // run over and over
{
    SCmd.readSerial();

    if (changed) {
      turn_leds();  // Change leds status
      strip.show(); // Display new leds colors
      changed = false;
    }
    delay(1000);    // sleep for 1 second
}


