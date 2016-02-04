It measures performance of different way to communicate data between processes.

Results
--------------------------------------

- Client sends a message to server and sends next message after getting response
- Optimizations like buffering and sending multiple messages will not benifit in this method.
- To rum client and sever in the same process, run the executable. To run them in seperate process, run the executable in two bash console with -s and -c option.
- The measurements are done in a Intel core i7 machine.

Communication methods tested for client and server in the same machine: 
- Libevent based tcp client and server.
- Client and server using select Api.
- A simple client and server in the same process communicating using memcpy.
- Using kqueue and tcp. (only for Mac).
- Using kqueue with udp. (only for Mac).
- Using epoll and tcp. (only for Linux).
- Using epoll and udp. (only for Linux).
- Using shared memory with spin lock.
- Using shared memory with shared semaphore.
- Client and server using memory mapped file with spin lock.
- Client and server using ZromMQ socket.

Performnce in an Mac OSX machine
<pre>
-----------------------------------
                Same    | 
Method        | Process | Messages /sec
-----------------------------------
livevent      | Y       | 29K
              | N       | 28K
-----------------------------------
Select        | Y       | 39K
              | N       | 41K
-----------------------------------
UDP Select    | Y       | 41K
              | N       | 36K
----------------------------------
memcpy        | Y       | 11M
----------------------------------
kevent        | Y       | 42K
              | N       | 44K
----------------------------------
udp kevent    | Y       | 38K
              | N       | 35K
----------------------------------
shared mem    | Y       | 10M
              | N       | 5 M
----------------------------------
shared mem    | Y       | 728K
sem           | N       | 130K
---------------------------------
mmap          | Y       | 15M
              | N       | 5M
---------------------------------
zeromq        | Y       | 12K
              | N       | 12K
----------------------------------

</pre>

Performance in a Linux VM in Parallells desktop running in OSX

<pre>
                Same
Method        | Process | Messages /sec
-----------------------------------
libevent      | Y       | 54 K
              | N       | 12K
----------------------------------
select        | Y       | 68K
              | N       | 15K
----------------------------------
udp select    | Y       | 160K
              | N       | 17K
----------------------------------
memcpy        | Y       | 18M
----------------------------------
epoll         | Y       | 111K
              | N       | 15K
---------------------------------
udp epoll     | Y       | 165K
              | N       | 16K
--------------------------------
shared mem    | Y       | 9.5M
              | N       | 7 M
---------------------------------
shared mem    | Y       | 2M
sem           | N       | 25K
---------------------------------
mmap          | Y       | 16M
              | N       | 7M
--------------------------------
zeromq        | Y       | 5K
              | N       | 5K
-------------------------------
</pre>

Note:

Shared men: Shared memory with busy wait.

Shared mem sem: Shared memory synchronized by semaphore


