/*
*코드 설명
조도 센서 값의 변화 범위 조건이 "바뀔 때만" 현 상태와 시간을 저장합니다. 매 초 저장하지 않습니다.
배열이 가득 찰 경우 맨 앞에서부터 덮어씁니다.
*가이드
이 코드는 배열 연습용 입니다. 간단한 세팅으로 센서와 시간을 활용한 작업을 할 수 있습니다.
Uno에서는 동작하나 WeMos에서는 동작하지 않고 추가 작업이 필요합니다.
구하기 쉽고 널리 쓰이는 조도센서를 사용했습니다.
이 코드는 메인 프로젝트의 <NTPClient.h> 라이브러리와는 다릅니다. 현재 시간을 불러오지 않습니다.
코드가 동작하지 않을 경우, 조도 센서 값의 범위를 확인하십시오.
*/
#include <TimeLib.h>

// 작동 기록 배열
bool isBright; // 상태 변화 제어용(toggle처럼 작동)
const int arraySize = 5; // 본인이 테스트하기 편하도록 작게 설정하였다. 사용자의 환경과 용도에 맞게 사이즈를 설정하면 된다.
String timeLog[arraySize];
int index = 0;

void setup() {
  Serial.begin(9600);
  setTime(0, 0, 0, 1, 1, 2024); // 초기 시간: 2024/1/1 00:00:00
  int sensorValue = analogRead(A0);
  saveLog(sensorValue);
  Serial.print(sensorValue);
  (sensorValue >= 100) ? isBright = true : isBright = false; // 최초 실행 시 조도 센서 상태 감지
}

void loop() {
  int sensorValue = analogRead(A0);
  // 조도 센서값 범위가 바뀔 때만 동작(아래 조건문이 없을 경우 센서값이 매 초 저장되는 문제가 있다.)
  if ((sensorValue >= 100 && !isBright) || (sensorValue < 100 && isBright)) {
    saveLog(sensorValue);
    Serial.print("sensorValue:");
    Serial.print(sensorValue);
    isBright = !isBright; // 상태 변화를 저장
  }
  delay(500);
}

void saveLog(int sensorValue) { // 예) [열림] 2024/1/1 12:20:15
  // String time = String(year())+"/"+String(month())+"/"+String(day())+" "+String(hour())+":"+String(minute())+":"+String(second());
  String time = String(minute())+":"+String(second()); // 분:초 형식이라 짧은 시간 단위 테스트에 간편하다.
  timeLog[index] = (sensorValue >= 100) ? "[bright]" + time : "[dark]" + time;
  Serial.println("");
  Serial.println("Time Log");
  for (int i = 0; i <= index; i++) {
    Serial.print(timeLog[i]);
    Serial.print(" | ");
  }
  index = (index + 1) % arraySize; // 배열이 가득차면 앞에서부터 덮어씌움(overflow)
}
