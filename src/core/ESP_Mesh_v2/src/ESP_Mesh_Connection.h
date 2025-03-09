#ifndef ESP_Mesh_Connection_h
#define ESP_Mesh_Connection_h

#include <Arduino.h>
#include <painlessMesh.h>
#include <ArduinoJson.h>

// local dependencies
#include "../../LinkedList/LinkedList.h"

// internal dependencies
#include "ESP_Mesh_Utility.h"

/*
  a connection instance.
  each instance can be setup as a master or slave device
*/
class ESP_Mesh_Connection
{
  friend class ESP_Mesh;
  struct published_bucket_struct; // forward declaration

public:
  /*
    This is where the behavior of the connection will be determined
    The connection will be setup as a master or slave device
  */
  ESP_Mesh_Connection(uint32_t node_id, void (*recvCallback)(const char *, ESP_Mesh_Connection *));

  /*
    not really required since no fancy pointers are used.
    the linked lists can destroy themselves
  */
  // ~ESP_Mesh_Connection();

  /*
    send persistent packet.
    input a null-terminated character buffer

    TODO: include group values?
    add a timeout for when packet context becomes irelevent. timeout = 0 will indicate no timeout
  */
  bool sendPersistentPacket(const char *, uint8_t packetGroup, uint32_t timeout_ms);

  /*
    send simple packet.
    input a null-terminated character buffer

    this will send the packet once with no ack
  */
  bool sendSimplePacket(const char *);

  /*
    subscribe to a bucket id
    buffer for the bucket is managed internally
  */
  void bucket_subscribe(uint8_t bucket_id);

  /*
    access methods for subscribed buckets
    -1 or nullptr are returned in the event of a missing subscribed bucket
  */
  void *getSubscribedBucket(uint8_t bucket_id);
  int32_t getSubscribedBucketSize(uint8_t bucket_id);

  /*
    publish a bucket id and provide the source buffer for data to transmit
    call each time the buffer size or location changes
  */
  void bucket_publish(uint8_t bucket_id, void *buffer, uint16_t length);

  /*
    get pointer for published bucket.
    returns null pointer if bucket is not found
  */
  void *getPublishedBucket(uint8_t bucket_id);

  /*
    run connection
  */
  void run();

  /*
    checks change status of all buckets and updates only if needed
  */
  void checkBucketPublish();

  /*
    getter methods:
  */
  bool isConnected() { return _connected; }

  uint32_t getRemoteId() { return _remote_node_id; }

private:
  /*
    connection constants
  */
  const uint32_t BEACON_PACKET_POLL = 10000;
  const uint32_t HEARTBEAT_PACKET_TIMEOUT = 5000;
  const uint32_t HEARTBEAT_PACKET_POLL = 300;
  const uint32_t DISCONNECT_TIMEOUT = 8000;
  const uint32_t REALIGNMENT_PACKET_POLL = 300;

  /*
    packet constants
  */
  const uint8_t MAX_PACKET = 10;            // declare max number of persistent packets allowed to hold
  const uint32_t SEND_PACKET_POLL = 300;    // how often a missed packet will attempt to send
  const uint32_t BUCKET_COMPARE_POLL = 100; // how often published buckets are checked for changes

  // resource lock. Will crash if another thread attempts to use resources.
  // to do: implement spinlocks or mutex's instead of crash
  ESP_Mesh_util::rscSync _resourceLock{};

  // determines if this connection instance behaves as master or slave
  // this is determined at connection
  bool _is_master;

  // /*
  //   headless mode can only be used as a slave device.
  //   headless master would mean broadcasting a beacon.
  //   A different mechanism is needed for headless master
  // */
  // bool _headless_mode;

  // remote node to target
  uint32_t _remote_node_id;

  /*
    pid's:
      _local_PID: This will contain the next PID that will be used in reception and up to the next 127 PID's
      _remote_PID: This is the last verified PID that was received from remote
      _transmit_PID: This will keep the next PID that will be used in transmission OR the current one sending

    Packet id ranges from 0-255
  */
  uint8_t _local_PID = 0;
  uint8_t _remote_PID = 0;
  uint8_t _transmit_PID = 0;

  // connection stats. used and modified by _connection_class and _packet_class
  bool _connected = false;

  /*
    This signifies that the remote PID cannot be verified
    If a packet is dropped or connection is reset,
    a PID realignment will be required before sending anymore persistent packets can be attempted
  */
  bool _packetRealignment = false;

  /*
    timers:
      _disconnectTimer is only updated during conenction and heartbeat packets
      while it makes perfect sense to update this timer during packet transfers or PID realignments,
      The disconnect time MUST match both devices.
      For the sake of consistency and safety, the disconnect timer is only reset during connection and heartbeats
  */
  /*
    connection timers
  */
  uint32_t _beaconTimer = 0;
  uint32_t _disconnectTimer = 0;
  uint32_t _realignmentTimer = 0;
  uint32_t _heartbeatTimer = 0;

  /*
    packet timers:
  */
  uint32_t _sendPacketTimer = 0;
  uint32_t _bucketCompareTimer = 0;

  /*
    linkedlists for various functions
      - packetbuffer will contain all persistent and eternal packets
      - Bucket lists will contain all the subscribed and published buckets.
        These will contain all buffers, identifiers and flags for each bucket
  */
  linkedList _packetBuffer;
  linkedList _subscribedBuckets;
  linkedList _publishedBuckets;

  /*
    callback for user when new packets come
  */
  void (*_recvCallback)(const char *, ESP_Mesh_Connection *) = nullptr;

  /*
    received data will be assumed to be a JSON object located in the static mesh master class
  */
  void _onDataRecv(uint32_t node_id);
  void _connection_onDataRecv(uint32_t node_id);
  void _packet_onDataRecv(uint32_t node_id);

  void _connection_run();
  void _packet_run();

  void _checkPacketTimers();

  /*
    instead of checking for timers, this will force and update on a specific bucket.
    useful with first time publishes and requests
  */
  void _forcePushBucketPublish(published_bucket_struct *bucket);

  /*
    take raw pid and handle response.
    adjust pid values as necessary

    returns true if new and valid packet
  */
  bool _handlePacketID_ack(uint8_t raw_pid);

  // take published bucket pointer and push to queue. remove other instances of the same bucket
  void _pushBucketPublish(published_bucket_struct *bucket);

  /*
    This is stored into a linked list and pulled when ready to send out
    One of the extended struct is stored in the linkedlist.
    Determining which one can be done by checking the base struct packet 'type'
  */
  /*
    once transmit PID don't match, the packet will be marked as transmitted
  */
  struct packet_base_struct
  {
    uint8_t type = PCK_Null;

    /*
      a structure flag is choosen instead of a global flag primarily for easy and safe iteration
    */
    bool isSending = false;
  };

  struct persistent_packet_struct : packet_base_struct
  {
    uint8_t packetGroup = 0; // default group
    uint32_t timeout = 0;
    uint32_t timer = 0;
    char msg[]; // flexible message buffer
  };

  struct bucket_packet_struct : packet_base_struct
  {
    uint8_t bucket_ID = 0;
    char msg[]; // flexible message buffer. not needed for requests
  };

  /*
    this is used to keep track of buckets and when a change occurs
  */
  struct subscribed_bucket_struct
  {
    uint8_t bucket_ID = 0;
    uint16_t bucket_size = 0;
    byte rx_buffer[]; // flexible message buffer
  };

  struct published_bucket_struct
  {
    uint8_t bucket_ID = 0;
    uint16_t bucket_size = 0;
    void *user_buffer = nullptr;
    byte tx_buffer[]; // flexible message buffer
  };

  /*
    enum for consist packet types:
  */
  enum
  {
    PCK_Null = 0,              // 0 = null
    PCK_Beacon = 1,            // 1 = beacon packet
    PCK_ConnectionRequest = 2, // 2 = connection request
    PCK_Heartbeat = 3,         // 3 = heartbeat packet
    PCK_PID_Realignment = 4,   // 4 = PID realignment
    PCK_SimplePacket = 5,      // 5 = simple packet
    PCK_PersistentPacket = 6,  // 6 = persistent packet
    PCK_BucketRequest = 7,     // 7 = bucket request
    PCK_BucketPublish = 8,     // 8 = bucket publish
    PCK_PacketAck = 9          // 9 = packet ack
  };
};

#endif