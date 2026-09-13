// Runs the actual .ino with substituted GPIO, clock and Servo interfaces.
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Arduino.h"
uint32_t fakeMs=0, fakeUs=0;
int pins[24]={};
bool blocked=false, shortLow=false;
FakeSerial Serial;
#include "../firmware/smart_parking_barrier/smart_parking_barrier.ino"

void require(bool condition,const char* message) {
  if(!condition){fprintf(stderr,"FAIL: %s\n",message); exit(1);}
}
uint16_t distanceMm=240;
bool sonarOk=true, jam=false, bothLimits=false, hold=false;
bool physicalOpen=false, physicalClosed=true;
bool suppressClosed=false;
Demand lastDemand=Demand::OPEN;
uint32_t motionStart=0, lastSample=0;

void tick() {
  ++fakeMs; fakeUs+=1000;
  if(demand!=lastDemand) {lastDemand=demand;motionStart=fakeMs;}
  if(!jam && demand!=Demand::STOP) {
    uint32_t age=fakeMs-motionStart;
    if(age>50) {physicalOpen=false;physicalClosed=false;}
    if(age>=600) {
      physicalOpen=demand==Demand::OPEN;
      physicalClosed=demand==Demand::CLOSE;
    }
  }
  pins[OPEN_LIMIT_PIN]=(bothLimits || physicalOpen) ? LOW : HIGH;
  pins[CLOSED_LIMIT_PIN]=(bothLimits || (physicalClosed && !suppressClosed)) ? LOW : HIGH;
  manual=hold;
  openLimit.update(pins[OPEN_LIMIT_PIN]==LOW,fakeMs);
  closedLimit.update(pins[CLOSED_LIMIT_PIN]==LOW,fakeMs);
  updateBeam(fakeMs);
  if(uint32_t(fakeMs-lastSample)>=PING_MS) {
    lastSample=fakeMs; approach.sample(distanceMm,sonarOk,fakeMs);
  }
  superviseActuator(fakeMs);
  updateStateMachine(fakeMs);
  serviceSerial(fakeMs);
  updateServo(fakeMs);
  // Invariant at each scheduler iteration, including random adversarial inputs.
  if(state==State::CLOSING)
    require(allClear(fakeMs) && fault==Fault::NONE,"unsafe closing state");
  if(demand==Demand::CLOSE) {
    require(beamPhase==0 && pins[EMITTER_PIN]==HIGH,"beam blanked during descent");
    require(!blocked && !hold,"physical fixture hazard ignored during descent");
  }
  if(actuatorStopped) require(!barrier.attached(),"stopped actuator reattached");
}
void run(uint32_t duration){for(uint32_t n=0;n<duration;++n)tick();}
void boot(uint32_t start=0, bool expectIdle=true) {
  fakeMs=start; fakeUs=start*1000UL;
  motionStart=lastSample=start;
  if(physicalOpen) motionStart=start-600;
  pins[MANUAL_PIN]=HIGH;
  pins[OPEN_LIMIT_PIN]=physicalOpen ? LOW : HIGH;
  pins[CLOSED_LIMIT_PIN]=physicalClosed ? LOW : HIGH;
  setup(); run(4500);
  require(state==(expectIdle ? State::IDLE : State::PASSING),"startup did not safely reach expected state");
}
void arrive(){distanceMm=100;run(1000);require(state==State::OPEN,"arrival did not open");}
void pass(){blocked=true;run(200);distanceMm=240;run(300);blocked=false;run(3500);}

int main(int argc,char** argv) {
  require(argc==2,"supply scenario");
  struct Name {
    const char* value;
    bool operator==(const char* other) const {return strcmp(value,other)==0;}
  } name={argv[1]};
  if(name=="reset_open") {physicalOpen=true;physicalClosed=false;}
  if(name=="reset_mid") physicalClosed=false;
  if(name=="reset_blocked") blocked=true;
  if(name=="rollover") boot(UINT32_MAX-1500); else boot(0,!(name=="reset_blocked"));
  if(name=="long_stop_recovers") {
    arrive();blocked=true;run(31000);blocked=false;distanceMm=240;run(3500);
    require(state==State::IDLE,"normal long stop required fault acknowledgement");
  } else if(name=="beam_monitor_closing") {
    arrive();distanceMm=240;
    for(int n=0;n<4000 && state!=State::CLOSING;++n)tick();
    require(state==State::CLOSING,"closing was not reached");
    for(int n=0;n<400;++n) {
      tick();require(pins[EMITTER_PIN]==HIGH,"diagnostic blinded beam during descent");
    }
  } else if(name=="reversal_endpoint") {
    arrive();setBarrier(Demand::CLOSE,fakeMs);changeState(State::CLOSING);
    blocked=true;beamBlocked=true;updateStateMachine(fakeMs);
    require(demand==Demand::OPEN,"hazard did not request open");
    // The old debounced open switch remains true briefly after leaving it.
    openLimit.raw=false;openLimit.active=true;openLimit.changed=fakeMs;
    superviseActuator(fakeMs+1);
    openLimit.active=false;superviseActuator(fakeMs+25);
    require(!actuatorStopped && moving,"stale endpoint defeated reopening");
    openLimit.raw=openLimit.active=true;superviseActuator(fakeMs+100);
    superviseActuator(fakeMs+199);require(moving,"reopen settled before fresh dwell");
    superviseActuator(fakeMs+200);require(!moving && !actuatorStopped,"reopen never completed fresh dwell");
  } else if(name=="reverse_source_coast") {
    arrive();openLimit.raw=openLimit.active=false;
    setBarrier(Demand::CLOSE,fakeMs);setBarrier(Demand::OPEN,fakeMs);
    superviseActuator(fakeMs+1);
    closedLimit.raw=closedLimit.active=true;superviseActuator(fakeMs+50);
    require(!actuatorStopped,"brief reverse coast tripped source-reactivation fault");
    closedLimit.raw=closedLimit.active=false;superviseActuator(fakeMs+100);
    openLimit.raw=openLimit.active=true;superviseActuator(fakeMs+200);
    superviseActuator(fakeMs+300);
    require(!moving && !actuatorStopped,"coasting reversal did not complete");
  } else if(name=="late_echo") {
    pingAt=fakeMs;pingWaiting=true;pingStartedUs=fakeUs;
    echoArmed=true;echoRisen=false;echoDone=false;
    fakeUs+=26000;pins[ECHO_PIN]=HIGH;echoISR();
    fakeUs+=1392;pins[ECHO_PIN]=LOW;echoISR();
    readDistance(fakeMs+28);
    require(!approach.valid,"late echo authorized a fresh reading");
  } else if(name=="buffered_echo_stale") {
    pingWaiting=true;pingAt=fakeMs;pingStartedUs=fakeUs;
    echoArmed=true;echoRisen=false;echoDone=false;
    pins[ECHO_PIN]=HIGH;echoISR();fakeUs+=1392;pins[ECHO_PIN]=LOW;echoISR();
    fakeUs+=301000;readDistance(fakeMs+302);
    require(!approach.valid,"old buffered echo became fresh");
  } else if(name=="preclose_fresh_interval") {
    arrive();distanceMm=240;
    for(int n=0;n<2000 && state!=State::CLEARANCE;++n)tick();
    require(beamPhase==1 && demand==Demand::OPEN,"no raised preclose diagnostic");
    run(BEAM_PHASE_MS*2+1199);
    require(demand==Demand::OPEN,"diagnostic time counted as confirmed clearance");
    run(200);require(state==State::CLOSING,"clearance did not finish after fresh samples");
  } else if(name=="single_near_closing") {
    arrive();distanceMm=240;
    for(int n=0;n<4000 && state!=State::CLOSING;++n)tick();
    require(state==State::CLOSING,"closing not reached");
    approach.sample(100,true,fakeMs);updateStateMachine(fakeMs);
    require(!approach.occupied && demand==Demand::OPEN,"raw near reading waited for three confirmations");
  } else if(name=="closing_speed") {
    arrive();distanceMm=240;
    for(int n=0;n<4000 && state!=State::CLOSING;++n)tick();
    require(state==State::CLOSING,"closing not reached");
    uint32_t start=fakeMs;int previous=commandAngle;
    while(state==State::CLOSING && uint32_t(fakeMs-start)<TRAVEL_MS+100) {
      tick();require(abs(int(commandAngle)-previous)<=1,"closing command jumped");
      previous=commandAngle;
    }
    require(state==State::IDLE && uint32_t(fakeMs-start)>=ARM_DEGREES*CLOSE_STEP_MS,
            "closing did not complete through the bounded ramp");
  } else if(name=="sonar_fault_closing") {
    arrive();distanceMm=240;
    for(int n=0;n<4000 && state!=State::CLOSING;++n)tick();
    require(state==State::CLOSING,"closing not reached");
    sonarOk=false;run(1000);
    require(state==State::FAULT && demand==Demand::OPEN && !actuatorStopped,"sonar failure during descent did not reopen");
  } else if(name=="raw_endpoint_veto") {
    arrive();
    for(int n=0;n<3;++n)approach.sample(240,true,fakeMs);
    openLimit.raw=false;openLimit.active=true;
    updateStateMachine(fakeMs);
    require(state==State::OPEN && beamPhase==0,"diagnostic started on stale open endpoint");
    changeState(State::CLEARANCE);beamTestPassed=true;clearanceTiming=true;
    clearanceAt=fakeMs-CLEARANCE_MS-1;updateStateMachine(fakeMs);
    require(demand==Demand::OPEN,"closing started with unconfirmed raw endpoint");
  } else if(name=="obstruction_withdrawn_reopen") {
    arrive();distanceMm=240;
    for(int n=0;n<4000 && state!=State::CLOSING;++n)tick();
    require(state==State::CLOSING,"closing not reached");
    blocked=true;tick();blocked=false;run(350);
    require(state==State::OPENING && demand==Demand::OPEN,"withdrawn hazard cancelled necessary reopening");
    run(4000);require(state==State::IDLE,"withdrawn hazard did not recover");
  } else if(name=="beam_false_clear_pulse") {
    arrive();blocked=true;distanceMm=240;run(500);
    shortLow=true;run(50);shortLow=false;run(500);
    require(state==State::PASSING && demand==Demand::OPEN && fault==Fault::NONE,
            "brief false-clear pulse authorized descent");
  } else if(name=="closed_endpoint_missing") {
    arrive();distanceMm=240;
    for(int n=0;n<4000 && state!=State::CLOSING;++n)tick();
    require(state==State::CLOSING,"closing not reached");
    suppressClosed=true;run(TRAVEL_MS+100);
    require(state==State::FAULT && actuatorStopped,"missing closed feedback did not stop motion");
  } else if(name=="repeated_cycles") {
    for(int n=0;n<20;++n) {
      arrive();require(!crossingSeen && !waitNotice,"old session flags leaked");
      pass();require(state==State::IDLE && crossingSeen,"repeated crossing failed");
    }
  } else if(name=="timing_boundaries") {
    // Group boundary checks instead of reporting each assertion as a new test.
    const uint32_t base=UINT32_MAX-1000;
    Range range;
    for(int n=0;n<3;++n)range.sample(240,true,base);
    require(range.clear(base+299),"fresh sample rejected before stale limit");
    require(!range.clear(base+300) && range.failed(base+300),"stale limit not enforced at equality");
    require(!range.clear(base+301),"stale sample accepted after limit");
    Debounced sw;
    sw.update(true,base);sw.update(true,base+19);require(!sw.active,"debounce too early");
    sw.update(true,base+20);require(sw.active,"debounce missing at equality");
    sw.update(true,base+21);require(sw.active,"debounce lost after limit");
    blocked=false;beamPhase=0;beamClearTiming=false;digitalWrite(EMITTER_PIN,HIGH);
    updateBeam(base);updateBeam(base+99);require(!beamClear,"beam cleared early");
    updateBeam(base+100);require(beamClear,"beam clear equality failed");
    updateBeam(base+101);require(beamClear,"beam clear after-limit failed");
    for(int offset=-1;offset<=1;++offset) {
      uint32_t now=base+CLEARANCE_MS+offset;
      state=State::CLEARANCE;fault=Fault::NONE;actuatorStopped=false;moving=false;
      demand=Demand::OPEN;barrier.enabled=true;openLimit.raw=openLimit.active=true;
      closedLimit.raw=closedLimit.active=false;manual=false;beamBlocked=false;
      beamClear=true;beamTestPassed=true;beamTestFault=false;beamPhase=0;
      clearanceTiming=true;clearanceAt=base;
      for(int n=0;n<3;++n)approach.sample(240,true,now);
      updateStateMachine(now);
      require((demand==Demand::CLOSE)==(offset>=0),"clearance threshold wrong");
    }
    for(int offset=-1;offset<=1;++offset) {
      fault=Fault::NONE;actuatorStopped=false;moving=true;demand=Demand::OPEN;
      travelAt=base;endpointTiming=false;openLimit.raw=openLimit.active=false;
      closedLimit.raw=closedLimit.active=true;
      superviseActuator(base+DEPART_MS+offset);
      require(actuatorStopped==(offset>=0),"source release deadline wrong");
    }
    for(int offset=-1;offset<=1;++offset) {
      fault=Fault::NONE;actuatorStopped=false;moving=true;demand=Demand::OPEN;
      travelAt=base;endpointTiming=false;openLimit.raw=openLimit.active=false;
      closedLimit.raw=closedLimit.active=false;
      superviseActuator(base+TRAVEL_MS+offset);
      require(actuatorStopped==(offset>=0),"missing target deadline wrong");
    }
    for(int offset=-1;offset<=1;++offset) {
      fault=Fault::NONE;actuatorStopped=false;moving=true;demand=Demand::OPEN;
      travelAt=base;endpointTiming=true;endpointAt=base+TRAVEL_MS-ENDPOINT_SETTLE_MS;
      openLimit.raw=openLimit.active=true;closedLimit.raw=closedLimit.active=false;
      superviseActuator(base+TRAVEL_MS+offset);
      require(offset<0 ? (moving && !actuatorStopped) :
              offset==0 ? (!moving && !actuatorStopped) : actuatorStopped,
              "late settled target accepted after travel deadline");
    }
    for(int offset=-1;offset<=1;++offset) {
      fault=Fault::NONE;actuatorStopped=false;demand=Demand::CLOSE;
      commandAngle=OPEN_ANGLE;servoStepAt=base;
      updateServo(base+CLOSE_STEP_MS+offset);
      require(commandAngle==(offset<0 ? OPEN_ANGLE : OPEN_ANGLE-1),"servo step boundary wrong");
    }
    for(int offset=-1;offset<=1;++offset) {
      fakeUs=100000;pingStartedUs=fakeUs;pingAt=fakeMs;pingWaiting=true;
      echoArmed=true;echoRisen=false;echoDone=false;
      fakeUs=pingStartedUs+ECHO_TIMEOUT_US+offset-1392;
      pins[ECHO_PIN]=HIGH;echoISR();fakeUs+=1392;pins[ECHO_PIN]=LOW;echoISR();
      readDistance(fakeMs);
      require(approach.valid==(offset<=0),"echo transaction deadline wrong");
    }
    for(int offset=-1;offset<=1;++offset) {
      uint32_t now=base+WAIT_NOTICE_MS+offset;
      state=State::PASSING;fault=Fault::NONE;waitNotice=false;sessionAt=base;
      beamBlocked=true;beamPhase=0;
      approach.sample(240,true,now);updateStateMachine(now);
      require(waitNotice==(offset>=0) && state==State::PASSING && fault==Fault::NONE,
              "wait advisory boundary changed state");
    }
  } else if(name=="normal" || name=="rollover") {
    arrive();pass();require(state==State::IDLE && crossingSeen,"normal crossing failed");
  } else if(name=="reset_open" || name=="reset_mid") {
    require(state==State::IDLE && demand==Demand::CLOSE,"reset recovery failed");
  } else if(name=="reset_blocked") {
    require(demand==Demand::OPEN,"reset closed on occupied beam");
    blocked=false;run(3500);require(state==State::IDLE,"reset clearance recovery failed");
  } else if(name=="arrival_timeout") {
    distanceMm=100;run(400);jam=true;physicalOpen=physicalClosed=false;run(3000);
    require(state==State::FAULT && actuatorStopped,"arrival timeout missed");
  } else if(name=="lost_endpoint") {
    jam=true;physicalClosed=false;run(30);
    require(state==State::FAULT && actuatorStopped,"lost endpoint at rest missed");
  } else if(name=="stale") {
    approach.lastGood=fakeMs-STALE_MS;updateStateMachine(fakeMs);
    require(state==State::FAULT && fault==Fault::SONAR,"stale sample accepted");
  } else if(name=="stop_approach") {
    arrive();run(5000);require(demand==Demand::OPEN,"closed on waiting car");
  } else if(name=="slow") {
    arrive();blocked=true;run(5000);distanceMm=240;run(5000);
    require(state==State::PASSING,"slow car not held");blocked=false;run(3500);
    require(state==State::IDLE,"slow crossing did not recover");
  } else if(name=="under_arm") {
    arrive();blocked=true;distanceMm=240;run(10000);
    require(demand==Demand::OPEN && state==State::PASSING,"under-arm hazard ignored");
  } else if(name=="noisy_approach") {
    for(int i=0;i<10;++i){distanceMm=100;run(65);distanceMm=240;run(195);}
    require(state==State::IDLE,"isolated noisy samples opened barrier");
  } else if(name=="exit_first") {
    blocked=true;run(1000);require(state==State::PASSING,"exit-first not held open");
    blocked=false;run(3500);require(state==State::IDLE,"exit-first recovery failed");
    require(crossingSeen,"exit-first occupancy was not recorded");
  } else if(name=="reverse") {
    arrive();blocked=true;run(200);blocked=false;run(500);
    require(demand==Demand::OPEN,"reverse closed while approach occupied");
    distanceMm=240;run(3500);require(state==State::IDLE,"reverse recovery failed");
  } else if(name=="abandon") {
    arrive();distanceMm=240;run(3500);
    require(state==State::IDLE && !crossingSeen,"aborted approach failed");
  } else if(name=="tailgate") {
    arrive();blocked=true;run(200);distanceMm=240;run(300);blocked=false;run(400);
    distanceMm=100;blocked=true;run(1000);
    require(demand==Demand::OPEN,"tailgate caused close");pass();
    require(state==State::IDLE,"tailgate recovery failed");
  } else if(name=="sonar_disconnect") {
    arrive();sonarOk=false;run(250);
    require(state==State::FAULT && fault==Fault::SONAR && demand==Demand::OPEN,"sonar fault failed");
    sonarOk=true;distanceMm=240;run(1000);Serial.queued='r';run(3500);
    require(state==State::IDLE,"repaired sonar acknowledgement failed");
  } else if(name=="beam_disconnect") {
    blocked=true;run(31000);
    require(state==State::PASSING && waitNotice && fault==Fault::NONE && demand==Demand::OPEN,"beam loss closed or latched nuisance fault");
  } else if(name=="beam_short") {
    arrive();shortLow=true;distanceMm=240;run(2000);
    require(state==State::FAULT && fault==Fault::BEAM_TEST,"beam short not detected");
  } else if(name=="rapid") {
    arrive();distanceMm=240;
    for(int i=0;i<100;++i){blocked=true;run(10);blocked=false;run(10);}
    require(demand==Demand::OPEN,"rapid beam changes caused close");
    run(3500);require(state==State::IDLE,"rapid recovery failed");
  } else if(name=="reopen") {
    arrive();distanceMm=240;
    while(state!=State::CLOSING) tick();
    blocked=true;run(10);
    require(demand==Demand::OPEN,"did not reopen during close");
  } else if(name=="single_invalid_closing") {
    arrive();distanceMm=240;while(state!=State::CLOSING)tick();
    approach.sample(0,false,fakeMs);tick();
    require(demand==Demand::OPEN,"invalid latest sample failed to veto close");
  } else if(name=="jam") {
    jam=true;distanceMm=100;run(1000);
    require(state==State::FAULT && actuatorStopped,"jam not stopped");
    Serial.queued='r';run(1000);require(actuatorStopped,"serial restarted jam");
  } else if(name=="limits") {
    bothLimits=true;run(30);
    require(state==State::FAULT && fault==Fault::LIMITS && actuatorStopped,"contradictory limits failed");
  } else if(name=="manual") {
    hold=true;run(35000);require(demand==Demand::OPEN && fault==Fault::NONE,"manual hold failed");
    hold=false;run(3500);require(state==State::IDLE,"manual release recovery failed");
  } else if(name=="dwell") {
    arrive();blocked=true;run(31000);
    require(state==State::PASSING && waitNotice && fault==Fault::NONE && demand==Demand::OPEN,"dwell forced closure or nuisance fault");
  } else if(name=="hysteresis") {
    arrive();distanceMm=180;run(3000);
    require(approach.occupied && demand==Demand::OPEN,"hysteresis lost occupied state");
  } else if(name=="echo") {
    echoArmed=true;echoRisen=false;echoDone=false;fakeUs=UINT32_MAX-500;
    pingStartedUs=fakeUs; pins[ECHO_PIN]=HIGH;echoISR();fakeUs+=1392; pins[ECHO_PIN]=LOW;echoISR();
    require(echoDone && echoWidthUs==1392,"interrupt duration wrap failed");
    pingWaiting=true;pingAt=fakeMs;readDistance(fakeMs);
    require(approach.valid && approach.mm==240,"echo conversion failed");
  } else if(name=="echo_timeout") {
    echoArmed=true;echoDone=false;pingWaiting=true;pingStartedUs=fakeUs-25001;pingAt=fakeMs;
    readDistance(fakeMs);require(!approach.valid && !pingWaiting,"echo timeout blocked/cleared");
  } else if(name=="random") {
    uint32_t seed=1234567;
    for(int i=0;i<20000;++i) {
      seed=seed*1664525UL+1013904223UL;
      if(i%83==0){distanceMm=(seed&1)?100:240;blocked=(seed&2)!=0;}
      tick();
    }
  } else {require(false,"unknown scenario");}
  printf("PASS %s\n",name.value);
}
