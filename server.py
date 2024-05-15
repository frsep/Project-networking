import datetime
import socket
import sys
import cgi
import csv
import typing
import select


def send_udp_message(message, addrport):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.sendto(message.encode(), addrport)
            print(f"Sending: {message}")
    except Exception as e:
        print(f"Error sending message to server on port {addrport}: {e}")


def parse_message(msg):
    result = {}
    for line in msg.split('\n'):
        parts = line.split('_')
        key = parts[0].strip()
        value = parts[1].strip()
        result[key] = value
    msg_type = result["Type"]

    data = result["Data"]
    if msg_type != "Name":
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


def best_response(response_array):
    # given list of responses from servers, return only the best response from the array
    best_route = None
    best_time = datetime.datetime.strptime("23:59", '%H:%M')
    for message in response_array:
        response = parse_message(message)
        if response["Result"] == "Success":
            if response["Segments"][-1]["arrival-time"] <= best_time:
                best_route = message
                best_time = response["Segments"][-1]["arrival-time"]
    return best_route


class Stations:
    # def __init__(self, name, browser_port, query_port, *neighbour_query_ports):
    def __init__(self, name: str, browser_port: int, query_port: int, neighbours: typing.List[str]):
        self.browser_port = browser_port
        self.name = name
        self.query_port = query_port
        self.neighbours = neighbours
        self.neighbour_names = {}  # Dictionary with key = station name, value = address of server

        self.servers_waiting = {}  # Dictionary with key = (destination, departure from this station), value = name of station that queried
        self.responses = {}  # Dictionary with key = (destination, departure from this station), value = response array

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

    def best_route(self, destination, current_time):
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
                    message = parse_message(data.decode())
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
                    elif message["Type"] == "Query":
                        self.handle_query(data.decode())
                    elif message["Type"] == "Response":
                        self.handle_response(data.decode())
                    print(f"UDP datagram received from {address}: {data.decode('utf-8')}")

            for s in exceptional:
                inputs.remove(s)
                if s in outputs:
                    outputs.remove(s)
                s.close()

            for key, response_array in self.responses.items():
                num_responses = len(response_array)
                if num_responses == len(self.neighbours):  # Response array is full, all servers responded
                    if self.client_connections[key]:  # Query about this destination at this time came from our client
                        print(f"Sending stored response to client!")
                        response_content = best_response(response_array)
                        if response_content:
                            http_response = f"HTTP/1.1 200 OK\r\nContent-Length: {len(response_content)}\r\n\r\n{response_content}"
                            self.client_connections[key].sendall(http_response.encode('utf-8'))
                        else:  # No route found
                            response_content = "No route found"
                            http_response = f"HTTP/1.1 200 OK\r\nContent-Length: {len(response_content)}\r\n\r\n{response_content}"
                            self.client_connections[key].sendall(http_response.encode('utf-8'))

                elif num_responses == len(
                        self.neighbours) - 1:  # Response array is full, all servers responded except for the one that asked
                    if self.servers_waiting[key]:  # Query came from a neighbouring server
                        print(f"Sending stored response to server!")
                        best_route = best_response(response_array)
                        if best_route:
                            self.create_response(best_response(response_array),
                                                 self.neighbour_names[self.servers_waiting[key]], True)
                        else:
                            self.create_response(response_array[0],
                                                 self.neighbour_names[self.servers_waiting[key]], True)

    def send_name(self, neighbour):
        addr, port = neighbour.split(':')
        send_udp_message(f"Type_Name\nData_{self.name};{self.query_port}", (addr, int(port)))

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
            # Set current time
            current_time = datetime.datetime.now().strftime('%H:%M')
            if self.best_route(dest, current_time):
                response_content = self.best_route(dest, current_time)
                http_response = f"HTTP/1.1 200 OK\r\nContent-Length: {len(response_content)}\r\n\r\n{response_content}"
                # Send the response to the client
                connection.sendall(http_response.encode('utf-8'))
            else:
                # Send UDP requests at this point
                for name, address in self.neighbour_names.items():  # Sends query out to all neighbours
                    send_udp_message(self.create_query(dest, name), address)
                    self.responses[dest, current_time][
                        0] = []
                    self.client_connections[dest, current_time][
                        0] = connection

        else:
            response_content = "<h1>No destination parameter found!</h1>"
            http_response = f"HTTP/1.1 200 OK\r\nContent-Length: {len(response_content)}\r\n\r\n{response_content}"
            # Send the response to the client
            connection.sendall(http_response.encode('utf-8'))

    def create_query(self, final_destination, neighbour):
        return f"Type_Query\nDestination_{final_destination}\nData_{self.best_route(neighbour, datetime.datetime.now().strftime('%H:%M'))};"

    def handle_query(self, message):
        # Extract information from the query
        query = parse_message(message)
        destination = query["Destination"]
        data = query["Data"]
        segments = query["Segments"]

        # Determine the sending station
        last_station = segments[-1]["departing_from"]

        # Check if this station has already been visited in the query
        for segment in segments:
            if segment["departing_from"] == self.name:  # Station already queried
                # Respond with no route found
                self.create_response(message, self.neighbour_names[last_station], False)  # Respond Fail

        if self.best_route(destination,
                           segments[-1]["arrival-time"]):  # This station has a direct route to final destination
            self.create_response(str(data) + "\n" + self.best_route(destination, segments[-1]["arrival-time"]),
                                 self.neighbour_names[last_station], True)
            # Respond by adding servers route to destination and return to sender
        else:
            self.forward_query(message)

    def forward_query(self, message):
        query = parse_message(message)
        exclude = [segment["departing-from"] for segment in query["Segments"]]
        forwarded = False
        # send to all neighbors
        for name, address in self.neighbour_names.items():
            if name not in exclude:  # Skip over neighbours that have already seen this query
                new_message = message + f';{self.best_route(name, datetime.datetime.strftime(query["Segments"][-1]["arrival-time"], "%H:%M"))}'  # Add route to next query receiver
                send_udp_message(new_message, address)
                forwarded = True
        if not forwarded:  # All neighbours have already been queried
            self.create_response(message, self.neighbour_names[query["Segments"][-1]["departing_from"]],
                                 False)  # Respond Fail

    def create_response(self, message, address, result):
        response = parse_message(message)
        segments = response["Segments"]
        destination = response["Destination"]
        new_response = str(response["Data"]) + "\n" + self.best_route(destination, segments[-1]["arrival-time"])
        if result:
            send_udp_message(f"Type_Response\nResult_Success\nData_{new_response};", address)
        else:
            send_udp_message(f"Type_Response\nResult_Fail\nData_{new_response};", address)

    def handle_response(self, message):
        response = parse_message(message)
        for segment in response["Segments"]:
            if segment["departing-from"] == self.name:
                # Appends dictionary to the response array - to be dealt with in the main loop when full
                self.responses[(response["Destination"], segment["departure-time"])].append(response)


def main(name: str, browser_port: str, query_port: str, neighbours: typing.List[str]):
    station = Stations(name, int(browser_port), int(query_port), neighbours)
    station.listen()


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: ./station-server station-name browser-port query-port neighbour1 [neighbour2 ...]")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4:])
