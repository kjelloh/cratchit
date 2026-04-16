# Consider to show help with cents rounding by showing diff-to-balance in journal entry edit?

When I edit a template for a journal entry that posts a 'cents rounding' (BAS 3740) I have to manually calculate what is missing to go to a 3740 posting.

* Consider to always show the unbalanced diff in 'Edit journal entry'

* It seems to be in state ATIndex?
* But the prompting happens already in 

```c++
  case PromptState::JEAggregateOptionIndex: {
    // ":had:je:1or*";
    if (auto had_iter = model->selected_had()) {
      auto& had = *(*had_iter);
      if (had.optional.current_candidate) {
      // ...
      switch (ix) {

        case 2: {
          // Allow the user to edit individual account postings
          if (true) {
            // 20240526 - new 'enter state' prompt mechanism
            auto new_state = PromptState::ATIndex;
            prompt << model->to_prompt_for_entering_state(new_state);
            model->prompt_state = new_state;
          }
        // ...
```

* And then the magic should happen in:

```c++
  std::string to_prompt_for_entering_state(PromptState const& new_state) {
    std::ostringstream result{"\n<>"};
    std::cout << "\nto_prompt_for_entering_state";
    auto had_iter = this->selected_had();
    if (had_iter and new_state == PromptState::ATIndex) {
      auto& had = *(*had_iter);
      unsigned int i{};
      std::for_each(
         had.optional.current_candidate->defacto.account_postings.begin()
        ,had.optional.current_candidate->defacto.account_postings.end()
        ,[&i,&result](auto const& ap){
        result << "\n  " << i++ << " " << ap;
      });
    }
    else {
      result << options_list_of_prompt_state(new_state);
    }

    return result.str();
  }

```

DARN! The code is convoluted with a lambda so accumulation is not straight forward?