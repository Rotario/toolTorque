#include "chuckParser.h"


int readChuckFile(Chuck * models, File * f) {
  char buf[100];
  int cNo = -1;//Wait til find first chuck
  uint8_t sizeNo = 0;
  uint8_t shankNo = 0;
  char * pch;
  if (f) {
    Serial1.println(F("success"));
    while (f->available()) {
      if (readToCharCode(f, 0x0D, sizeof(buf), buf) != 0)
        break; //Fill messagebuffer with file until newline CR
      f->read(); //Dump out LF
      pch = strtok(buf, "="); //Split into spaces
      if (pch != NULL) {
        if (strcmp("chuck", pch) == 0 ) {
          cNo++;//Iterate chuck No
          sizeNo = 0;
          shankNo = 0;
          Serial1.print(F("Found chuck "));
          pch = strtok(NULL, ""); //Read string after space;
          Serial1.println(pch);
          if (strlen(pch) > MODEL_STRLEN - 1) {
            Serial1.println(F("Chuck Strlen too big"));
            return -1;
          }
          if (cNo >= NO_MODELS) {
            Serial1.println(F("No Chucks Too Big"));
            return -1;
          }
          strncpy(models[cNo].model, pch, sizeof(models[cNo].model));

        } else if (strcmp("pre", pch) == 0 ) {
          Serial1.print(F("Found pre"));
          pch = strtok(NULL, ""); //Read string after space;
          Serial1.println(pch);
          if (strlen(pch) > PRE_STRLEN - 1) {
            Serial1.println(F("Chuck prefix too big"));

            return -1;
          }
          strncpy(models[cNo].prefix, pch, sizeof(models[cNo].prefix));

        } else if (atoi(pch)) {
          if (sizeNo < NO_SIZES) {
            //iterate through number of sizes for this chuck
            snprintf(models[cNo].sizes[sizeNo], SIZE_STRLEN, "%s%s", models[cNo].prefix, pch);
            Serial1.printf("Chuck %s\t", models[cNo].sizes[sizeNo]);

            while (pch != NULL && shankNo < NO_SHANKS) {
              //Iterate through number of shanks for this size
              pch = strtok(NULL, ":");
              if (pch != NULL) {
                if (strlen(pch) > SHANK_STRLEN - 1) {
                  Serial1.print("Shank Strlen too big");

                  return -1;
                }
                strncpy(models[cNo].shanks[sizeNo][shankNo], pch, sizeof(models[cNo].shanks[sizeNo][shankNo]));
                Serial1.printf("Shank: %s\t", models[cNo].shanks[sizeNo][shankNo]);
              } else { //END OF LINE
                shankNo = 0;
                break;
              }

              pch = strtok(NULL, " ");
              models[cNo].torques[sizeNo][shankNo] = atoi(pch);
              Serial1.printf("Torque: %i\t", models[cNo].torques[sizeNo][shankNo]);
              shankNo++;
            }
            sizeNo++;
            Serial1.println();
          } else {
            Serial1.print("Too many Chuck Sizes");
          }

        } else {
          Serial1.println(F("Not Recognised:"));
          Serial1.print(buf);
          Serial1.println();
          return -1;
        }
      }
    }
  } else {
    Serial1.println(F("...failed"));
    return -1; //dont read SVM parameters if svm model file failed
  }
  Serial1.printf("Finished!");
  return 0;
}

void printChuckData(Chuck * models) {
  int i;
  int j;
  int k;
  for (i = 0; i < NO_MODELS; i++) {
    Serial1.printf("Chuck Model %s\n", models[i].model);
    j = 0;
    while (models[i].sizes[j] && j < NO_SIZES) {
      Serial1.printf("Size: %i\t", models[i].sizes[j]);
      k = 0;
      while (models[i].torques[j][k] && k < NO_SHANKS) {
        Serial1.printf("%s:%i\t", models[i].shanks[j][k], models[i].torques[j][k]);
        k++;
      }
      Serial1.println();
      j++;
    }
  }
}

int readToCharCode(File * f, char c, int bufSize, char * buf) {
  int i = 0;
  if (f) {
    do {
      if (f->available()) {
        if (i < bufSize) {
          buf[i] = f->read();
          i++;
        } else {
          Serial1.println(F("Specified char code not found, buffer filled"));
          return -1;
        }
      } else {
        Serial1.println(F("Specified char code not found, EOF reached"));
        return -2;
      }
    } while (buf[i - 1] != c); //look for given char
    buf[i - 1] = '\0'; //Replace given char by null pointer
  } else {
    Serial1.println(F("File needs opening before readToCharCode called"));
    return -3;
  }
  return 0;
}
