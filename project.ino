#include <stdio.h>
#include <stdlib.h>
#include <MFRC522.h> 
#include <SPI.h>

/*global variable*/
#define SS_PIN 10 
#define RST_PIN 9 

MFRC522 mfrc522(SS_PIN, RST_PIN); // MFRC522 인스턴스 생성 
MFRC522 rc522(SS_PIN, RST_PIN);

struct _UID { /*카드 정보를 UID로 접근해 저장해둡니다*/
  int uid[4];
  int att;
  int def;
  int typ;
}card_UID[10] = { 0xC7,0x90,0x9F,0x2A, 30, 50, 3,
0xB7,0x33,0xCE,0x2A, 10, 70, 3,
0x97,0x3E,0xCB,0x2A, 20, 30, 3,
0xD7,0x47,0x9F,0x2A, 30, 70, 2,
0x77,0x06,0xAF,0x2A, 35, 65, 2,
0xB7,0xF0,0xB5,0x2A, 65, 100, 2,
0x77,0x00,0xCB,0x2A, 80, 40, 1,
0xB7,0x15,0xB6,0x2A, 35, 80, 1,
0x67,0xAE,0xAE,0x2A, 20, 120, 1,
0x47,0xF4,0xAE,0x2A, 65, 65, 1 };

typedef struct _player {/*게임 중 플레이어 데이터*/
  int card_attack[5];
  int card_defense[5];
  int cardtype[5];/* 3가지 값만 넣을 것임 1 -> fire / 2 -> water / 3 -> plant */
}player;

/*player playerA = { { 2,3,4,1,5 },{ 2,2,1,4,1 },{ 0,2,1,2,0 } };카드에서 정보 읽어와 넣는 것으로 추후 수정할 것*/
/*player playerB = { { 4,1,2,2,3 },{ 1,3,1,2,1 },{ 1,0,1,0,2 } };*/

player playerA = {0,};
player playerB = {0,};

int phase = 1; /*턴 수 == 페이즈 (동일개념), 홀수면 A의 턴이고 짝수면 B의 턴*/

int tempButtonStatus = HIGH;         
int buttonStatus;             
long lastDebounceTime = 0;  
long debounceWaitSpeed = 50;    
long momentTime;

/*constant variable*/
const int buttonPinfiled = 3;  
const int buttonPinset = 2;  
const int ARedPin = A0;  
const int AGreenPin = A1;
const int ABluePin = A2;
const int BRedPin = A3;  
const int BGreenPin = A4;
const int BBluePin = A5;
   

/*function parameter */
void AcolorRGB(int, int, int);/*LED켜기*/
void BcolorRGB(int, int, int);/*LED켜기*/
void game();/*메인 게임 시스템 함수, 공격*/
int Ais_full();/*A필드가 꽉 차있는지 확인하는 함수*/
int Bis_full();/*B필드가 꽉 차있는지 확인하는 함수*/
void Ais_die();/*A의 필드 카드가 다 죽었는지 확인하는 함수*/
void Bis_die();/*B의 필드 카드가 다 죽었는지 확인하는 함수*/
void card_set();/*카드 등록 함수*/
int attackfiled();/*공, 수 필드 위치를 1, 2, 3, 4, 5 로 반환하는 함수*/
int aLEDstatus(int aLEDnum);/* A플레이어 방향의 LED임. 0을 받으면 파란색, 1을 받으면 빨간색, 2를 받으면 노란색 점등부탁, -1은 off*/
int bLEDstatus(int bLEDnum);/* B플레이어 방향의 LED임*/
              /*LED가 노란색으로 점등되면, 시리얼 통신으로 필드 번호를 입력
              ex) 내가 A플레이어고, 이번턴이 첫 번째 턴이면 A플레이어에게 가까운 쪽 LED가 푸르게 점등됐다가,
              노랗게 점등될 것임. 노란 점등 상태에서 내가 공격에 쓸 필드(카드)를 지정 (시리얼 통신 필드 번호 입력)
              직후 B플레이어에 가까운 LED가 노랗게 점등 될 것임. 내가 공격을 할 상대 필드(카드)를 지정 (역시 번호 입력)*/

/*void  setup */              
void setup() {
  pinMode(buttonPinfiled, INPUT);
  pinMode(buttonPinset, INPUT);
  pinMode(ARedPin, OUTPUT);
  pinMode(ABluePin, OUTPUT);
  pinMode(AGreenPin, OUTPUT);
  pinMode(BRedPin, OUTPUT);
  pinMode(BBluePin, OUTPUT);
  pinMode(BGreenPin, OUTPUT);
  digitalWrite(buttonPinfiled, tempButtonStatus);
  digitalWrite(buttonPinset, tempButtonStatus);

  Serial.begin(9600);
  SPI.begin(); // SPI 버스 사용을 위한 초기화를 합니다 
  mfrc522.PCD_Init(); // MFRC522를 초기화 합니다. 
  rc522.PCD_Init();
}


/*void main*/
void loop() {

  int i=0;
  
  card_set();/*필드에 카드를 셋팅합니다. 처음부터 5장씩 깔고 가는 방식*/
  
  if(Ais_full() && Bis_full()){
    Serial.println("Reading Success.");
    for (phase = 1; phase < 19; phase++) { /*총 18페이즈, 플레이어 당 9턴씩 게임합니다.*/
      Ais_die();
      Bis_die();
      game();
    }
  }
  else {
    Serial.println("Reading Fail.");
    Serial.println("_____exit_____");
   }
}

void AcolorRGB(int R, int G, int B)
{
  analogWrite(ARedPin, R);
  analogWrite(AGreenPin, G);
  analogWrite(ABluePin, B);
}

void BcolorRGB(int R, int G, int B)
{
  analogWrite(BRedPin, R);
  analogWrite(BGreenPin, G);
  analogWrite(BBluePin, B);
}

int aLEDstatus(int aLEDnum) {
  if (aLEDnum == 0) {                  //blue
    AcolorRGB(0, 0, 255);
  }
  else if (aLEDnum == 1) {           //red
    AcolorRGB(255, 0, 0);
  }
  else if (aLEDnum == 2) {            //yellow
    AcolorRGB(150, 150, 70);
  }
  else if (aLEDnum == -1) {             //turn off
    AcolorRGB(0, 0, 0);
  }
  else {
    AcolorRGB(255, 255, 255);       //white
  }
}

int bLEDstatus(int bLEDnum) {
  if (bLEDnum == 0) {      //blue
    BcolorRGB(0, 0, 255);
  }
  else if (bLEDnum == 1) { //red
    BcolorRGB(255, 0, 0);
  }
  else if (bLEDnum == 2) { //yellow
    BcolorRGB(150, 150, 70);
  }
  else if (bLEDnum == -1) { //turn off
    BcolorRGB(0, 0, 0);
  }
  else {     //white
    BcolorRGB(255, 255, 255);
  }
}

int Ais_full(){
   int count = 0;

   for (int i = 0; i < 5; i++)
   {
      if (playerA.card_attack[i] == NULL && playerA.card_defense[i] == NULL && playerA.cardtype[i]== NULL )
      {
         count++;
         continue;
      }
   }
   if (count == 0) return 1;
   else return 0;
}

void Ais_die(){
   int count = 0;
   for (int i = 0; i < 5; i++)
   {
      if (playerA.card_attack[i] == 0 && playerA.card_defense[i] == 0 && playerA.cardtype[i]== 0 )
      {
         count++;
         continue;
      }
   }
   if (count == 5){
    Serial.println("B player's victory!"); /*A필드 카드가 전부 0 (die) 됐으므로 B플레이어의 승리*/
   }
}

int Bis_full(){
   int count = 0;

   for (int i = 0; i < 5; i++)
   {
      if (playerB.card_attack[i] == NULL && playerB.card_defense[i] == NULL && playerB.cardtype[i]== NULL )
      {
         count++;
         continue;
      }
   }
   if (count == 0) return 1;
   else return 0;
}

void Bis_die(){
   int count = 0;
   for (int i = 0; i < 5; i++)
   {
      if (playerB.card_attack[i] == 0 && playerB.card_defense[i] == 0 && playerB.cardtype[i]== 0 )
      {
         count++;
         continue;
      }
   }
   if (count == 5){
    Serial.println("A player's victory!");  
   }
}

void card_set(){
  int i=0;
  int j=0;
  aLEDstatus(2);/*양쪽 다 노란색점등*/
  bLEDstatus(2);
  Serial.println("You enter the card registration process!");
  Serial.println("A player's cards registe in order.");
  delay(2000);
  for(i=0;i<10;i++){/*카드의 UID확인해서 등록되어 있는 카드면 필드에 배치하는 개념의 일을 합니다*/
    for(j=0;j<5;j++){
      if(!mfrc522.PICC_IsNewCardPresent());     // Look for new cards
      if(!mfrc522.PICC_ReadCardSerial());
      if (mfrc522.uid.uidByte[0]==card_UID[i].uid[0] && mfrc522.uid.uidByte[1]==card_UID[i].uid[1] && mfrc522.uid.uidByte[2]==card_UID[i].uid[2] && mfrc522.uid.uidByte[3]==card_UID[i].uid[3]){
        playerA.card_attack[j] = card_UID[i].att;
        playerA.card_defense[j] = card_UID[i].def;
        playerA.cardtype[j] = card_UID[i].typ; 
        Serial.print(card_UID[i].att);
        Serial.print("  ");
        Serial.print(card_UID[i].def);
        Serial.print("  ");
        Serial.print(card_UID[i].typ);
        Serial.print("  ");
        Serial.print(playerA.card_attack[j]);
        Serial.println(" ");
        delay(2000);
        if (j<4){
        Serial.println("Touch new card!");
        delay(4000);
        }
      }
      else{
      rc522.PICC_HaltA(); 
      }
    }
  }

  if (Ais_full()){
    Serial.println("A : card registe success!");
  }

  Serial.println("You enter the card registration process!");
  Serial.println("B player's cards registe in order.");
  delay(2000);
  for(i=0;i<10;i++){/*카드의 UID확인해서 등록되어 있는 카드면 필드에 배치하는 개념의 일을 합니다*/
    for(j=0;j<5;j++){
      if(!mfrc522.PICC_IsNewCardPresent());     // Look for new cards
      if(!mfrc522.PICC_ReadCardSerial());
      if (mfrc522.uid.uidByte[0]==card_UID[i].uid[0] && mfrc522.uid.uidByte[1]==card_UID[i].uid[1] && mfrc522.uid.uidByte[2]==card_UID[i].uid[2] && mfrc522.uid.uidByte[3]==card_UID[i].uid[3]){
        playerB.card_attack[j] = card_UID[i].att;
        playerB.card_defense[j] = card_UID[i].def;
        playerB.cardtype[j] = card_UID[i].typ; 
        Serial.print(card_UID[i].att);
        Serial.print("  ");
        Serial.print(card_UID[i].def);
        Serial.print("  ");
        Serial.print(card_UID[i].typ);
        Serial.print("  ");
        Serial.print(playerB.card_attack[j]);
        Serial.println(" ");
        delay(2000);
        if (j<4){
        Serial.println("Touch new card!");
        delay(4000);
        }
      }
      else{
       rc522.PICC_HaltA();
      } 
    }
  }

  if (Bis_full()){
    Serial.println("B : card registe success!");
  }
}

int attackfiled() {
 char s_num = NULL;

 Serial.println("__Please specify field number__");
 delay(5000);
 s_num = Serial.read();/*시리얼에서 공격할 필드 번호를 받아옵니다*/
  
  if (s_num-'0' > -1 && s_num-'0' < 6) {
    Serial.print("input: ");
    Serial.println(s_num);
    return (s_num-'0')-1;
    }
   else{
    return -1;
   }
}

void game() {/*phase가 0이면 A의 턴, 1이면 B의 턴*/
  int _ABT = -1;/*attackfiled*/
  int _DBT = -1;/*attackfiled 함수를 이용하지만, defens로 취급할 것임*/
  int filedstatus = 0;/*공격받은 필드의 상태*/

  Serial.print("phase : ");
  Serial.println(phase);

  Serial.println("                            A");
  Serial.print("__ATTACK__");
  Serial.print("     ");
  Serial.print(playerA.card_attack[0]);
  Serial.print("     |   ");
  Serial.print(playerA.card_attack[1]);
  Serial.print("     |   ");
  Serial.print(playerA.card_attack[2]);
  Serial.print("     |   ");
  Serial.print(playerA.card_attack[3]);
  Serial.print("     |   ");
  Serial.print(playerA.card_attack[4]);
  Serial.println("     |   ");

  Serial.print("__DEFENSE_");
  Serial.print("     ");
  Serial.print(playerA.card_defense[0]);
  Serial.print("     |   ");
  Serial.print(playerA.card_defense[1]);
  Serial.print("     |   ");
  Serial.print(playerA.card_defense[2]);
  Serial.print("     |   ");
  Serial.print(playerA.card_defense[3]);
  Serial.print("     |   ");
  Serial.print(playerA.card_defense[4]);
  Serial.println("     |   ");

  Serial.println("    ");
  Serial.print("__ATTACK__");
  Serial.print("     ");
  Serial.print(playerB.card_attack[0]);
  Serial.print("     |   ");
  Serial.print(playerB.card_attack[1]);
  Serial.print("     |   ");
  Serial.print(playerB.card_attack[2]);
  Serial.print("     |   ");
  Serial.print(playerB.card_attack[3]);
  Serial.print("     |   ");
  Serial.print(playerB.card_attack[4]);
  Serial.println("     |   ");

  Serial.print("__DEFENSE_");
  Serial.print("     ");
  Serial.print(playerB.card_defense[0]);
  Serial.print("     |   ");
  Serial.print(playerB.card_defense[1]);
  Serial.print("     |   ");
  Serial.print(playerB.card_defense[2]);
  Serial.print("     |   ");
  Serial.print(playerB.card_defense[3]);
  Serial.print("     |   ");
  Serial.print(playerB.card_defense[4]);
  Serial.println("     |   ");
  Serial.println("                            B");
 

  if (phase % 2 == 1) {/*A 플레이어의 공격 페이즈 - 홀수 턴*/

    aLEDstatus(0);/*A플레이어 LED를 푸르게 점등 - 이번 턴에 공격할 것을 알려줌*/
    bLEDstatus(1);/*B플레이어 LED를 붉게 점등 - 이번 턴에 공격 받을 것을 알려줌*/
    aLEDstatus(2);/*A의 공격 주체 필드 지정 알림 점등 - a 노란색*/
    while (_ABT == -1){
    delay(100);
    _ABT = attackfiled();
    }/*버튼 반환값 받아옴, 필드 번호 지정용*/
    aLEDstatus(0);
    delay(3000);

    if (_ABT > -1 && _ABT < 6) {/*1,2,3,4,5번 필드 내에서만*/
      while (_DBT == -1){
      bLEDstatus(2);/*상대 필드 지정 알림 점등 - b 노란색*/
      delay(100);
      _DBT = attackfiled();
      }
      bLEDstatus(1);
      filedstatus = (playerB.card_defense[_DBT]) - (playerA.card_attack[_ABT]);/*상대 _DBT번째 필드에 공격하는 개념*/

      if (filedstatus <= 0) { /*0이거나 음수면 카드가 죽은 것 (파괴됨) ,
                  오버플로우 고려 안 한 이유는 모든 변수값이 세자리를 안 넘기 때문*/
        playerB.card_attack[_DBT] = 0; /*정보 0은 죽은 카드 취급 할 것 (NULL로 날리면 오류가능성 있어서)*/
        playerB.card_defense[_DBT] = 0;
        playerB.cardtype[_DBT] = 0;
      }

      else {
        playerB.card_defense[_DBT] = filedstatus; /*공격받고 남은 체력으로 업데이트*/
      } /*A 공격 끝남*/

    }
    else {/*오류구문*/
      Serial.println("The field value is not valid!");
      Serial.println("_____exit_____");
    }
  }

  else if (phase % 2 == 0) {/*B 플레이어의 공격 페이즈 - 짝수 턴*/

    bLEDstatus(0);/*B플레이어 LED를 푸르게 점등 - 이번 턴에 공격할 것을 알려줌*/
    aLEDstatus(1);/*A플레이어 LED를 붉게 점등 - 이번 턴에 공격 받을 것을 알려줌*/
    bLEDstatus(2);/*B의 공격 주체 필드 지정 알림 점등 - B 노란색*/
    while (_ABT == -1){
    delay(100);
    _ABT = attackfiled();
    }/*버튼 반환값 받아옴, 필드 번호 지정용*/
    bLEDstatus(0);
    delay(3000);

    if (_ABT > -1 && _ABT < 6) {/*1,2,3,4,5번 필드 내에서만*/
      aLEDstatus(2);/*상대 필드 지정 알림 점등 - A 노란색*/
      while (_DBT == -1){
      delay(100);
      _DBT = attackfiled();
      }
      aLEDstatus(1);
      filedstatus = (playerA.card_defense[_DBT]) - (playerB.card_attack[_ABT]);/*상대 _DBT번째 필드에 공격하는 개념*/

      if (filedstatus <= 0) { /*0이거나 음수면 카드가 죽은 것 (파괴됨) ,
                  오버플로우 고려 안 한 이유는 모든 변수값이 세자리를 안 넘기 때문*/
        playerA.card_attack[_DBT] = 0; /*정보 0은 죽은 카드 취급 할 것 (NULL로 날리면 오류가능성 있어서)*/
        playerA.card_defense[_DBT] = 0;
        playerA.cardtype[_DBT] = 0;
      }

      else {
        playerA.card_defense[_DBT] = filedstatus; /*공격받고 남은 체력으로 업데이트*/
      } /*B 공격 끝남*/

    }
    else {/*오류구문*/
      Serial.println("The field value is not valid!");
      Serial.println("_____exit_____");
    }
  }
  else {/*오류구문*/
    Serial.println("attack button input error!");
    Serial.println("_____exit_____");
  }
  aLEDstatus(-1);/*한 턴 끝남!! LED 다 꺼줌*/
  bLEDstatus(-1);
  delay(5000);

}
