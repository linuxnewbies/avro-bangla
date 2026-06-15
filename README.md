# Avro Bangla - Cross-Platform Phonetic Input Method

A native, system-wide Bangla phonetic input method for **FreeBSD**, **GhostBSD**, **NomadBSD**, **Windows 10/11**, and other operating systems. This engine replicates the exact behavior of the popular Avro Phonetic keyboard (formerly by OmicronLab) using modern, efficient code.

Type naturally in Latin characters (e.g., `ami banglay likhi`) and get beautiful Bengali text (`আমি বাংলায় লিখি`) in any application.

## 🌍 Platform Support

| Platform | Status | Build Type | Installation |
|----------|--------|------------|--------------|
| **FreeBSD / GhostBSD / NomadBSD** | ✅ Stable | Go + IBus | Binary + XML |
| **Windows 10/11** | ✅ Stable | C + IME | DLL Registration |
| **Linux (with IBus)** | 🟡 Untested | Go + IBus | Similar to FreeBSD |

## ✨ Features

- **100% Compatible with Avro Rules:** Uses the same logic as the original Avro Phonetic engine.
- **System-Wide:** Works in all applications on your operating system.
- **Native Performance:** 
  - FreeBSD: Written in pure Go with IBus framework
  - Windows: Pure C with Windows IME API
- **Lightweight:** Minimal memory footprint.
- **Easy Installation:** Simple build processes for each platform.
- **Cross-Platform Development:** Same conversion logic across all platforms.

---

## 📖 Table of Contents

1. [FreeBSD / GhostBSD / NomadBSD Installation](#freebsd--ghostbsd--nomadbsd-installation)
2. [Windows 10/11 Installation](#windows-1011-installation)
3. [Troubleshooting](#troubleshooting)
4. [Project Structure](#project-structure)
5. [Contributing](#contributing)

---

## FreeBSD / GhostBSD / NomadBSD Installation

### Prerequisites

Install dependencies using `pkg`:

```bash
sudo pkg install -y go ibus dbus-glib git pkgconf
```

### Build & Install

```bash
cd /workspace
make build
sudo make install
```

### Configure IBus

1. Start IBus daemon (if not running):
   ```bash
   ibus-daemon -drx
   ```

2. Add the engine:
   - Run `ibus-setup`
   - Click **Add** → Find **Bengali** → Select **Avro Bangla Phonetic**

3. Switch to Bangla:
   - Press `Super + Space` (Win + Space)
   - Or click keyboard icon in system tray

### Usage Examples

```
ami banglay likhi    →  আমি বাংলায় লিখি
goner choto          →  গানের ছোট
bhalo achi           →  ভালো আছি
```

### Uninstall (FreeBSD)

```bash
sudo make uninstall
ibus exit
ibus-daemon -drx
```

---

## Windows 10/11 Installation

### 🚀 Easy Installation (No Command Prompt!)

**We now provide a one-click installer!**

1. **Download** `avro-bangla-installer.exe` from releases
2. **Double-click** to run it
3. Click **"Install"**
4. Done! ✅

The installer automatically:
- Copies files to the correct location
- Registers the keyboard with Windows
- Makes "Avro Bangla" available in Win+Space

### Manual Installation (Advanced)

If you prefer manual control or want to build from source:

#### Build the DLL

**From FreeBSD (Cross-compile):**
```bash
cd windows
make windows
```

**From Windows (MinGW):**
```bash
cd windows
gcc -shared -o avro-bangla.dll main.c avro_engine.c -limm32 -lole32 -luuid
```

#### Install Using Batch Script

Create `install.bat`:
```batch
@echo off
copy avro-bangla.dll "C:\Program Files\Avro Bangla\"
reg add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\IMM\ImeFile" /v "Avro Bangla" /t REG_SZ /d "C:\Program Files\Avro Bangla\avro-bangla.dll" /f
echo Installation complete! Restart your computer.
pause
```

Run as Administrator, then restart.

### Usage

1. Press **Win + Space** to switch keyboards
2. Select **Avro Bangla**
3. Type phonetically: `ami banglay likhi` → `আমি বাংলায় লিখি`

See [`windows/README-WINDOWS.md`](windows/README-WINDOWS.md) for detailed instructions.

### Uninstall

**Easy way:** Run `avro-bangla-uninstaller.exe`

**Manual way:** Create and run `uninstall.bat` as Administrator:
```batch
@echo off
reg delete "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\IMM\ImeFile" /v "Avro Bangla" /f
del "C:\Program Files\Avro Bangla\avro-bangla.dll"
echo Uninstallation complete!
pause
```

---

## Troubleshooting

### FreeBSD/GhostBSD Issues

#### IBus doesn't show "Avro Bangla"
1. Verify XML file exists: `ls /usr/local/share/ibus/component/org.avro.bangla.xml`
2. Restart IBus: `ibus exit && ibus-daemon -drx`
3. Log out and log back in

#### Typing produces Latin instead of Bangla
- Ensure Avro Bangla is selected (check system tray)
- Use `Super + Space` to cycle input methods

#### Build fails with missing headers
```bash
sudo pkg install pkgconf
make clean && make build
```

### Windows Issues

#### IME doesn't appear in keyboard list
- Ensure DLL is 64-bit for 64-bit Windows
- Check registration: `reg query "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\IMM\ImeFile"`
- Restart Windows Explorer or reboot

#### Typing doesn't convert
- Verify Avro Bangla is active (check taskbar language indicator)
- Press Space after typing to commit
- Try switching to another keyboard and back

#### Application compatibility
- Some UWP apps have limited IME support
- Test in traditional Win32 apps first (Notepad, Word, Chrome)

---

## Project Structure

```
avro-bangla-ibus/
├── README.md                 # This file
├── LICENSE                   # MIT License
├── Makefile                  # FreeBSD build & install
├── go.mod                    # Go module definition
├── main.go                   # IBus engine (FreeBSD)
├── avro.go                   # Core Avro parsing logic
├── avro_rules.go             # Complete Avro rule set
├── org.avro.bangla.xml       # IBus component (FreeBSD)
└── windows/                  # Windows IME implementation
    ├── README-WINDOWS.md     # Detailed Windows guide
    ├── Makefile              # Windows build script
    ├── main.c                # Windows IME entry point
    └── avro_engine.c         # Cross-platform Avro logic (C)
```

---

## Contributing

Contributions are welcome! Areas for improvement:

- **FreeBSD:** Better TSF integration, more test coverage
- **Windows:** Migrate to Text Services Framework (TSF) for better Windows 10/11 support
- **Both:** Additional Avro features, emoji support, custom dictionaries

Please submit issues or pull requests on GitHub.

---

## 📄 License

MIT License - See LICENSE file for details.

## 🙏 Acknowledgments

- Original Avro Phonetic by OmicronLab
- IBus Framework developers
- Windows IME/TSF documentation
- FreeBSD community

---

*Built with ❤️ for BSD and Windows users worldwide.*
