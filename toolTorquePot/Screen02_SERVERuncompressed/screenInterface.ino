void changeForm(SCREEN_FORMS idx) {
  screen.form = idx;
  genie.WriteObject(GENIE_OBJ_FORM, idx, 0);

  switch (idx) {
    case F_TORQUE:
      updateScreenTorqueModels();
      break;
    case F_SETTINGS:
      genie.WriteObject(GENIE_OBJ_4DBUTTON, B_BUZZEN, enBuzzer);//update buzzer button with current state
  }
}

void updateTorqueScreen(int measured, int scaled) {
  genie.WriteObject(GENIE_OBJ_GAUGE, 0, scaled);
  char buf[10];
  size_t len = snprintf(buf, 10, "%i Nm", measured);
  
  if (screen.form == F_TORQUE){
    genie.WriteStr(STR_TORQUE, buf);
  } else if (screen.form == F_SETTINGS){
    genie.WriteStr(STR_CALTORQUE, buf);
  }
}

void updateScreenTorqueModels() {
  genie.WriteStr(STR_MODEL, screen.modelStr());
  genie.WriteStr(STR_CHUCK, screen.chuckStr());
  genie.WriteStr(STR_SHANK, screen.shankStr());
  char buf[10];
  size_t len = snprintf(buf, 10, "%i Nm", screen.setPoint());
  genie.WriteStr(STR_SETPOINT, buf);
}


void myGenieEventHandler(void) {
  genieFrame Event;
  genie.DequeueEvent(&Event); // Remove the next queued event from the buffer, and process it below

  switch (screen.form) {
      if (Event.reportObject.cmd != GENIE_REPORT_EVENT)
        return;
    case F_INIT:
      break;
    case F_TORQUE:
      switch (Event.reportObject.object) {
        case GENIE_OBJ_4DBUTTON:
          switch (Event.reportObject.index) { //Change model

            case B_MODEL:
              model++;
              if (model < NO_MODELS && modelArr[model].model[0]) { //Is first char of model string null?
                size = 0;
                shank = 0;
              } else {
                model = 0;
                size = 0;
                shank = 0;
              }
              
              updateScreenTorqueModels();

              scaleFactor = gaugeMax / modelArr[model].torques[size][shank];
              updateWebsocketStrings();
              break;

            case B_CHUCK:
              size++;
              if (size < NO_SIZES && modelArr[model].sizes[size][0]) {
                shank = 0;
              } else {
                size = 0;
                shank = 0;
              }
              
              updateScreenTorqueModels();

              scaleFactor = gaugeMax / modelArr[model].torques[size][shank];
              updateWebsocketStrings();
              break;
            
            case B_SHANK:
              shank++;
              if (shank < NO_SHANKS && modelArr[model].shanks[size][shank][0]){
              } else {
                shank = 0;
              }
              updateScreenTorqueModels();

              scaleFactor = gaugeMax / modelArr[model].torques[size][shank];
              updateWebsocketStrings();
              break;

            case B_HOLD:
              holdOn = genie.GetEventData(&Event);
              break;
          }
          break;
        case GENIE_OBJ_WINBUTTON://Settings button pressed
          changeForm(F_AUTH);
          break;
      }
      break;
    case F_AUTH:
      switch (Event.reportObject.object) {
        case GENIE_OBJ_4DBUTTON:
          switch (Event.reportObject.index) { //Change model
          }
          break;
          
        case GENIE_OBJ_WINBUTTON://Back button pressed
          changeForm(F_TORQUE);
          break;

        case GENIE_OBJ_KEYBOARD://Back button pressed
          char temp = genie.GetEventData(&Event);
          if (authBufidx < AUTH_BUF_SIZE - 1) {
            if (temp == 8) {//backspace
              if (authBufidx > 0) {
                authBufidx--;
                authBuf[authBufidx] = 0;//remove this char and decrement counter
              }
            } else {
              authBuf[authBufidx++] = temp;
              authBuf[authBufidx] = 0;//Add null
            }
            genie.WriteStr(STR_AUTHFIELD, authBuf);
          }
          if (authBufidx == sizeof(screen.pw)) {
            if (strcmp(authBuf, screen.pw) != 0) { //filled buffer and pw not correct
              authBufidx = 0;
              authBuf[0] = 0;
              genie.WriteStr(STR_AUTHFIELD, "Wrong PW!");
              //put in 4 chars
            } else { //Correct PW! Reset and load settings page
              authBufidx = 0;
              authBuf[0] = 0;
              changeForm(F_SETTINGS);
            }
          }
          break;
      }
      break;
    case F_SETTINGS:
      switch (Event.reportObject.object) {
        
        case GENIE_OBJ_WINBUTTON://Back button pressed
          changeForm(F_TORQUE);
          break;
        case GENIE_OBJ_4DBUTTON:
          switch (Event.reportObject.index){
            case B_CALIBRATE:
              calibrateScale();
              break;
              
            case B_BUZZEN:
              enBuzzer = genie.GetEventData(&Event);
              EEPROM.put(ADDR_ENBUZZER, enBuzzer);
              EEPROM.commit();
              break;
              
            case B_TARE:
              scale.tare();
              break;
            
          }
          break;
      }
      break;
  }
}
