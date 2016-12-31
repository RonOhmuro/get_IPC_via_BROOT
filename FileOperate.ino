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

