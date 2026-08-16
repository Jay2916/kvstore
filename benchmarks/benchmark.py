import subprocess
import threading
import time

CLIENTS = 10
WORKLOAD = 100
ADDR = "20.233.197.38"
PORT = "1234"

def run_client(client_id, seq):
    result = subprocess.run(
        [
            "./bench",
            "--in", seq,
            "-a", ADDR,
            "-p", PORT,
        ],
        capture_output=True,
        text=True
    )

    if result.returncode != 0:
        print(f"Client {client_id} failed:")
        print(result.stderr)

import random

def generate_workload(length, g_ratio, s_ratio, d_ratio):
    total_ratio = g_ratio + s_ratio + d_ratio

    if total_ratio <= 0:
        raise ValueError("Ratios must have a positive sum")

    g_ratio /= total_ratio
    s_ratio /= total_ratio
    d_ratio /= total_ratio

    g_count = round(length * g_ratio)
    s_count = round(length * s_ratio)
    d_count = length - g_count - s_count

    workload = (
        ["G"] * g_count +
        ["S"] * s_count +
        ["D"] * d_count
    )

    random.shuffle(workload)

    return "".join(workload)


def main():
    threads = []

    # Create all threads

    for i in range(CLIENTS):
        seq = generate_workload(WORKLOAD, 1, 1, 1)
        thread = threading.Thread(
            target=run_client,
            args=(i, seq)
        )
        threads.append(thread)

    # Start timing
    start = time.perf_counter_ns()

    # Start all clients
    for thread in threads:
        thread.start()

    # Wait for all clients
    for thread in threads:
        thread.join()

    end = time.perf_counter_ns()

    elapsed_ns = end - start

    print(f"Clients: {CLIENTS}")
    print(f"Workload per client: {WORKLOAD}")
    print(f"Total time: {elapsed_ns / 1_000_000:.6f} ms")
    secs = elapsed_ns / 1_000_000_000
    print(f"Total time: {secs:.6f} sec")
    print(f"Throughput: {(WORKLOAD*CLIENTS) / secs} /sec")


if __name__ == "__main__":
    main()