import subprocess
import time
import matplotlib.pyplot as plt


def get_process_time(n_threads: int) -> float:
    t = time.time()
    subprocess.run(["./main.out", f"{n_threads}"], capture_output=True)
    return time.time() - t


def get_average_process_time(n_threads: int, n_iter: int) -> float:
    return sum([get_process_time(n_threads) for _ in range(n_iter)]) / n_iter


xs = list(range(1, 20))
ys = [get_average_process_time(x, 10) for x in xs]
print(ys)
plt.plot(xs, ys, "k", xs, ys, "bo")
plt.xticks(xs)
plt.ylabel("execution time")
plt.xlabel("number of thread")
plt.savefig("graph.svg")
