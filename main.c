#include <mega16a.h>
#define IS_ON_LED_OUT PORTC .0
#define ALARM_OUT PORTC .1
#define IS_SENSOR_PAUSED_LED PORTB.0

int PAUSE_TIME_AFTER_DISCARD_SECONDS = 5;

typedef enum {
  rmt_no_action, // Do nothing, prevents default behaviour of enums.
  rmt_turn_on,
  rmt_turn_off,
  rmt_discard_alarm,
} eRemoteAction;



bit alarm_triggered = 0; // If alarm is triggered, this will be 1 until alarm is discarded
bit is_silent_mode = 0; // A few seconds after alarm discard, the sensor input will be ignored.

void temporarily_pause_sesonr() { 
    // with prescale set to 1024 and clk frequency = 1MHz, timer counts almost 1ms each clock.
    int timer_bottom = (65 - PAUSE_TIME_AFTER_DISCARD_SECONDS) * 1000;
    is_silent_mode = 1;
    TCNT1 = timer_bottom;
    TCCR1B |= 0x05; // prescale set to 5 -> 1MHz / 1024 ~= 1ms   
}

void handle_remote_action(eRemoteAction rmt_action) {
  if (rmt_action == rmt_no_action)
    return;

  if (rmt_action == rmt_turn_on) {
    IS_ON_LED_OUT = 1;
  }
  if (rmt_action == rmt_turn_off) {
    IS_ON_LED_OUT = 0;
    ALARM_OUT = 0;
  }
  if (rmt_action == rmt_discard_alarm) {
    alarm_triggered = 0;
    temporarily_pause_sesonr();
  }
}



void main() {
#asm("sei") // Enable interrupt
  GICR = 0b11000000;  // Enabling interrupt 0
  DDRC = 0xff;
  PORTC = 0;
  DDRB = 0x1000000;
  PORTB |= 0x00;
  IS_ON_LED_OUT = 1;
  ALARM_OUT = 0;
  TIMSK |=(1<<TOIE1); // Enable Timer1 Overflow
  while (1) {
    eRemoteAction rmt_action = rmt_no_action;
    if (PINA .0 == 1) {
      rmt_action = rmt_turn_on;
    }

    if (PINA .1 == 1) {
      rmt_action = rmt_turn_off;
    }

    if (PINA .2 == 1) {
      rmt_action = rmt_discard_alarm;
    }

    if (PINA .3 == 1) {
      rmt_action = rmt_turn_on;
    }
    handle_remote_action(rmt_action);
    ALARM_OUT = (alarm_triggered && !is_silent_mode) ? 1 : 0;
    IS_SENSOR_PAUSED_LED = is_silent_mode ? 1 : 0;
  }
}

interrupt[2] void trigger_alarm() {
  if (IS_ON_LED_OUT == 1) {
    alarm_triggered = 1;
  }
}

interrupt [TIM1_OVF] void exit_silent_mode () {
    is_silent_mode = 0;
    TCCR1B=0x00; // turn off the timer
}