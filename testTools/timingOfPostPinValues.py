import requests
import time

ESP_IP = "192.168.1.100"  # replace with your ESP IP
JSON_URL = f"http://{ESP_IP}/pinValues"
BINARY_URL = f"http://{ESP_IP}/pinValuesBinary"

# --- Example commands ---

json_cmd = {
    "digital": {7: 1, 9: 0},
    "pwm": {5: 50},
    "fastLed": {8: {"r": 255, "g": 100, "b": 50}}
}

binary_cmd = bytearray([
    0xAA,       # header
    2, 7,1,9,0, # digital pins
    1, 5,50,    # PWM pins
    1, 8,255,100,50  # FastLED pins
])

# --- Test functions ---

def run_json_test():
    start = time.time()
    r = requests.post(JSON_URL, json=json_cmd)
    end = time.time()
    data = r.json()
    round_trip_ms = (end - start) * 1000
    esp_us = data.get("elapsed_us", None)
    return round_trip_ms, esp_us

def run_binary_test():
    start = time.time()
    r = requests.post(BINARY_URL, data=binary_cmd)
    end = time.time()
    data = r.json()
    round_trip_ms = (end - start) * 1000
    esp_us = data.get("elapsed_us", None)
    return round_trip_ms, esp_us

# --- Run multiple iterations ---
ITERATIONS = 20

json_times = []
json_esp = []
bin_times = []
bin_esp = []

for i in range(ITERATIONS):
    rt, esp = run_json_test()
    json_times.append(rt)
    if esp is not None:
        json_esp.append(esp)

    rt, esp = run_binary_test()
    bin_times.append(rt)
    if esp is not None:
        bin_esp.append(esp)

# --- Helper to summarize ---
def summarize(label, times, esp_times):
    print(f"--- {label} ---")
    print(f"Round-trip (ms): min={min(times):.2f}, max={max(times):.2f}, avg={sum(times)/len(times):.2f}")
    if esp_times:
        print(f"ESP pin update (µs): min={min(esp_times)}, max={max(esp_times)}, avg={sum(esp_times)//len(esp_times)}")
    print("")

# --- Print results ---
summarize("JSON", json_times, json_esp)
summarize("Binary", bin_times, bin_esp)
