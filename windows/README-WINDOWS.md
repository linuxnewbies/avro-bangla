# Avro Bangla for Windows - Easy Installation Guide

## 🚀 Quick Start (No Command Prompt Needed!)

We've made installation super easy! Just double-click the installer and you're done.

### Option 1: Use the Installer (Recommended) ⭐

1. **Download** `avro-bangla-installer.exe`
2. **Double-click** to run it
3. Click **"Install"**
4. Done! ✅

The installer will:
- Copy files automatically
- Register the keyboard with Windows
- Add "Avro Bangla" to your keyboard list

### Option 2: Manual Installation (Advanced)

If you prefer manual control or the installer doesn't work:

#### Step 1: Get the DLL
Copy `avro-bangla.dll` to a permanent location:
```
C:\Program Files\Avro Bangla\avro-bangla.dll
```

#### Step 2: Create Installation Script
Create a file named `install.bat` with this content:

```batch
@echo off
echo Installing Avro Bangla Keyboard...
copy avro-bangla.dll "C:\Program Files\Avro Bangla\"
reg add "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\IMM\ImeFile" /v "Avro Bangla" /t REG_SZ /d "C:\Program Files\Avro Bangla\avro-bangla.dll" /f
echo Installation complete! Please restart your computer.
pause
```

#### Step 3: Run as Administrator
- Right-click `install.bat`
- Select **"Run as administrator"**
- Restart your computer

---

## Using Avro Bangla

After installation:

1. Press **Win + Space** to switch keyboards
2. Select **"Avro Bangla"** from the list
3. Start typing phonetically!

### Examples:
```
ami banglay likhi    →  আমি বাংলায় লিখি
bhalo                →  ভালো
dhonnobad            →  ধন্যবাদ
```

---

## Uninstallation

### Option 1: Use Uninstaller
Run `avro-bangla-uninstaller.exe` (included in download)

### Option 2: Manual Removal
Create `uninstall.bat`:

```batch
@echo off
echo Uninstalling Avro Bangla Keyboard...
reg delete "HKLM\SOFTWARE\Microsoft\Windows\CurrentVersion\IMM\ImeFile" /v "Avro Bangla" /f
del "C:\Program Files\Avro Bangla\avro-bangla.dll"
rmdir "C:\Program Files\Avro Bangla"
echo Uninstallation complete!
pause
```

Run as administrator, then restart.

---

## Troubleshooting

### Keyboard doesn't appear in Win+Space list
- Restart your computer after installation
- Check if DLL is registered: Open Registry Editor (`regedit`)
  - Navigate to: `HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows\CurrentVersion\IMM\ImeFile`
  - Verify "Avro Bangla" entry exists

### Typing doesn't convert to Bengali
- Make sure "Avro Bangla" is selected (check taskbar)
- Try pressing Space after typing a word
- Switch to another keyboard and back

### Works in some apps but not others
- Some modern UWP apps have limited IME support
- Works best in: Notepad, Word, Chrome, Firefox, etc.

---

## Building from Source (Optional)

Only needed if you want to compile yourself.

### On Windows with MinGW:
```bash
cd windows
gcc -shared -o avro-bangla.dll main.c avro_engine.c -limm32 -lole32 -luuid
```

### Cross-compile from FreeBSD:
```bash
pkg install mingw-w64-gcc
cd windows
make windows
```

---

## Support

For issues or questions:
- Check the main README.md in the project root
- Report bugs on GitHub
- Community forums

**Enjoy typing in Bangla!** 🎉
