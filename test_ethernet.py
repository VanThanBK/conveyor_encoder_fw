import sys
import time
import socket
from PySide6.QtWidgets import (
    QApplication, QWidget, QVBoxLayout, QLineEdit, QPushButton, QTextEdit, QLabel, QHBoxLayout
)
from PySide6.QtCore import Qt, QEvent

class EthernetClient(QWidget):
    """ PySide6 GUI for TCP Client Communication with an Ethernet Module """

    def __init__(self):
        super().__init__()
        self.init_ui()
        self.client_socket = None  # TCP Socket
        self.connected = False

        # Command History
        self.command_history = []
        self.history_index = -1  # Position in history (-1 means no history selected)

    def init_ui(self):
        """ Initialize GUI Components """
        self.setWindowTitle("Ethernet Module Communication")
        self.setGeometry(300, 300, 500, 300)

        layout = QVBoxLayout()

        # IP & Port Input
        self.ip_input = QLineEdit(self)
        self.ip_input.setPlaceholderText("Enter IP Address (e.g., 192.168.1.169)")
        self.ip_input.setText("192.168.1.169")  # Default IP

        self.port_input = QLineEdit(self)
        self.port_input.setPlaceholderText("Enter Port (e.g., 8844)")
        self.port_input.setText("8844")  # Default Port

        # Connect Button
        self.connect_button = QPushButton("Connect", self)
        self.connect_button.clicked.connect(self.connect_to_server)

        # Message Input & Send Button
        self.message_input = QLineEdit(self)
        self.message_input.setPlaceholderText("Enter command to send")
        self.message_input.returnPressed.connect(self.send_message)  # Detect Enter key
        self.message_input.installEventFilter(self)  # Enable key event handling for Up/Down arrow keys

        self.send_button = QPushButton("Send", self)
        self.send_button.clicked.connect(self.send_message)

        # Response Display
        self.response_area = QTextEdit(self)
        self.response_area.setReadOnly(True)
        self.response_area.setMinimumSize(480, 150)  # Prevents Negative Size Error

        # Layout Management
        ip_port_layout = QHBoxLayout()
        ip_port_layout.addWidget(QLabel("IP:"))
        ip_port_layout.addWidget(self.ip_input)
        ip_port_layout.addWidget(QLabel("Port:"))
        ip_port_layout.addWidget(self.port_input)
        ip_port_layout.addWidget(self.connect_button)

        message_layout = QHBoxLayout()
        message_layout.addWidget(self.message_input)
        message_layout.addWidget(self.send_button)

        layout.addLayout(ip_port_layout)
        layout.addLayout(message_layout)
        layout.addWidget(QLabel("Response:"))
        layout.addWidget(self.response_area)

        self.setLayout(layout)

    def eventFilter(self, obj, event):
        """ Handle Up/Down arrow key events for command history """
        if obj == self.message_input and event.type() == QEvent.KeyPress:
            if event.key() == Qt.Key_Up:
                self.show_previous_command()
                return True
            elif event.key() == Qt.Key_Down:
                self.show_next_command()
                return True
        return super().eventFilter(obj, event)

    def show_previous_command(self):
        """ Show the previous command in history """
        if self.command_history and self.history_index > 0:
            self.history_index -= 1
            self.message_input.setText(self.command_history[self.history_index])
        elif self.history_index == -1 and self.command_history:
            self.history_index = len(self.command_history) - 1
            self.message_input.setText(self.command_history[self.history_index])

    def show_next_command(self):
        """ Show the next command in history """
        if self.command_history and self.history_index < len(self.command_history) - 1:
            self.history_index += 1
            self.message_input.setText(self.command_history[self.history_index])
        else:
            self.history_index = -1
            self.message_input.clear()

    def connect_to_server(self):
        """ Connect to Ethernet Module via TCP """
        ip = self.ip_input.text().strip()
        port = self.port_input.text().strip()

        if not ip or not port.isdigit():
            self.response_area.append("⚠️ Invalid IP or Port!")
            return

        port = int(port)

        try:
            # Create a TCP/IP socket
            self.client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            self.client_socket.connect((ip, port))
            self.connected = True
            self.response_area.append(f"✅ Connected to {ip}:{port}")

            # Change button to Disconnect
            self.connect_button.setText("Disconnect")
            self.connect_button.clicked.disconnect()
            self.connect_button.clicked.connect(self.disconnect)

        except Exception as e:
            self.response_area.append(f"❌ Connection Failed: {e}")

    def send_message(self):
        """ Send a command to the Ethernet module """
        if not self.connected or not self.client_socket:
            self.response_area.append("⚠️ Not connected to a server!")
            return

        message = self.message_input.text().strip()
        if not message:
            self.response_area.append("⚠️ Message cannot be empty!")
            return

        # Ensure message ends with a newline
        if not message.endswith("\n"):
            message += "\n"

        try:
            # Send the message
            self.client_socket.sendall(message.encode())
            self.response_area.append(f"➡️ Sent: {message.strip()}")

            time.sleep(0.05)

            # Store command in history (if new and not duplicate)
            if not self.command_history or (self.command_history and self.command_history[-1] != message.strip()):
                self.command_history.append(message.strip())
                self.message_input.clear()

            # Keep only last 50 commands
            if len(self.command_history) > 50:
                self.command_history.pop(0)

            # Reset history index
            self.history_index = -1

            # Receive response
            response = self.client_socket.recv(1024).decode()
            self.response_area.append(f"⬅️ Received: {response.strip()}")

            # Clear input field after sending
            self.message_input.clear()

        except Exception as e:
            self.response_area.append(f"❌ Error sending message: {e}")

    def disconnect(self):
        """ Disconnect from the Ethernet module """
        if self.client_socket:
            self.client_socket.close()
            self.client_socket = None
        self.connected = False
        self.response_area.append("🔌 Disconnected.")

        # Change button back to Connect
        self.connect_button.setText("Connect")
        self.connect_button.clicked.disconnect()
        self.connect_button.clicked.connect(self.connect_to_server)

# Run Application
if __name__ == "__main__":
    app = QApplication(sys.argv)
    window = EthernetClient()
    window.show()
    sys.exit(app.exec())
