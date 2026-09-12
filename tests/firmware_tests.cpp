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
  pins[CLOSED_LIMIT_PIN]=(bothLimits || physicalClosed) ? LOW : HIGH;
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
  // Invariant at each scheduler iteration, including random adversarial inputs.
  if(state==State::CLOSING)
    require(allClear(fakeMs) && fault==Fault::NONE,"unsafe closing state");
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
  setup(); run(3000);
  require(state==(expectIdle ? State::IDLE : State::PASSING),"startup did not safely reach expected state");
}
void arrive(){distanceMm=100;run(1000);require(state==State::OPEN,"arrival did not open");}
void pass(){blocked=true;run(200);distanceMm=240;run(300);blocked=false;run(2400);}

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
  if(name=="normal" || name=="rollover") {
    arrive();pass();require(state==State::IDLE && forwardEvidence,"normal crossing failed");
  } else if(name=="reset_open" || name=="reset_mid") {
    require(state==State::IDLE && demand==Demand::CLOSE,"reset recovery failed");
  } else if(name=="reset_blocked") {
    require(demand==Demand::OPEN,"reset closed on occupied beam");
    blocked=false;run(2400);require(state==State::IDLE,"reset clearance recovery failed");
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
    require(state==State::PASSING,"slow car not held");blocked=false;run(2400);
    require(state==State::IDLE,"slow crossing did not recover");
  } else if(name=="under_arm") {
    arrive();blocked=true;distanceMm=240;run(10000);
    require(demand==Demand::OPEN && state==State::PASSING,"under-arm hazard ignored");
  } else if(name=="noisy_approach") {
    for(int i=0;i<10;++i){distanceMm=100;run(65);distanceMm=240;run(195);}
    require(state==State::IDLE,"isolated noisy samples opened barrier");
  } else if(name=="exit_first") {
    blocked=true;run(1000);require(state==State::PASSING,"exit-first not held open");
    blocked=false;run(2400);require(state==State::IDLE,"exit-first recovery failed");
    require(!forwardEvidence,"exit-first fabricated forward passage");
  } else if(name=="reverse") {
    arrive();blocked=true;run(200);blocked=false;run(500);
    require(demand==Demand::OPEN,"reverse closed while approach occupied");
    distanceMm=240;run(2400);require(state==State::IDLE && !forwardEvidence,"reverse recovery failed");
  } else if(name=="abandon") {
    arrive();distanceMm=240;run(2400);
    require(state==State::IDLE && !crossingSeen,"aborted approach failed");
  } else if(name=="tailgate") {
    arrive();blocked=true;run(200);distanceMm=240;run(300);blocked=false;run(400);
    distanceMm=100;blocked=true;run(1000);
    require(demand==Demand::OPEN,"tailgate caused close");pass();
    require(state==State::IDLE,"tailgate recovery failed");
  } else if(name=="sonar_disconnect") {
    arrive();sonarOk=false;run(250);
    require(state==State::FAULT && fault==Fault::SONAR && demand==Demand::OPEN,"sonar fault failed");
    sonarOk=true;distanceMm=240;run(1000);Serial.queued='r';run(2400);
    require(state==State::IDLE,"repaired sonar acknowledgement failed");
  } else if(name=="beam_disconnect") {
    blocked=true;run(31000);
    require(state==State::FAULT && fault==Fault::DWELL && demand==Demand::OPEN,"beam loss closed");
  } else if(name=="beam_short") {
    shortLow=true;run(500);
    require(state==State::FAULT && fault==Fault::BEAM_TEST,"beam short not detected");
  } else if(name=="rapid") {
    arrive();distanceMm=240;
    for(int i=0;i<100;++i){blocked=true;run(10);blocked=false;run(10);}
    require(demand==Demand::OPEN,"rapid beam changes caused close");
    run(2400);require(state==State::IDLE,"rapid recovery failed");
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
    hold=false;run(2400);require(state==State::IDLE,"manual release recovery failed");
  } else if(name=="dwell") {
    arrive();blocked=true;run(31000);
    require(state==State::FAULT && demand==Demand::OPEN,"dwell forced closure");
  } else if(name=="hysteresis") {
    arrive();distanceMm=180;run(3000);
    require(approach.occupied && demand==Demand::OPEN,"hysteresis lost occupied state");
  } else if(name=="echo") {
    echoArmed=true;echoRisen=false;echoDone=false;fakeUs=UINT32_MAX-500;
    pins[ECHO_PIN]=HIGH;echoISR();fakeUs+=1392; pins[ECHO_PIN]=LOW;echoISR();
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
