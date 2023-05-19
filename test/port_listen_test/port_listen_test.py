import socket

# Create a TCP/IP socket
sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

# Bind the socket to the address given on the command line
server_address = ('localhost', 8787)
sock.bind(server_address)
print(f'Starting up on {server_address[0]} port {server_address[1]}')

sock.listen(1)

while True:
    print('waiting for a connection')
    connection, client_address = sock.accept()
    try:
        print('client connected:', client_address)
        while True:
            data = connection.recv(16)
            print('received {!r}'.format(data))
            if data:
                # Send data back to the client
                connection.sendall(data)
            else:
                break
    finally:
        connection.close()
