import sys
from PyQt5.QtWidgets import (
    QApplication, QMainWindow, QWidget, QVBoxLayout, QHBoxLayout,
    QLabel, QLineEdit, QPushButton, QGridLayout, QFrame, QComboBox, QSpinBox
)
from EndPointFunctions import get_pin_designation, get_pin_values, post_pin_values, post_pin_designation

class ESP32C3SetPinsGUI(QMainWindow):
    def __init__(self):
        super().__init__()

        # Debug parameter
        self.debug = True

        # API Variables
        self.base_url = None
        self.available_pins = set()
        self.reserved_pins = set()
        self.pin_values = {}

        # Main Layout
        self.setWindowTitle("ESP32-C3 Set Pins")
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

        # Global Pin Control
        self.global_layout = QHBoxLayout()
        self.main_layout.addLayout(self.global_layout)

        self.global_pin_label = QLabel("Set All Pins:")
        self.global_layout.addWidget(self.global_pin_label)

        self.global_pin_dropdown = QComboBox()
        self.global_pin_dropdown.addItems(["None", "PWM", "Digital", "FastLED"])
        self.global_layout.addWidget(self.global_pin_dropdown)

        self.global_pin_value = QSpinBox()
        self.global_pin_value.setRange(0, 255)
        self.global_layout.addWidget(self.global_pin_value)

        self.global_apply_button = QPushButton("Apply to All")
        self.global_apply_button.clicked.connect(self.apply_global_setting)
        self.global_layout.addWidget(self.global_apply_button)

        # Grid Layout for Pins
        self.pin_grid = QGridLayout()
        self.main_layout.addLayout(self.pin_grid)

        # Pin Definitions
        self.pin_labels = ["5V", "GND", "3V3", "4", "3", "2", "1", "0", "5", "6", "7", "8", "9", "10", "20", "21"]

        # Pin Controls
        self.pin_controls = {}
        for i, pin_label_text in enumerate(self.pin_labels):
            pin_frame = QFrame()
            pin_frame.setFrameShape(QFrame.Box)
            pin_frame.setLayout(QVBoxLayout())

            pin_label = QLabel(pin_label_text)
            pin_label.setStyleSheet("font-weight: bold;")
            pin_frame.layout().addWidget(pin_label)

            if pin_label_text not in ["5V", "GND", "3V3"]:
                pin_dropdown = QComboBox()
                pin_dropdown.addItems(["None", "PWM", "Digital", "FastLED"])
                pin_frame.layout().addWidget(pin_dropdown)

                pin_value_input = QSpinBox()
                pin_value_input.setRange(0, 255)
                pin_value_input.setEnabled(False)
                pin_frame.layout().addWidget(pin_value_input)

                pin_dropdown.currentTextChanged.connect(
                    lambda designation, input_field=pin_value_input: self.update_input_field(designation, input_field)
                )

                self.pin_controls[pin_label_text] = (pin_label, pin_dropdown, pin_value_input)
            else:
                self.pin_controls[pin_label_text] = (pin_label, None, None)

            row = i % 8
            col = i // 8
            self.pin_grid.addWidget(pin_frame, row, col)

        # Update Button
        self.update_button = QPushButton("Update")
        self.update_button.clicked.connect(self.update_pin_settings)
        self.main_layout.addWidget(self.update_button)

    def connect_to_device(self):
        hostname = self.hostname_input.text().strip()
        if not hostname:
            self.statusBar().showMessage("Please enter a valid hostname or IP.", 5000)
            return

        self.base_url = f"http://{hostname}"
        self.pin_designation = get_pin_designation(self.base_url)
        if self.pin_designation is None:
            self.statusBar().showMessage("Failed to obtain pin designations.", 5000)
            return

        print("Successfully obtained pinDesignation")
        self.available_pins = set(map(str, self.pin_designation.get("availablePins", [])))
        self.reserved_pins = set(map(str, self.pin_designation.get("reservedPins", [])))

        # Get initial pin values
        self.pin_values = get_pin_values(self.base_url) or {}
        print("Successfully obtained pin values", self.pin_values)

        self.update_pin_display()

    def update_pin_display(self):
        for pin_label_text, (pin_label, pin_dropdown, pin_value_input) in self.pin_controls.items():
            if pin_label_text in self.reserved_pins:
                pin_label.setStyleSheet("color: red; font-weight: bold;")
                if pin_dropdown:
                    pin_dropdown.setEnabled(False)
                    pin_dropdown.hide()
                if pin_value_input:
                    pin_value_input.setEnabled(False)
                    pin_value_input.hide()
            elif pin_label_text in self.available_pins:
                pin_label.setStyleSheet("color: green; font-weight: bold;")
                if pin_dropdown:
                    pin_dropdown.setEnabled(True)
                    pin_dropdown.show()
                    try:    
                        if int(pin_label.text()) in self.pin_designation['pwmPins']:
                            pin_dropdown.setCurrentText('PWM')
                        elif int(pin_label.text()) in self.pin_designation['digitalPins']:
                            pin_dropdown.setCurrentText('Digital')
                    except:
                        pass
                if pin_value_input:
                    pin_value_input.setEnabled(True)
                    pin_value_input.show()    
                    try:   
                        if int(pin_label.text()) in self.pin_designation['pwmPins']:
                            pin_value_input.setRange(0, 100)
                        elif int(pin_label.text()) in self.pin_designation['digitalPins']:    
                            pin_value_input.setRange(0, 1)
                    except:
                        pass
            if pin_value_input is not None and pin_label_text in self.pin_values:
                pin_value_input.setValue(self.pin_values[pin_label_text])
                
    def update_input_field(self, designation, input_field):
        if designation == "PWM":
            input_field.setEnabled(True)
            input_field.setRange(0, 100)
        elif designation == "Digital":
            input_field.setEnabled(True)
            input_field.setRange(0, 1)
        else:
            input_field.setEnabled(False)

    def apply_global_setting(self):
        designation = self.global_pin_dropdown.currentText()
        value = self.global_pin_value.value()

        for pin_label_text, (_, pin_dropdown, pin_value_input) in self.pin_controls.items():
            if pin_dropdown and pin_value_input and pin_label_text in self.available_pins:
                pin_dropdown.setCurrentText(designation)
                pin_value_input.setValue(value)

    # def update_pin_settings(self):
    #     if not self.base_url:
    #         self.statusBar().showMessage("Please connect to a device first.", 5000)
    #         return

    #     pin_values_payload = {}
    #     pwm = {}
    #     digital = {}
    #     fastled = {}

    #     for pin_label_text, (_, pin_dropdown, pin_value_input) in self.pin_controls.items():
    #         if pin_dropdown and pin_value_input and pin_label_text in self.available_pins:
    #             designation = pin_dropdown.currentText()
    #             value = pin_value_input.value()

    #             if designation  == "PWM":
    #                 pwm[pin_label_text] = value                
    #             elif designation  == "Digital":
    #                 digital[pin_label_text] = value   
    #             elif designation == "FastLED":
    #                 pin_values_payload[pin_label_text] = {"type": "WS2812", "numLeds": value}

    #     pin_values_payload['pwm'] = pwm
    #     pin_values_payload['digital'] = digital
    #     #pin_values_payload['fastled'] = fastled
        
    #     print("Pin Values Payload:", pin_values_payload)
    #     post_pin_values(self.base_url, pin_values_payload)
    #     self.statusBar().showMessage("Pin settings updated successfully.", 5000)

    def update_pin_settings(self):
        if not self.base_url:
            self.statusBar().showMessage("Please connect to a device first.", 5000)
            return
    
        # Construct the pin designation payload
        pin_designation_payload = {"pwmPins": [], "digitalPins": []}
    
        for pin_label_text, (_, pin_dropdown, _) in self.pin_controls.items():
            if pin_dropdown and pin_label_text in self.available_pins and pin_label_text not in self.reserved_pins:
                designation = pin_dropdown.currentText()
                if designation == "PWM":
                    pin_designation_payload["pwmPins"].append(int(pin_label_text))
                elif designation == "Digital":
                    pin_designation_payload["digitalPins"].append(int(pin_label_text))
    
        print("Posting Pin Designation Payload:", pin_designation_payload)
        response = post_pin_designation(self.base_url, pin_designation_payload)
        print(response)
        if response is None:
            self.statusBar().showMessage("Failed to update pin designation.", 5000)
            return
    
        # Construct the pin values payload
        pin_values_payload = {}
        pwm = {}
        digital = {}
    
        for pin_label_text, (_, pin_dropdown, pin_value_input) in self.pin_controls.items():
            if pin_dropdown and pin_value_input and pin_label_text in self.available_pins and pin_label_text not in self.reserved_pins:
                designation = pin_dropdown.currentText()
                value = pin_value_input.value()
    
                if designation == "PWM":
                    pwm[pin_label_text] = value
                elif designation == "Digital":
                    digital[pin_label_text] = value
    
        pin_values_payload['pwm'] = pwm
        pin_values_payload['digital'] = digital
    
        print("Posting Pin Values Payload:", pin_values_payload)
        response = post_pin_values(self.base_url, pin_values_payload)
        print(response)
        

if __name__ == "__main__":
    app = QApplication(sys.argv)
    gui = ESP32C3SetPinsGUI()
    gui.show()
    sys.exit(app.exec_())
