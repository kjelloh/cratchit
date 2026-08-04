#include "CmdHandler.hpp"
#include "cmd_to_msg.hpp" // client provided binding descriptor -> result_type -> Msg

#include <map>

namespace detail {

  template <typename ConcreteCmd>
  class Executor {
  public:
  private:
  }; // Executor<>

  template <>
  class Executor<TestCmdDescriptor> {
  public:
    void start() {
      ++m_activation_count;
    }
  private:
    size_t m_activation_count{};
  }; // Executor<>


} // detail

using CmdExecutor = std::variant<
  detail::Executor<TestCmdDescriptor>
>;

class CmdHandler::Impl {
public:
  Impl();

  void execute(Cmd const&);  
  std::vector<tea::Msg> poll();
private:
  std::map<Cmd,CmdExecutor> m_running_commands{};
}; // CmdHandler::Impl

CmdHandler::Impl::Impl() {}

void CmdHandler::Impl::execute(Cmd const& cmd) {
  // try_emplace = emplace if not exist, otherwise leave existing instance as-is
  auto [iter,inserted] = this->m_running_commands.try_emplace(
     cmd
    ,std::visit(
      [](auto const& concrete_cmd){
        using ConcreteCmd = std::decay_t<decltype(concrete_cmd)>;
        return detail::Executor<ConcreteCmd>();
      }
      ,cmd
    )
  );

  // TODO: Consider to log the value of inserted to detect multiple request for the same command

  // Dispatch to executor to start
  // Note: It is possible for client to request the same cmd twice or more.
  //       It is up to the executor to decide if and how to handle this.
  //       It may produce an appropriate message on poll().
  std::visit(
    [](auto& concrete_executor){
      return concrete_executor.start();
    }
    ,iter->second
  );
} // CmdHandler::Impl::execute

std::vector<tea::Msg> CmdHandler::Impl::poll() {
  return {};
} // CmdHandler::Impl::poll

CmdHandler::CmdHandler() 
  : m_pimpl{std::make_unique<Impl>()} {}

// Define destcrutor here after having fully defined CmdHandler::Impl
CmdHandler::~CmdHandler() = default;

void CmdHandler::execute(Cmd const& cmd) {
  return this->m_pimpl->execute(cmd);
}

std::vector<tea::Msg> CmdHandler::poll() {
  return this->m_pimpl->poll();
}
