#include <hardware/pio.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>

#include "joybus.h"

// S_DAT pin
#define JOYBUS_PIN 5

// Holds the EEPROM data
uint8_t mem[2 * 1024];

// Populated by IRQ handler with data incoming from pio program / console S_DAT
volatile uint32_t incoming[4];
volatile uint8_t incoming_length = 0;

// Joybus abstraction
joybus_port_t joybus_rx_port;
joybus_port_t joybus_tx_port;

enum class N64Command {
    PROBE = 0x00,
    RESET = 0xFF,
    READ_EEPROM = 0x04,
    WRITE_EEPROM = 0x05,
};

enum class N64EEPROMType {
    EEPROM_4K = 0x8000,
    EEPROM_16K = 0xC000,
};

typedef struct __attribute__((packed)) {
    uint16_t device;
    uint8_t status;
} n64_status_t;

N64EEPROMType type = N64EEPROMType::EEPROM_16K;
n64_status_t eeprom_status = (n64_status_t){
  .device = (uint16_t) type,
  .status = 0x00,
};

static void pio_rx_irq_func(void) {
  // Read all incoming words from pio RX FIFO
  incoming_length = 0;
  while (!pio_sm_is_rx_fifo_empty(joybus_rx_port.pio, joybus_rx_port.sm)) {
    incoming[incoming_length++] = pio_sm_get_blocking(joybus_rx_port.pio, joybus_rx_port.sm);
  }
  pio_interrupt_clear(joybus_rx_port.pio, 0);
}

static void pio_tx_irq_func(void) {
  // Done transmitting, switch to rx program
  joybus_port_terminate_tx(&joybus_tx_port);
  pio_interrupt_clear(joybus_tx_port.pio, 1);
  joybus_port_init_rx(&joybus_rx_port, JOYBUS_PIN, pio_rx_irq_func);
}

void send_response(uint8_t* data, uint8_t length) {
  // Switch to tx program
  joybus_port_terminate_rx(&joybus_rx_port);
  joybus_port_init_tx(&joybus_tx_port, JOYBUS_PIN, pio_tx_irq_func);
  joybus_send_bytes(&joybus_tx_port, data, length);
}

uint32_t swap32(uint32_t data) {
  return ((data>>24)&0xff) |
          ((data<<8)&0xff0000) |
          ((data>>8)&0xff00) |
          ((data<<24)&0xff000000);
}

int main()
{

  stdio_init_all();

  set_sys_clock_khz(266'000, true);
  
  memset(mem, 0x0f, sizeof(mem));

  joybus_port_init_rx(&joybus_rx_port, JOYBUS_PIN, pio_rx_irq_func);

  while (true) {
      tight_loop_contents();

      if (incoming_length > 0) {
        // Read and handle incoming command
        uint8_t command = incoming[0] & 0xff;
        switch ((N64Command)command) {
          case N64Command::RESET:
          case N64Command::PROBE:
            send_response((uint8_t *)&eeprom_status, sizeof(n64_status_t));
            break;
          case N64Command::READ_EEPROM:
            send_response(&mem[incoming[1]*8], 8);
            break;
          case N64Command::WRITE_EEPROM:
            send_response(0, 1);
            uint8_t block = (incoming[1] >> 24);
            uint8_t* address = (uint8_t*)(mem) + block*8;
            uint32_t data1 = swap32((incoming[1] << 8) | (incoming[2] >> 24));
            uint32_t data2 = swap32((incoming[2] << 8) | (incoming[3] & 0xff));
            memcpy(address, &data1, 4);
            memcpy(address + 4, &data2, 4);
            break;
        }

        incoming_length = 0;
      }
  }
}
