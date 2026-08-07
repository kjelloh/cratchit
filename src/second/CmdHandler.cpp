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

  enum class Status {
     Unknown
    ,InProgress
    ,Done
    ,Undefined
  }; // Type

  using MaybeMsg = std::optional<tea::Msg>;
  
  template <typename ConcreteCmd>
  class Executor {
  public:
  private:
  }; // Executor<>

  // #TEA::Cmd
  template <>
  class Executor<TestCmdDescriptor> {
  public:
    Executor(TestCmdDescriptor const& descriptor)
      : m_descriptor{descriptor} {}

      void start() {
      ++m_activation_count;
      m_start_time = std::chrono::steady_clock::now();
    }

    std::tuple<Status,MaybeMsg> poll() {

      auto current_time = std::chrono::steady_clock::now();

      auto elapsed_time = current_time - this->m_start_time;
      if (elapsed_time >= this->m_duration_time) {

        // #TEA::Cmd - Done
        return {
             Status::Done
            ,cmd_to_msg(
               this->m_descriptor
              ,TestCmdDescriptor::payload_type{
                 CmdResponseType::Done
                ,m_current_progress_ix
              })};

      } // If done

      auto next_progress_time = m_start_time + (m_current_progress_ix+1)*m_duration_time / m_progress_counts;
      if (current_time >= next_progress_time) {
        ++m_current_progress_ix;

        // #TEA::Cmd - Progress
        return {
           Status::InProgress
          ,cmd_to_msg(
             this->m_descriptor
            ,TestCmdDescriptor::payload_type{
               CmdResponseType::ProgressReport
              ,m_current_progress_ix
            }
          )
        };
      } // If progress

      return {Status::InProgress,std::nullopt};

    } // poll
  private:

    // #TEA::Cmd - Executor stores descriptor (for cmd _to_msg)
    TestCmdDescriptor const& m_descriptor;
    size_t m_activation_count{};

    std::chrono::steady_clock::time_point m_start_time{};
    std::chrono::steady_clock::duration m_duration_time{3000ms};
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

  std::vector<decltype(m_running_commands)::iterator> to_erase{};

  for (
       auto iter = this->m_running_commands.begin()
      ;iter != this->m_running_commands.end()
      ;++iter) {
    auto& [cmd,executor] = *iter;
    auto [status,maybe_msg] = std::visit(
      [](auto& concrete_executor){
        return concrete_executor.poll();
      }
      ,executor
    );

    if (maybe_msg) {

      log_development_trace(
        "CmdHandler::Impl::poll: cmd:{} -> (status:{},msg:{})"
        ,cmd_to_string(iter->first)
        ,static_cast<size_t>(status)
        ,msg_to_string(maybe_msg.value())
      );

      result.push_back(maybe_msg.value());
    }

    if (status == detail::Status::Done) {
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

// Note: Here CmdHandler::Impl is fully defined
//       We can define members that applies to Impl

CmdHandler::CmdHandler() 
  : m_pimpl{std::make_unique<Impl>()} {}

CmdHandler::~CmdHandler() = default;

void CmdHandler::execute(Cmd const& cmd) {
  return this->m_pimpl->execute(cmd);
}

std::vector<tea::Msg> CmdHandler::poll() {
  return this->m_pimpl->poll();
}
