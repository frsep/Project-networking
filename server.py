import socket
import sys
import cgi


class Stations:
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
        # Create a TCP socket
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.bind(('127.0.0.1', int(self.browser_port)))
        s.listen(1)

        while True:
            webbroswersocket, address = s.accept()
            print(f"Connection from {address} has been established!")

            # Receive data from the client
            request = webbroswersocket.recv(1024).decode('utf-8')
            print("Received request:")
            print(request)

            # Extract query string from the request
            query_string = ""
            if "?" in request:
                query_string = request.split("?")[1].split()[0]

            # Parse query string into a dictionary
            form = cgi.FieldStorage(fp=None, environ={'QUERY_STRING': query_string})

            # Get the 'to' parameter from the form data, can be indexed like a regular python dictionary
            dest = form.getvalue('to', '')

            # Prepare the HTTP response based on the 'to' parameter
            if dest:
                response_content = f"<h1>The best route from {self.name} to {dest} is:</h1>"
            else:
                response_content = "<h1>No destination parameter found!</h1>"

            http_response = f"HTTP/1.1 200 OK\r\nContent-Length: {len(response_content)}\r\n\r\n{response_content}"

            # Send the response to the client
            webbroswersocket.sendall(http_response.encode('utf-8'))

            # Close the client connection
            webbroswersocket.close()


# def main(name, browser_port, query_port, *neighbour_query_ports):
def main(name: str, browser_port: str):
    station = Stations(name, int(browser_port))
    station.listen()
    

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2])
