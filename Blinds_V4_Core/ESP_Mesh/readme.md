# ESP Mesh
## Overview
This lib will work together with 'PainlessMesh' and add functionality on top of it.
This will convert the central-less network into a master-slave type network aimed as a home-automation solution.\
This network will allow for very persistent packets as certain home-automation solutions may warrant extreme communication reliability.
## Protocol
In this protocol, the master is preset with all target slave devices ahead of time.\
Each device is identified using painless mesh's hardware chip ID'ing\
A beacon packet is sent out periodically by the master device to alert the target slave for a connection.\
The target slave device does not need to know the master id. This is known as a headless slave. A master device ID can be set but this will simply make it ignore all other master beacons if present.

### Simple Packet
A simple packet is sent without any acknowledgment or need for connection

### Persistent Packet
A persistent packet is designed to be stubborn and attempt to resend even between disconnects.
It should also prevent packet duplication with the use of packet ID's\
A persistent packet can be set with or with out a timeout(for when a packet becomes irrelevant)
A persistent packet will attempt to send in between reconnects but will be dropped if PID's mismatch.
(PID mismatch signals packet already received or device reset)

### Packet Buckets
Originally, the persistent packets would be organized into 'buckets' / a simple integer identifier for each packet.
Packets can be repackaged or canceled using these identifiers.\
A new version of packet buckets would act more as a shared variable between the master and slave.
This can be simple variables or entire structs. How read write operations are handled will need to be decided. Two write operations from both devices can clash leading to unknown states 
Updating configuration on a target receiver would no longer be a tedious mechanism of sending acknowledging and periodic checking.