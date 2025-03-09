#include "ESP_Mesh_Connection.h"

ESP_Mesh_Connection::ESP_Mesh_Connection(uint32_t node_id, void (*recvCallback)(const char *, ESP_Mesh_Connection *))
{
  _remote_node_id = node_id;
  _recvCallback = recvCallback;
}

bool ESP_Mesh_Connection::sendPersistentPacket(const char *c, uint8_t packetGroup, uint32_t timeout_ms)
{
  if (!c)
    return false;

  /*
    get rid of timed out packets first
  */
  _checkPacketTimers();

  /*
    check how much memory left in queue and push new packet if possible
  */
  uint32_t total_used = 0;
  for (auto &packet : _packetBuffer)
  {
    /*
      count only if persistent packet
    */
    if (packet.getNodeData<packet_base_struct>()->type == PCK_PersistentPacket)
      total_used++;
  }

  if (total_used >= MAX_PACKET)
    return false;

  // add new packet to queue
  persistent_packet_struct *this_packet = _packetBuffer.addNode<persistent_packet_struct>(-1, strlen(c) + 1);

  if (!this_packet)
    return false; // couldn't allocate required memory

  this_packet->type = PCK_PersistentPacket;
  this_packet->timeout = timeout_ms;
  this_packet->timer = millis();
  this_packet->isSending = false;
  this_packet->packetGroup = packetGroup;
  strcpy(this_packet->msg, c);
  return true;
}

bool ESP_Mesh_Connection::sendSimplePacket(const char *c)
{
  if (!c)
    return false;

  if (_connected)
  {
    ESP_Mesh_util::jsonDocTx.clear();
    ESP_Mesh_util::jsonDocTx["type"] = PCK_SimplePacket;
    ESP_Mesh_util::jsonDocTx["msg"] = c;

    char c_msg_tx[measureJson(ESP_Mesh_util::jsonDocTx) + 1];
    serializeJson(ESP_Mesh_util::jsonDocTx, c_msg_tx, sizeof(c_msg_tx));

    Serial.print("TX to: ");
    Serial.println(_remote_node_id);
    Serial.println("\t" + String(c_msg_tx));
    ESP_Mesh_util::meshPointer->sendSingle(_remote_node_id, c_msg_tx);
    return true;
  }
  return false;
}

void ESP_Mesh_Connection::bucket_subscribe(uint8_t bucket_id)
{
  /*
    check if bucket already exists
  */
  for (auto &bucket : _subscribedBuckets)
  {
    if (bucket.getNodeData<subscribed_bucket_struct>()->bucket_ID == bucket_id)
    {
      /*
        bucket already exists, nothing to subscribe to
      */
      return;
    }
  }

  /*
    add new bucket
  */
  subscribed_bucket_struct *newBucket = _subscribedBuckets.addNode<subscribed_bucket_struct>(0, 0);

  if (!newBucket)
    return; // couldn't allocate required memory

  newBucket->bucket_ID = bucket_id;
  newBucket->bucket_size = 0; // uninitialized bucket. will need to be resized when data is received

  /*
    find and remove any packets in queue that are requesting this bucket?
    this should be impossible as deleting a subscribed bucket SHOULD delete all requests as well
  */

  /*
    push bucket request into send queue
  */
  bucket_packet_struct *newPacket = _packetBuffer.addNode<bucket_packet_struct>(-1, 0);
  if (!newPacket)
    return; // couldn't allocate required memory

  newPacket->type = PCK_BucketRequest;
  newPacket->bucket_ID = bucket_id;
  newPacket->isSending = false;
  // newPacket->msg[0]; // empty message, this is the only packet type that doesn't require a message
}

void *ESP_Mesh_Connection::getSubscribedBucket(uint8_t bucket_id)
{
  for (auto &bucket : _subscribedBuckets)
  {
    if (bucket.getNodeData<subscribed_bucket_struct>()->bucket_ID == bucket_id)
      return bucket.getNodeData<subscribed_bucket_struct>()->rx_buffer;
  }
  return nullptr;
}
int32_t ESP_Mesh_Connection::getSubscribedBucketSize(uint8_t bucket_id)
{
  for (auto &bucket : _subscribedBuckets)
  {
    if (bucket.getNodeData<subscribed_bucket_struct>()->bucket_ID == bucket_id)
      return bucket.getNodeData<subscribed_bucket_struct>()->bucket_size;
  }

  return -1;
}

void *ESP_Mesh_Connection::getPublishedBucket(uint8_t bucket_id)
{
  for (auto &bucket : _publishedBuckets)
  {
    if (bucket.getNodeData<published_bucket_struct>()->bucket_ID == bucket_id)
      return bucket.getNodeData<published_bucket_struct>()->user_buffer;
  }
  return nullptr;
}

void ESP_Mesh_Connection::bucket_publish(uint8_t bucket_id, void *buffer, uint16_t length)
{
  if (!buffer)
    return;

  /*
    check if bucket already exists
  */
  for (auto &bucket : _publishedBuckets)
  {
    if (bucket.getNodeData<published_bucket_struct>()->bucket_ID == bucket_id)
    {
      /*
        bucket already exists, delete
      */
      bucket.deleteNode();
      break; // there should only be one bucket with the same ID
    }
  }

  /*
    add new bucket
  */
  published_bucket_struct *newBucket = _publishedBuckets.addNode<published_bucket_struct>(0, length);

  if (!newBucket)
    return; // couldn't allocate required memory

  newBucket->bucket_ID = bucket_id;
  newBucket->user_buffer = buffer;
  newBucket->bucket_size = length;
  /*
    todo: bucket timing
  */
  // newBucket->timer = millis();
  // newBucket->change_timeout = 0;

  /*
    push bucket publish into send queue
  */
  _forcePushBucketPublish(newBucket);
}

void ESP_Mesh_Connection::run()
{
  _resourceLock.lock();

  _connection_run();
  _packet_run();

  _resourceLock.unlock();
}

void ESP_Mesh_Connection::checkBucketPublish()
{
  /*
  check and update all buckets that are being published if needed
*/
  for (auto &bucket : _publishedBuckets)
  {
    /*
      compare for bucket changes
    */
    published_bucket_struct *this_bucket = bucket.getNodeData<published_bucket_struct>();
    if (memcmp(this_bucket->tx_buffer, this_bucket->user_buffer, this_bucket->bucket_size))
    {
      /*
        a difference between tx buffer and user buffer was found, force into send queue
      */
      _forcePushBucketPublish(this_bucket);
      // Serial.println("Bucket ID " + String(this_bucket->bucket_ID) + " Pushed to send queue");
    }
  }
}

void ESP_Mesh_Connection::_onDataRecv(uint32_t node_id)
{
  _resourceLock.lock();

  _connection_onDataRecv(node_id);
  _packet_onDataRecv(node_id);

  _resourceLock.unlock();
}

void ESP_Mesh_Connection::_connection_run()
{
  if (_connected)
  {
    /*
      Device is connected.
      run heartbeat packets to keep connection alive (only if master)
    */
    if (_is_master)
    {
      if (millis() - _disconnectTimer >= HEARTBEAT_PACKET_TIMEOUT)
      {
        if (millis() - _heartbeatTimer >= HEARTBEAT_PACKET_POLL)
        {
          /*
            send heartbeat packet
          */
          ESP_Mesh_util::jsonDocTx.clear();
          ESP_Mesh_util::jsonDocTx["type"] = PCK_Heartbeat;

          char c_msg_tx[measureJson(ESP_Mesh_util::jsonDocTx) + 1];
          serializeJson(ESP_Mesh_util::jsonDocTx, c_msg_tx, sizeof(c_msg_tx));

          Serial.print("TX to: ");
          Serial.println(_remote_node_id);
          Serial.println("\t" + String(c_msg_tx));
          ESP_Mesh_util::meshPointer->sendSingle(_remote_node_id, c_msg_tx);

          _heartbeatTimer = millis();
        }
      }
    }

    /*
      run realignment packets
    */
    if (_packetRealignment)
    {
      if (millis() - _realignmentTimer >= REALIGNMENT_PACKET_POLL)
      {
        /*
          send realignment packet
        */
        ESP_Mesh_util::jsonDocTx.clear();
        ESP_Mesh_util::jsonDocTx["type"] = PCK_PID_Realignment;
        ESP_Mesh_util::jsonDocTx["pid"] = _transmit_PID;

        char c_msg_tx[measureJson(ESP_Mesh_util::jsonDocTx) + 1];
        serializeJson(ESP_Mesh_util::jsonDocTx, c_msg_tx, sizeof(c_msg_tx));

        Serial.print("TX to: ");
        Serial.println(_remote_node_id);
        Serial.println("\t" + String(c_msg_tx));
        ESP_Mesh_util::meshPointer->sendSingle(_remote_node_id, c_msg_tx);

        _realignmentTimer = millis();
      }
    }

    /*
      run watchdog timer.
      If no heartbeat packets from other device, disconnect
    */
    if (millis() - _disconnectTimer >= DISCONNECT_TIMEOUT)
    {
      // Serial.println("Disconnection Timeout from " + String(_remote_node_id));
      _connected = false;
      _packetRealignment = false; // alignment not needed when reconnecting
      _beaconTimer = millis() - BEACON_PACKET_POLL;
    }
  }
  else
  {
    /*
      currently disconnected.
      Send beacon packets to find other devices
    */
    if (millis() - _beaconTimer >= BEACON_PACKET_POLL)
    {
      /*
        send beacon packet
      */
      ESP_Mesh_util::jsonDocTx.clear();
      ESP_Mesh_util::jsonDocTx["type"] = PCK_Beacon;

      char c_msg_tx[measureJson(ESP_Mesh_util::jsonDocTx) + 1];
      serializeJson(ESP_Mesh_util::jsonDocTx, c_msg_tx, sizeof(c_msg_tx));

      Serial.print("TX to: ");
      Serial.println(_remote_node_id);
      Serial.println("\t" + String(c_msg_tx));
      ESP_Mesh_util::meshPointer->sendSingle(_remote_node_id, c_msg_tx);

      _beaconTimer = millis();
    }
  }
}

void ESP_Mesh_Connection::_connection_onDataRecv(uint32_t node_id)
{
  if (node_id == _remote_node_id)
  {
    /*
      parse once to save on processing later
    */
    uint8_t packet_type = ESP_Mesh_util::jsonDocRx["type"] | (uint8_t)PCK_Null;
    bool is_ack = ESP_Mesh_util::jsonDocRx["ack"] | false;
    int16_t raw_pid = ESP_Mesh_util::jsonDocRx["pid"] | (int16_t)-1;
    bool pid_error = ESP_Mesh_util::jsonDocRx["err"] | false;

    /*
      this will run all the default commands for disconnected state
    */
    if (!_connected)
    {
      switch (packet_type)
      {
      case PCK_Beacon:
      {
        /*
          send connection request and send transmit PID
        */
        ESP_Mesh_util::jsonDocTx.clear();
        ESP_Mesh_util::jsonDocTx["type"] = PCK_ConnectionRequest;
        ESP_Mesh_util::jsonDocTx["pid"] = _transmit_PID;

        char c_msg_tx[measureJson(ESP_Mesh_util::jsonDocTx) + 1];
        serializeJson(ESP_Mesh_util::jsonDocTx, c_msg_tx, sizeof(c_msg_tx));

        Serial.print("TX to: ");
        Serial.println(_remote_node_id);
        Serial.println("\t" + String(c_msg_tx));
        ESP_Mesh_util::meshPointer->sendSingle(_remote_node_id, c_msg_tx);

        // Serial.println("Beacon received from " + String(_remote_node_id));
      }
      break;
      case PCK_ConnectionRequest:
        if (raw_pid > -1 && raw_pid < 256) // required field and required range
        {
          /*
            send response (if not ack packet)
            mark as connected

            update local PID with received PID
          */

          _local_PID = static_cast<uint8_t>(raw_pid); // set local PID to received PID

          _remote_PID = _transmit_PID; // reset remote PID to transmit PID
          _packetRealignment = false;  // alignment not needed

          /*
            mark connected
          */
          _connected = true;
          _disconnectTimer = millis();
          _heartbeatTimer = millis();

          /*
            master is the device that accepts the connection ack
          */

          if (!is_ack)
          {
            /*
              send response back
              this is not going to be the master
            */
            _is_master = false;

            ESP_Mesh_util::jsonDocTx.clear();
            ESP_Mesh_util::jsonDocTx["type"] = PCK_ConnectionRequest;
            ESP_Mesh_util::jsonDocTx["ack"] = true;
            ESP_Mesh_util::jsonDocTx["pid"] = _transmit_PID; // send PID going to send next

            char c_msg_tx[measureJson(ESP_Mesh_util::jsonDocTx) + 1];
            serializeJson(ESP_Mesh_util::jsonDocTx, c_msg_tx, sizeof(c_msg_tx));

            Serial.print("TX to: ");
            Serial.println(_remote_node_id);
            Serial.println("\t" + String(c_msg_tx));
            ESP_Mesh_util::meshPointer->sendSingle(_remote_node_id, c_msg_tx);
            // Serial.println("Connection request received from " + String(_remote_node_id));
          }
          else
          {
            /*
              no need to send anything back
              mark as the master device
            */

            _is_master = true;
            // Serial.println("Connection response received from " + String(_remote_node_id));
          }
        }
        break;
      }
    }
    else
    {
      switch (packet_type)
      {
      case PCK_Heartbeat:
        if (!is_ack)
        {
          /*
            This is a request that was received
            This is sent by the master.
            The master is checking if we are is still alive and letting us know its still alive.
          */
          /*
            send ack
          */
          ESP_Mesh_util::jsonDocTx.clear();
          ESP_Mesh_util::jsonDocTx["type"] = PCK_Heartbeat;
          ESP_Mesh_util::jsonDocTx["ack"] = true;

          // String msg_tx;
          // serializeJson(ESP_Mesh_util::jsonDocTx, msg_tx);

          char c_msg_tx[measureJson(ESP_Mesh_util::jsonDocTx) + 1];
          serializeJson(ESP_Mesh_util::jsonDocTx, c_msg_tx, sizeof(c_msg_tx));

          Serial.print("TX to: ");
          Serial.println(_remote_node_id);
          Serial.println("\t" + String(c_msg_tx));
          ESP_Mesh_util::meshPointer->sendSingle(_remote_node_id, c_msg_tx);
        }
        else
        {
          /*
            This is a response that was received
            This is sent by the slave.
          */
        }

        /*
          reset connection timer
        */
        _disconnectTimer = millis();

        break;
      case PCK_PID_Realignment:
        if (raw_pid > -1 && raw_pid < 256) // required field and required range
        {
          if (!is_ack)
          {
            /*
              this is a request, this can be sent by either device

              This type of request is attempting to change our local PID
              Verify that PID is within acceptable range, otherwise, assume as duplicate/delayed packet

              send a valid response
            */
            // Serial.println("PID Realignment request received from " + String(_remote_node_id));
            bool error = false;
            if (ESP_Mesh_util::byte_diff(_local_PID, raw_pid) < 128)
            {
              /*
                PID is within acceptable range
                set local PID to received PID
              */
              _local_PID = static_cast<uint8_t>(raw_pid);
            }
            else
            {
              /*
                PID is outside acceptable range
                flag error so transmitter knows
              */
              error = true;
              // Serial.println("\tPID error");
            }

            /*
              send response
            */
            ESP_Mesh_util::jsonDocTx.clear();
            ESP_Mesh_util::jsonDocTx["type"] = PCK_PID_Realignment;
            ESP_Mesh_util::jsonDocTx["ack"] = true;
            ESP_Mesh_util::jsonDocTx["pid"] = raw_pid;

            if (error)
              ESP_Mesh_util::jsonDocTx["err"] = true;

            // String msg_tx;
            // serializeJson(ESP_Mesh_util::jsonDocTx, msg_tx);

            char c_msg_tx[measureJson(ESP_Mesh_util::jsonDocTx) + 1];
            serializeJson(ESP_Mesh_util::jsonDocTx, c_msg_tx, sizeof(c_msg_tx));

            Serial.print("TX to: ");
            Serial.println(_remote_node_id);
            Serial.println("\t" + String(c_msg_tx));
            ESP_Mesh_util::meshPointer->sendSingle(_remote_node_id, c_msg_tx);
          }
          else
          {
            // Serial.println("PID Realignment response received from " + String(_remote_node_id));
            /*
              this is a response packet
              check that we are actually expecting a realignment and that is matches exactly
            */
            if (_packetRealignment && raw_pid == _transmit_PID)
            {
              /*
                check for error flag
              */
              if (pid_error)
              {
                /*
                  couldn't realign PID, critical error
                  try reconnecting
                */
                _connected = false;
                // Serial.println("\tRealignment error, disconnecting");
              }
              else
              {
                /*
                  PID's match, no error, realignment successful
                  update our local PID
                  set realignment to false
                */
                _remote_PID = _transmit_PID;
                _packetRealignment = false;
                // Serial.println("\tRealignment Success");
              }
            }
          }
        }
        break;
      }
    }
  }
}

void ESP_Mesh_Connection::_packet_run()
{
  /*
    bucket management handle:
      TODO:
  */

  /*
    packet queue handle:
  */
  if (_connected)
  {
    /*
      check that at least one node exists to send
      Also check that we are NOT currently realigning packets
    */
    packet_base_struct *packet = _packetBuffer.getNodeData<packet_base_struct>(0);
    if (packet && !_packetRealignment)
    {
      /*
        check timeout of the first packet
        get rid of the packet if possible and start sending the next one
      */
      if (packet->type == PCK_PersistentPacket)
      {
        persistent_packet_struct *p = reinterpret_cast<persistent_packet_struct *>(packet);
        if (p->timeout && millis() - p->timer >= p->timeout)
        {
          /*
            timeout reached, remove packet
            check if it was currently sending, if so, adjust transmit_PID
          */
          if (packet->isSending)
            (_transmit_PID)++; // prepare transmit PID for next packet

          _packetBuffer.deleteNode(0); // remove packet
          // Serial.println("Packet timeout");

          /*
            nothing else to do.
            we need to check timer of the next packet anyways, cant take shortcuts
          */
          return;
        }
      }

      /*
        check if current packet is sending
          if not, mark as sending
      */
      if (!(packet->isSending))
      {
        /*
          currently not sending packet, begin sending one
        */
        if (ESP_Mesh_util::byte_diff(_remote_PID, _transmit_PID) < 128)
        {
          /*
            valid transmit PID, although it could be on the very edge
          */
          packet->isSending = true; // mark as sending

          /*
            reset send timer for quicker send delay next time
          */
          _sendPacketTimer = millis() - SEND_PACKET_POLL;
        }
        else
        {
          /*
            outside of sendable range
            get packet realignment first
          */
          _packetRealignment = true;
          // Serial.println("PID Realignment needed while sending");
          return; // nothing else to do while realigning
        }
      }

      /*
        check if current packet is sending (this should always be true, otherwise function would have returned by now)
      */
      if (packet->isSending)
      {
        /*
          check send packet timer
          send again when timer reaches SEND_PACKET_POLL
        */
        if (millis() - _sendPacketTimer >= SEND_PACKET_POLL)
        {
          _sendPacketTimer = millis();

          /*
            send packet
            construct head of the packet
          */
          ESP_Mesh_util::jsonDocTx.clear();
          ESP_Mesh_util::jsonDocTx["type"] = packet->type;
          ESP_Mesh_util::jsonDocTx["pid"] = _transmit_PID;

          /*
            construct according to wether it is a persistent packet or a bucket packet
          */
          switch (packet->type)
          {
          case PCK_PersistentPacket:
            ESP_Mesh_util::jsonDocTx["msg"] = reinterpret_cast<persistent_packet_struct *>(packet)->msg;
            break;
          case PCK_BucketRequest:
            ESP_Mesh_util::jsonDocTx["bck"] = reinterpret_cast<bucket_packet_struct *>(packet)->bucket_ID;
            break;
          case PCK_BucketPublish:
            ESP_Mesh_util::jsonDocTx["bck"] = reinterpret_cast<bucket_packet_struct *>(packet)->bucket_ID;
            ESP_Mesh_util::jsonDocTx["msg"] = reinterpret_cast<bucket_packet_struct *>(packet)->msg;
            break;
          default:
            // error!
            // unknown packet type!
            _packetBuffer.deleteNode(0);
            // _transmit_PID++; // prepare PID for next packet? not really needed since it should be impossible to send a packet with an invalid type
            return; // get out, nothing else to do for now
            break;
          }

          char c_msg_tx[measureJson(ESP_Mesh_util::jsonDocTx) + 1];
          serializeJson(ESP_Mesh_util::jsonDocTx, c_msg_tx, sizeof(c_msg_tx));

          Serial.print("TX to: ");
          Serial.println(_remote_node_id);
          Serial.println("\t" + String(c_msg_tx));
          ESP_Mesh_util::meshPointer->sendSingle(_remote_node_id, c_msg_tx);
        }
      }
    }
    else
    {
      /*
        no packets to send OR Currently realigning PID's
        check that we won't need a realignment when we start sending one
      */
      if (!_packetRealignment)
      {
        if (_remote_PID != _transmit_PID)
        {
          _packetRealignment = true;
          // Serial.println("PID Realignment needed while idling");
        }
      }
    }
  }
}

void ESP_Mesh_Connection::_packet_onDataRecv(uint32_t node_id)
{
  if (node_id == _remote_node_id && _connected)
  {
    /*
      parse once to save on processing later
    */
    uint8_t packet_type = ESP_Mesh_util::jsonDocRx["type"] | (uint8_t)PCK_Null;
    // bool is_ack = ESP_Mesh_util::jsonDocRx["ack"] | false; // not needed for any packet_class types of packets
    int16_t raw_pid = ESP_Mesh_util::jsonDocRx["pid"] | (int16_t)-1;
    int16_t bucket_id = ESP_Mesh_util::jsonDocRx["bck"] | (int16_t)-1;
    bool pid_error = ESP_Mesh_util::jsonDocRx["err"] | false;
    const char *msg = ESP_Mesh_util::jsonDocRx["msg"];

    switch (packet_type)
    {
    case PCK_SimplePacket:
      if (_recvCallback)
        _recvCallback(ESP_Mesh_util::jsonDocRx["msg"].as<const char *>(), this);
      break;
    case PCK_PersistentPacket:
      /*
        received persistent packet
        call user callback
      */
      if (raw_pid > -1 && raw_pid < 256) // PID required in packet
      {
        if (_handlePacketID_ack(raw_pid))
          if (_recvCallback)
            _recvCallback(ESP_Mesh_util::jsonDocRx["msg"].as<const char *>(), this);
      }
      break;
    case PCK_BucketRequest:
      /*
        bucket request
          check if we have requested bucket ID and force push update to queue
          pull any other current sending bucket publishes out of the queue
      */
      if (raw_pid > -1 && raw_pid < 256 &&   // PID required in packet
          bucket_id > -1 && bucket_id < 256) // Bucket ID required in packet
      {
        if (_handlePacketID_ack(raw_pid))
        {
          // Serial.println("Bucket request received from " + String(_remote_node_id));
          /*
            valid packet received
            check if we have the correct requested bucket ID and force push update to queue
            pull any other current sending bucket publishes out of the queue

            if bucket is not found, send a blank publish to indicated no bucket
          */
          published_bucket_struct *this_bucket = nullptr;
          for (auto &bucket : _publishedBuckets)
          {
            if (bucket.getNodeData<published_bucket_struct>()->bucket_ID == bucket_id)
            {
              this_bucket = bucket.getNodeData<published_bucket_struct>();
              break;
            }
          }

          /*
            check if bucket was located
          */
          if (this_bucket)
          {
            // Serial.println("\tBucket ID " + String(this_bucket->bucket_ID) + " Pushed to send queue");
            _forcePushBucketPublish(this_bucket);
          }
          else
          {
            // Serial.println("\tBucket ID " + String(bucket_id) + " not found, sending empty bucket");
            /*
              set up temporary bucket with no size to push to send queue
            */
            published_bucket_struct temp_bucket;
            byte temp_byte[0];

            temp_bucket.bucket_ID = bucket_id;
            temp_bucket.bucket_size = 0;
            temp_bucket.user_buffer = temp_byte;

            this_bucket = &temp_bucket;
            _forcePushBucketPublish(this_bucket);
          }
        }
      }
      break;
    case PCK_BucketPublish:
      /*
        bucket publish
          other device is pushing an update to us. See if we want this bucket
      */
      if (raw_pid > -1 && raw_pid < 256 &&   // PID required in packet
          bucket_id > -1 && bucket_id < 256) // Bucket ID required in packet
      {
        if (_handlePacketID_ack(raw_pid))
        {
          // Serial.println("Bucket publish received from " + String(_remote_node_id));
          /*
            valid packet received

            check if we are subscribed to this bucket ID.
            If we are, update the bucket with the new data
          */
          subscribed_bucket_struct *this_bucket = nullptr;
          for (auto &bucket : _subscribedBuckets)
          {
            if (bucket.getNodeData<subscribed_bucket_struct>()->bucket_ID == bucket_id)
            {
              this_bucket = bucket.getNodeData<subscribed_bucket_struct>();
              break;
            }
          }

          /*
            if subscribed bucket was not found, ignore
          */
          if (this_bucket)
          {
            // Serial.println("\tBucket ID " + String(this_bucket->bucket_ID) + " Updated: " + String(this_bucket->bucket_size) + " : " + String(ESP_Mesh_util::decode_base64_size(msg)));
            /*
              calculate the size of bucket and resize subscribe bucket if needed
            */
            if (this_bucket->bucket_size != ESP_Mesh_util::decode_base64_size(msg))
            {
              // Serial.println("\tBucket size mismatch, resizing to " + String(ESP_Mesh_util::decode_base64_size(msg)));
              // resize buffer
              for (auto &bucket : _subscribedBuckets)
                if (bucket.getNodeData<subscribed_bucket_struct>()->bucket_ID == bucket_id)
                {
                  bucket.deleteNode();
                  break;
                }

              this_bucket = _subscribedBuckets.addNode<subscribed_bucket_struct>(0, ESP_Mesh_util::decode_base64_size(msg));
              if (!this_bucket)
                return; // couldn't allocate required memory

              this_bucket->bucket_ID = bucket_id;
            }

            // get size of buffer and dump decoded data into internal buffer
            this_bucket->bucket_size = ESP_Mesh_util::decode_base64(msg, this_bucket->rx_buffer);
          }
        }
      }
      break;
    case PCK_PacketAck:
      /*
        received ack packet
        check if currently sending something that expects an ack packet.
        Check the received PID matches what we are sending
        PID required
      */
      if (raw_pid > -1 && raw_pid < 256) // PID found in packet
      {
        packet_base_struct *packet = _packetBuffer.getNodeData<packet_base_struct>(0);

        if (packet && packet->isSending && raw_pid == _transmit_PID)
        {
          // Serial.println("Ack received from " + String(_remote_node_id) + " for PID " + String(raw_pid));
          /*
            everything checks out, make sure no error was found
          */
          if (pid_error)
          {
            // Serial.println("\tError found in packet");
            /*
              receiver did not accept the packet ID, get realignment
              if realignment fails to set PID also, force reconnect
            */
            _packetRealignment = true;
          }
          else
          {
            _remote_PID = raw_pid; // update local PID
            (_remote_PID)++;       // increment

            // update transmit for next packet. This is essentially (_transmit_PID)++;
            _transmit_PID = _remote_PID;

            // delete packet
            _packetBuffer.deleteNode(0);
          }
        }
      }
      break;
    }
  }
}

void ESP_Mesh_Connection::_checkPacketTimers()
{
  /*
    check all persistent packets within the queue and remove all timed out packets
  */
  for (auto &packet : _packetBuffer)
  {
    if (packet.getNodeData<packet_base_struct>()->type == PCK_PersistentPacket)
    {
      persistent_packet_struct *this_packet = packet.getNodeData<persistent_packet_struct>();
      if (this_packet->timeout && millis() - this_packet->timer >= this_packet->timeout)
      {
        // Serial.println("Packet timeout");
        /*
          timeout reached, remove packet
          check if it was currently sending, if so, adjust transmit_PID
        */
        if (packet.getNodeData<packet_base_struct>()->isSending)
          (_transmit_PID)++; // prepare transmit PID for next packet

        packet.deleteNode();
      }
    }
  }
}

void ESP_Mesh_Connection::_forcePushBucketPublish(published_bucket_struct *bucket)
{
  /*
    remove any other instances of the same bucket from send queue and
    push the bucket publish to the queue
  */

  /*
    copy all data into tx_buffer to signal updated status
  */
  memcpy(bucket->tx_buffer, bucket->user_buffer, bucket->bucket_size);

  /*
    find and remove any packets in queue that are publishing this bucket
  */
  for (auto &packet : _packetBuffer)
  {
    if (packet.getNodeData<packet_base_struct>()->type == PCK_BucketPublish &&
        packet.getNodeData<bucket_packet_struct>()->bucket_ID == bucket->bucket_ID)
    {
      /*
        a packet was found, remove from the queue
      */
      if (packet.getNodeData<packet_base_struct>()->isSending)
        (_transmit_PID)++;

      packet.deleteNode();
    }
  }

  char encodedBucket[(((bucket->bucket_size / 3) + 1) << 2) + 1]; // calc required length
  ESP_Mesh_util::encode_base64(encodedBucket, bucket->tx_buffer, bucket->bucket_size);

  /*
    add new node with required space at the end of linkedlist
  */
  bucket_packet_struct *newPacket = _packetBuffer.addNode<bucket_packet_struct>(-1, sizeof(encodedBucket));

  if (!newPacket)
    return; // abort instead? couldn't allocate required memory

  newPacket->type = PCK_BucketPublish;
  newPacket->isSending = false;
  newPacket->bucket_ID = bucket->bucket_ID;
  memcpy(newPacket->msg, encodedBucket, sizeof(encodedBucket));
}

bool ESP_Mesh_Connection::_handlePacketID_ack(uint8_t raw_pid)
{
  bool is_new_msg = false;  // flag true if message is received first time and valid
  bool isError_PID = false; // check if this is duplicate packet or a error

  /*
    check that this PID is within acceptable range before accepting the packet
    |-----_local_PID - - - Acceptable Range - - - _local_PID + 127------|
  */
  if (ESP_Mesh_util::byte_diff(_local_PID, raw_pid) < 128)
  {
    _local_PID = raw_pid; // increment. rollover at byte size
    (_local_PID)++;       // only accept next packets.

    is_new_msg = true;
  }
  /*
    check if raw_pid is just a duplicate packet. Thats not an error
  */
  else if (ESP_Mesh_util::byte_diff(_local_PID, raw_pid) != 255)
  {
    /*
      unrecognized packet, signal error
    */
    isError_PID = true;
  }

  /*
    send ack
  */
  ESP_Mesh_util::jsonDocTx.clear();
  ESP_Mesh_util::jsonDocTx["type"] = PCK_PacketAck;
  ESP_Mesh_util::jsonDocTx["pid"] = raw_pid;
  if (isError_PID)
    ESP_Mesh_util::jsonDocTx["err"] = true;

  char c_msg_tx[measureJson(ESP_Mesh_util::jsonDocTx) + 1];
  serializeJson(ESP_Mesh_util::jsonDocTx, c_msg_tx, sizeof(c_msg_tx));

  Serial.print("TX to: ");
  Serial.println(_remote_node_id);
  Serial.println("\t" + String(c_msg_tx));
  ESP_Mesh_util::meshPointer->sendSingle(_remote_node_id, c_msg_tx);

  return is_new_msg;
}