#include "Context.h"
#include "State.h"

Context::Context(State *state) : state_(nullptr)
{
  this->transition_to(state);
}

void Context::transition_to(State *state)
{
  if (this->state_ != nullptr)
  {
    this->state_->on_exit();
  }

  this->state_ = state;

  this->state_->set_context(this);

  this->state_->on_entry();
}

void Context::reset()
{
  this->state_->reset();
}

void Context::on_operate()
{
  this->state_->on_operate();
}

void Context::on_pre_operate()
{
  this->state_->on_pre_operate();
}

void Context::on_fault()
{
  this->state_->on_fault();
}

State *Context::get_state() const
{
  return this->state_;
}