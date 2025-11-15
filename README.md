# PSP Hello World Homebrew

This is a simple Hello World application for Sony PSP homebrew development.

## Installation

### ⚠️ Important for macOS Monterey (2015 MacBook) Users

Due to compatibility issues with macOS Monterey, Xcode 13, and the inability to install Docker Desktop, there are **three options**:

### Option 1: GitHub Codespaces (Recommended ✅ FREE!)

The **easiest and most reliable** solution - develop in the cloud:

1. **Set up Git repository:**
   ```bash
   cd /Users/harriahola/Projects/PSP/helloworld
   git init
   git add .
   git commit -m "Initial PSP Hello World"
   ```

2. **Push to GitHub** (create a new repository on github.com first):
   ```bash
   git remote add origin https://github.com/YOUR_USERNAME/psp-helloworld.git
   git push -u origin main
   ```

3. **Open GitHub Codespaces:**
   - Go to your repository on github.com
   - Click **Code** → **Codespaces** → **Create codespace on main**
   - The environment will automatically set up with PSP toolchain!

4. **Build in Codespaces:**
   ```bash
   make
   ```

5. **Download EBOOT.PBP** to your Mac and test!

**Why Codespaces?**
- ✅ Free 60 hours/month
- ✅ Full Ubuntu Linux environment
- ✅ No local compatibility issues
- ✅ PSP toolchain pre-configured (using `.devcontainer/`)

### Option 2: Native Build (Try It)

Attempt to build with system compiler:

```bash
chmod +x install_native_minimal.sh
./install_native_minimal.sh
```

**Warning:** This may fail due to GCC compatibility issues on macOS Monterey.

### Option 3: Virtual Machine

Install VirtualBox and Ubuntu:
1. Install VirtualBox: `brew install --cask virtualbox`
2. Download Ubuntu 22.04 LTS
3. Follow standard PSP toolchain installation in Ubuntu

## Building the Hello World Application

### If using Docker (Recommended):

```bash
~/psp-make
```

Or with the full Docker command:
```bash
docker run --rm -v "$PWD:/build" -w /build pspdev/pspdev:latest make
```

### If using Native Installation:

Once the toolchain is installed, build the application:

```bash
make
```

Both methods will create an `EBOOT.PBP` file, which is the executable format for PSP.

## Running on PSP

### On a Real PSP:

1. Copy the `EBOOT.PBP` file to your PSP's memory stick:
   - Create a folder structure: `PSP/GAME/HelloWorld/`
   - Place `EBOOT.PBP` inside the `HelloWorld` folder

2. On your PSP:
   - Navigate to Game → Memory Stick
   - You should see "Hello World"
   - Press X to launch it

### Using PPSSPP Emulator:

1. Install PPSSPP:
   ```bash
   brew install --cask ppsspp
   ```

2. Open PPSSPP and load the `EBOOT.PBP` file

## Controls

- **X Button**: Exit the application

## Cleaning Build Files

To clean up compiled files:

```bash
make clean
```

## Project Structure

- `main.c` - Main source code with Hello World implementation
- `Makefile` - Build configuration for PSP
- `install_psp_toolchain.sh` - Installation script for PSP development tools
- `README.md` - This file

## Troubleshooting

### "psp-gcc: command not found"

Make sure the environment variables are set. Run:
```bash
source ~/.zshrc
```

Or check that these lines are in your `~/.zshrc`:
```bash
export PSPDEV="$HOME/pspdev"
export PSPSDK="$PSPDEV/psp/sdk"
export PATH="$PATH:$PSPDEV/bin:$PSPSDK/bin"
```

### Build Errors

Make sure all dependencies are installed:
```bash
brew install wget cmake autoconf automake libtool gcc make patch doxygen
```

## Next Steps

Now that you have a working PSP development environment, you can:
- Modify `main.c` to add more features
- Learn about PSP graphics with the `pspgu` library
- Add 3D graphics using the GU (Graphics Utility) library
- Implement game logic, audio, and more!

## Resources

- [PSP Development Wiki](https://www.ps2dev.org/psp/)
- [PSPSDK Documentation](https://github.com/pspdev/pspsdk)
- [PSP Homebrew Community](https://forums.ps2dev.org/)

Happy coding! 🎮
