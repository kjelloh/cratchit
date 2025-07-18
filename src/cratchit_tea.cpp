#include "cratchit_tea.hpp"
#include "states/StateImpl.hpp"
#include "states/FrameworkState.hpp"
#include "to_type_name.hpp"
#include "sub.hpp"
#include <filesystem>
#include <ranges>

namespace first {

  // Factory function for framework state
  auto framework_state_factory = []() {
    std::filesystem::path root_path = std::filesystem::current_path();
    return make_state<FrameworkState>("Framework",root_path);
  };

  bool is_quit_msg(Msg const& msg) {
    return msg == QUIT_MSG;
  }

  InitResult init() {
    Model model{}; // Default initialization

    auto new_framework_state_cmd = []() -> Msg {
      auto msg = std::make_shared<PushStateMsg>(framework_state_factory());
      return msg;
    };
    return {model,is_quit_msg,new_framework_state_cmd};
  }

  ModelUpdateResult update(Model model, Msg msg) {
    Cmd cmd = Nop;

    if (model.ui_states.size()>0 and model.ui_states.back() == nullptr) {
      spdlog::error("cratchit::update: DESIGN_INSUFFICIENCY - nullptr on stack!");
      model.ui_states.pop_back();
      spdlog::error("cratchit::update: nullptr popped");
      return {model,Nop};
    }

    auto try_state_update = [&model](Msg const& msg) -> StateUpdateResult {
      bool ask_state_to_update = (model.ui_states.size()>0) and (model.user_input_state.buffer().size()==0);
      if (ask_state_to_update) {
        spdlog::info("cratchit::update:::try_state_update: Passing Msg to state");
        if (auto update_result = model.ui_states.back()->update(msg)) {
          spdlog::info("cratchit::update:::try_state_update: State returned update_result = handled by state");
          return update_result;
        }
        else if (auto key_msg_ptr = std::dynamic_pointer_cast<NCursesKeyMsg>(msg); key_msg_ptr != nullptr) {
          spdlog::info("cratchit::update:::try_state_update: - NCursesKeyMsg");
          auto ch = key_msg_ptr->key;
          if (auto update_result = model.ui_states.back()->update_options().apply(ch)) {
            return update_result;
          }
          else if (ch == 'q') {
            if (model.ui_states.size() <= 1) {
              return {std::nullopt,DO_QUIT};
            }
          }
          else if (ch == '-') {
            // Default pop-state (no payload child -> parent state)
            // 250714 - Only required for 'dummy' state pushed as StateImpl instance (prototyping)
            return {std::nullopt,[]() -> std::optional<Msg>{
              return std::make_shared<PopStateMsg>();
            }};
          }
          spdlog::info("try_state_update: NCursesKeyMsg -  No handler for key {}",ch);
        }
      }
      spdlog::info("cratchit::update:::try_state_update: No handling found for this message");
      return {std::nullopt,Cmd{}};
    };

    if (auto update_result = try_state_update(msg)) {
      update_result.apply(model.ui_states.back(),cmd);

      // Logging
      if (true) {
        if (update_result.maybe_state) {
          auto& ref = *update_result.maybe_state;
          spdlog::info("cratchit::update:  state::update -> state:{}[{}] <- use_count: {}"
            ,to_type_name(typeid(ref))
            ,static_cast<void*>(update_result.maybe_state->get())
            ,update_result.maybe_state->use_count());
        }
        if (update_result.maybe_null_cmd) {
          spdlog::info("cratchit::update:  state::update -> cmd");
        }
      }
    }
    else if (auto update_result = model.user_input_state.update(msg)) {
      // user input state handled the message - apply the changes
      update_result.apply(model.user_input_state,cmd);
    }
    else if (auto pimpl = std::dynamic_pointer_cast<PushStateMsg>(msg); pimpl != nullptr) {
      if (pimpl->m_state != nullptr) {
        model.ui_states.push_back(pimpl->m_state);
      }
      else {
        spdlog::error("cratchit::update: DESIGN_INSUFFICIENCY - PushStateMsg carried new_state == nullptr");
      }
    }
    else if (auto pimpl = std::dynamic_pointer_cast<PopStateMsg>(msg); pimpl != nullptr) {
      if (model.ui_states.size() > 0) {
        spdlog::info("cratchit::update:  Popping top state with use_count: {}", model.ui_states.back().use_count());
        auto popped_state = model.ui_states.back();
        model.ui_states.pop_back();

        auto const& ref = *popped_state.get();
        spdlog::info("cratchit::update:  Popped {}[{}]", to_type_name(typeid(ref)), static_cast<void*>(popped_state.get()));

        if (auto maybe_null_cargo_msg = pimpl->m_maybe_null_cargo_msg) {
          auto const& ref = *maybe_null_cargo_msg;
          spdlog::info("cratchit::update: PopStateMsg provided maybe_null_cargo_msg {}", to_type_name(typeid(ref)));

          cmd = [maybe_null_cargo_msg]() {
            return maybe_null_cargo_msg;
          };
        }
      }
    }
    else {
      // Trace unhandled messages
      auto const& ref = *msg;
      spdlog::info("cratchit::update:  Unhandled message - type: {}", to_type_name(typeid(ref)));
    }

    // Dump stack to log
    if (true) {
      spdlog::info("cratchit::update:  State stack size:{}",model.ui_states.size());

      auto enumerated_view = [](auto && range) {
        return std::views::zip(
          std::views::iota(0)
          ,std::forward<decltype(range)>(range));
      };

      for (auto const& [index,m] :  enumerated_view(model.ui_states)) {
        if (m != nullptr) {
          auto& ref = *m; // Silence typeid(*m) warning about side effects
          spdlog::info("   {}: {}[{}] <- use_count: {}"
            ,index
            ,to_type_name(typeid(ref))
            ,static_cast<void*>(m.get())
            ,m.use_count());
        }
        else {
          spdlog::info("   {}: {}[{}] <- use_count: {}"
            ,index
            ,"NULL"
            ,static_cast<void*>(nullptr)
            ,-1);
        }
      }
    }

    // If no stack after any Msg, then tell runtime to quit
    if (model.ui_states.size() == 0) {
      return {model,DO_QUIT};
    }

    return {model,cmd}; // Return updated model
  }

  ViewResult view(const Model &model) {

    ViewResult ui{};
    auto& doc = ui.doc;

    // Note: HTML doc may be tested for validity at:
    // https://www.w3schools.com/html/tryit.asp?filename=tryhtml_intro

    // Create the root HTML element
    pugi::xml_node html = doc.append_child("html");

    // Create the body
    pugi::xml_node body = html.append_child("body");

    // Generate UI content from current state (TEA-style)
    std::string top_content;
    std::string main_content;
        
    if (model.ui_states.size() > 0 and model.ui_states.back() == nullptr) {
      spdlog::error("cratchit::view: DESIGN_INSUFFICIENCY - No view for nullptr on stack!");
    }
    else if (model.ui_states.size() > 0) {

      // StateImpl UX (top window)
      for (std::size_t i=0;i<model.ui_states.back()->ux().size();++i) {
        if (i>0) top_content.push_back('\n');
        top_content += model.ui_states.back()->ux()[i];
      }
      

      for (auto const& [ch, update_option] : model.ui_states.back()->update_options().view()) {
        std::string entry{};
        entry.push_back(ch);
        entry.append(" = ");
        entry.append(update_option.first);
        entry.push_back('\n');
        main_content.append(entry);
      }
    }

    // Generate stack breadcrumb
    std::string breadcrumb_content;
    if (model.ui_states.size() > 0) {
      for (std::size_t i = 0; i < model.ui_states.size(); ++i) {
        if (i > 0) breadcrumb_content.append(" -> ");
        breadcrumb_content.append(model.ui_states[i]->caption());
      }
    }

    // Create the top section with class "content"
    pugi::xml_node top = body.append_child("div");
    top.append_attribute("class") = "content";
    top.text().set(top_content.c_str());

    // Create the main section with class "content"
    pugi::xml_node main = body.append_child("div");
    main.append_attribute("class") = "content";
    main.text().set(main_content.c_str());

    // Create the user prompt section with class "user-prompt"
    pugi::xml_node prompt = body.append_child("div");
    prompt.append_attribute("class") = "user-prompt";
    
    // Add breadcrumb as first line
    pugi::xml_node breadcrumb = prompt.append_child("div");
    breadcrumb.append_attribute("class") = "breadcrumb";
    breadcrumb.text().set(breadcrumb_content.c_str());
    
    // Add empty line (spacer)
    pugi::xml_node spacer = prompt.append_child("div");
    spacer.append_attribute("class") = "spacer";
    spacer.text().set("");
    
    // Add a label element for the prompt text
    pugi::xml_node label = prompt.append_child("label");
    std::string input_text = model.user_input_state.buffer();
    label.text().set((">" + input_text).c_str());

    // Make prompt 'html-correct' (even though render does not care for now)
    pugi::xml_node input = prompt.append_child("input");
    input.append_attribute("type") = "text";
    input.append_attribute("id") = "command";
    input.append_attribute("name") = "command";

    ui.event_handlers["OnKey"] = onKey;
    return ui;
  }

} // namespace first