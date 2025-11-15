# PSP Hello World - Quick Start Guide

## 🎉 SUCCESS! Toolchain Built Locally!

Good news - the PSP GCC toolchain **successfully built** on your macOS Monterey system!

## Current Status:
- ✅ PSP GCC compiler: **WORKING** (v15.1.1)
- ⏳ PSPSDK libraries: **In Progress** (needs completion)
- 🎯 Solution: Use GitHub Codespaces for full environment

## ✅ Option 1: GitHub Codespaces (Recommended - Ready Now!)

1. **Initialize Git** (run in your terminal):
   ```bash
   cd /Users/harriahola/Projects/PSP/helloworld
   git init
   git add .
   git commit -m "PSP Hello World project"
   ```

2. **Create GitHub repository:**
   - Go to https://github.com/new
   - Name it: `psp-helloworld`
   - Don't initialize with README (we already have files)
   - Click "Create repository"

3. **Push your code:**
   ```bash
   git remote add origin https://github.com/YOUR_USERNAME/psp-helloworld.git
   git branch -M main
   git push -u origin main
   ```

4. **Open in Codespaces:**
   - Go to your repo: `https://github.com/YOUR_USERNAME/psp-helloworld`
   - Click green **Code** button
   - Click **Codespaces** tab
   - Click **Create codespace on main**
   - Wait 3-5 minutes for setup (it builds PSP toolchain automatically!)

5. **Build your app** (in Codespaces terminal):
   ```bash
   make
   ```

6. **Download EBOOT.PBP:**
   - Right-click `EBOOT.PBP` in file explorer
   - Select "Download"
   - Test on PSP or PPSSPP!

## 🎮 Running Your App

### On Real PSP:
1. Connect PSP memory stick to Mac
2. Create folder: `PSP/GAME/HelloWorld/`
3. Copy `EBOOT.PBP` into that folder
4. Eject, go to Game → Memory Stick on PSP
5. Run "Hello World"!

### On PPSSPP Emulator:
```bash
brew install --cask ppsspp
# Open PPSSPP and load EBOOT.PBP
```

## 🔧 Alternative: Try Native Build

If you want to try building locally (may fail):

```bash
./install_native_minimal.sh
```

If it works:
```bash
make
```

## 📚 What's Included

- `main.c` - Hello World source code
- `Makefile` - Build configuration
- `.devcontainer/` - Codespaces auto-setup
- `Dockerfile` - Container definition
- Various installation scripts

## 💡 Why Codespaces?

- ✅ **Free**: 60 hours/month on free tier
- ✅ **Fast**: Pre-configured environment
- ✅ **Works**: No compatibility issues
- ✅ **Easy**: Browser-based VS Code
- ✅ **No install**: Everything in the cloud

## 🆘 Need Help?

Check `README.md` for full documentation or open an issue on GitHub!

Happy PSP coding! 🎮✨
