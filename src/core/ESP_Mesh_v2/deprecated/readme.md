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

The highest level class will be the ```ESP_Mesh```\
This will be a single static instance and control all other connections dynamically\
The next level class will be ```ESP_Mesh_Connection``` which can assume the master or slave role.\
Only one instance can exist per ID. Each connection can easily be set up as a master or slave.

```ESP_Mesh``` will do the job of rerouting packets to the correct instance(s) whether an incoming or outgoing connection (incoming being slave device and outgoing being master device)

### Transmission Types:
#### Simple Packet
A simple packet is sent without any acknowledgment or need for connection, although by default they won't send without a connection

#### Persistent Packet
A persistent packet is designed to be stubborn and attempt to resend even between disconnects.
It should also prevent packet duplication with the use of packet ID's\
A persistent packet can be set with or with out a timeout(for when a packet becomes irrelevant)\
A persistent packet will attempt to send in between reconnects but will be dropped if PID's mismatch.
(ACK PID will always be sent out by the recipient signalling the packet has ben received) (PID mismatch signals transmission error) (Uninitialized PID means device reset)

A persistent packet can be setup with a timer to prevent overflowing the send buffer.\
It can also be set up with a group ID, to have more control over controlling different group controls

#### ~~Eternal Packet~~
~~This packet is similar to the Persistent Packet, but it will never give up sending until it receives a valid acknowledgment.~~

~~The major difference between an Eternal Packet and a Persistent Packet is that duplication protection cannot be guaranteed.~~

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
The only major sideeffect is unnecessary transmissions that only one device should be carrying out

Each JSON formatted packet will come with one (second one is assumed state) required field

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
  3 = Heartbeat Packet
  4 = PID realignment
  5 = simple packet
  6 = persistent packet
  7 = bucket request
  8 = bucket publish
  9 = packet ack
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
A negative value denotes a reset device, meaning no PID has been assigned. PID realignment will be needed.
The PID will wrap around an unsigned byte value 0-255

**Each device will track three values for PID:**

```
Local_PID -> Receiving PID
Remote_PID -> Remote device receiving PID
Transmit_PID -> Current transmitting PID
```

**NOTE:** Even though certain packets are denoted as "Only Slave sends" a master can also send this type of packet.\
This is only to clarify the role of the specific packet. After all, each master is just a slave with added functionality.

```
Beacon packet (info):
  Only master sends this packet. Just invitation packet. [Packet Command]

  {
    "type": 1
  }
```
```
Connection request (request):
  Only Slave sends this packet. [Packet Command][Expected slave PID]

  {
    "type": 2
    "pid" = int // negative = reset device. realignment required. otherwise, byte wrap around 0-255
    // this PID is the receive PID of the sender
  }
```
```
Connection request ack (response):
  Only Master Sends. ACK Packet. [Packet Command][Ack][Expected master PID]

  {
    "type": 2
    "ack": true
    "pid" = int // negative = reset device. all packet id's are valid. otherwise, byte wrap around 0-255
  }

  master assumes successful connection after this packet is sent
  slave assumes successful connection only after this packet is received
```
```
Heartbeat Packet (request):
  Only Master Sends. [Packet Command]

  {
    "type": 3
  }
```
```
Heartbeat Packet ack (response):
  Only Slave Sends. [Packet Command][Ack]

  {
    "type": 3
    "ack": true
  }
```
#### PID realignment
This is used when the Transmit PID outpaces the Remote PID to much.\
Before another packet can be safely sent.\
The remote PID must be set correctly otherwise the receiver might throw out new packets as duplicates

If Receiver gets a PID that is invalid (behind the actual remote PID), then this is an error.\
The correct ack is still sent out regardless.\
It will be assumed that the incorrectly received PID was simply delayed resulting in the wrong number.
```
PID realignment (request):
  Either Device Sends. [Packet Command]

  {
    "type": 4
    "pid": int // positive 0-255
    // This tells the receiver what to set receive-PID to
  }
```
```
PID realignment ack (response):
  Either Device Sends. [Packet Command][Ack][PID]

  {
    "type": 4
    "ack": true
    "pid": int
    // the pid must match the requested one
  }
```
```
Simple Packet (info):
  Either Device Sends. [Packet Command][PayLoad]

  {
    "type": 5
    "msg": String
  }
```
#### In the case of persistent packets:
Slave and Master will both implement a circular buffer of sorts.\
This buffer is not limited to 0-255 but not a lot is needed.\

This circular buffer will be split into two parts. For simplicity this will probably be two half's (128 and 128)

Each device will track three PID values:\
Transmit_PID\
Remote_PID\
Local_PID

The receiver will receive ANY PID from the first half of the buffer (relative to local PID), anything behind will be considered duplicate.\
When a new valid PID is received, the local PID will be automatically updated and pushed forward.\
If pushing forward of the local PID happens often enough, PID realignment may never be needed.

This is designed that every packet is required to have a unique PID.\
Its always safer to use the next PID than to realign and attempt to reuse an old PID because of possible packet delays

<img src="image.png" alt="alt text" height="200">

```
Persistent Packet  (expects packet ack)(info)(request):
  Either Device Sends. [Packet Command][PID][PayLoad]

  {
    "type": 6
    "pid": int // Transmit PID
    "msg": String
  }
```
#### Buckets
These type of packets will use the Exact same PID system to sync buckets.\
Each bucket will be given an unique ID.

#### TODO:
They will transmit exactly the same as persistent packets with two minor exceptions:
- all bucket packets (request and publish) will persist that packet even with misaligned or invalid PID's (indicating device reset)
- If PID's happen to mismatch when a bucket is published, an extra error field will be pushed to indicate sync failure
```
Bucket Request (expects packet ack)(request)
  Either Device Sends. [Packet Command][PID][Bucket ID][PayLoad]
  {
    "type": 7
    "pid": int // Transmit PID
    "bck": int // bucket ID
  }

// this command should only be used when a subscriber restarts as the publisher can't know that
```
```
Bucket publish (expects packet ack)(info)(request)
  Either Device Sends. [Packet Command][PID][Bucket ID][PayLoad]
  {
    "type": 8
    "pid": int // Transmit PID
    "bck": int // bucket ID
    "msg": String
  }
```

#### Packet Acknowledge
This is the general ack packet that should be used by the Persistent-Packet and both forms of the Bucket_Packets\
The PID returned is the exact that is received, even if the packet hasn't been applied due to duplication.
```
Packet ack (response):
  Either Device Sends. [Packet Command]

  {
    "type": 9
    "pid": int // the exact packet ID that is received
    // "ack": true // not necessary, since it has its own packet type
  }
```