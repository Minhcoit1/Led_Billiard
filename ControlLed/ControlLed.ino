#include <EEPROM.h>

const int latchPin = 21;  // Pin connected to ST_CP of 74HC595
const int clockPin = 18;  // Pin connected to SH_CP of 74HC595
const int dataPin = 3;    // Pin connected to DS of 74HC595

// Pin của nút nhấn
const int buttonUpAPin = 6;    // Nút tăng A
const int buttonDownAPin = 7;  // Nút giảm A
const int buttonUpBPin = 8;    // Nút tăng B
const int buttonDownBPin = 9;  // Nút giảm B
const int buzzerPin = 16;  // Chân GPIO điều khiển còi bíp
volatile bool buzzerActive = false;  // Cờ báo còi đang kêu
volatile unsigned long buzzerStartTime = 0;  // Thời điểm bắt đầu kêu
const unsigned long buzzerDuration = 50;    // Thời gian kêu (ms)

// Giá trị lưu trữ số của hai bên A và B
volatile int scoreA = 0;  // Biến lưu giá trị hiện tại của A
volatile int scoreB = 0;  // Biến lưu giá trị hiện tại của B

volatile bool mosconiMode = false;
volatile bool countdownActive = false;
unsigned long countdownStartTime = 0;

unsigned long lastDebounceTimeAUp = 0;
unsigned long lastDebounceTimeADown = 0;
unsigned long lastDebounceTimeBUp = 0;
unsigned long lastDebounceTimeBDown = 0;
const unsigned long debounceDelay = 300;  // Thời gian chống dội nút (ms)

unsigned long pressStartTime = 0;
bool resetInProgress = false;
unsigned long lastReleaseTime = 0;  // Thời gian nhả nút cuối cùng

int countdownTime = 30;  // Đếm ngược từ 30 giây

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

void beepBuzzer(int duration) {
  digitalWrite(buzzerPin, LOW);  // Bật còi bíp
  delay(duration);                // Giữ còi trong thời gian duration (ms)
  digitalWrite(buzzerPin, HIGH);   // Tắt còi bíp
}


// Hàm hiển thị 4 số lên LED 7 đoạn
void displayNumber(int scoreA, int scoreB) {
  int tensA = scoreA / 10;  // Chữ số hàng chục của A
  int onesA = scoreA % 10;  // Chữ số hàng đơn vị của A
  int tensB = scoreB / 10;  // Chữ số hàng chục của B
  int onesB = scoreB % 10;  // Chữ số hàng đơn vị của B

  digitalWrite(latchPin, LOW);  // Hạ chân chốt để chuẩn bị gửi dữ liệu

  // Gửi dữ liệu cho LED 7 đoạn tương ứng với các chữ số
  shiftOut(dataPin, clockPin, MSBFIRST, digits[tensA]);
  shiftOut(dataPin, clockPin, MSBFIRST, digits[onesA]);
  shiftOut(dataPin, clockPin, MSBFIRST, digits[tensB]);
  shiftOut(dataPin, clockPin, MSBFIRST, digits[onesB]);

  digitalWrite(latchPin, HIGH); // Nâng chân chốt để hiển thị dữ liệu
}


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

// Hàm tắt hẳn tất cả LED
void turnOffAllLEDs() {
  digitalWrite(latchPin, LOW);
  shiftOut(dataPin, clockPin, MSBFIRST, 0x00);  // Tắt LED 7 đoạn
  shiftOut(dataPin, clockPin, MSBFIRST, 0x00);  // Tắt LED 7 đoạn
  shiftOut(dataPin, clockPin, MSBFIRST, 0x00);  // Tắt LED 7 đoạn
  shiftOut(dataPin, clockPin, MSBFIRST, 0x00);  // Tắt LED 7 đoạn
  digitalWrite(latchPin, HIGH);  // Nâng chân chốt để hoàn thành
}

// Hàm lưu giá trị vào EEPROM (sử dụng EEPROM.put)
void saveScoresToEEPROM() {
  int eepromScoreA, eepromScoreB;
  EEPROM.get(0, eepromScoreA);
  EEPROM.get(sizeof(scoreA), eepromScoreB);
  
  // Chỉ ghi lại nếu giá trị thay đổi
  if (eepromScoreA != scoreA) {
    EEPROM.put(0, scoreA);
  }
  if (eepromScoreB != scoreB) {
    EEPROM.put(sizeof(scoreA), scoreB);
  }
}

void loadScoresFromEEPROM() {
  EEPROM.get(0, scoreA);
  EEPROM.get(sizeof(scoreA), scoreB);
  
  // Kiểm tra nếu giá trị ngoài phạm vi hợp lệ (ví dụ EEPROM chưa được ghi lần đầu)
  if (scoreA < 0 || scoreA > 99) scoreA = 0;
  if (scoreB < 0 || scoreB > 99) scoreB = 0;
}

// Nhấp nháy tất cả LED
void blinkAllLEDs(int times) {
  for (int i = 0; i < times; i++) {
    displayNumber(88,88);  // Tất cả các LED bật
    delay(200);
    turnOffAllLEDs();        // Tắt tất cả các LED hoàn toàn
    delay(200);
  }
}

// ISR cho nút tăng A
void increaseScoreA() {
  if (millis() - lastDebounceTimeAUp > debounceDelay) {
    buzzerActive = true;  // Kích hoạt còi
    buzzerStartTime = millis();  // Lưu thời gian bắt đầu
    scoreA++;
    if (scoreA > 99) scoreA = 99;
    displayScore(scoreA, scoreB);
    lastDebounceTimeAUp = millis();
    saveScoresToEEPROM();  // Lưu giá trị sau mỗi lần thay đổi
    
  }
}

// ISR cho nút giảm A
void decreaseScoreA() {
  if (millis() - lastDebounceTimeADown > debounceDelay) {
    buzzerActive = true;  // Kích hoạt còi
    buzzerStartTime = millis();  // Lưu thời gian bắt đầu
    if (mosconiMode) {
      countdownActive = !countdownActive;  // Kết thúc đếm ngược
    } else {
      scoreA--;
      if (scoreA < 0) scoreA = 0;
      displayScore(scoreA, scoreB);
      lastDebounceTimeADown = millis();
      saveScoresToEEPROM();
      
    }
  }
}

// ISR cho nút tăng B
void increaseScoreB() {
  if (millis() - lastDebounceTimeBUp > debounceDelay) {
    buzzerActive = true;  // Kích hoạt còi
    buzzerStartTime = millis();  // Lưu thời gian bắt đầu
    // Kiểm tra xem có đang ở chế độ Mosconi hay không
    if (mosconiMode) {
      // Bắt đầu đếm ngược nếu đang ở chế độ Mosconi
      countdownTime = 30;  // Đặt lại thời gian đếm ngược về 30 giây
      countdownActive = true;
      countdownStartTime = millis();  // Ghi lại thời gian bắt đầu đếm ngược
      //mosconiMode = false;  // Tắt chế độ Mosconi sau khi bắt đầu đếm ngược
      displayNumber(0, countdownTime);  // Hiển thị thời gian đếm ngược
    } else {
      // Nếu không ở chế độ Mosconi, tăng điểm số
      scoreB++;
      if (scoreB > 99) scoreB = 99;
      displayScore(scoreA, scoreB);
      saveScoresToEEPROM();
    }
    
    lastDebounceTimeBUp = millis();  // Cập nhật thời gian cuối cùng nhấn nút
  }
}


// ISR cho nút giảm B
void decreaseScoreB() {
  if (millis() - lastDebounceTimeBUp > debounceDelay) {
    buzzerActive = true;  // Kích hoạt còi
    buzzerStartTime = millis();  // Lưu thời gian bắt đầu
    if (mosconiMode) {
      // Bắt đầu đếm ngược nếu đang ở chế độ Mosconi
        countdownTime = 45;  // Đặt lại thời gian đếm ngược về 30 giây
        countdownActive = true;
        countdownStartTime = millis();  // Ghi lại thời gian bắt đầu đếm ngược
        //mosconiMode = false;  // Tắt chế độ Mosconi sau khi bắt đầu đếm ngược
        displayNumber(0, countdownTime);  // Hiển thị thời gian đếm ngược
    }
    else if(millis() - lastDebounceTimeBDown > debounceDelay) {
      scoreB--;
      if (scoreB < 0) scoreB = 0;
      displayScore(scoreA, scoreB);
      lastDebounceTimeBDown = millis();
      saveScoresToEEPROM();
    }
    
    lastDebounceTimeBUp = millis();  // Cập nhật thời gian cuối cùng nhấn nút
  }
}

// Hàm kiểm tra Mosconi mode
void checkMosconiMode() {
  if (digitalRead(buttonUpBPin) == LOW && digitalRead(buttonDownBPin) == LOW) {
    blinkAllLEDs(3);  // Nhấp nháy LED 3 lần
    mosconiMode = !mosconiMode;  // Đảo ngược chế độ Mosconi
    countdownActive = false;  // Tắt đếm ngược khi thoát Mosconi mode
    if (!mosconiMode) {
      turnOffAllLEDs();  // Tắt LED khi thoát Mosconi mode
    }
  }
}

// Kiểm tra nút B+ để bắt đầu đếm ngược
void startCountdown() {
  if (mosconiMode && digitalRead(buttonUpBPin) == LOW && millis() - lastDebounceTimeBUp > debounceDelay) {
    countdownStartTime = millis();
    countdownActive = true;  // Bắt đầu đếm ngược
    lastDebounceTimeBUp = millis();  // Cập nhật thời gian nhấn
    buzzerActive = true;  // Kích hoạt còi
    buzzerStartTime = millis();  // Lưu thời gian bắt đầu
  }
}

// Kiểm tra nút B- để tăng mức đếm
void increaseCountdownTime() {
  if (mosconiMode && digitalRead(buttonDownBPin) == LOW && millis() - lastDebounceTimeBDown > debounceDelay) {
    lastDebounceTimeBDown = millis();  // Cập nhật thời gian nhấn
    buzzerActive = true;  // Kích hoạt còi
    buzzerStartTime = millis();  // Lưu thời gian bắt đầu
  }
}

// Hàm xử lý đếm ngược
void handleCountdown() {
  if (countdownActive) {
    int elapsed = (millis() - countdownStartTime) / 1000;
    int remainingTime = countdownTime - elapsed;

    if (remainingTime >= 0) {
      displayNumber(0, remainingTime);  // Hiển thị thời gian còn lại
    } else {
      countdownActive = false;  // Kết thúc đếm ngược
      displayNumber(0, 0);  // Tắt LED khi hết thời gian
      mosconiMode = false;  // Tắt chế độ Mosconi sau khi bắt đầu đếm ngược
      displayScore(scoreA, scoreB);
    }
  }
}


// Kiểm tra nút A- có được giữ trong 5 giây không
void checkResetButton() {
  if (digitalRead(buttonDownAPin) == LOW && !resetInProgress) {
    if (pressStartTime == 0) {
      pressStartTime = millis();
    } else if (millis() - pressStartTime > 5000) {
      // Nếu giữ nút trong 5 giây, reset điểm
      scoreA = 0;
      scoreB = 0;
      displayScore(scoreA, scoreB);
      saveScoresToEEPROM();
      resetInProgress = true;
      buzzerActive = true;  // Kích hoạt còi
      buzzerStartTime = millis();  // Lưu thời gian bắt đầu
    }
  } else if (digitalRead(buttonDownAPin) == HIGH) {
    // Đảm bảo khi nhả nút sẽ không xảy ra hành động giảm điểm số thêm 1
    if (millis() - lastReleaseTime > debounceDelay) {
      pressStartTime = 0;  // Đặt lại thời gian bắt đầu
      resetInProgress = false;
      lastReleaseTime = millis();  // Cập nhật thời gian nhả nút
    }
  }
}

void buzzerTask(void *pvParameters) {
  while (1) {
    if (buzzerActive) {
      digitalWrite(buzzerPin, HIGH);  // Bật còi
      if (millis() - buzzerStartTime > buzzerDuration) {
        digitalWrite(buzzerPin, LOW);  // Tắt còi sau thời gian định trước
        buzzerActive = false;  // Đặt lại trạng thái còi
      }
    }
    vTaskDelay(10 / portTICK_PERIOD_MS);  // Giảm tải CPU
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
  pinMode(buzzerPin, OUTPUT);  // Thiết lập chân còi bíp là OUTPUT
  digitalWrite(buzzerPin, LOW);  // Tắt còi bíp ban đầu

  loadScoresFromEEPROM();  // Đọc giá trị từ EEPROM khi khởi động
  displayScore(scoreA, scoreB);  // Hiển thị giá trị ban đầu

  // Thiết lập ngắt ngoài cho các nút nhấn
  attachInterrupt(digitalPinToInterrupt(buttonUpAPin), increaseScoreA, FALLING);
  attachInterrupt(digitalPinToInterrupt(buttonDownAPin), decreaseScoreA, FALLING);
  attachInterrupt(digitalPinToInterrupt(buttonUpBPin), increaseScoreB, FALLING);
  attachInterrupt(digitalPinToInterrupt(buttonDownBPin), decreaseScoreB, FALLING);

  xTaskCreate(buzzerTask, "Buzzer Task", 1024, NULL, 1, NULL);

}

void loop() {
  // Kiểm tra trạng thái nút A- để thực hiện reset
  checkResetButton();

  // Các phần xử lý khác (nếu có)
  checkMosconiMode();
  // Nếu ở chế độ Mosconi, nhấn B+ để bắt đầu đếm lại từ đầu và B- để tăng mức đếm
  //if (mosconiMode) {
  //  startCountdown();
  //  increaseCountdownTime();
  //}

  // Xử lý đếm ngược
  handleCountdown();
}
