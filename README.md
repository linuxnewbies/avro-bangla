# Avro Bangla IBus Engine for FreeBSD

A native, system-wide Bangla phonetic input method for **FreeBSD**, **GhostBSD**, **NomadBSD**, and other FreeBSD-based operating systems. This engine replicates the exact behavior of the popular Avro Phonetic keyboard (formerly by OmicronLab) using the Go programming language and the IBus framework.

Type naturally in Latin characters (e.g., `ami banglay likhi`) and get beautiful Bengali text (`আমি বাংলায় লিখি`) in any application.

## ✨ Features

- **100% Compatible with Avro Rules:** Uses the same logic as the original Avro Phonetic engine.
- **System-Wide:** Works in Firefox, LibreOffice, Terminal, VS Code, and every other GTK/Qt application.
- **Native Performance:** Written in pure Go, no heavy C dependencies or Java runtime required.
- **Lightweight:** Minimal memory footprint, perfect for desktop BSD environments.
- **Easy Installation:** Simple build process using standard FreeBSD tools.
- **FreeBSD Optimized:** Uses `/usr/local` paths consistent with FreeBSD hierarchy.

## 📋 Prerequisites

Before building, ensure you have the necessary development tools and IBus libraries installed.

### 1. Install Dependencies

Open a terminal and run the following command (requires `sudo`):

```bash
sudo pkg install -y go ibus dbus-glib git pkgconf
```

*   `go`: The Go compiler.
*   `ibus`: The Intelligent Input Bus framework.
*   `dbus-glib`: Required for IBus D-Bus communication.
*   `git`: To clone the repository (if applicable).
*   `pkgconf`: Helps locate library headers during compilation.

### 2. Verify IBus Installation

Ensure IBus is installed and available:

```bash
ibus --version
```

*(If this fails, log out and log back in, or restart your session after installing the packages.)*

### 3. Start IBus Daemon (if not running)

IBus must be running for the engine to work:

```bash
# Check if IBus is running
ps aux | grep ibus-daemon

# If not running, start it
ibus-daemon -drx
```

Add `ibus-daemon -drx` to your `~/.xinitrc` or desktop environment's autostart to launch IBus automatically on login.

## 🛠️ Building from Source

### 1. Clone or Download

If you haven't already, place the project files in a directory (e.g., `~/avro-bangla-ibus`).

```bash
cd ~/avro-bangla-ibus
```

### 2. Build the Binary

Navigate to the project directory and compile:

```bash
make build
```

This will generate the `avro-bangla-ibus` binary.

### 3. Install System-Wide

Install the binary and the IBus component XML file to the correct system locations:

```bash
sudo make install
```

*This copies the binary to `/usr/local/bin` and the XML config to `/usr/local/share/ibus/component`.*

### 4. Refresh IBus Cache

Tell IBus to recognize the new engine:

```bash
# Restart IBus daemon
ibus exit
ibus-daemon -drx
```

Alternatively, log out and log back in.

## ⚙️ Configuration & Usage

### Step 1: Add the Engine

1.  Open **IBus Preferences**:
    -   Run `ibus-setup` in your terminal.
    -   Or look for "IBus Preferences" in your Application Menu (usually under Settings or Input Methods).
2.  Click on the **Input Method** tab.
3.  Click the **Add** button (+).
4.  Scroll down to find **Bengali** (or search "Avro" or "Bangla").
5.  Select **Avro Bangla Phonetic** and click **Add**.

### Step 2: Typing

1.  Switch to the Bengali input method:
    -   Press `Super + Space` (Windows Key + Space).
    -   Or click the keyboard icon in your system tray and select **Bengali - Avro Bangla Phonetic**.
2.  Start typing!
    -   Type: `ami banglay gan gai`
    -   Output: `আমি বাংলায় গান গাই`

### Step 3: Auto-commit

The engine automatically commits text when you press:
-   `Space`
-   `Enter`
-   Punctuation marks (`.`, `,`, `;`, `:`, `!`, `?`)

To toggle between English and Bangla without switching layouts, press `Ctrl + G` (standard Avro behavior) – *Note: This feature may require additional implementation in future versions.*

## 🧹 Uninstallation

To remove the engine from your system:

```bash
sudo make uninstall
```

Then restart IBus:

```bash
ibus exit
ibus-daemon -drx
```

Finally, remove "Avro Bangla Phonetic" from `ibus-setup`.

## 🏗️ Project Structure

```text
avro-bangla-ibus/
├── main.go           # IBus event loop and key handling
├── avro.go           # Core Avro parsing logic
├── avro_rules.go     # Complete set of Avro phonetic rules
├── org.avro.bangla.xml # IBus component definition
├── Makefile          # Build and install scripts
├── go.mod            # Go module definition
└── README.md         # This file
```

## 🐛 Troubleshooting

### Issue: IBus doesn't show "Avro Bangla" in the list

**Fix:**
1.  Ensure `org.avro.bangla.xml` was copied to `/usr/local/share/ibus/component/`.
2.  Run `ibus exit` followed by `ibus-daemon -drx` to reload.
3.  Check that the XML file has correct permissions: `ls -l /usr/local/share/ibus/component/org.avro.bangla.xml`

### Issue: Typing produces Latin characters instead of Bangla

**Fix:**
1.  Check if the "Avro Bangla Phonetic" engine is actually selected in the system tray. You might still be on "English (US)".
2.  Use `Super + Space` to cycle through input methods until Avro Bangla is active.

### Issue: Build fails with missing headers

**Fix:**
1.  Ensure `pkg-config` is installed: `sudo pkg install pkgconf`
2.  Verify you have installed `ibus` and `dbus-glib` via `pkg`: `pkg info ibus dbus-glib`
3.  Try cleaning and rebuilding: `make clean && make build`

### Issue: IBus daemon won't start

**Fix:**
1.  Check if another input method is conflicting (e.g., fcitx).
2.  Ensure your user is part of the appropriate groups (usually not required on FreeBSD).
3.  Check logs: `tail -f ~/.xsession-errors` (for X11) or console output.

### Issue: Icons not showing

**Fix:**
The engine uses `/usr/local/share/icons/hicolor/48x48/apps/ibus-engine.png`. If this icon doesn't exist, IBus may show a generic keyboard icon. This is cosmetic and doesn't affect functionality.

## 🌐 Compatibility

This engine has been tested and designed for:
-   **GhostBSD** (MATE, XFCE, KDE)
-   **NomadBSD**
-   **TrueOS** (legacy)
-   **pfSense/OPNsense** (desktop installations only)
-   Any FreeBSD-based system with IBus installed

## 📄 License

This project is open-source under the MIT License. The Avro rule set is based on the widely used Avro Phonetic dictionary.

## 🤝 Contributing

Contributions to improve FreeBSD compatibility, add features, or fix bugs are welcome! Please submit issues or pull requests on GitHub.

## 🙏 Acknowledgments

-   Original Avro Phonetic by OmicronLab
-   IBus Framework developers
-   FreeBSD community for excellent documentation

---

*Built with ❤️ for the FreeBSD Community.*
