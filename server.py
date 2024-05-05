import datetime
import socket
import sys
import cgi
import csv


class Stations:
    # def __init__(self, name, browser_port, query_port, *neighbour_query_ports):
    def __init__(self, name: str, browser_port: int):
        self.browser_port = browser_port
        self.name = name
        self.timetable = {}

        self.load_timetable()

        # self.query_port = query_port
        # neighbour_ports = []
        
        # for i in neighbour_query_ports:
        #     if i != None:
        #         self.neighbour_ports.append(i)

    def load_timetable(self):
        timetable_file = f"tt-{self.name}"
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
                response_content = self.best_route(dest)
            else:
                response_content = "<h1>No destination parameter found!</h1>"

            http_response = f"HTTP/1.1 200 OK\r\nContent-Length: {len(response_content)}\r\n\r\n{response_content}"

            # Send the response to the client
            webbroswersocket.sendall(http_response.encode('utf-8'))

            # Close the client connection
            webbroswersocket.close()


# def main(name, browser_port, query_port, *neighbour_query_ports):
def main(name: str, destination: str, browser_port: str):
    station = Stations(name, int(browser_port))
    station.listen()
    

if __name__ == "__main__":
    main(sys.argv[1], sys.argv[2], sys.argv[3])
