import sys
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QLineEdit, QPushButton, QGridLayout, QFrame, QSpinBox
)
from PyQt5.QtCore import QTimer
from EndPointFunctions import get_pin_designation, get_pin_values


class ESP32C3GUI(QMainWindow):
    def __init__(self):
        super().__init__()

        # API Variables
        self.base_url = None

        # Main Layout
        self.setWindowTitle("ESP32-C3 Get Pins")
        self.central_widget = QWidget()
        self.setCentralWidget(self.central_widget)

        self.main_layout = QVBoxLayout()
        self.central_widget.setLayout(self.main_layout)

        # Hostname/IP Input Layout
        self.hostname_layout = QHBoxLayout()
        self.main_layout.addLayout(self.hostname_layout)

        self.hostname_label = QLabel("Hostname/IP:")
        self.hostname_layout.addWidget(self.hostname_label)

        self.hostname_input = QLineEdit()
        self.hostname_layout.addWidget(self.hostname_input)

        self.connect_button = QPushButton("Connect")
        self.connect_button.clicked.connect(self.connect_to_device)
        self.hostname_layout.addWidget(self.connect_button)

        # Refresh Interval Input Layout
        self.refresh_layout = QHBoxLayout()
        self.main_layout.addLayout(self.refresh_layout)

        self.refresh_label = QLabel("Refresh Interval (ms):")
        self.refresh_layout.addWidget(self.refresh_label)

        self.refresh_input = QSpinBox()
        self.refresh_input.setRange(100, 10000)
        self.refresh_input.setValue(1000)  # Default refresh interval
        self.refresh_input.valueChanged.connect(self.update_refresh_interval)
        self.refresh_layout.addWidget(self.refresh_input)

        # Grid Layout for Pins
        self.pin_grid = QGridLayout()
        self.main_layout.addLayout(self.pin_grid)

        # Pin Definitions
        self.pin_labels = [
            "5V", "GND", "3V3", "4", "3", "2", "1", "0",  # First column
            "5", "6", "7", "8", "9", "10", "20", "21"   # Second column
        ]

        # Pin Widgets
        self.pin_widgets = {}
        for i, pin_label_text in enumerate(self.pin_labels):
            pin_frame = QFrame()
            pin_frame.setFrameShape(QFrame.Box)
            pin_frame.setLayout(QVBoxLayout())

            pin_label = QLabel(pin_label_text)
            pin_label.setStyleSheet("font-weight: bold;")
            pin_frame.layout().addWidget(pin_label)

            # For 3V3, 5V, and GND, no value label is added
            if pin_label_text not in ["3V3", "5V", "GND"]:
                pin_value = QLabel("N/A")  # Default value for other pins
                pin_frame.layout().addWidget(pin_value)
                self.pin_widgets[pin_label_text] = (pin_label, pin_value)
            else:
                self.pin_widgets[pin_label_text] = (pin_label, None)

            row = i % 8
            col = i // 8
            self.pin_grid.addWidget(pin_frame, row, col)

        # Timer for refreshing pin data
        self.timer = QTimer()
        self.timer.timeout.connect(self.update_pin_data)

    def connect_to_device(self):
        hostname = self.hostname_input.text().strip()
        if not hostname:
            self.statusBar().showMessage("Please enter a valid hostname or IP.", 5000)
            return

        self.base_url = f"http://{hostname}"
        self.update_pin_data()
        self.timer.start(self.refresh_input.value())

    def update_refresh_interval(self):
        self.timer.setInterval(self.refresh_input.value())

    def update_pin_data(self):
        if not self.base_url:
            return
    
        # Fetch pin designations
        try:
            pin_designation = get_pin_designation(self.base_url)
            if pin_designation is None:
                raise ValueError("Invalid pin designation response")
        except Exception as e:
            self.statusBar().showMessage(f"Failed to fetch pin designations: {e}", 5000)
            return
    
        # Fetch pin values
        try:
            pin_values = get_pin_values(self.base_url)
            if pin_values is None:
                raise ValueError("Invalid pin values response")
        except Exception as e:
            self.statusBar().showMessage(f"Failed to fetch pin values: {e}", 5000)
            return
    
        # Update UI
        pwm_pins = pin_designation.get("pwmPins", [])
        digital_pins = pin_designation.get("digitalPins", [])
        fast_led_pins = pin_designation.get("fastLedPins", [])
        reserved_pins = pin_designation.get("reservedPins", [])
        
        pwm_values = pin_values.get("pwmPins", [])
        digital_values = pin_values.get("digitalPins", [])
        fast_led_values = pin_values.get("fastLedPins", [])
        
        for pin_label_text, (pin_label, pin_value_label) in self.pin_widgets.items():
            # Skip updating static pins (3V3, 5V, and GND)
            if pin_label_text in ["3V3", "5V", "GND"]:
                continue
    
            pin_index = int(pin_label_text)
            designation = "N/A"
            value = "N/A"
    
            # Determine designation and value
            if pin_index in pwm_pins:
                designation = "PWM"
                value = pwm_values[str(pin_index)]
            elif pin_index in digital_pins:
                designation = "Digital"
                value = digital_values[str(pin_index)]
            elif pin_index in fast_led_pins:
                designation = "FastLED"
                value = 'tbd'
            elif pin_index in reserved_pins:
                designation = "Reserved pin"
                value = "N/A"
            # Update UI elements
            pin_label.setText(f"{pin_label_text} ({designation})")
            if pin_value_label:
                pin_value_label.setText(str(value))



if __name__ == "__main__":
    app = QApplication(sys.argv)
    gui = ESP32C3GUI()
    gui.show()
    sys.exit(app.exec_())