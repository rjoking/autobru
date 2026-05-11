#include "MachineController.h"

void MachineController::begin() {
  pinMode(MANUAL_PIN, INPUT_PULLUP); //New hardware does not require external pullups, but keeping this for backwards compatibility
  pinMode(ONE_CUP_PIN, INPUT_PULLUP);
  pinMode(TWO_CUP_PIN, INPUT_PULLUP);
  pinMode(MAN_LED_PIN, INPUT_PULLUP);
  pinMode(PWR_LED_PIN, INPUT_PULLUP);

  pinMode(BREW_SWITCH_PIN, OUTPUT);
  digitalWrite(BREW_SWITCH_PIN, LOW);

  pinMode(POWER_SWITCH_PIN, OUTPUT);
  digitalWrite(POWER_SWITCH_PIN, LOW);

  manualBtn.pin = MANUAL_PIN;
  oneCupBtn.pin = ONE_CUP_PIN;
  twoCupBtn.pin = TWO_CUP_PIN;
}

void MachineController::update() {
  updateButton(manualBtn);
  updateButton(oneCupBtn);
  updateButton(twoCupBtn);
  
  updateState();

  if (relayActive && !relayLatching && millis() >= relayReleaseTime) {
    digitalWrite(BREW_SWITCH_PIN, LOW);
    relayActive = false;
  }

  if (macroRunning) {
    if (macroStep == 0 && millis() >= macroNextActionTime) {
      // 200ms have passed since user pressed the brew button, we click the
      // relay to stop that brew
      clickRelay();

      macroStep = 1;

      macroNextActionTime = millis() + 500;
    } else if (macroStep == 1 && millis() >= macroNextActionTime) {
      // now we can start proper preinfusion via the relay;
      holdRelay();

      macroRunning = false;
      macroFinished = true;
    }
  }

  if (stopSequenceRunning) {
    if (millis() >= stopSequenceStepTime) {
      stopSequenceRunning = false;
      clickRelay();
    }
  }
}

void MachineController::clickRelay() {
  digitalWrite(BREW_SWITCH_PIN, HIGH);
  relayActive = true;
  relayLatching = false;
  relayReleaseTime = millis() + RELAY_PULSE_TIME;
}

void MachineController::holdRelay() {
  digitalWrite(BREW_SWITCH_PIN, HIGH);
  relayActive = true;
  relayLatching = true;
}

void MachineController::releaseRelay() {
  digitalWrite(BREW_SWITCH_PIN, LOW);
  relayActive = false;
  relayLatching = false;
}

void MachineController::startPreinfusionMacro() {
  macroRunning = true;
  macroFinished = false;
  macroStep = 0;

  macroNextActionTime = millis() + 200;
}

bool MachineController::isMacroComplete() {
  if (macroFinished) {
    macroFinished = false;
    return true;
  }
  return false;
}

void MachineController::stopFromPreinfusion() {
  // release currently latched relay, lets machine go to full flow
  releaseRelay();
  stopSequenceRunning = true;
  stopSequenceStepTime = millis() + 150;
}

void MachineController::updateButton(DebouncedButton &btn) {
  bool raw = digitalRead(btn.pin);
  uint32_t now = millis();

  btn.fellEdge = false;
  btn.roseEdge = false;

  if (raw != btn.lastRawState) {
    btn.lastRawState = raw;
    btn.lastChangeMs = now;
  } else if ((now - btn.lastChangeMs) >= BUTTON_DEBOUNCE_TIME &&
             raw != btn.stableState) {
    btn.stableState = raw;
    if (!raw)
      btn.fellEdge = true;
    else
      btn.roseEdge = true;
  }
}

void MachineController::updateState() {
  bool currentPwr = digitalRead(PWR_LED_PIN);
  bool currentMan = digitalRead(MAN_LED_PIN);
  uint32_t now = millis();

  // Power LED Flashing/Steady detection
  if (currentPwr != lastPwrRaw) {
    lastPwrRaw = currentPwr;
    if (now - lastPwrChange <= FLASH_THRESHOLD) {
      pwrEdgeCount++;
    } else {
      pwrEdgeCount = 1;
    }
    lastPwrChange = now;

    if (pwrEdgeCount > 1) {
      isPwrFlashing = true;
    }
  } else if (now - lastPwrChange > FLASH_THRESHOLD) {
    isPwrFlashing = false;
    pwrSteadyState = currentPwr;
    pwrEdgeCount = 0;
  }

  // Manual LED Flashing/Steady detection
  if (currentMan != lastManRaw) {
    lastManRaw = currentMan;
    if (now - lastManChange <= FLASH_THRESHOLD) {
      manEdgeCount++;
    } else {
      manEdgeCount = 1;
    }
    lastManChange = now;

    if (manEdgeCount > 1) {
      isManFlashing = true;
    }
  } else if (now - lastManChange > FLASH_THRESHOLD) {
    isManFlashing = false;
    manSteadyState = currentMan;
    manEdgeCount = 0;
  }

  MachineState newState = currentState;

  // State resolution (Logic: LOW = ON, HIGH = OFF)
  if (!isPwrFlashing && pwrSteadyState == HIGH && !isManFlashing && manSteadyState == HIGH) {
    newState = MACHINE_OFF;
  } else if (isPwrFlashing && !isManFlashing && manSteadyState == HIGH) {
    newState = MACHINE_WARM_UP;
  } else if (!isPwrFlashing && pwrSteadyState == LOW && !isManFlashing && manSteadyState == LOW) {
    newState = MACHINE_READY;
  } else if (!isPwrFlashing && pwrSteadyState == LOW && isManFlashing) {
    newState = MACHINE_BREWING;
  }

  if (newState != currentState) {
    Serial.printf("[STATE] %s -> %s\n", getStateName(currentState), getStateName(newState));
    // TODO: Add Syslog call for a separate log of state changes, e.g. syslog.logf(LOG_INFO, "[STATE] %s -> %s", getStateName(currentState), getStateName(newState))
    // e.g. syslog.logf(LOG_INFO, "[STATE] %s -> %s", getStateName(currentState), getStateName(newState));
    
    currentState = newState;
  }
}

const char* MachineController::getStateName(MachineState state) {
  switch (state) {
    case MACHINE_OFF: return "OFF";
    case MACHINE_WARM_UP: return "WARM_UP";
    case MACHINE_READY: return "READY";
    case MACHINE_BREWING: return "BREWING";
    default: return "UNKNOWN";
  }
}
