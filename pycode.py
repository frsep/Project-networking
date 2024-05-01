import socket


class staions:
    def __init__(self, name, browser_port, query_port, *neighbour_query_ports):
        self.browser_port = browser_port
        self.name = name
        self.query_port = query_port
        neighbour_ports = []
        for i in neighbour_query_ports:
            self.neighbour_ports.append(i)
    def listen(self):
        s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        s.bind((socket.gethostname(), self.query_port))
        s.listen()
        while True:
            neighboursocket, address = s.accept()
            print(f"Connection from {address} has been established!")
            neighboursocket.send(bytes("Welcome to the server!", "utf-8"))
            neighboursocket.close()
    