#include <Arduino_FreeRTOS.h>

#define MASCON_1 47
#define MASCON_2 49
#define MASCON_3 51
#define MASCON_4 53

#define KEY_SWITCH    30
#define DEADMAN_RESET 31
#define BUZZER        32

#define ZERO_SPEED_PIN 28  

#define DIR_SWITCH 29    
#define RELAY_U    33    
#define RELAY_V    34    
#define RELAY_W    35    

#define LED_EB 2
#define LED_B8 3
#define LED_B7 4
#define LED_B6 5
#define LED_B5 6
#define LED_B4 7
#define LED_B3 8
#define LED_B2 9
#define LED_B1 10

#define LED_N  11
#define LED_P1 12
#define LED_P2 13 
#define LED_P3 48
#define LED_P4 50
#define LED_P5 52

#define BTN_UP   44 
#define BTN_DOWN 45 

const int DEADMAN_TIME = 1000 * 30;

int current_level = 14; 
int pre_level = -1;
unsigned long level_change_time = 0;
int last_dir_switch_state = -1; 

bool last_btn_up_state = HIGH;
bool last_btn_down_state = HIGH;

void setup() {
  Serial.begin(9600);

  pinMode(KEY_SWITCH, INPUT);
  pinMode(DEADMAN_RESET, INPUT);
  
  pinMode(ZERO_SPEED_PIN, INPUT); 
   
  pinMode(DIR_SWITCH, INPUT_PULLUP); 
  pinMode(RELAY_U, OUTPUT);
  pinMode(RELAY_V, OUTPUT);
  pinMode(RELAY_W, OUTPUT);

  digitalWrite(RELAY_U, LOW); 
  digitalWrite(RELAY_V, LOW);
  digitalWrite(RELAY_W, LOW);

  pinMode(MASCON_1, OUTPUT);
  pinMode(MASCON_2, OUTPUT);
  pinMode(MASCON_3, OUTPUT);
  pinMode(MASCON_4, OUTPUT);

  pinMode(LED_EB, OUTPUT);
  pinMode(LED_B8, OUTPUT);
  pinMode(LED_B7, OUTPUT);
  pinMode(LED_B6, OUTPUT);
  pinMode(LED_B5, OUTPUT);
  pinMode(LED_B4, OUTPUT);
  pinMode(LED_B3, OUTPUT);
  pinMode(LED_B2, OUTPUT);
  pinMode(LED_B1, OUTPUT);
  pinMode(LED_N,  OUTPUT);
  pinMode(LED_P1, OUTPUT);
  pinMode(LED_P2, OUTPUT);
  pinMode(LED_P3, OUTPUT);
  pinMode(LED_P4, OUTPUT);
  pinMode(LED_P5, OUTPUT);

  pinMode(BTN_UP, INPUT_PULLUP);
  pinMode(BTN_DOWN, INPUT_PULLUP);
}

void loop() {
  bool btn_up_state = digitalRead(BTN_UP);
  bool btn_down_state = digitalRead(BTN_DOWN);

  if (btn_up_state == LOW && last_btn_up_state == HIGH) {
    if (current_level < 14) {
      current_level++;
    }
    delay(30); 
  }
  
  if (btn_down_state == LOW && last_btn_down_state == HIGH) {
    if (current_level > 0) {
      current_level--;
    }
    delay(30);
  }

  last_btn_up_state = btn_up_state;
  last_btn_down_state = btn_down_state;

  if (current_level != pre_level) {
    pre_level = current_level;
    level_change_time = millis();
    
  }

  int current_dir_switch = digitalRead(DIR_SWITCH);
  int is_zero_speed = digitalRead(ZERO_SPEED_PIN); 

  if (current_dir_switch != last_dir_switch_state && is_zero_speed == HIGH) {
    last_dir_switch_state = current_dir_switch;

    digitalWrite(RELAY_U, LOW); 

    if (current_dir_switch == HIGH) {
      digitalWrite(RELAY_V, HIGH);
      digitalWrite(RELAY_W, HIGH);
    } else {
      digitalWrite(RELAY_V, LOW);
      digitalWrite(RELAY_W, LOW);
    }
  }

  if (millis() - level_change_time >= DEADMAN_TIME) {
    current_level = 14; 
  }

  mascon_led_control(current_level);
  throttle_led(current_level);

  delay(50); 
}

void throttle_led(int level) {
  switch (level) {
    case 0:  throttle_led_control(false, false, false, false, false, false, false, false, false, false, true, true, true, true, true); break;
    case 1:  throttle_led_control(false, false, false, false, false, false, false, false, false, false, true, true, true, true, false); break;
    case 2:  throttle_led_control(false, false, false, false, false, false, false, false, false, false, true, true, true, false, false); break;
    case 3:  throttle_led_control(false, false, false, false, false, false, false, false, false, false, true, true, false, false, false); break;
    case 4:  throttle_led_control(false, false, false, false, false, false, false, false, false, false, true, false, false, false, false); break;
    case 5:  throttle_led_control(false, false, false, false, false, false, false, false, false, true, false, false, false, false, false); break;
    case 6:  throttle_led_control(false, false, false, false, false, false, false, false, true, false, false, false, false, false, false); break;
    case 7:  throttle_led_control(false, false, false, false, false, false, false, true, true, false, false, false, false, false, false); break;
    case 8:  throttle_led_control(false, false, false, false, false, false, true, true, true, false, false, false, false, false, false); break;
    case 9:  throttle_led_control(false, false, false, false, false, true, true, true, true, false, false, false, false, false, false); break;
    case 10: throttle_led_control(false, false, false, false, true, true, true, true, true, false, false, false, false, false, false); break;
    case 11: throttle_led_control(false, false, false, true, true, true, true, true, true, false, false, false, false, false, false); break;
    case 12: throttle_led_control(false, false, true, true, true, true, true, true, true, false, false, false, false, false, false); break;
    case 13: throttle_led_control(false, true, true, true, true, true, true, true, true, false, false, false, false, false, false); break;
    case 14: throttle_led_control(true, true, true, true, true, true, true, true, true, false, false, false, false, false, false); break;
    default: throttle_led_control(true, true, true, true, true, true, true, true, true, true, true, true, true, true, true); break;
  }
}

void throttle_led_control(
  bool eb, bool b8, bool b7, bool b6, bool b5, 
  bool b4, bool b3, bool b2, bool b1, bool n, 
  bool p1, bool p2, bool p3, bool p4, bool p5
  ) {
  digitalWrite(LED_EB,  eb  ? HIGH : LOW);
  digitalWrite(LED_B8,  b8  ? HIGH : LOW);
  digitalWrite(LED_B7,  b7  ? HIGH : LOW);
  digitalWrite(LED_B6,  b6  ? HIGH : LOW);
  digitalWrite(LED_B5,  b5  ? HIGH : LOW);
  digitalWrite(LED_B4,  b4  ? HIGH : LOW);
  digitalWrite(LED_B3,  b3  ? HIGH : LOW);
  digitalWrite(LED_B2,  b2  ? HIGH : LOW);
  digitalWrite(LED_B1,  b1  ? HIGH : LOW);
  digitalWrite(LED_N,   n   ? HIGH : LOW);
  digitalWrite(LED_P1,  p1  ? HIGH : LOW);
  digitalWrite(LED_P2,  p2  ? HIGH : LOW);
  digitalWrite(LED_P3,  p3  ? HIGH : LOW);
  digitalWrite(LED_P4,  p4  ? HIGH : LOW);
  digitalWrite(LED_P5,  p5  ? HIGH : LOW);
}

void mascon_led_control(int level) {
  digitalWrite(MASCON_1, (level & 0x8) ? HIGH : LOW);
  digitalWrite(MASCON_2, (level & 0x4) ? HIGH : LOW);
  digitalWrite(MASCON_3, (level & 0x2) ? HIGH : LOW);
  digitalWrite(MASCON_4, (level & 0x1) ? HIGH : LOW);
}