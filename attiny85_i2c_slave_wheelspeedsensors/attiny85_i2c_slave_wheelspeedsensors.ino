/*
 * Example sketch for writing to and reading from a slave in transactional manner
 *
 * NOTE: You must not use delay() or I2C communications will fail, use tws_delay() instead (or preferably some smarter timing system)
 *
 * On write the first byte received is considered the register addres to modify/read
 * On each byte sent or read the register address is incremented (and it will loop back to 0)
 *
 * You can try this with the Arduino I2C REPL sketch at https://github.com/rambo/I2C/blob/master/examples/i2crepl/i2crepl.ino 
 * If you have bus-pirate remember that the older revisions do not like the slave streching the clock, this leads to all sorts of weird behaviour
 *
 * To read third value (register number 2 since counting starts at 0) send "[ 8 2 [ 9 r ]", value read should be 0xBE
 * If you then send "[ 9 r r r ]" you should get 0xEF 0xDE 0xAD as response (demonstrating the register counter looping back to zero)
 *
 * You need to have at least 8MHz clock on the ATTiny for this to work (and in fact I have so far tested it only on ATTiny85 @8MHz using internal oscillator)
 * Remember to "Burn bootloader" to make sure your chip is in correct mode 
 */


/**
 * Pin notes by Suovula, see also http://hlt.media.mit.edu/?p=1229
 *
 * DIP and SOIC have same pinout, however the SOIC chips are much cheaper, especially if you buy more than 5 at a time
 * For nice breakout boards see https://github.com/rambo/attiny_boards
 *
 * Basically the arduino pin numbers map directly to the PORTB bit numbers.
 *
// I2C
arduino pin 0 = not(OC1A) = PORTB <- _BV(0) = SOIC pin 5 (I2C SDA, PWM)
arduino pin 2 =           = PORTB <- _BV(2) = SOIC pin 7 (I2C SCL, Analog 1)
// Timer1 -> PWM
arduino pin 1 =     OC1A  = PORTB <- _BV(1) = SOIC pin 6 (PWM)
arduino pin 3 = not(OC1B) = PORTB <- _BV(3) = SOIC pin 2 (Analog 3)
arduino pin 4 =     OC1B  = PORTB <- _BV(4) = SOIC pin 3 (Analog 2)
 */
#define I2C_SLAVE_ADDRESS 0x4 // the 7-bit address (remember to change this when adapting this example)
// Get this from https://github.com/rambo/TinyWire
#include <TinyWireS.h>
// The default buffer size, Can't recall the scope of defines right now
#ifndef TWI_RX_BUFFER_SIZE
#define TWI_RX_BUFFER_SIZE ( 16 )
#endif
#define DISTANCE_PER_INTERRUPT 1

//scaling value for low pass filter to prevent outrageous changes
#define SCALING_VALUE 0.5

//minimum value in ms to wait for the next interrupt
#define DEBOUNCE_WAIT_VAL_MILLIS 50

volatile unsigned long lastTimeTriggered = 0; //storage location for last time triggered

// Tracks the current register pointer position
volatile byte reg_position;
const byte reg_size = 4;

union byte_float_union{
  char bVal[4];
  float fVal;
}; 

volatile byte_float_union wheelSpeed; 

/**
 * This is called for each read request we receive, never put more than one byte of data (with TinyWireS.send) to the 
 * send-buffer when using this callback
 */
void requestEvent()
{  
    TinyWireS.send(wheelSpeed.bVal[reg_position]);

    // Increment the reg position on each read, and loop back to zero
    reg_position++;
    if (reg_position >= reg_size)
    {
        reg_position = 0;
    }
}

/**
 * The I2C data received -handler
 *
 * This needs to complete before the next incoming transaction (start, data, restart/stop) on the bus does
 * so be quick, set flags for long running tasks to be called from the mainloop instead of running them directly,
 */
void receiveEvent(uint8_t howMany)
{
   _NOP();
}

ISR(PCINT0_vect)
{
  /*
   * TODO: trigger only on one edge
   */
  cli(); //disable interrupts for the moment
  if(!(PINB & _BV(PB1)))
  {
    unsigned long curTimeTriggered = millis();
    //wheelSpeed.fVal = ((curTimeTriggered - lastTimeTriggered) > DEBOUNCE_WAIT_VAL_MILLIS) ? ((1-SCALING_VALUE)*wheelSpeed.fVal) + SCALING_VALUE*((float)(DISTANCE_PER_INTERRUPT)/(float)((curTimeTriggered - lastTimeTriggered)/10E3)) :  wheelSpeed.fVal; //calculate wheel rotation speed in m/s
    if((curTimeTriggered - lastTimeTriggered) > DEBOUNCE_WAIT_VAL_MILLIS)
    {
      //wheelSpeed.fVal = (float)(DISTANCE_PER_INTERRUPT)/(float)((curTimeTriggered - lastTimeTriggered)/10E3);
      wheelSpeed.fVal = ((1-SCALING_VALUE)*wheelSpeed.fVal) + SCALING_VALUE*((float)(DISTANCE_PER_INTERRUPT)/(float)((curTimeTriggered - lastTimeTriggered)/10E3));
      lastTimeTriggered = curTimeTriggered;
    }
  }
    sei(); //re-
}

void setup()
{
    cli();
    pinMode(1,INPUT_PULLUP); //enable internal pullup resistors on PB2
    PCMSK |= (1 << PCINT1); //enable PCINT1 in Pin Change Interrupt reegister
    GIMSK |= (1 << PCIE); //enable pin change interrupts in general interrupt mask
    
    
    pinMode(3, OUTPUT); // OC1B-, Arduino pin 3, ADC
    pinMode(1, OUTPUT); // OC1A, also The only HW-PWM -pin supported by the tiny core analogWrite

    /**
     * Reminder: taking care of pull-ups is the masters job
     */

    TinyWireS.begin(I2C_SLAVE_ADDRESS);
    TinyWireS.onReceive(receiveEvent);
    TinyWireS.onRequest(requestEvent);

    
    // Whatever other setup routines ?
    
    wheelSpeed.fVal = 0.0; //test value
    sei(); //enable global interrupts
}

void loop()
{
    /**
     * This is the only way we can detect stop condition (http://www.avrfreaks.net/index.php?name=PNphpBB2&file=viewtopic&p=984716&sid=82e9dc7299a8243b86cf7969dd41b5b5#984716)
     * it needs to be called in a very tight loop in order not to miss any (REMINDER: Do *not* use delay() anywhere, use tws_delay() instead).
     * It will call the function registered via TinyWireS.onReceive(); if there is data in the buffer on stop.
     */
    TinyWireS_stop_check();
    
}
