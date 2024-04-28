#class for a staion that communicats to serounding stations through udp ports
class staions:
    def __init__(self, name, browser_port, query_port, *neighbour_query_ports):
        self.browser_port = browser_port
        self.name = name
        self.query_port = query_port
        neighbour_ports = []
        for i in neighbour_query_ports:
            self.neighbour_ports.append(i)