#define COMMAND_WAIT_TIME 300000	//5 minites

int incomingByte = 0;	// for received data

///****************************
///operation from serial
///****************************
void serialOperate() {
	if (Serial.available() > 0) { // received data exist
		incomingByte = Serial.read(); // read received data
		if(incomingByte == 'i') {
			Serial.print("\r\nI received: "); // reply received data
			Serial.println(incomingByte, DEC);
			operate();
		} 
	}
}



///********************************
///actual operation
///********************************
void operate() {
	while(1) {
		Serial.print("> ");
		String command = getInput();
		Serial.println("I received command: " + command);
		if(command == "exit") return;
		if(command == "help") dispHelp();
		if(command == "setSSS") setSSS(0);
		if(command == "setWISUN") setWI_SUN();
		if(command == "readWISUN") readWI_SUN();
		if(command == "dispIPC") dispIPC();
		if(command == "brootBegin") bBegin();
		if(command == "brootTerm") bROOTTERM();
		if(command == "dirOP") dirOP();
	}
}




///*****************************
/// display help
///*****************************
void dispHelp() {
	Serial.println("help: this display");
	Serial.println("exit:   exit command mode");
	Serial.println("setSSS : set SoftwareSerial speed for BP35A1(WI-SUN)");
	Serial.println("setWISUN : set WI-SUN(B ROOT) parametaers");
	Serial.println("readWISUN : read WI-SUN(B ROOT) parametaers");
	Serial.println("dispIPC : disp Integral Power Consumption");
	Serial.println("brootBegin : WI-SUN(B ROOT) begin");
	Serial.println("brootTerm : WI-SUN(B ROOT) terminate");
	Serial.println("dirOP : directly send command to WI-SUN(B ROOT) ");

	return;
}



///********************************
///direct operation
///********************************
void dirOP() {
	while(1) {
		Serial.print("direct > ");
		String command = getInput();
		Serial.println("I received command: " + command);
		if(command == "exit") return;
		SoftSerial.println(command);
		wait2("OK",30000);
	}
}



///******************************************
/// display Integral Power Consumption
///******************************************
void dispIPC() {
    String IPC;
    eErrBroot result;
    result = getIntegralPowerConsumption(&IPC);
    //if(Serial) Serial.println("Integral Power Consumption : " + IPC);

	return;
}



///******************************************
/// b root operation begin
/// (connect to smart meter)
///******************************************
void bBegin() {
	eErrBroot result;
    result = brootBegin();
}



///********************************
///get input command & data
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