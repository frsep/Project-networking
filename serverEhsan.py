import socket
import sys


class Staions:
    # def __init__(self, name, browser_port, query_port, *neighbour_query_ports):
    def __init__(self, name: str, browser_port: int):
        self.browser_port = browser_port
        self.name = name
        # self.query_port = query_port
        # neighbour_ports = []
        
        # for i in neighbour_query_ports:
        #     if i != None:
        #         self.neighbour_ports.append(i)
    
    def listen(self):
        # create a TCP socket 
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)   
        s.bind(('127.0.0.1', self.browser_port))
        s.listen(1)  
        while True:
            webbroswersocket, address = s.accept()
            print(f"Connection from {address} has been established!")
            http_response = "HTTP/1.1 200 OK\r\nContent-Length: 13\r\n\r\nHello, world!"
            webbroswersocket.sendall(http_response.encode('utf-8'))
            webbroswersocket.close()
    
# def main(name, browser_port, query_port, *neighbour_query_ports):
def main(name: str, browser_port: str):
    station = Staions(name, int(browser_port))
    station.listen()
    

if __name__ == "__main__":
    main(sys.argv[1],sys.argv[2])
