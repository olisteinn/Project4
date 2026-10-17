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

  void reset();

  void on_operate();

  void on_pre_operate();

  void on_fault();

  State *get_state() const; 

};