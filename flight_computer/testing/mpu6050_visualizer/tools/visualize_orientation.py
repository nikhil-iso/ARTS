import argparse
import math

import matplotlib.animation as animation
import matplotlib.pyplot as plt
import numpy as np
import serial


orientation = {
    "pitch": 0.0,
    "roll": 0.0,
    "yaw": 0.0,
}


def rotation_matrix(pitch_deg, roll_deg, yaw_deg):
    pitch = math.radians(pitch_deg)
    roll = math.radians(roll_deg)
    yaw = math.radians(yaw_deg)

    rx = np.array(
        [
            [1, 0, 0],
            [0, math.cos(roll), -math.sin(roll)],
            [0, math.sin(roll), math.cos(roll)],
        ]
    )

    ry = np.array(
        [
            [math.cos(pitch), 0, math.sin(pitch)],
            [0, 1, 0],
            [-math.sin(pitch), 0, math.cos(pitch)],
        ]
    )

    rz = np.array(
        [
            [math.cos(yaw), -math.sin(yaw), 0],
            [math.sin(yaw), math.cos(yaw), 0],
            [0, 0, 1],
        ]
    )

    return rz @ ry @ rx


def read_latest_orientation(serial_port):
    while True:
        line = serial_port.readline().decode(errors="ignore").strip()
        if not line:
            break

        if not line.startswith("ORIENT,"):
            print(line)
            continue

        parts = line.split(",")
        if len(parts) < 5 or parts[1] == "millis":
            continue

        orientation["pitch"] = float(parts[2])
        orientation["roll"] = float(parts[3])
        orientation["yaw"] = float(parts[4])


def draw_vector(ax, start, end, color, label):
    ax.plot(
        [start[0], end[0]],
        [start[1], end[1]],
        [start[2], end[2]],
        color=color,
        linewidth=3,
        label=label,
    )


def main():
    parser = argparse.ArgumentParser(description="Live MPU6050 rocket orientation visualizer.")
    parser.add_argument("--port", required=True, help="Serial port, for example COM5 on Windows.")
    parser.add_argument("--baud", type=int, default=115200)
    args = parser.parse_args()

    serial_port = serial.Serial(args.port, args.baud, timeout=0)

    fig = plt.figure(figsize=(8, 8))
    ax = fig.add_subplot(111, projection="3d")

    def update(_frame):
        read_latest_orientation(serial_port)

        pitch = orientation["pitch"]
        roll = orientation["roll"]
        yaw = orientation["yaw"]

        r = rotation_matrix(pitch, roll, yaw)

        # The rocket body is drawn along local +Y. If your MPU6050 is mounted
        # differently, change this vector to match the rocket nose direction.
        nose = r @ np.array([0, 1.0, 0])
        tail = r @ np.array([0, -1.0, 0])
        right = r @ np.array([0.55, 0, 0])
        up = r @ np.array([0, 0, 0.55])

        ax.clear()
        ax.set_xlim(-1.2, 1.2)
        ax.set_ylim(-1.2, 1.2)
        ax.set_zlim(-1.2, 1.2)
        ax.set_xlabel("X")
        ax.set_ylabel("Y")
        ax.set_zlabel("Z")
        ax.set_title(f"Pitch {pitch:.1f} deg   Roll {roll:.1f} deg   Yaw {yaw:.1f} deg")

        draw_vector(ax, tail, nose, "red", "rocket body / nose")
        draw_vector(ax, np.zeros(3), right, "blue", "body X")
        draw_vector(ax, np.zeros(3), up, "green", "body Z")

        ax.legend(loc="upper left")

    live_animation = animation.FuncAnimation(fig, update, interval=50)
    plt.show()
    serial_port.close()

    # Keep the animation object alive until the plot window closes.
    return live_animation


if __name__ == "__main__":
    main()
