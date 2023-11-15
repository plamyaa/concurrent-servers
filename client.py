import socket

def simple_client():
    address = "localhost";
    port = 8080;

    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        client_socket.connect((address, port))
        message = "Hello!"
        client_socket.sendall(message.encode('utf-8'))

        data = client_socket.recv(1024)
        print(f"Recieved {data.decode('utf-8')}")

    except Exception as e:
        print(f"Error: {e}")

    finally:
        client_socket.close()
        print("Connection closed")

if __name__ == "__main__":
    simple_client()

