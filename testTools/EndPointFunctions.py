import requests
import json


#### 1. Pin Designation
def get_pin_designation(base_url):
    url = f"{base_url}/pinDesignation"
    try:
        response = requests.get(url)
        response.raise_for_status()
        return response.json()
    except requests.exceptions.RequestException as e:
        print(f"GET /pinDesignation failed: {e}")
        return None

def post_pin_designation(base_url, data):
    url = f"{base_url}/pinDesignation"
    try:
        response = requests.post(url, data={'body': json.dumps(data)})
        response.raise_for_status()
        return response.json()
    except requests.exceptions.RequestException as e:
        print(f"POST /pinDesignation failed: {e}")
        return None

#### 2. Pin Values
def get_pin_values(base_url):
    url = f"{base_url}/pinValues"
    try:
        response = requests.get(url)
        response.raise_for_status()
        return response.json()
    except requests.exceptions.RequestException as e:
        print(f"GET /pinValues failed: {e}")
        return None

def post_pin_values(base_url, data):
    url = f"{base_url}/pinValues"
    try:
        response = requests.post(url, data={'body': json.dumps(data)})
        response.raise_for_status()
        return response.json()
    except requests.exceptions.RequestException as e:
        print(f"POST /pinValues failed: {e}")
        return None

#### 3. Network Management
def get_network(base_url):
    url = f"{base_url}/network"
    try:
        response = requests.get(url)
        response.raise_for_status()
        return response.json()
    except requests.exceptions.RequestException as e:
        print(f"GET /network failed: {e}")
        return None

def post_network(base_url, data):
    url = f"{base_url}/network"
    try:
        response = requests.post(url, data={'body': json.dumps(data)})
        response.raise_for_status()
        return response.json()
    except requests.exceptions.RequestException as e:
        print(f"POST /network failed: {e}")
        return None

def delete_network(base_url, data):
    url = f"{base_url}/network"
    try:
        response = requests.delete(url, data={'body': json.dumps(data)})
        response.raise_for_status()
        return response.json()
    except requests.exceptions.RequestException as e:
        print(f"DELETE /network failed: {e}")
        return None

#### 4. Temporary Network Connection
def post_connect(base_url, data):
    url = f"{base_url}/connect"
    try:
        response = requests.post(url, data={'body': json.dumps(data)})
        response.raise_for_status()
        return response.json()
    except requests.exceptions.RequestException as e:
        print(f"POST /connect failed: {e}")
        return None

#### 5. Log Retrieval
def get_log(base_url, limit=None):
    url = f"{base_url}/log"
    if limit:
        url += f"?limit={limit}"
    try:
        response = requests.get(url)
        response.raise_for_status()
        return response.json()
    except requests.exceptions.RequestException as e:
        print(f"GET /log failed: {e}")
        return None

#### 6. Device Information
def get_device_info(base_url):
    url = f"{base_url}/device"
    try:
        response = requests.get(url)
        response.raise_for_status()
        return response.json()
    except requests.exceptions.RequestException as e:
        print(f"GET /device failed: {e}")
        return None

#### 7. Server Status
def get_server_status(base_url):
    url = f"{base_url}/test"
    try:
        response = requests.get(url)
        response.raise_for_status()
        return response.text
    except requests.exceptions.RequestException as e:
        print(f"GET /test failed: {e}")
        return None

#### Example Usage
if __name__ == "__main__":
    hostName = "esp32-controller"
    BASE_URL = f"http://{hostName}"

    # Example 1.1: Get pin designation
    print("Example 1.1: Get pin designation")
    print(get_pin_designation(BASE_URL))
    print()

    # Example 1.2: Post pin designation
    print("Example 1.2: Post pin designation")
    pin_designation_payload = {
        "pwmPins": [1, 2],
        "digitalPins": [3, 4],
        "fastLedPins": [5]
    }
    print(post_pin_designation(BASE_URL, pin_designation_payload))
    print()

    # Example 2.1: Get pin values
    print("Example 2.1: Get pin values")
    print(get_pin_values(BASE_URL))
    print()

    # Example 2.2: Post pin values
    print("Example 2.2: Post pin values")
    pin_values_payload = {
        "digital": {"7": 1, "9": 0},
        "pwm": {"5": 50},
        "fastLed": {"8": {"r": 255, "g": 100, "b": 50}}
    }
    print(post_pin_values(BASE_URL, pin_values_payload))
    print()

    # Example 3.1: Get stored networks
    print("Example 3.1: Get stored networks")
    print(get_network(BASE_URL))
    print()

    # Example 3.2: Add a network
    print("Example 3.2: Add a network")
    network_payload = {
        "ssid": "NewNetwork",
        "password": "password123",
        "isDefault": True
    }
    print(post_network(BASE_URL, network_payload))
    print()

    # Example 3.3: Delete a network
    print("Example 3.3: Delete a network")
    network_to_delete = {"ssid": "NewNetwork"}
    print(delete_network(BASE_URL, network_to_delete))
    print()

    # Example 4: Connect to a network temporarily
    print("Example 4: Connect to a network temporarily")
    connect_payload = {"ssid": "GuestWiFi"}
    print(post_connect(BASE_URL, connect_payload))
    print()

    # Example 5: Get logs
    print("Example 5: Get logs")
    print(get_log(BASE_URL, limit=5))
    print()

    # Example 6: Get device info
    print("Example 6: Get device info")
    print(get_device_info(BASE_URL))
    print()

    # Example 7: Check server status
    print("Example 7: Check server status")
    print(get_server_status(BASE_URL))
    print()
