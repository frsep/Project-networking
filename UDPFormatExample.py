'''
------------------Format---------------------
Any message between servers will follow the same format. First, the main point is to separate different pieces of info
and describe what that information is. This is done by separating the type of info from its value with an '_'
    Ex. Variable_Value      # Capitalisation matters
Each variable or piece of information is seperated by a '\n' or newline
    ex. Variable_Value\nVariable2_ValueTwo

In the case of separating trip segments or legs of the trip, each segment is separated by a semicolon ';'
The format for each segment is:
    departure-time,route-name,departing-from,arrival-time,arrival-station   # Separated by commas, no spaces

Meaning the overall Data variable is laid out like:
    Data_departure-time,route-name,departing-from,arrival-time,arrival-station;departure-time2,route-name2,departing-from2,arrival-time2,arrival-station2
---------------------Names-------------------------
Two things need to be comunnicated:
1. That it is sending a name
2. Its name and query port
'Type_Name/nData_StationC;4006'
Note: I've put them into data so that the function that parses the message can do the same thing for all types. Open to
changing it if people have better ideas
---------------------Queries-----------------------
Two things need to be comunnicated:
1. That it is sending a Query
2. Its final destination
3. Its route up to this point
    # This gets appended to each time the query is sent to another station
    # It can also be used to make sure you don't forward a query on to a station that has already seen it
'Type_Query\nDestination_Station4\nData_10:00,Route 1,Station3,10:15,Route 1,Warwick-Stn;10:15,Route 1,Warwick-Stn,10:30,Perth-Stn;10:40,Route 2,Perth-Stn,11:00,Glendalough-Stn'

--------------------Responses----------------------
Four things need to be comunnicated:
1. that the message is a response
2. whether the server could succesfully find a route
    Success or Failure
3. Intended final destination - so the receiver can keep track of what query this response is related to
    Note: I'm keeping track of what servers are waiting for things and the response arrays using
    (destination, departure time leaving current station) because I think it's a reasonable unique key for each
    query. If people have better ideas let me know.
4. the route, either to send back to source if successful or to keep track of which queries have been answered
    Each station along the way back forwards it to the one before it on the route

Example showing a successful response of a trip from Station3 to Perth-Stn
'Type_Response
Result_Success
Destination_Perth-Stn
Data_10:00,Route 1,Station3,10:15,Route 1,Warwick-Stn;10:15,Route 1,Warwick-Stn,10:30,Perth-Stn'
'''

# Example data

response_data = '''Type_Query
Destination_Station4
Data_10:00,Route 1,Station3,10:15,Route 1,Warwick-Stn;10:15,Route 1,Warwick-Stn,10:30,Perth-Stn;10:40,Route 2,Perth-Stn,11:00,Glendalough-Stn;11:10,Route 3,Glendalough-Stn,11:30,Greenwood-Stn'''

# Parse the response
response = {}
for line in response_data.split('\n'):
    parts = line.split('_')
    key = parts[0].strip()
    value = parts[1].strip()
    response[key] = value

# Extract information from the response
msg_type = response["Type"]
if msg_type == "Query":
    query_dest = response["Destination"]
data = response["Data"]

print("Type:", msg_type)

# Parse the data part
segments = []
segment_data = data.split(';')
this_station = "Perth-Stn"
next_receiver = ""  # if it's not meant for me who should I send it to
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
    if arrival_station == this_station:
        if msg_type == "Query":
            # discard this query and do nothing, it has looped around
            # maybe should have query ID's to prevent queries looping? but could
            pass
        else:
            next_receiver = departing_from

# Extract information from the response data
print("\nSegments:")
for i, segment in enumerate(segments, start=1):
    print(f"\nSegment {i}:")
    print("Departure Time:", segment["departure_time"])
    print("Route Name:", segment["route_name"])
    print("Departing From:", segment["departing_from"])
    print("Arrival Time:", segment["arrival_time"])
    print("Arrival Station:", segment["arrival_station"])

if response["Type"] == "Query":
    pass
    # Checks timetable for trips to response["Destination"] if known,
    # else send to all neighbours except sender this came from and set up response array identified by source station
else:
    # Determine the source station
    original_station = segments[0]["departing_from"]
    print("\nOriginal Station:", original_station)
    # Determine the final station
    final_destination = segments[-1]["departing_from"]
    print("\nFinal Destination Station:", final_destination)
    # Determine where it goes next
    if original_station == this_station:
        # Compare it to other results in array of results
        print("\nNow Sending To: TCP Client")
    else:
        # Compare it to other results in array of results
        # If best result send to next server along the path back to source
        print("\nNow Sending To:", next_receiver)
