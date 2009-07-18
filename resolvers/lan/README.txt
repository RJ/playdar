The LAN plugin broadcasts searches using UDP multicast.
Results are streamed over HTTP using the address the UDP result came from.
It should just automatically work on home/small office networks.
--
By default you don't need anything in the config file for this plugin, however
the following options are valid if you need any of them:

 "lan" : 
 {
     "listenip" : "your.lan.ip.address",
     "listenport" : 8888,
     "endpoints" : ["239.255.0.1", "10.1.2.3", "192.168.1.1"],
     "numcopies" : 3
 }

* If you change the "listenport", it'll only work with others on the same port.

* "listenip" might be useful if you have lots of interfaces.

* endpoints are where to send udp msgs to. The first, and default, is the 
  udp multicast address, which will be sent to the entire subnet.
  If you have multicast disabled, or multiple subnets, or some other exotic
  network configuration, you can put multicast addresses, or addresses of other
  hosts known to be running playdar in there. This means you can target any
  machine you can reach on the LAN, despite multicast not working.

* to change endpoint ports too: "endpoints" : [ ["10.1.2.3",1234], [..] ]

* for lossy networks, set copies to more than 1, so your UDP packets stand a 
  better chance of being received. default is 1.
