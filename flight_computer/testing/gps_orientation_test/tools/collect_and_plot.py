import argparse
import csv
import time
from pathlib import Path

import matplotlib.pyplot as plt
import serial


DATA_COLUMNS = [
    "millis",
    "pitch_deg",
    "roll_deg",
    "tilt_deg",
    "fix",
    "fix_quality",
    "sats_used",
    "sats_visible",
    "hdop",
    "avg_snr",
    "best_snr",
    "snr_count",
    "lat_deg",
    "lon_deg",
    "alt_m",
]


def to_float(value):
    if value == "":
        return None
    return float(value)


def read_rows(port, baud, seconds, output_csv):
    rows = []
    start_time = time.time()

    with serial.Serial(port, baud, timeout=2) as serial_port:
        with output_csv.open("w", newline="") as csv_file:
            writer = csv.DictWriter(csv_file, fieldnames=DATA_COLUMNS)
            writer.writeheader()

            print(f"Reading {port} at {baud} baud.")
            print("Move the antenna/board through orientations and hold each one for about 60 seconds.")
            print("Press Ctrl+C to stop early.")

            while True:
                if seconds is not None and time.time() - start_time >= seconds:
                    break

                line = serial_port.readline().decode(errors="ignore").strip()
                if not line.startswith("DATA,"):
                    if line:
                        print(line)
                    continue

                values = line.split(",")[1:]
                if len(values) != len(DATA_COLUMNS):
                    continue

                row = dict(zip(DATA_COLUMNS, values))
                writer.writerow(row)
                rows.append(row)

                print(
                    f"pitch={row['pitch_deg']} roll={row['roll_deg']} "
                    f"avg_snr={row['avg_snr']} sats={row['sats_visible']} fix={row['fix']}"
                )

    return rows


def make_plot(rows, metric, output_png):
    plot_rows = []

    for row in rows:
        pitch = to_float(row["pitch_deg"])
        roll = to_float(row["roll_deg"])
        value = to_float(row[metric])

        if pitch is not None and roll is not None and value is not None:
            plot_rows.append((pitch, roll, value))

    if not plot_rows:
        print("No plottable rows were collected.")
        return

    pitch_values = [row[0] for row in plot_rows]
    roll_values = [row[1] for row in plot_rows]
    metric_values = [row[2] for row in plot_rows]

    plt.figure(figsize=(9, 7))
    scatter = plt.scatter(
        roll_values,
        pitch_values,
        c=metric_values,
        cmap="viridis",
        s=55,
        edgecolors="black",
        linewidths=0.3,
    )
    plt.colorbar(scatter, label=metric)
    plt.xlabel("Roll angle (deg)")
    plt.ylabel("Pitch angle (deg)")
    plt.title(f"GPS antenna orientation hotspot map ({metric})")
    plt.grid(True, alpha=0.3)

    best_index = metric_values.index(max(metric_values))
    best_pitch, best_roll, best_value = plot_rows[best_index]
    plt.scatter([best_roll], [best_pitch], color="red", s=120, marker="x")
    plt.text(best_roll, best_pitch, f" best {best_value:.1f}", color="red")

    plt.tight_layout()
    plt.savefig(output_png, dpi=160)

    print(f"Saved plot: {output_png}")
    print(
        f"Best {metric}: {best_value:.1f} at "
        f"pitch={best_pitch:.2f} deg, roll={best_roll:.2f} deg"
    )


def main():
    parser = argparse.ArgumentParser(description="Collect GPS orientation test data and make a hotspot plot.")
    parser.add_argument("--port", required=True, help="Serial port, for example COM5 on Windows.")
    parser.add_argument("--baud", type=int, default=115200)
    parser.add_argument("--seconds", type=int, default=None, help="Optional collection time in seconds.")
    parser.add_argument("--metric", default="avg_snr", choices=["avg_snr", "best_snr", "sats_visible", "sats_used", "hdop"])
    parser.add_argument("--csv", default="gps_orientation_data.csv")
    parser.add_argument("--png", default="gps_orientation_hotspot.png")
    args = parser.parse_args()

    output_csv = Path(args.csv)
    output_png = Path(args.png)

    try:
        rows = read_rows(args.port, args.baud, args.seconds, output_csv)
    except KeyboardInterrupt:
        print("Stopped by user.")
        rows = []

    if not rows and output_csv.exists():
        with output_csv.open(newline="") as csv_file:
            rows = list(csv.DictReader(csv_file))

    print(f"Saved CSV: {output_csv}")
    make_plot(rows, args.metric, output_png)


if __name__ == "__main__":
    main()
