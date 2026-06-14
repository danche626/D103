"""
电机方向测试 - 按键控制电机，摄像头实时显示
A/D = X轴正/反转
W/S = Y轴正/反转
Q = 退出
"""
import cv2
import serial
import struct
import time


def calc_crc(data):
    crc = 0xFFFF
    for byte in data:
        crc ^= byte
        for _ in range(8):
            if crc & 0x0001:
                crc = (crc >> 1) ^ 0xA001
            else:
                crc >>= 1
    return crc & 0xFFFF


def move_forward(ser, addr, steps, speed=800, accel=300):
    if steps == 0:
        return
    val = steps + 0x100000000 if steps < 0 else steps
    h = (val >> 16) & 0xFFFF
    l = val & 0xFFFF
    payload = bytes([addr, 0x10, 0x00, 0x40, 0x00, 0x06, 0x0C])
    data = struct.pack(">HH", h, l)
    data += struct.pack(">H", 0x0000)
    data += struct.pack(">H", speed)
    data += struct.pack(">H", accel)
    data += struct.pack(">H", 100)
    payload += data
    crc = calc_crc(payload)
    frame = payload + struct.pack("<H", crc)
    ser.write(frame)
    time.sleep(0.02)
    ser.read(100)


COM_PORT = "COM9"
STEP_SIZE = 500

ser = serial.Serial(COM_PORT, 115200, timeout=0.1, bytesize=8, stopbits=1, parity="N")
ser.reset_input_buffer()

cap = cv2.VideoCapture(1, cv2.CAP_DSHOW)
cap.set(cv2.CAP_PROP_FRAME_WIDTH, 640)
cap.set(cv2.CAP_PROP_FRAME_HEIGHT, 480)

print("=== Direction Test ===")
print("A = X +500  |  D = X -500")
print("W = Y +500  |  S = Y -500")
print("Q = Quit")

while True:
    ret, frame = cap.read()
    if not ret:
        continue
    h, w = frame.shape[:2]
    cv2.drawMarker(frame, (w//2, h//2), (0, 255, 0), cv2.MARKER_CROSS, 30, 1)
    cv2.putText(frame, "A/D=X  W/S=Y  Q=quit", (10, 30),
                cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 0), 2)
    cv2.imshow("Direction Test", frame)
    key = cv2.waitKey(33) & 0xFF
    if key == ord('q'):
        break
    elif key == ord('a'):
        move_forward(ser, 0x01, STEP_SIZE, 800, 300)
        print("X +500")
    elif key == ord('d'):
        move_forward(ser, 0x01, -STEP_SIZE, 800, 300)
        print("X -500")
    elif key == ord('w'):
        move_forward(ser, 0x02, STEP_SIZE, 800, 300)
        print("Y +500")
    elif key == ord('s'):
        move_forward(ser, 0x02, -STEP_SIZE, 800, 300)
        print("Y -500")

cap.release()
cv2.destroyAllWindows()
ser.close()
print("Done")
