#include <FS.h>
#include <SoftwareSerial.h>



// set up a new serial object
#define rxPin 12
#define txPin 14
SoftwareSerial SoftSerial(rxPin, txPin); // RX, TX




//***** B Root
const String errBroot[] {
  "BROOT SUCCEEDED",
  "SKVER COMMAND FAILED",
  "SKSETPWD COMMAND FAILED",
  "SKSETRBID COMMAND FAILED",
  "SKSCAN FAILED",
  "TO GET SCAN END FAILED",
  "SKSREG S2 COMMAND FAILED",
  "SKSREG S3 COMMAND FAILED",
  "SKLL64 COMMAND FAILED",
  "SKJOIN COMMAND FAILED",
  "SKSENDTO COMMAND FAILED"
  "READ WI-SUN FILE FALED"
};

enum eErrBroot {
  BROOT_SUCCEEDED = 0,
  SKVER_COMMAND_FAILED = -1,
  SKSETPWD_COMMAND_FAILED = -2,
  SKSETRBID_COMMAND_FAILED = -3,
  SKSCAN_FAILED = -4,
  TO_GET_SCAN_END_FAILED = -5,
  SKSREG_S2_COMMAND_FAILED = -6,
  SKSREG_S3_COMMAND_FAILED = -7,
  SKLL64_COMMAND_FAILED = -8,
  SKJOIN_COMMAND_FAILED = -9,
  SKSENDTO_COMMAND_FAILED = -10,
  READ_WI_SUN_FILE_FALED = -11
};

String m_ipv6Addr = "";
float m_fCoefficient;
float m_fUnit;
#define WI_SUN_PARAM_LEN 64
char m_authenticationID[WI_SUN_PARAM_LEN];
char m_wi_sunPassword[WI_SUN_PARAM_LEN];
#define WI_SUN_PARAM_FILE "/wisunparafile"





///****************************
///setup
///****************************
void setup() {
  Serial.begin(115200);
  SPIFFS.begin();
  //reset BP35A1
  digitalWrite(5, LOW); delay(5); digitalWrite(5, HIGH);delay(10000);
  
  // The speed with BP35A1 went well only when using SoftwareSerial at the lowest speed of 2400
  setSSS(2400);   //This means SoftSerial.begin(2400);

  {
    eErrBroot result;
    result = brootBegin();
    String str = "broot begin result = ";
    if (result == BROOT_SUCCEEDED) str += " succeeded";
    else str += errBroot[-1 * static_cast<int>(result)];
    Serial.println(str);
  }

  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  pinMode(5, OUTPUT);       //BP35A1 RESET PIN

  digitalWrite(5, HIGH);
}


///****************************
///loop
///****************************
void loop() {
	serialOperate();
  delay(100);
}



//millis()の値がlast+cmpより小さいかどうか判定する
//millis()はunsigned longなので、約50日で一回りする
//last+cmpがunsigned longを超える時の処理を入れて比較するモジュール
boolean LT (unsigned long last, unsigned long cmp) {
  unsigned long mil = millis();
  unsigned long def = 0xFFFFFFFF - last;		//lastからoverflowまでの距離（差分）
  unsigned long nextDef = cmp - def;	//判定境界値

  if (def < cmp) {							//defがcmpより小さければmillis()が一回りする
    if (mil < 0xFFFFFFFF) return true;
    else if (mil < nextDef) return true;
    else return false;
  }
  else {									//defがcmpより大きければmillis()はoverflowしないので、通常の比較
    if (mil - last < cmp) return true;
    else return false;
  }
}

