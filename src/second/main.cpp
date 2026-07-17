#include "main.hpp"
#include "log.hpp"
#include "RawTerminal.hpp"
#include <print>
#include <iostream>

namespace second {
  int main(int argc, char *argv[]) {
    log_business("second::cratchit START");
    log_development_trace("Hello from second::main");
    log_design_insufficiency("This is a test");

    log_business("Test with formatting int value {}",2);
    log_development_trace("Test with formatting int value {}",3);
    log_design_insufficiency("Test with formatting int value {}",4);

    size_t console_window_line_height = 10;
    size_t consolde_window_column_width = 40;
    std::vector<std::string> console_grid(console_window_line_height,std::string(consolde_window_column_width,' '));
    bool loop_again(true);
    std::string in_buffer{};
    RawTerminal raw_terminal{};
    while (loop_again) {
      // Clear the console grid
      for (size_t r=0;r<console_grid.size();++r) console_grid[r].assign(consolde_window_column_width, ' ');
      // Read console key press
      auto ch = raw_terminal.wait_for_char(); // Blocking read
      unsigned char byte = static_cast<unsigned char>(ch);
      in_buffer += std::format("[{:X}]",byte);

      // Render the console window
      std::print("\033[H"); // move to 'home'
      for (size_t r=0;r<console_grid.size();++r) {
        for (size_t c=0;c<console_grid[0].size();++c) {
          if (r==0 or r==console_grid.size()-1) console_grid[r][c] = static_cast<char>('0' + c%10);
          else if (c==0 or c==console_grid[0].size()-1) console_grid[r][c] = static_cast<char>('0' + r%10);

          // render the input buffer
          if (r==1 and c>0 and c<console_grid[0].size()-1 and (c-1) < in_buffer.size()) {
            if (c>console_grid[0].size()-5) {
              for (size_t j=0;j<3;++j) console_grid[r][c+j] = '.'; // mark truncagtion with '.'s
            }
            else console_grid[r][c] = in_buffer[c-1];
          }
        }
      }
      
      // render the console
      for (size_t r=0;r<console_window_line_height;++r) {
        std::print("{}\n",console_grid[r]);
      }

    }
    return 0;
  }
} // second