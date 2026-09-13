// Smart Parking Barrier -- Arduino Uno R3 / ATmega328P, Servo 1.3.0
// Pin map and build constraints: ../../docs/wiring.md and construction.md.
#include <Arduino.h>
#include <Servo.h>

constexpr uint8_t TRIG_PIN=4, ECHO_PIN=2, BEAM_PIN=3, EMITTER_PIN=5;
constexpr uint8_t RED_PIN=6, GREEN_PIN=7, BUZZER_PIN=8, SERVO_PIN=9;
constexpr uint8_t OPEN_LIMIT_PIN=A0, CLOSED_LIMIT_PIN=A1, MANUAL_PIN=A2;
constexpr uint8_t OPEN_ANGLE=95, CLOSED_ANGLE=10; // Calibrate without arm first.
constexpr uint16_t OCCUPIED_MM=160, CLEAR_MM=200, MIN_MM=20, MAX_MM=350;
constexpr uint32_t PING_MS=65, ECHO_TIMEOUT_US=25000, STALE_MS=300;
constexpr uint32_t BEAM_PHASE_MS=10, BEAM_CLEAR_MS=100, ENDPOINT_SETTLE_MS=100;
constexpr uint32_t DEBOUNCE_MS=20, CLEARANCE_MS=1200, TRAVEL_MS=2500;
constexpr uint32_t DEPART_MS=500, WAIT_NOTICE_MS=30000, STARTUP_MS=2000;
constexpr uint32_t CLOSE_STEP_MS=20; // 1 degree per step: about 1.7 s for 85 degrees.
constexpr uint8_t CONFIRM_SAMPLES=3, BAD_LIMIT=3;
constexpr uint16_t ARM_DEGREES=OPEN_ANGLE>CLOSED_ANGLE ?
  OPEN_ANGLE-CLOSED_ANGLE : CLOSED_ANGLE-OPEN_ANGLE;
static_assert(OPEN_ANGLE<=180 && CLOSED_ANGLE<=180 && ARM_DEGREES>0,
              "Calibrate two distinct positional-servo angles");
static_assert(ARM_DEGREES*CLOSE_STEP_MS+ENDPOINT_SETTLE_MS+DEBOUNCE_MS<TRAVEL_MS,
              "Closing ramp must fit inside the travel deadline");

enum class State : uint8_t { BOOT, IDLE, OPENING, OPEN, PASSING,
                            CLEARANCE, CLOSING, FAULT };
enum class Fault : uint8_t { NONE, SONAR, BEAM_TEST, LIMITS, ACTUATOR };
enum class Demand : uint8_t { OPEN, CLOSE, STOP };

struct Debounced {
  bool raw=false, active=false;
  uint32_t changed=0;
  void update(bool value, uint32_t now) {
    if(value!=raw) { raw=value; changed=now; }
    if(uint32_t(now-changed)>=DEBOUNCE_MS) active=raw;
  }
};
struct Range {
  uint16_t mm=0;
  uint8_t nearRun=0, clearRun=0, badRun=0;
  bool occupied=false, valid=false, everValid=false;
  uint32_t lastGood=0;
  void sample(uint16_t value, bool ok, uint32_t now) {
    valid=ok && value>=MIN_MM && value<=MAX_MM;
    if(!valid) {
      if(badRun<255) ++badRun;
      nearRun=clearRun=0; // Unknown is never a clear measurement.
      return;
    }
    mm=value; lastGood=now; everValid=true; badRun=0;
    if(mm<=OCCUPIED_MM) {
      clearRun=0;
      if(nearRun<CONFIRM_SAMPLES) ++nearRun;
      if(nearRun>=CONFIRM_SAMPLES) occupied=true;
    } else if(mm>=CLEAR_MM) {
      nearRun=0;
      if(clearRun<CONFIRM_SAMPLES) ++clearRun;
      if(clearRun>=CONFIRM_SAMPLES) occupied=false;
    } else { nearRun=clearRun=0; } // Hysteresis band retains occupancy.
  }
  bool healthy(uint32_t now) const {
    return valid && everValid && uint32_t(now-lastGood)<STALE_MS;
  }
  bool clear(uint32_t now) const {
    return healthy(now) && !occupied && clearRun>=CONFIRM_SAMPLES && mm>=CLEAR_MM;
  }
  bool failed(uint32_t now) const {
    return badRun>=BAD_LIMIT ||
      (everValid && uint32_t(now-lastGood)>=STALE_MS);
  }
};

Servo barrier;
Range approach;
Debounced openLimit, closedLimit;
State state=State::BOOT;
Fault fault=Fault::NONE;
Demand demand=Demand::OPEN;
uint8_t commandAngle=OPEN_ANGLE;
uint32_t servoStepAt=0;
bool actuatorStopped=false, moving=true, manual=false;
bool beamBlocked=true, beamClear=false, beamTestFault=false, beamTestPassed=false;
bool beamClearTiming=false, clearanceTiming=false, endpointTiming=false;
bool crossingSeen=false, waitNotice=false;
uint8_t beamPhase=0; // 0=normal, 1=emitter off, 2=settling after on.
uint32_t bootAt=0, sessionAt=0, travelAt=0, clearanceAt=0;
uint32_t beamPhaseAt=0, beamClearAt=0, endpointAt=0, debugAt=0;
uint32_t pingAt=0, pingStartedUs=0;
bool pingWaiting=false;
volatile bool echoArmed=false, echoRisen=false, echoDone=false;
volatile uint32_t echoRiseUs=0, echoWidthUs=0, echoCompletedUs=0;

void echoISR() {
  if(!echoArmed) return;
  if(digitalRead(ECHO_PIN)==HIGH) {
    if(!echoRisen) { echoRiseUs=micros(); echoRisen=true; }
  } else if(echoRisen) {
    echoCompletedUs=micros();
    echoWidthUs=uint32_t(echoCompletedUs-echoRiseUs);
    echoDone=true; echoArmed=false;
  }
}

// Asynchronous echo capture; only the 10 us trigger pulse is synchronous.
void readDistance(uint32_t now) {
  if(pingWaiting) {
    noInterrupts();
    bool done=echoDone;
    uint32_t width=echoWidthUs;
    uint32_t completed=echoCompletedUs;
    bool expired=uint32_t(micros()-pingStartedUs)>=ECHO_TIMEOUT_US;
    if(done || expired) { echoArmed=false; echoDone=false; }
    interrupts();
    if(done || expired) {
      pingWaiting=false;
      uint32_t ageUs=uint32_t(micros()-completed);
      bool timely=done && uint32_t(completed-pingStartedUs)<=ECHO_TIMEOUT_US &&
                  ageUs<STALE_MS*1000UL;
      approach.sample(done ? uint16_t(width*10UL/58UL) : 0,
                      timely && width<=ECHO_TIMEOUT_US,
                      timely ? now-ageUs/1000UL : now);
    }
  }
  if(!pingWaiting && uint32_t(now-pingAt)>=PING_MS) {
    pingAt=now;
    if(digitalRead(ECHO_PIN)==HIGH) { approach.sample(0,false,now); return; }
    pingStartedUs=micros();
    noInterrupts();
    echoRisen=false; echoDone=false; echoArmed=true;
    interrupts();
    pingWaiting=true;
    digitalWrite(TRIG_PIN,HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN,LOW);
  }
}

void updateBeam(uint32_t now) {
  if(beamPhase==1 && uint32_t(now-beamPhaseAt)>=BEAM_PHASE_MS) {
    // Specified receiver: illuminated=LOW; emitter off must produce HIGH.
    if(digitalRead(BEAM_PIN)!=HIGH) beamTestFault=true;
    digitalWrite(EMITTER_PIN,HIGH);
    beamPhase=2; beamPhaseAt=now;
  } else if(beamPhase==2 && uint32_t(now-beamPhaseAt)>=BEAM_PHASE_MS) {
    beamPhase=0;
    beamTestPassed=!beamTestFault;
  }
  if(beamPhase!=0) {
    beamClear=false; beamClearTiming=false;
    return; // UNKNOWN during testing; arm must remain confirmed OPEN.
  }
  beamBlocked=digitalRead(BEAM_PIN)==HIGH;
  if(beamBlocked) { beamClear=false; beamClearTiming=false; }
  else {
    if(!beamClearTiming) { beamClearAt=now; beamClearTiming=true; }
    beamClear=uint32_t(now-beamClearAt)>=BEAM_CLEAR_MS;
  }
}

bool isVehicleApproaching() { return approach.occupied; }
bool isExitOccupied() { return beamBlocked; }
bool allClear(uint32_t now) {
  return approach.clear(now) && beamClear && !beamBlocked &&
         beamPhase==0 && !beamTestFault && !manual;
}

void changeState(State next) {
  state=next;
  clearanceTiming=false;
}

void startClearance(uint32_t now) {
  if(moving || !openLimit.active || !openLimit.raw) return;
  changeState(State::CLEARANCE);
  // One test per closing attempt, only while the arm is confirmed raised.
  // After restoring the beam, start a NEW continuous-clear interval.
  beamTestPassed=false; beamClear=false; beamClearTiming=false;
  digitalWrite(EMITTER_PIN,LOW);
  beamPhase=1; beamPhaseAt=now;
}

void setBarrier(Demand next, uint32_t now) {
  if(actuatorStopped && next!=Demand::STOP) return;
  if(next==Demand::STOP) {
    barrier.detach(); digitalWrite(SERVO_PIN,LOW);
    demand=next; moving=false; actuatorStopped=true; return;
  }
  if(demand==next && barrier.attached()) return;
  demand=next; travelAt=servoStepAt=now; moving=true; endpointTiming=false;
  // Preload the target before attach: avoid the Servo default centre pulse.
  if(next==Demand::OPEN) commandAngle=OPEN_ANGLE;
  barrier.write(commandAngle);
  if(!barrier.attached()) barrier.attach(SERVO_PIN);
}

void updateServo(uint32_t now) {
  if(actuatorStopped || demand!=Demand::CLOSE || commandAngle==CLOSED_ANGLE) return;
  if(uint32_t(now-servoStepAt)<CLOSE_STEP_MS) return;
  servoStepAt=now; // No catch-up jump after a delayed loop.
  if(commandAngle<CLOSED_ANGLE) ++commandAngle; else --commandAngle;
  barrier.write(commandAngle); // Setpoint only; endpoint switches remain necessary.
}

void enterFault(Fault reason, uint32_t now, bool stopActuator=false) {
  if(fault==Fault::NONE || stopActuator) fault=reason;
  changeState(State::FAULT);
  digitalWrite(EMITTER_PIN,HIGH); beamPhase=0; beamTestPassed=false;
  setBarrier(stopActuator ? Demand::STOP : Demand::OPEN,now);
}

void superviseActuator(uint32_t now) {
  if(actuatorStopped) return;
  if(openLimit.active && closedLimit.active) {
    enterFault(Fault::LIMITS,now,true); return;
  }
  bool target=demand==Demand::OPEN ? openLimit.active : closedLimit.active;
  bool source=demand==Demand::OPEN ? closedLimit.active : openLimit.active;
  bool targetRaw=demand==Demand::OPEN ? openLimit.raw : closedLimit.raw;
  bool sourceRaw=demand==Demand::OPEN ? closedLimit.raw : openLimit.raw;
  if(!moving) {
    if(!target || source) enterFault(Fault::ACTUATOR,now,true);
    return;
  }
  // A delayed loop cannot retrospectively prove arrival before the deadline.
  if(uint32_t(now-travelAt)>TRAVEL_MS) {
    enterFault(Fault::ACTUATOR,now,true); return;
  }
  // A reversed arm may coast through a switch before physically reversing.
  // Require a fresh continuous endpoint dwell after each demand change.
  bool commandReached=demand==Demand::OPEN || commandAngle==CLOSED_ANGLE;
  if(commandReached && target && targetRaw && !source && !sourceRaw) {
    if(!endpointTiming) { endpointTiming=true; endpointAt=now; }
    if(uint32_t(now-endpointAt)>=ENDPOINT_SETTLE_MS) { moving=false; return; }
  } else endpointTiming=false;
  if((source && uint32_t(now-travelAt)>=DEPART_MS) ||
     uint32_t(now-travelAt)>=TRAVEL_MS)
    enterFault(Fault::ACTUATOR,now,true);
}

void startOpening(uint32_t now, bool newSession) {
  if(newSession) {
    sessionAt=now;
    crossingSeen=isExitOccupied(); waitNotice=false;
  }
  digitalWrite(EMITTER_PIN,HIGH); beamPhase=0; beamTestPassed=false;
  setBarrier(Demand::OPEN,now);
  changeState(State::OPENING);
}

void updateStateMachine(uint32_t now) {
  if(state==State::FAULT) return;
  if(beamTestFault) { enterFault(Fault::BEAM_TEST,now); return; }
  if(approach.failed(now) ||
     (uint32_t(now-bootAt)>=STARTUP_MS && !approach.everValid)) {
    enterFault(Fault::SONAR,now); return;
  }
  if(state==State::IDLE) {
    if(isVehicleApproaching() || isExitOccupied() || manual)
      startOpening(now,true);
    return;
  }
  if(state!=State::BOOT && uint32_t(now-sessionAt)>=WAIT_NOTICE_MS)
    waitNotice=true; // Advisory only: stopping is not a hardware failure.
  if(isExitOccupied() && beamPhase==0) crossingSeen=true;
  switch(state) {
    case State::BOOT:
      if(!moving && openLimit.active && approach.healthy(now)) {
        sessionAt=now;
        changeState(isExitOccupied() ? State::PASSING : State::OPEN);
      }
      break;
    case State::OPENING:
      if(!moving && openLimit.active)
        changeState(isExitOccupied() ? State::PASSING : State::OPEN);
      break;
    case State::OPEN:
      if(isExitOccupied()) changeState(State::PASSING);
      else if(allClear(now)) startClearance(now);
      break;
    case State::PASSING:
      if(allClear(now)) startClearance(now);
      break;
    case State::CLEARANCE:
      if(beamPhase!=0) { clearanceTiming=false; break; }
      if(!approach.clear(now) || beamBlocked || manual) {
        changeState(isExitOccupied() ? State::PASSING : State::OPEN);
      } else if(allClear(now) && beamTestPassed) {
        if(!clearanceTiming) { clearanceTiming=true; clearanceAt=now; }
        if(uint32_t(now-clearanceAt)>=CLEARANCE_MS && beamPhase==0 &&
           !moving && openLimit.active && openLimit.raw) {
          setBarrier(Demand::CLOSE,now); changeState(State::CLOSING);
        }
      }
      break;
    case State::CLOSING:
      // Raw/latest hazards override filtered confirmation while moving down.
      if(!allClear(now)) startOpening(now,false);
      else if(!moving && closedLimit.active) {
        waitNotice=false; changeState(State::IDLE);
      }
      break;
    default: break;
  }
}

void updateSensors(uint32_t now) {
  manual=digitalRead(MANUAL_PIN)==LOW; // Immediate assertion; release is clearance-gated.
  openLimit.update(digitalRead(OPEN_LIMIT_PIN)==LOW,now);
  closedLimit.update(digitalRead(CLOSED_LIMIT_PIN)==LOW,now);
  updateBeam(now);
  readDistance(now);
}

void setIndicators(uint32_t now) {
  bool green=fault==Fault::NONE && !moving && openLimit.active &&
    (state==State::OPEN || state==State::PASSING || state==State::CLEARANCE);
  bool red=state==State::FAULT ? ((now/250)%2==0) : !green;
  digitalWrite(GREEN_PIN,green ? HIGH : LOW);
  digitalWrite(RED_PIN,red ? HIGH : LOW);
  // Passive piezo only. tone uses Timer2; Servo uses Timer1 on Uno R3.
  bool beep=(state==State::FAULT && now%1000<150) ||
            (state==State::CLOSING && now%500<70);
  static bool sounded=false;
  if(beep!=sounded) {
    if(beep) tone(BUZZER_PIN,2200); else noTone(BUZZER_PIN);
    sounded=beep;
  }
}

void printDebug(uint32_t now) {
  static State reported=State::FAULT;
  static Fault reportedFault=Fault::NONE;
  if((state!=reported || fault!=reportedFault) && Serial.availableForWrite()>=48) {
    Serial.print(F("STATE "));
    switch(state) {
      case State::BOOT: Serial.print(F("BOOT")); break;
      case State::IDLE: Serial.print(F("IDLE")); break;
      case State::OPENING: Serial.print(F("OPENING")); break;
      case State::OPEN: Serial.print(F("OPEN")); break;
      case State::PASSING: Serial.print(F("PASSING")); break;
      case State::CLEARANCE: Serial.print(F("CLEARANCE")); break;
      case State::CLOSING: Serial.print(F("CLOSING")); break;
      case State::FAULT: Serial.print(F("FAULT")); break;
    }
    Serial.print(F(" fault=")); Serial.println(uint8_t(fault));
    reported=state; reportedFault=fault;
    return;
  }
  if(uint32_t(now-debugAt)<250 || Serial.availableForWrite()<60) return;
  debugAt=now;
  // Fixed, bounded record (<60 bytes). State/fault numeric legend in docs/architecture.md.
  Serial.print(F("s=")); Serial.print(uint8_t(state));
  Serial.print(F(" f=")); Serial.print(uint8_t(fault));
  Serial.print(F(" mm=")); Serial.print(approach.mm);
  Serial.print(F(" v=")); Serial.print(approach.valid);
  Serial.print(F(" a=")); Serial.print(approach.occupied);
  Serial.print(F(" b=")); Serial.print(beamBlocked);
  Serial.print(F(" o=")); Serial.print(openLimit.active);
  Serial.print(F(" c=")); Serial.print(closedLimit.active);
  Serial.print(F(" seen=")); Serial.print(crossingSeen);
  Serial.print(F(" w=")); Serial.println(waitNotice);
}

void serviceSerial(uint32_t now) {
  if(!Serial.available()) return;
  char c=Serial.read();
  // 'r' acknowledges a repaired sonar fault; never a motion or beam-test fault.
  if(c=='r' && state==State::FAULT && fault==Fault::SONAR && !actuatorStopped &&
     allClear(now) && beamPhase==0 && openLimit.active && !closedLimit.active &&
     !moving) {
    fault=Fault::NONE; sessionAt=now; waitNotice=false; changeState(State::OPEN);
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(TRIG_PIN,OUTPUT); digitalWrite(TRIG_PIN,LOW);
  pinMode(ECHO_PIN,INPUT);
  pinMode(BEAM_PIN,INPUT_PULLUP);
  pinMode(EMITTER_PIN,OUTPUT); digitalWrite(EMITTER_PIN,HIGH);
  pinMode(RED_PIN,OUTPUT); pinMode(GREEN_PIN,OUTPUT); pinMode(BUZZER_PIN,OUTPUT);
  pinMode(OPEN_LIMIT_PIN,INPUT_PULLUP); pinMode(CLOSED_LIMIT_PIN,INPUT_PULLUP);
  pinMode(MANUAL_PIN,INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ECHO_PIN),echoISR,CHANGE);
  bootAt=sessionAt=millis();
  // Seed endpoint state from hardware; an initially pressed switch is not a
  // later reactivation of an endpoint that was already left during travel.
  openLimit.raw=openLimit.active=digitalRead(OPEN_LIMIT_PIN)==LOW;
  closedLimit.raw=closedLimit.active=digitalRead(CLOSED_LIMIT_PIN)==LOW;
  openLimit.changed=closedLimit.changed=bootAt;
  setBarrier(Demand::OPEN,bootAt); // Startup never commands closed.
  setIndicators(bootAt);
}

void loop() {
  uint32_t now=millis();
  updateSensors(now);
  // Check actuator first; a jam must prevent all subsequent drive requests.
  superviseActuator(now);
  updateStateMachine(now);
  serviceSerial(now);
  updateServo(now);
  setIndicators(now);
  printDebug(now);
}
