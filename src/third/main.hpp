#pragma once

// cratchit's variant third: the cross-platform cratchit in src/third.
namespace third {

// Runs cratchit third, as main() of a program of its own: argv[0] is the
// program and argv[1..argc-1] its arguments (e.g. --sie-to-ar ..., or none for
// the user interface). The program cratchit calls it with --third removed.
int main(int argc, char* argv[]);

} // namespace third
