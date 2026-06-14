import math
import os
import time

from machine import FPIOA, UART
from media.display import Display
from media.media import MediaManager
from media.sensor import Sensor


FRAME_WIDTH = 320
FRAME_HEIGHT = 240
THRESHOLD = [(0, 160)]
RECT_THRESHOLD = 10000
MIN_RECT_W = 50
MIN_RECT_H = 50
MAX_RECT_W = 200
MAX_RECT_H = 150
MIN_ASPECT_RATIO = 0.6
MAX_ASPECT_RATIO = 1.8
MAX_TRACK_DISTANCE = 80
AREA_RATIO_MIN = 0.55
AREA_RATIO_MAX = 1.8
DEFAULT_CX = 300
DEFAULT_CY = 300
LOST_FRAME_LIMIT = 4
SMOOTH_KEEP = 0.82
SMOOTH_NEW = 0.18
CENTER_DEADBAND = 2
RECT_DEADBAND = 3
TRACK_REJECT_DISTANCE = 110
UART_PORT = UART.UART2
UART_BAUDRATE = 115200
UART_TX_PIN = 11
UART_RX_PIN = 12
EXPOSURE_US = 50000
GAIN_DB = 15


def setup_uart():
    fpioa = FPIOA()
    fpioa.set_function(UART_TX_PIN, FPIOA.UART2_TXD)
    fpioa.set_function(UART_RX_PIN, FPIOA.UART2_RXD)
    try:
        return UART(UART_PORT, UART_BAUDRATE, UART.EIGHTBITS, UART.PARITY_NONE, UART.STOPBITS_ONE)
    except TypeError:
        return UART(UART_PORT, UART_BAUDRATE)


def try_call(func, *args, **kwargs):
    try:
        return func(*args, **kwargs)
    except TypeError:
        return None
    except AttributeError:
        return None
    except Exception as exc:
        print("warn:", exc)
        return None


def try_invoke(func, *args, **kwargs):
    try:
        func(*args, **kwargs)
        return True
    except TypeError:
        return False
    except AttributeError:
        return False
    except Exception as exc:
        print("warn:", exc)
        return False


def try_method(obj, method_name, *args, **kwargs):
    func = getattr(obj, method_name, None)
    if func is None:
        return None
    return try_call(func, *args, **kwargs)


def configure_sensor():
    sensor = Sensor()
    sensor.reset()
    try:
        sensor.set_framesize(width=FRAME_WIDTH, height=FRAME_HEIGHT)
    except TypeError:
        sensor.set_framesize(Sensor.QVGA)
    try:
        sensor.set_pixformat(Sensor.GRAYSCALE)
    except TypeError:
        sensor.set_pixformat(Sensor.GRAYSCALE)
    sensor.set_hmirror(False)
    sensor.set_vflip(False)

    try_method(sensor, "set_auto_exposure", 0)
    try_method(sensor, "set_auto_exposure", False)
    try_method(sensor, "auto_exposure", False)

    if not try_invoke(Display.init, Display.VIRT, width=FRAME_WIDTH, height=FRAME_HEIGHT, fps=30, to_ide=True):
        if not try_invoke(Display.init, Display.VIRT, width=FRAME_WIDTH, height=FRAME_HEIGHT):
            if not try_invoke(Display.init, Display.VIRT, FRAME_WIDTH, FRAME_HEIGHT, 30):
                Display.init(Display.VIRT)
    MediaManager.init()
    sensor.run()

    try_method(sensor, "set_auto_gain", 0)
    try_method(sensor, "set_auto_gain", False)
    try_method(sensor, "set_auto_gain", False, GAIN_DB)
    try_method(sensor, "again", GAIN_DB)
    try_method(sensor, "set_exposure", EXPOSURE_US)
    try_method(sensor, "exposure", EXPOSURE_US)

    time.sleep_ms(2000)
    return sensor


def send_center(uart, cx, cy):
    output_str = "%03d,%03d" % (int(cx), int(cy))
    uart.write(output_str + "\r\n")
    print("you send:", output_str)


def make_rect_info(rect, offset_x=0, offset_y=0):
    x = rect.x() + offset_x
    y = rect.y() + offset_y
    w = rect.w()
    h = rect.h()
    cx = x + w / 2
    cy = y + h / 2
    corners = []
    for point in rect.corners():
        corners.append((point[0] + offset_x, point[1] + offset_y))
    return {
        "x": x,
        "y": y,
        "w": w,
        "h": h,
        "cx": cx,
        "cy": cy,
        "area": w * h,
        "magnitude": rect.magnitude(),
        "corners": corners,
    }


def clamp_rect(x, y, w, h):
    if x < 0:
        w += x
        x = 0
    if y < 0:
        h += y
        y = 0
    if x + w > FRAME_WIDTH:
        w = FRAME_WIDTH - x
    if y + h > FRAME_HEIGHT:
        h = FRAME_HEIGHT - y
    if w < 1:
        w = 1
    if h < 1:
        h = 1
    return int(x), int(y), int(w), int(h)


def build_track_roi(last_rect):
    margin_x = max(20, int(last_rect["w"] * 0.25))
    margin_y = max(20, int(last_rect["h"] * 0.25))
    roi_x = last_rect["x"] - margin_x
    roi_y = last_rect["y"] - margin_y
    roi_w = last_rect["w"] + margin_x * 2
    roi_h = last_rect["h"] + margin_y * 2
    return clamp_rect(roi_x, roi_y, roi_w, roi_h)


def rect_ratio_ok(rect_info):
    if rect_info["h"] == 0:
        return False
    ratio = rect_info["w"] / rect_info["h"]
    return MIN_ASPECT_RATIO <= ratio <= MAX_ASPECT_RATIO


def area_ratio_ok(rect_info, last_rect):
    if not last_rect:
        return True
    last_area = last_rect["area"]
    if last_area <= 0:
        return True
    ratio = rect_info["area"] / last_area
    return AREA_RATIO_MIN <= ratio <= AREA_RATIO_MAX


def distance_between(rect_info, last_rect):
    if not last_rect:
        return 0
    dx = rect_info["cx"] - last_rect["cx"]
    dy = rect_info["cy"] - last_rect["cy"]
    return math.sqrt(dx * dx + dy * dy)


def collect_candidates(search_img, roi, last_rect, is_global):
    candidates = []

    if roi is None:
        rects = search_img.find_rects(threshold=RECT_THRESHOLD)
    else:
        rects = search_img.find_rects(roi=roi, threshold=RECT_THRESHOLD)

    for rect in rects:
        if roi is None:
            offset_x = 0
            offset_y = 0
        else:
            offset_x = roi[0]
            offset_y = roi[1]
        rect_info = make_rect_info(rect, offset_x, offset_y)
        if rect_info["w"] > MAX_RECT_W or rect_info["h"] > MAX_RECT_H:
            continue
        if rect_info["w"] < MIN_RECT_W or rect_info["h"] < MIN_RECT_H:
            continue
        if not rect_ratio_ok(rect_info):
            continue
        if last_rect and not area_ratio_ok(rect_info, last_rect):
            continue
        distance = distance_between(rect_info, last_rect)
        rect_info["distance"] = distance
        if last_rect and (not is_global) and distance > MAX_TRACK_DISTANCE:
            continue
        candidates.append(rect_info)
    return candidates


def choose_candidate(candidates, last_rect):
    if not candidates:
        return None

    best = None
    best_score = None
    for rect_info in candidates:
        if not last_rect:
            score = rect_info["magnitude"] * 1000 + rect_info["area"]
        else:
            distance_score = max(0, MAX_TRACK_DISTANCE - min(rect_info["distance"], MAX_TRACK_DISTANCE))
            area_delta = abs(rect_info["area"] - last_rect["area"])
            area_score = max(0, last_rect["area"] - area_delta) if last_rect["area"] > 0 else rect_info["area"]
            score = distance_score * 1000000 + rect_info["magnitude"] * 1000 + area_score

        if best is None or score > best_score:
            best = rect_info
            best_score = score
            continue

        if score == best_score and last_rect and rect_info["distance"] < best["distance"]:
            best = rect_info
            best_score = score

    return best


def detect_rect(work_img, last_rect):
    if last_rect:
        roi_x, roi_y, roi_w, roi_h = build_track_roi(last_rect)
        local_candidates = collect_candidates(work_img, (roi_x, roi_y, roi_w, roi_h), last_rect, False)
        local_best = choose_candidate(local_candidates, last_rect)
        if local_best:
            return local_best

    global_candidates = collect_candidates(work_img, None, last_rect, True)
    global_best = choose_candidate(global_candidates, last_rect)
    if last_rect and global_best and global_best["distance"] > TRACK_REJECT_DISTANCE:
        return None
    return global_best


def smooth_value(last_value, current_value):
    if abs(current_value - last_value) <= CENTER_DEADBAND:
        return last_value
    return last_value * SMOOTH_KEEP + current_value * SMOOTH_NEW


def stabilize_rect(rect_info, last_rect):
    if not last_rect:
        return rect_info

    stable_rect = dict(rect_info)
    if abs(rect_info["x"] - last_rect["x"]) <= RECT_DEADBAND:
        stable_rect["x"] = last_rect["x"]
    if abs(rect_info["y"] - last_rect["y"]) <= RECT_DEADBAND:
        stable_rect["y"] = last_rect["y"]
    if abs(rect_info["w"] - last_rect["w"]) <= RECT_DEADBAND:
        stable_rect["w"] = last_rect["w"]
    if abs(rect_info["h"] - last_rect["h"]) <= RECT_DEADBAND:
        stable_rect["h"] = last_rect["h"]

    stable_rect["cx"] = stable_rect["x"] + stable_rect["w"] / 2
    stable_rect["cy"] = stable_rect["y"] + stable_rect["h"] / 2
    stable_rect["area"] = stable_rect["w"] * stable_rect["h"]
    return stable_rect


def draw_tracking(frame, rect_info, stable_cx, stable_cy, last_cx, last_cy):
    frame.draw_rectangle(rect_info["x"], rect_info["y"], rect_info["w"], rect_info["h"], color=255, thickness=2)
    for point in rect_info["corners"]:
        frame.draw_circle(point[0], point[1], 5, color=255, thickness=2, fill=False)
    frame.draw_circle(int(stable_cx), int(stable_cy), 4, color=255, thickness=2, fill=True)
    if last_cx is not None and last_cy is not None:
        frame.draw_line(int(last_cx), int(last_cy), int(rect_info["cx"]), int(rect_info["cy"]), color=255, thickness=1)


def main():
    uart = setup_uart()
    sensor = configure_sensor()
    clock = time.clock()
    last_rect = None
    last_cx = None
    last_cy = None
    stable_cx = None
    stable_cy = None
    lost_count = 0

    try:
        while True:
            clock.tick()
            os.exitpoint()
            frame = sensor.snapshot()
            work_img = frame.binary(THRESHOLD)

            rect_info = detect_rect(work_img, last_rect)

            if rect_info is None:
                if last_rect and stable_cx is not None and stable_cy is not None and lost_count < LOST_FRAME_LIMIT - 1:
                    lost_count += 1
                    print("hold rect:", lost_count)
                    send_center(uart, stable_cx, stable_cy)
                    draw_tracking(frame, last_rect, stable_cx, stable_cy, last_cx, last_cy)
                else:
                    print("no rect")
                    send_center(uart, DEFAULT_CX, DEFAULT_CY)
                    last_rect = None
                    last_cx = None
                    last_cy = None
                    stable_cx = None
                    stable_cy = None
                    lost_count = LOST_FRAME_LIMIT

                Display.show_image(frame)
                continue

            current_cx = rect_info["cx"]
            current_cy = rect_info["cy"]
            rect_info = stabilize_rect(rect_info, last_rect)
            current_cx = rect_info["cx"]
            current_cy = rect_info["cy"]

            if stable_cx is None or stable_cy is None:
                stable_cx = current_cx
                stable_cy = current_cy
            else:
                stable_cx = smooth_value(stable_cx, current_cx)
                stable_cy = smooth_value(stable_cy, current_cy)

            send_center(uart, stable_cx, stable_cy)
            draw_tracking(frame, rect_info, stable_cx, stable_cy, last_cx, last_cy)

            last_rect = rect_info
            last_cx = current_cx
            last_cy = current_cy
            lost_count = 0

            Display.show_image(frame)
    except KeyboardInterrupt:
        print("stopped")
    finally:
        try_call(sensor.stop)
        try_call(Display.deinit)
        try_call(MediaManager.deinit)
        try_call(uart.deinit)


main()
