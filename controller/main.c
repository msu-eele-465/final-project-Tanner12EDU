#include <msp430.h>
#include <stdint.h>
#include <math.h>

#define RTC_ADDRESS 0x68
#define LCD_ADDRESS 0x01
#define TX_BYTES 6
#define LEDA BIT0
#define LEDB BIT1
#define LEDC BIT2
#define LEDD BIT3
#define LEDE BIT4
#define LEDF BIT3
#define LEDG BIT4

volatile uint8_t window_size = 3;
volatile uint8_t adc_channel_state = 0;  // 0: ambient, 1: plant, 2: UV

uint16_t ambient_results[10] = {0};
uint16_t plant_results[10] = {0};
uint16_t uv_results[10] = {0};
uint8_t ambient_sample_index = 0;
uint8_t plant_sample_index = 0;
uint8_t uv_sample_index = 0;
uint8_t times_watered = 0;

volatile uint8_t ambient_temperature_integer = 0;
volatile uint8_t ambient_temperature_decimal = 0;
volatile uint8_t plant_temperature_integer = 0;
volatile uint8_t plant_temperature_decimal = 0;
volatile uint8_t uv_intensity_integer = 0;

uint32_t servo_last_move_time = 0;
uint32_t servo_cooldown_period = 600;

volatile uint8_t tx_index = 0;
uint8_t tx_buffer[TX_BYTES] = {0};

uint8_t bcd_to_decimal(uint8_t bcd) {
    return ((bcd / 16 * 10) + (bcd % 16));
}

uint8_t decimal_to_bcd(uint8_t decimal) {
    return ((decimal / 10 * 16) + (decimal % 10));
}

void delay_ms(unsigned int ms) {
    while (ms--) {
        __delay_cycles(1000);
    }
}

void move_servos() {
    uint32_t current_time = 0;
    uint8_t hours = 0, minutes = 0;
    ds3231_get_time(&hours, &minutes);

    // Calculate current time in seconds (assuming hours and minutes are accurate)
    current_time = hours * 3600 + minutes * 60;

    // Check if enough time has passed since the last move (cooldown check)
    if ((current_time - servo_last_move_time) >= servo_cooldown_period) {
        // Move servos to water plants
        TB3CCR1 = 1200;  // Clockwise
        TB3CCR2 = 1800;  // Counter-clockwise
        delay_ms(1000);
        TB3CCR1 = 1500;  // Stop
        TB3CCR2 = 1500;

        // Pause for 3 seconds
        delay_ms(3000);

        TB3CCR1 = 1800;
        TB3CCR2 = 1200;
        delay_ms(1000);
        TB3CCR1 = 1500;
        TB3CCR2 = 1500;

        times_watered++;

        // Update last move time to current time
        servo_last_move_time = current_time;
    }
}

void send_I2C_data() {
    if (!(UCB0STATW & UCBBUSY)) {
        tx_index = 0;
        UCB0IE |= UCTXIE0;
        UCB0CTLW0 |= UCTR | UCTXSTT;
    }
}

void start_ADC_conversion() {
    ADCCTL0 |= ADCSC;
}

void get_temperature() {
    uint8_t i;
    uint32_t ambient_total_adc = 0;
    uint32_t plant_total_adc = 0;

    for (i = 0; i < window_size; i++) {
        ambient_total_adc += ambient_results[i];
        plant_total_adc += plant_results[i];
    }

    float a_voltage = (ambient_total_adc * 3.3f) / (window_size * 4095.0f);
    float a_temp = -1481.96 + sqrt(2.1962e6 + ((1.8639 - a_voltage) / (3.88e-6)));
    ambient_temperature_integer = (uint8_t)(a_temp / 8);
    ambient_temperature_decimal = (uint8_t)((((a_temp / 8.0f) - ambient_temperature_integer) * 10));

    float p_voltage = (plant_total_adc * 3.3f) / (window_size * 4095.0f);
    float p_temp = -1481.96 + sqrt(2.1962e6 + ((1.8639 - p_voltage) / (3.88e-6)));
    plant_temperature_integer = (uint8_t)(p_temp / 8);
    plant_temperature_decimal = (uint8_t)((((p_temp / 8.0f) - plant_temperature_integer) * 10));

    uint8_t hours = 0, minutes = 0;
    ds3231_get_time(&hours, &minutes);

    tx_buffer[0] = hours;
    tx_buffer[1] = minutes;
    tx_buffer[2] = ambient_temperature_integer;
    tx_buffer[3] = ambient_temperature_decimal;
    tx_buffer[4] = plant_temperature_integer;
    tx_buffer[5] = plant_temperature_decimal;
    tx_buffer[6] = times_watered;

    send_I2C_data();

    uint8_t delta = ambient_temperature_integer - plant_temperature_integer;
    if (delta < 0) delta = -delta;
    if (delta <= 2) move_servos();
}

void get_uv() {
    uint8_t i;
    uint8_t old_uv = uv_intensity_integer;
    uint32_t uv_total = 0;

    for (i = 0; i < window_size; i++) {
        uv_total += uv_results[i];
    }

    float voltage = (uv_total * 3.3f) / (window_size * 4095.0f);
    float intensity = voltage * 10;
    uv_intensity_integer = (uint8_t)intensity;

    if (old_uv != uv_intensity_integer) {
        update_7segment();
    }
}

void update_7segment() {
    switch (uv_intensity_integer) {
        case 0:
            P5OUT |= (LEDA | LEDB | LEDC | LEDD | LEDE);
            P6OUT |= LEDF;
            P6OUT &= ~LEDG;
            break;
        case 1:
            P5OUT |= (LEDB | LEDC);
            P5OUT &= ~(LEDA | LEDD | LEDE);
            P6OUT &= ~(LEDF | LEDG);
            break;
        case 2:
            P5OUT |= (LEDA | LEDB | LEDD | LEDE);
            P6OUT |= LEDG;
            P5OUT &= ~(LEDC);
            P6OUT &= ~LEDF;
            break;
        case 3:
            P5OUT |= (LEDA | LEDB | LEDC | LEDD);
            P6OUT |= LEDG;
            P5OUT &= ~LEDE;
            P6OUT &= ~LEDF;
            break;
        case 4:
            P5OUT |= (LEDB | LEDC);
            P6OUT |= (LEDF | LEDG);
            P5OUT &= ~(LEDA | LEDD | LEDE);
            break;
        case 5:
            P5OUT |= (LEDA | LEDC | LEDD);
            P6OUT |= (LEDF | LEDG);
            P5OUT &= ~(LEDB | LEDE);
            break;
        case 6:
            P5OUT |= (LEDA | LEDC | LEDD | LEDE);
            P6OUT |= (LEDF | LEDG);
            P5OUT &= ~LEDB;
            break;
        case 7:
            P5OUT |= (LEDA | LEDB | LEDC);
            P5OUT &= ~(LEDD | LEDE);
            P6OUT &= ~(LEDF | LEDG);
            break;
        case 8:
            P5OUT |= (LEDA | LEDB | LEDC | LEDD | LEDE);
            P6OUT |= (LEDF | LEDG);
            break;
        case 9:
            P5OUT |= (LEDA | LEDB | LEDC);
            P6OUT |= (LEDF | LEDG);
            P5OUT &= ~(LEDD | LEDE);
            break;
        case 10:
            P5OUT |= (LEDA | LEDB | LEDC | LEDE);
            P6OUT |= (LEDF | LEDG);
            P5OUT &= ~LEDD;
            break;
        default:
            P5OUT |= (LEDC | LEDD | LEDE);
            P6OUT |= (LEDF | LEDG);
            P5OUT &= ~(LEDA | LEDB);
            break;
    }
}

void ds3231_set_time(uint8_t hours, uint8_t minutes) {
    UCB1I2CSA = RTC_ADDRESS;
    UCB1CTLW0 |= UCTR | UCTXSTT;
    while (UCB1CTLW0 & UCTXSTT);

    while (!(UCB1IFG & UCTXIFG0));
    UCB1TXBUF = 0x00;

    while (!(UCB1IFG & UCTXIFG0));
    UCB1TXBUF = decimal_to_bcd(0);

    while (!(UCB1IFG & UCTXIFG0));
    UCB1TXBUF = decimal_to_bcd(minutes);

    while (!(UCB1IFG & UCTXIFG0));
    UCB1TXBUF = decimal_to_bcd(hours);

    while (!(UCB1IFG & UCTXIFG0));
    UCB1CTLW0 |= UCTXSTP;
    while (UCB1CTLW0 & UCTXSTP);
}

void ds3231_get_time(uint8_t *hours, uint8_t *minutes) {
    UCB1I2CSA = RTC_ADDRESS;

    UCB1CTLW0 |= UCTR | UCTXSTT;
    while (UCB1CTLW0 & UCTXSTT);
    while (!(UCB1IFG & UCTXIFG0));
    UCB1TXBUF = 0x00;

    while (!(UCB1IFG & UCTXIFG0));
    UCB1CTLW0 &= ~UCTR;
    UCB1CTLW0 |= UCTXSTT;
    while (UCB1CTLW0 & UCTXSTT);

    while (!(UCB1IFG & UCRXIFG0));
    volatile uint8_t seconds = UCB1RXBUF;

    while (!(UCB1IFG & UCRXIFG0));
    *minutes = bcd_to_decimal(UCB1RXBUF);

    while (!(UCB1IFG & UCRXIFG0));
    *hours = bcd_to_decimal(UCB1RXBUF);

    UCB1CTLW0 |= UCTXSTP;
    while (UCB1CTLW0 & UCTXSTP);
}

int main(void) {
    WDTCTL = WDTPW | WDTHOLD;

    P1SEL0 |= BIT1 | BIT3 | BIT4;
    P1SEL1 |= BIT1 | BIT3 | BIT4;

    ADCCTL0 &= ~ADCENC;
    ADCCTL0 |= ADCON | ADCSHT_2;
    ADCCTL1 |= ADCSHP | ADCSSEL_2;
    ADCCTL2 |= ADCRES_2;
    ADCIE |= ADCIE0;
    ADCCTL0 |= ADCENC;

    P1DIR |= BIT0;
    P1OUT |= BIT0;
    P6DIR |= BIT6;
    P6OUT &= ~BIT6;

    P5DIR |= LEDA | LEDB | LEDC | LEDD | LEDE;
    P6DIR |= LEDF | LEDG;
    P5OUT |= LEDA | LEDB | LEDC | LEDD | LEDE;
    P6OUT &= ~LEDG;
    P6OUT |= LEDF;


    TB0CTL = TBSSEL__ACLK | ID__8 | MC__UP | TBCLR;
    TB0CCR0 = 20480;
    TB0CCTL0 |= CCIE;

    TB1CTL = TBSSEL__ACLK | MC__UP | TBCLR;
    TB1CCR0 = 32768;
    TB1CCTL0 |= CCIE;

    P6DIR |= BIT0 | BIT1;
    P6SEL0 |= BIT0 | BIT1;

    TB3CTL = TBSSEL__SMCLK | MC__UP | TBCLR;
    TB3CCR0 = 20000;
    TB3CCTL1 = OUTMOD_7;
    TB3CCTL2 = OUTMOD_7;
    TB3CCR1 = 1500;
    TB3CCR2 = 1500;

    ds3231_set_time(9, 0);

    P1SEL0 |= BIT2 | BIT3;
    P4SEL0 |= BIT6 | BIT7;
    UCB0CTLW0 = UCSWRST | UCMODE_3 | UCMST | UCSYNC | UCSSEL_3;
    UCB0BRW = 10;
    UCB0I2CSA = LCD_ADDRESS;
    UCB0CTLW0 &= ~UCSWRST;

    P4SEL0 |= BIT6 | BIT7;
    UCB1CTLW0 = UCSWRST | UCMODE_3 | UCMST | UCSYNC | UCSSEL_3;
    UCB1BRW = 10;
    UCB1I2CSA = RTC_ADDRESS;
    UCB1CTLW0 &= ~UCSWRST;
    UCB1IE |= UCRXIE0;

    send_I2C_data();
    PM5CTL0 &= ~LOCKLPM5;
    __enable_interrupt();

    while (1) {
        __low_power_mode_3();
    }
}

#pragma vector = TIMER0_B0_VECTOR
__interrupt void ISR_TB0_ADC(void) {
    switch (adc_channel_state) {
        case 0:
            ADCMCTL0 = ADCINCH_1;  // Ambient
            break;
        case 1:
            ADCMCTL0 = ADCINCH_4;  // Plant
            break;
        case 2:
            ADCMCTL0 = ADCINCH_5;  // UV
            break;
    }
    ADCCTL0 |= ADCSC;  // Start ADC conversion
    TB0CCTL0 &= ~CCIFG;
}

#pragma vector = TIMER1_B0_VECTOR
__interrupt void ISR_TB1_Heartbeat(void) {
    P1OUT ^= BIT0;
    P6OUT ^= BIT6;
    TB1CCTL0 &= ~CCIFG;
}

#pragma vector = ADC_VECTOR
__interrupt void ADC_ISR(void) {
    static uint8_t index = 0;
    switch (adc_channel_state) {
        case 0:
            ambient_results[index] = ADCMEM0;
            adc_channel_state = 1;
            break;
        case 1:
            plant_results[index] = ADCMEM0;
            adc_channel_state = 2;
            break;
        case 2:
            uv_results[index] = ADCMEM0;
            adc_channel_state = 0;

            // Only increment index and process once all 3 have been sampled
            index++;
            if (index >= window_size) {
                index = 0;
                get_temperature();
                get_uv();
            }
            break;
    }

    ADCIFG &= ~ADCIFG0;
}

#pragma vector = USCI_B0_VECTOR
__interrupt void USCI_B0_ISR(void) {
    if (UCB0IV == 0x18) {
        if (tx_index < TX_BYTES) {
            UCB0TXBUF = tx_buffer[tx_index++];
        } else {
            UCB0CTLW0 |= UCTXSTP;
            UCB0IE &= ~UCTXIE0;
            tx_index = 0;
        }
    }
}
