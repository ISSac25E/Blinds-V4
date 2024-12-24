# ESP Mesh v2

## Changes from v1
v1 focused on experimenting on a good design for reliable packet transfer using both espnow protocol and later upgraded to 'PainlessMesh'

This lib will work together with 'PainlessMesh' and add functionality on top of it and on top of v1 in two major ways

- This new version will no longer have a dedicated master or slave device.\
  The incoming and outgoing connections can be completely managed dynamically.\
  While each connection pair will still require at least one acting as a master, the actual roles don't matter for data transmission.
- This new version will also add the functionality of synced variables across connection pairs.\
  This feature will greatly simplify and optimize sharing states and config data between devices.\
  To remove the possibility of race conditions, each synced variable(group) will be read-only on the subscriber and write-only on the publisher

The highest level class will be the ```ESP_Mesh_MasterClass```\
This will be a single static instance and control all other connections dynamically\
The next level class will be ```ESP_Mesh_Connection``` which can assume the master or slave role.\
Only one instance can exist per ID. Each connection can easily be set up as a master or slave.

```ESP_Mesh_MasterClass``` will do the job of rerouting packets to the correct instance(s) whether an incoming or outgoing connection (incoming being slave device and outgoing being master device)

### Transmission Types:
#### Simple Packet
A simple packet is sent without any acknowledgment or need for connection, although by default they won't send without a connection

#### Persistent Packet
A persistent packet is designed to be stubborn and attempt to resend even between disconnects.
It should also prevent packet duplication with the use of packet ID's\
A persistent packet can be set with or with out a timeout(for when a packet becomes irrelevant)
A persistent packet will attempt to send in between reconnects but will be dropped if PID's mismatch.
(PID+1 signals packet already received) (PID mismatch signals transmission error) (Uninitialized PID means device reset)

A persistent packet can be setup with a timer to prevent overflowing the send buffer.

#### Eternal Packet
This packet is similar to the Persistent Packet, but it will never give up sending until it receives a valid acknowledgment.

The major difference between an Eternal Packet and a Persistent Packet is that duplication protection cannot be guaranteed.\

#### Buckets
This is a more complex type of communication that will be handled behind the scenes using a combination of previous packets.

It is designed to sync a common variable or structure between multiple devices.\
The variables will automatically resync on device reset or disconnect.

Although the ideal version of this is for both devices to have read-write capabilities, racing conditions prevent that from being simple.\
To ensure data integrity, each bucket will be one way only. The publisher can write-only, the subscriber can read-only.

This form will be designed to avoid needless retransmissions and yet be perfectly reliable.

Each "Bucket" will be identified using a unique id number. This number needs to be unique for all buckets relative to the target device.\
You can have the same bucket id for each set (published and subscribed buckets)

For each bucket, there is a subscriber and a publisher.\
The publisher owns the bucket and ultimately controls its state. The publisher also defines the bucket type and size.\
The subscriber is expected to know the type and size of the bucket ahead of time.

### Protocol

Each device(Master & Slave) will have a set of remote and local PID(Packet Identifier) for each connection pair(Master <-> Slave).\
The local PID is always accurate but the device will attempt to always keep the remote PID up to date.

Theoretically, both devices in a communication pair can be masters with no real issues.\
The only major sideeffect is unnecessary transmissions only one device should be carrying out

Each JSON formatted packet will come with two required fields

```
{
  "type": int,
  "ack": bool // this field is ignored if the type doesn't expect an ack, otherwise defaults to false
}
```
The 'ack' denotes whether the packet is acknowledging a previous request

These are all the packet types:
```
"type"
  0 = null
  1 = beacon packet
  2 = connection request
  3 = null
  4 = PID realignment << TODO: remove extra packet type
  5 = Heartbeat Packet / PID realignment
  6 = simple packet
  7 = persistent packet
  8 = eternal packet
```
Simple topology of communication:
```
  Master --> Beacon Packet (simple invitation to connect)
  Master --> Beacon Packet (simple invitation to connect)
  ...
  Master --> Beacon Packet (simple invitation to connect)
  Master --> Beacon Packet (simple invitation to connect)
  Slave --> Connection Request (send current expected packet id [0-255, -1 if no packet id])(does NOT require a beacon packet to initialize)
  Master --> Connection Request ack (Send Master's current expected packet id [0-255, -1 if no packet id])
  Master --> Heartbeat
  Slave --> Heartbeat ack
  Master --> Heartbeat
  Slave --> Heartbeat ack
  Master --> Heartbeat
  Slave --> Heartbeat ack
  ...
```

The PID sent will be a signed integer.
A negative value denotes a reset device, meaning no PID has been assigned. Any incoming packet will be valid.
The PID will wrap around an unsigned byte value 0-255

**NOTE:** Even though certain packets are denoted as "Only Slave sends" a master can also send this type of packet.\
This is only to clarify the role of the specific packet. After all, each master is just a slave with added functionality.

```
Beacon packet (info):
  Only master sends this packet. 1 byte, nothing else needed.

  {
    "type": 1
    // ack is assumed false
  }
```
```
Connection request (request):
  Only Slave sends this packet. [Command packet][Expected slave PID]

  {
    "type": 2
    "pid" = int // negative = reset device. all packet id's are valid. otherwise, byte wrap around 0-255
  }
```
```
Connection request ack (response):
  Only Master Sends. ACK Packet. [Command packet][Expected master PID]

  {
    "type": 2
    "ack": true
    "pid" = int // negative = reset device. all packet id's are valid. otherwise, byte wrap around 0-255
  }

  master assumes successful connection after this packet is sent
  slave assumes successful connection only after this packet is received
```
```
PID realignment (request):
  Either Device Sends. [Command packet]

  {
    "type": 4
  }
```
```
PID realignment ack (response):
  Either Device Sends. [Command packet][PID]

  {
    "type": 4
    "ack": true
  }
```
```
Heartbeat Packet (request):
  Only Master Sends. [Command packet][expected master PID]

  {
    "type": 5
    "pid": int
  }
  The [expected master PID] is completely redundant but used to make sure there is limited possibility for some sort of drift
```
```
Heartbeat Packet ack (response):
  Only Slave Sends. [Command packet][expected slave PID]

  {
    "type": 5
    "ack": true
    "pid": int
  }

  The [expected slave PID] is completely redundant but used to make sure there is limited possibility for some sort of drift
```
```
Simple Packet (info):
  Either Device Sends. [Command packet][PayLoad]

  {
    "type": 6
    "msg":
    {
      ...
    }
  }
```
**In the case of persistent packets:** Slave and Master will only accept the packet id that they are expecting (ie. the PID they are currently sending).
Any other packet id will be considered duplicate.
A packet ACK will include the NEXT expected packet id. This will be an opportunity to realign PID's.
```
Persistent Packet (info)(request):
  Either Device Sends. [Command packet][PID][PayLoad]

  {
    "type": 7
    "pid": int
    "msg":
    {
      ...
    }
  }
```
```
Persistent Packet ack (response):
  Either Device Sends. [Command packet]

  {
    "type": 8
    "pid": int // new expected PID
    "ack": true
  }
```
```
Eternal Packet (info)(request):
  Either Device Sends. [Command packet][PID][PayLoad]

  {
    "type": 8
    "pid": int
    "msg":
    {
      ...
    }
  }
```
```
Eternal Packet ack (response):
  Either Device Sends. [Command packet][PID]

  {
    "type": 8
    "pid": int // new expected PID
    "ack": true

    "bid": int // only for buckets (synced variables)
  }
```

While at first glace Eternal packets and Persistent packets looks exactly the same, the response and behavior of the packets differ.

Persistent packets will drop a packet if device reconnects before sending an acknowledge packet because duplication cannot be guaranteed.\
Eternal packets on the other hand will resync the PID and continue sending until an acknowledgment is received. Duplication is disregarded

TODO: make eternal packets accept any pid except PID - 1? Or no PID at all for Eternal packets?


For buckets (synced-variables) the receiver expects an extra ```"bid": int```\
A certain range of bid's will be general use but the remaining will be reserved for use as synced variables.