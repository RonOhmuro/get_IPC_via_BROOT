// Use WiFiClient class to create TCP connections
//-----------SMTP----------------
WiFiClient client;

eErrSmtp mailSend(String sendFile) {

  eErrSmtp hr = SUCCEEDED;

  if(m_host[0] == '\0') {
    if(!readSmtpParaFile()){
      hr = READ_PARAM_FILE_FAILED;
      return hr;
    }
  }

      //debug control
      if (m_inputChar == 'a' || m_inputChar == 'b') {
        String str = m_year + "/" + m_month + "/" + m_day + " " + m_hour + ":" + m_minute + ":" + m_second + " = " + sendFile;
        Serial.println(str);
          //debug
          if (m_inputChar == 'b') {
            debugWrite(str);
          }
      }

  if(!SPIFFS.exists(sendFile)) {
    if (m_inputChar == 'a' || m_inputChar == 'b') {
        String str = "file not exists : in mailSend";
        Serial.println(str);
        debugWrite(str);
    }
    hr = SEND_FILE_NOT_EXIST;
    return hr;
  }

  if (m_inputChar == 'a' || m_inputChar == 'b') {
    String str = "connect SMTP\r\n host = " + String(m_host) + " : httpPort = " + String(m_httpPort);
    Serial.println(str);
    debugWrite(str);
  }

  do {

    if (!client.connect(m_host, m_httpPort)) {
      String str = "connection failed";
      Serial.println(str);
      debugWrite(str);
      hr = CONNECTION_FAILED;
      break;
    }
    else {
        debugWrite("client number = " + String(client));
    }

    client.print("ehlo ");
    client.println(m_host);
    if (!ReadReply(1000, "220")) {
      hr = EHLO_FAILED;
      break;
    }

    client.println("AUTH PLAIN");
    if (!ReadReply(1000, "334")) {
      hr = AUTH_PLAIN_FAILED;
      break;
    }

#ifdef DEBUG_MD
    Serial.println(m_auth_pass);
#endif
    client.println(m_auth_pass);
    delay(100);
    if (!ReadReply(1000, "OK Authenticated")) {
      hr = PASSWORD_FAILED;
      break;
    }

#ifdef DEBUG_MD
    Serial.println(m_mail_from);
#endif
    client.print("mail from:");
    client.println(m_mail_from);
    if (!ReadReply(1000, "Sender ok")) {
      hr = MAIL_FROM_FAILED;
      break;
    }

    client.print("rcpt to:");
    client.println(m_rcpt_to);
    if (!ReadReply(1000, "Recipient ok")) {
      hr = RCPT_TO_FAILED;
      break;
    }

    client.println("data");
    if (!ReadReply(1000, "354")) {
      hr = DATA_FAILED;
      break;
    }
//    client.print(header);
//    client.println(SUBJECT + m_beforeYear + m_beforeMonth + m_beforeDay + "-" + m_year + m_month + m_day);
//    client.println("\r\n\r\n");

//    for (int i = 0; i < m_iSendData; i++) client.println(&m_smtpData[i][0]);
/*
#ifdef DEBUG_MD
    Serial.print(header);
    Serial.println(SUBJECT + m_beforeYear + m_beforeMonth + m_beforeDay + "-" + m_year + m_month + m_day);
    Serial.println("m_iSendData = " + String(m_iSendData));
    for (int i = 0; i < m_iSendData; i++) Serial.println(&m_smtpData[i][0]);
#endif
*/
    //ファイル内容を出力
    File smtpFile = SPIFFS.open(sendFile, "r");
    if (!smtpFile) {
      if (m_inputChar == 'a' || m_inputChar == 'b') {
          String str = "file open failed : in mailSend";
          Serial.println(str);
          debugWrite(str);
      }
			hr = SEND_FILE_OPEN_FAILED;
			break;
    }
    while (1) {
      String line = smtpFile.readStringUntil('\n');
      if (line == NULL) break;
      client.println(line);
#ifdef DEBUG_MD
      Serial.println(line);
#endif

    }
    smtpFile.close();

    client.println(".");
    client.println("quit");

  } while (false);

  client.stop();

  Serial.println();
  if (hr < 0) Serial.println(errSMTP[-1 * static_cast<int>(hr)]);
  else Serial.println(SMTP_SUCCEED);
  Serial.println("closing connection");

  if (hr < 0) debugWrite("\r\n" + errSMTP[-1 * static_cast<int>(hr)]);
  else debugWrite("\r\n" + String(SMTP_SUCCEED));
  debugWrite("closing connection");
  debugWrite("client number = " + String(client));
  return hr;
}


boolean ReadReply(int period, String str) {
  boolean hr = false;
  unsigned long lastTime = millis();

  while (LT(lastTime, period)) {
    if (client.available()) {
      String line = client.readStringUntil('\n');
#ifdef DEBUG_MD
      Serial.print(line);
#endif
      if (line.indexOf(str) >= 0) {
        hr = true;
        break;
      }
    }
  }

  return hr;
}

