#pragma once

#include "Msg.hpp"
#include "Cmd.hpp"
#include <memory> // std::unique_ptr
#include <vector>

class CmdHandler {
public:
  CmdHandler();
  ~CmdHandler();
  void execute(Cmd const&);
  std::vector<app::Msg> poll();
private:
  // Hide implementation from client
  class Impl;
  std::unique_ptr<Impl> m_pimpl;
}; // CmdHandler