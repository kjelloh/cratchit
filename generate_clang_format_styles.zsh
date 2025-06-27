#!/bin/zsh

# List of clang-format built-in styles
styles=("llvm" "Google" "Chromium" "Mozilla" "WebKit" "Microsoft" "GNU")

# Base output directory
base_dir="clang-format-style"

# Create base directory
mkdir -p "$base_dir"

# Loop through each style and generate the config
for style in "${styles[@]}"; do
  echo "Generating config for style: $style"
  style_dir="$base_dir/$style"
  mkdir -p "$style_dir"
  clang-format -style=$style -dump-config > "$style_dir/.clang-format"
done

echo "Done! All styles are saved under '$base_dir/'."