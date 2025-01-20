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

public:
  /*
    This is where the behavior of the connection will be determined
    The connection will be setup as a master or slave device
  */
  ESP_Mesh_Connection(uint32_t node_id, void (*recvCallback)(const char *, ESP_Mesh_Connection *), bool is_master = false);

  /*
    not really required since no fancy pointers are used.
    the linked lists can destroy themselves
  */
  // ~ESP_Mesh_Connection();

  /*
    send persistent packet.
    input a null-terminated character buffer

    TODO: include bucket values?
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
    subscribe to a bucket id and provide the target buffer for data received
  */
  void bucket_subscribe(uint8_t bucket_id, void *buffer);

  /*
    publish a bucket id and provide the source buffer for data to transmit
  */
  void bucket_publish(uint8_t bucket_id, void *buffer, uint16_t length);

  /*
    run connection
  */
  void run();

  /*
    checks change status of all buckets and updates accordingly
    This is not really used as it can eat into CPU resources unless if called only when needed

    Each bucket should contain a timer that will keep track of when to check for changes
  */
  void checkBucketPublish();

  /*
    getter methods:
  */
  bool isConnected() { return _connected; }

  uint32_t getRemoteId()
  {
    return _remote_node_id;
  }

private:
  // resource lock. Will crash if another thread attempts to use resources.
  // to do: implement spinlocks or mutex's instead of crash
  ESP_Mesh_util::rscSync _resourceLock{};

  // determines if this connection instance behaves as master or slave
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
    callback for user when new packets come
  */
  void (*_recvCallback)(const char *, ESP_Mesh_Connection *) = nullptr;

  /*
    received data will be assumed to be a JSON object located in the static mesh master class
  */
  void _onDataRecv(uint32_t node_id);

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

  /*
    this class will maintain the connection and reconnect when possible
    This will also handle PID realigning
  */
  class connection_class
  {
  public:
    const uint32_t BEACON_PACKET_POLL = 10000;
    const uint32_t HEARTBEAT_PACKET_TIMEOUT = 5000;
    const uint32_t HEARTBEAT_PACKET_POLL = 300;
    const uint32_t DISCONNECT_TIMEOUT = 8000;
    const uint32_t REALIGNMENT_PACKET_POLL = 300;

    connection_class(ESP_Mesh_Connection *p) : _pointer(p) {}

    void run();
    void onDataRecv(uint32_t node_id);

    /*
      timers:
        _disconnectTimer is only updated during conenction and heartbeat packets
        while it makes perfect sense to update this timer during packet transfers or PID realignments,
        The disconnect time MUST match both devices.
        For the sake of consistency and safety, the disconnect timer is only reset during connection and heartbeats
    */
    uint32_t _beaconTimer = 0;
    uint32_t _disconnectTimer = 0;
    uint32_t _realignmentTimer = 0;
    uint32_t _heartbeatTimer = 0;
    uint32_t _sendPacketTimer = 0;

    ESP_Mesh_Connection *_pointer;
  };
  connection_class _connection{this};

  /*
    this class will manage all packets going in and out.
    This will also handle the buckets
  */
  class packet_class
  {
  public:
    struct published_bucket_struct; // forward declaration

    const uint8_t MAX_PACKET = 10;            // declare max number of persistent packets allowed to hold
    const uint32_t SEND_PACKET_POLL = 300;    // how often a missed packet will attempt to send
    const uint32_t BUCKET_COMPARE_POLL = 100; // how often published buckets are checked for changes

    packet_class(ESP_Mesh_Connection *p) : _pointer(p) {}
    void run();
    void onDataRecv(uint32_t node_id);

    void checkPacketTimers();

    void forcePushBucketPublish(published_bucket_struct *bucket);

    /*
      checks change status of all buckets and updates accordingly
      This is not really used as it can eat into CPU resources unless if called only when needed

      (edit: single global timer is enough) Each bucket should contain a timer that will keep track of when to check for changes
    */
    void checkBucketPublish();

    /*
      instead of checking for timers, this will force and update on a specific bucket.
      useful with first time publishes and requests
    */
    // void bucketPublish(uint8_t bucket_ID);

    // void removeBucketPublish(uint8_t bucket_ID);

    /*
      take raw pid and handle response.
      adjust pid values as necessary

      returns true if new and valid packet
    */
    bool handlePacketID_ack(uint8_t raw_pid);

    /*
      timers:
    */
    uint32_t _sendPacketTimer = 0;
    uint32_t _bucketCompareTimer = 0;

    /*
      linkedlists for various functions
        - packetbuffer will contain all persistent and eternal packets
        - Bucket lists will contain all the receive and sent buckets. These will contain important timers and for compare
    */
    linkedList _packetBuffer;
    linkedList _subscribedBuckets;
    linkedList _publishedBuckets;

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
      char msg[]; // flexible message buffer
    };

    /*
      this is used to keep track of buckets and when a change occurs
    */
    struct subscribed_bucket_struct
    {
      uint8_t bucket_ID = 0;
      uint16_t buffer_size = 0;
      void *user_buffer = nullptr;
    };

    struct published_bucket_struct
    {
      uint8_t bucket_ID = 0;
      uint16_t bucket_size = 0;
      void *user_buffer = nullptr;

      // will work on a global timer instead
      // uint32_t timer = 0;
      // uint32_t change_timeout = 0;

      byte tx_buffer[]; // flexible message buffer
    };

    // take published bucket pointer and push to queue. remove other instances of the same bucket
    void pushBucketPublish(published_bucket_struct *bucket);

    ESP_Mesh_Connection *_pointer;
  };
  packet_class _packet{this};
};

#endif