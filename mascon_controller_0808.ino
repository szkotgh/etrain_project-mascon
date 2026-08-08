//#include <Arduino_FreeRTOS.h>

#define KEY_SWITCH    30
#define DEADMAN_RESET 31
#define BUZZER        32

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
int last_reset_btn_state = HIGH; 
int last_key_switch_state = -1; // 키 스위치 이전 상태 저장
bool is_deadman_active = false;  

void setup() {
  Serial.begin(9600);
  Serial.println("SYSTEM:READY");

  // 키 스위치 핀 설정 (하드웨어 배선에 따라 INPUT 또는 INPUT_PULLUP)
  pinMode(KEY_SWITCH, INPUT_PULLUP);
  
  pinMode(DEADMAN_RESET, INPUT_PULLUP); 
  pinMode(DIR_SWITCH, INPUT_PULLUP); 
  
  pinMode(RELAY_U, OUTPUT);
  pinMode(RELAY_V, OUTPUT);
  pinMode(RELAY_W, OUTPUT);

  digitalWrite(RELAY_U, LOW); 
  digitalWrite(RELAY_V, LOW);
  digitalWrite(RELAY_W, LOW);

  pinMode(LED_EB, OUTPUT); pinMode(LED_B8, OUTPUT); pinMode(LED_B7, OUTPUT);
  pinMode(LED_B6, OUTPUT); pinMode(LED_B5, OUTPUT); pinMode(LED_B4, OUTPUT);
  pinMode(LED_B3, OUTPUT); pinMode(LED_B2, OUTPUT); pinMode(LED_B1, OUTPUT);
  pinMode(LED_N,  OUTPUT); pinMode(LED_P1, OUTPUT); pinMode(LED_P2, OUTPUT);
  pinMode(LED_P3, OUTPUT); pinMode(LED_P4, OUTPUT); pinMode(LED_P5, OUTPUT);
}

void loop() {
  // 1. 키 스위치 상태 감지
  int current_key_switch = digitalRead(KEY_SWITCH);

  // 키 스위치 상태 변경 시 시리얼 출력
  if (current_key_switch != last_key_switch_state) {
    last_key_switch_state = current_key_switch;
    if (current_key_switch == LOW) {
      Serial.println("KEY:ON");
      level_change_time = millis(); // 키를 켤 때 데드맨 타이머 초기화
    } else {
      Serial.println("KEY:OFF");
      turn_off_all_outputs(); // 키가 꺼지면 모든 릴레이/LED 오프
    }
  }

  // 2. 키 스위치가 꺼져있으면(LOW) 아래 제어 로직을 통과시키지 않음 (인터락)
  if (current_key_switch == HIGH) {
    delay(100);
    return;
  }

  // --- 이하 키 스위치가 ON(HIGH)일 때만 동작하는 구역 ---

  // 3. 아날로그 입력에 따른 레벨 결정
  int inputValue = analogRead(A0);
  int level = 14; 
  for (int i = 0; i < 14; i++) {
    if (inputValue <= level_thresholds[i]) {
      level = i;
      break; 
    }
  }

  // 4. 데드맨 리셋 버튼 조작 감지
  int current_reset_btn = digitalRead(DEADMAN_RESET);
  if (current_reset_btn == LOW && last_reset_btn_state == HIGH) {
    Serial.println("DEADMAN:RESET");
    level_change_time = millis();
    is_deadman_active = false;
  }
  last_reset_btn_state = current_reset_btn;

  // 5. 마스콘 레벨 변경 감지
  if (level != pre_level) {
    pre_level = level;
    level_change_time = millis();
    
    Serial.print("LEVEL:");
    Serial.println(level);
    
    if (is_deadman_active) {
      Serial.println("DEADMAN:RELEASED_BY_MASCON");
      is_deadman_active = false;
    }
  }

  // 6. 방향 스위치 체크 (Zero Speed 조건 제거 적용)
  int current_dir_switch = digitalRead(DIR_SWITCH);
  if (current_dir_switch != last_dir_switch_state) {
    last_dir_switch_state = current_dir_switch;
    digitalWrite(RELAY_U, LOW); 

    if (current_dir_switch == HIGH) {
      Serial.println("DIR:FORWARD");
      digitalWrite(RELAY_V, HIGH);
      digitalWrite(RELAY_W, HIGH);
    } else {
      Serial.println("DIR:BACKWARD");
      digitalWrite(RELAY_V, LOW);
      digitalWrite(RELAY_W, LOW);
    }
  }

  // 7. 데드맨 타이머 체크 (30초 무조작)
  if (millis() - level_change_time >= (unsigned long)DEADMAN_TIME) {
    level = 14; // 강제 비상 제동
    
    if (!is_deadman_active) {
      Serial.println("DEADMAN:TRIGGERED");
      Serial.print("LEVEL:");
      Serial.println(level);
      is_deadman_active = true;
    }
  }

  // 8. LED 출력 제어
  throttle_led(level);

  delay(100);
}

// 키 스위치가 OFF일 때 모든 릴레이와 LED를 끄는 전용 함수
void turn_off_all_outputs() {
  digitalWrite(RELAY_U, LOW);
  digitalWrite(RELAY_V, LOW);
  digitalWrite(RELAY_W, LOW);
  throttle_led_control(false, false, false, false, false, false, false, false, false, false, false, false, false, false, false);
  pre_level = -1; // 레벨 재초기화
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