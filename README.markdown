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
