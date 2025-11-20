import ctypes
from ctypes import wintypes
import time, msvcrt, sys

# --- Константы WinAPI ---
GENERIC_READ  = 0x80000000
GENERIC_WRITE = 0x40000000
OPEN_EXISTING = 3

RTS_CONTROL_TOGGLE = 3
DTR_CONTROL_ENABLE = 1

PURGE_RXABORT = 0x0002
PURGE_RXCLEAR = 0x0008

SETDTR = 5
CLRDTR = 6
SETRTS = 3
CLRRTS = 4

MS_CTS_ON  = 0x10
MS_DSR_ON  = 0x20
MS_RING_ON = 0x40
MS_RLSD_ON = 0x80

INVALID_HANDLE_VALUE = ctypes.c_void_p(-1).value

# --- Структуры ---
class DCB(ctypes.Structure):
    _fields_ = [
        ("DCBlength", wintypes.DWORD),
        ("BaudRate", wintypes.DWORD),
        ("fFlags", wintypes.DWORD),
        ("wReserved", wintypes.WORD),
        ("XonLim", wintypes.WORD),
        ("XoffLim", wintypes.WORD),
        ("ByteSize", ctypes.c_ubyte),
        ("Parity", ctypes.c_ubyte),
        ("StopBits", ctypes.c_ubyte),
        ("XonChar", ctypes.c_char),
        ("XoffChar", ctypes.c_char),
        ("ErrorChar", ctypes.c_char),
        ("EofChar", ctypes.c_char),
        ("EvtChar", ctypes.c_char),
        ("wReserved1", ctypes.c_ushort),
    ]

class COMMTIMEOUTS(ctypes.Structure):
    _fields_ = [
        ("ReadIntervalTimeout", wintypes.DWORD),
        ("ReadTotalTimeoutMultiplier", wintypes.DWORD),
        ("ReadTotalTimeoutConstant", wintypes.DWORD),
        ("WriteTotalTimeoutMultiplier", wintypes.DWORD),
        ("WriteTotalTimeoutConstant", wintypes.DWORD),
    ]

kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)

def check(ok, msg="WinAPI error"):
    if not ok:
        err = ctypes.get_last_error()
        raise ctypes.WinError(err, f"{msg} (code={err})")

# --- Настройка и инициализация порта ---
def open_and_configure(port_name):
    # if not port_name.startswith(r"\\"):
    #     port_name = r"\\\\.\\{}".format(port_name)

    print(f"🔌 Открываю {port_name} ...")
    handle = kernel32.CreateFileW(
        port_name,
        GENERIC_READ | GENERIC_WRITE,
        0, None,
        OPEN_EXISTING,
        0, None,
    )
    if handle == INVALID_HANDLE_VALUE or handle == 0:
        err = ctypes.get_last_error()
        raise ctypes.WinError(err, f"Не удалось открыть {port_name}")

    # дать драйверу время инициализироваться
    time.sleep(0.1)

    # задать размер буферов
    check(kernel32.SetupComm(handle, 1024, 1024), "SetupComm")

    # поднять DTR и RTS
    check(kernel32.EscapeCommFunction(handle, SETDTR), "SETDTR")
    check(kernel32.EscapeCommFunction(handle, SETRTS), "SETRTS")
    print("📶 DTR и RTS подняты (инициализация устройства)")
    time.sleep(0.2)

    # конфигурация DCB
    dcb = DCB()
    dcb.DCBlength = ctypes.sizeof(DCB)
    check(kernel32.GetCommState(handle, ctypes.byref(dcb)))

    dcb.BaudRate = 115200
    dcb.ByteSize = 8
    dcb.Parity = 0
    dcb.StopBits = 0
    dcb.XonLim = 2048
    dcb.XoffLim = 512

    f = 0
    f |= (1 << 0)  # fBinary
    f |= (DTR_CONTROL_ENABLE << 4)  # DTR=ON
    f |= (RTS_CONTROL_TOGGLE << 12) # RTS=TOGGLE
    f |= (1 << 10) # fErrorChar
    f &= ~(1 << 6) # fDsrSensitivity=0
    dcb.fFlags = f
    dcb.ErrorChar = b'?'

    check(kernel32.SetCommState(handle, ctypes.byref(dcb)))
    to = COMMTIMEOUTS()
    check(kernel32.SetCommTimeouts(handle, ctypes.byref(to)))
    check(kernel32.PurgeComm(handle, PURGE_RXABORT | PURGE_RXCLEAR))
    print("✅ RTS=TOGGLE, DTR=ON, ErrorChar='?', Buffers=1024, Timeout=OFF")

    # инициализационный байт 0x00
    sent = wintypes.DWORD()
    buf = (ctypes.c_char * 1)(b'\x00')
    kernel32.WriteFile(handle, buf, 1, ctypes.byref(sent), None)
    time.sleep(0.1)

    # команда “T” (Start Transmission)
    kernel32.WriteFile(handle, (ctypes.c_char * 1)(b'T'), 1, ctypes.byref(sent), None)
    print("📤 Отправлено: 'T' (Start Transmission)")
    time.sleep(0.1)

    return handle

# --- чтение данных ---
def print_modem_status(h):
    status = wintypes.DWORD()
    if kernel32.GetCommModemStatus(h, ctypes.byref(status)):
        s = status.value
        print(f"CTS={'ON' if s & MS_CTS_ON else 'off'} | "
              f"DSR={'ON' if s & MS_DSR_ON else 'off'} | "
              f"RING={'ON' if s & MS_RING_ON else 'off'} | "
              f"RLSD={'ON' if s & MS_RLSD_ON else 'off'}")

def read_loop(h):
    buf = ctypes.create_string_buffer(1024)
    n = wintypes.DWORD()
    print("\n📡 Ожидание данных... (нажмите 'q' для выхода)\n")
    try:
        while True:
            if msvcrt.kbhit() and msvcrt.getch() == b'q':
                break
            ok = kernel32.ReadFile(h, buf, 1024, ctypes.byref(n), None)
            if ok and n.value > 0:
                data = buf.raw[:n.value]
                print("📥", " ".join(f"{b:02X}" for b in data))
            else:
                print_modem_status(h)
                time.sleep(0.2)
    finally:
        # команда “P” (Stop Transmission)
        sent = wintypes.DWORD()
        kernel32.WriteFile(h, (ctypes.c_char * 1)(b'P'), 1, ctypes.byref(sent), None)
        print("📤 Отправлено: 'P' (Stop Transmission)")
        kernel32.CloseHandle(h)
        print("🔒 Порт закрыт")

# --- вход ---
if __name__ == "__main__":
    try:
        port = "COM1"  # укажи свой порт, например "COM3" или "COM10"
        h = open_and_configure(port)
        read_loop(h)
    except Exception as e:
        print("❌ Ошибка:", e)
        sys.exit(1)
