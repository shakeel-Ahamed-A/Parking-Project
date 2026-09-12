#pragma once
struct Servo {
  bool enabled=false;
  int angle=90;
  void write(int value){angle=value;}
  void attach(int){enabled=true;}
  void detach(){enabled=false;}
  bool attached(){return enabled;}
};
