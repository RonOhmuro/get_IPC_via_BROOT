String createMdFilename(bool bToday) {
  String path;

  time_t tm = now();
  if (bToday) tm += 60 * 60 * 24;
  else tm -= 60 * 60 * 24;
  {
    int y = year(tm);
    int mon = month(tm);
    int d = day(tm);

    String Year = String(y);

    String Month;
    if (mon < 10) Month = "0" + String(mon);
    else Month = String(mon);

    String Day;
    if (d < 10) Day = "0" + String(d);
    else Day = String(d);

    if (bToday)
      path = "/" + m_year + m_month + m_day + "-" + Year + Month + Day;
    else
      path = "/" + Year + Month + Day + "-" + m_year + m_month + m_day;
  }

  //debug
  if (m_inputChar == 'a' || m_inputChar == 'b') {
    Serial.println(path);
  }
  return path;
}



///**********************************************************
String createDebugFilename() {
  return m_mdFilename + "_debug.txt";
}



///**********************************************************
///**********************************************************
void createSmtpFile() {
  getSpiffsSpace();

  File smtpFile = SPIFFS.open(m_mdFilename, "a");
  if (!smtpFile) {
    //debug
    if (m_inputChar == 'a' || m_inputChar == 'b') {
      String str = "smtp file open failed";
      Serial.println(str);
        //debug
        if (m_inputChar == 'b') {
          debugWrite(str);
        }
    }
    return;
  }
  smtpFile.print(m_header);
  smtpFile.println(SUBJECT + m_mdFilename.substring(1));
  smtpFile.println();
  smtpFile.close();
    //debug
    if (m_inputChar == 'a' || m_inputChar == 'b') {
      String str = "createSmtpFile succeeded";
      Serial.println(str);
        //debug
        if (m_inputChar == 'b') {
          debugWrite(str);
        }
    }
}



///**********************************************************
///SPIFFSのdirectory情報を取得・表示する
///*min 一番古い日付を表す数値　例：20160827
///*size ファイルシステム使用済みバイト数
///**********************************************************
void getDirInfo(long *min, long *size) {
  {
    String nowDay = "";
    if (m_year != "") nowDay = m_year + m_month + m_day;
    else {
      getDateTime();
      nowDay = m_year + m_month + m_day;
    }
//    *min = nowDay.toInt();
    *min = 30000101;
    *size = 0;

    //debug
    if (m_inputChar == 'a' || m_inputChar == 'b') {
      Serial.print("*min = ");
      Serial.println(*min);
    }

    Dir dir = SPIFFS.openDir("/");

    int fileNumber = 1;
    {
      String tmpFileName = dir.fileName();
      File f = SPIFFS.open(tmpFileName, "r");
      if(f) {
        String fileInfo = tmpFileName.substring(1, 1 + 8); //+8 year+month+day 20160806 のこと
        if (tmpFileName != WIFI_PARAM_FILE
          && tmpFileName != SMTP_PARAM_FILE
          && tmpFileName != WI_SUN_PARAM_FILE
          && *min > fileInfo.toInt())
          *min = fileInfo.toInt();
        dispFileInfo(fileNumber, tmpFileName, f.size(), *min);
        f.close();
      }

      fileNumber++;
    }
    while (dir.next()) {
      String tmpFileName = dir.fileName();
      File f = SPIFFS.open(tmpFileName, "r");
      if(f) {
        String fileInfo = tmpFileName.substring(1, 1 + 8); //+8 year+month+day 20160806 のこと
        if (tmpFileName != WIFI_PARAM_FILE
          && tmpFileName != SMTP_PARAM_FILE
          && tmpFileName != WI_SUN_PARAM_FILE
          && *min > fileInfo.toInt())
          *min = fileInfo.toInt();
        dispFileInfo(fileNumber, tmpFileName, f.size(), *min);
        f.close();
      }

      fileNumber++;
    }

    FSInfo fs_info;
    if(SPIFFS.info(fs_info)) {
      *size = static_cast<long>(fs_info.usedBytes);
    }
    else {
      String str = "get FSInfo failed・・・";
      Serial.println(str);
      debugWrite(str);
    }
  }
}




///**********************************************************
///SPIFFSのfile number numのfile nameを取得する
///**********************************************************
String getDirInfo(int num) {
  {
    Dir dir = SPIFFS.openDir("/");
    String hrStr;
    int fileNumber = 1;
    if (fileNumber <= num) {
      //スコープの外で
      //String tmpFileName = "";
      //を記述し、
      //tmpFileName = dir.fileName();
      //でfilenameを取得しようとしたら、スタックしたので、直接String変数を作るコードにした。
      String tmpFileName = dir.fileName();
      dispFileInfo(fileNumber, tmpFileName);
      hrStr = tmpFileName;
      fileNumber++;
    }
    delay(10);
    while (fileNumber <= num && dir.next()) {
      String tmpFileName = dir.fileName();
      dispFileInfo(fileNumber, tmpFileName);
      hrStr = tmpFileName;
      fileNumber++;
    }
    return hrStr;
  }
}




///**********************************************************
///**********************************************************
void dispFileInfo(int fileNumber, String fileName, int fileSize, long min) {
        String str = "";
        if(fileNumber < 10) str = "[   " + String(fileNumber) + "] ";
        else if(fileNumber < 100) str = "[  " + String(fileNumber) + "] ";
        else if(fileNumber < 1000) str = "[ " + String(fileNumber) + "] ";
        else str = "[" + String(fileNumber) + "] ";
        str += fileName + ": ";
        str += String(fileSize);
        str += "Byte  :  *min = ";
        str += String(min);
        if(Serial) Serial.println(str);
        if(wClient) wClient.println(str);

          //debug ★★★★ directoryをloopで回しながら書き込みを行うとdir.next()がおかしくなる。
          //dir.next()ループ内で、write()を使うコードは入れてはいけない。
          //if (m_inputChar == 'b') {
          //  debugWrite(str);
          //}
}




///**********************************************************
///**********************************************************
void dispFileInfo(int fileNumber, String fileName) {
        String str = "";
        if(fileNumber < 10) str = "[   " + String(fileNumber) + "] ";
        else if(fileNumber < 100) str = "[  " + String(fileNumber) + "] ";
        else if(fileNumber < 1000) str = "[ " + String(fileNumber) + "] ";
        else str = "[" + String(fileNumber) + "] ";
        str += fileName;
        if(Serial) Serial.println(str);
        if(wClient) wClient.println(str);

          //debug ★★★★ directoryをloopで回しながら書き込みを行うとdir.next()がおかしくなる。
          //dir.next()ループ内で、write()を使うコードは入れてはいけない。
          //if (m_inputChar == 'b') {
          //  debugWrite(str);
          //}
}




///**********************************************************
//dir information ********************************************************************
///**********************************************************
void getSpiffsSpace() {

  long minimumDay = 0;
  long usingBytes = 0;
  getDirInfo(&minimumDay, &usingBytes);

  while (usingBytes > MAX_SPIFFS_USING_BYTES) {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String tmpFileName = dir.fileName();
      tmpFileName = tmpFileName.substring(1, 1 + 8); //+8 year+month+day 20160806 のこと
      if (minimumDay == tmpFileName.toInt()) {
        if (!SPIFFS.remove(dir.fileName())) {
          String str = dir.fileName() + " not deleted";
          Serial.println(str);
          debugWrite(str);
        }
      }
    }
    getDirInfo(&minimumDay, &usingBytes);
  }
}




///**********************************************************
///**********************************************************
void dispDebugFileContents(String m_debugFilename) {
  m_debugFile = SPIFFS.open(m_debugFilename,"r");
  if(!m_debugFile) {
    String str = "debug file open failed for read (in diapDebugFileContents) ...";
    if(wClient) wClient.println(str);
    Serial.println(str);
    return;
  }
  else {
    String str = "debug file open succeeded for read ...";
    if(wClient) wClient.println(str);
    Serial.println(str);
    str = "file size = " + String(m_debugFile.size());
    if(wClient) wClient.println(str);
    Serial.println(str);
    str = "file position = " + String(m_debugFile.position());
    if(wClient) wClient.println(str);
    Serial.println(str);
    while(1) {
      String line = m_debugFile.readStringUntil('\n');
      if(line == NULL) break;
      Serial.println(line);
      if(wClient) wClient.println(line);
    }
    m_debugFile.close();
  }
}





///**********************************************************
///**********************************************************
void debugWrite(String str) {
  //getSpiffsSpace();

  m_debugFilename = createDebugFilename();
  m_debugFile = SPIFFS.open(m_debugFilename, "a");
  if (!m_debugFile) {
    Serial.println("debug file open failed...");
    ESP.restart();
  }
  if(!m_debugFile.println(str)){
    Serial.println("did not write ; " + str);
    ESP.restart();
  }
  m_debugFile.close();
}



///************************************************
/// readWiFi
///************************************************
bool readWiFi(){
  if(SPIFFS.exists(WIFI_PARAM_FILE)){
    File f = SPIFFS.open(WIFI_PARAM_FILE,"r");
    if(f){
      String str = f.readStringUntil('\n');
      if(str != NULL) {
        str.toCharArray(m_ssid,str.length(),0);
        Serial.println(String(m_ssid));
      }
      else {
        Serial.println("m_ssid is NULL");
      }

      str = f.readStringUntil('\n');
      if(str != NULL) {
        str.toCharArray(m_wifiPassword,str.length(),0);
        Serial.println(String(m_wifiPassword));
      }
      else {
        Serial.println("m_wifiPassword is NULL");
      }
      f.close();
      return true;
    }
    else {
      Serial.println("could not open " + String(WIFI_PARAM_FILE));
      return false;
    }
  }
}



///************************************************
/// setWiFi
///************************************************
void setWiFi(){
  getSpiffsSpace();

  File f = SPIFFS.open(WIFI_PARAM_FILE,"w");
  if(f){
    Serial.print("input ssid : ");
    String str = getInput();
    str.toCharArray(m_ssid,str.length() + 1,0);
    f.println(str);
    Serial.print("input WiFi password : ");
    str = getInput();
    str.toCharArray(m_wifiPassword,str.length() + 1,0);
    f.println(str);
    f.close();
  }
  return;
}



///************************************************
/// readSmtpParaFile
///************************************************
bool readSmtpParaFile(){
  if(SPIFFS.exists(SMTP_PARAM_FILE)){
    File f = SPIFFS.open(SMTP_PARAM_FILE,"r");
    if(f){
      String str = f.readStringUntil('\n');
      if(str != NULL) {
        str.toCharArray(m_host,str.length(),0);
        Serial.println(String(m_host));
      }
      else {
        Serial.println("m_host is NULL");
      }

      str = f.readStringUntil('\n');
      if(str != NULL) {
        str.toCharArray(m_auth_pass,str.length(),0);
        Serial.println(String(m_auth_pass));
      }
      else {
        Serial.println("m_auth_pass is NULL");
      }

      str = f.readStringUntil('\n');
      if(str != NULL) {
        str.toCharArray(m_mail_from,str.length(),0);
        Serial.println(String(m_mail_from));
      }
      else {
        Serial.println("m_mail_from is NULL");
      }

      str = f.readStringUntil('\n');
      if(str != NULL) {
        str.toCharArray(m_rcpt_to,str.length(),0);
        Serial.println(String(m_rcpt_to));
      }
      else {
        Serial.println("m_rcpt_to is NULL");
      }
  
      str = f.readStringUntil('\n');
      if(str != NULL) {
        m_httpPort = str.toInt();
        Serial.println(String(m_httpPort));
      }
      else {
        Serial.println("m_httpPort is NULL");
      }
   
      f.close();

      str = "From:" + String(m_mail_from) + "\r\nTo:" + String(m_rcpt_to) + "\r\n";
      str.toCharArray(m_header,str.length() + 1,0);
      Serial.println(String(m_header));

      return true;
    }
    else {
      Serial.println("could not open " + String(SMTP_PARAM_FILE));
      return false;
    }
  }
}



///************************************************
/// setSmtpParaFile
///************************************************
void setSmtpParaFile(){
  getSpiffsSpace();

  File f = SPIFFS.open(SMTP_PARAM_FILE,"w");
  if(f){
    Serial.print("input host : ");
    String str = getInput();
    str.toCharArray(m_host,str.length() + 1,0);
    f.println(str);
    Serial.print("input auth_pass : ");
    str = getInput();
    str.toCharArray(m_auth_pass,str.length() + 1,0);
    f.println(str);
    Serial.print("input mail_from : ");
    str = getInput();
    str.toCharArray(m_mail_from,str.length() + 1,0);
    f.println(str);

    Serial.print("input rcpt_to : ");
    str = getInput();
    str.toCharArray(m_rcpt_to,str.length() + 1,0);
    f.println(str);
    
    str = "From:" + String(m_mail_from) + "\r\nTo:" + String(m_rcpt_to) + "\r\n";
    str.toCharArray(m_header,str.length() + 1,0);
    Serial.println(String(m_header));

    while(1) {
      Serial.print("input httpPort : ");
      str = getInput();
      m_httpPort = str.toInt();
      if(m_httpPort == 0) {
        Serial.println("ERROR: wrong number\r\ninput is natural numeric number which is more than zero !");
        continue;
      }
      f.println(str);
      break;
    }

    f.close();
  }
  return;
}



///************************************************
/// readWI-SUN
///************************************************
bool readWI_SUN(){
  if(SPIFFS.exists(WI_SUN_PARAM_FILE)){
    File f = SPIFFS.open(WI_SUN_PARAM_FILE,"r");
    if(f){
      String str = f.readStringUntil('\n');
      if(str != NULL) {
        str.toCharArray(m_authenticationID,str.length(),0);
        Serial.println(String(m_authenticationID));
      }
      else {
        Serial.println("B root authentication ID is NULL");
      }

      str = f.readStringUntil('\n');
      if(str != NULL) {
        str.toCharArray(m_wi_sunPassword,str.length(),0);
        Serial.println(String(m_wi_sunPassword));
      }
      else {
        Serial.println("B root password is NULL");
      }
      f.close();
      return true;
    }
    else {
      Serial.println("could not open " + String(WI_SUN_PARAM_FILE));
      return false;
    }
  }
}



///************************************************
/// setWI-SUN
///************************************************
void setWI_SUN(){
  getSpiffsSpace();

  File f = SPIFFS.open(WI_SUN_PARAM_FILE,"w");
  if(f){
    Serial.print("input B root authentication ID : ");
    String str = getInput();
    str.toCharArray(m_authenticationID,str.length() + 1,0);
    f.println(str);
    Serial.print("input B root password : ");
    str = getInput();
    str.toCharArray(m_wi_sunPassword,str.length() + 1,0);
    f.println(str);
    f.close();
  }
  return;
}

