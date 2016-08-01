/*******************************************************************************************************************
   La Skala Attenuator: A ladder-type volume attenuator with Arduino control.

   Rewriten version of original project by dimdim.gr:
   Original Project Page: http://www.dimdim.gr/diyaudio/la-skala-attenuator/

   Fork mainentained by vojcio   : https://github.com/vojcio/laskala

   v1.00    01/08/2016 : - fork by vojcio, revision controll moved to GitHub

   v0.52    30/05/2016 : - Bug fixes. Everything seems to be working OK.
                         - First public release.
                         - Commented out TFT code to make code fit in an UNO.

   v0.44    11/05/2016 : - Added code to take into account the number of relays that switch states, so as to
                           reduce power supply loading due to many relays activating at the same time.
                         - Attempt to eliminate small glitches occuring at certain steps by implementing different
                           function for increasing & decreasing attenuation.

   v0.30    15/02/2016 : - First attempt at coding an appropriate algorithm. It works!
                         - Outputs status info to serial port and TFT (ILI9341 with SPI).
 *******************************************************************************************************************/

//#include <UTFT.h>                                    // Library for the TFT display.
#include <Wire.h>                                     // Library for the I2C communication.
#include "Adafruit_MCP23008.h"                        // Library for the I/O expander.
#include <RotaryEncoder.h>                            // Library for the encoder.

float volume = 10;                                    // Default attenuation.
float volume_old = 1000;                              // Store old volume.
int energ_relays;                                     // Store the number of energized relays.
int energ_relays_old;                                 // Store the number of previously energized relays.
int relay_delay;

extern uint8_t BigFont[];

Adafruit_MCP23008 mcp;

#define mcp0 64                                       // First relay will attenuate by 64db.
#define mcp1 32                                       // Second relay will attenuate by 32db.
#define mcp2 16                                       // Third relay will attenuate by 16db.
#define mcp3 8                                        // Fourth relay will attenuate by 8db.
#define mcp4 4                                        // Fifth relay will attenuate by 4db.
#define mcp5 2                                        // Sixth relay will attenuate by 2db.
#define mcp6 1                                        // Seventh relay will attenuate by 1db.
#define mcp7 0.5                                      // Eighth relay will attenuate by 0.5db.

boolean relay[8] = {0, 0, 0, 0, 0, 0, 0, 0};

//memset(relay, 0, sizeof(relay));

int VOLUPPIN = 'A3';           // RotEnc A terminal for right rotary encoder.
int VOLDOWNPIN = 'A2';         // RotEnc B terminal for right rotary encoder.

RotaryEncoder encoder(VOLDOWNPIN, VOLUPPIN);       // Setup the first Rotary Encoder

void setup() {

  mcp.begin();                                // use default address 0
  mcp.pinMode(0, OUTPUT);
  mcp.pinMode(1, OUTPUT);
  mcp.pinMode(2, OUTPUT);
  mcp.pinMode(3, OUTPUT);
  mcp.pinMode(4, OUTPUT);
  mcp.pinMode(5, OUTPUT);
  mcp.pinMode(6, OUTPUT);
  mcp.pinMode(7, OUTPUT);

  setVol(volume);                 // Set the attenuation at its default value. Must be as close to the beginning of the code as possible to avoid audio spikes at start-up.

  pinMode(VOLUPPIN, INPUT);       // Button switch or Encoder pin for volume up
  digitalWrite(VOLUPPIN, HIGH);   // If H/W debouncing is implemented, set to LOW
  pinMode(VOLDOWNPIN, INPUT);     // Button switch or Encoder pin for volume down
  digitalWrite(VOLDOWNPIN, HIGH); // If H/W debouncing is implemented, set to LOW

  Serial.begin(115200);
}

void loop() {

    setVol(calcChange());
  }


int calcChange() {
  static int pos = 0;                        // Read the rotary encoder and increase or decrease attenuation.
  encoder.tick();
  int newPos = encoder.getPosition();
  if (pos != newPos)
  {
    if (pos < newPos)
    {
      if (volume > 0)
      {
        volume = volume - 0.5;
      }
      pos = newPos;
    }
    else if (pos > newPos)
    {
      if (volume < 127)
      {
        volume = volume + 0.5;
      }
      pos = newPos;
    }
  }
}

void setVol(float volume_temp)              // Set the volume by controlling the 8 relays.
{
  if (volume_temp != volume_old) {
    Serial.print("Setting volume to: ");
    Serial.println(volume_temp);
    energ_relays = 0;
    float vol_temp_2 = volume_temp;

    for (int i = 0 ; i < 8 ; i++)
    {
      if (volume_temp >= mcp0)
      {
        relay[i] = 1;
        energ_relays++;
        volume_temp = volume_temp - mcp0;
        Serial.println(volume_temp);
      } else
      {
        relay[i] = 0;
      }
    }

    Serial.print("Difference in switched relays: ");                  // Determine how many relays will be switching states. Useful to predict the current load imposed on the power supply.
    Serial.println(abs(energ_relays_old - energ_relays));

    if (abs(energ_relays_old - energ_relays) <= 3)
    {
      relay_delay = 0;
    }
    if (abs(energ_relays_old - energ_relays) == 4)                  // When a large number of relays is expected to switch states, introduce a delay between activations to ease the burden of the power supply (and decrease switching noise).
    {
      relay_delay = 5;
    }
    if (abs(energ_relays_old - energ_relays) == 5)
    {
      relay_delay = 10;
    }
    if (abs(energ_relays_old - energ_relays) == 6)
    {
      relay_delay = 20;
    }
    if (abs(energ_relays_old - energ_relays) >= 7)
    {
      relay_delay = 50;
    }

    energ_relays_old = energ_relays;

    if (vol_temp_2 > volume_old)                                    // If we are increasing the attenuation
    {
      for (int i ; i < 8 ; i++)
      {
        Serial.println("Increasing the attenuation");
        if (relay[i] == 0)
        {
          mcp.digitalWrite(i, LOW);
        }
        else mcp.digitalWrite(i, HIGH);
      }
      volume_old = vol_temp_2;
    }

    if (vol_temp_2 < volume_old)                                  // If we are decreasing the attenuation
    {  
      for (int i ; i > 8 ; i--)
      {
        Serial.println("Decreasing the attenuation");
        if (relay[i] == 0)
        {
          mcp.digitalWrite(i, LOW);
        }
        else mcp.digitalWrite(i, HIGH);

        delay(relay_delay);
        volume_old = vol_temp_2;
      }
    }
  }
}


