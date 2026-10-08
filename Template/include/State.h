#pragma once
#include <stdint.h>

class Context;
class State
{
  /**
   * @var Context
   */

protected:
  Context *context_;

public:

  void set_context(Context *context)
  {
    this->context_ = context;
  }

  virtual void on_do() = 0;

  virtual void on_entry() = 0;

  virtual void on_exit() = 0;

  virtual void reset() = 0;

  virtual void on_operate() = 0;

  virtual void on_pre_operate() = 0;

  virtual void on_fault() = 0;

  virtual void controller_selector(char* inp) {};

  virtual void set_parameters(char* inp) {};

}; 