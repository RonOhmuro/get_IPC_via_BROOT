eErrBroot brootBegin()
{
	eErrBroot hr = BROOT_SUCCEEDED;
 
	while(Serial.available() > 0) { Serial.read(); }
	Serial.println("B ROOT MODE IN\r\n");

	if(!readWI_SUN()) {
		Serial.println("READ WI-SUN FILE FALED (" + String(WI_SUN_PARAM_FILE) + ")");
		hr = READ_WI_SUN_FILE_FALED;
		return hr;
	}

	SoftSerial.println();
	while(SoftSerial.available() > 0) {	SoftSerial.read(); }

	String str = "";
	//Serial.println("SKVER");
	SoftSerial.println("SKVER");
	if(!wait("OK",10000)) {
		Serial.println("SKVER COMMAND FAILED");
		hr = SKVER_COMMAND_FAILED;
		return hr;
	}
	Serial.println("SKVER COMMAND SUCCEEDED\r\n");
	delay(500);

	Serial.println("SKSETPWD C " + String(m_wi_sunPassword));
	SoftSerial.println("SKSETPWD C " + String(m_wi_sunPassword));
	if(!wait("OK",10000)) {
		Serial.println("SKSETPWD COMMAND FAILED");
		hr = SKSETPWD_COMMAND_FAILED;
		return hr;
	}
	Serial.println("SKSETPWD COMMAND SUCCEEDED\r\n");
	delay(500);

	Serial.println("SKSETRBID " + String(m_authenticationID));
	SoftSerial.println("SKSETRBID " + String(m_authenticationID));
	if(!wait("OK",10000)) {
		Serial.println("SKSETRBID COMMAND FAILED");
		hr = SKSETRBID_COMMAND_FAILED;
		return hr;
	}
	Serial.println("SKSETRBID COMMAND SUCCEEDED\r\n");
	delay(500);


	String argStr;
	String channel;
	String panID;
	String addr;
	int scanLimit = 10;
	int loopCount = 0;
	while(loopCount < scanLimit) 
	{
		SoftSerial.println("SKSCAN 2 FFFFFFFF 6");

		if(!wait("Channel:",180000,&argStr)) {
			if(argStr.startsWith("EVENT 22")) {
				loopCount++;
				if(loopCount < scanLimit) {
					Serial.println("RETRY SKSCAN");
					continue;
				}
			}
			Serial.println("SKSCAN FAILED");
			hr = SKSCAN_FAILED;
			return hr;
		}
		argStr = argStr.substring(argStr.indexOf("Channel"));
		unsigned int from = argStr.indexOf(":") + 1;
		channel = argStr.substring(from, from + 2);
		argStr = argStr.substring(argStr.indexOf("Pan ID"));
		from = argStr.indexOf(":") + 1;
		panID = argStr.substring(from, from + 4);
		argStr = argStr.substring(argStr.indexOf("Addr"));
		from = argStr.indexOf(":") + 1;
		addr = argStr.substring(from, from + 16);

		break;
 	}
	Serial.println("SKSCAN COMMAND SUCCEEDED\r\n");
	if(!wait("EVENT 22",15000)) {
		Serial.println("TO GET SCAN END FAILED");
		hr = TO_GET_SCAN_END_FAILED;
		return hr;
	}
	delay(300);
	


	SoftSerial.println("SKSREG S2 " + channel);
	if(!wait("OK",10000)) {
		Serial.println("SKSREG S2 COMMAND FAILED");
		hr = SKSREG_S2_COMMAND_FAILED;
		return hr;
	}
	Serial.println("SKSREG S2 COMMAND SUCCEEDED\r\n");
	delay(500);
	
	SoftSerial.println("SKSREG S3 " + panID);
	if(!wait("OK",10000)) {
		Serial.println("SKSREG S3 COMMAND FAILED");
		hr = SKSREG_S3_COMMAND_FAILED;
		return hr;
	}
	Serial.println("SKSREG S3 COMMAND SUCCEEDED\r\n");
	

	String ipv6Addr;
	SoftSerial.println("SKLL64 " + addr);
	if(!wait(":" + panID,10000,&ipv6Addr)) {
		Serial.println("SKLL64 COMMAND FAILED");
		hr = SKLL64_COMMAND_FAILED;
		return hr;
	}
	if(ipv6Addr.indexOf("SKLL64") != -1) {
		m_ipv6Addr = ipv6Addr.substring(ipv6Addr.indexOf(":") - 4, ipv6Addr.indexOf(":") + (5 * 7));;
		Serial.println("SKLL64 COMMAND SUCCEEDED\r\nget " + m_ipv6Addr);
	}
	else {
		Serial.println("SKLL64 COMMAND FAILED");
		hr = SKLL64_COMMAND_FAILED;
		return hr;
	}
	delay(300);


	int joinTryCount = 0;
	int joinTryCountLimit = 3;
	while(joinTryCount < joinTryCountLimit) {
		SoftSerial.println("SKJOIN " + m_ipv6Addr);
		if(wait("EVENT 25",60000)) {
			Serial.println("SKJOIN COMMAND SUCCEEDED\r\n");
			break;
		}
		joinTryCount++;
		if(joinTryCount < joinTryCountLimit) Serial.println("SKJOIN COMMAND RETRY");
	}
	if(joinTryCount == joinTryCountLimit){
		Serial.println("SKJOIN COMMAND FAILED");
		hr = SKJOIN_COMMAND_FAILED;
		return hr;
 	}

 	return hr;
}




void bROOTTERM() {
	SoftSerial.println("SKTERM");
	if(wait("EVENT 27",60000)) {
		Serial.println("SKJOIN COMMAND SUCCEEDED\r\n");
	}
	else {
		Serial.println("SKTERM COMMAND FAILED");
	}
	return;
}





eErrBroot getInstantaneousPower(long *retValue) {
	eErrBroot hr = eErrBroot(0);

	int iDataStrLen = 14;
	char dataStr[iDataStrLen];
	dataStr[0] = char(0x10); dataStr[1] = char(0x81);			//EHD (参考:EL p.3-2)
	dataStr[2] = char(0x00); dataStr[3] = char(0x01);			//TID (参考:EL p.3-3)

	dataStr[4] = char(0x05); dataStr[5] = char(0xff); dataStr[6] = char(0x01);			//SEOJ (参考:EL p.3-3 AppH p.3-408～)
	dataStr[7] = char(0x02); dataStr[8] = char(0x88); dataStr[9] = char(0x01);			//DEOJ (参考:EL p.3-3 AppH p.3-274～)
	dataStr[10] = char(0x62);			//ESV(62:プロパティ値読み出し要求) (参考:EL p.3-5)

	dataStr[11] = char(0x01);			//OPC(1個)(参考:EL p.3-7)
	dataStr[12] = char(0xe7);			//EPC(参考:EL p.3-7 AppH p.3-275)
	dataStr[13] = char(0x00);			//PDC(参考:EL p.3-9)

	String dataStrLen = String(iDataStrLen,HEX);
	int strLength = dataStrLen.length();

	for(int i = 0; i < 4 - strLength; i++) dataStrLen = "0" + dataStrLen;  //<= '0' ではなく "0" か？

	String comStr = "SKSENDTO 1 " + m_ipv6Addr + " 0E1A 1 " + dataStrLen + " ";
	byte comBytes[1024];
	comStr.getBytes(comBytes,comStr.length() + 1);
	for(int i = 0; i < iDataStrLen; i++) comBytes[comStr.length() + i] = dataStr[i];

	do{
		String IPMV;				//Instantaneous power measurement value(瞬時電力計測値)
		SoftSerial.write(comBytes, comStr.length() + iDataStrLen);
		SoftSerial.println();
		String eoj = "1081000102880105FF01";
		if(!wait(eoj,30000, &IPMV)) {
			Serial.println("SKSENDTO COMMAND FAILED");
			*retValue = 99996;
			hr = SKSENDTO_COMMAND_FAILED;
			break;
		}
		IPMV = IPMV.substring(IPMV.indexOf(eoj) + eoj.length());
		Serial.println("eoj = " + IPMV);
		if(!(IPMV.indexOf("72") != -1 && IPMV.indexOf("E7") != -1)) {
			Serial.println("SKSENDTO COMMAND FAILED");
			*retValue = 99996;
			hr = SKSENDTO_COMMAND_FAILED;
			break;
		}
		Serial.println("SSKSENDTO COMMAND SUCCEEDED\r\n");

		int startData = IPMV.indexOf("E7") + 4;
		String hexPower = IPMV.substring(startData, startData + 8);
		Serial.println(hexPower);
		unsigned int hexPowerChar = hexToDec(hexPower);
		Serial.println("瞬時電力計測値: " + String(hexPowerChar) + "W");
 
		*retValue = (long)hexPowerChar;

	} while(false);

	return hr;
}



eErrBroot getIntegralPowerConsumption(String *IPC)
{
	eErrBroot hr = BROOT_SUCCEEDED;

	*IPC = "";

	{
		int iDataStrLen = 15;
		char dataStr[iDataStrLen];
		dataStr[0] = char(0x10); dataStr[1] = char(0x81);			//EHD (参考:EL p.3-2)
		dataStr[2] = char(0x00); dataStr[3] = char(0x01);			//TID (参考:EL p.3-3)

		dataStr[4] = char(0x05); dataStr[5] = char(0xff); dataStr[6] = char(0x01);			//SEOJ (参考:EL p.3-3 AppH p.3-408～)
		dataStr[7] = char(0x02); dataStr[8] = char(0x88); dataStr[9] = char(0x01);			//DEOJ (参考:EL p.3-3 AppH p.3-274～)
		dataStr[10] = char(0x61);			//ESV(62:プロパティ値読み出し要求) (参考:EL p.3-5)

		dataStr[11] = char(0x01);			//OPC(1個)(参考:EL p.3-7)
		dataStr[12] = char(0xe5);			//EPC(参考:EL p.3-7 AppH p.3-275)
		dataStr[13] = char(0x01);			//PDC(参考:EL p.3-9)
		dataStr[14] = char(0x01);			//EDT(参考:EL p.3-9)

		String dataStrLen = String(iDataStrLen,HEX);
		int strLength = dataStrLen.length();

		for(int i = 0; i < 4 - strLength; i++) dataStrLen = "0" + dataStrLen;  //<= '0' ではなく "0" か？

		Serial.println(dataStr);
		Serial.println(dataStrLen);

		String comStr = "SKSENDTO 1 " + m_ipv6Addr + " 0E1A 1 " + dataStrLen + " ";
		byte comBytes[1024];
		comStr.getBytes(comBytes,comStr.length() + 1);
		for(int i = 0; i < iDataStrLen; i++) comBytes[comStr.length() + i] = dataStr[i];

		do{
			String IPMV;				//Instantaneous power measurement value(瞬時電力計測値)
			SoftSerial.write(comBytes, comStr.length() + iDataStrLen);
			SoftSerial.println();
			String eoj = "1081000102880105FF01";
			if(!wait(eoj,30000, &IPMV)) {
				Serial.println("SKSENDTO COMMAND (積算履歴収集日1設定) FAILED");
				hr = SKSENDTO_COMMAND_FAILED;
				break;
			}
			IPMV = IPMV.substring(IPMV.indexOf(eoj) + eoj.length());
			Serial.println("EDATA = " + IPMV);
			if(!(IPMV.indexOf("71") != -1 || IPMV.indexOf("51") != -1)) {
				Serial.println("SKSENDTO COMMAND (積算履歴収集日1設定) FAILED");
				hr = SKSENDTO_COMMAND_FAILED;
				break;
			}

			String hOPC = IPMV.substring(2,4);
			unsigned int iOPC = hexToDec(hOPC);
			Serial.println("OPC = " + String(iOPC));
			IPMV = IPMV.substring(4);

			int startData = IPMV.indexOf("E5") + 2;
			int endData = startData + 2;
			String hPDC = IPMV.substring(startData, endData);
			if(hPDC == "00") Serial.println("SSKSENDTO COMMAND (積算履歴収集日1設定) SUCCEEDED\r\n");
			else Serial.println("SSKSENDTO COMMAND (積算履歴収集日1設定) FAILED\r\n");
		} while(false);
		if(hr != BROOT_SUCCEEDED) {
			*IPC = "1 SET REQUEST : E50101 FAILED";
			return hr;
		}
	}

	{
		int iDataStrLen = 22;
		char dataStr[iDataStrLen];
		dataStr[0] = char(0x10); dataStr[1] = char(0x81);			//EHD (参考:EL p.3-2)
		dataStr[2] = char(0x00); dataStr[3] = char(0x01);			//TID (参考:EL p.3-3)

		dataStr[4] = char(0x05); dataStr[5] = char(0xff); dataStr[6] = char(0x01);			//SEOJ (参考:EL p.3-3 AppH p.3-408～)
		dataStr[7] = char(0x02); dataStr[8] = char(0x88); dataStr[9] = char(0x01);			//DEOJ (参考:EL p.3-3 AppH p.3-274～)
		dataStr[10] = char(0x62);			//ESV(62:プロパティ値読み出し要求) (参考:EL p.3-5)

		dataStr[11] = char(0x05);			//OPC(1個)(参考:EL p.3-7)
		dataStr[12] = char(0xe1);			//EPC(参考:EL p.3-7 AppH p.3-275)
		dataStr[13] = char(0x00);			//PDC(参考:EL p.3-9)

		dataStr[14] = char(0xd3);			//EPC(参考:EL p.3-7 AppH p.3-275)
		dataStr[15] = char(0x00);			//PDC(参考:EL p.3-9)

		dataStr[16] = char(0xd7);			//EPC(参考:EL p.3-7 AppH p.3-275)
		dataStr[17] = char(0x00);			//PDC(参考:EL p.3-9)

		dataStr[18] = char(0xe0);			//EPC(参考:EL p.3-7 AppH p.3-275)
		dataStr[19] = char(0x00);			//PDC(参考:EL p.3-9)

		dataStr[20] = char(0xe5);			//EPC(参考:EL p.3-7 AppH p.3-275)
		dataStr[21] = char(0x00);			//PDC(参考:EL p.3-9)

		String dataStrLen = String(iDataStrLen,HEX);
		int strLength = dataStrLen.length();

		for(int i = 0; i < 4 - strLength; i++) dataStrLen = "0" + dataStrLen;  //<= '0' ではなく "0" か？

		Serial.println(dataStr);
		Serial.println(dataStrLen);

		String comStr = "SKSENDTO 1 " + m_ipv6Addr + " 0E1A 1 " + dataStrLen + " ";
		byte comBytes[1024];
		comStr.getBytes(comBytes,comStr.length() + 1);
		for(int i = 0; i < iDataStrLen; i++) comBytes[comStr.length() + i] = dataStr[i];

		do{
			String IPMV;				//Instantaneous power measurement value(瞬時電力計測値)
			SoftSerial.write(comBytes, comStr.length() + iDataStrLen);
			SoftSerial.println();
			String eoj = "1081000102880105FF01";
			if(!wait(eoj,30000, &IPMV)) {
				Serial.println("SKSENDTO COMMAND (5 RESPONSE REQUIREMENT : E100D300D700E000E500) FAILED");
				hr = SKSENDTO_COMMAND_FAILED;
				break;
			}
			IPMV = IPMV.substring(IPMV.indexOf(eoj) + eoj.length());
			Serial.println("EDATA = " + IPMV);
			if(!(IPMV.indexOf("72") != -1 || IPMV.indexOf("52") != -1)) {
				Serial.println("SKSENDTO COMMAND (5 RESPONSE REQUIREMENT : E100D300D700E000E500) FAILED");
				hr = SKSENDTO_COMMAND_FAILED;
				break;
			}
			Serial.println("SSKSENDTO COMMAND (5 RESPONSE REQUIREMENT : E100D300D700E000E500) 受付を確認\r\n");

			String hOPC = IPMV.substring(2,4);
			unsigned int iOPC = hexToDec(hOPC);
			Serial.println("OPC = " + String(iOPC));
			IPMV = IPMV.substring(4);

			int startData = IPMV.indexOf("E1") + 2;
			int endData = startData + 2;
			String hPDC = IPMV.substring(startData, endData);
			unsigned int iPDC = hexToDec(hPDC);
			Serial.print("PDC = "); Serial.println(iPDC);
			startData = endData;
			endData = startData + 2 * iPDC;
			String hexPower = IPMV.substring(startData, endData);
			Serial.print("EDT = "); Serial.println(hexPower);
			unsigned int hexPowerChar = hexToDec(hexPower);
			Serial.println("積算電力量単位: " + String(hexPowerChar));
			m_fUnit = 0;
			switch(hexPowerChar) {
				case 0:
					m_fUnit = 1.0;
					break;
				case 1:
					m_fUnit = 0.1;
					break;
				case 2:
					m_fUnit = 0.01;
					break;
				case 3:
					m_fUnit = 0.001;
					break;
				case 4:
					m_fUnit = 0.0001;
					break;
				case 10:
					m_fUnit = 10;
					break;
				case 11:
					m_fUnit = 100;
					break;
				case 12:
					m_fUnit = 1000;
					break;
				case 13:
					m_fUnit = 10000;
					break;
			}

			IPMV = IPMV.substring(endData);
			startData = IPMV.indexOf("D3") + 2;
			endData = startData + 2;
			hPDC = IPMV.substring(startData, endData);
			iPDC = hexToDec(hPDC);
			Serial.print("PDC = "); Serial.println(iPDC);
			startData = endData;
			endData = startData + 2 * iPDC;
			hexPower = IPMV.substring(startData, endData);
			Serial.print("EDT = "); Serial.println(hexPower);
			hexPowerChar = hexToDec(hexPower);
			Serial.println("係数: " + String(hexPowerChar));
			m_fCoefficient = float(hexPowerChar);

			IPMV = IPMV.substring(endData);
			startData = IPMV.indexOf("D7") + 2;
			endData = startData + 2;
			hPDC = IPMV.substring(startData, endData);
			iPDC = hexToDec(hPDC);
			Serial.print("PDC = "); Serial.println(iPDC);
			startData = endData;
			endData = startData + 2 * iPDC;
			hexPower = IPMV.substring(startData, endData);
			Serial.print("EDT = "); Serial.println(hexPower);
			hexPowerChar = hexToDec(hexPower);
			Serial.println("積算電力有効桁数: " + String(hexPowerChar));

			IPMV = IPMV.substring(endData);
			startData = IPMV.indexOf("E0") + 2;
			endData = startData + 2;
			hPDC = IPMV.substring(startData, endData);
			iPDC = hexToDec(hPDC);
			Serial.print("PDC = "); Serial.println(iPDC);
			startData = endData;
			endData = startData + 2 * iPDC;
			hexPower = IPMV.substring(startData, endData);
			Serial.print("EDT = "); Serial.println(hexPower);
			hexPowerChar = hexToDec(hexPower);
			Serial.println("積算電力量計測値（正方向計測値） 係数、単位乗算前: " + String(hexPowerChar));
			Serial.println("積算電力量計測値（正方向計測値）: " + String(float(hexPowerChar) * m_fCoefficient * m_fUnit) + " kWh");

			IPMV = IPMV.substring(endData);
			startData = IPMV.indexOf("E5") + 2;
			endData = startData + 2;
			hPDC = IPMV.substring(startData, endData);
			iPDC = hexToDec(hPDC);
			Serial.print("PDC = "); Serial.println(iPDC);
			startData = endData;
			endData = startData + 2 * iPDC;
			hexPower = IPMV.substring(startData, endData);
			Serial.print("EDT = "); Serial.println(hexPower);
			hexPowerChar = hexToDec(hexPower);
			Serial.println("積算履歴収集日1: " + String(hexPowerChar)); 
		} while(false);
		if(hr != BROOT_SUCCEEDED) {
			*IPC = "5 RESPONSE REQUIREMENT : E100D300D700E000E500 FAILED";
			return hr;
		}

		iDataStrLen = 14;
		dataStr[11] = char(0x01);			//OPC(1個)(参考:EL p.3-7)
		dataStr[12] = char(0xe2);			//EPC(参考:EL p.3-7 AppH p.3-275)
		dataStr[13] = char(0x00);			//PDC(参考:EL p.3-9)

		dataStrLen = String(iDataStrLen,HEX);
		strLength = dataStrLen.length();

		for(int i = 0; i < 4 - strLength; i++) dataStrLen = "0" + dataStrLen;  //<= '0' ではなく "0" か？

		Serial.println(dataStr);
		Serial.println(dataStrLen);

		comStr = "SKSENDTO 1 " + m_ipv6Addr + " 0E1A 1 " + dataStrLen + " ";
		Serial.println(comStr);

		comBytes[1024];
		comStr.getBytes(comBytes,comStr.length() + 1);
		for(int i = 0; i < iDataStrLen; i++) comBytes[comStr.length() + i] = dataStr[i];

		do{
			String IPMV;				//Instantaneous power measurement value(瞬時電力計測値)
			SoftSerial.write(comBytes, comStr.length() + iDataStrLen);
			SoftSerial.println();
			String eoj = "1081000102880105FF01";
			if(!wait(eoj,180000, &IPMV)) {
				Serial.println("SKSENDTO COMMAND FAILED");
				hr = SKSENDTO_COMMAND_FAILED;
				break;
			}
			IPMV = IPMV.substring(IPMV.indexOf(eoj) + eoj.length());
			Serial.println("EDATA = " + IPMV);
			if(!(IPMV.indexOf("72") != -1 || IPMV.indexOf("52") != -1)) {
				Serial.println("SKSENDTO COMMAND FAILED");
				hr = SKSENDTO_COMMAND_FAILED;
				break;
			}
			Serial.println("SSKSENDTO COMMAND SUCCEEDED\r\n");

			int startData = IPMV.indexOf("E2") + 2;
			int endData = startData + 2;
			String hPDC = IPMV.substring(startData, endData);
			unsigned int iPDC = hexToDec(hPDC);
			Serial.print("PDC = "); Serial.println(iPDC);
			startData = endData;
			endData = startData + 2 * iPDC;
			IPMV = IPMV.substring(startData, endData);
			Serial.print("EDT = "); Serial.println(IPMV);
			startData = 0;
			endData = 4;
			String hexadecimal = IPMV.substring(startData, endData);
			Serial.print("積算履歴収集日1 = "); Serial.println(hexadecimal);
			IPMV = IPMV.substring(endData);
			String sDum = IPMV;
			*IPC = sDum;
			endData = 8;
			for(int i = 0; i < 48; i++) {
				String hPower = IPMV.substring(startData, endData);
				unsigned int iPower = hexToDec(hPower);
				Serial.println(String(float(i / 2.0)) + ": " + String(float(iPower) * m_fCoefficient * m_fUnit) + " kWh");
				delay(1);
				IPMV = IPMV.substring(endData);
			}
		} while(false);
	}

	return hr;
}


void getBroot(String *IPC)
{
	*IPC = "";
 	eErrBroot result;
 	result = getIntegralPowerConsumption(IPC);
 	Serial.println(*IPC);
}


bool wait(String argStr,uint32_t waitTime)
{
	String str;

	return wait(argStr, waitTime, &str);
}

bool wait(String argStr, uint32_t wait, String *str)
{
	uint32_t inTime = millis();

	delay(100);
	while(LT(inTime,wait)) {
		*str = SoftSerial.readStringUntil('\0');
		if(str->length() == 0) {
			//delay(100);
			continue;
		}
		Serial.println(*str);
		Serial.println();
		delay(1);
		if(str->indexOf(argStr) != -1) {
 			return true;
 		}
		if(str->indexOf("EVENT 24") != -1) break;
		if(str->indexOf("EVENT 22") != -1) break;
	}
	return false;
}

bool wait2(String argStr, uint32_t wait, String *str)
{
	uint32_t inTime = millis();

	delay(100);
	while(LT(inTime,wait)) {
		*str = SoftSerial.readStringUntil('\0');
		if(str->length() == 0) {
			//delay(100);
			continue;
		}
		Serial.println(*str);
		Serial.println();
		delay(1);
		if(str->indexOf(argStr) != -1) {
 			return true;
 		}
		if(str->indexOf("EVENT 24") != -1) break;
		if(str->indexOf("EVENT 22") != -1) break;
		if(str->indexOf("FAIL") != -1) break;
	}
	return false;
}




// Converting from Hex to Decimal:
//
// NOTE: This function can handle a positive hex value from 0 - 65,535 (a four digit hex string).
//       For larger/longer values, change "unsigned int" to "long" in both places.
unsigned int hexToDec(String hexString) {
  
  unsigned int decValue = 0;
  int nextInt;
  
  for (int i = 0; i < hexString.length(); i++) {
    
    nextInt = int(hexString.charAt(i));
    if (nextInt >= 48 && nextInt <= 57) nextInt = map(nextInt, 48, 57, 0, 9);		//0-9
    if (nextInt >= 65 && nextInt <= 70) nextInt = map(nextInt, 65, 70, 10, 15);		//A-F
    if (nextInt >= 97 && nextInt <= 102) nextInt = map(nextInt, 97, 102, 10, 15);	//a-f
    nextInt = constrain(nextInt, 0, 15);
    decValue = (decValue * 16) + nextInt;
    SoftSerial.println(decValue);
  }
  
  return decValue;
}


















///************************************************
/// setSSS
/// set SoftwareSerial speed
///************************************************
void setSSS(long spd){
	int spdMode;

	if(spd == 0) {
		Serial.print("speed : ");
		String str = getInput();
		spd = (long)str.toInt();
	}
	else {
		SoftSerial.begin(spd);
		SoftSerial.println();
		delay(10);
		SoftSerial.println("SKVER");
		if(wait("OK",10000)) {
			return;
		}
	}

	spdMode = -1;
	switch (spd) {
		case 2400:
			spdMode = 1;
			break;
		case 4800:
			spdMode = 2;
			break;
		case 9600:
			spdMode = 3;
			break;
		case 19200:
			spdMode = 4;
			break;
		case 38400:
			spdMode = 5;
			break;
		case 57600:
			spdMode = 6;
			break;
		case 115200:
			spdMode = 0;
			break;
		default:
			Serial.println("input value wrong");
			break;
	}

	if(spdMode == -1) {
		return;
	}
	else {
		SoftSerial.println();
		delay(10);
		SoftSerial.println("SKVER");
		if(wait("OK",10000)) {
			SoftSerial.println();
			delay(10);
			SoftSerial.println("WUART " + String(spdMode));
			wait("OK",5000);
		}
		else {
			long speed[] = {115200,2400,4800,9600,19200,38400,57600};
			Serial.println("did not answer OK\r\nAttempting to connect BP35A1");
			for(int mode = 6; mode >= 0; mode--) {
				SoftSerial.begin(speed[mode]);
				SoftSerial.println();
				delay(10);
				SoftSerial.println("SKVER");
				if(wait("OK",5000)) {
					SoftSerial.println();
					delay(10);
					SoftSerial.println("WUART " + String(spdMode));
					if(wait("OK",5000)) break;
				}
				Serial.println(String(speed[mode]) + " : FAILED");
			}
		}
		SoftSerial.begin(spd);
		digitalWrite(5, LOW); delay(5); digitalWrite(5, HIGH);delay(10000);
		delay(10);
		SoftSerial.println("SKVER");
		if(!wait("OK",10000)) {
			Serial.println("speed change failed\r\nMAY BE YOU HAVE TO DO SOMETHING");
			return;
		}
		Serial.println("OK");
		return;
	}
}



void resetWI_SUN() {
	debugWrite("resetWI_SUN : " + m_hour + ":" + m_minute + ":" + m_second);
	digitalWrite(5, LOW); delay(5); digitalWrite(5, HIGH);
	Serial.setTimeout(300);
	String str;
	while(1) {
		str = SoftSerial.readStringUntil('\0');
		if(str.length() == 0) {
			SoftSerial.println("SKVER");
			if(wait("OK",5000)) break;
		}
		delay(10);
	}

	return;
}