import argparse
import socket
import sys
import threading

CHUNK_SIZE = 1024


class Client(object):
    def __init__(self, port, host):
        self.port = port
        self.host = host

    def send_file(self, f_name):
        sock = socket.socket()
        sock.connect((self.host, self.port))
        with open(f_name, 'rb') as f:
            while True:
                data = f.read(CHUNK_SIZE)
                if not data:
                    break
                sock.send(data)


class Server(object):
    def __init__(self, port=0, host='0.0.0.0'):
        self.host = host
        self.port = port

    def handle_conn(self, conn, addr, f_name):
        print("Connection from: {}".format(addr))
        with open(f_name, 'wb') as f:
            while True:
                data = conn.recv(CHUNK_SIZE)
                if not data:
                    break
                f.write(data)
        print("File written: {}".format(f_name))
        conn.close()

    def listen(self, f_name):
        s = socket.socket()
        s.bind((self.host, self.port))

        print("Server started on port: {}".format(s.getsockname()[1]))

        s.listen()
        while True:
            conn, addr = s.accept()
            listener = threading.Thread(target=self.handle_conn, args=(conn, addr, f_name))
            listener.start()


def main():
    args = parse_args(sys.argv[1:])

    if args.server:
        Server(args.port, args.host).listen(args.file)
    else:
        Client(args.port, args.host).send_file(args.file)


def parse_args(args):
    parser = argparse.ArgumentParser(description='Encrypt a file with AES encryption.')
    group = parser.add_mutually_exclusive_group(required=True)
    group.add_argument('-s', '--server', help='Flag that we encrypt the file.', action='store_true')
    group.add_argument('-c', '--client', help='Flag to send file.', action='store_true')
    parser.add_argument('-ho', '--host', help='IP of listener', default='0.0.0.0')
    parser.add_argument('-p', '--port', help='The port that the server is listening', type=int, default=0)
    parser.add_argument('-f', '--file',
                        help='If in client mode, sends the specified file. In server mode saves contents'
                             'to that file', required=True)
    return parser.parse_args(args)


if __name__ == '__main__':
    main()
