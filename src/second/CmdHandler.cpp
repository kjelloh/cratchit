#include "CmdHandler.hpp"
#include "cmd_to_msg.hpp" // client provided binding descriptor -> result_type -> Msg
#include "log.hpp"
#include "cmd_to_string.hpp"
#include "msg_to_string.hpp"

#include <map>
#include <optional>
#include <chrono>

using namespace std::chrono_literals; // ms,

namespace detail {

  struct ConcretePollResult {
    enum class Type {
       Unknown
      ,Nop
      ,ProgressReport
      ,Done
      ,Undefined
    }; // Type
    std::optional<tea::Msg> maybe_msg;
    Type type;
  }; // ConcretePollResult

  template <typename ConcreteCmd>
  class Executor {
  public:
  private:
  }; // Executor<>

  template <>
  class Executor<TestCmdDescriptor> {
  public:
    Executor(TestCmdDescriptor const& descriptor)
      : m_descriptor{descriptor} {}

      void start() {
      ++m_activation_count;
      m_start_time = std::chrono::steady_clock::now();
    }

    ConcretePollResult poll() {
      auto current_time = std::chrono::steady_clock::now();
      auto elapsed_time = current_time - this->m_start_time;
      if (elapsed_time >= this->m_duration_time) {
        return {
          cmd_to_msg(this->m_descriptor,TestCmdDescriptor::payload_type{})
          ,ConcretePollResult::Type::Done
        };
      } // If done
      auto next_progress_time = m_start_time + (m_current_progress_ix+1)*m_duration_time / m_progress_counts;
      if (current_time >= next_progress_time) {
        ++m_current_progress_ix;
        return {
          cmd_to_msg(this->m_descriptor,TestCmdDescriptor::payload_type{m_current_progress_ix})
          ,ConcretePollResult::Type::ProgressReport
        };
      }
      return {
        std::nullopt
        ,ConcretePollResult::Type::Nop
      };

    } // poll
  private:
    TestCmdDescriptor const& m_descriptor;
    size_t m_activation_count{};

    std::chrono::steady_clock::time_point m_start_time{};
    std::chrono::steady_clock::duration m_duration_time{1000ms};
    uint8_t m_progress_counts{7};
    uint8_t m_current_progress_ix{};    
  }; // Executor<TestCmdDescriptor>


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
  log_development_trace(
    "CmdHandler::Impl::execute(cmd:{})"
    ,cmd_to_string(cmd)
  );
  // try_emplace = emplace if not exist, otherwise leave existing instance as-is
  auto [iter,inserted] = this->m_running_commands.try_emplace(
     cmd
    ,std::visit(
      [](auto const& concrete_cmd){
        using ConcreteCmd = std::decay_t<decltype(concrete_cmd)>;
        return detail::Executor<ConcreteCmd>(concrete_cmd);
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
  std::vector<tea::Msg> result{};

  std::vector<decltype(m_running_commands)::iterator> to_erase;

  for (
       auto iter = this->m_running_commands.begin()
      ;iter != this->m_running_commands.end()
      ;++iter) {
    auto& [cmd,executor] = *iter;
    auto poll_result = std::visit(
      [](auto& concrete_executor){
        return concrete_executor.poll();
      }
      ,executor
    );
    if (poll_result.maybe_msg) {

      log_development_trace(
        "CmdHandler::Impl::poll: cmd:{} -> (msg:{},type:{})"
        ,cmd_to_string(iter->first)
        ,msg_to_string(poll_result.maybe_msg.value())
        ,static_cast<int>(poll_result.type)
      );

      result.push_back(poll_result.maybe_msg.value());
    }


    if (poll_result.type == detail::ConcretePollResult::Type::Done) {
      to_erase.push_back(iter);
    }
  } // for
  for (auto iter : to_erase) {
    log_development_trace(
       "CmdHandler::Impl::poll: erased cmd:{}"
      ,cmd_to_string(iter->first)
    );
    this->m_running_commands.erase(iter);
  }
  return result;
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
