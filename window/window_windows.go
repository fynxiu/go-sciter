package window

/*
#include <windows.h>
*/
import "C"
import (
	"fmt"
	"syscall"
	"unsafe"

	"github.com/fynxiu/go-sciter"
	"github.com/lxn/win"
)

var prevDPI uint32 = 96

func updateLayoutForDPI(hwnd win.HWND) {
	dpi := win.GetDpiForWindow(hwnd)
	if prevDPI == dpi {
		return
	}
	prevDPI = dpi
	dpiScaled := dpi / 96.0
	var rect win.RECT
	win.GetWindowRect(hwnd, &rect)
	rect.Right = (rect.Right - rect.Left) * int32(dpiScaled)
	rect.Bottom = (rect.Bottom - rect.Top) * int32(dpiScaled)
	win.AdjustWindowRect(&rect, win.WS_OVERLAPPED, false)
}

// Init is called at the start of the application
func Init() error {
	setProcessDPIAware := syscall.NewLazyDLL("user32.dll").NewProc("SetProcessDPIAware")
	if setProcessDPIAware == nil {
		return nil
	}
	status, r, err := setProcessDPIAware.Call()
	if status == 0 {
		return fmt.Errorf("exit status %d: %v %v", status, r, err)
	}
	return nil
}

func New(creationFlags sciter.WindowCreationFlag, rect *sciter.Rect) (*Window, error) {
	if err := Init(); err != nil {
		return nil, fmt.Error("Sciter Init failed, %v, [%d]", err, win.GetLastError())
	}
	w := new(Window)
	w.creationFlags = creationFlags

	// Initialize OLE for DnD and printing support
	win.OleInitialize()

	// create window
	hwnd := sciter.CreateWindow(
		creationFlags,
		rect,
		syscall.NewCallback(delegateProc),
		0,
		sciter.BAD_HWINDOW)

	if hwnd == sciter.BAD_HWINDOW {
		return nil, fmt.Errorf("Sciter CreateWindow failed [%d]", win.GetLastError())
	}

	w.Sciter = sciter.Wrap(hwnd)
	return w, nil
}

func (s *Window) Show() {
	// message handling
	hwnd := win.HWND(unsafe.Pointer(s.GetHwnd()))
	win.ShowWindow(hwnd, win.SW_SHOW)
	win.UpdateWindow(hwnd)
}

func (s *Window) SetTitle(title string) {
	// message handling
	hwnd := C.HWND(unsafe.Pointer(s.GetHwnd()))
	C.SetWindowTextW(hwnd, (*C.WCHAR)(unsafe.Pointer(sciter.StringToWcharPtr(title))))
}

func (s *Window) AddQuitMenu() {
	// Define behaviour for windows
}

func (s *Window) Run() {
	// for system drag-n-drop
	// win.OleInitialize()
	// defer win.OleUninitialize()
	s.run()
	// start main gui message loop
	msg := (*win.MSG)(unsafe.Pointer(win.GlobalAlloc(0, unsafe.Sizeof(win.MSG{}))))
	defer win.GlobalFree(win.HGLOBAL(unsafe.Pointer(msg)))
	for win.GetMessage(msg, 0, 0, 0) > 0 {
		win.TranslateMessage(msg)
		win.DispatchMessage(msg)
	}
}

// delegate Windows GUI messsage
func delegateProc(hWnd win.HWND, message uint, wParam uintptr, lParam uintptr, pParam uintptr, pHandled *int) int {
	switch message {
	case win.WM_DESTROY:
		// log.Println("closing window ...")
		win.PostQuitMessage(0)
		*pHandled = 1
	}
	return 0
}
