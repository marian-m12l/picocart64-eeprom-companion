#include <hardware/pio.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <pico/stdlib.h>

#include "joybus.h"

#define JOYBUS_PIN 5

uint8_t mem[2 * 1024];
//FIXME uint8_t incoming[10];
volatile uint32_t incoming[4];
volatile uint8_t incoming_length = 0;

joybus_port_t joybus_rx_port;
joybus_port_t joybus_tx_port;


typedef struct __attribute__((packed)) {
    uint16_t device;
    uint8_t status;
} n64_status_t;

enum class N64Command {
    PROBE = 0x00,
    RESET = 0xFF,
    POLL = 0x01,
    READ_EXPANSION_BUS = 0x02,
    WRITE_EXPANSION_BUS = 0x03,
    READ_EEPROM = 0x04,
    WRITE_EEPROM = 0x05,
};

enum class N64EEPROMType {
    EEPROM_4K = 0x8000,
    EEPROM_16K = 0xC000,
};

N64EEPROMType type = N64EEPROMType::EEPROM_16K;
n64_status_t eeprom_status = (n64_status_t){
  .device = (uint16_t) type,
  .status = 0x00,
};
absolute_time_t receive_end;
uint64_t reply_delay = 6;

static void pio_irq_func(void) {
  // TODO Should have a command ready in FIFO
  // TODO Read FIFO into ram array
  // FIXME use rxf array instead of functions ???
  // FIXME read all bytes at once ??
  // TODO blink LED ???
  //printf("INT\n");

  incoming_length = 0;
  //printf("RX FIFO: %d\n", pio_sm_get_rx_fifo_level(joybus_rx_port.pio, joybus_rx_port.sm));
  while (!pio_sm_is_rx_fifo_empty(joybus_rx_port.pio, joybus_rx_port.sm)) {
    // TODO read 8 or 32 bits ???
    //printf("get %d\n", incoming_length);
    incoming[incoming_length++] = pio_sm_get_blocking(joybus_rx_port.pio, joybus_rx_port.sm);
    //printf("got %d\n", incoming_length);
  }
  // Pause
  //printf("PAUSE\n");
  //joybus_rx_pause(&joybus_rx_port);
  // TODO Clear IRQ
  //printf("CLEAR: %d\n", incoming_length);
  pio_interrupt_clear(joybus_rx_port.pio, 0);

  // TODO Process ram array command out of IRQ???
  /*switch ((N64Command)incoming[0]) {
    case N64Command::RESET:
    case N64Command::PROBE:
      // Wait for stop bit before responding.
      busy_wait_us(reply_delay);
      joybus_send_bytes(&joybus_tx_port, (uint8_t *)&eeprom_status, sizeof(n64_status_t));
    case N64Command::READ_EEPROM:
      // Send requested data
      // TODO Handle delay before sending data back !!!
      //eeprom->SendData(&mem[incoming[1]*8], 8);
      receive_end = make_timeout_time_us(reply_delay);
      // FIXME DON'T WAIT IN IRQ HANDLER !!!
      while (!time_reached(receive_end)) {
          tight_loop_contents();
      }
      joybus_send_bytes(&joybus_tx_port, &mem[incoming[1]*8], 8);
      break;
    case N64Command::WRITE_EEPROM:
      // Store incoming data
      // TODO pointer to a 0 byte instead of 0 value !!
      // TODO Handle delay before sending data back !!!
      //eeprom->SendData(0, 1); // Acknowledge write operation
      receive_end = make_timeout_time_us(reply_delay);
      // FIXME DON'T WAIT IN IRQ HANDLER !!!
      while (!time_reached(receive_end)) {
          tight_loop_contents();
      }
      joybus_send_bytes(&joybus_tx_port, 0, 1);
      memcpy(&mem[incoming[0]*8], &incoming[2], 8);
      // TODO store into flash
      break;
  }*/
}

int main()
{

  stdio_init_all();

  // FIXME support 266 and 133 MHz ???
  set_sys_clock_khz(130'000, true);

  sleep_us(2000000);
  printf("START\n");

  //N64EEPROM* eeprom = new N64EEPROM(N64EEPROMType::EEPROM_16K, JOYBUS_PIN, pio0);
  //joybus_port_init(&joybus_rx_port, &joybus_tx_port, JOYBUS_PIN);
  printf("INIT RX\n");
  joybus_port_init_rx(&joybus_rx_port, JOYBUS_PIN, pio_irq_func);
  
  printf("START 2\n");
  
  // Enable interrupt
  //irq_add_shared_handler(PIO0_IRQ_0, pio_irq_func, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY); // Add a shared IRQ handler
  //irq_set_enabled(PIO0_IRQ_0, true); // Enable the IRQ
  //joybus_register_rx_handler(&joybus_rx_port, pio_irq_func);
  
  memset(mem, 0x0f, sizeof(mem));


  printf("GO\n");
  int rounds = 0;
  while (true) {
      /*n64_eeprom_operation_t operation = eeprom->WaitForCommand();
      
      switch (operation.command) {
          case N64Command::READ_EEPROM:
            // Send requested data
            eeprom->SendData(&mem[operation.page*8], 8);
            break;
          case N64Command::WRITE_EEPROM:
            // Store incoming data
            // TODO pointer to a 0 byte instead of 0 value !!
            eeprom->SendData(0, 1); // Acknowledge write operation
            memcpy(&mem[operation.page*8], operation.data, 8);
            // TODO store into flash
            break;
      }*/
      tight_loop_contents();

      // FIXME NEEDED ???
      //busy_wait_us(1);
      //busy_wait_us(100);

      // TODO Print heartbeat
      /*if (rounds == 0) {
        printf("heartbeat...\n");
      }
      rounds = (rounds + 1) % 20000;*/

      //printf("incoming_length ? %d\n", incoming_length);
      if (incoming_length > 0) {
        
        //printf("LEN: %d\n", incoming_length);
        /*for (int i=0; i<incoming_length; i++) {
          printf("  incoming[%d]: 0x%08x\n", i, incoming[i]);
        }*/

        // FIXME Remove workaround
        uint8_t command = incoming[0] & 0xff;
        /*if (((incoming[0] & 0xff00) >> 8) == 0x04) {
          // TODO Read command
          command = ((incoming[0] & 0xff00) >> 8);
        }*/

        /*if (command != 0 && command != 4) {
          printf("LEN: %d\n", incoming_length);
          printf("CMD: 0x%02x\n", command);
          printf("incoming_length: %d\n", incoming_length);
          for (int i=0; i<incoming_length; i++) {
            printf("  incoming[%d]: 0x%08x\n", i, incoming[i]);
          }
        }*/

        switch ((N64Command)command) {
          case N64Command::RESET:
          case N64Command::PROBE:
            // Wait for stop bit before responding.
            //printf("PROBE\n");
            busy_wait_us(reply_delay);
            // FIXME single port + switch program !!
            //printf("TERMINATE RX\n");
            joybus_port_terminate_rx(&joybus_rx_port);
            //printf("INIT TX\n");
            joybus_port_init_tx(&joybus_tx_port, JOYBUS_PIN);
            //printf("SEND\n");
            joybus_send_bytes(/*&joybus_rx_port,*/ &joybus_tx_port, (uint8_t *)&eeprom_status, sizeof(n64_status_t));
            //printf("TERMINATE TX\n");
            // TODO Need to wait for data to be sent!!! --> interrupt?? fifo empty???
            //while (!pio_sm_is_tx_fifo_empty(joybus_tx_port.pio, joybus_tx_port.sm)) {
            while (!pio_interrupt_get(joybus_tx_port.pio, 1)) {
              // FIXME busy_wait_us(1000);
              //printf("SENDING... %d\n", pio_sm_get_tx_fifo_level(joybus_tx_port.pio, joybus_tx_port.sm));
              tight_loop_contents();
            }
            joybus_port_terminate_tx(&joybus_tx_port);
            pio_interrupt_clear(joybus_tx_port.pio, 1);
            //printf("INIT RX\n");
            joybus_port_init_rx(&joybus_rx_port, JOYBUS_PIN, pio_irq_func);
            //printf("PROBE END\n");
            // Do we need to reset pio ???
            /*printf("PIO TERMINATE\n");
            joybus_port_terminate(&joybus_rx_port, &joybus_tx_port);
            printf("PIO INIT\n");
            joybus_port_init(&joybus_rx_port, &joybus_tx_port, JOYBUS_PIN);
            joybus_register_rx_handler(&joybus_rx_port, pio_irq_func);
            printf("PIO HAS BEEN RESET\n");*/
            break;
          case N64Command::READ_EEPROM:
            //printf("READ\n");
            // Send requested data
            // TODO Handle delay before sending data back !!!
            //eeprom->SendData(&mem[incoming[1]*8], 8);
            /*receive_end = make_timeout_time_us(reply_delay);
            // FIXME DON'T WAIT IN IRQ HANDLER !!!
            while (!time_reached(receive_end)) {
                tight_loop_contents();
            }*/
            busy_wait_us(reply_delay);
            // FIXME single port + switch program !!
            joybus_port_terminate_rx(&joybus_rx_port);
            joybus_port_init_tx(&joybus_tx_port, JOYBUS_PIN);
            joybus_send_bytes(/*&joybus_rx_port,*/ &joybus_tx_port, &mem[incoming[1]*8], 8);
            while (!pio_interrupt_get(joybus_tx_port.pio, 1)) {
              // FIXME busy_wait_us(1000);
              //printf("SENDING... %d\n", pio_sm_get_tx_fifo_level(joybus_tx_port.pio, joybus_tx_port.sm));
              tight_loop_contents();
            }
            joybus_port_terminate_tx(&joybus_tx_port);
            pio_interrupt_clear(joybus_tx_port.pio, 1);
            joybus_port_init_rx(&joybus_rx_port, JOYBUS_PIN, pio_irq_func);
            //printf("READ END: 0x%08x\n", incoming[1]);
            if (incoming[1] < 2) {
              printf("R%d: (0x%08x) 0x%08x 0x%08x\n", incoming[1], &mem[incoming[1]*8], *((uint32_t*)(&mem[incoming[1]*8])), *((uint32_t*)((&mem[incoming[1]*8])+4)));
            }
            break;
          case N64Command::WRITE_EEPROM:
            //printf("WRITE\n");
            // Store incoming data
            // TODO pointer to a 0 byte instead of 0 value !!
            // TODO Handle delay before sending data back !!!
            //eeprom->SendData(0, 1); // Acknowledge write operation
            /*receive_end = make_timeout_time_us(reply_delay);
            // FIXME DON'T WAIT IN IRQ HANDLER !!!
            while (!time_reached(receive_end)) {
                tight_loop_contents();
            }*/
            busy_wait_us(reply_delay);
            joybus_port_terminate_rx(&joybus_rx_port);
            joybus_port_init_tx(&joybus_tx_port, JOYBUS_PIN);
            joybus_send_bytes(/*&joybus_rx_port,*/ &joybus_tx_port, 0, 1);
            while (!pio_interrupt_get(joybus_tx_port.pio, 1)) {
              // FIXME busy_wait_us(1000);
              //printf("SENDING... %d\n", pio_sm_get_tx_fifo_level(joybus_tx_port.pio, joybus_tx_port.sm));
              tight_loop_contents();
            }
            joybus_port_terminate_tx(&joybus_tx_port);
            pio_interrupt_clear(joybus_tx_port.pio, 1);
            joybus_port_init_rx(&joybus_rx_port, JOYBUS_PIN, pio_irq_func);

            /*
LEN: 4
CMD: 0x05
incoming_length: 4
  incoming[0]: 0x00000005
  incoming[1]: 0x00656570
  incoming[2]: 0x0604e43b
  incoming[3]: 0x000000e8
SENDING... 0
*/
            
            /*printf("WRITE: incoming_length: %d\n", incoming_length);
            for (int i=0; i<incoming_length; i++) {
              printf("  incoming[%d]: 0x%08x\n", i, incoming[i]);
            }*/
            //printf("WRITING data 0x%08x into chunk 0x%08x at address 0x%08x (mem base address 0x%08x)\n", incoming[2], incoming[1], mem[incoming[1]*8], mem);
            // FIXME memcpy(&mem[incoming[1]*8], &incoming[2], 8); // FIXME data in incoming 2 and 3 !!!
            // TODO store into flash
            uint8_t block = (incoming[1] >> 24);
            uint8_t* address = (uint8_t*)(mem) + block*8;// FIXME mem + block*8;
            uint32_t data1 = (incoming[1] << 8) | (incoming[2] >> 24);
            data1 = ((data1>>24)&0xff) |
                    ((data1<<8)&0xff0000) |
                    ((data1>>8)&0xff00) |
                    ((data1<<24)&0xff000000);
            uint32_t data2 = (incoming[2] << 8) | (incoming[3] & 0xff);
            data2 = ((data2>>24)&0xff) |
                    ((data2<<8)&0xff0000) |
                    ((data2>>8)&0xff00) |
                    ((data2<<24)&0xff000000);
            //printf("WRITE END: 0x%08x / 0x%08x <-- 0x%08x 0x%08x\n", block, address, data1, data2);
            memcpy(address, &data1, 4);
            memcpy(address + 4, &data2, 4);
            //printf("WRITE END: 0x%08x = 0x%08x 0x%08x\n", address, *((uint32_t*)address), *((uint32_t*)(address+4)));
            if (block < 2) {
              printf("W%d: (0x%08x) 0x%08x 0x%08x\n", block, address, *((uint32_t*)address), *((uint32_t*)(address+4)));
            }
            break;
        }
        //printf("DONE (%d / 0x%08x / 0x%02x)\n", incoming_length, incoming[0], command);
        /*printf("0x%02x (%d)\n", command, incoming_length);
        for (int i=0; i<incoming_length; i++) {
          printf("  incoming[%d]: 0x%08x\n", i, incoming[i]);
        }*/
        
        /*if (command != 0 && command != 4) {
          printf("incoming_length: %d\n", incoming_length);
          for (int i=0; i<incoming_length; i++) {
            printf("  incoming[%d]: 0x%08x\n", i, incoming[i]);
          }
        }*/

        incoming_length = 0;
      }
  }
}
