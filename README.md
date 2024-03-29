# QuickMath
A math network game between client and server, in which the player has to answer to random-generated simple math equations asap. Then the score is computed and stored in the SQL database. It uses TCP for playing part and UDP for Multicast service discovery.

To play you need SQLite3 library downloaded for the files to compile. Works best on linux systems.

## Quick instructuions
1. Download the files and compile them using Makefile.
2. Execute the server program.
3. Execute the client program with the network interface specified.
4. Enjoy the competition.

Make sure the client and server are in the same network.
