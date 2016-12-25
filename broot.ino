#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <TimeLib.h>
#include <FS.h>
#include <SoftwareSerial.h>


#define DEBUG_MD
//#define DEBUG_MAX

String m_startTime = "";

#define MILISECS_PER_MIN (60000UL)
//_time_に指定された分辺りのミリ秒
#define numberOfMiliSeconds(_time_) (_time_ * MILISECS_PER_MIN)
#define PERIOD_OF_CONNECT_NTP 10 //何分毎に接続するか(1～59)
#define PERIOD_OF_COUNTER_RESET 10    //motion detectorのカウンタのリセット間隔(分)
#define PERIOD_OF_WIFI_RESET 5       //WIFIが切れるのを防ぐため、リセットしてみる間隔（分）
#define SMTP_CONNECT_TIME 12        //メールを送る時刻

#define WIFI_PARAM_LEN 128
char m_ssid[WIFI_PARAM_LEN];
char m_wifiPassword[WIFI_PARAM_LEN];
#define WIFI_PARAM_FILE "/wifiparafile"

boolean SSE_on = false;//Server-Sent Events設定が済んだかどうかのフラグ

WiFiServer server(23);
WiFiClient wClient;

//-------NTPサーバー定義----------------
unsigned int localPort = 2390;      // local port to listen for UDP packets
IPAddress timeServerIP; // NTP server address
const char* ntpServerName = "ntp.nict.jp";
const int NTP_PACKET_SIZE = 48; // NTP time stamp is in the first 48 bytes of the message
byte packetBuffer[ NTP_PACKET_SIZE]; //buffer to hold incoming and outgoing packets
WiFiUDP udp;
#define NTP_SYNC_INTERVAL 1800

String m_year;
String m_month;
String m_day;
String m_hour;
String m_minute;
String m_second;

int m_iSensingCount = 0;
time_t m_detectSecond;
uint32_t m_wifiConnectTime;
#define WIFI_CONNECT_PERIOD 300000


//----------SMTP定義-------------
#define SMTP_PARAM_FILE "/SmtpParaFile"
#define SMTP_PARAM_LEN 256

char m_host[SMTP_PARAM_LEN];
char m_auth_pass[SMTP_PARAM_LEN];
char m_mail_from[SMTP_PARAM_LEN];
char m_rcpt_to[SMTP_PARAM_LEN];
char m_header[SMTP_PARAM_LEN];
#define SUBJECT "Subject:mail "
int m_httpPort;     //SMTPサーバーがポート587を使っている場合

#define SMTP_SUCCEED "succeed"

boolean m_sensingCountResetFlag = false;
boolean m_smtpConnectFlag = false;
boolean m_createSmtpFileFlag = false;

const String errSMTP[] {
  "SUCCEEDED",
  "EHLO FAILED",
  "AUTH PLAIN FAILED",
  "PASSWORD FAILED",
  "MAIL FROM FAILED",
  "RCPT TO FAILED",
  "DATA FAILED",
  "SEND FILE OPEN FAILED",
  "CONNECTION FAILED",
  "SEND FILE NOT EXIST",
  "READ PARAM FILE FAILED"
};

enum eErrSmtp {
  SUCCEEDED = 0,
  EHLO_FAILED = -1,
  AUTH_PLAIN_FAILED = -2,
  PASSWORD_FAILED = -3,
  MAIL_FROM_FAILED = -4,
  RCPT_TO_FAILED = -5,
  DATA_FAILED = -6,
  SEND_FILE_OPEN_FAILED = -7,
  CONNECTION_FAILED = -8,
  SEND_FILE_NOT_EXIST = -9,
  READ_PARAM_FILE_FAILED = -10
};


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

//**** Control Parameter
String m_beforeYear = "";
String m_beforeMonth = "";
String m_beforeDay = "";
String m_mdFilename = "";
String m_sendFilename = "";


#define MAX_SPIFFS_USING_BYTES 690000

int m_periodEnd = 0;
int m_NextTime = 0;
int m_wifiRestTime = 0;
boolean wifiResetFlag = false;
int m_smtpSendErrorCount = 0;


/*
  control with Serial Event

  When new serial data arrives, this sketch gets it to a char.
  Then this program will be controlled with the char.


	m_inputChar    function
  	 'n'         non
     'a'         minimum debug
     'b'         maximum debug
     'c'         display debug file
*/
char m_inputChar = 'n';         // a string to hold incoming data
String m_debugFilename = "";
File m_debugFile;

//for debug
int m_dCount = 0;

#define rxPin 12
#define txPin 14

// set up a new serial object
SoftwareSerial SoftSerial(rxPin, txPin); // RX, TX



///****************************
///setup
///****************************
void setup() {
  Serial.begin(115200);
  SPIFFS.begin();
  //reset BP35A1
  digitalWrite(5, LOW); delay(5); digitalWrite(5, HIGH);delay(10000);
  //SoftSerial.begin(2400);
  setSSS(2400); // BP35A1との速度調整時のテスト用 ソフトウェアシリアルの初期化（速度も同時に2400に戻す）

  {
    eErrBroot result;
    result = brootBegin();
    String str = "broot begin result = ";
    if (result == BROOT_SUCCEEDED) str += " succeeded";
    else str += errBroot[-1 * static_cast<int>(result)];
    Serial.println(str);
  }

  pinMode(13, OUTPUT);
  pinMode(4, INPUT);
  pinMode(rxPin, INPUT);
  pinMode(txPin, OUTPUT);
  pinMode(5, OUTPUT);


  //SPDT reset
  pinMode(5, OUTPUT);
  digitalWrite(5, HIGH);

  wifiStart();
  setupArduinoOTA();

#ifdef DEBUG_MD
  Serial.println("call connectNtpServer");
#endif
  connectNtpServer();

  getDateTime();
  m_startTime = m_year + "/" + m_month + "/" + m_day + "-" + m_hour + ":" + m_minute + ":" + m_second;

  m_mdFilename = createMdFilename(true);  //filename is today-tomorrow
  m_sendFilename = m_mdFilename;

#ifdef DEBUG_MD
  if (SPIFFS.exists(m_mdFilename)) {
    Serial.println(m_mdFilename + "  exists");
  }
  else Serial.println(m_mdFilename + " not exists");
#endif

  if (m_hour.toInt() <= SMTP_CONNECT_TIME & !SPIFFS.exists(m_mdFilename)) {
    m_mdFilename = createMdFilename(false);    //filename is yesterday-today
    m_sendFilename = m_mdFilename;
  }
#ifdef DEBUG_MD
  if (SPIFFS.exists(m_mdFilename)) Serial.println(m_mdFilename + "  exists");
  else Serial.println(m_mdFilename + " not exists");
#endif

  if (!SPIFFS.exists(m_mdFilename))
    createSmtpFile();

  m_periodEnd = m_minute.toInt() + PERIOD_OF_COUNTER_RESET;
  if (m_periodEnd > 59) m_periodEnd -= 60;
#ifdef DEBUG_MD
  Serial.println("m_periodEnd = " + String(m_periodEnd));
#endif

  int nextTime = m_minute.toInt() + PERIOD_OF_CONNECT_NTP;
  if (nextTime < 60) m_NextTime = nextTime;
  else m_NextTime = nextTime - 60;

  wClient = server.available();
  if (wClient == 0) Serial.println("wClient = 0");
  Serial.println();
}


///****************************
///loop
///****************************
void loop() {
  ArduinoOTA.handle();

	serialOperate();
  tcpOperate();

  getDateTime();

  ///detect Mosion
  if (digitalRead(4) == HIGH) {
  	digitalWrite(13, HIGH);
  	delay(30);
  	digitalWrite(13, LOW);
  	delay(30);
  	if(m_detectSecond != now()) {      //前回保持した時刻と異なっていたら実行する
  		m_iSensingCount++;
  		m_detectSecond = now();
  	}
  }

  if (m_minute.toInt() == m_periodEnd) {
    if (m_sensingCountResetFlag) {

      //カウンタの書式を整える
      String sCount;
      if (m_iSensingCount < 10) sCount = "00" + String(m_iSensingCount);
      else if (m_iSensingCount < 100) sCount = "0" + String(m_iSensingCount);
      else sCount = String(m_iSensingCount);
      //出力データ
      String str = m_year + "/" + m_month + "/" + m_day + " " + m_hour + ":" + m_minute + ":" + m_second + " = " + sCount;
//      str.toCharArray(&m_smtpData[m_iSendData][0], DATA_SIZE);

      //debug control
      if (m_inputChar == 'a' || m_inputChar == 'b') {
        Serial.println(str);
          //debug
          if (m_inputChar == 'b') {
            debugWrite(str);
          }
      }

      //ファイル出力
      File smtpFile = SPIFFS.open(m_mdFilename, "a");
      if (smtpFile) {

        //debug control
        if (m_inputChar == 'a' || m_inputChar == 'b') {
          if (smtpFile) {
            String str = "file open scceeded : in loop";
            Serial.println(str);
              //debug
              if (m_inputChar == 'b') {
                debugWrite(str);
              }
          }
          else {
            String str = "file open failed : in loop";
            Serial.println(str);
              //debug
              if (m_inputChar == 'b') {
                debugWrite(str);
              }
            ESP.restart();
          }
        }

        long val;
        int BrootCount = -1;
        while(BrootCount++ < 15) {
          eErrBroot result;
          result = getInstantaneousPower(&val);
          debugWrite("after getBroot : " + m_hour + ":" + m_minute + ":" + m_second + " = " + val);
          if(result == BROOT_SUCCEEDED or BrootCount == 15) break;
          else {
            delay(5000);
            String str = "get Integral Power Consumption, result = ";
            if(BrootCount == 10) {
              resetWI_SUN();
              result = brootBegin();
              str = "broot begin result = ";
            }
            if (result == BROOT_SUCCEEDED) str += " succeeded";
            else str += errBroot[-1 * static_cast<int>(result)];
            if(Serial) Serial.println(str);
          }
        }

        smtpFile.println(str + " : " + String(val));
        smtpFile.close();
        debugWrite("after smtpFile.println : " + m_hour + ":" + m_minute + ":" + m_second);
      }

      //カウンタをリセット
      m_iSensingCount = 0;
      //次にカウンタをリセットする時刻（分）をセット
      m_periodEnd = m_minute.toInt() + PERIOD_OF_COUNTER_RESET;
      if (m_periodEnd > 59) m_periodEnd -= 60;
      //カウンタリセットフラグを取り消す
      m_sensingCountResetFlag = false;//カウンタをリセットしたので、同一時刻（分）内でのリセットが起こらない様にフラグを降ろす
    }
  }
  else m_sensingCountResetFlag = true;    //カウンタをリセットする時刻（分）ではないので、次回のためにリセットフラグをたてる


  if (m_minute.toInt() == m_NextTime) {
    int nextTime = m_minute.toInt() + PERIOD_OF_CONNECT_NTP;
    if (nextTime < 60) m_NextTime = nextTime;
    else m_NextTime = nextTime - 60;
  }


  //if(m_minute.toInt() % 10 == 0) {
  //if(m_minute == SMTP_CONNECT_TIME) {
  //if (m_minute == SMTP_CONNECT_TIME) {
  if (m_hour.toInt() == SMTP_CONNECT_TIME) {

    if (m_createSmtpFileFlag) {
      m_beforeYear = m_year;
      m_beforeMonth = m_month;
      m_beforeDay = m_day;
      m_mdFilename = createMdFilename(true);  //filename is today-tomorrow
      createSmtpFile();
      //ファイル出力
      File smtpFile = SPIFFS.open(m_mdFilename, "a");
      if (smtpFile) {
        String IPC;
        eErrBroot result;
        result = getIntegralPowerConsumption(&IPC);
        smtpFile.println("Integral Power Consumption : " + IPC);
        smtpFile.close();
      }
      m_createSmtpFileFlag = false;         //m_hourがSMTP_CONNECT_TIMEの以外になるまでfalse
    }

    if (m_smtpConnectFlag) {
      eErrSmtp result = mailSend(m_sendFilename);

      //debug control
      if (m_inputChar == 'a' || m_inputChar == 'b') {
          String str = "mailSend result = ";
          if (result == SUCCEEDED) str += " mailSend succeeded at " + m_minute + " minute\r\n";
          else str += errSMTP[-1 * static_cast<int>(result)] + " at " + m_minute + " minute\r\n";
          Serial.println(str);
            //debug
            if (m_inputChar == 'b') {
              debugWrite(str);
            }
          if (result == CONNECTION_FAILED)  {
            WiFi.disconnect();
            wifiReStart();
        }
      }

      m_smtpSendErrorCount++;

      if (result == SUCCEEDED
       || m_smtpSendErrorCount > 100   //errorが解消しないため、送信を断念する
       )
      {
        m_sendFilename = m_mdFilename;
        m_smtpConnectFlag = false;
        m_smtpSendErrorCount = 0;
      }
    }
  }
  else {
    m_createSmtpFileFlag = true;
    m_smtpConnectFlag = true;
  }
}





///****************************
///getDateTIme
///****************************
void getDateTime() {
  time_t t = now();

  int y = year(t);
  int mon = month(t);
  int d = day(t);
  int h = hour(t);
  int m = minute(t);
  int sec = second(t);

  m_year = String(y);

  if (mon < 10) m_month = "0" + String(mon);
  else m_month = String(mon);

  if (d < 10) m_day = "0" + String(d);
  else m_day = String(d);

  if (h < 10) m_hour = "0" + String(h);
  else m_hour = String(h);

  if (m < 10) m_minute = "0" + String(m);
  else m_minute = String(m);

  if (sec < 10) m_second = "0" + String(sec);
  else m_second = String(sec);
}


///****************************
///NTPサーバーでタイムを取得
///****************************
void connectNtpServer() {
  WiFi.hostByName(ntpServerName, timeServerIP);
  setSyncInterval(NTP_SYNC_INTERVAL);
  setSyncProvider(getNtpTime);

  //debug control
  if (m_inputChar == 'a' || m_inputChar == 'b') {
    String str = "called setSyncProvider & getNtpTime";
    Serial.println(str);
        //debug
        if (m_inputChar == 'b') {
          //debugWrite(str);
        }
  }
}

///****************************
//************NTPサーバータイム取得関数*****************************
const int timeZone = 9;     // 日本時間
//const int timeZone = 1;     // Central European Time
//const int timeZone = -5;  // Eastern Standard Time (USA)
//const int timeZone = -4;  // Eastern Daylight Time (USA)
//const int timeZone = -8;  // Pacific Standard Time (USA)
//const int timeZone = -7;  // Pacific Daylight Time (USA)

time_t getNtpTime()
{
  //debug control
  if (m_inputChar == 'a' || m_inputChar == 'b') {
    String str = "call sendNTPpacket from " + String(0);
    Serial.println(str);
        //debug
        if (m_inputChar == 'b') {
          //debugWrite(str);
        }
  }

  time_t hr = 0; // return 0 if unable to get the time

  //NTPサーバーでタイムを取得するためのport
  if(udp.begin(localPort) == 0) {
        //debug control
        if (m_inputChar == 'a' || m_inputChar == 'b') {
          String str = "there are no sockets available which udp use";
          Serial.println(str);
              //debug
              if (m_inputChar == 'b') {
                debugWrite(str);
              }
        }
  }

  while (udp.parsePacket() > 0) ; // discard any previously received packets

  sendNTPpacket(timeServerIP);
  uint32_t beginWait = millis();
  delay(1);//これを入れないと更新できない場合がある。
  while ( LT(beginWait, 1500) ) {
    int size = udp.parsePacket();
    if (size >= NTP_PACKET_SIZE) {
      udp.read(packetBuffer, NTP_PACKET_SIZE);  // read packet into the buffer
      unsigned long secsSince1900;
      // convert four bytes starting at location 40 to a long integer
      secsSince1900 =  (unsigned long)packetBuffer[40] << 24;
      secsSince1900 |= (unsigned long)packetBuffer[41] << 16;
      secsSince1900 |= (unsigned long)packetBuffer[42] << 8;
      secsSince1900 |= (unsigned long)packetBuffer[43];
      hr = secsSince1900 - 2208988800UL + timeZone * SECS_PER_HOUR;
      break;
    }
  }
  udp.stop();

  return hr; // return 0 if unable to get the time
}


///****************************
//******************NTPリクエストパケット送信****************************
///****************************
unsigned long sendNTPpacket(IPAddress& address)
{
  //debug control
  if (m_inputChar == 'a' || m_inputChar == 'b') {
    String str = m_year + "/" + m_month + "/" + m_day + " " + m_hour + ":" + m_minute + ":" + m_second + "\r\nsending NTP packet...";
    Serial.println(str);
        //debug
        if (m_inputChar == 'b') {
          debugWrite(str);
        }
  }

  // set all bytes in the buffer to 0
  memset(packetBuffer, 0, NTP_PACKET_SIZE);
  // Initialize values needed to form NTP request
  // (see URL above for details on the packets)
  packetBuffer[0] = 0b11100011;   // LI, Version, Mode
  packetBuffer[1] = 0;     // Stratum, or type of clock
  packetBuffer[2] = 6;     // Polling Interval
  packetBuffer[3] = 0xEC;  // Peer Clock Precision
  // 8 bytes of zero for Root Delay & Root Dispersion
  packetBuffer[12]  = 49;
  packetBuffer[13]  = 0x4E;
  packetBuffer[14]  = 49;
  packetBuffer[15]  = 52;

  // all NTP fields have been given values, now
  // you can send a packet requesting a timestamp:
  udp.beginPacket(address, 123); //NTP requests are to port 123
  udp.write(packetBuffer, NTP_PACKET_SIZE);
  udp.endPacket();
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

