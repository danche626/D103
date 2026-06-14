"""
Laser Gimbal Tracker v3 - Jetson Nano
- Fullscreen camera display
- KEY1: toggle tracking (rising OR falling edge, auto-detect)
- KEY2: position reset
- No buzzer
- Auto-start tracking on boot
- Laser offset compensation
"""
import os
import cv2
import numpy as np
import serial
import struct
import time
import signal
import sys
import glob
import argparse

os.environ["DISPLAY"] = ":0"
sys.stdout = os.fdopen(sys.stdout.fileno(), 'w', buffering=1)

try:
    import Jetson.GPIO as GPIO
    GPIO_AVAILABLE = True
except ImportError:
    GPIO_AVAILABLE = False

PIN_LASER = 12
PIN_KEY1 = 7     # Pin7(GPIO4)有上拉，接按钮到GND(Pin9)即可检测
PIN_KEY2 = 11    # Pin11(GPIO17)备用，接按钮到GND

def find_serial_port():
    candidates = glob.glob("/dev/ttyUSB*") + glob.glob("/dev/ttyACM*")
    if not candidates:
        return None
    for c in sorted(candidates):
        try:
            s = serial.Serial(c, 115200, timeout=0.1)
            s.close()
            return c
        except (serial.SerialException, OSError):
            continue
    return None

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


class MotorController:
    def __init__(self, port, baudrate=115200):
        self.ser = serial.Serial(port, baudrate, timeout=0.01,
                                 bytesize=8, stopbits=1, parity="N")
        self.ser.reset_input_buffer()
        self._last_spd = {0x01: 0, 0x02: 0}

    def set_velocity(self, addr, speed, accel=40):
        speed = int(speed)
        if abs(speed - self._last_spd[addr]) < 5:
            return
        self._last_spd[addr] = speed
        if speed == 0:
            self._stop(addr)
            return
        val = speed & 0xFFFF
        payload = bytes([addr, 0x10, 0x00, 0x60, 0x00, 0x04, 0x08])
        payload += struct.pack(">HHHH", 0x0000, val, accel, 100)
        crc = calc_crc(payload)
        self.ser.write(payload + struct.pack("<H", crc))
        self.ser.reset_input_buffer()

    def _stop(self, addr):
        payload = bytes([addr, 0x10, 0x00, 0x01, 0x00, 0x02, 0x04])
        payload += struct.pack(">HH", 0x0000, 0x0000)
        crc = calc_crc(payload)
        self.ser.write(payload + struct.pack("<H", crc))
        self.ser.reset_input_buffer()

    def stop_all(self):
        self._stop(0x01)
        self._stop(0x02)
        self._last_spd = {0x01: 0, 0x02: 0}

    def close(self):
        self.stop_all()
        self.ser.close()


class LaserController:
    def __init__(self, pin=PIN_LASER):
        self.pin = pin
        self.on = False
        if GPIO_AVAILABLE:
            GPIO.setup(self.pin, GPIO.OUT, initial=GPIO.LOW)

    def fire(self):
        if self.on:
            return
        self.on = True
        if GPIO_AVAILABLE:
            GPIO.output(self.pin, GPIO.HIGH)

    def off(self):
        if not self.on:
            return
        self.on = False
        if GPIO_AVAILABLE:
            GPIO.output(self.pin, GPIO.LOW)


def detect_target(gray):
    _, binary = cv2.threshold(gray, 0, 255, cv2.THRESH_BINARY + cv2.THRESH_OTSU)
    _, contours, _ = cv2.findContours(binary, cv2.RETR_TREE, cv2.CHAIN_APPROX_SIMPLE)
    h, w = gray.shape
    frame_area = h * w
    best = None
    best_area = 0
    for contour in contours:
        area = cv2.contourArea(contour)
        if area < frame_area * 0.02 or area > frame_area * 0.5:
            continue
        peri = cv2.arcLength(contour, True)
        approx = cv2.approxPolyDP(contour, 0.02 * peri, True)
        if len(approx) != 4:
            continue
        if not cv2.isContourConvex(approx):
            continue
        rect = cv2.minAreaRect(contour)
        rw, rh = rect[1]
        if min(rw, rh) < 10:
            continue
        if max(rw, rh) / min(rw, rh) > 2.0:
            continue
        mask = np.zeros(gray.shape, dtype=np.uint8)
        cv2.drawContours(mask, [approx], -1, 255, -1)
        if cv2.mean(binary, mask=mask)[0] < 180:
            continue
        if area > best_area:
            best_area = area
            M = cv2.moments(approx)
            if M["m00"] > 0:
                cx = int(M["m10"] / M["m00"])
                cy = int(M["m01"] / M["m00"])
                best = (cx, cy, area, approx)
    return best


class DualPID:
    def __init__(self, kp, kd, ki_vel, max_out, deadzone):
        self.kp = kp
        self.kd = kd
        self.ki_vel = ki_vel
        self.max_out = max_out
        self.deadzone = deadzone
        self._prev_err = 0.0
        self._output = 0.0
        self._integral = 0.0
        self._prev_t = None

    def compute(self, error):
        now = time.time()
        dt = 0.015 if self._prev_t is None else max(now - self._prev_t, 0.001)
        self._prev_t = now
        if abs(error) < self.deadzone:
            self._output *= 0.4
            self._integral = 0
            self._prev_err = 0
            if abs(self._output) < 3:
                self._output = 0
            return int(self._output)
        deriv = (error - self._prev_err) / dt
        target_v = self.kp * error + self.kd * deriv
        target_v = np.clip(target_v, -self.max_out * 1.5, self.max_out * 1.5)
        self._prev_err = error
        v_err = target_v - self._output
        self._integral += v_err * dt
        self._integral = np.clip(self._integral, -200, 200)
        self._output += 0.75 * v_err + self.ki_vel * self._integral
        self._output = np.clip(self._output, -self.max_out, self.max_out)
        return int(self._output)

    def reset(self):
        self._prev_err = 0.0
        self._output = 0.0
        self._integral = 0.0
        self._prev_t = None


# === CONFIG ===
WIDTH, HEIGHT = 320, 240
DEADZONE = 8
KP = 5.5
KD = 1.0
KI_VEL = 6.0
MAX_SPEED = 650
ALPHA = 0.65
Y_LIMIT_UP = 55.0
Y_LIMIT_DOWN = 15.0
ALIGN_THRESHOLD = 5
# 激光笔在摄像头正下方4.5cm，目标参考点需上移，使激光打到靶心
# 负值=参考点上移，实测调整此值（距离越近值越大）
LASER_OFFSET_Y = -25


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port", default=None)
    parser.add_argument("--camera", type=int, default=0)
    parser.add_argument("--no-fullscreen", action="store_true")
    args = parser.parse_args()

    port = args.port or find_serial_port()
    if port is None:
        print("ERROR: No serial port found!")
        sys.exit(1)
    print(f"[INIT] Serial: {port}")

    if GPIO_AVAILABLE:
        GPIO.setmode(GPIO.BOARD)
        GPIO.setup(PIN_KEY1, GPIO.IN, pull_up_down=GPIO.PUD_UP)
        GPIO.setup(PIN_KEY2, GPIO.IN, pull_up_down=GPIO.PUD_UP)

    motor = MotorController(port)
    laser = LaserController()

    cap = cv2.VideoCapture(args.camera)
    cap.set(cv2.CAP_PROP_FOURCC, cv2.VideoWriter_fourcc('M','J','P','G'))
    cap.set(cv2.CAP_PROP_FRAME_WIDTH, WIDTH)
    cap.set(cv2.CAP_PROP_FRAME_HEIGHT, HEIGHT)
    cap.set(cv2.CAP_PROP_FPS, 120)
    cap.set(cv2.CAP_PROP_BUFFERSIZE, 1)
    if not cap.isOpened():
        print("ERROR: Camera failed!")
        motor.close()
        sys.exit(1)
    print("[INIT] Camera OK")

    # Fullscreen window
    if not args.no_fullscreen:
        cv2.namedWindow("Track", cv2.WINDOW_NORMAL)
        cv2.setWindowProperty("Track", cv2.WND_PROP_FULLSCREEN, cv2.WINDOW_FULLSCREEN)

    for _ in range(10):
        cap.read()

    pid_x = DualPID(KP, KD, KI_VEL, MAX_SPEED, DEADZONE)
    pid_y = DualPID(KP, KD, KI_VEL, MAX_SPEED, DEADZONE)

    # Start tracking immediately
    tracking = True
    aligned_frames = 0
    fx, fy = None, None
    prev_area = None
    lost_count = 0
    pos_y_deg = 0.0
    prev_loop_t = time.time()

    # KEY state: read actual GPIO to prevent false trigger on first frame
    if GPIO_AVAILABLE:
        key1_last = GPIO.input(PIN_KEY1)
        key2_last = GPIO.input(PIN_KEY2)
    else:
        key1_last = 1
        key2_last = 1
    # Ignore first transition for 0.5s after boot
    boot_time = time.time()

    running = True
    def sig_handler(sig, _f):
        nonlocal running
        running = False
    signal.signal(signal.SIGINT, sig_handler)
    signal.signal(signal.SIGTERM, signal.SIG_IGN)
    signal.signal(signal.SIGHUP, signal.SIG_IGN)

    print("[READY] Tracking auto-started. KEY1=toggle KEY2=reset")

    while running:
        # KEY detection - ignore for first 0.5s to avoid boot glitch
        if GPIO_AVAILABLE and (time.time() - boot_time) > 0.5:
            key1_now = GPIO.input(PIN_KEY1)
            key2_now = GPIO.input(PIN_KEY2)

            # Detect ANY edge change on KEY1 (works for both active-high and active-low)
            if key1_now != key1_last:
                time.sleep(0.03)
                key1_confirm = GPIO.input(PIN_KEY1)
                if key1_confirm == key1_now:
                    tracking = not tracking
                    if tracking:
                        print("[KEY1] ON")
                    else:
                        print("[KEY1] OFF")
                        motor.stop_all()
                        laser.off()
                        pid_x.reset()
                        pid_y.reset()
                        aligned_frames = 0
                        fx = fy = None
                        prev_area = None
            key1_last = key1_now

            if key2_now != key2_last:
                time.sleep(0.03)
                key2_confirm = GPIO.input(PIN_KEY2)
                if key2_confirm == key2_now:
                    print("[KEY2] Reset")
                    motor.stop_all()
                    laser.off()
                    pid_x.reset()
                    pid_y.reset()
                    aligned_frames = 0
                    fx = fy = None
                    prev_area = None
                    pos_y_deg = 0.0
            key2_last = key2_now

        # Read frame
        cap.grab()
        ret, frame = cap.read()
        if not ret:
            continue

        h, w = frame.shape[:2]
        cx, cy = w // 2, h // 2

        if not tracking:
            cv2.putText(frame, "STANDBY", (cx - 50, cy),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.7, (128, 128, 128), 2)
            cv2.imshow("Track", frame)
            key = cv2.waitKey(1) & 0xFF
            if key == ord(' '):
                tracking = True
                print("[SPACE] ON")
            continue

        # Tracking logic
        gray = cv2.cvtColor(frame, cv2.COLOR_BGR2GRAY)
        det = detect_target(gray)

        if det and prev_area is not None:
            ratio = det[2] / prev_area
            if ratio < 0.4 or ratio > 2.5:
                det = None

        if det:
            tx, ty, area, approx = det
            prev_area = area
            lost_count = 0
            if fx is None:
                fx, fy = float(tx), float(ty)
            else:
                fx = ALPHA * tx + (1 - ALPHA) * fx
                fy = ALPHA * ty + (1 - ALPHA) * fy

            err_x = fx - cx
            err_y = fy - (cy + LASER_OFFSET_Y)
            spd_x = pid_x.compute(err_x)
            spd_y = pid_y.compute(err_y)

            now_t = time.time()
            dt_loop = now_t - prev_loop_t
            prev_loop_t = now_t
            if motor._last_spd[0x02] != 0:
                pos_y_deg += motor._last_spd[0x02] * 0.12 * dt_loop

            cmd_x = -spd_x
            cmd_y = -spd_y
            if pos_y_deg > Y_LIMIT_UP and cmd_y > 0:
                cmd_y = 0
            elif pos_y_deg < -Y_LIMIT_DOWN and cmd_y < 0:
                cmd_y = 0

            motor.set_velocity(0x01, cmd_x)
            motor.set_velocity(0x02, cmd_y)

            if abs(err_x) < DEADZONE and abs(err_y) < DEADZONE:
                aligned_frames += 1
                if aligned_frames >= ALIGN_THRESHOLD:
                    laser.fire()
            else:
                aligned_frames = 0
                laser.off()

            cv2.drawContours(frame, [approx], -1, (0, 255, 255), 2)
            cv2.circle(frame, (int(fx), int(fy)), 5, (0, 0, 255), -1)
        else:
            lost_count += 1
            aligned_frames = 0
            laser.off()
            if lost_count > 2:
                spd_x = pid_x.compute(0)
                spd_y = pid_y.compute(0)
                motor.set_velocity(0x01, -spd_x)
                motor.set_velocity(0x02, -spd_y)
            if lost_count > 15:
                motor.stop_all()
                pid_x.reset()
                pid_y.reset()
                fx = fy = None
                prev_area = None

        # Display
        cv2.drawMarker(frame, (cx, cy), (0, 255, 0), cv2.MARKER_CROSS, 15, 1)
        if laser.on:
            cv2.circle(frame, (cx, cy), 20, (0, 255, 0), 3)
            cv2.putText(frame, "LASER", (cx - 25, cy - 25),
                        cv2.FONT_HERSHEY_SIMPLEX, 0.5, (0, 255, 0), 2)
        status = "TRACKING" if tracking else "STANDBY"
        cv2.putText(frame, status, (5, h - 8),
                    cv2.FONT_HERSHEY_SIMPLEX, 0.4, (0, 255, 0), 1)
        cv2.imshow("Track", frame)
        key = cv2.waitKey(1) & 0xFF
        if key == ord(' '):
            tracking = not tracking
            if not tracking:
                print("[SPACE] OFF")
                motor.stop_all()
                laser.off()
                pid_x.reset()
                pid_y.reset()
                aligned_frames = 0
                fx = fy = None
                prev_area = None
            else:
                print("[SPACE] ON")

    # Cleanup
    laser.off()
    motor.close()
    cap.release()
    if GPIO_AVAILABLE:
        GPIO.cleanup()
    cv2.destroyAllWindows()
    print("[EXIT] Done")


if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        pass
    finally:
        if GPIO_AVAILABLE:
            GPIO.cleanup()
