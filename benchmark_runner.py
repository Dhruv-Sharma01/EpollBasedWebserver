import subprocess
import re
import time
import os
import csv

def run_benchmark(concurrency):
    print(f"Running benchmark with concurrency {concurrency}...")
    # Run for 5 seconds
    result = subprocess.run(
        ["./build/benchmark", "127.0.0.1", "8080", str(concurrency), "5"],
        capture_output=True, text=True
    )
    output = result.stdout
    
    rps = 0.0
    latency = 0.0
    
    rps_match = re.search(r"RPS: ([\d\.]+)", output)
    if rps_match:
        rps = float(rps_match.group(1))
        
    lat_match = re.search(r"Avg Latency \(ms\): ([\d\.]+)", output)
    if lat_match:
        latency = float(lat_match.group(1))
        
    return rps, latency

def main():
    # Ensure server is running
    print("Starting server...")
    server_process = subprocess.Popen(["./build/http_server"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
    time.sleep(2) # Wait for startup

    concurrencies = [10, 50, 100, 200, 500]
    results = []

    try:
        print(f"{'Concurrency':<15} {'RPS':<15} {'Avg Latency (ms)':<20}")
        print("-" * 50)
        for c in concurrencies:
            r, l = run_benchmark(c)
            results.append((c, r, l))
            print(f"{c:<15} {r:<15.2f} {l:<20.2f}")
    finally:
        server_process.terminate()
        server_process.wait()

    # Save to CSV
    with open('benchmark_results.csv', 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(['Concurrency', 'RPS', 'Avg Latency (ms)'])
        writer.writerows(results)
    
    print("\nResults saved to benchmark_results.csv")

if __name__ == "__main__":
    main()
