# ESP Mesh v2

## Changes from v1
v1 focused on experimenting on a good design for reliable packet transfer using both espnow protocol and later upgraded to 'PainlessMesh'


This lib will work together with 'PainlessMesh' and add functionality on top of it.
This will convert the central-less network into a master-slave type network aimed as a home-automation solution.\
This network will allow for very persistent packets as certain home-automation solutions may warrant extreme communication reliability.

v2 will focus on implementing a robust json protocol that implements everything learned in v1.

v2 will also implement a shared-variable system for easy communication between devices

## Implementation
In this protocol, the master is preset with all target slave devices ahead of time.\
Each device is identified using painless mesh's hardware chip ID'ing\
A beacon packet is sent out periodically by the master device to alert the target slave for a connection.\
The target slave device does not need to know the master id. This is known as a headless slave.

TODO: implement static slave mode? This will benefit in not needing a beacon packet

### Transmission Types:
#### Simple Packet
A simple packet is sent without any acknowledgment or need for connection

#### Persistent Packet
A persistent packet is designed to be stubborn and attempt to resend even between disconnects.
It should also prevent packet duplication with the use of packet ID's\
A persistent packet can be set with or with out a timeout(for when a packet becomes irrelevant)
A persistent packet will attempt to send in between reconnects but will be dropped if PID's mismatch.
(PID mismatch signals packet already received) (Invalid PID means device reset)

A persistent packet can be setup with a timer to prevent overflowing the send buffer.

#### Eternal Packet
This packet is similar to the Persistent Packet, but it will never give up sending until it receives a valid acknowledgment.

The major difference between an Eternal Packet and a Persistent Packet is that duplication protection cannot be guaranteed.\
TODO: implement timeout for this packet?

#### Buckets
This is a more complex type of communication that will be handled behind the scenes using a combination of all previous packets.

It is designed to sync a common variable or structure between multiple devices.\
The variables will automatically resync on device reset or disconnect.

This form will be designed to avoid needless retransmissions and yet be perfectly reliable.

Each "Bucket" will be identified using a unique id number. This number needs to be unique for all buckets relative to the target device.

For each bucket, there is a subscriber and a publisher.\
The publisher owns the bucket and ultimately controls its state and read/write flags. The publisher also defines the bucket type and size.\
The subscriber is expected to know the type and size of the bucket ahead of time.

### Protocol

Each device(Master & Slave) will have a set of remote and local PID(Packet Identifier) for each connection pair(Master <-> Slave).\
The local PID is always accurate but the device will attempt to always keep the remote PID up to date.


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
  4 = PID realignment
  5 = Heartbeat Packet
  6 = simple packet
  7 = persistent packet
  8 = eternal packet
```
Simple topology of communication:
```
  Master --> Beacon Packet (simple invitation to connect)
  Slave --> Connection Request (send current expected packet id [1-255, 0 if no packet id])(does NOT require a beacon packet to initialize)
  Master --> Connection Response (ack of connection request. Send Master's current expected packet id [1-255, 0 if no packet id])
  Master --> Keep Alive (ACK)
  Master --> Keep Alive (ACK)
  Master --> Keep Alive (ACK)
```
Slave and Master will only accept the packet id that they are expecting (ie. the PID they are currently sending).
Any other packet id will be considered duplicate.
A packet ACK will include the NEXT expected packet id. This will be an opportunity to realign PID's

The PID sent will be a signed integer.
A negative value denotes a reset device, meaning no PID has been assigned. Any incoming packet will be valid.
The PID will wrap around an unsigned byte value 0-255

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
  Only Master Sends. [Command packet]

  {
    "type": 5
  }
```
```
Heartbeat Packet ack (response):
  Only Slave Sends. [Command packet][expected slave PID]

  {
    "type": 5
    "ack": true
    "pid": int
  }
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
  }
```

While at first glace Eternal packets and Persistent packets looks exactly the same, the response and behavior of the packets differ.

Persistent packets will drop a packet if device reconnects before sending an acknowledge packet because duplication cannot be guaranteed.\
Eternal packets on the other hand will resync the PID and continue sending until an acknowledgment is received. Duplication is disregarded

TODO: make eternal packets accept any pid except PID - 1? Or no PID at all for Eternal packets?