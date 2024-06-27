/*
*코드 설명
웹 브라우저로 도어락에 명령을 내려서 서보모터로 도어락을 제어하며 활동은 센서로 감지합니다.
센서가 활동을 감지할 경우 사용 기록(OPEN/LOCK)과 사용 시간을 저장합니다.
웹페이지에서 사용 기록과 도어락 상태를 볼 수 있습니다. 새로고침해야 정보가 갱신됩니다.
사용 기록은 지정한 최대 수를 넘어가면 오래된 것부터 덮어씌워집니다.
보드에 고정 IP를 할당합니다. 이로 인해 보드의 재부팅, 코드 업로드 등으로 보드 IP가 바뀌지 않습니다.
*가이드
보드가 성공적으로 동작하면 시리얼 모니터에 보드의 IP가 출력됩니다. 출력된 IP뒤에 콜론(:)과 포트번호를 붙여 주소창에 입력하면 관리 페이지에 접속합니다. 예)123.45.6.78:100
포트포워딩에 성공하면 외부 네트워크에서 보드와 통신할 수 있습니다. 외부 접속 방법은 위처럼 공인IP뒤에 포트번호를 붙이면 됩니다.
포트포워딩 시 외부포트, 내부포트, 보드포트를 같게 하는 것을 권장합니다.
현재 설정은 기울기센서입니다. 기울기센서(0,1)과 근접,조도센서(0~1023) 중 사용 부품에 맞게 코드를 설정하십시오.
기울기센서는  digitalRead를 쓰며 출력값이 0,1 / 근접센서는 analogRead를 쓰며 출력값이 아날로그(0~1023) 입니다.
수정하기 수월하게 부품 코드를 가깝게 붙여두었습니다.
*/
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <NTPClient.h>
#include <WiFiUdp.h>
#include <Servo.h>

char SSID[] = "KT_GiGA_2G_E7BD";
char PASSWORD[] = "0bd80dg272"; // 비밀번호가 없을 경우 ""
int PORT_NUMBER = 100; // http 웹포트인 80으로 설정할 경우 주소 맨 뒤에 ":100"과 같은 포트번호를 붙이지 않아도 된다.
ESP8266WebServer server(PORT_NUMBER);

// 고정 IP 할당. 이 기능이 필요없다면 모두 지워도 무방. 지울 경우 아래의 WiFi.config(local_IP, ...)코드도 지워야한다.
// 명령 프롬프트에 ipconfig를 입력하면 연결된 인터넷 정보가 나온다. ip.., sub.., gate..에 사용자의 환경값을 넣어주면 된다.
int TARGET_IP = 100; // 보드의 ip. 이것을 고정하기 위해 Static IP 코드를 사용한다.
IPAddress local_IP(172, 30, 1, TARGET_IP); // IPv4 주소
IPAddress subnet(255, 255, 255, 0);   // 서브넷 마스크
IPAddress gateway(172, 30, 1, 254);   // 기본 게이트웨이

// IPAddress local_IP(ip1, ip2, ip3, TARGET_IP); // IPv4 주소
// IPAddress subnet(sub1, sub2, sub3, 0);        // 서브넷 마스크
// IPAddress gateway(gate1, gate2, gate3, 254);  // 기본 게이트웨이
// 아래 값은 고정. NTPClient에 제대로 접속하기 위해 사용. 이 코드가 없을 경우 현재 시간은 1970년으로 불러와진다.
IPAddress primaryDNS(8, 8, 8, 8);   // Google DNS
IPAddress secondaryDNS(8, 8, 4, 4); // Google DNS

Servo myservo;
const int servoPin = D8; // 서보 모터 핀
const int tiltPin = D7;  // 기울기 센서 핀
int boardLED = 2; // 보드 내장 LED 핀. *WeMos 보드는 내장 LED의 ON/OFF 출력이 반대.

// 현재 시간
WiFiUDP ntpUDP;
NTPClient timeClient(ntpUDP, "pool.ntp.org", 32400, 3600000);
String daysOfTheWeek[7] = {"일", "월", "화", "수", "목", "금", "토"};

// 작동 기록 배열
bool isLock; // 상태 변화 제어용(toggle처럼 작동)
const int ARRAY_SIZE = 5; // 테스트하기 편하도록 작게 설정하였다. 사용자의 환경과 용도에 맞게 사이즈를 설정하면 된다.
String timeLog[ARRAY_SIZE];
int logIndex = 0;
int numLogs = 0;

void setup() {
  // 핀
  Serial.begin(115200);
  pinMode(boardLED, OUTPUT);
  pinMode(tiltPin, INPUT_PULLUP); // INPUT_PULLUP은 보드 내부 저항 활성화. 이 기능을 사용하면 센서에 저항을 연결하지 않아도 된다.
  digitalWrite(boardLED, LOW);
  myservo.attach(servoPin, 500, 2400); // WeMos보드의 특성인지 각도가 작은폭으로 움직이는데 이를 보정.
  myservo.write(0);

  // 와이파이 접속
  WiFi.config(local_IP, gateway, subnet, primaryDNS, secondaryDNS); // 고정 IP 할당. 이 기능을 사용하지 않을 경우 위의 IPAddress도 지워야 한다.
  WiFi.begin(SSID, PASSWORD);
  Serial.println("");
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); 
    Serial.print(".");
  }
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP()); // 접속된 와이파이 주소 출력

  // 작동기록
  int sensorValue = digitalRead(tiltPin);
  // int sensorValue = analogRead(A0);
  saveLog(sensorValue);
  Serial.print(sensorValue);
  (sensorValue == 1) ? isLock = true : isLock = false; // 최초 실행 시 도어락 상태 감지
  // (sensorValue >= 100) ? isLock = true : isLock = false; // 최초 실행 시 도어락 상태 감지
  // 각 페이지에 접속 되었을때 실행할 함수
  server.on("/", handleRoot); // 기본 페이지
  server.on("/open", handleOpen); // 도어락 열림
  server.on("/lock", handleLock); // 도어락 잠금
  server.begin(); // 웹서버 시작
  Serial.println("HTTP server started");
}

unsigned long previousMillis = 0; // delay용 변수

void loop() {
  server.handleClient();
  unsigned long currentMillis = millis(); // 하드웨어 시간 생성
  if ((currentMillis - previousMillis >= 1000) || // 1초 delay
  (currentMillis < previousMillis)) { // overflow
    previousMillis = currentMillis;
    getTime();
    Serial.println(getTime());
    // loop문 안에서 센서값을 읽어올때 딜레이가 없으면 보드 연산이 부하된다. 이 경우 현재 시간을 제대로 읽어오지 못하기에 analogRead의 위치를 delay안으로 넣었다.
    int sensorValue = digitalRead(tiltPin);
    Serial.println(sensorValue);
    // int sensorValue = analogRead(A0);
    // 도어락 상태가 바뀔 때만 동작(아래 조건문이 없을 경우 센서값이 매 초 저장되는 문제가 있다.)
    if ((sensorValue == 1 && !isLock) || (sensorValue == 0 && isLock)) {
    // if ((sensorValue >= 100 && !isLock) || (sensorValue < 100 && isLock)) {
      saveLog(sensorValue);
      Serial.print("REGIST: ");
      Serial.print(sensorValue);
      isLock = !isLock;
    }
  }
}


void saveLog(int sensorValue) { // 도어락 사용 기록을 저장. 예) [lock] 2024/1/1 12:20:15
  String currentTime = getTime();
  timeLog[logIndex] = (sensorValue == 1) ? "[lock]" + currentTime : "[open]" + currentTime;
  // timeLog[logIndex] = (sensorValue >= 100) ? "[open]" + currentTime : "[lock]" + currentTime;
  Serial.println("Time Log");
  for (int i = 0; i <= logIndex; i++) {
    Serial.print(timeLog[i] + " | ");
  } // 배열이 가득차면 앞에서부터 덮어씌움(overflow)
  logIndex = (logIndex + 1) % ARRAY_SIZE; // 배열이 가득차면 앞에서부터 덮어씌움(overflow)
  if (numLogs < ARRAY_SIZE) numLogs++; // 아래 HTML LOG에 적용
}

String getTime() { // 현재 시간을 가져옴
  timeClient.update();
  time_t epochTime = timeClient.getEpochTime();
  struct tm *ptm = gmtime((time_t *)&epochTime);
  int currentYear = ptm->tm_year + 1900;
  int currentMonth = ptm->tm_mon + 1;
  int monthDay = ptm->tm_mday;
  String currentDate = String(currentYear) + "/" + String(currentMonth) + "/" + String(monthDay) + " " + timeClient.getFormattedTime(); // getFormattedTime: 00:00:00형식
  return currentDate;
}

void handleRoot() { // 메인 페이지 접속
  commonHTML("waiting order..");
}

void handleOpen() { // open 명령(페이지 접속)
  digitalWrite(boardLED, HIGH);
  myservo.write(90);
  commonHTML("OPEN");
  Serial.println("OPEN");
}

void handleLock() { // lock 명령(페이지 접속)
  digitalWrite(boardLED, LOW);
  myservo.write(0);
  commonHTML("LOCK");
 Serial.println("LOCK");
}

void commonHTML(String state) { // 각 페이지에 반복되는 텍스트를 묶음
  int sensorValue = digitalRead(tiltPin);
  // int sensorValue = analogRead(A0);
  String LOG = "";
    for (int i = 0; i < numLogs; i++) { // 사용자의 편의성을 위해 사용 기록을 최근순으로 나열(배열의 역순)
      int index = (logIndex - 1 - i + ARRAY_SIZE) % ARRAY_SIZE;
      LOG += "<h2>" + timeLog[index] + "</h2>";
    }
  String HTMLTEXT =
    "<h1>state: " + state + "</h1>" // 현재 도어락 작동 상태
    "<h1><a href=\"/open\">OPEN</a></h1>"
    "<h1><a href=\"/lock\">LOCK</a></h1>"
    // "<h1>sensorValue: " + String(sensorValue) + "</h1>" // 센서값 테스트용
    + LOG; // 사용 기록
  server.send(200, "text/html", HTMLTEXT);
}
