import subprocess
import time
import matplotlib.pyplot as plt


def get_process_time(k: int,
                     tour: int,
                     score_first: int,
                     score_second: int,
                     exps: int,
                     threads: int,) -> float:
    t = time.time()
    subprocess.run(["./a.out",
                    f"{k}", f"{tour}", f"{score_first}", f"{score_second}", f"{exps}", f"{threads}"],
                    capture_output=True)
    return time.time() - t


xs = list(range(1, 13))
ys = [get_process_time(10, 0, 1, 1, 10_000_000, x) for x in xs]
print(ys)
plt.plot(xs, ys, "k", xs, ys, "bo")
plt.xticks(xs)
plt.ylabel("time")
plt.xlabel("thread")
plt.savefig("graph.svg")