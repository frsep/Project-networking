import datetime
import socket
import sys
import cgi
import csv
import typing
import select


class Stations:
    # def __init__(self, name, browser_port, query_port, *neighbour_query_ports):
    def __init__(self, name: str, browser_port: int, query_port: int, neighbours: typing.List[str]):
        self.browser_port = browser_port
        self.name = name
        self.query_port = query_port
        self.neighbours = neighbours

        self.client_responses = {}  # To store responses corresponding to clients

        self.timetable = {}
        self.load_timetable()

    def load_timetable(self):
        timetable_file = f"script/tt-{self.name}"  # Goes into the scripts folder
        print(f"Opening file: {timetable_file}")
        with open(timetable_file, 'r') as file:
            reader = csv.reader(file)
            for row in reader:
                if not row or len(row) < 5 or row[0].startswith('#'):  # Skip empty lines and comments
                    continue
                departure_time = row[0]
                route_name = row[1]
                arrival_time = row[3]
                arrival_station = row[4]

                if arrival_station not in self.timetable:
                    self.timetable[arrival_station] = []
                self.timetable[arrival_station].append((departure_time, route_name, arrival_time))

    def best_route(self, destination):
        current_time = datetime.datetime.now().strftime('%H:%M')
        if destination not in self.timetable:
            return f"No route found from {self.name} to {destination}"

        available_departures = [(time, route) for time, route, _ in self.timetable[destination] if time >= current_time]
        if not available_departures:
            return f"No available departure times found from {self.name} to {destination}."

        available_departures.sort(key=lambda x: x[0])

        earliest_departure_time, route_name = available_departures[0]

        return f"<h2>The best route from {self.name} to {destination} is to to take {route_name} at {earliest_departure_time}<h2>"

    def listen(self):
        # Create a TCP socket
        tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        tcp_socket.bind(('127.0.0.1', int(self.browser_port)))
        tcp_socket.listen(5)

        # Create socket for UDP queries
        query_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        query_socket.bind(('127.0.0.1', int(self.query_port)))
        inputs = [tcp_socket, query_socket]
        outputs = []

        #  Here just as an example, attempts to send dgram to first neighbour
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        message = input("Enter message: ")
        address, port_str = self.neighbours[0].split(":")
        if message:  #  Can skip by pressing enter
            sock.sendto(message.encode(), (address, int(port_str)))
            print("Awaiting reply...")
            inputs.append(sock)  # Adds to the list of inputs select is listening to

        while inputs:
            readable, writable, exceptional = select.select(inputs, outputs, inputs)

            for s in readable:  # Something is trying to contact the server
                if s is tcp_socket:  # New webpage connection
                    # Accept new TCP connections
                    connection, address = s.accept()
                    print(f"Connection from {address} has been established!")

                    # Receive data from TCP client
                    request = connection.recv(1024).decode('utf-8')
                    print("Received request:")
                    print(request)

                    # Extract query string from the request
                    query_string = ""
                    if "?" in request:
                        query_string = request.split("?")[1].split()[0]  # Isolates destination from rest of the request

                    # Parse query string into a dictionary
                    form = cgi.FieldStorage(fp=None, environ={'QUERY_STRING': query_string})

                    # Get the 'to' parameter from the form data, can be indexed like a regular python dictionary
                    dest = form.getvalue('to', '')

                    # Prepare the HTTP response based on the 'to' parameter
                    if dest:
                        if self.best_route(dest)[0] != 'N':  # Checking if departure time found, should be more clear when best route updated for UDP
                            response_content = self.best_route(dest)
                            http_response = f"HTTP/1.1 200 OK\r\nContent-Length: {len(response_content)}\r\n\r\n{response_content}"
                            # Send the response to the client
                            connection.sendall(http_response.encode('utf-8'))
                        else:
                            # Send UDP requests at this point
                            # Store response content along with client socket
                            self.client_responses[connection] = dest
                    else:
                        response_content = "<h1>No destination parameter found!</h1>"
                        http_response = f"HTTP/1.1 200 OK\r\nContent-Length: {len(response_content)}\r\n\r\n{response_content}"
                        # Send the response to the client
                        connection.sendall(http_response.encode('utf-8'))

                else:
                    # Handle UDP datagrams
                    data, address = s.recvfrom(1024)

                    print(f"UDP datagram received from {address}: {data.decode('utf-8')}")
                    message = input("Reply? ")
                    if message:
                        s.sendto(message.encode(), address)

            for s in exceptional:
                inputs.remove(s)
                if s in outputs:
                    outputs.remove(s)
                s.close()

            # Send stored responses to the corresponding clients
            for client_socket in list(self.client_responses.keys()):
                # if other udp servers have all responded about this destination:
                print(f"Sending stored response!")
                response_content = self.client_responses.pop(client_socket)
                http_response = f"HTTP/1.1 200 OK\r\nContent-Length: {len(response_content)}\r\n\r\n{response_content}"
                client_socket.sendall(http_response.encode('utf-8'))


def main(name: str, browser_port: str, query_port: str, neighbours: typing.List[str]):
    station = Stations(name, int(browser_port), int(query_port), neighbours)
    station.listen()


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: ./station-server station-name browser-port query-port neighbour1 [neighbour2 ...]")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4:])
