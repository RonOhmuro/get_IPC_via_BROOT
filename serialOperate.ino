#define COMMAND_WAIT_TIME 300000	//5分

int incomingByte = 0;	// 受信データ用

///****************************
///Serialからの操作
///****************************
void serialOperate() {
	if (Serial.available() > 0) { // 受信したデータが存在する
		incomingByte = Serial.read(); // 受信データを読み込む
		if(incomingByte == 'i') {
			Serial.print("\r\nI received: "); // 受信データを送りかえす
			Serial.println(incomingByte, DEC);
			operate();
		} 
		if(incomingByte == 'p') {
			Serial.print("\r\nI received: "); // 受信データを送りかえす
			Serial.println(incomingByte, DEC);
			//String dum;
			//getBroot(&dum);
	        String IPC;
	        eErrBroot result;
	        result = getIntegralPowerConsumption(&IPC);
	        if(Serial) Serial.println("Integral Power Consumption : " + IPC);
			if(Serial) Serial.println("getIntegralPowerConsumption end");
		} 
	}
}



///********************************
///実際の操作
///********************************
void operate() {
	while(1) {
		Serial.print("> ");
		String command = getInput();
		Serial.println("I received command: " + command);
		if(command == "exit") return;
		if(command == "restart") systemRestart();
		if(command == "rewifi") resetWiFi();
		if(command == "help") dispHelp();
		if(command == "getFC") dispFileContents();
		if(command == "getST") dispStartTime();
		if(command == "getDI") dispDirectoryInfo();
		if(command == "flush") deleteAllFiles();
		if(command == "delFL") deleteFile();
		if(command == "debug?") dispDebugLevel();
		if(command == "debugA") chgDebugLevel('a');
		if(command == "debugB") chgDebugLevel('b');
		if(command == "showFS") showFSInfo();
		if(command == "readWiFi") readWiFi();
		if(command == "setWiFi") setWiFi();
		if(command == "readSMTP") readSmtpParaFile();
		if(command == "setSMTP") setSmtpParaFile();
		if(command == "setSSS") setSSS(0);
		if(command == "setWISUN") setWI_SUN();
		if(command == "readWISUN") readWI_SUN();
		if(command == "showMA") showMA();
	}
}


void showMA() {
	byte mac[6];                     // the MAC address of your Wifi shield
	WiFi.macAddress(mac);
	Serial.print("CIPSTAMAC: ");
	Serial.print(mac[5],HEX);
	Serial.print(":");
	Serial.print(mac[4],HEX);
	Serial.print(":");
	Serial.print(mac[3],HEX);
	Serial.print(":");
	Serial.print(mac[2],HEX);
	Serial.print(":");
	Serial.print(mac[1],HEX);
	Serial.print(":");
	Serial.println(mac[0],HEX);

	WiFi.softAPmacAddress(mac);
	Serial.print("CIPAPMAC: ");
	Serial.print(mac[5],HEX);
	Serial.print(":");
	Serial.print(mac[4],HEX);
	Serial.print(":");
	Serial.print(mac[3],HEX);
	Serial.print(":");
	Serial.print(mac[2],HEX);
	Serial.print(":");
	Serial.print(mac[1],HEX);
	Serial.print(":");
	Serial.println(mac[0],HEX);
}


///************************************************
/// restart
///************************************************
void systemRestart(){
	ESP.restart();
	return;
}



///************************************************
/// reset WiFi
///************************************************
void resetWiFi(){
	WiFi.disconnect();
	wifiReStart();
	return;
}



///************************************************
/// delete all files
///************************************************
void deleteFile(){
	Serial.print("Input delete file number : ");
	String str = getInput();
	int fileNumber = str.toInt();
	Serial.println();
	String fileName = getDirInfo(fileNumber);
	Serial.print("delete file number is " + String(fileNumber) + " file name is " + fileName + " (y/n): ");
	str = getInput();
	if(str == "y"){
		if(SPIFFS.remove(fileName)) Serial.println("\r\nfile number " + String(fileNumber) + " " + fileName + " deleted"); 
		else Serial.println("\r\nfile number " + String(fileNumber) + " " + fileName + " delete failed.");
	}
	return;
}



///************************************************
/// delete all files
///************************************************
void deleteAllFiles() {
	Serial.print("delete all files (y/n) :");
	if(getInput() != "y") {
		Serial.println("\r\nstop delete !!!");
		return;
	}

	Serial.print("really delete all files (y/n) :");
	if(getInput() != "y") {
		Serial.println("\r\nOoops ...   stop delete !!!");
		return;
	}

	Dir dir = SPIFFS.openDir("/");

	while (dir.next()) {
		String tmpFileName = dir.fileName();
		if(SPIFFS.remove(tmpFileName)) wClient.println(tmpFileName + "  deleted");
	}

	Serial.print("sorry I delete all files.");
	return;
}



///************************************************
/// display directory information (file names)
///************************************************
void dispDirectoryInfo() {
	long a,b;			//dummy
	getDirInfo(&a, &b);
}



///*****************************
/// display file contents
///*****************************
void dispFileContents() {
	Serial.print("Input display file number : ");
	String sNum = getInput();
	int fileNumber = sNum.toInt();
	if(fileNumber == 0) {
		Serial.println("input number failed・・・");
		return;
	}

	Serial.println();
	String fileName = getDirInfo(fileNumber);
	if(fileName != "") {
		Serial.print("display file number is " + String(fileNumber) + " file name is " + fileName + " (y/n): ");
		if(getInput() != "y") {
			return;	
		}
		dispDebugFileContents(fileName);
		Serial.println();
	}
	else {
		Serial.println("wrong number !!\r\n");
	}

	return;
}




///*************************************
/// display the most recent start time
///*************************************
void dispStartTime() {
	Serial.println("m_startTime : " + m_startTime);
	return;
}





///*****************************
/// helpの表示
///*****************************
void dispHelp() {
	Serial.println("help: this display");
	Serial.println("exit:   exit command mode");
	Serial.println("getFC:  get file contents");
	Serial.println("getST:  get the most recent start time");
	Serial.println("getDI:  get directory infomation (get file names)");
	Serial.println("flush:  delete all files");
	Serial.println("delFL:  delete a file");
	Serial.println("debug?: display debug level");
	Serial.println("debugA: set debug level A");
	Serial.println("debugB: set debug level B");
	Serial.println("showFS: show FSInfo");
	Serial.println("rewifi: reset WiFi");
	Serial.println("readSMTP : read SMTP parametaers");
	Serial.println("setSMTP : change SMTP parametaers");
	Serial.println("setWiFi : change WiFi parametaers");
	Serial.println("readWiFi : read WiFi parametaers");
	Serial.println("setSSS : set SoftwareSerial speed for BP35A1(WI-SUN)");
	Serial.println("setWISUN : set WI-SUN(B ROOT) parametaers");
	Serial.println("readWISUN : read WI-SUN(B ROOT) parametaers");
	Serial.println("restart : system reset");
	Serial.println("showMA : show MAC ADDRESS");

	return;
}


///**********************************
/// display debug level
///**********************************
void dispDebugLevel() {
	Serial.println("debug mode : " + String(m_inputChar));

	return;
}


///**********************************
/// change debug level to dLevel
///**********************************
void chgDebugLevel(char dLevel) {
	m_inputChar = dLevel;
	Serial.println("debug mode : " + String(m_inputChar));

	return;
}


///**********************************
/// FSInfoの表示
///**********************************
void showFSInfo() {
	FSInfo fs_info;
	if(SPIFFS.info(fs_info)) {
	Serial.println("totalBytes    = " + String(fs_info.totalBytes));
	Serial.println("usedBytes     = " + String(fs_info.usedBytes));
	Serial.println("blockSize     = " + String(fs_info.blockSize));
	Serial.println("pageSize      = " + String(fs_info.pageSize));
	Serial.println("maxOpenFiles  = " + String(fs_info.maxOpenFiles));
	Serial.println("maxPathLength = " + String(fs_info.maxPathLength));
	}
	else Serial.println("get FSInfo failed・・・");

	return;
  }




///********************************
///入力コマンド＆データ取得
///********************************
String getInput() {
	uint32_t inTime = millis();
	String command = "";
	int iComCount = 0;

	while(1) {
		if(!LT(inTime,COMMAND_WAIT_TIME)) {
			Serial.println("command wait time is over");
			return "exit";
		}
		if(!(Serial.available() >0)) continue;
		else {
			char c = Serial.read();
			if(c == '\r') continue;
			if(c == '\n') {
				return command;
			}
			if(c == '\b') {
				Serial.print(" ");
				if(iComCount > 0) {
					iComCount--;
					command.remove(iComCount);
					Serial.print("\b");
				}
				continue;
			}
			command += String(c);
			iComCount++;
			if(iComCount == 256) {
				Serial.println("\r\nThe num of characters over\r\n");
				return "";
			}

		}
	}
}