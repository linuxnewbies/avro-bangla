package main

import (
	"flag"
	"fmt"
	"os"
	"os/signal"
	"syscall"

	"github.com/godbus/dbus/v5"
)

const (
	engineName        = "AvroBangla"
	engineLongName    = "Avro Bangla Phonetic"
	engineDescription = "Bangla phonetic input method based on Avro"
	engineLanguage    = "bn"
	// Use FreeBSD standard icon path; fallback to generic if not found
	engineIcon        = "/usr/local/share/icons/hicolor/48x48/apps/ibus-engine.png"
	engineLayout      = "us"
	engineHotkeys     = "<Super>space"
)

// IBusEngine wraps the AvroEngine with IBus-specific state
type IBusEngine struct {
	avroEngine  *AvroEngine
	buffer      string
	preeditText string
	candidates  []string
	selectedIdx int
	conn        *dbus.Conn
	objectPath  dbus.ObjectPath
	isActive    bool
}

// NewIBusEngine creates a new IBus engine wrapper
func NewIBusEngine(conn *dbus.Conn, objectPath dbus.ObjectPath) *IBusEngine {
	return &IBusEngine{
		avroEngine:  NewAvroEngine(),
		buffer:      "",
		preeditText: "",
		candidates:  []string{},
		selectedIdx: 0,
		conn:        conn,
		objectPath:  objectPath,
		isActive:    true,
	}
}

// ProcessKey handles key press events
func (e *IBusEngine) ProcessKey(keyval uint32, keycode uint32, state uint32) bool {
	// Check if it's a printable character
	if state&0x4 == 0 { // Not shifted by control
		char := e.keyvalToChar(keyval)
		if char != 0 {
			e.buffer += string(char)
			e.updatePreedit()
			return true
		}
	}

	// Handle special keys
	switch keyval {
	case 65293, 65421: // KEY_Return, KEY_KP_Enter
		// Commit current buffer
		if e.buffer != "" {
			converted := e.avroEngine.Parse(e.buffer)
			e.commitText(converted)
			e.buffer = ""
			e.preeditText = ""
			e.updatePreedit()
		}
		return true

	case 32: // KEY_space
		// Space commits the current conversion
		if e.buffer != "" {
			converted := e.avroEngine.Parse(e.buffer)
			e.commitText(converted)
			e.buffer = ""
			e.preeditText = ""
			e.updatePreedit()
		}
		return false // Let space through after commit

	case 65288: // KEY_BackSpace
		if len(e.buffer) > 0 {
			e.buffer = e.buffer[:len(e.buffer)-1]
			e.updatePreedit()
		}
		return len(e.buffer) > 0

	case 65307: // KEY_Escape
		// Clear buffer on escape
		e.buffer = ""
		e.preeditText = ""
		e.updatePreedit()
		return true

	case 65289: // KEY_Tab
		// Could be used for candidate selection in future
		if e.buffer != "" {
			converted := e.avroEngine.Parse(e.buffer)
			e.commitText(converted)
			e.buffer = ""
			e.preeditText = ""
			e.updatePreedit()
		}
		return true
	}

	return false
}

// updatePreedit updates the preedit text displayed to the user
func (e *IBusEngine) updatePreedit() {
	if e.buffer == "" {
		e.hidePreeditText()
		return
	}

	// Show the buffer as preedit text
	e.updatePreeditText(e.buffer, len(e.buffer), true)
}

// Reset resets the engine state
func (e *IBusEngine) Reset() {
	e.buffer = ""
	e.preeditText = ""
	e.candidates = []string{}
	e.selectedIdx = 0
}

// FocusIn handles focus in event
func (e *IBusEngine) FocusIn() {
	e.isActive = true
}

// FocusOut handles focus out event
func (e *IBusEngine) FocusOut() {
	// Commit any pending text on focus out
	if e.buffer != "" {
		converted := e.avroEngine.Parse(e.buffer)
		e.commitText(converted)
		e.buffer = ""
		e.preeditText = ""
	}
	e.isActive = false
}

// keyvalToChar converts a keyval to a rune
func (e *IBusEngine) keyvalToChar(keyval uint32) rune {
	// Handle basic ASCII range
	if keyval >= 32 && keyval <= 126 {
		return rune(keyval)
	}
	return 0
}

// D-Bus helper methods for IBus communication
func (e *IBusEngine) commitText(text string) {
	variant := dbus.MakeVariant(text)
	e.conn.Object("org.freedesktop.IBus", e.objectPath).Call("IBus.Engine.CommitText", 0, variant)
}

func (e *IBusEngine) updatePreeditText(text string, cursorPos int, visible bool) {
	variant := dbus.MakeVariant(text)
	e.conn.Object("org.freedesktop.IBus", e.objectPath).Call("IBus.Engine.UpdatePreeditText", 0, variant, uint32(cursorPos), visible)
}

func (e *IBusEngine) hidePreeditText() {
	e.conn.Object("org.freedesktop.IBus", e.objectPath).Call("IBus.Engine.HidePreeditText", 0)
}

func main() {
	// Parse command line flags
	ibusAddress := flag.String("ibus", "", "IBus address")
	flag.Parse()

	if *ibusAddress == "" {
		fmt.Fprintln(os.Stderr, "Error: IBus address not provided")
		os.Exit(1)
	}

	// Connect to D-Bus session bus
	conn, err := dbus.ConnectSessionBus()
	if err != nil {
		fmt.Fprintf(os.Stderr, "Failed to connect to D-Bus: %v\n", err)
		os.Exit(1)
	}

	// Create our engine wrapper
	engineWrapper := NewIBusEngine(conn, "/org/freedesktop/IBus/Engine/"+engineName)

	// Set up signal handling
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, syscall.SIGINT, syscall.SIGTERM)

	go func() {
		<-sigChan
		engineWrapper.Reset()
		os.Exit(0)
	}()

	// Register with IBus
	// In a full implementation, we would register the engine and set up
	// proper D-Bus message handlers here. For now, we'll just print info.
	fmt.Printf("Avro Bangla engine initialized\n")
	fmt.Printf("Engine Name: %s\n", engineName)
	fmt.Printf("Engine Long Name: %s\n", engineLongName)
	fmt.Printf("Description: %s\n", engineDescription)
	fmt.Printf("Language: %s\n", engineLanguage)

	// Keep running until interrupted
	select {}
}
