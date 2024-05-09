# Construct response data with multiple segments and specify the intermediate station
response_data = '''Type_Response
Data_10:00 AM,Route 1,Station3,10:15 AM,Route 1,Warwick-Stn;10:15 AM,Route 1,Warwick-Stn,10:30 AM,Perth-Stn;10:40 AM,Route 2,Perth-Stn,11:00 AM,Glendalough-Stn;11:10 AM,Route 3,Glendalough-Stn,11:30 AM,Greenwood-Stn'''

"""
response_data = '''Type_Query
Destination_Station4
Data_10:00 AM,Route 1,Station3,10:15 AM,Route 1,Warwick-Stn;10:15 AM,Route 1,Warwick-Stn,10:30 AM,Perth-Stn;10:40 AM,Route 2,Perth-Stn,11:00 AM,Glendalough-Stn;11:10 AM,Route 3,Glendalough-Stn,11:30 AM,Greenwood-Stn'''
"""
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


