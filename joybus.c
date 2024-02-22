#include "joybus.h"

#include "joybus_rx.pio.h"
#include "joybus_tx.pio.h"

#include <hardware/pio.h>
#include <pico/stdlib.h>

void joybus_port_init(joybus_port_t *port_rx, joybus_port_t *port_tx, uint pin) {
    // Init RX and TX

    // TODO Can we load the rx/tx program as needed in a single pio instance ???

    int sm = pio_claim_unused_sm(pio0, true);
    int offset = pio_add_program(pio0, &joybus_rx_program);

    port_rx->pin = pin;
    port_rx->pio = pio0;
    port_rx->sm = sm;
    port_rx->offset = offset;
    port_rx->config = joybus_rx_program_get_config(pio0, sm, offset, pin);

    sm = pio_claim_unused_sm(pio1, true);
    offset = pio_add_program(pio1, &joybus_tx_program);

    port_tx->pin = pin;
    port_tx->pio = pio1;
    port_tx->sm = sm;
    port_tx->offset = offset;
    port_tx->config = joybus_tx_program_get_config(pio1, sm, offset, pin);

    joybus_port_reset(port_rx, port_tx);
}

void joybus_port_terminate(joybus_port_t *port_rx, joybus_port_t *port_tx) {
    pio_sm_set_enabled(port_rx->pio, port_rx->sm, false);
    pio_sm_unclaim(port_rx->pio, port_rx->sm);
    pio_remove_program(port_rx->pio, &joybus_rx_program, port_rx->offset);
    pio_sm_set_enabled(port_tx->pio, port_tx->sm, false);
    pio_sm_unclaim(port_tx->pio, port_tx->sm);
    pio_remove_program(port_tx->pio, &joybus_tx_program, port_tx->offset);
}

void joybus_port_reset(joybus_port_t *port_rx, joybus_port_t *port_tx) {
    joybus_rx_program_receive_init(port_rx->pio, port_rx->sm, port_rx->offset, port_rx->pin, &port_rx->config);
    joybus_tx_program_pause(port_tx->pio, port_tx->sm);
}

void joybus_rx_pause(joybus_port_t *port_rx) {
    joybus_rx_program_pause(port_rx->pio, port_rx->sm);
}

void joybus_register_rx_handler(joybus_port_t *port_rx, irq_handler_t handler) {
    // FIXME IRQ depends on port_rx->pio
    irq_add_shared_handler(PIO0_IRQ_0, handler, PICO_SHARED_IRQ_HANDLER_DEFAULT_ORDER_PRIORITY);
    pio_set_irq0_source_enabled(port_rx->pio, PIO_INTR_SM0_LSB, true);    // FIXME needed ??
    irq_set_enabled(PIO0_IRQ_0, true);
}

void __no_inline_not_in_flash_func(joybus_send_bytes)(
    joybus_port_t *port_rx,
    joybus_port_t *port_tx,
    uint8_t *bytes,
    uint len
) {

    // TODO Need to stop RX while sending data...
    joybus_rx_program_pause(port_rx->pio, port_rx->sm);


    // Wait for line to be high before sending anything.
    while (!gpio_get(port_tx->pin)) {
        tight_loop_contents();
    }

    joybus_tx_program_send_init(port_tx->pio, port_tx->sm, port_tx->offset, port_tx->pin, &port_tx->config);

    for (int i = 0; i < len; i++) {
        joybus_send_byte(port_tx, bytes[i], i == len - 1);
    }

    //joybus_rx_program_resume(port_rx->pio, port_rx->sm);
    //joybus_rx_program_receive_init(port_rx->pio, port_rx->sm, port_rx->offset, port_rx->pin, &port_rx->config);
    joybus_port_reset(port_rx, port_tx);
}

void __no_inline_not_in_flash_func(joybus_send_byte)(joybus_port_t *port_tx, uint8_t byte, bool stop) {
    uint32_t data_shifted = (byte << 24) | (stop << 23);
    pio_sm_put_blocking(port_tx->pio, port_tx->sm, data_shifted);
}
