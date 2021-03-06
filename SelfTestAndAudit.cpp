#include <arduino.h>
#include "SelfTestAndAudit.h"
#include "BallySternOS.h"

#define MACHINE_STATE_ATTRACT         0

unsigned long LastSolTestTime = 0;
unsigned long LastSelfTestChange = 0;
unsigned long SavedValue = 0;
unsigned long ResetHold = 0;
unsigned long NextSpeedyValueChange = 0;
unsigned long NumSpeedyChanges = 0;
unsigned long LastResetPress = 0;
byte curLamp = 0;
boolean SolenoidCycle = true;


int RunBaseSelfTest(int curState, boolean curStateChanged, unsigned long CurrentTime, byte resetSwitch) {
  byte curSwitch = BSOS_PullFirstFromSwitchStack();
  int returnState = curState;
  boolean resetDoubleClick = false;
  unsigned short savedScoreStartByte = 0;
  unsigned short auditNumStartByte = 0;

  if (curSwitch==resetSwitch) {
    ResetHold = CurrentTime;
    if ((CurrentTime-LastResetPress)<400) {
      resetDoubleClick = true;
      curSwitch = SWITCH_STACK_EMPTY;
    }
    LastResetPress = CurrentTime;
  }

  if (ResetHold!=0 && !BSOS_ReadSingleSwitchState(resetSwitch)) {
    ResetHold = 0;
    NextSpeedyValueChange = 0;
  }

  boolean resetBeingHeld = false;
  if (ResetHold!=0 && (CurrentTime-ResetHold)>1300) {
    resetBeingHeld = true;
    if (NextSpeedyValueChange==0) {
      NextSpeedyValueChange = CurrentTime;
      NumSpeedyChanges = 0;
    }
  }

  // Things to add:
  //  Free Play Mode
  //  Extra ball/credit award scores (3)
  
  if (curSwitch==SW_SELF_TEST_SWITCH && (CurrentTime-LastSelfTestChange)>250) {
    returnState -= 1;
    if (returnState==MACHINE_STATE_TEST_DONE) returnState = MACHINE_STATE_ATTRACT;
    LastSelfTestChange = CurrentTime;
  }

  if (curStateChanged) {
    for (int count=0; count<4; count++) {
      BSOS_SetDisplay(count, 0);
      BSOS_SetDisplayBlank(count, 0x00);        
    }

    if (curState<=MACHINE_STATE_TEST_SCORE_LEVEL_1) {
      BSOS_SetDisplayCredits(abs(curState)-4);
      BSOS_SetDisplayBallInPlay(0, false);
    }
  }

  if (curState==MACHINE_STATE_TEST_LIGHTS) {
    if (curStateChanged) {
      BSOS_DisableSolenoidStack();        
      BSOS_SetDisableFlippers(true);
      BSOS_SetDisplayCredits(0);
      BSOS_SetDisplayBallInPlay(1);
      BSOS_TurnOffAllLamps();
      for (int count=0; count<60; count++) {
        BSOS_SetLampState(count, 1, 0, 500);
      }
      curLamp = 99;
      BSOS_SetDisplay(0, curLamp);  
      BSOS_SetDisplayBlankByMagnitude(0, curLamp);      
    }
    if (curSwitch==resetSwitch || resetDoubleClick) {
      curLamp += 1;
      if (curLamp>99) curLamp = 0;
      if (curLamp==60) {
        curLamp = 99;
        for (int count=0; count<60; count++) {
          BSOS_SetLampState(count, 1, 0, 500);
        }
      } else {
        BSOS_TurnOffAllLamps();
        BSOS_SetLampState(curLamp, 1, 0, 500);
      }      
      BSOS_SetDisplay(0, curLamp);  
      BSOS_SetDisplayBlankByMagnitude(0, curLamp);      
    }    
  } else if (curState==MACHINE_STATE_TEST_DISPLAYS) {
    if (curStateChanged) {
      BSOS_TurnOffAllLamps();
      BSOS_SetDisplayCredits(0);
      BSOS_SetDisplayBallInPlay(2);
      for (int count=0; count<4; count++) {
        BSOS_SetDisplayBlank(count, 0x3F);        
      }
    }
    BSOS_CycleAllDisplays(CurrentTime);
  } else if (curState==MACHINE_STATE_TEST_SOLENOIDS) {
    if (curStateChanged) {
      BSOS_TurnOffAllLamps();
      LastSolTestTime = CurrentTime;
      BSOS_EnableSolenoidStack(); 
      BSOS_SetDisableFlippers(false);
      BSOS_SetDisplayCredits(0);
      BSOS_SetDisplayBallInPlay(3);
      SolenoidCycle = true;
      SavedValue = 0;
      BSOS_PushToSolenoidStack(SavedValue, 3);
    }
    if (curSwitch==resetSwitch || resetDoubleClick) {
      SolenoidCycle = (SolenoidCycle) ? false : true;
    }

    if ((CurrentTime-LastSolTestTime)>1000) {
      if (SolenoidCycle) {
        SavedValue += 1;
        if (SavedValue>14) SavedValue = 0;
      }
      BSOS_PushToSolenoidStack(SavedValue, 3);
      BSOS_SetDisplay(0, SavedValue);
      BSOS_SetDisplayBlankByMagnitude(0, SavedValue);
      LastSolTestTime = CurrentTime;
    }
    
    BSOS_SetDisplayFlash(4, CurrentTime, 500);    
  } else if (curState==MACHINE_STATE_TEST_SWITCHES) {
    if (curStateChanged) {
      BSOS_TurnOffAllLamps();
      BSOS_DisableSolenoidStack(); 
      BSOS_SetDisableFlippers(true);
      BSOS_SetDisplayCredits(0);
      BSOS_SetDisplayBallInPlay(4);
    }

    byte displayOutput = 0;
    for (byte switchCount=0; switchCount<40 && displayOutput<4; switchCount++) {
      if (BSOS_ReadSingleSwitchState(switchCount)) {
        BSOS_SetDisplay(displayOutput, switchCount);
        BSOS_SetDisplayBlankByMagnitude(displayOutput, switchCount);
        displayOutput += 1;
      }
    }

    if (displayOutput<4) {
      for (int count=displayOutput; count<4; count++) {
        BSOS_SetDisplayBlank(count, 0x00);
      }
    }

  } else if (curState==MACHINE_STATE_TEST_SCORE_LEVEL_1) {
    savedScoreStartByte = BSOS_AWARD_SCORE_1_EEPROM_START_BYTE;
  } else if (curState==MACHINE_STATE_TEST_SCORE_LEVEL_2) {
    savedScoreStartByte = BSOS_AWARD_SCORE_2_EEPROM_START_BYTE;
  } else if (curState==MACHINE_STATE_TEST_SCORE_LEVEL_3) {
    savedScoreStartByte = BSOS_AWARD_SCORE_3_EEPROM_START_BYTE;
  } else if (curState==MACHINE_STATE_TEST_HISCR) {
    savedScoreStartByte = BSOS_HIGHSCORE_EEPROM_START_BYTE;
  } else if (curState==MACHINE_STATE_TEST_CREDITS) {
    if (curStateChanged) {
      SavedValue = BSOS_ReadCreditsFromEEProm();
      BSOS_SetDisplay(0, SavedValue);
      BSOS_SetDisplayBlankByMagnitude(0, SavedValue);
    }
    if (curSwitch==resetSwitch || resetDoubleClick) {
      SavedValue += 1;
      if (SavedValue>20) SavedValue = 0;
      BSOS_SetDisplay(0, SavedValue);
      BSOS_SetDisplayBlankByMagnitude(0, SavedValue);
      BSOS_WriteCreditsToEEProm(SavedValue & 0x000000FF);
    }
  } else if (curState==MACHINE_STATE_TEST_TOTAL_PLAYS) {
    auditNumStartByte = BSOS_TOTAL_PLAYS_EEPROM_START_BYTE;
  } else if (curState==MACHINE_STATE_TEST_TOTAL_REPLAYS) {
    auditNumStartByte = BSOS_TOTAL_REPLAYS_EEPROM_START_BYTE;
  } else if (curState==MACHINE_STATE_TEST_HISCR_BEAT) {
    auditNumStartByte = BSOS_TOTAL_HISCORE_BEATEN_START_BYTE;
  } else if (curState==MACHINE_STATE_TEST_CHUTE_2_COINS) {
    auditNumStartByte = BSOS_CHUTE_2_COINS_START_BYTE;
  } else if (curState==MACHINE_STATE_TEST_CHUTE_1_COINS) {
    auditNumStartByte = BSOS_CHUTE_1_COINS_START_BYTE;
  } else if (curState==MACHINE_STATE_TEST_CHUTE_3_COINS) {
    auditNumStartByte = BSOS_CHUTE_3_COINS_START_BYTE;
  }

  if (savedScoreStartByte) {
    if (curStateChanged) {
      SavedValue = BSOS_ReadULFromEEProm(savedScoreStartByte);
      BSOS_SetDisplay(0, SavedValue);  
      BSOS_SetDisplayBlankByMagnitude(0, SavedValue);
    }

    if (curSwitch==resetSwitch) {
      SavedValue += 1000;
      BSOS_SetDisplay(0, SavedValue);  
      BSOS_SetDisplayBlankByMagnitude(0, SavedValue);
      BSOS_WriteULToEEProm(savedScoreStartByte, SavedValue);
    }

    if (resetBeingHeld && (CurrentTime>=NextSpeedyValueChange)) {
      SavedValue += 1000;
      BSOS_SetDisplay(0, SavedValue);  
      BSOS_SetDisplayBlankByMagnitude(0, SavedValue);
      if (NumSpeedyChanges<6) NextSpeedyValueChange = CurrentTime + 400;
      else if (NumSpeedyChanges<50) NextSpeedyValueChange = CurrentTime + 50;
      else NextSpeedyValueChange = CurrentTime + 10;
      NumSpeedyChanges += 1;
    }

    if (!resetBeingHeld && NumSpeedyChanges>0) {
      BSOS_WriteULToEEProm(savedScoreStartByte, SavedValue);
      NumSpeedyChanges = 0;
    }
    
    if (resetDoubleClick) {
      SavedValue = 0;
      BSOS_SetDisplay(0, SavedValue);  
      BSOS_SetDisplayBlankByMagnitude(0, SavedValue);
      BSOS_WriteULToEEProm(savedScoreStartByte, SavedValue);
    }
  }

  if (auditNumStartByte) {
    if (curStateChanged) {
      SavedValue = BSOS_ReadULFromEEProm(auditNumStartByte);
      BSOS_SetDisplay(0, SavedValue);  
      BSOS_SetDisplayBlankByMagnitude(0, SavedValue);
    }

    if (resetDoubleClick) {
      SavedValue = 0;
      BSOS_SetDisplay(0, SavedValue);  
      BSOS_SetDisplayBlankByMagnitude(0, SavedValue);
      BSOS_WriteULToEEProm(auditNumStartByte, SavedValue);
    }
    
  }
  
  return returnState;
}

unsigned long GetLastSelfTestChangedTime() {
  return LastSelfTestChange;
}


void SetLastSelfTestChangedTime(unsigned long setSelfTestChange) {
  LastSelfTestChange = setSelfTestChange;
}
