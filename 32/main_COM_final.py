import serial
import time

PORT = "COM1"       # ← замени при необходимости
BAUD = 115200

ser = serial.Serial(
    port=PORT,
    baudrate=BAUD,
    bytesize=serial.EIGHTBITS,
    parity=serial.PARITY_NONE,
    stopbits=serial.STOPBITS_ONE,
    timeout=0,          # неблокирующий режим
    rtscts=False,
    dsrdtr=False
)

print(f"✅ Порт {PORT} открыт ({BAUD} бод)")

# DTR и RTS включаем вручную (если нужно)
ser.dtr = True
ser.rts = True
time.sleep(0.1)

# Главное: отправить "T"
ser.write(b'T')
print("📤 Отправлено: 'T' (Start Transmission)")
time.sleep(0.1)

try:
    while True:
        if ser.in_waiting:
            data = ser.read(ser.in_waiting)
            print("📥", data)
        time.sleep(0.2)
except KeyboardInterrupt:
    # При выходе — отправляем "P"
    ser.write(b'P')
    print("\n📤 Отправлено: 'P' (Stop Transmission)")
finally:
    ser.close()
    print("🔒 Порт закрыт")
