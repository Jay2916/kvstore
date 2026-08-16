import subprocess
import threading
import time
import random
import csv
import matplotlib.pyplot as plt


# ============================================================
# Configuration
# ============================================================

MIN_CLIENTS = 10
MAX_CLIENTS = 20
CLIENT_STEP = 10

MIN_WORKLOAD = 10_000
MAX_WORKLOAD = 20_000
WORKLOAD_STEP = 10_000

ADDR = "localhost"
PORT = 1234

G_RATIO = 5
S_RATIO = 3
D_RATIO = 1

OUTPUT_CSV = "benchmark_results.csv"


# ============================================================
# Workload generation
# ============================================================

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


# ============================================================
# Run one client
# ============================================================

def run_client(client_id, seq):
    result = subprocess.run(
        [
            "./bench",
            "--in", seq,
            "-a", ADDR,
            "-p", str(PORT),
        ],
        capture_output=True,
        text=True
    )

    if result.returncode != 0:
        print(f"Client {client_id} failed:")
        print(result.stderr)

        return False

    return True


# ============================================================
# Run one benchmark configuration
# ============================================================

def run_benchmark(num_clients, workload_size):

    print(
        f"\nRunning: "
        f"clients={num_clients}, "
        f"workload={workload_size}"
    )

    threads = []

    # Generate workload for every client
    workloads = [
        generate_workload(
            workload_size,
            G_RATIO,
            S_RATIO,
            D_RATIO
        )
        for _ in range(num_clients)
    ]

    # Create threads
    for client_id in range(num_clients):

        thread = threading.Thread(
            target=run_client,
            args=(client_id, workloads[client_id])
        )

        threads.append(thread)

    # --------------------------------------------------------
    # Start timing
    # --------------------------------------------------------

    start = time.perf_counter_ns()

    for thread in threads:
        thread.start()

    # Wait for every client
    for thread in threads:
        thread.join()

    end = time.perf_counter_ns()

    # --------------------------------------------------------
    # Results
    # --------------------------------------------------------

    elapsed_ns = end - start

    elapsed_ms = elapsed_ns / 1_000_000
    elapsed_sec = elapsed_ns / 1_000_000_000

    total_operations = num_clients * workload_size

    throughput = total_operations / elapsed_sec

    print(
        f"Time: {elapsed_ms:.3f} ms | "
        f"Throughput: {throughput:.2f} ops/sec"
    )

    return {
        "clients": num_clients,
        "workload": workload_size,
        "time_ms": elapsed_ms,
        "throughput": throughput,
        "total_operations": total_operations
    }


# ============================================================
# Generate benchmark ranges
# ============================================================

def generate_range(min_value, max_value, step):

    if step <= 0:
        raise ValueError("Step must be greater than zero")

    values = list(range(min_value, max_value + 1, step))

    # Make sure max value is included even if the step
    # doesn't land exactly on it.
    if values[-1] != max_value:
        values.append(max_value)

    return values


# ============================================================
# Save results
# ============================================================

def save_csv(results):

    with open(OUTPUT_CSV, "w", newline="") as f:

        writer = csv.DictWriter(
            f,
            fieldnames=[
                "clients",
                "workload",
                "time_ms",
                "throughput",
                "total_operations"
            ]
        )

        writer.writeheader()
        writer.writerows(results)

    print(f"\nResults saved to {OUTPUT_CSV}")


# ============================================================
# Graphs
# ============================================================

def make_graphs(results):

    # --------------------------------------------------------
    # Graph 1: Time vs Clients
    # One line for every workload
    # --------------------------------------------------------

    workloads = sorted(
        set(r["workload"] for r in results)
    )

    plt.figure()

    for workload in workloads:

        subset = [
            r for r in results
            if r["workload"] == workload
        ]

        subset.sort(key=lambda r: r["clients"])

        clients = [
            r["clients"] for r in subset
        ]

        times = [
            r["time_ms"] for r in subset
        ]

        plt.plot(
            clients,
            times,
            marker="o",
            label=f"{workload:,} ops"
        )

    plt.xlabel("Number of clients")
    plt.ylabel("Total time (ms)")
    plt.title("KV Store Latency vs Number of Clients")
    plt.grid(True)
    plt.legend()

    plt.savefig("time_vs_clients.png", dpi=200)
    plt.show()

    # --------------------------------------------------------
    # Graph 2: Throughput vs Clients
    # --------------------------------------------------------

    plt.figure()

    for workload in workloads:

        subset = [
            r for r in results
            if r["workload"] == workload
        ]

        subset.sort(key=lambda r: r["clients"])

        clients = [
            r["clients"] for r in subset
        ]

        throughput = [
            r["throughput"] for r in subset
        ]

        plt.plot(
            clients,
            throughput,
            marker="o",
            label=f"{workload:,} ops"
        )

    plt.xlabel("Number of clients")
    plt.ylabel("Throughput (ops/sec)")
    plt.title("KV Store Throughput vs Number of Clients")
    plt.grid(True)
    plt.legend()

    plt.savefig("throughput_vs_clients.png", dpi=200)
    plt.show()

    # --------------------------------------------------------
    # Graph 3: Time vs Workload
    # One line for every client count
    # --------------------------------------------------------

    client_counts = sorted(
        set(r["clients"] for r in results)
    )

    plt.figure()

    for clients in client_counts:

        subset = [
            r for r in results
            if r["clients"] == clients
        ]

        subset.sort(key=lambda r: r["workload"])

        workloads_x = [
            r["workload"] for r in subset
        ]

        times = [
            r["time_ms"] for r in subset
        ]

        plt.plot(
            workloads_x,
            times,
            marker="o",
            label=f"{clients} clients"
        )

    plt.xlabel("Workload per client")
    plt.ylabel("Total time (ms)")
    plt.title("KV Store Time vs Workload")
    plt.grid(True)
    plt.legend()

    plt.savefig("time_vs_workload.png", dpi=200)
    plt.show()


# ============================================================
# Main
# ============================================================

def main():

    clients_range = generate_range(
        MIN_CLIENTS,
        MAX_CLIENTS,
        CLIENT_STEP
    )

    workload_range = generate_range(
        MIN_WORKLOAD,
        MAX_WORKLOAD,
        WORKLOAD_STEP
    )

    total_tests = (
        len(clients_range) *
        len(workload_range)
    )

    print("========================================")
    print("KV Store Benchmark")
    print("========================================")
    print(f"Clients:   {clients_range}")
    print(f"Workloads: {workload_range}")
    print(f"Total tests: {total_tests}")
    print("========================================")

    results = []

    test_number = 0

    for clients in clients_range:

        for workload in workload_range:

            test_number += 1

            print(
                f"\n[{test_number}/{total_tests}]"
            )

            result = run_benchmark(
                clients,
                workload
            )

            results.append(result)

    save_csv(results)

    make_graphs(results)


if __name__ == "__main__":
    main()