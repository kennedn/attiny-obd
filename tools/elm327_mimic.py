import serial
import time

# Define the device responses
responses = {
    b'ATZ\r': b'\r\rELM327 v1.5\r\r>',
    b'ATE0\r': b'OK\r\r>',
    b'ATH1\r': b'OK\r\r>',
    b'ATL0\r': b'OK\r\r>',
    b'AT RV\r': b'12.0V\r\r>',
    b'ATRV\r': b'12.0V\r\r>',
    b'ATSP0\r': b'OK\r\r>',
    b'ATDPN\r': b'A0\r\r>',
    b'ATTP6\r': b'OK\r\r>',
    b'0100\r': b'7E8 06 41 00 BE 33 45 20 42 38 20 31 33 20 \r>',
    b'0120\r': b'7E8 06 41 20 80 31 46 20 41 30 20 30 31 20 \r>',
    b'0140\r': b'7E8 06 41 40 FE 44 43 20 38 30 20 30 30 20 \r>',
    b'0600\r': b'7E8 06 46 00 C0 30 30 20 30 30 20 30 31 20 \r>',
    b'0620\r': b'7E8 06 46 20 80 30 30 20 30 30 20 30 31 20 \r>',
    b'0640\r': b'7E8 06 46 40 C0 30 30 20 30 30 20 30 31 20 \r>',
    b'0660\r': b'7E8 06 46 60 00 30 30 20 30 30 20 30 31 20 \r>',
    b'0680\r': b'7E8 06 46 80 00 30 30 20 30 30 20 30 31 20 \r>',
    b'06A0\r': b'7E8 06 46 A0 78 30 30 20 30 30 20 30 30 20 \r>',
    b'0105\r': b'7E8 03 41 05 49 \r>',
}

def read_until():
    eol = b'\r'
    line = b''
    while True:
        c = ser.read(1)
        if c:
            if c == b'\x00': 
                continue
            line += c
            if c == eol:
                break
        else:
            break
    return line


# Open the serial connection
ser = serial.Serial('/tmp/ttyV1', 38400, timeout=1)

# Read and send responses
while True:
    line = read_until()
    response = responses.get(line, b'ERROR\r\r>')
    print(f"{line}: {response}")
    ser.write(response)
    time.sleep(0.1)  # Wait for the response to be sent

