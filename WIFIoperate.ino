  
///****************************
///wifiStart
///****************************
void wifiStart() {
  Serial.println("\r\nBooting");
  int count = 0;

  if(!readWiFi()){
    setWiFi();
  }

  WiFi.mode(WIFI_STA);
  Serial.println("m_ssid = " + String(m_ssid) + " ; m_wifiPassword = " + String(m_wifiPassword));
  bool fHigh = false;
  int status = WL_IDLE_STATUS;
  while (1) {
    status = WiFi.status();
    if(status == WL_CONNECTED) break;
    fHigh = !fHigh;
    digitalWrite(13, fHigh);
    delay(500);
    Serial.print(".");
    if(status == WL_NO_SHIELD) Serial.print("WL_NO_SHIELD");
    if(status == WL_IDLE_STATUS) Serial.print("WL_IDLE_STATUS");
    if(status == WL_NO_SSID_AVAIL) Serial.print("WL_NO_SSID_AVAIL");
    if(status == WL_SCAN_COMPLETED) Serial.print("WL_SCAN_COMPLETED");
    if(status == WL_CONNECT_FAILED) Serial.print("WL_CONNECT_FAILED");
    if(status == WL_CONNECTION_LOST) Serial.print("WL_CONNECTION_LOST");
    if(status == WL_DISCONNECTED) { Serial.print("WL_DISCONNECTED"); }
    if (count > 10) {
      Serial.print("\r\nConnection Failed!\r\nDo you change WiFi setting parameters ? ");
      String str = getInput();
      if(str == "y"){
        count = 0;
        setWiFi();
        WiFi.begin(m_ssid, m_wifiPassword);
        continue;
      }
      count = 0;
      Serial.println("\r\nAttempting to connect to WEP network, SSID: " + String(m_ssid));
      WiFi.begin(m_ssid, m_wifiPassword);
      delay(5000);
    }
    count++;
  }
  Serial.println("");
  Serial.println("WiFi connected");
  server.begin();
  server.setNoDelay(true);
  return;
}








///****************************
///wifiReStart
///****************************
void wifiReStart() {
  return;
}



///****************************
///wifiReStart
///****************************
void wifiReStart(bool b) {
  String str = "\n\rRestarting";
  Serial.println(str);
        //debug
        if (m_inputChar == 'b') {
          debugWrite(str);
        }
  int count = 0;

  WiFi.disconnect();
  WiFi.mode(WIFI_STA);
  WiFi.begin(m_ssid, m_wifiPassword);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (count > 20) {
      str = "Connection Failed! Rebooting...";
      Serial.println(str);
        //debug
        if (m_inputChar == 'b') {
          debugWrite(str);
        }
      ESP.restart();
    }
    count++;
  }
  str = "\r\nWiFi connected " + m_year + "/" + m_month + "/" + m_day + "-" + m_hour + ":" + m_minute + ":" + m_second + "\r\n";
  Serial.println(str);
        //debug
        if (m_inputChar == 'b') {
          debugWrite(str);
        }
  server.begin();
  //server.setNoDelay(true);
  return;
}



///****************************
///setupArduinoOTA
///****************************
void setupArduinoOTA() {

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  // ArduinoOTA.setHostname("myesp8266");

  // No authentication by default
  //uncomment by Ryuji
  ArduinoOTA.setPassword((const char *)"");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
    });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
    });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
    });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  return;
}




///****************************
///tcpからの操作
///****************************
void tcpOperate() {

  if (!wClient) {
    wClient = server.available();
    delay(10);
    if (wClient) {
      m_wifiConnectTime = millis();
      String str = "TCP connected ...";
      Serial.println(str);
      wClient.println(str);
      if(m_inputChar == 'b') {
        debugWrite(str);
      }

      //接続時に送ってくるデータを破棄する
      while (wClient.available()) {
        char c = wClient.read();
      }
    }
  }
  
  if (wClient) {
    if( !LT(m_wifiConnectTime, WIFI_CONNECT_PERIOD) ) {
      wifiDisconnect();
      return;
    }
    bool bTcpEnd = false;
    bool bDebugDisp = false;
    bool bInput = false;
    bool bGetFileInfo = false;
    bool bDeleteFile = false;
    bool bDispStartTime = false;
    bool bDispFileContents = false;
    bool bHelp = false;
    bool bDeleteAllFile = false;
    bool bShowFSInfo = false;
    while (wClient.available()) {
      char c = wClient.read();
      if (c == 'z' ) {
        bTcpEnd = true;
      }
      if (c == 'y') {
        bInput = true;
        m_wifiConnectTime = millis();
      }
      if (c == 'x') {
        bGetFileInfo = true;
        m_wifiConnectTime = millis();
      }
      if (c == 'w') {
        bDispStartTime = true;
        m_wifiConnectTime = millis();
      }
      if (c == 'v') {
        bDispFileContents = true;
        m_wifiConnectTime = millis();
      }
      if (c == 'u') {
        bDeleteAllFile = true;
        m_wifiConnectTime = millis();
      }
      if (c == 't') {
        bShowFSInfo = true;
        m_wifiConnectTime = millis();
      }
      if (c == 'h') {
        bHelp = true;
        m_wifiConnectTime = millis();
      }
      if (c == 'd') {
        bDeleteFile = true;
        m_wifiConnectTime = millis();
      }
      if (c == 'c') {
        bDebugDisp = true;
        m_wifiConnectTime = millis();
      }
      if (c == 'n' || c == 'a' || c == 'b') {
        m_inputChar = c;
        bInput = true;
        m_wifiConnectTime = millis();
      }
    }
    if(bHelp) {
      wClient.println("n:no debug");
      wClient.println("a:debug level a");
      wClient.println("b:debug level b");
      wClient.println("c:display last debug log file contents");
      wClient.println("d:delete file");
      wClient.println("h:help (this display)");
      wClient.println("t:show FSInfo");
      wClient.println("u:delete all file");
      wClient.println("v:display file contents");
      wClient.println("w:display last start time");
      wClient.println("x:display files");
      wClient.println("y:display debug level");
      wClient.println("z:exit");
    }
    if(bInput) {
      wClient.println("debug mode : " + String(m_inputChar));
      Serial.println("debug mode : " + String(m_inputChar));
    }
    if (bTcpEnd) {
      wifiDisconnect();
    }
    if (bGetFileInfo) {
      long a,b;
      getDirInfo(&a, &b);
    }
    if (bShowFSInfo) {
      FSInfo fs_info;
      if(SPIFFS.info(fs_info)) {
        wClient.println("totalBytes    = " + String(fs_info.totalBytes));
        wClient.println("usedBytes     = " + String(fs_info.usedBytes));
        wClient.println("blockSize     = " + String(fs_info.blockSize));
        wClient.println("pageSize      = " + String(fs_info.pageSize));
        wClient.println("maxOpenFiles  = " + String(fs_info.maxOpenFiles));
        wClient.println("maxPathLength = " + String(fs_info.maxPathLength));
      }
      else wClient.println("get FSInfo failed・・・");
    }
    if (bDeleteAllFile) {
      int fileNumber = 0;
      wClient.print("delete all file (y/n): ");
      bDeleteAllFile = false;
      while(1) {
        char c = wClient.read();
        if(c == 'y') bDeleteAllFile = true;
        if(c == '\n') break;
      }
      if(bDeleteAllFile) {
        wClient.print("really delete all file (y/n): ");
        bDeleteAllFile = false;
        while(1) {
          char c = wClient.read();
          if(c == 'y') bDeleteAllFile = true;
          if(c == '\n') break;
        }
      }
      if(bDeleteAllFile) {
        Dir dir = SPIFFS.openDir("/");

        while (dir.next()) {
          String tmpFileName = dir.fileName();
          if(SPIFFS.remove(tmpFileName)) wClient.println(tmpFileName + "  deleted");
        }
      }
      wClient.println();
    }
    if (bDeleteFile) {
      int fileNumber = 0;
      wClient.println("Input delete file number : ");
      while(1) {
        char c = wClient.read();
        if(47 < c && c < 58) fileNumber = fileNumber*10 + static_cast<int>(c-48);
        if(c == '\n') break;
      }
      wClient.println();
      String fileName = getDirInfo(fileNumber);
      wClient.print("delete file number is " + String(fileNumber) + " file name is " + fileName + " (y/n): ");
      bDeleteFile = false;
      while(1) {
        char c = wClient.read();
        if(c == 'y') bDeleteFile = true;
        if(c == '\n') break;
      }
      if(bDeleteFile) {
        if(SPIFFS.remove(fileName)) wClient.println("\r\nfile number " + String(fileNumber) + " " + fileName + " deleted"); 
        else wClient.println("\r\nfile number " + String(fileNumber) + " " + fileName + " delete failed.");
      }
      wClient.println();
    }
    if (bDispFileContents) {
      int fileNumber = 0;
      wClient.print("Input display file number : ");
      while(1) {
        char c = wClient.read();
        if(47 < c && c < 58) fileNumber = fileNumber*10 + static_cast<int>(c-48);
        if(c == '\n') break;
      }
      wClient.println();
      String fileName = getDirInfo(fileNumber);
      wClient.print("display file number is " + String(fileNumber) + " file name is " + fileName + " (y/n): ");
      bDispFileContents = false;
      while(1) {
        char c = wClient.read();
        if(c == 'y') bDispFileContents = true;
        if(c == '\n') break;
      }
      if(bDispFileContents) {
        dispDebugFileContents(fileName);
      }
      wClient.println();
    }
    if (bDebugDisp) {
      String str = "Disp debug file contents";
      wClient.println(str);
      Serial.println(str);
      dispDebugFileContents(createDebugFilename());
    }
    if (bDispStartTime) {
      wClient.println("m_startTime : " + m_startTime);
    }
  }
  return;
}



///
///disconnect表示
///
void wifiDisconnect () {
  String str = "TCP disconnected ...";
  wClient.println(str);
  wClient.stop();
  Serial.println(str);
  if(m_inputChar == 'b') {
    debugWrite(str);
  }
  return;
}