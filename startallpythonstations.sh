python3 server.py StationA 4001 4002 localhost:4010 &
python3 server.py TerminalB 4003 4004 localhost:4006 &
python3 server.py JunctionC 4005 4006 localhost:4004 localhost:4010 &
python3 server.py BusportD 4007 4008 localhost:4010 localhost:4012 &
python3 server.py StationE 4009 4010 localhost:4002 localhost:4006 localhost:4008 &
python3 server.py TerminalF 4011 4012 localhost:4008 &
