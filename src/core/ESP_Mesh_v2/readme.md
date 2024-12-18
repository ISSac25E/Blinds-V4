# ESP Mesh v2

## Changes from v1
v1 focused on experimenting on a good design for reliable packet transfer using both espnow protocol and later upgraded to 'PainlessMesh'

This lib will work together with 'PainlessMesh' and add functionality on top of it and on top of v1 in two major ways

- This new version will no longer have a dedicated master or slave device.\
  The incoming and outgoing connections will be completely managed dynamically.\
  While each connection pair will still require a slave and master, this will be determined on who connects first. The actual roles don't matter for data transmission.
- This new version will also add the functionality of synced variables across connection pairs.\
  This feature will greatly simplify and optimize sharing states and config data between devices.

Each mesh instance has two types of connections that they can perform: outgoing and incoming connections
```
ESP_Mesh my_mesh
painlessMesh mesh;

my_mesh.init(&mesh, rx_callback); // the subclass for each connection type will be exactly the same.

my_mesh.add_target(target_id, rx_callback); ; this device initializes this connection (ie. becomes the master)

my_mesh.get_connections() // returns the linked list with active incoming and outgoing connections

```