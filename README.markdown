MotoProtoShield
===============

The MotoProtoShield is a stackable Arduino shield for driving low-power DC or
stepper motors over I2C.

Features:

 - 100% stackable. The number of stacked shields you can drive is limited only
   by the available current on the vin (motor) and 5v rails, and the number of
   I2C addresses available.
 - PWM control for variable speed control.
 - Each board can drive two 1A motors, or the outputs can be combined to drive
   one higher current motor.
 - Optional kickback diodes protect the H-Bridge from inductive kickback.
 - Can be configured to use any valid I2C address in software.
 - Screw terminals for two inputs per motor support a pair of limit switches or
   quadrature encoder per motor. Set-and-forget operation takes the strain of
   handling encoder input off the Arduino.
 - Events can be signalled back to the host board on interrupt pins 2 or 3,
   software-selectable.

Hardware
--------

The MotoProtoShield has two main components: An ATTiny2313 MCU, and an SN754410
quad half H-Bridge. The MCU takes care of the logic and communicates over the
I2C bus, while the H-Bridge takes care of motor driving. Optional kickback
diodes provide additional protection against inductive kickback; also included
are optional power smoothing capacitors and I2C pullups.

Pin assignments for the MCU are as follows:

 - OC1A and OC1B connect to the H-Bridge's enable lines,
   permitting PWM control of each motor.
 - PB0-PB2 and PB6 connect to the input sensors for limit switches or quadrature
   encoders. Pin change interrupts on these lines allow effective handling of
   encoder events.
 - PD2-PD5 connect to the direction control lines on each H-Bridge.
 - PD1 and PD6 connect to Arduino pins 2 and 3 for signalling interrupts to the
   host processor.
 - SCL and SDA connect to the Arduino's A4 and A5 pins for I2C communication
   with the host Arduino.

Software
--------

The ATTiny2313 MCU on each shield exposes an I2C based control interface. All
functionality is exposed via a set of 8-bit registers which can be read and
written over I2C.

To read a register, write a single byte to the device - the register number -
then read a single byte, which will be the contents of the register.

To write a register, write two bytes to the device - the register number and its
value, respectively.

By default, register changes are not persistent, and the device will reset to
its default state if power is removed and restored. Register changes can be 
made permanent by writing to register 255, STORE.

Register mappings are described below.

### Register 0: ADDR

<table>
  <tr><th>7</th><th>6</th><th>5</th><th>4</th><th>3</th><th>2</th><th>1</th><th>0</th></tr>
  <tr><td colspan="8">ADDRESS</td></tr>
</table>

The ADDR register stores the I2C address of the shield as an 8-bit unsigned
integer. Writing to this register will change the shield's address for all future
communications.

### Register 1: STATUS

<table>
  <tr><th>7</th><th>6</th><th>5</th><th>4</th><th>3</th><th>2</th><th>1</th><th>0</th></tr>
  <tr><td colspan="2">Reserved</td><td>INT1</td><td>INT0</td><td>M2_I2</td><td>M2_I1</td><td>M1_I2</td><td>M1_I1</td></tr>
</table>

The STATUS register contains general device status information. The INT0 and
INT1 bits reflect the current status of the two interrupt pins. Writing a 0 to
them will clear the interrupt, while writing a 1 has no effect. The Mx_Iy bits
reflect the current status of input y on motor x. If the relevant
INOPT[x].INVERT_Iy bit is set, the value here will be the inverse of the state
of the actual pin.

### Register 2: DIR

<table>
  <tr><th>7</th><th>6</th><th>5</th><th>4</th><th>3</th><th>2</th><th>1</th><th>0</th></tr>
  <tr><td colspan="4">Reserved</td><td colspan="2">M2</td><td colspan="2">M1</td></tr>
</table>

The DIR register controls the direction of motion for each motor. For each,
values of 00 and 11 indicate no movement - active braking if the motor is
engaged - while 01 and 10 indicate clockwise and counterclockwise motion,
respectively (how this translates into motion on your motor will depend on how
it is wired up).

One special case to note is that if limit switch behaviour is enabled with
INOPT[x].LIMIT_Iy, the limit switch being triggered will set the corresponding
bit of the direction register to 0. If the register was set to 11, this could
cause the motor to start moving. If this behaviour is not desired, set the
register to 00 when no motion is desired.

### Register 3: Reserved

This register is reserved for future use. All reads return 0, and writes will be
ignored.

### Registers 4 & 5: SPEED

<table>
  <tr><th>7</th><th>6</th><th>5</th><th>4</th><th>3</th><th>2</th><th>1</th><th>0</th></tr>
  <tr><td colspan="8">SPEED</td></tr>
</table>

The two speed registers control the speed of each motor. SPEED[0] controls motor
1, while SPEED[1] controls motor 2. Each register is treated as a single 8-bit
unsigned integer. Setting the value to 0 holds the enable pin of the H-Bridge
port for the relevant motor low, while setting it to 255 holds it high.
Intermediate values cause a PWM waveform to be output with a corresponding
duty-cycle.

### Registers 6 & 7: INOPTS

<table>
  <tr><th>7</th><th>6</th><th>5</th><th>4</th><th>3</th><th>2</th><th>1</th><th>0</th></tr>
  <tr><td colspan="2">Reserved</td><td>LIMIT_I2</td><td>LIMIT_I1</td><td>INVERT_I2</td><td>INVERT_I1</td><td>PULLUP_I2</td><td>PULLUP_I1</td></tr>
</table>

The two inopts registers control input options for each motor. INOPTS[0]
controls settings for the first motor, while INOPTS[1] controls settings for the
second one.

LIMIT_I1 and LIMIT_I2 control limit behaviour. If the LIMIT_In bit is set, and
the correspondingly numbered input becomes high (after being inverted, if
specified by INVERT_In), the relevant bit in the direction register will be set
to 0, halting the motor if it was moving. Thus, limit switches can be connected
to the relevant inputs to cause the motor to halt when it reaches a limit. Note
that while the direction bit will be unset, the speed register will be left
unmodified.

INVERT_I1 and INVERT_I2 control how inputs are treated. If the INVERT_In bit is
set, the correspondingly numbered input has its value inverted. This inverted
value is used in all locations, including when considering limit behaviour and
in the STATUS register.

PULLUP_I1 and PULLUP_I2 control the pullups on each input pin. If the relevant
bit is set, pullups will be enabled for that input pin.

### Registers 8 & 9: IMASK

<table>
  <tr><th>7</th><th>6</th><th>5</th><th>4</th><th>3</th><th>2</th><th>1</th><th>0</th></tr>
  <tr><td colspan="4">Reserved</td><td>LIMIT_M2_I2</td><td>LIMIT_M2_I1</td><td>LIMIT_M1_I2</td><td>LIMIT_M1_I1</td></tr>
</table>

The two imask registers control the conditions under which an interrupt will be
triggered. Triggering an interrupt results in setting the relevant bit in the
STATUS register, and pulling the relevant input pin LOW.

When an interrupt is not being triggered, the pin is left in a high-impedance
state. This convention is present for two reasons: By pulling the pin low,
rather than high, pullups can be used by the Arduino to make the pin normally
high. By setting the normal state to high impedance, multiple motor shields can
use the same interrupt line, and if the interrupt is not required, the pin can
be used for other purposes.

Each of the LIMIT_Ma_Ib bits controls whether the input will be triggered when
the corresponding input pin becomes high (after inversion if INOPTS[a].INVERT_Ib
is set).

### Register 127: STORE

Normally, changes made to registers persist only until power is removed. On each
startup, default values for each register are read from the processor's EEPROM.

Writing any value to the STORE register causes the current state of all
registers to be asynchronously written to permanent EEPROM storage. On
subsequent boots, the stored values will be loaded and used as defaults.

Default values, when loaded, are treated the same as if they had been set using
standard I2C commands. As a result, regardless of the value of the STATUS
register when STORE was written to, it will always be initialized with all bits
unset. All other registers, including the speed and direction registers, will
be initialized to the values they had when the write occurred.
