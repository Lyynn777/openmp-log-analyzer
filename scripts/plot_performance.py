import matplotlib.pyplot as plt

# Threads tested
threads = [1, 2, 4, 8]

# Execution times (seconds) from measured / estimated data
time_seconds = [1.75, 0.88, 0.438, 0.32]

# Throughput in MB/s
file_size_MB = 119.21
throughput = [file_size_MB / t for t in time_seconds]

# Plot Execution Time vs Threads
plt.figure(figsize=(8,4))
plt.plot(threads, time_seconds, marker='o', label='Execution Time (s)')
plt.xlabel('Number of Threads')
plt.ylabel('Time (s)')
plt.title('OpenMP Log Analyzer Performance')
plt.xticks(threads)
plt.grid(True)
plt.legend()
plt.savefig("../data/performance_time.png")
plt.show()

# Plot Throughput vs Threads
plt.figure(figsize=(8,4))
plt.plot(threads, throughput, marker='o', color='green', label='Throughput (MB/s)')
plt.xlabel('Number of Threads')
plt.ylabel('Throughput (MB/s)')
plt.title('OpenMP Log Analyzer Throughput')
plt.xticks(threads)
plt.grid(True)
plt.legend()
plt.savefig("../data/performance_throughput.png")
plt.show()
