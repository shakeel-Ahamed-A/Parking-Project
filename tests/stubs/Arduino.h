#pragma once
#include <stdint.h>
#include <stddef.h>
#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
#define INPUT_PULLUP 2
#define CHANGE 3
#define A0 14
#define A1 15
#define A2 16
#define F(s) s
extern uint32_t fakeMs, fakeUs;
extern int pins[24];
extern bool blocked, shortLow;
inline uint32_t millis(){return fakeMs;}
inline uint32_t micros(){return fakeUs;}
inline void pinMode(int,int){}
inline void digitalWrite(int p,int v){pins[p]=v;}
inline int digitalRead(int p){
  if(p==3) return shortLow ? LOW : ((pins[5]==HIGH && !blocked) ? LOW : HIGH);
  return pins[p];
}
inline void delayMicroseconds(int n){fakeUs+=n;}
inline void noInterrupts(){}
inline void interrupts(){}
inline int digitalPinToInterrupt(int p){return p;}
inline void attachInterrupt(int,void(*)(),int){}
inline void tone(int,int){}
inline void noTone(int){}
struct FakeSerial {
  int queued=-1;
  void begin(int){}
  int availableForWrite(){return 64;}
  int available(){return queued>=0;}
  int read(){int c=queued; queued=-1; return c;}
  template<class T> void print(T){}
  template<class T> void println(T){}
};
extern FakeSerial Serial;
