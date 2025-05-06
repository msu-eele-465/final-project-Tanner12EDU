#include <msp430.h> 

// Port definitions
#define PXOUT P1OUT
#define PXSEL0 P1SEL0
#define PXSEL1 P1SEL1
#define PXDIR P1DIR

// Pin definitions
#define D4 BIT0
#define D5 BIT1
#define D6 BIT4
#define D7 BIT5

#define E BIT6
#define RS BIT7

// I2C definitions
#define ADDRESS 0x01    // Address for microcontroller

// LCD Variables

int hours, minutes;

int ambient_int, ambient_dec;

int plant_int, plant_dec;

int watered_amount;


void lcd_pulse_enable(){
    // Pulses the enable pin so LCD knows to take next nibble.
    PXOUT |= E;         // Enable Enable pin
    PXOUT &= ~E;        // Disable Enable pin
}

void lcd_send_nibble(char nibble){
    // Sends out four bits to the LCD by setting the last four pins (D4 - 7) to the nibble.

    PXOUT &= ~(D4 | D5 | D6 | D7); // Clear the out bits associated with our data lines.

    // For each bit of the nibble, set the associated data line to it.
    if (nibble & BIT0) PXOUT |= D4;
    if (nibble & BIT1) PXOUT |= D5;
    if (nibble & BIT2) PXOUT |= D6;
    if (nibble & BIT3) PXOUT |= D7;

    //PXOUT = (PXOUT &= 0xF0) | (nibble & 0x0F); // First we clear the first four bits, then we set them to the nibble.
    lcd_pulse_enable();  // Pulse enable so LCD reads our input
}

void lcd_send_command(char command){
    // Takes an 8-bit command and sends out the two nibbles sequentially.
    PXOUT &= ~RS; // CLEAR RS to set to command mode
    lcd_send_nibble(command >> 4); // Send upper nibble by bit shifting it to lower nibble
    lcd_send_nibble(command & 0x0F); // Send lower nibble by clearing upper nibble.
}

void lcd_send_data(char data){
    // Takes an 8-bit data and sends out the two nibbles sequentially.
    PXOUT |= RS; // SET RS to set to data mode
    lcd_send_nibble(data >> 4); // Send upper nibble by bit shifting it to lower nibble
    lcd_send_nibble(data & 0x0F); // Send lower nibble by clearing upper nibble.
}

void lcd_print_sentence(char *str){
    // Takes a string and iterates character by character, sending that character to be written out, until \0 is reached.
    while(*str){
        lcd_send_data(*str);
        str++;
    }
}

void lcd_clear(){
    // Clearing the screen is temperamental and requires a good delay, this is pretty arbitrary with a good safety margin.
    lcd_send_command(0x01);   // Clear display
    __delay_cycles(2000);   // Clear display needs some time, I'm aware __delay_cycles generally isn't advised, but it works in this context
}

void lcdInit(){
    // Initializes the LCD to 4 bit mode, 2 lines 5x8 font, with enabled display and cursor,
    // clear the display, then set cursor to proper location.

    PXOUT &= ~RS; // Explicitly set RS to 0 so we are in command mode

    // We need to send the code 3h, 3 times, to properly wake up the LCD screen
    lcd_send_nibble(0x03);
    lcd_send_nibble(0x03);
    lcd_send_nibble(0x03);

    lcd_send_nibble(0x02);    // Code 2h sets it to 4-bit mode after waking up

    lcd_send_command(0x28);   // Code 28h sets it to 2 line, 5x8 font.

    lcd_send_command(0x0C);   // Turns display on, turns cursor off, turns blink off.

    lcd_send_command(0x06);   // Increments cursor on each input

    lcd_clear();             // Clear display
}

void lcd_write(){
    /*  Ultimately dictates what will be present on screen after an I2C transmission.
    */

    lcd_clear();

    lcd_send_command(0x80); // Set cursor to line 1 position 1

    char time_string[6]; // Buffer for storing time string
    int i = 0;

    // Format hours
    time_string[i++] = (hours / 10) + '0';  // Tens place of hours
    time_string[i++] = (hours % 10) + '0';  // Ones place of hours

    time_string[i++] = ':'; // Add colon separator

    // Format minutes
    time_string[i++] = (minutes / 10) + '0';  // Tens place of minutes
    time_string[i++] = (minutes % 10) + '0';  // Ones place of minutes

    time_string[i] = '\0'; // Null-terminate the string

    lcd_print_sentence(time_string);

    char ambient_string[7]; // Buffer for converting ambient temp value to string
    i = 0;

    ambient_string[i++] = (ambient_int / 10) + '0';  // Tens place
    ambient_string[i++] = (ambient_int % 10) + '0';  // Ones place
    ambient_string[i++] = '.';
    ambient_string[i++] = (ambient_dec / 10) + '0';  // First decimal place
    ambient_string[i++] = 0b11011111; // Degrees symbol
    ambient_string[i++] = 'C';

    char plant_string[7]; // Buffer for converting peltier temp value to string
    i = 0;

    plant_string[i++] = (plant_int / 10) + '0';  // Tens place
    plant_string[i++] = (plant_int % 10) + '0';  // Ones place
    plant_string[i++] = '.';
    plant_string[i++] = (plant_dec / 10) + '0';  // First decimal place
    plant_string[i++] = 0b11011111; // Degrees symbol
    plant_string[i++] = 'C';

    lcd_send_command(0x88); // Set cursor to line 1 position 9
    lcd_print_sentence("A:");
    lcd_print_sentence(ambient_string);

    lcd_send_command(0xC0); // Set cursor to line 2 position 1

    char watered_amount_array[2];
    watered_amount_array[0] = (watered_amount % 10) + '0';

    lcd_print_sentence(watered_amount_array);

    lcd_send_command(0xC8); // Set cursor to line 1 position 9
    lcd_print_sentence("P:");
    lcd_print_sentence(plant_string);

    lcd_send_command(0x80); // Return cursor to line 1 position 1
}

int main(void)
{
    WDTCTL = WDTPW | WDTHOLD;   // stop watchdog timer

    //---------------- Configure TB0 ----------------
    TB0CTL |= TBCLR;            // Clear TB0 timer and dividers
    TB0CTL |= TBSSEL__ACLK;     // Select ACLK as clock source
    TB0CTL |= MC__UP;           // Choose UP counting

    TB0CCR0 = 32767;            // ACLK = 32.768 KHz, from 0 count up to 32767, takes 1 second.
    TB0CCTL0 &= ~CCIFG;         // Clear CCR0 interrupt flag
    TB0CCTL0 |= CCIE;           // Enable interrupt vector for CCR0
    //---------------- End Configure TB0 ----------------

    //---------------- Configure LCD Ports ----------------

    // Configure Port for digital I/O
    PXSEL0 &= 0x00;
    PXSEL1 &= 0x00;

    PXDIR |= 0XFF;  // SET all bits so Port is OUTPUT mode

    PXOUT &= 0x00;  // CLEAR all bits in output register
    //---------------- End Configure Ports ----------------

    //---------------- Configure UCB0 I2C ----------------

    // Configure P1.2 (SDA) and P1.3 (SCL) for I2C
    P1SEL0 |= BIT2 | BIT3;
    P1SEL1 &= ~(BIT2 | BIT3);

    UCB0CTLW0 = UCSWRST;                 // Put eUSCI in reset
    UCB0CTLW0 |= UCMODE_3 | UCSYNC;      // I2C mode, synchronous mode
    UCB0I2COA0 = ADDRESS | UCOAEN;       // Set slave address and enable
    UCB0CTLW0 &= ~UCSWRST;               // Release eUSCI from reset
    UCB0IE |= UCRXIE0;                   // Enable receive interrupt
    //---------------- End Configure UCB0 I2C ----------------

    PM5CTL0 &= ~LOCKLPM5;       // Clear lock bit
    __bis_SR_register(GIE);     // Enable global interrupts

    lcdInit();

    while(1){

    }

    return 0;
}

//-------------------------------------------------------------------------------
// Interrupt Service Routines
//-------------------------------------------------------------------------------

#pragma vector=USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void) {
    //ISR For receiving I2C transmissions
    /* The microcontroller expects four bytes to be transmitted from the master.
     * [state], [pattern], [temperature_int], [temperature_dec], [window_size]
     * These bytes are sequentially added to the pattern_index, period_index, and key values.
     *
     * These values are then processed by lcd_write(), where more information can be found about their handling.
     */
    static int byte_count = 0;
    if(UCB0IV == 0x16){  // RXIFG0 Flag, RX buffer is full and can be processed
        switch(byte_count){

            case 0:
                hours = UCB0RXBUF;
                break;
            case 1:
                minutes = UCB0RXBUF;
                break;
            case 2:
                ambient_int= UCB0RXBUF;
                break;
            case 3:
                ambient_dec = UCB0RXBUF;
                break;
            case 4:
                plant_int = UCB0RXBUF;
                break;
            case 5:
                plant_dec = UCB0RXBUF;
                break;
            case 6:
                watered_amount = UCB0RXBUF;
                break;
            default:
                break;
        }
        byte_count++;
        if(byte_count >= 7){
            byte_count = 0;
        }
    }
}

//---------------- START ISR_TB0_SwitchColumn ----------------
//-- TB0 CCR0 interrupt, one second pulse for counting operation time.
#pragma vector = TIMER0_B0_VECTOR
__interrupt void ISR_TB0_OneSecondPulse(void)
{

    lcd_write();

    TB0CCTL0 &= ~TBIFG;
}
