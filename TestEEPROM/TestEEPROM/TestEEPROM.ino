#include <EEPROM.h>

#define EEPROM_SIZE 1      // Chỉ cần lưu 1 byte cho trạng thái còi
#define BUZZER_PIN 16      // Chân kết nối với còi
#define BUTTON_PIN 6       // Chân kết nối với nút nhấn

bool buzzerState = false;  // Trạng thái còi
bool lastButtonState = HIGH; // Trạng thái trước đó của nút nhấn (HIGH khi chưa nhấn)

// Hàm cập nhật trạng thái còi
void updateBuzzer() {
  digitalWrite(BUZZER_PIN, buzzerState ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);

  // Cấu hình chân nút nhấn và còi
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  // Khởi tạo EEPROM
  if (!EEPROM.begin(EEPROM_SIZE)) {
    Serial.println("Failed to initialize EEPROM");
    while (true);
  }

  // Đọc trạng thái còi từ EEPROM
  buzzerState = EEPROM.read(0);
  Serial.print("Buzzer state restored from EEPROM: ");
  Serial.println(buzzerState ? "ON" : "OFF");

  // Cập nhật trạng thái còi theo giá trị đã lưu
  updateBuzzer();
}

void loop() {
  bool buttonState = digitalRead(BUTTON_PIN);

  // Phát hiện cạnh xuống của nút nhấn (nhấn xuống)
  if (buttonState == LOW && lastButtonState == HIGH) {
    delay(50); // Chống dội nút
    if (digitalRead(BUTTON_PIN) == LOW) {
      buzzerState = !buzzerState; // Đảo trạng thái còi
      updateBuzzer();

      // Lưu trạng thái mới vào EEPROM
      EEPROM.write(0, buzzerState);
      EEPROM.commit(); // Ghi dữ liệu vào flash
      Serial.print("Buzzer state saved to EEPROM: ");
      Serial.println(buzzerState ? "ON" : "OFF");
    }
  }

  // Cập nhật trạng thái nút nhấn
  lastButtonState = buttonState;

  delay(10); // Giảm tải CPU
}
