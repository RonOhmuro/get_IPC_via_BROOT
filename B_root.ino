const static int m_scanLimit = 20;		//アクティブスキャンのリトライ回数
const static int m_joinTryCountLimit = 3;	//SKJOIN（Pac（PANA認証クライアント）としてのPANA接続シーケンス）のリトライ回数



eErrBroot brootBegin()
{
	eErrBroot hr = BROOT_SUCCEEDED;
 

	//SoftwareSerialに残った余計なデータを一度吐き出させる
	SoftSerial.println();
	while(SoftSerial.available() > 0) {	SoftSerial.readStringUntil('\0'); }

	Serial.println("B ROOT MODE IN\r\n");

	//B ROOTのIDとパスワードをファイルから取り出す
	if(!readWI_SUN()) {
		Serial.println("READ WI-SUN FILE FALED (" + String(WI_SUN_PARAM_FILE) + ")");
		hr = READ_WI_SUN_FILE_FALED;
		return hr;
	}


	//B35A1にB ROOTパスワードを設定
	SoftSerial.println("SKSETPWD C " + String(m_wi_sunPassword));
	if(!wait("OK",10000)) {
		Serial.println("SKSETPWD COMMAND FAILED");
		hr = SKSETPWD_COMMAND_FAILED;
		return hr;
	}
	Serial.println("SKSETPWD COMMAND SUCCEEDED\r\n");


	//B35A1にB ROOTIDを設定
	SoftSerial.println("SKSETRBID " + String(m_authenticationID));
	if(!wait("OK",10000)) {
		Serial.println("SKSETRBID COMMAND FAILED");
		hr = SKSETRBID_COMMAND_FAILED;
		return hr;
	}
	Serial.println("SKSETRBID COMMAND SUCCEEDED\r\n");

	//再認識シーケンスを開始してみて再認識されれば、接続手続きを割愛する
	SoftSerial.println("SKREJOIN");
	String reIpv6Addr;
	if(wait2("EVENT 25",10000,&reIpv6Addr)) {
		m_ipv6Addr = reIpv6Addr.substring(reIpv6Addr.indexOf(":") - 4, reIpv6Addr.indexOf(":") + (5 * 7));
		Serial.println("REJOIN TO " + m_ipv6Addr + " SUCCEEDED");
		return hr;
	}


	//アクティブスキャンして接続先を探す
	String argStr;
	String channel;
	String panID;
	String addr;
	int loopCount = 0;
	while(loopCount < m_scanLimit) 
	{
		//PairIDは、B ROOT IDの末尾8バイトが自動的に設定される
		SoftSerial.println("SKSCAN 2 FFFFFFFF 3");

		//拡張ビーコン応答（"EPANDESC"）を待つ
		if(!wait("EPANDESC",180000,&argStr)) {
			//もし拡張ビーコン応答より先に"EVENT 22"（スキャン終了）が返ってきたらリトライする
			if(argStr.startsWith("EVENT 22")) {
				loopCount++;
				if(loopCount < m_scanLimit) {
					Serial.println("RETRY SKSCAN");
					continue;
				}
			}
			//m_scanLimit回スキャンしても拡張ビーコン応答が返ってこなかったら
			Serial.println("SKSCAN FAILED");
			hr = SKSCAN_FAILED;
			return hr;
		}

		//拡張ビーコン応答の中からChannelとPan IDとAddrの値を取り出す
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
	//アクティブスキャンが完全に終了するまで待つ
	if(!wait("EVENT 22",15000)) {
		Serial.println("TO GET SCAN END FAILED");
		hr = TO_GET_SCAN_END_FAILED;
		return hr;
	}
	

	//仮想レジスタのレジスタ番号S2に拡張ビーコン応答で得られたチャンネル（自端末が使用する周波数の論理チャンネル番号）を設定する
	SoftSerial.println("SKSREG S2 " + channel);
	if(!wait("OK",10000)) {
		Serial.println("SKSREG S2 COMMAND FAILED");
		hr = SKSREG_S2_COMMAND_FAILED;
		return hr;
	}
	Serial.println("SKSREG S2 COMMAND SUCCEEDED\r\n");

	//仮想レジスタのレジスタ番号S3に拡張ビーコン応答で得られたPan IDを設定する	
	SoftSerial.println("SKSREG S3 " + panID);
	if(!wait("OK",10000)) {
		Serial.println("SKSREG S3 COMMAND FAILED");
		hr = SKSREG_S3_COMMAND_FAILED;
		return hr;
	}
	Serial.println("SKSREG S3 COMMAND SUCCEEDED\r\n");
	
	//拡張ビーコン応答で得られたAddrからIPv6リンクローカルアドレスに変換する
	String ipv6Addr;
	SoftSerial.println("SKLL64 " + addr);
	if(!wait(":" + panID,10000,&ipv6Addr)) {
		Serial.println("SKLL64 COMMAND FAILED");
		hr = SKLL64_COMMAND_FAILED;
		return hr;
	}
	if(ipv6Addr.indexOf("SKLL64") != -1) {
		m_ipv6Addr = ipv6Addr.substring(ipv6Addr.indexOf(":") - 4, ipv6Addr.indexOf(":") + (5 * 7));
		Serial.println("SKLL64 COMMAND SUCCEEDED\r\nget " + m_ipv6Addr);
	}
	else {
		Serial.println("SKLL64 COMMAND FAILED");
		hr = SKLL64_COMMAND_FAILED;
		return hr;
	}


	//PANA認証クライアント）としてのPANA接続シーケンスを開始する
	int joinTryCount = 0;
	while(joinTryCount < m_joinTryCountLimit) {
		SoftSerial.println("SKJOIN " + m_ipv6Addr);
		if(wait("EVENT 25",60000)) {
			Serial.println("SKJOIN COMMAND SUCCEEDED\r\n");
			break;
		}
		joinTryCount++;
		if(joinTryCount < m_joinTryCountLimit) Serial.println("SKJOIN COMMAND RETRY");
	}
	if(joinTryCount == m_joinTryCountLimit){
		Serial.println("SKJOIN COMMAND FAILED");
		hr = SKJOIN_COMMAND_FAILED;
		return hr;
 	}

 	return hr;
}





//PANA接続を解除する
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

	for(int i = 0; i < 4 - strLength; i++) dataStrLen = "0" + dataStrLen;  //<= '0' ではなく "0" 

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

	//変数の宣言を冗長に行うため、{}でスコープを分離している
	{
		int iDataStrLen = 15;	//EDATAのバイト数
		char dataStr[iDataStrLen];	//EDATAのcharバッファ
		dataStr[0] = char(0x10);			//EHD1（ECHONET Liteヘッダ1）:ECHONET Lite規格を示す
		dataStr[1] = char(0x81);			//EHD2:EDATA部の電文形式がECHONET Lite仕様で定義する電文形式（規定電文形式）であることを示す
		dataStr[2] = char(0x00); dataStr[3] = char(0x01);			//TID (参考:EL p.3-3)

		dataStr[4] = char(0x05); dataStr[5] = char(0xff); dataStr[6] = char(0x01);			//SEOJ コントローラクラス
		dataStr[7] = char(0x02); dataStr[8] = char(0x88); dataStr[9] = char(0x01);			//DEOJ 低圧スマート電力量メータクラス
		dataStr[10] = char(0x61);			//ESV(61:プロパティ値書き込み要求) (参考:EL p.3-5)

		dataStr[11] = char(0x01);			//OPC(1個)(参考:EL p.3-7)
		dataStr[12] = char(0xe5);			//EPC(参考:EL p.3-7 AppH p.3-275)(E5:積算履歴収集日1)
		dataStr[13] = char(0x01);			//PDC(参考:EL p.3-9)
		dataStr[14] = char(0x01);			//EDT(参考:EL p.3-9)(1日前)

		String dataStrLen = String(iDataStrLen,HEX);
		//EDATAの長さを4文字で指定するために文字数を調整
		int strLength = dataStrLen.length();
		for(int i = 0; i < 4 - strLength; i++) dataStrLen = "0" + dataStrLen;  //<= '0' ではなく "0"

		//スマートメータにコマンドを送る準備（0E1AはUDPポート番号）
		//dataStr部はバイナリで送るため、文字で指定する部分とバイナリ部を接続したbyte配列を作る
		String comStr = "SKSENDTO 1 " + m_ipv6Addr + " 0E1A 1 " + dataStrLen + " ";
		byte comBytes[1024];
		comStr.getBytes(comBytes,comStr.length() + 1);
		for(int i = 0; i < iDataStrLen; i++) comBytes[comStr.length() + i] = dataStr[i];

		do{
			String IPMV;				//Instantaneous power measurement value(瞬時電力計測値)
			//SKSENDTOのコマンドをバイナリで書き込み
			SoftSerial.write(comBytes, comStr.length() + iDataStrLen);
			//BP35A1にコマンド列の終端を認識させるため、改行文字（"\r\n"）を送る
			SoftSerial.println();
			//UDPに多くのデータが流れ込むため、決まった応答が返ってくるまで待つ
			//決まった応答とはEHD1,EHD2に続き送信時のSEOJとDEOJの順番が入れ替わった応答時のSEOJ(028801)とDEOJ(05FF01)だ
			String eoj = "1081000102880105FF01";
			if(!wait(eoj,30000, &IPMV)) {
				Serial.println("SKSENDTO COMMAND (積算履歴収集日1設定) FAILED");
				hr = SKSENDTO_COMMAND_FAILED;
				break;
			}
			IPMV = IPMV.substring(IPMV.indexOf(eoj) + eoj.length());
			Serial.println("EDATA = " + IPMV);
			//応答に71:要求受付か51:不可応答がある。
			//ここでは書き込み要求に1つのEPCしか入れてないので、71が返ってくることを期待している。
			//将来多くのEPCを付けて書き込み要求(0x61)を出した時に51が返って来ることもあるので、冗長なコードにしてみた
			if(!(IPMV.indexOf("71") != -1 || IPMV.indexOf("51") != -1)) {
				Serial.println("SKSENDTO COMMAND (積算履歴収集日1設定) FAILED");
				hr = SKSENDTO_COMMAND_FAILED;
				break;
			}

			//以下は書き込み要求を1つしか出していないのに71と不可応答51を見ているので、要求を受け付けたかどうかEDATAの中身を解析している
			String hOPC = IPMV.substring(2,4);
			unsigned int iOPC = hexToDec(hOPC);		//EPCの個数をここで拾っているので、この後個数分繰り返して詳細な応答を解析することも出来る
			IPMV = IPMV.substring(4);

			//EPC"E5"の処理応答を調べる
			int startData = IPMV.indexOf("E5") + 2;
			int endData = startData + 2;
			String hPDC = IPMV.substring(startData, endData);
			//PDCに"0"が設定されていたら要求を受け付けたことを表している
			if(hPDC == "00") Serial.println("SSKSENDTO COMMAND (積算履歴収集日1設定) SUCCEEDED\r\n");
			else Serial.println("SSKSENDTO COMMAND (積算履歴収集日1設定) FAILED\r\n");
		} while(false);

		if(hr != BROOT_SUCCEEDED) {
			*IPC = "1 SET REQUEST : E50101 FAILED";
			return hr;
		}
	}

	//変数の宣言を冗長に行うため、{}でスコープを分離している
	{
		int iDataStrLen = 22;	//EDATAのバイト数
		char dataStr[iDataStrLen];	//EDATAのcharバッファ
		dataStr[0] = char(0x10);			//EHD1（ECHONET Liteヘッダ1）:ECHONET Lite規格を示す
		dataStr[1] = char(0x81);			//EHD2:EDATA部の電文形式がECHONET Lite仕様で定義する電文形式（規定電文形式）であることを示す
		dataStr[2] = char(0x00); dataStr[3] = char(0x01);			//TID (参考:EL p.3-3)

		dataStr[4] = char(0x05); dataStr[5] = char(0xff); dataStr[6] = char(0x01);			//SEOJ コントローラクラス
		dataStr[7] = char(0x02); dataStr[8] = char(0x88); dataStr[9] = char(0x01);			//DEOJ 低圧スマート電力量メータクラス
		dataStr[10] = char(0x62);			//ESV(62:プロパティ値読み出し要求) (参考:EL p.3-5)

		dataStr[11] = char(0x05);			//OPC(1個)(参考:EL p.3-7)
		dataStr[12] = char(0xe1);			//EPC(参考:EL p.3-7 AppH p.3-275)(E1:積算電力量単位)
		dataStr[13] = char(0x00);			//PDC(参考:EL p.3-9)(読み込み要求の場合は"0"に設定)

		dataStr[14] = char(0xd3);			//EPC(参考:EL p.3-7 AppH p.3-275)(D3:係数)
		dataStr[15] = char(0x00);			//PDC(参考:EL p.3-9)(読み込み要求の場合は"0"に設定)

		dataStr[16] = char(0xd7);			//EPC(参考:EL p.3-7 AppH p.3-275)(D7:積算電力量有効桁数)
		dataStr[17] = char(0x00);			//PDC(参考:EL p.3-9)(読み込み要求の場合は"0"に設定)

		dataStr[18] = char(0xe0);			//EPC(参考:EL p.3-7 AppH p.3-275)(E0:積算電力量計測値(現在値))
		dataStr[19] = char(0x00);			//PDC(参考:EL p.3-9)(読み込み要求の場合は"0"に設定)

		dataStr[20] = char(0xe5);			//EPC(参考:EL p.3-7 AppH p.3-275)(E5:積算履歴収集日1)
		dataStr[21] = char(0x00);			//PDC(参考:EL p.3-9)(読み込み要求の場合は"0"に設定)

		String dataStrLen = String(iDataStrLen,HEX);
		int strLength = dataStrLen.length();

		for(int i = 0; i < 4 - strLength; i++) dataStrLen = "0" + dataStrLen;  //<= '0' ではなく "0" 

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
				hr = SKSENDTO_COMMAND_FAILED;
				break;
			}
			IPMV = IPMV.substring(IPMV.indexOf(eoj) + eoj.length());
			Serial.println("EDATA = " + IPMV);
			if(!(IPMV.indexOf("72") != -1 || IPMV.indexOf("52") != -1)) {
				hr = SKSENDTO_COMMAND_FAILED;
				break;
			}
			Serial.println("SSKSENDTO COMMAND (5 RESPONSE REQUIREMENT : E100D300D700E000E500) 受付を確認\r\n");

			String hOPC = IPMV.substring(2,4);
			unsigned int iOPC = hexToDec(hOPC);
			IPMV = IPMV.substring(4);

			int startData = IPMV.indexOf("E1") + 2;
			int endData = startData + 2;
			String hPDC = IPMV.substring(startData, endData);
			unsigned int iPDC = hexToDec(hPDC);
			startData = endData;
			endData = startData + 2 * iPDC;
			String hexPower = IPMV.substring(startData, endData);
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
			startData = endData;
			endData = startData + 2 * iPDC;
			hexPower = IPMV.substring(startData, endData);
			hexPowerChar = hexToDec(hexPower);
			Serial.println("係数: " + String(hexPowerChar));
			m_fCoefficient = float(hexPowerChar);

			IPMV = IPMV.substring(endData);
			startData = IPMV.indexOf("D7") + 2;
			endData = startData + 2;
			hPDC = IPMV.substring(startData, endData);
			iPDC = hexToDec(hPDC);
			startData = endData;
			endData = startData + 2 * iPDC;
			hexPower = IPMV.substring(startData, endData);
			hexPowerChar = hexToDec(hexPower);
			Serial.println("積算電力有効桁数: " + String(hexPowerChar));

			IPMV = IPMV.substring(endData);
			startData = IPMV.indexOf("E0") + 2;
			endData = startData + 2;
			hPDC = IPMV.substring(startData, endData);
			iPDC = hexToDec(hPDC);
			startData = endData;
			endData = startData + 2 * iPDC;
			hexPower = IPMV.substring(startData, endData);
			hexPowerChar = hexToDec(hexPower);
			Serial.println("積算電力量計測値（正方向計測値） 係数、単位乗算前: " + String(hexPowerChar));
			Serial.println("積算電力量計測値（正方向計測値）: " + String(float(hexPowerChar) * m_fCoefficient * m_fUnit) + " kWh");

			IPMV = IPMV.substring(endData);
			startData = IPMV.indexOf("E5") + 2;
			endData = startData + 2;
			hPDC = IPMV.substring(startData, endData);
			iPDC = hexToDec(hPDC);
			startData = endData;
			endData = startData + 2 * iPDC;
			hexPower = IPMV.substring(startData, endData);
			hexPowerChar = hexToDec(hexPower);
			Serial.println("積算履歴収集日1: " + String(hexPowerChar)); 
		} while(false);
		if(hr != BROOT_SUCCEEDED) {
			*IPC = "5 RESPONSE REQUIREMENT : E100D300D700E000E500 FAILED";
			return hr;
		}
	}

	//変数の宣言を冗長に行うため、{}でスコープを分離している
	{
		int iDataStrLen = 14;	//EDATAのバイト数
		char dataStr[iDataStrLen];	//EDATAのcharバッファ
		dataStr[0] = char(0x10);			//EHD1（ECHONET Liteヘッダ1）:ECHONET Lite規格を示す
		dataStr[1] = char(0x81);			//EHD2:EDATA部の電文形式がECHONET Lite仕様で定義する電文形式（規定電文形式）であることを示す
		dataStr[2] = char(0x00); dataStr[3] = char(0x01);			//TID (参考:EL p.3-3)

		dataStr[4] = char(0x05); dataStr[5] = char(0xff); dataStr[6] = char(0x01);			//SEOJ コントローラクラス
		dataStr[7] = char(0x02); dataStr[8] = char(0x88); dataStr[9] = char(0x01);			//DEOJ 低圧スマート電力量メータクラス
		dataStr[10] = char(0x62);			//ESV(62:プロパティ値読み出し要求) (参考:EL p.3-5)

		dataStr[11] = char(0x01);			//OPC(1個)(参考:EL p.3-7)
		dataStr[12] = char(0xe2);			//EPC(参考:EL p.3-7 AppH p.3-275)(E2:積算電力量計測値履歴1)
		dataStr[13] = char(0x00);			//PDC(参考:EL p.3-9)(読み込み要求の場合は"0"に設定)

		String dataStrLen = String(iDataStrLen,HEX);
		int strLength = dataStrLen.length();

		for(int i = 0; i < 4 - strLength; i++) dataStrLen = "0" + dataStrLen;  //<= '0' ではなく "0" 

		String comStr = "SKSENDTO 1 " + m_ipv6Addr + " 0E1A 1 " + dataStrLen + " ";

		byte comBytes[1024];
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
			startData = endData;
			endData = startData + 2 * iPDC;
			IPMV = IPMV.substring(startData, endData);
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


bool wait2(String argStr,uint32_t waitTime)
{
	String str;

	return wait2(argStr, waitTime, &str);
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