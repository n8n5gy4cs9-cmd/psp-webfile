#!/bin/zsh

echo "Starting VS Code Git Editor Setup..."
echo "-------------------------------------"

# 1. Define the directory where the 'code' executable is located
VSCODE_BIN_DIR="/Applications/Visual Studio Code.app/Contents/Resources/app/bin"

# 2. Check if the directory exists
if [ -d "$VSCODE_BIN_DIR" ]; then
    echo "VS Code binary path found: $VSCODE_BIN_DIR"
    
    # 3. Add VS Code binary path to Zsh environment if not already present
    if ! grep -q "$VSCODE_BIN_DIR" ~/.zshrc; then
        echo "Adding VS Code binary path to ~/.zshrc..."
        echo '' >> ~/.zshrc
        echo '# Set VS Code binary path for Git operations' >> ~/.zshrc
        echo "export PATH=\$PATH:\"$VSCODE_BIN_DIR\"" >> ~/.zshrc
        echo "Successfully updated ~/.zshrc."
    else
        echo "VS Code binary path already in ~/.zshrc. Skipping PATH update."
    fi

    # 4. Set VS Code as the global Git editor
    # The --wait flag is crucial for Git to wait for VS Code to close the file.
    if ! git config --global core.editor | grep -q 'code --wait'; then
        echo "Setting 'code --wait' as the global Git editor..."
        git config --global core.editor "code --wait"
        echo "Successfully set Git core.editor to VS Code."
    else
        echo "Git core.editor is already set to 'code --wait'. Skipping."
    fi

    # 5. Source the file to apply changes immediately
    source ~/.zshrc
    echo "Configuration applied! Try your 'git rebase -i' command now."
else
    echo "ERROR: Could not find VS Code binary at $VSCODE_BIN_DIR."
    echo "Please ensure 'Visual Studio Code.app' is located in your /Applications folder."
fi

echo "-------------------------------------"