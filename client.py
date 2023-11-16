import socket
import logging
import time
from argparse import ArgumentParser
import threading

class ReadThread(threading.Thread):
    def __init__(self, name, client_socket):
        super().__init__()
        self.client_socket = client_socket
        self.name = name
        self.bufsize = 8 * 1024

    def run(self):
        fullbuf = b''
        while True:
            buf = self.client_socket.recv(self.bufsize)
            logging.info('{0} received {1}'.format(self.name, buf))
            fullbuf += buf
            if b'1111' in fullbuf:
                break

def make_connection(name, host, port):
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

    try:
        client_socket.connect((host, port))
        recv = client_socket.recv(1)
        if recv != b'*':
            logging.error('Error in recv. Did not receive *')
        logging.info('{0} connected and received: {1}'.format(name, recv))

        rthread = ReadThread(name, client_socket)
        rthread.start()

        s = b'^abc$de^abte$f'
        logging.info('{0} sending {1}'.format(name, s))
        client_socket.send(s)
        time.sleep(1.0)

        s = b'xyz^123'
        logging.info('{0} sending {1}'.format(name, s))
        client_socket.send(s)
        time.sleep(1.0)

        s = b'25$^ab0000$abab'
        logging.info('{0} sending {1}'.format(name, s))
        client_socket.send(s)
        time.sleep(0.2)

        rthread.join()
        client_socket.close()
        logging.info('{0} disconnecting'.format(name))

    except Exception as e:
        print(f"Error: {e}")

    finally:
        client_socket.close()
        logging.info('{0} disconnecting'.format(name))

def simple_client():
    argparser = ArgumentParser('Simple TCP client')
    argparser.add_argument('-host')
    argparser.add_argument('-port', type=int)
    argparser.add_argument('-n', '--num_concurrent', type=int, default=1)
    args = argparser.parse_args()

    logging.basicConfig(
        level=logging.DEBUG, 
        format='%(levelname)s:%(asctime)s:%(message)s')

    timer = time.time()
    connections = []

    for i in range(args.num_concurrent):
        name = 'conn{0}'.format(i)
        tconn = threading.Thread(
            target=make_connection, 
            args=(name, args.host, args.port))

        tconn.start()
        connections.append(tconn)

    for conn in connections:
        conn.join()

    print("Done: ", time.time() - timer)

if __name__ == "__main__":
    simple_client()
