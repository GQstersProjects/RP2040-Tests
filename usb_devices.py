# Prints out all the usb devices

# Required: pyserial

import subprocess
import sys

def check_and_install_pyserial():
    try:
        import serial
    except ImportError:
        response = input("pyserial is not installed. Do you want to install it? (yes/no): ").strip().lower()
        if response == 'yes':
            subprocess.check_call([sys.executable, "-m", "pip", "install", "pyserial"])
            import serial
        else:
            print("pyserial is required to run this script.")
            sys.exit(1)

def serial_ports():
    import serial.tools.list_ports_windows
    ports = serial.tools.list_ports_windows.comports()
    for port, desc, hwid in sorted(ports):
        print("{}: {} [{}]".format(port, desc, hwid))

if __name__ == '__main__':
    check_and_install_pyserial()
    serial_ports()