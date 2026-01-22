#!/bin/zsh

# Exit on error
set -e

VENV_DIR="venv"
PYTHON="$VENV_DIR/bin/python"
PIP="$VENV_DIR/bin/pip"

echo "🔧 Setting up Python environment..."

# Create virtual environment if it doesn't exist
if [[ ! -d "$VENV_DIR" ]]; then
    echo "Creating virtual environment..."
    python3 -m venv "$VENV_DIR"
    echo "✓ Virtual environment created"
fi

# Install/upgrade watchdog
echo "Ensuring watchdog is installed..."
"$PIP" install --quiet --upgrade watchdog

echo "✓ Ready to go!"
echo ""

# Run the script using the venv's Python directly
# No need to activate/deactivate - just use the venv's Python binary
"$PYTHON" rinse_and_repeat.py

# When the Python script exits (Ctrl+C or error), this script exits
# and the venv is automatically "deactivated" (since we never activated it in the parent shell)
