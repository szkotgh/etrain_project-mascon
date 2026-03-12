//#include <Arduino_FreeRTOS.h>

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

const int DEADMAN_TIME = 1000 * 30; // 30초

const int level_thresholds[14] = {
  30, 85, 135, 190, 235, 290, 340, 455, 560, 670, 745, 805, 855, 925 
};

int pre_level = -1;
unsigned long level_change_time = 0; 
int last_dir_switch_state = -1;

// 텍스트 중복 출력을 방지하고 풀업 방식에 맞춘 상태 변수들
int last_reset_btn_state = HIGH; // 풀업 방식이므로 기본 상태가 HIGH입니다.
bool is_deadman_active = false;  

void setup() {
  // 시리얼 통신 시작 (보드레이트 9600)
  Serial.begin(9600);
  Serial.println("시스템 부팅 완료. 마스콘 제어를 시작합니다.");

  pinMode(KEY_SWITCH, INPUT);
  
  // FILN-22FJ 버튼에 맞춰 내부 풀업 저항 사용 (GND와 연결)
  pinMode(DEADMAN_RESET, INPUT_PULLUP); 
  
  pinMode(ZERO_SPEED_PIN, INPUT);
  
  // ZB2-BE101C 스위치 (GND와 연결)
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

  pinMode(LED_EB, OUTPUT); pinMode(LED_B8, OUTPUT); pinMode(LED_B7, OUTPUT);
  pinMode(LED_B6, OUTPUT); pinMode(LED_B5, OUTPUT); pinMode(LED_B4, OUTPUT);
  pinMode(LED_B3, OUTPUT); pinMode(LED_B2, OUTPUT); pinMode(LED_B1, OUTPUT);
  pinMode(LED_N,  OUTPUT); pinMode(LED_P1, OUTPUT); pinMode(LED_P2, OUTPUT);
  pinMode(LED_P3, OUTPUT); pinMode(LED_P4, OUTPUT); pinMode(LED_P5, OUTPUT);
}

void loop() {
  int inputValue = analogRead(A0);

  // 1. 아날로그 입력에 따른 레벨 결정
  int level = 14; 
  for (int i = 0; i < 14; i++) {
    if (inputValue <= level_thresholds[i]) {
      level = i;
      break; 
    }
  }

  // 2. 데드맨 리셋 버튼 조작 감지 및 텍스트 출력
  int current_reset_btn = digitalRead(DEADMAN_RESET);
  
  // 버튼이 안 눌려있다가(HIGH) 눌린 순간(LOW) 감지 (Falling Edge)
  if (current_reset_btn == LOW && last_reset_btn_state == HIGH) {
    Serial.println("[알림] 데드맨 리셋 버튼 조작: 타이머가 초기화되었습니다.");
    level_change_time = millis();
    is_deadman_active = false; // 데드맨 발동 상태 해제
  }
  last_reset_btn_state = current_reset_btn; // 이전 버튼 상태 업데이트

  // 3. 마스콘 레벨 변경 감지 및 타이머 초기화
  if (level != pre_level) {
    pre_level = level;
    level_change_time = millis();
    
    // 데드맨이 발동된 상태에서 마스콘을 조작해 해제된 경우 텍스트 출력
    if (is_deadman_active) {
      Serial.println("[알림] 마스콘 조작 감지: 데드맨 발동이 해제되었습니다.");
      is_deadman_active = false;
    }
  }

  // 4. 방향 스위치 체크 및 텍스트 출력
  int current_dir_switch = digitalRead(DIR_SWITCH);
  int is_zero_speed = digitalRead(ZERO_SPEED_PIN); 

  if (current_dir_switch != last_dir_switch_state && is_zero_speed == HIGH) {
    last_dir_switch_state = current_dir_switch;
    digitalWrite(RELAY_U, LOW); 

    if (current_dir_switch == HIGH) {
      // 스위치가 떨어져 있는 상태 (INPUT_PULLUP이라 HIGH)
      Serial.println("[방향] 진행 (Forward) 모드로 변경되었습니다.");
      digitalWrite(RELAY_V, HIGH);
      digitalWrite(RELAY_W, HIGH);
    } else {
      // 스위치 접점이 붙은 상태 (GND와 연결되어 LOW)
      Serial.println("[방향] 퇴행 (Backward) 모드로 변경되었습니다.");
      digitalWrite(RELAY_V, LOW);
      digitalWrite(RELAY_W, LOW);
    }
  }

  // 5. 데드맨 타이머 체크 (최종 레벨 강제 조정) 및 텍스트 출력
  if (millis() - level_change_time >= (unsigned long)DEADMAN_TIME) {
    level = 14; // 강제 비상 제동
    
    // 데드맨이 처음 발동되는 순간에만 텍스트 1회 출력
    if (!is_deadman_active) {
      Serial.println("!!! 경고 !!! 데드맨 발동: 30초 무조작으로 인해 비상제동(Level 14)이 체결됩니다.");
      is_deadman_active = true;
    }
  }

  // 6. 출력 제어
  mascon_led_control(level);
  throttle_led(level);

  delay(100);
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

void throttle_led_control(bool eb, bool b8, bool b7, bool b6, bool b5, bool b4, bool b3, bool b2, bool b1, bool n, bool p1, bool p2, bool p3, bool p4, bool p5) {
  digitalWrite(LED_EB, eb ? HIGH : LOW);
  digitalWrite(LED_B8, b8 ? HIGH : LOW);
  digitalWrite(LED_B7, b7 ? HIGH : LOW);
  digitalWrite(LED_B6, b6 ? HIGH : LOW);
  digitalWrite(LED_B5, b5 ? HIGH : LOW);
  digitalWrite(LED_B4, b4 ? HIGH : LOW);
  digitalWrite(LED_B3, b3 ? HIGH : LOW);
  digitalWrite(LED_B2, b2 ? HIGH : LOW);
  digitalWrite(LED_B1, b1 ? HIGH : LOW);
  digitalWrite(LED_N,  n  ? HIGH : LOW);
  digitalWrite(LED_P1, p1 ? HIGH : LOW);
  digitalWrite(LED_P2, p2 ? HIGH : LOW);
  digitalWrite(LED_P3, p3 ? HIGH : LOW);
  digitalWrite(LED_P4, p4 ? HIGH : LOW);
  digitalWrite(LED_P5, p5 ? HIGH : LOW);
}

void mascon_led_control(int level) {
  digitalWrite(MASCON_1, (level & 0x8) ? HIGH : LOW);
  digitalWrite(MASCON_2, (level & 0x4) ? HIGH : LOW);
  digitalWrite(MASCON_3, (level & 0x2) ? HIGH : LOW);
  digitalWrite(MASCON_4, (level & 0x1) ? HIGH : LOW);
}