The beginnings of a darknet implementation.

You connect to all your friends, they connect to theirs.
Queries will be flood based, fwding with a delay of ~100ms.
Query cancellations fwded immediately so cancellations can reach search
frontier quickly. Max query survival time of a few seconds.
You only ever get msgs from immediate friends, all queries and fetching of
data appear to come from immediate friends, although they might be relaying
to one of their friends. You can't necesarily tell.
Never know true source of a resolved item. Friends re-transmit to you.

Must provide ip:port of peers atm. Plan to bootstrap the mesh network using 
xmpp accounts and rosters, possibly with gloox library and gtalk servers.

will also need upnp auto-port-fwd setup code. possibly from libtorrent.
