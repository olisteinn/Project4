#pragma once
#include "State.h"

class StopState : public State
{
public:
  void on_do() override;
  void on_entry() override;
  void on_exit() override;
  void reset() override;
  void on_operate() override;
  void on_pre_operate() override;
  void on_fault() override;
};

extern StopState stop_state;