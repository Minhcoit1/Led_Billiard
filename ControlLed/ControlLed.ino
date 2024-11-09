#include <EEPROM.h>

const int latchPin = 21;  // Pin connected to ST_CP of 74HC595
const int clockPin = 18;  // Pin connected to SH_CP of 74HC595
const int dataPin = 3;    // Pin connected to DS of 74HC595

// Pin của nút nhấn
const int buttonUpAPin = 6;    // Nút tăng A
const int buttonDownAPin = 7;  // Nút giảm A
const int buttonUpBPin = 8;    // Nút tăng B
const int buttonDownBPin = 9;  // Nút giảm B

// Giá trị lưu trữ số của hai bên A và B
volatile int scoreA = 0;  // Biến lưu giá trị hiện tại của A
volatile int scoreB = 0;  // Biến lưu giá trị hiện tại của B

unsigned long lastDebounceTimeAUp = 0;
unsigned long lastDebounceTimeADown = 0;
unsigned long lastDebounceTimeBUp = 0;
unsigned long lastDebounceTimeBDown = 0;
const unsigned long debounceDelay = 300;  // Thời gian chống dội nút (ms)

unsigned long pressStartTime = 0;
bool resetInProgress = false;

// Bảng mã bit cho các số từ 0-9 trên LED 7 đoạn
byte digits[] = {
  0b00111111, // 0
  0b00000110, // 1
  0b01011011, // 2
  0b01001111, // 3
  0b01100110, // 4
  0b01101101, // 5
  0b01111101, // 6
  0b00000111, // 7
  0b01111111, // 8
  0b01101111  // 9
};

// Hàm hiển thị số lên LED 7 đoạn
void displayScore(int scoreA, int scoreB) {
  int tensA = scoreA / 10;  // Chữ số hàng chục bên A
  int onesA = scoreA % 10;  // Chữ số hàng đơn vị bên A
  int tensB = scoreB / 10;  // Chữ số hàng chục bên B
  int onesB = scoreB % 10;  // Chữ số hàng đơn vị bên B

  digitalWrite(latchPin, LOW);  // Hạ chân chốt để chuẩn bị gửi dữ liệu

  // Gửi dữ liệu tương ứng với các chữ số
  shiftOut(dataPin, clockPin, MSBFIRST, digits[tensA]);
  shiftOut(dataPin, clockPin, MSBFIRST, digits[onesA]);
  shiftOut(dataPin, clockPin, MSBFIRST, digits[tensB]);
  shiftOut(dataPin, clockPin, MSBFIRST, digits[onesB]);

  digitalWrite(latchPin, HIGH); // Nâng chân chốt để hiển thị dữ liệu
}

// Hàm lưu giá trị vào EEPROM (sử dụng EEPROM.put)
void saveScoresToEEPROM() {
  EEPROM.put(0, scoreA);  // Lưu giá trị của A vào EEPROM tại địa chỉ 0
  EEPROM.put(sizeof(scoreA), scoreB);  // Lưu giá trị của B vào EEPROM tại địa chỉ tiếp theo
}

// Hàm đọc giá trị từ EEPROM (sử dụng EEPROM.get)
void loadScoresFromEEPROM() {
  EEPROM.get(0, scoreA);  // Đọc giá trị của A từ EEPROM
  EEPROM.get(sizeof(scoreA), scoreB);  // Đọc giá trị của B từ EEPROM
}

// ISR cho nút tăng A
void increaseScoreA() {
  if (millis() - lastDebounceTimeAUp > debounceDelay) {
    scoreA++;
    if (scoreA > 99) scoreA = 0;
    displayScore(scoreA, scoreB);
    lastDebounceTimeAUp = millis();
    saveScoresToEEPROM();  // Lưu giá trị sau mỗi lần thay đổi
  }
}

// ISR cho nút giảm A
void decreaseScoreA() {
  if (millis() - lastDebounceTimeADown > debounceDelay) {
    scoreA--;
    if (scoreA < 0) scoreA = 99;
    displayScore(scoreA, scoreB);
    lastDebounceTimeADown = millis();
    saveScoresToEEPROM();
  }
}

// ISR cho nút tăng B
void increaseScoreB() {
  if (millis() - lastDebounceTimeBUp > debounceDelay) {
    scoreB++;
    if (scoreB > 99) scoreB = 0;
    displayScore(scoreA, scoreB);
    lastDebounceTimeBUp = millis();
    saveScoresToEEPROM();
  }
}

// ISR cho nút giảm B
void decreaseScoreB() {
  if (millis() - lastDebounceTimeBDown > debounceDelay) {
    scoreB--;
    if (scoreB < 0) scoreB = 99;
    displayScore(scoreA, scoreB);
    lastDebounceTimeBDown = millis();
    saveScoresToEEPROM();
  }
}

void setup() {
  pinMode(latchPin, OUTPUT);
  pinMode(clockPin, OUTPUT);
  pinMode(dataPin, OUTPUT);

  pinMode(buttonUpAPin, INPUT_PULLUP);
  pinMode(buttonDownAPin, INPUT_PULLUP);
  pinMode(buttonUpBPin, INPUT_PULLUP);
  pinMode(buttonDownBPin, INPUT_PULLUP);

  loadScoresFromEEPROM();  // Đọc giá trị từ EEPROM khi khởi động
  displayScore(scoreA, scoreB);  // Hiển thị giá trị ban đầu

  // Thiết lập ngắt ngoài cho các nút nhấn
  attachInterrupt(digitalPinToInterrupt(buttonUpAPin), increaseScoreA, FALLING);
  attachInterrupt(digitalPinToInterrupt(buttonDownAPin), decreaseScoreA, FALLING);
  attachInterrupt(digitalPinToInterrupt(buttonUpBPin), increaseScoreB, FALLING);
  attachInterrupt(digitalPinToInterrupt(buttonDownBPin), decreaseScoreB, FALLING);
}

void loop() {
  // Kiểm tra xem nút A- có được giữ trong 5 giây không
  if (digitalRead(buttonDownAPin) == LOW && !resetInProgress) {
    if (pressStartTime == 0) {
      pressStartTime = millis();
    } else if (millis() - pressStartTime > 5000) {
      scoreA = 0;
      scoreB = 0;
      displayScore(scoreA, scoreB);
      saveScoresToEEPROM();
      resetInProgress = true;
    }
  } else if (digitalRead(buttonDownAPin) == HIGH) {
    pressStartTime = 0;
    resetInProgress = false;
  }
}
