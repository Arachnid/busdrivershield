#define F_CPU 1000000UL  // 1 MHz

#include <util/delay.h>
#include <avr/io.h>
#include <avr/eeprom.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include "register.h"
#include "usiTwiSlave.h"

#define TRUE 1
#define FALSE 0

#define copy_bit(from_reg, from_bit, to_reg, to_bit) if((from_reg) & _BV(from_bit)) { \
    to_reg |= _BV(to_bit); \
} else { \
    to_reg &= ~_BV(to_bit); \
}

#define DDR_ENABLE DDRB
#define DD_ENABLE1 DDB4
#define DD_ENABLE2 DDB3
#define ENABLE_PWM_SETUP() TCCR1A = _BV(WGM10), TCCR1B = _BV(WGM12) | _BV(CS10)
#define TCCR_ENABLE TCCR1A
#define COM_ENABLE1 _BV(COM1B1)
#define COM_ENABLE2 _BV(COM1A1)
#define OCR_ENABLE1 OCR1B
#define OCR_ENABLE2 OCR1A

#define PORT_INPUT PORTB
#define PIN_INPUT PINB
#define P_INPUT_M1_I1 PB6
#define P_INPUT_M1_I2 PB2
#define P_INPUT_M2_I1 PB0
#define P_INPUT_M2_I2 PB1
#define PCINT_MASK _BV(PCINT0) | _BV(PCINT1) | _BV(PCINT2) | _BV(PCINT6)

#define PORT_DIR PORTD
#define P_DIR_M1_CCW PD3
#define P_DIR_M1_CW PD2
#define P_DIR_M2_CCW PD5
#define P_DIR_M2_CW PD4
#define DDR_DIR DDRD
#define DD_DIR1 DDD3
#define DD_DIR2 DDD2
#define DD_DIR3 DDD5
#define DD_DIR4 DDD4

#define PORT_INT PORTD
#define DDR_INT DDRD
#define DD_INT0 DDD6
#define P_INT0 PD6
#define DD_INT1 DDD1
#define P_INT1 PD1

typedef void (*register_write_handler)(uint8_t reg, uint8_t *value);

// Predefinitions of read/write handlers so we can reference them
uint8_t i2c_read(uint8_t reg);
void i2c_write(uint8_t reg, uint8_t value);
void main(void) __attribute__((noreturn));

register_t registers;
// EEPROM copy of registers; updated on write to register 255.
register_t EEMEM eeprom_registers = {
    .reg = {
        .slave_addr = 0x26,
        .status = 0,
        .direction = 0,
        .reserved = 0,
        .speed = {0, 0},
        .inopts = {0, 0},
        .int_mask = {0, 0},
    }
};

volatile uint8_t eeprom_dirty = FALSE;

void write_slave_addr(uint8_t reg, uint8_t *value) {
    usiTwiSlaveInit(*value, i2c_read, i2c_write);
}

void write_status(uint8_t reg, uint8_t *value) {
    // Clear interrupt pins if requested
    if(!(_BV(STATUS_INT0) & *value))
        DDR_INT &= ~_BV(DD_INT0);
    if(!(_BV(STATUS_INT1) & *value))
        DDR_INT &= ~_BV(DD_INT1);
    
    // No writing to input bits, and interrupt bits can only be cleared
    int value_mask = *value | ~(_BV(STATUS_INT0) | _BV(STATUS_INT1));
    *value = value_mask & registers.reg.status;
}

void write_direction(uint8_t reg, uint8_t *value) {
    // Mask out unused bits
    *value &= _BV(DIR_M1_CCW) | _BV(DIR_M1_CW) |
              _BV(DIR_M2_CCW) | _BV(DIR_M2_CW);
    
    // Set outputs as required
    copy_bit(*value, DIR_M1_CCW, PORT_DIR, P_DIR_M1_CCW);
    copy_bit(*value, DIR_M1_CW, PORT_DIR, P_DIR_M1_CW);
    copy_bit(*value, DIR_M2_CCW, PORT_DIR, P_DIR_M2_CCW);
    copy_bit(*value, DIR_M2_CW, PORT_DIR, P_DIR_M2_CW);
}

void write_reserved(uint8_t reg, uint8_t *value) {
    //*value = 0;
}

void write_speed(uint8_t reg, uint8_t *value) {
    uint8_t tccr; // TCCR register bitmask to activate the specified motor
    volatile uint16_t *ocr; // Register to set PWM duty cycle for specified motor

    if((reg & 1) == 0) {
        // Motor 1
        tccr = COM_ENABLE1;
        ocr = &OCR_ENABLE1;
    } else {
        // Motor 2
        tccr = COM_ENABLE2;
        ocr = &OCR_ENABLE2;
    }

    if(*value == 0) {
        // Disable PWM for constant 0
        TCCR_ENABLE &= ~tccr;
    } else {
        // Enable PWM and set duty cycle
        TCCR_ENABLE |= tccr;
        *ocr = *value;
    }
}

void write_inopts(uint8_t reg, uint8_t *value) {
    if(reg & 1) {
        copy_bit(*value, INOPT_PULLUP_I1, PORT_INPUT, P_INPUT_M2_I1);
        copy_bit(*value, INOPT_PULLUP_I2, PORT_INPUT, P_INPUT_M2_I2);
    } else {
        copy_bit(*value, INOPT_PULLUP_I1, PORT_INPUT, P_INPUT_M1_I1);
        copy_bit(*value, INOPT_PULLUP_I2, PORT_INPUT, P_INPUT_M1_I2);
    }
}

void write_int_mask(uint8_t reg, uint8_t *value) {
}

const register_write_handler write_handlers[] = {
    write_slave_addr,
    write_status,
    write_direction,
    write_reserved,
    write_speed,
    write_speed,
    write_inopts,
    write_inopts,
    write_int_mask,
    write_int_mask,
};

uint8_t i2c_read(uint8_t reg) {
    if(reg < NUM_REGISTERS) {
        return registers.bytes[reg];
    } else {
        return 0;
    }
}

void i2c_write(uint8_t reg, uint8_t value) {
    if(reg < NUM_REGISTERS) {
        write_handlers[reg](reg, &value);
        registers.bytes[reg] = value;
    } else if(reg == 127) {
        // Update eeprom (asynchronously, so we don't block the interrupt).
        eeprom_dirty = TRUE;
    } else {
        // Invalid register - do nothing.
    }
}

void update_input_pin(uint8_t pin, uint8_t input_id) {
    // This function makes the following assumptions:
    // 1) There are exactly 4 inputs
    // 2) The bit position for each input is the same in the STATUS and IMASK registers
    // 3) Bit 1 of input_id can be used to choose which inopts register to select
    // 4) Bit 0 of input_id can be used to choose which offset into the inopts
    //    register to use for the invert and limit bits.
    // 5) The bit position for each input matches the bit in the direction register
    //    it should clear if limit behaviour is configured.
    
    // Get the contents of the inopts register for this input (assumption 3)
    uint8_t inopts = registers.reg.inopts[input_id >> 1];

    // Get the pin value
    uint8_t val = PIN_INPUT & _BV(pin);

    // If the invert flag is set, invert the input (assumption 4)
    if(inopts & _BV(INOPT_INVERT_I1 + (input_id & 1))) {
        val = !val;
    }
    
    // Update the status register (assumption 2)
    if(val) {
        registers.reg.status |= _BV(input_id);
    } else {
        registers.reg.status &= ~_BV(input_id);
    }
    
    // Stop the motor if limits are set (assumptions 4 & 5)
    if(val && (inopts & _BV(INOPT_LIMIT_I1 + (input_id & 1)))) {
        if(registers.reg.direction & _BV(input_id)) {
            registers.reg.direction &= ~_BV(input_id);
            write_direction(2, &registers.reg.direction);
        }
    }
    
    // Signal an interrupt if requested
    if(registers.reg.int_mask[0] & _BV(input_id)) {
        registers.reg.status |= _BV(STATUS_INT0);
        DDR_INT |= _BV(DD_INT0);
    }
    if(registers.reg.int_mask[1] & _BV(input_id)) {
        registers.reg.status |= _BV(STATUS_INT1);
        DDR_INT |= _BV(DD_INT1);
    }
}

void update_inputs() {
    update_input_pin(P_INPUT_M1_I1, 0);
    update_input_pin(P_INPUT_M1_I2, 1);
    update_input_pin(P_INPUT_M2_I1, 2);
    update_input_pin(P_INPUT_M2_I2, 3);
}   

ISR(PCINT_vect) {
    // One of the input pins has changed value.
    update_inputs();
}

void ioinit(void) {
    // Set the enable pins as outputs
    DDR_ENABLE |= _BV(DD_ENABLE1) | _BV(DD_ENABLE2);
    // Setup PWM: Fast PWM mode, 8-bit, no prescaling. Don't enable it yet.
    ENABLE_PWM_SETUP();
    // Set the direction pins as outputs
    DDR_DIR |= _BV(DD_DIR1) | _BV(DD_DIR2) | _BV(DD_DIR3) | _BV(DD_DIR4);
}

void configure_interrupts(void) {
    // Enable the pin change interrupt for the relevant pins
    PCMSK |= PCINT_MASK;
    // Turn on the pin change interrupt globally
    GIMSK |= _BV(PCIE);
}

void read_registers(void) {
    // Initialize from eeprom
    eeprom_read_block(&registers, &eeprom_registers, sizeof(registers));

    // Run all the write funcs in order to initialize the device
    for(int i = 0; i < NUM_REGISTERS; i++)
        write_handlers[i](i, &registers.bytes[i]);
}

void main(void) {
    ioinit();
    configure_interrupts();
    read_registers();
    // Enable interrupts
    sei();
    
    for(;;) {
        if(eeprom_dirty) {
            eeprom_write_block(&registers, &eeprom_registers, sizeof(registers));
            eeprom_dirty = FALSE;
        }
    }
}
