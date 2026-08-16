import subprocess
import random

ADDR = "20.233.197.38"
WORKLOAD = 1000


def generate_workload(length, g_ratio, s_ratio, d_ratio):
    total_ratio = g_ratio + s_ratio + d_ratio

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


seq = generate_workload(WORKLOAD, 1, 1, 1)

print("Workload length:", len(seq))
print("First 100:", seq[:100])

subprocess.run(
    [
        "./bench",
        "--in", seq,
        "-a", ADDR,
        "-p", "1234"
    ],
    timeout=30
)