import ctypes
from ctypes import wintypes
import time
import msvcrt

# --- Константы WinAPI ---
GENERIC_READ  = 0x80000000
GENERIC_WRITE = 0x40000000
OPEN_EXISTING = 3

RTS_CONTROL_DISABLE = 0
RTS_CONTROL_ENABLE = 1
RTS_CONTROL_HANDSHAKE = 2
RTS_CONTROL_TOGGLE = 3
DTR_CONTROL_DISABLE = 0
DTR_CONTROL_ENABLE = 1

PURGE_TXABORT = 0x0001
PURGE_RXABORT = 0x0002
PURGE_TXCLEAR = 0x0004
PURGE_RXCLEAR = 0x0008

MS_CTS_ON  = 0x10
MS_DSR_ON  = 0x20
MS_RING_ON = 0x40
MS_RLSD_ON = 0x80

INVALID_HANDLE_VALUE = ctypes.c_void_p(-1).value

# --- Структуры DCB и TIMEOUTS ---
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

def check(result):
    if not result:
        raise ctypes.WinError(ctypes.get_last_error())

# --- Открытие COM-порта ---
def open_com_port(port_name):
    handle = kernel32.CreateFileW(
        port_name,
        GENERIC_READ | GENERIC_WRITE,
        0,
        None,
        OPEN_EXISTING,
        0,
        None,
    )
    if handle == INVALID_HANDLE_VALUE:
        raise ctypes.WinError(ctypes.get_last_error())
    return handle

# --- Настройка DCB ---
def configure_port(handle):
    dcb = DCB()
    dcb.DCBlength = ctypes.sizeof(DCB)
    check(kernel32.GetCommState(handle, ctypes.byref(dcb)))

    dcb.BaudRate = 115200
    dcb.ByteSize = 8
    dcb.Parity = 0
    dcb.StopBits = 0
    dcb.XonLim = 2048
    dcb.XoffLim = 512

    # флаги: RTS=TOGGLE + CTS/DSR flow + DTR=ENABLE + Binary
    dcb.fFlags = 0
    dcb.fFlags |= (1 << 0)  # fBinary
    dcb.fFlags |= (1 << 1)  # fOutxCtsFlow
    dcb.fFlags |= (1 << 2)  # fOutxDsrFlow
    dcb.fFlags |= (DTR_CONTROL_ENABLE << 4)
    dcb.fFlags |= (RTS_CONTROL_TOGGLE << 12)

    check(kernel32.SetCommState(handle, ctypes.byref(dcb)))

    # Таймауты = OFF
    timeouts = COMMTIMEOUTS()
    check(kernel32.SetCommTimeouts(handle, ctypes.byref(timeouts)))

    # Очистка портов
    purge_flags = PURGE_RXABORT | PURGE_TXABORT | PURGE_RXCLEAR | PURGE_TXCLEAR
    check(kernel32.PurgeComm(handle, purge_flags))

    print("✅ Порт настроен: RTS=TOGGLE + CTS/DSR flow + DTR=ON, Timeout=OFF")

# --- Получение статуса линий ---
def print_modem_status(handle):
    modem_status = wintypes.DWORD()
    if kernel32.GetCommModemStatus(handle, ctypes.byref(modem_status)):
        status = modem_status.value
        print("CTS:", "ON" if status & MS_CTS_ON else "off",
              "| DSR:", "ON" if status & MS_DSR_ON else "off",
              "| RING:", "ON" if status & MS_RING_ON else "off",
              "| RLSD:", "ON" if status & MS_RLSD_ON else "off")

# --- Чтение данных ---
def read_loop(handle):
    buf = ctypes.create_string_buffer(1024)
    bytes_read = wintypes.DWORD()
    print("📡 Ожидание данных... (нажмите 'q' чтобы выйти)")
    while True:
        if msvcrt.kbhit() and msvcrt.getch() == b'q':
            break
        success = kernel32.ReadFile(handle, buf, 1024, ctypes.byref(bytes_read), None)
        if success and bytes_read.value > 0:
            data = buf.raw[:bytes_read.value]
            print("📥", " ".join(f"{b:02X}" for b in data))
        else:
            print_modem_status(handle)
            time.sleep(0.3)
# --- Основная программа ---
if __name__ == "main":
    port = r"\\\\.\\COM1"  # важно — для COM10+ и полной совместимости
    h = open_com_port(port)
    configure_port(h)
    try:
        read_loop(h)
    finally:
        kernel32.CloseHandle(h)
        print("🔒 Порт закрыт")