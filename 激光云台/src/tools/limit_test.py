"""
限位测试 - 手动转电机，显示累计角度
A/D = X轴左/右
W/S = Y轴上/下
R = 归零
Q = 退出
转到极限位置时记下角度值告诉我
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


def move_forward(ser, addr, steps, speed=800, accel=200):
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
    ser.write(payload + struct.pack("<H", crc))
    time.sleep(0.02)
    ser.read(100)


COM_PORT = "COM9"
STEPS_PER_DEG_X = 6400 * 10 / 360.0
STEPS_PER_DEG_Y = 6400 * 5 / 360.0
STEP_SIZE = 500

ser = serial.Serial(COM_PORT, 115200, timeout=0.1, bytesize=8, stopbits=1, parity="N")
ser.reset_input_buffer()

pos_x = 0.0
pos_y = 0.0

print("=== Limit Test ===")
print("A=left  D=right  W=down  S=up  R=zero  Q=quit")
print("Watch the angle on screen, note the safe limits")

img = None
while True:
    frame = 200 * __import__('numpy').ones((200, 400, 3), dtype='uint8')
    cv2.putText(frame, "LIMIT TEST", (120, 30), cv2.FONT_HERSHEY_SIMPLEX, 0.7, (0, 255, 255), 2)
    cv2.putText(frame, f"X: {pos_x:+.1f} deg", (50, 90), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2)
    cv2.putText(frame, f"Y: {pos_y:+.1f} deg", (50, 140), cv2.FONT_HERSHEY_SIMPLEX, 0.8, (255, 255, 255), 2)
    cv2.putText(frame, "A/D=X  W/S=Y  R=zero  Q=quit", (30, 185), cv2.FONT_HERSHEY_SIMPLEX, 0.4, (150, 150, 150), 1)
    cv2.imshow("Limit Test", frame)

    key = cv2.waitKey(100) & 0xFF
    if key == ord('q'):
        break
    elif key == ord('a'):
        move_forward(ser, 0x01, STEP_SIZE, 800, 200)
        pos_x += STEP_SIZE / STEPS_PER_DEG_X
        print(f"X: {pos_x:+.1f} deg")
    elif key == ord('d'):
        move_forward(ser, 0x01, -STEP_SIZE, 800, 200)
        pos_x -= STEP_SIZE / STEPS_PER_DEG_X
        print(f"X: {pos_x:+.1f} deg")
    elif key == ord('w'):
        move_forward(ser, 0x02, STEP_SIZE, 800, 200)
        pos_y += STEP_SIZE / STEPS_PER_DEG_Y
        print(f"Y: {pos_y:+.1f} deg")
    elif key == ord('s'):
        move_forward(ser, 0x02, -STEP_SIZE, 800, 200)
        pos_y -= STEP_SIZE / STEPS_PER_DEG_Y
        print(f"Y: {pos_y:+.1f} deg")
    elif key == ord('r'):
        pos_x = 0.0
        pos_y = 0.0
        print("Zeroed!")

cv2.destroyAllWindows()
ser.close()
print(f"\nFinal: X={pos_x:+.1f} deg, Y={pos_y:+.1f} deg")
