#!/bin/zsh

if [[ -z "$1" ]]; then
  echo "No style provided."
  echo "Usage: $0 <style>"
  echo "Available styles: google, chromium, mozilla, webkit, llvm"
  if [[ -f .clang-format ]]; then
    read "REPLY?Do you want to delete the existing .clang-format file? [y/N] "
    if [[ "$REPLY" =~ ^[Yy]$ ]]; then
      rm .clang-format
      echo ".clang-format deleted."
    else
      echo "Keeping existing .clang-format."
    fi
  else
    echo "No .clang-format file exists."
  fi
  exit 1
fi

cp "clang-format-style/$1/.clang-format" .clang-format
echo "Copied $1 style into .clang-format"
