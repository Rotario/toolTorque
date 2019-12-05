enum SCREEN_FORMS {
  F_INIT,
  F_TORQUE,
  F_AUTH,
  F_SETTINGS
};

enum SCREEN_STRINGS {
  STR_MODEL,
  STR_CHUCK,
  STR_SETPOINT,
  STR_TORQUE,
  STR_AUTHFIELD,
  STR_BUZZER_PERC,
  STR_SHANK,
  STR_CALDIALOG,
  STR_CALTORQUE
};

enum SCREEN_4DBUTTONS{
  B_MODEL,
  B_CHUCK,
  B_SHANK,
  B_HOLD,
  B_CALIBRATE,
  B_BUZZEN,
  B_TARE
};

enum SCREEN_WINBUTTONS{
  WIN_SETTINGS,
  WIN_AUTHBACK,
  WIN_SETTINGSBACK,
  WIN_OK,
  WIN_HIDEOK
};

enum SETTINGS_ADDR{
  ADDR_CAL,
  ADDR_ENBUZZER = ADDR_CAL + sizeof(calibration_factor),
};

struct ScreenStruct{
  SCREEN_FORMS form = F_INIT;
  const char * pw = "1248";//make sure this is smaller than AUTH_BUF_SIZE
  const char * modelStr(){
    return modelArr[model].model;
  }
  const char * chuckStr(){
    return modelArr[model].sizes[size];
  }
  const char * shankStr(){
    return modelArr[model].shanks[size][shank];
  }
  uint16_t setPoint(){
    return modelArr[model].torques[size][shank];
  }
} screen;
