import serial
import ctypes
from ctypes import wintypes
import time

# --- Константы WinAPI ---
RTS_CONTROL_DISABLE = 0
RTS_CONTROL_ENABLE = 1
RTS_CONTROL_HANDSHAKE = 2
RTS_CONTROL_TOGGLE = 3
DTR_CONTROL_DISABLE = 0
DTR_CONTROL_ENABLE = 1

PURGE_RXABORT = 0x0002
PURGE_RXCLEAR = 0x0008

# --- Структура DCB из WinAPI ---
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


def set_rts_toggle(handle):
    """Полная настройка: RTS=TOGGLE + CTS/DSR handshake + DTR=ON, как у старой программы"""
    dcb = DCB()
    dcb.DCBlength = ctypes.sizeof(DCB)
    check(kernel32.GetCommState(handle, ctypes.byref(dcb)))

    # --- Основные параметры ---
    dcb.BaudRate = 115200
    dcb.ByteSize = 8
    dcb.Parity = 0
    dcb.StopBits = 0  # 1 стоп-бит
    dcb.XonLim = 2048
    dcb.XoffLim = 512

    # --- Флаги DCB (побитно как в старой программе) ---
    dcb.fFlags = 0
    dcb.fFlags |= (1 << 0)  # fBinary = 1
    dcb.fFlags |= (1 << 1)  # fOutxCtsFlow = 1
    dcb.fFlags |= (1 << 2)  # fOutxDsrFlow = 1
    dcb.fFlags |= (DTR_CONTROL_ENABLE << 4)  # DTR=ON
    dcb.fFlags |= (RTS_CONTROL_TOGGLE << 12)  # RTS=TOGGLE
    dcb.fFlags &= ~(1 << 7)  # fAbortOnError = 0
    dcb.fFlags &= ~(1 << 8)  # fOutX = 0
    dcb.fFlags &= ~(1 << 9)  # fInX = 0

    # --- Буферы: 1024 как в старой программе ---
    check(kernel32.SetupComm(handle, 1024, 1024))

    # --- Применяем DCB ---
    check(kernel32.SetCommState(handle, ctypes.byref(dcb)))

    # --- Отключаем таймауты (Timeout OFF) ---
    timeouts = COMMTIMEOUTS()
    timeouts.ReadIntervalTimeout = 0
    timeouts.ReadTotalTimeoutMultiplier = 0
    timeouts.ReadTotalTimeoutConstant = 0
    timeouts.WriteTotalTimeoutMultiplier = 0
    timeouts.WriteTotalTimeoutConstant = 0
    check(kernel32.SetCommTimeouts(handle, ctypes.byref(timeouts)))

    # --- Очистка ТОЛЬКО RX ---
    check(kernel32.PurgeComm(handle, PURGE_RXABORT | PURGE_RXCLEAR))

    print("✅ RTS=TOGGLE + CTS/DSR handshake, DTR=ON, XonLim=2048, XoffLim=512")
    print("✅ Буферы = 1024 / 1024, Timeout=OFF, очищен RX\n")


# --- Основной код ---
PORT = "COM1"
BAUD = 115200

ser = serial.Serial(
    port=PORT,
    baudrate=BAUD,
    bytesize=serial.EIGHTBITS,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    timeout=0,        # неблокирующее чтение
    rtscts=False,
    dsrdtr=False,
)

print(f"🔌 Порт {PORT} открыт на {BAUD} бод")

# Настройка линии через WinAPI
set_rts_toggle(ser.fileno())

# --- Цикл чтения ---
try:
    print("📡 Ожидание данных...")
    while True:
        waiting = ser.in_waiting
        if waiting:
            data = ser.read(waiting)
            print("📥 Получено:", " ".join(f"{b:02X}" for b in data))
        else:
            print("…тишина (in_waiting=0)")
        time.sleep(0.3)
except KeyboardInterrupt:
    pass
finally:
    ser.close()
    print("🔒 Порт закрыт")
