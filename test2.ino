#include <Servo.h>
#include <SoftwareSerial.h>
#include <Wire.h>
#include <DHT.h>
#include <MsTimer2.h>

#define DEBUG

#define ARR_CNT 5
#define CMD_SIZE 60
char sendBuf[CMD_SIZE];
char recvId[10] = "KSH_SQL";  // SQL 저장 클라이이언트 ID
unsigned int secCount;
int angle = 0;

SoftwareSerial BTSerial(10, 11); // RX ==>BT:TXD, TX ==> BT:RXD
Servo myservo1;
Servo myservo2;

void setup()
{
#ifdef DEBUG
  Serial.begin(115200);
  Serial.println("setup() start!");
#endif
 
  BTSerial.begin(9600); 
  myservo1.attach(6);
  myservo2.attach(9);
  myservo1.write(angle);
  myservo2.write(angle);
}

void loop()
{
  if (BTSerial.available())
    bluetoothEvent();

#ifdef DEBUG
  if (Serial.available())
    BTSerial.write(Serial.read());
#endif
}
void bluetoothEvent()
{
  int i = 0;
  char * pToken;
  char * pArray[ARR_CNT] = {0};
  char recvBuf[CMD_SIZE] = {0};
  int len = BTSerial.readBytesUntil('\n', recvBuf, sizeof(recvBuf) - 1);

#ifdef DEBUG
  Serial.print("Recv : ");
  Serial.println(recvBuf);
#endif

  pToken = strtok(recvBuf, "[@]");
  while (pToken != NULL)
  {
    pArray[i] =  pToken;
    if (++i >= ARR_CNT)
      break;
    pToken = strtok(NULL, "[@]");
  }
  
  if (!strcmp(pArray[1], "SERVO")) {
    sprintf(sendBuf, "[%s]%s\n", pArray[0], pArray[1]);
    for(int i=0;i<10;i++){
      myservo1.write(angle+180);
      myservo2.write(angle+180);
      delay(2000);
      myservo1.write(angle);
      myservo2.write(angle);
      delay(2000);
    }  
  }
  else if (!strncmp(pArray[1], " New", 4)) // New Connected
  {
    return ;
  }
  else if (!strncmp(pArray[1], " Alr", 4)) //Already logged
  {
    return ;
  }
  else{
    return ;
  }
#ifdef DEBUG
  Serial.print("Send : ");
  Serial.print(sendBuf);
#endif
  BTSerial.write(sendBuf);
}

