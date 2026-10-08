#pragma once
#include "State.h"

class PreOperational : public State
{
public:
  void on_do() override;
  void on_entry() override;
  void on_exit() override;

  void reset() override;
  void on_operate() override;
  void on_pre_operate() override;
  void on_fault() override;
  void controller_selector(char* inp) override;
  void set_parameters(char* inp) override;
};

extern PreOperational pre_op_state;