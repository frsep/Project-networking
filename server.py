import datetime
import socket
import sys
import cgi
import csv
import typing
import select

ACKNOWLEDGEMENTS = False


def parse_message(msg: str):
    try:
        print(msg)
        result = {}
        for line in msg.split('\n'):
            parts = line.split('_', 1)
            key = parts[0].strip()
            value = parts[1].strip()
            result[key] = value
        msg_type = result["Type"]
        data = result["Data"]
        if (msg_type != "Name") and (msg_type != "ACK"):
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

        print(result)
        return result
    except Exception as e:
        print(f"Error parsing message: {e}")
        return None


def best_response(response_array):
    # given list of responses from servers, return only the best response from the array
    best_route = None
    best_time = "23:59"
    for message in response_array:
        response = parse_message(message)
        if response["Result"] == "Success":
            if response["Segments"][-1]["arrival_time"] <= best_time:
                best_route = message
                best_time = response["Segments"][-1]["arrival_time"]
    return best_route


def send_udp_message(message, addrport):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.sendto(message.encode(), addrport)
            print(f"Sending: {message}")
    except Exception as e:
        print(f"Error sending message to server on port {addrport}: {e}")


class Stations:
    # def __init__(self, name, browser_port, query_port, *neighbour_query_ports):
    def __init__(self, name: str, browser_port: int, query_port: int, neighbours: typing.List[str]):
        self.browser_port = browser_port
        self.name = name
        self.query_port = query_port
        self.neighbours = neighbours
        self.neighbour_addresses = {}  # Dictionary with key = station name, value = address of server

        self.neighbour_queues = {}  # Dictionary with key = station address, value = [boolean ACK, queue of messages to be sent]

        self.servers_waiting = {}  # Dictionary with key = (destination, [stations up to (and including) this one]), value = name of station that queried
        self.responses = {}  # Dictionary with key = (destination, [stations up to (and including) this one]), value = response array

        self.client_connections = {}  # Dictionary with key = (destination, [stations up to (and including) this one]), value = socket

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

                if arrival_station not in self.neighbour_addresses:
                    self.neighbour_addresses[arrival_station] = None

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

    def add_to_queue(self, message, addrport):
        if ACKNOWLEDGEMENTS:
            self.neighbour_queues.setdefault(addrport, [False, []])
            print(f"Adding new message to queue: {message}")
            self.neighbour_queues[addrport][1].append(message)
        else:
            send_udp_message(message, addrport)

    def resend_pending_messages(self):
        for address, value in self.neighbour_queues.items():  # Iterates over each servers queue
            if value[0]:  # If ACK has been received
                print(f"Removing: {value[1][0]}")
                value[1].pop(0)  # Remove from the dict
                value[0] = False
            else:  # No corresponding ACK received
                if value[1]:
                    first_message = value[1][0]
                    print(f"Resending: {first_message}")
                    send_udp_message(first_message, address)

    def send_acknowledgment(self, message: dict):
        new_message = f"Type_ACK\nName_{self.name}\nData_" + str(message["Data"])
        sender = None
        if message["Type"] == "Name":
            sender, recv_port = message["Data"].split(';')
        elif message["Type"] == "Query":
            sender = message["Segments"][-1]["departing_from"]
        elif message["Type"] == "Response":
            for segment in message["Segments"]:
                if segment["departing_from"] == self.name:
                    sender = segment["arrival_station"]
        if sender:
            send_udp_message(new_message, self.neighbour_addresses[sender])

    def listen(self):
        try:
            # Create a TCP socket
            tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            tcp_socket.bind(('127.0.0.1', int(self.browser_port)))
            tcp_socket.listen(5)
        except socket.error as e:
            print(f"TCP socket could not be created: {e}")
            sys.exit(1)

        try:

            # Create socket for UDP queries
            query_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            query_socket.bind(('127.0.0.1', int(self.query_port)))
        except socket.error as e:
            print(f"UDP socket could not be created: {e}")
            tcp_socket.close()
            sys.exit(1)

        inputs = [tcp_socket, query_socket]
        outputs = []

        for neighbour in self.neighbours:
            self.send_name(neighbour)
        while inputs:
            self.resend_pending_messages()
            readable, writable, exceptional = select.select(inputs, outputs, inputs, 1)

            for s in readable:  # Something is trying to contact the server
                if s is tcp_socket:  # New webpage connection
                    self.handle_tcp_request(s)
                else:
                    # Handle UDP datagrams
                    data, address = s.recvfrom(1024)
                    message = parse_message(data.decode())
                    if (message["Type"] == "ACK") and ACKNOWLEDGEMENTS:
                        for address, value in self.neighbour_queues.items():
                            if address == self.neighbour_addresses[message["Name"]]:
                                for queue_item in value[1]:
                                    if message["Data"] == parse_message(queue_item)["Data"]:
                                        value[0] = True  # Set ACK to received
                                        print("RECEIVED ACK")

                    else:
                        if message["Type"] == "Name":
                            name, recv_port = message["Data"].split(';')
                            for neighbour in self.neighbours:
                                addr, port = neighbour.split(':')
                                if recv_port == port:
                                    if self.neighbour_addresses[name]:
                                        pass  # Already been seen and recorded
                                    else:
                                        self.neighbour_addresses[name] = (
                                            addr, int(port))  # Adds to list of active stations for ease of access
                                        self.send_name(neighbour)
                        elif message["Type"] == "Query":
                            self.handle_query(data.decode())
                        elif message["Type"] == "Response":
                            self.handle_response(data.decode())
                        print(f"UDP datagram received from {address}: {data.decode('utf-8')}")
                        if ACKNOWLEDGEMENTS:
                            self.send_acknowledgment(message)

            for key, response_array in list(self.responses.items()):
                num_responses = len(response_array)
                print(
                    f"key: {key}, response_array: {response_array}, num_responses: {num_responses}, num_neighbours: {len(self.neighbour_addresses)}")
                if num_responses >= len(self.neighbour_addresses):  # Response array is full, all servers responded
                    if key in self.client_connections.keys():  # Query about this destination at this time came from our client
                        print(f"Sending stored response to client!")
                        response_content = best_response(response_array)
                        if response_content:
                            response = f"<h2>{response_content}<h2>"
                            http_response = f"HTTP/1.1 200 OK\r\nContent-Length: {len(response)}\r\n\r\n{response}"
                            self.client_connections[key].sendall(http_response.encode('utf-8'))
                        else:  # No route found
                            print("No Route Found")
                            response_content = "No route found"
                            response = f"<h2>{response_content}<h2>"
                            http_response = f"HTTP/1.1 200 OK\r\nContent-Length: {len(response)}\r\n\r\n{response}"
                            self.client_connections[key].sendall(http_response.encode('utf-8'))
                        del self.client_connections[key]
                        del self.responses[key]
                elif (num_responses == len(
                        self.neighbour_addresses) - 1):  # Response array is full, all servers responded except for the one that asked
                    if key in self.servers_waiting.keys():  # Query came from a neighbouring server
                        print(f"Sending stored response to server!")
                        best_route = best_response(response_array)
                        if best_route:
                            self.forward_response(best_route)
                        else:
                            self.create_response(response_array[0],
                                                 self.neighbour_addresses[self.servers_waiting[key]], False)
                        del self.servers_waiting[key]
                        del self.responses[key]

    def send_name(self, neighbour):
        try:
            addr, port = neighbour.split(':')
            self.add_to_queue(f"Type_Name\nData_{self.name};{self.query_port}", (addr, int(port)))
        except Exception as e:
            print(f"Failed to send name to {neighbour}: {e}")

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
                response = f"<h2>{response_content}<h2>"
                http_response = f"HTTP/1.1 200 OK\r\nContent-Length: {len(response)}\r\n\r\n{response}"
                # Send the response to the client
                connection.sendall(http_response.encode('utf-8'))
            else:
                # Send UDP requests at this point
                sent = False
                for name, address in self.neighbour_addresses.items():  # Sends query out to all neighbours
                    if self.best_route(name, current_time):
                        self.add_to_queue(self.create_query(dest, name, current_time), address)
                        sent = True
                if sent:  # Query(s) successfully sent out
                    self.responses.setdefault((dest, tuple([self.name])), [])
                    self.client_connections.setdefault((dest, tuple([self.name])), None)
                    self.client_connections[(dest, tuple([self.name]))] = connection
                else:  # No routes to neighbours available (probably too late)
                    response_content = f"<h1>No route found to {dest}</h1>"
                    http_response = f"HTTP/1.1 200 OK\r\nContent-Length: {len(response_content)}\r\n\r\n{response_content}"
                    # Send the response to the client
                    connection.sendall(http_response.encode('utf-8'))

        else:
            response_content = "<h1>No destination parameter found!</h1>"
            http_response = f"HTTP/1.1 200 OK\r\nContent-Length: {len(response_content)}\r\n\r\n{response_content}"
            # Send the response to the client
            connection.sendall(http_response.encode('utf-8'))

    def create_query(self, final_destination, neighbour, current_time):
        try:
            return f"Type_Query\nDestination_{final_destination}\nData_{self.best_route(neighbour, current_time)}"
        except Exception as e:
            print(f"Error creating query: {e}")

    def handle_query(self, message):
        # Extract information from the query
        query = parse_message(message)
        if query is None:
            print("Failed to parse message")
            return
        destination = query["Destination"]
        data = query["Data"]
        segments = query["Segments"]

        # Determine the sending station
        last_station = segments[-1]["departing_from"]

        # Check if this station has already been visited in the query
        for segment in segments:
            if segment["departing_from"] == self.name:  # Station already queried
                # Respond with no route found
                self.create_response(message, self.neighbour_addresses[last_station], False)  # Respond Fail

        if self.best_route(destination,
                           segments[-1]["arrival_time"]):  # This station has a direct route to final destination
            self.create_response(message, self.neighbour_addresses[last_station], True)
            print("Found Route")
            # Respond by adding servers route to destination and return to sender
        else:
            print("Forwarding")
            self.forward_query(message)

    def forward_query(self, message):
        query = parse_message(message)
        if query is None:
            print("Failed to parse message")
            return
        try:
            exclude = [segment["departing_from"] for segment in query["Segments"]]  # Excludes stations visited already
            forwarded = False
            # send to all neighbors
            for name, address in self.neighbour_addresses.items():
                if name not in exclude:  # Skip over neighbours that have already seen this query
                    if self.best_route(name, query["Segments"][-1]["arrival_time"]):
                        new_message = message + f';{self.best_route(name, query["Segments"][-1]["arrival_time"])}'  # Add route to next query receiver
                        self.add_to_queue(new_message, address)
                        forwarded = True
            if forwarded:  # At least one query has been sent out, set up response arrays
                key_list = [segment["departing_from"] for segment in query["Segments"]]
                key_list.append(self.name)
                self.responses.setdefault((query["Destination"], tuple(key_list)), [])
                self.servers_waiting.setdefault((query["Destination"], tuple(key_list)),
                                                query["Segments"][-1]["departing_from"])
            if not forwarded:  # All neighbours have already been queried
                self.create_response(message, self.neighbour_addresses[query["Segments"][-1]["departing_from"]],
                                     False)  # Respond Fail
        except Exception as e:
            print(f"Error forwarding query: {e}")

    def create_response(self, message, address, result):
        response = parse_message(message)
        if response is None:
            print("Failed to parse message")
            return
        try:
            segments = response["Segments"]
            destination = response["Destination"]
            data = response["Data"]
        except Exception as e:
            print(f"Failed to create response: {e}")
            return
        print("Sending Response!")
        if result:
            new_response = str(data) + ";" + self.best_route(destination, segments[-1]["arrival_time"])
            self.add_to_queue(f"Type_Response\nResult_Success\nDestination_{destination}\nData_{new_response}",
                              address)
        else:
            self.add_to_queue(f"Type_Response\nResult_Fail\nDestination_{destination}\nData_{str(data)}", address)

    def handle_response(self, message):
        response = parse_message(message)
        if response is None:
            print("Failed to parse message")
            return
        try:
            key_list = []
            for segment in response["Segments"]:
                key_list.append(segment["departing_from"])
                if segment["departing_from"] == self.name:
                    break
            for segment in response["Segments"]:
                if segment["departing_from"] == self.name:
                    # Appends dictionary to the response array - to be dealt with in the main loop when full
                    if message not in self.responses[(response["Destination"], tuple(key_list))]:  # Make sure message isn't duplicate
                        self.responses[(response["Destination"], tuple(key_list))].append(message)
                    print(f"Key = {(response['Destination'], tuple(key_list))}")
        except Exception as e:
            print(f"Failed to handle response: {e}")

    def forward_response(self, message):
        response = parse_message(message)
        receiver = None
        for segment in response["Segments"]:
            if segment["arrival_station"] == self.name:
                receiver = segment["departing_from"]
        self.add_to_queue(message, self.neighbour_addresses[receiver])


def main(name: str, browser_port: str, query_port: str, neighbours: typing.List[str]):
    station = Stations(name, int(browser_port), int(query_port), neighbours)
    station.listen()


if __name__ == "__main__":
    if len(sys.argv) < 4:
        print("Usage: ./station-server station-name browser-port query-port neighbour1 [neighbour2 ...]")
        sys.exit(1)
    main(sys.argv[1], sys.argv[2], sys.argv[3], sys.argv[4:])
