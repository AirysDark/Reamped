#pragma once

class Tas5731m;
class TestTone;

class SerialConsole {
public:
  SerialConsole(Tas5731m &amp, TestTone &tone);

  void printHelp();
  void printStatus();
  void service();

private:
  Tas5731m &amp_;
  TestTone &tone_;
};
