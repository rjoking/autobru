#ifndef MACHINE_CONTROLLER_H
#define MACHINE_CONTROLLER_H

#include <Arduino.h>

struct DebouncedButton {
  uint8_t pin;
  bool stableState = true;
  bool lastRawState = true;
  bool fellEdge = false;
  bool roseEdge = false;
  uint32_t lastChangeMs = 0;
};

enum MachineState {
  MACHINE_UNKNOWN,
  MACHINE_OFF,
  MACHINE_WARM_UP,
  MACHINE_READY,
  MACHINE_BREWING
};

class MachineController {
public:
  void begin();
  void update();

  // input queries
  bool isManualStart() const { return manualBtn.fellEdge; }
  bool isOneCupStart() const { return oneCupBtn.fellEdge; }
  bool isTwoCupStart() const { return twoCupBtn.fellEdge; }

  bool isStopPressed() const {
    return manualBtn.fellEdge || oneCupBtn.fellEdge || twoCupBtn.fellEdge;
  }

  bool isManualReleased() const { return manualBtn.roseEdge; }

  // output commands
  void clickRelay();
  void holdRelay();
  void releaseRelay();

  // macros
  void startPreinfusionMacro();
  bool isMacroComplete();

  void stopFromPreinfusion();

  MachineState getCurrentState() const { return currentState; }

private:
  void updateButton(DebouncedButton &btn);
  void updateState();
  const char* getStateName(MachineState state);

#ifdef DEBUG_BUILD
  static constexpr uint8_t MANUAL_PIN = 25;
  static constexpr uint8_t TWO_CUP_PIN = 26;
  static constexpr uint8_t ONE_CUP_PIN = 32;
  static constexpr uint8_t BREW_SWITCH_PIN = 33;
  static constexpr uint8_t POWER_SWITCH_PIN = 27; // Dummy pin for debug build config
  static constexpr uint8_t MAN_LED_PIN = 14;      // Dummy pin for debug build config
  static constexpr uint8_t PWR_LED_PIN = 12;      // Dummy pin for debug build config
#else
  static constexpr uint8_t MANUAL_PIN = 1;
  static constexpr uint8_t TWO_CUP_PIN = 2;
  static constexpr uint8_t ONE_CUP_PIN = 3;
  static constexpr uint8_t BREW_SWITCH_PIN = 4;
  static constexpr uint8_t POWER_SWITCH_PIN = 5;
  static constexpr uint8_t MAN_LED_PIN = 6;
  static constexpr uint8_t PWR_LED_PIN = 7;
#endif

  static constexpr ulong BUTTON_DEBOUNCE_TIME = 50;
  static constexpr ulong RELAY_PULSE_TIME = 100;

  DebouncedButton manualBtn;
  DebouncedButton oneCupBtn;
  DebouncedButton twoCupBtn;

  // relay state
  bool relayActive = false;
  bool relayLatching = false;
  uint32_t relayReleaseTime = 0;

  // start macro state
  bool macroRunning = false;
  bool macroFinished = false;
  uint8_t macroStep = 0;
  uint32_t macroNextActionTime = 0;

  // stop macro state
  bool stopSequenceRunning = false;
  uint32_t stopSequenceStepTime = 0;

  // state machine
  MachineState currentState = MACHINE_UNKNOWN;
  static constexpr uint32_t FLASH_THRESHOLD = 750;
  
  bool lastPwrRaw = true;
  bool lastManRaw = true;
  uint32_t lastPwrChange = 0;
  uint32_t lastManChange = 0;
  uint8_t pwrEdgeCount = 0;
  uint8_t manEdgeCount = 0;
  bool isPwrFlashing = false;
  bool isManFlashing = false;
  bool pwrSteadyState = true;
  bool manSteadyState = true;
};

#endif
