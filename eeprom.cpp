#include <hardware/pio.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>

#include "N64EEPROM.hpp"
#include "n64_definitions.h"


#define JOYBUS_PIN 5

uint8_t mem[2 * 1024];


int main()
{
  set_sys_clock_khz(130'000, true);

  N64EEPROM* eeprom = new N64EEPROM(N64EEPROMType::EEPROM_16K, JOYBUS_PIN, pio0);
  
  memset(mem, 0x0f, sizeof(mem));

  while (true) {
      n64_eeprom_operation_t operation = eeprom->WaitForCommand();
      
      switch (operation.command) {
          case N64Command::READ_EEPROM:
            // Send requested data
            eeprom->SendData(&mem[operation.page*8], 8);
            break;
          case N64Command::WRITE_EEPROM:
            // Store incoming data
            eeprom->SendData(0, 1); // Acknowledge write operation
            memcpy(&mem[operation.page*8], operation.data, 8);
            // TODO store into flash
            break;
      }
  }
}
