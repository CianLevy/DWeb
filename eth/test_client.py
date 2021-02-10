import socket

HOST = "localhost"
PORT = 3000


class TestClient:
    def __init__(self):
        self.sock = socket.socket(socket.AF_INET,  # Internet
                                  socket.SOCK_DGRAM)  # UDP

    def send_message(self, message) -> None:
        self.sock.sendto(message.encode(), (HOST, PORT))

    def recieve_message(self):
        data = self.sock.recv(1024)
        return data

    def run(self):
        input_line = None
        data_len = 10

        while input_line != 'exit':
            input_line = input('Enter request: ')

            if 'p' in input_line: # produce
                line = input_line.split(' ')
                self.send_message(f'set/test_client/{line[1]}/{line[1] * data_len}')

            elif 'c' in input_line: # consume
                line = input_line.split(' ')
                self.send_message('get/' + line[1])
                res = self.recieve_message()


if __name__ == "__main__":
    client = TestClient()
    client.run()
    
