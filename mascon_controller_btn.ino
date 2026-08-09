//#include <Arduino_FreeRTOS.h>

#define KEY_SWITCH    30
#define DEADMAN_RESET 31
#define BUZZER        32

#define DIR_SWITCH    29    
#define EMO_BUTTON    27 // EMO(비상정지) 버튼 입력 핀

// [임시 수정] 스로틀 대신 사용할 레벨 조절 버튼 핀 정의
#define BTN_LEVEL_UP   A0 // 레벨 증가 버튼
#define BTN_LEVEL_DOWN A1 // 레벨 감소 버튼

// 인버터 제어용 핀 정의 (4비트 바이너리 출력)
#define MASCON_1 47
#define MASCON_2 49
#define MASCON_3 51
#define MASCON_4 53

// 라즈베리파이 전송용 출력 핀 정의
#define RPI_SIGNAL_PIN 28 // 스위치 신호 바이패스 핀 (DIR_SWITCH 상태 그대로 출력)
#define EMO_RPI_PIN    26 // EMO 상태 전송 핀 (눌림: HIGH, 해제: LOW)

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

// 현재 스로틀 레벨 (0: EB ~ 14: P5)
int current_level = 0; // 초기값: EB(비상 제동)

int pre_level = -1;
unsigned long level_change_time = 0; 
int last_dir_switch_state = -1;
int last_reset_btn_state = HIGH; 
int last_key_switch_state = -1; 
int last_emo_state = HIGH;
bool is_deadman_active = false;  
bool is_coasting = false;

// 레벨 조절 버튼의 이전 상태 저장용
int last_up_btn_state = HIGH;
int last_down_btn_state = HIGH;

void update_rpi_signal(int level);
void mascon_led_control(int level);
void turn_off_all_outputs();
void throttle_led(int level);
void throttle_led_control(bool eb, bool b8, bool b7, bool b6, bool b5, bool b4, bool b3, bool b2, bool b1, bool n, bool p1, bool p2, bool p3, bool p4, bool p5);

void setup() {
  Serial.begin(9600);
  Serial.println("SYSTEM:READY");

  // 스위치 및 임시 버튼 입력 핀 설정 (내부 풀업 사용)
  pinMode(KEY_SWITCH, INPUT_PULLUP);
  pinMode(DEADMAN_RESET, INPUT_PULLUP); 
  pinMode(DIR_SWITCH, INPUT_PULLUP); 
  pinMode(EMO_BUTTON, INPUT_PULLUP);
  
  pinMode(BTN_LEVEL_UP, INPUT_PULLUP);
  pinMode(BTN_LEVEL_DOWN, INPUT_PULLUP);

  // 인버터 제어용 출력 핀 설정
  pinMode(MASCON_1, OUTPUT);
  pinMode(MASCON_2, OUTPUT);
  pinMode(MASCON_3, OUTPUT);
  pinMode(MASCON_4, OUTPUT);
  
  // 라즈베리파이용 출력 핀 설정
  pinMode(RPI_SIGNAL_PIN, OUTPUT);
  pinMode(EMO_RPI_PIN, OUTPUT);

  digitalWrite(RPI_SIGNAL_PIN, LOW);
  digitalWrite(EMO_RPI_PIN, LOW);

  pinMode(LED_EB, OUTPUT); pinMode(LED_B8, OUTPUT); pinMode(LED_B7, OUTPUT);
  pinMode(LED_B6, OUTPUT); pinMode(LED_B5, OUTPUT); pinMode(LED_B4, OUTPUT);
  pinMode(LED_B3, OUTPUT); pinMode(LED_B2, OUTPUT); pinMode(LED_B1, OUTPUT);
  pinMode(LED_N,  OUTPUT); pinMode(LED_P1, OUTPUT); pinMode(LED_P2, OUTPUT);
  pinMode(LED_P3, OUTPUT); pinMode(LED_P4, OUTPUT); pinMode(LED_P5, OUTPUT);

  mascon_led_control(0);
}

void loop() {
  // 0. EMO 스위치 상태 감지 (상시 체크)
  int current_emo = digitalRead(EMO_BUTTON);
  if (current_emo != last_emo_state) {
    last_emo_state = current_emo;
    if (current_emo == LOW) { // EMO 버튼 눌림
      digitalWrite(EMO_RPI_PIN, HIGH);
      Serial.println("EMO:ACTIVE");
    } else { // EMO 버튼 해제
      digitalWrite(EMO_RPI_PIN, LOW);
      Serial.println("EMO:RELEASED");
    }
  }

  // 1. 키 스위치 상태 감지
  int current_key_switch = digitalRead(KEY_SWITCH);

  if (current_key_switch != last_key_switch_state) {
    last_key_switch_state = current_key_switch;
    if (current_key_switch == LOW) {
      Serial.println("KEY:ON");
      level_change_time = millis(); 
    } else {
      Serial.println("KEY:OFF");
      turn_off_all_outputs(); 
    }
  }

  // 2. 키 스위치 OFF(HIGH) 인터락
  if (current_key_switch == HIGH) {
    delay(50);
    return;
  }

  // --- 키 스위치 ON(LOW) 동작 구역 ---

  // 3. 버튼 입력을 통한 레벨(0~14) 제어
  int up_btn = digitalRead(BTN_LEVEL_UP);
  int down_btn = digitalRead(BTN_LEVEL_DOWN);

  // 레벨 증가 버튼 처리 (LOW일 때 눌림)
  if (up_btn == LOW && last_up_btn_state == HIGH) {
    if (current_level < 14) {
      current_level++;
    }
    delay(20); // 채터링 방지용 바운스 대기
  }
  last_up_btn_state = up_btn;

  // 레벨 감소 버튼 처리 (LOW일 때 눌림)
  if (down_btn == LOW && last_down_btn_state == HIGH) {
    if (current_level > 0) {
      current_level--;
    }
    delay(20); // 채터링 방지용 바운스 대기
  }
  last_down_btn_state = down_btn;

  int level = current_level;

  // 4. 데드맨 리셋 버튼 감지
  int current_reset_btn = digitalRead(DEADMAN_RESET);
  if (current_reset_btn == LOW && last_reset_btn_state == HIGH) {
    Serial.println("DEADMAN:RESET");
    level_change_time = millis();
    is_deadman_active = false;
  }
  last_reset_btn_state = current_reset_btn;

  // 5. 마스콘 레벨 변경 감지 및 시리얼 출력
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

  // 6. 방향 스위치(DIR_SWITCH) 상태 감지
  int current_dir_switch = digitalRead(DIR_SWITCH);
  if (current_dir_switch != last_dir_switch_state) {
    last_dir_switch_state = current_dir_switch;
    if (current_dir_switch == HIGH) {
      Serial.println("DIR:FORWARD");
    } else {
      Serial.println("DIR:BACKWARD");
    }
  }

  // 7. 데드맨 타이머 체크 (30초 무조작 시 비상 제동 Level 0 적용)
  if (millis() - level_change_time >= (unsigned long)DEADMAN_TIME) {
    level = 0; // 강제 비상 제동 (EB)
    
    if (!is_deadman_active) {
      Serial.println("DEADMAN:TRIGGERED");
      Serial.print("LEVEL:");
      Serial.println(level);
      is_deadman_active = true;
    }
  }

  // 8. 라즈베리파이 신호 업데이트 (스위치 상태 직통 전달)
  update_rpi_signal(level);

  // 9. 출력 제어 (LED 및 인버터 4비트 바이너리 신호)
  throttle_led(level);
  mascon_led_control(level);

  delay(30);
}

// 인버터 4비트 바이너리 출력 제어 함수
void mascon_led_control(int level) {
  digitalWrite(MASCON_1, (level & 0x8) ? HIGH : LOW);
  digitalWrite(MASCON_2, (level & 0x4) ? HIGH : LOW);
  digitalWrite(MASCON_3, (level & 0x2) ? HIGH : LOW);
  digitalWrite(MASCON_4, (level & 0x1) ? HIGH : LOW);
}

// 라즈베리파이 신호 출력 제어 함수
void update_rpi_signal(int level) {
  int current_dir = digitalRead(DIR_SWITCH);
  digitalWrite(RPI_SIGNAL_PIN, current_dir);
}

// 키 스위치 OFF 시 전체 출력 초기화
void turn_off_all_outputs() {
  is_coasting = false;
  digitalWrite(RPI_SIGNAL_PIN, LOW);
  
  mascon_led_control(0);
  throttle_led_control(false, false, false, false, false, false, false, false, false, false, false, false, false, false, false);
  
  pre_level = -1;
  last_dir_switch_state = -1;
  current_level = 0; // 레벨 초기화
}

// [0~8: 제동 / 9: 중립 / 10~14: 가속] LED 매핑
void throttle_led(int level) {
  switch (level) {
    // [0~8] 브레이크 (제동 레벨이 높을수록 B1 -> EB 점등 확장)
    case 0: throttle_led_control(true, true, true, true, true, true, true, true, true, false, false, false, false, false, false); break; // EB
    case 1: throttle_led_control(false, true, true, true, true, true, true, true, true, false, false, false, false, false, false); break; // B8
    case 2: throttle_led_control(false, false, true, true, true, true, true, true, true, false, false, false, false, false, false); break; // B7
    case 3: throttle_led_control(false, false, false, true, true, true, true, true, true, false, false, false, false, false, false); break; // B6
    case 4: throttle_led_control(false, false, false, false, true, true, true, true, true, false, false, false, false, false, false); break; // B5
    case 5: throttle_led_control(false, false, false, false, false, true, true, true, true, false, false, false, false, false, false); break; // B4
    case 6: throttle_led_control(false, false, false, false, false, false, true, true, true, false, false, false, false, false, false); break; // B3
    case 7: throttle_led_control(false, false, false, false, false, false, false, true, true, false, false, false, false, false, false); break; // B2
    case 8: throttle_led_control(false, false, false, false, false, false, false, false, true, false, false, false, false, false, false); break; // B1

    // [9] 중립
    case 9: throttle_led_control(false, false, false, false, false, false, false, false, false, true, false, false, false, false, false); break; // N

    // [10~14] 가속 (가속 레벨이 높을수록 P1 -> P5 점등 확장)
    case 10: throttle_led_control(false, false, false, false, false, false, false, false, false, false, true, false, false, false, false); break; // P1
    case 11: throttle_led_control(false, false, false, false, false, false, false, false, false, false, true, true, false, false, false); break; // P2
    case 12: throttle_led_control(false, false, false, false, false, false, false, false, false, false, true, true, true, false, false); break; // P3
    case 13: throttle_led_control(false, false, false, false, false, false, false, false, false, false, true, true, true, true, false); break; // P4
    case 14: throttle_led_control(false, false, false, false, false, false, false, false, false, false, true, true, true, true, true); break; // P5

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