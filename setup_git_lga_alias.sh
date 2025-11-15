#!/bin/zsh

echo "Starting Git Log Alias (git lga) Setup..."
echo "-------------------------------------------"

# Define the alias command with datetime first
GIT_LGA_ALIAS='log --all --graph --pretty=format:"%C(cyan)[%ci]%Creset %Cgreen(%cr)%Creset%C(yellow)%d%Creset%n%Cred%H%Creset %C(bold blue)<%an>%Creset%n%w(0,0,5) %s"'

# 1. Set the global Git alias
echo "Setting global Git alias 'lga'..."
git config --global alias.lga "$GIT_LGA_ALIAS"

# 2. Add an initialization function to Zsh to add 'air' (padding)
# This function wraps the original 'git log' command to run 'echo' first.
if ! grep -q 'git_lga_wrapper' ~/.zshrc; then
    echo "Adding git lga wrapper function to ~/.zshrc for initial padding..."
    
    # Define the Zsh function to wrap the alias and add newlines
    LGA_FUNCTION=$(cat <<EOF
    # Modern Git Log with Padding
    git() {
      if [[ "\$1" == "lga" ]]; then
        echo ''
        echo ''
        command git "\$@"
      else
        command git "\$@"
      fi
    }
EOF
)
    # Append the function to the .zshrc file
    echo "\n# Enhanced Git Log Function (lga)" >> ~/.zshrc
    echo "$LGA_FUNCTION" >> ~/.zshrc
    echo "Successfully updated ~/.zshrc with the wrapper function."
else
    echo "Git lga wrapper function already found in ~/.zshrc. Skipping function update."
fi

# 3. Source the file to apply changes immediately
source ~/.zshrc
echo "Configuration applied! You can now use: git lga"

echo "-------------------------------------------"