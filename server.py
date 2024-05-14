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
        self.neighbour_names = {}  # Dictionary with key = station name, value = socket of server

        self.server_connections = {}  # Dictionary with key = (destination, departure from this station), value = socket of server that queried
        self.server_responses = {}  # Dictionary with key = (destination, departure from this station), value = response array

        self.client_connections = {}  # Dictionary with key = (destination, departure from this station), value = socket

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

                if arrival_station not in self.neighbour_names:
                    self.neighbour_names[arrival_station] = None

                if arrival_station not in self.timetable:
                    self.timetable[arrival_station] = []
                self.timetable[arrival_station].append((departure_time, route_name, arrival_time))

    def best_route(self, destination):
        current_time = datetime.datetime.now().strftime('%H:%M')
        if destination not in self.timetable:
            return None
            # return f"No route found from {self.name} to {destination}"

        available_departures = [(time, route, arrival) for time, route, arrival in self.timetable[destination] if
                                time >= current_time]
        if not available_departures:
            return None
            # return f"No available departure times found from {self.name} to {destination}."

        available_departures.sort(key=lambda x: x[0])

        earliest_departure_time, route_name, arrival_time = available_departures[0]
        return f"{earliest_departure_time},{route_name},{self.name},{arrival_time},{destination}"
        # return f"<h2>The best route from {self.name} to {destination} is to take {route_name} at {earliest_departure_time}<h2>"

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

        """        #  Here just as an example, attempts to send dgram to first neighbour
        sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        message = input("Enter message: ")
        address, port_str = self.neighbours[0].split(":")
        if message:  # Can skip by pressing enter
            sock.sendto(message.encode(), (address, int(port_str)))
            print("Awaiting reply...")"""

        self.send_name(self.neighbours[0])

        while inputs:
            readable, writable, exceptional = select.select(inputs, outputs, inputs)

            for s in readable:  # Something is trying to contact the server
                if s is tcp_socket:  # New webpage connection
                    self.handle_tcp_request(s)
                else:
                    # Handle UDP datagrams
                    data, address = s.recvfrom(1024)
                    message = self.parse_message(data.decode())
                    if message["Type"] == "Name":
                        name, recv_port = message["Data"].split(';')
                        for neighbour in self.neighbours:
                            addr, port = neighbour.split(':')
                            if recv_port == port:
                                if self.neighbour_names[name]:
                                    pass  # Already been seen and recorded
                                else:
                                    self.neighbour_names[name] = (
                                    addr, int(port))  # Adds to list of active stations for ease of access
                                    self.send_name(neighbour)
                    print(f"UDP datagram received from {address}: {data.decode('utf-8')}")

            for s in exceptional:
                inputs.remove(s)
                if s in outputs:
                    outputs.remove(s)
                s.close()

            for key, response_array in self.server_responses:
                num_responses = len(response_array)
                if num_responses == len(self.neighbours):  # Response array is full, all servers responded
                    if self.client_connections[key]:  # Query about this destination at this time came from our client
                        print(f"Sending stored response to client!")
                        response_content = self.best_response(response_array)
                        http_response = f"HTTP/1.1 200 OK\r\nContent-Length: {len(response_content)}\r\n\r\n{response_content}"
                        self.client_connections[key].sendall(http_response.encode('utf-8'))
                elif num_responses == len(
                        self.neighbours) - 1:  # Response array is full, all servers responded except for the one that asked
                    if self.server_connections[key]:  # Query about this destination at this time came from a neighbouring server
                        server_socket = self.server_connections[key]
                        print(f"Sending stored response to server!")
                        response_content = self.best_response(response_array)
                        self.forward_response(response_content, server_socket)

    def send_udp_message(self, message, addrport):
        try:
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
                s.sendto(message.encode(), addrport)
                print(f"Sending: {message}")
        except Exception as e:
            print(f"Error sending message to server on port {addrport}: {e}")

    def send_name(self, neighbour):
        addr, port = neighbour.split(':')
        self.send_udp_message(f"Type_Name\nData_{self.name};{self.query_port}", (addr, int(port)))

    def handle_tcp_request(self, tcp_socket):
        connection, address = tcp_socket.accept()
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
            if self.best_route(dest):
                response_content = self.best_route(dest)
                http_response = f"HTTP/1.1 200 OK\r\nContent-Length: {len(response_content)}\r\n\r\n{response_content}"
                # Send the response to the client
                connection.sendall(http_response.encode('utf-8'))
            else:
                # Send UDP requests at this point
                # Store response content along with client socket
                self.client_connections[dest, datetime.datetime.now().strftime('%H:%M')] = connection
        else:
            response_content = "<h1>No destination parameter found!</h1>"
            http_response = f"HTTP/1.1 200 OK\r\nContent-Length: {len(response_content)}\r\n\r\n{response_content}"
            # Send the response to the client
            connection.sendall(http_response.encode('utf-8'))

    def best_response(self, response_array):
        # given list of responses from servers, return only the best response from the array
        return "A"

    def create_query(self, final_destination, neighbour):
        query = f"Type_Query\nDestination_{final_destination}\nData_{self.best_route(neighbour)};"

    def handle_query(self, query: dict):
        # Extract information from the query

        destination = query["Destination"]
        data = query["Data"]

        # Parse the data part
        segments = query["Segments"]
        this_station = self.name

        # Check if this station has already been visited in the query
        for segment in segments:
            if segment["arrival_station"] == this_station:
                # Respond with no route found
                return f"No route found from {this_station} to {destination}"

        # Determine the source station
        last_station = segments[-1]["departing_from"]
        self.forward_query(data, last_station)

    def forward_query(self, query, last_station):
        # send to all neighbors
        for key, neighbour_socket in self.server_connections:
            pass
        pass

    def create_response(self):
        pass

    def handle_response(self):
        pass

    def forward_response(self, response, receiver_socket):
        pass

    def parse_message(self, msg):
        result = {}
        for line in msg.split('\n'):
            parts = line.split('_')
            key = parts[0].strip()
            value = parts[1].strip()
            result[key] = value
        msg_type = result["Type"]

        data = result["Data"]
        if msg_type == "Name":
            name, port = data.split(';')
            self.neighbour_names[name] = port
        else:
            segments = []
            segment_data = data.split(';')
            for segment_str in segment_data:
                segment_parts = segment_str.split(',')
                departure_time = segment_parts[0].strip()
                route_name = segment_parts[1].strip()
                departing_from = segment_parts[2].strip()
                arrival_time = segment_parts[3].strip()
                arrival_station = segment_parts[4].strip()
                segments.append({
                    'departure_time': departure_time,
                    'route_name': route_name,
                    'departing_from': departing_from,
                    'arrival_time': arrival_time,
                    'arrival_station': arrival_station
                })
                result["Segments"] = segments
        return result


def main(name: str, browser_port: str, query_port: str, neighbours: typing.List[str]):
    station = Stations(name, int(browser_port), int(query_port), neighbours)
    station.listen()


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: ./station-server station-name browser-port query-port neighbour1 [neighbour2 ...]")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4:])
