#pragma once

class State;

class Context
{
private:
  State *state_;

public:
  Context(State *state);
  ~Context();

  void transition_to(State *state);

  void do_work();

  void reset();

  void on_operate();

  void on_pre_operate();

  void on_fault();

  void controller_selector(char* inp);

  void set_parameters(char* inp);

  State *get_state() const; 

};