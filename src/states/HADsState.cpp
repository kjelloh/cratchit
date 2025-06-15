#include "HADsState.hpp"
#include "HADState.hpp"
#include "tokenize.hpp"

namespace first {
  // ----------------------------------
  HADsState::HADsState(HADs all_hads,Mod10View mod10_view)
    :  m_mod10_view{mod10_view}
      ,m_all_hads{all_hads}
      ,StateImpl({}) {


    struct HADs_subrange_factory {
      // HAD subrange StateImpl factory
      HADsState::HADs m_all_hads{};
      Mod10View m_mod10_view;

      auto operator()() {
        return std::make_shared<HADsState>(m_all_hads, m_mod10_view);
      }

      HADs_subrange_factory(HADsState::HADs all_hads, Mod10View mod10_view)
          : m_mod10_view{mod10_view}, m_all_hads{all_hads} {}
    };

    auto subranges = m_mod10_view.subranges();
    for (size_t i=0;i<subranges.size();++i) {
      auto const subrange = subranges[i];
      auto const& [begin,end] = subrange;
      auto caption = std::to_string(begin);
      if (end-begin==1) {
        // Single HAD in range option
        this->add_option(static_cast<char>('0'+i),{caption,[had=m_all_hads[begin]](){
          // Single RBT factory
          auto HAD_ux = StateImpl::UX{
            "HAD UX goes here"
          };
          return std::make_shared<HADState>(had);
        }});
      }
      else {
        caption += " .. ";
        caption += std::to_string(end-1);
        this->add_option(static_cast<char>('0'+i),{caption,HADs_subrange_factory(m_all_hads,subrange)});
      }
    }

    // Initiate view UX
    for (size_t i=m_mod10_view.m_range.first;i<m_mod10_view.m_range.second;++i) {
      auto entry = std::to_string(i);
      entry += ". ";
      entry += to_string(m_all_hads[i]);
      this->ux().push_back(entry);
    }

  }

  // ----------------------------------
  HADsState::HADsState(HADs all_hads) : HADsState(all_hads,Mod10View(all_hads)) {}

  std::pair<std::optional<State>, Cmd> HADsState::update(Msg const &msg) {
      std::optional<State> state{};
      Cmd cmd = Nop;
      if (auto entry_msg_ptr = std::dynamic_pointer_cast<UserEntryMsg>(msg);entry_msg_ptr != nullptr) {
        std::string command(entry_msg_ptr->m_entry);
        auto tokens = tokenize::splits(command,tokenize::SplitOn::TextAmountAndDate);
        if (auto had = to_had(tokens)) {
          auto mutated_state = std::make_shared<HADsState>(*this); // clone
          mutated_state->m_all_hads.push_back(*had);
          state = mutated_state;
        }

      }
      return {state,cmd};
  }

} // namespace first