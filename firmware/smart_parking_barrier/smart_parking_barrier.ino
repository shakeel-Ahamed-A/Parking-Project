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
constexpr uint32_t BEAM_TEST_MS=250, BEAM_PHASE_MS=3, BEAM_CLEAR_MS=100;
constexpr uint32_t DEBOUNCE_MS=20, CLEARANCE_MS=1200, TRAVEL_MS=2500;
constexpr uint32_t DEPART_MS=500, SESSION_MS=30000, STARTUP_MS=2000;
constexpr uint8_t CONFIRM_SAMPLES=3, BAD_LIMIT=3;

enum class State : uint8_t { BOOT, IDLE, OPENING, OPEN, PASSING,
                            CLEARANCE, CLOSING, FAULT };
enum class Fault : uint8_t { NONE, SONAR, BEAM_TEST, DWELL, LIMITS, ACTUATOR };
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
bool actuatorStopped=false, moving=true, manual=false;
bool beamBlocked=true, beamClear=false, beamTestFault=false, beamTestPassed=false;
bool beamClearTiming=false, clearanceTiming=false, sourceReleased=false;
bool approachSeen=false, crossingSeen=false, forwardEvidence=false;
uint8_t beamPhase=0; // 0=normal, 1=emitter off, 2=settling after on.
uint32_t bootAt=0, sessionAt=0, travelAt=0, clearanceAt=0;
uint32_t beamPhaseAt=0, beamTestAt=0, beamClearAt=0, debugAt=0;
uint32_t pingAt=0, pingStartedUs=0;
bool pingWaiting=false;
volatile bool echoArmed=false, echoRisen=false, echoDone=false;
volatile uint32_t echoRiseUs=0, echoWidthUs=0;

void echoISR() {
  if(!echoArmed) return;
  if(digitalRead(ECHO_PIN)==HIGH) {
    if(!echoRisen) { echoRiseUs=micros(); echoRisen=true; }
  } else if(echoRisen) {
    echoWidthUs=uint32_t(micros()-echoRiseUs);
    echoDone=true; echoArmed=false;
  }
}

// Asynchronous echo capture; only the 10 us trigger pulse is synchronous.
void readDistance(uint32_t now) {
  if(pingWaiting) {
    noInterrupts();
    bool done=echoDone;
    uint32_t width=echoWidthUs;
    interrupts();
    if(done || uint32_t(micros()-pingStartedUs)>=ECHO_TIMEOUT_US) {
      noInterrupts(); echoArmed=false; echoDone=false; interrupts();
      pingWaiting=false;
      approach.sample(done ? uint16_t(width*10UL/58UL) : 0,
                      done && width<=ECHO_TIMEOUT_US, now);
    }
  }
  if(!pingWaiting && uint32_t(now-pingAt)>=PING_MS) {
    pingAt=now;
    if(digitalRead(ECHO_PIN)==HIGH) { approach.sample(0,false,now); return; }
    noInterrupts();
    echoRisen=false; echoDone=false; echoArmed=true;
    interrupts();
    pingStartedUs=micros(); pingWaiting=true;
    digitalWrite(TRIG_PIN,HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN,LOW);
  }
}

void updateBeam(uint32_t now) {
  if(beamPhase==1 && uint32_t(now-beamPhaseAt)>=BEAM_PHASE_MS) {
    // Specified receiver: illuminated=LOW; emitter off must produce HIGH.
    if(digitalRead(BEAM_PIN)!=HIGH) beamTestFault=true;
    else { beamTestPassed=true; beamTestAt=now; }
    digitalWrite(EMITTER_PIN,HIGH);
    beamPhase=2; beamPhaseAt=now;
  } else if(beamPhase==2 && uint32_t(now-beamPhaseAt)>=BEAM_PHASE_MS) {
    beamPhase=0;
  }
  if(beamPhase!=0) return; // Retain last observation during bounded 6 ms test.
  beamBlocked=digitalRead(BEAM_PIN)==HIGH;
  if(beamBlocked) { beamClear=false; beamClearTiming=false; }
  else {
    if(!beamClearTiming) { beamClearAt=now; beamClearTiming=true; }
    beamClear=uint32_t(now-beamClearAt)>=BEAM_CLEAR_MS;
  }
  if(uint32_t(now-beamTestAt)>=BEAM_TEST_MS) {
    digitalWrite(EMITTER_PIN,LOW);
    beamPhase=1; beamPhaseAt=now;
  }
}

bool isVehicleApproaching() { return approach.occupied; }
bool isExitOccupied() { return beamBlocked; }
bool allClear(uint32_t now) {
  return approach.clear(now) && beamClear && !beamBlocked &&
         beamTestPassed && !beamTestFault && !manual;
}

void changeState(State next) {
  state=next;
  clearanceTiming=false;
}

void setBarrier(Demand next, uint32_t now) {
  if(actuatorStopped && next!=Demand::STOP) return;
  if(next==Demand::STOP) {
    barrier.detach(); digitalWrite(SERVO_PIN,LOW);
    demand=next; moving=false; actuatorStopped=true; return;
  }
  if(demand==next && barrier.attached()) return;
  demand=next; travelAt=now; moving=true; sourceReleased=false;
  // Preload the target before attach: avoid the Servo default centre pulse.
  barrier.write(next==Demand::OPEN ? OPEN_ANGLE : CLOSED_ANGLE);
  if(!barrier.attached()) barrier.attach(SERVO_PIN);
}

void enterFault(Fault reason, uint32_t now, bool stopActuator=false) {
  if(fault==Fault::NONE || stopActuator) fault=reason;
  changeState(State::FAULT);
  setBarrier(stopActuator ? Demand::STOP : Demand::OPEN,now);
}

void superviseActuator(uint32_t now) {
  if(actuatorStopped) return;
  if(openLimit.active && closedLimit.active) {
    enterFault(Fault::LIMITS,now,true); return;
  }
  bool target=demand==Demand::OPEN ? openLimit.active : closedLimit.active;
  bool source=demand==Demand::OPEN ? closedLimit.active : openLimit.active;
  if(!moving) {
    if(!target || source) enterFault(Fault::ACTUATOR,now,true);
    return;
  }
  if(!source) sourceReleased=true;
  if(target && !source) { moving=false; return; }
  if((sourceReleased && source) ||
     (source && uint32_t(now-travelAt)>=DEPART_MS) ||
     uint32_t(now-travelAt)>=TRAVEL_MS)
    enterFault(Fault::ACTUATOR,now,true);
}

void startOpening(uint32_t now, bool newSession) {
  if(newSession) {
    sessionAt=now;
    approachSeen=isVehicleApproaching();
    crossingSeen=false; forwardEvidence=false;
  }
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
  if(manual) sessionAt=now; // Intentional hold-open has no dwell deadline.
  if(state!=State::BOOT && uint32_t(now-sessionAt)>=SESSION_MS) {
    enterFault(Fault::DWELL,now); return; // Never force-close on timeout.
  }
  approachSeen=approachSeen || isVehicleApproaching();
  if(isExitOccupied()) {
    crossingSeen=true;
    if(approachSeen && approach.clear(now)) forwardEvidence=true;
  }
  switch(state) {
    case State::BOOT:
      if(!moving && openLimit.active && approach.healthy(now) && beamTestPassed) {
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
      else if(allClear(now)) changeState(State::CLEARANCE);
      break;
    case State::PASSING:
      if(allClear(now)) changeState(State::CLEARANCE);
      break;
    case State::CLEARANCE:
      if(!allClear(now)) {
        changeState(isExitOccupied() ? State::PASSING : State::OPEN);
      } else {
        if(!clearanceTiming) { clearanceTiming=true; clearanceAt=now; }
        if(uint32_t(now-clearanceAt)>=CLEARANCE_MS && beamPhase==0) {
          setBarrier(Demand::CLOSE,now); changeState(State::CLOSING);
        }
      }
      break;
    case State::CLOSING:
      // Raw/latest hazards override filtered confirmation while moving down.
      if(!allClear(now)) startOpening(now,false);
      else if(!moving && closedLimit.active) changeState(State::IDLE);
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
  if(uint32_t(now-debugAt)<250 || Serial.availableForWrite()<60) return;
  debugAt=now;
  // Fixed, bounded record (<60 bytes). State/fault numeric legend in README.
  Serial.print(F("s=")); Serial.print(uint8_t(state));
  Serial.print(F(" f=")); Serial.print(uint8_t(fault));
  Serial.print(F(" mm=")); Serial.print(approach.mm);
  Serial.print(F(" v=")); Serial.print(approach.valid);
  Serial.print(F(" a=")); Serial.print(approach.occupied);
  Serial.print(F(" b=")); Serial.print(beamBlocked);
  Serial.print(F(" o=")); Serial.print(openLimit.active);
  Serial.print(F(" c=")); Serial.print(closedLimit.active);
  Serial.print(F(" p=")); Serial.println(forwardEvidence);
}

void serviceSerial(uint32_t now) {
  if(!Serial.available()) return;
  char c=Serial.read();
  // 'r' acknowledges sensor/dwell faults after inspection; never a motion fault.
  if(c=='r' && state==State::FAULT && !actuatorStopped &&
     allClear(now) && beamPhase==0 && openLimit.active && !closedLimit.active &&
     !moving) {
    fault=Fault::NONE; sessionAt=now; changeState(State::OPEN);
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
  setIndicators(now);
  printDebug(now);
}
