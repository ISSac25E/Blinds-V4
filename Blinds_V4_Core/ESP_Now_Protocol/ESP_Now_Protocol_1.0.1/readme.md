/*
v1.0.1

changes from v1.0.0 to v1.0.1:
  - reorganize implementation of protocol
  - create dedicated files for different devices
  - finish package transfer implementation

Reliably connect to target device and send messages back and forth
There will be a dedicated master device and slave device. The master device will initiate and maintain the connection.
After connection, this will function as a normal full duplex communication

A packet can be sent with or without acknowledgment. This is determined by the sender.
If sent with acknowledgment, a packet id will be required.

Simple Packet:
  A simple packet is sent without any acknowledgment or need for connection

Persistent Packet:
  A persistent packet can be set with or with out a timeout(for when a packet becomes irrelevant)
  Within the program, the persistent packets will be organized into 'buckets' / a simple integer identifier for each packet.
  Packets can be repackaged or canceled using these identifiers
  A persistent packet will attempt to send in between reconnects but will be dropped if PID's mismatch (this indicates reset device or multi master system)
  (PID mismatch signals packet already received or device reset)

simple topology of communication:
  Master --> Beacon Packet (simple invitation to connect)
  Slave --> Connection Request (send current expected packet id [1-255, 0 if no packet id])
  Master --> Connection Response (Simple ack of connection request. Send Master's current expected packet id [1-255, 0 if no packet id])
  Master --> Keep Alive (ACK)
  Master --> Keep Alive (ACK)
  Master --> Keep Alive (ACK)

Sending Persistent packet topology:
  - get next packet in queue ready
  - check if connected, it not then wait
  - check if packet ID is aligned, it not then wait
  - If ID is aligned and receiver is connected, assign receiver's PID to packet(cannot be undone). If PID is 0, means first packet, set to 1
    Once a PID is linked to a packet, this PID cannot be changed as it can't be verified for packet duplication anymore
    If the receiver suddenly declares a different PID than the sending one, then assume packet was acknowledged. Go to next packet in queue and repeat

Packet Alignment Topology:
  Two passive actions are run together:
  - if misaligned, send alignment request every so often
  - if response is received, mark as aligned
Slave and Master will only accept the packet id that they are expecting.
Any other packet id will be considered duplicate.
A packet ACK will include the next expected packet id. This will be an opportunity to detect packet misalignment

each packet will follow these general guidelines:
  the first byte is the command byte:
  [00000000]

  The first 4 bits are the packet command:
    [0000xxxx]
    0 = null
    1 = beacon packet
    2 = connection request
    3 = null (was connection response)
    4 = PID realignment
    5 = Keep Alive Packet
    6 = simple packet
    7 = persistent packet

  The 7th bit is the ACK bit
    [x0000000]
    0 = normal/request packet
    1 = ack/response packet (this will also set the command bytes to signify which packet is being acknowledged)

  beacon packet[1]:
    only master send this packet. 1 byte, nothing else needed.
    [00000001]

  connection request[2]:
    This is only sent by the slave. 2 bytes. [Command packet][Expected slave PID]
    [00000010][PID]

  connection request ack[2]:
    This is only sent by the master. ACK Packet. 2 bytes. [Command packet][Expected master PID]
    [10000010][PID]

  PID realignment [4]:
    This can be sent by either device. 1 byte. [Command packet]
    [00000100]

  PID realignment ack[4]:
    This can be sent by either device. 2 byte. [Command packet][PID]
    [10000100][PID]

  Keep Alive Packet[5]:
    This is only sent by the master. 1 byte. [Command packet]
    [00000101]

  Keep Alive Packet ack[5]:
    This is only sent by the slave. ACK Packet. 1 byte. [Command packet]
    [10001001]

  simple packet [6]:
    This can be sent by either master or slave. minimume 2 bytes. [Command packet][simple PID][PayLoad]
    This PID is different. It can be any number. It expires fairly quickly. This is only meant to prevent duplication in the event of a Mesh System (to be developed)
    [00000110][PID][PayLoad]

  persistent packet [7]:
    This can be sent by either master or slave. minimume 2 bytes. [Command packet][PID][PayLoad]
    [00000111][PID][PayLoad]

  persistent packet ack[7]:
    This can be sent by either master or slave. 2 bytes. [Command packet][next expected PID (a packet is considered successfully sent once this number mismatches)]
    [10000111][PID]
*/