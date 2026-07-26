# Networking Stack

## Overview

FKernel implements a complete TCP/IP networking stack with E1000 ethernet driver, supporting IPv4, TCP, UDP, ARP, ICMP, DHCP, and DNS. The stack is designed for both kernel-internal use (NFS, remote debugging) and userspace applications.

## Architecture

```mermaid
flowchart TD
    subgraph "Userspace"
        APP["Applications<br/>BSD socket API"]
    end
    subgraph "Socket Layer"
        US["Unix Sockets<br/>AF_UNIX"]
        TS["TCP Sockets<br/>AF_INET SOCK_STREAM"]
        US2["UDP Sockets<br/>AF_INET SOCK_DGRAM"]
    end
    subgraph "Transport Layer"
        TCP["TCP<br/>3-way handshake, streams"]
        UDP["UDP<br/>Datagrams, port demux"]
    end
    subgraph "Network Layer"
        IP["IPv4<br/>Routing, fragmentation"]
        ARP["ARP<br/>MAC resolution"]
        ICMP["ICMP<br/>Echo/ping"]
    end
    subgraph "Link Layer"
        E1K["E1000 Driver<br/>MMIO, RX/TX rings"]
    end
    subgraph "Services"
        DHCP["DHCP Client<br/>DISCOVER/OFFER/REQUEST/ACK"]
        DNS["DNS Resolver<br/>UDP A-record query"]
        ROUTE["Routing Table<br/>Default GW + subnets"]
    end
    APP --> US
    APP --> TS
    APP --> US2
    US --> TCP
    US --> UDP
    TS --> TCP
    US2 --> UDP
    TCP --> IP
    UDP --> IP
    IP --> ARP
    IP --> ICMP
    IP --> ROUTE
    IP --> E1K
    DHCP --> UDP
    DNS --> UDP
```

## TCP Connection State Machine

```mermaid
stateDiagram-v2
    [*] --> CLOSED
    CLOSED --> SYN_SENT : connect() (client)
    CLOSED --> LISTEN : listen() (server)
    LISTEN --> SYN_RECEIVED : receive SYN
    SYN_SENT --> ESTABLISHED : receive SYN-ACK, send ACK
    SYN_RECEIVED --> ESTABLISHED : receive ACK
    ESTABLISHED --> FIN_WAIT_1 : close() (active)
    ESTABLISHED --> CLOSE_WAIT : receive FIN (passive)
    FIN_WAIT_1 --> FIN_WAIT_2 : receive ACK
    FIN_WAIT_1 --> CLOSING : receive FIN simultaneously
    FIN_WAIT_2 --> TIME_WAIT : receive FIN
    CLOSE_WAIT --> LAST_ACK : close()
    LAST_ACK --> CLOSED : receive ACK
    TIME_WAIT --> CLOSED : 2MSL timeout
```

## Key Components

### E1000 Ethernet Driver
- MMIO register access via BAR0
- RX/TX descriptor rings (128 entries each)
- MAC address from RAL/RAH registers
- Interrupt-driven TX completion (scheduler blocks on TX ring availability)
- PCI bus mastering enabled for DMA

### TCP Implementation
- 3-way handshake (SYN → SYN-ACK → ACK)
- MSS segmentation (1460 bytes)
- FIN-based connection teardown
- Per-connection state machine
- Port-based demultiplexing

**Known Limitations:**
- No retransmission timer
- No congestion control
- No out-of-order buffer
- TX checksum not computed

### UDP Implementation
- Simple send/receive
- Port-based demultiplexing
- No checksum verification

### ARP
- Request/reply handling
- Vector-based table (no expiry)

### DHCP Client
- Full DORA protocol
- Option parsing (message type, server ID, subnet, router, DNS)
- Configures IP/GW/DNS on NetworkStack

### DNS Resolver
- UDP A-record queries
- Name compression support
- Multiple retry attempts

## Socket API

| Syscall | Implementation |
|---------|---------------|
| `socket(domain, type, protocol)` | Creates AF_UNIX, AF_INET TCP/UDP socket |
| `bind(fd, addr, len)` | Binds to port/IP |
| `connect(fd, addr, len)` | TCP 3-way handshake or UDP remote set |
| `listen(fd, backlog)` | TCP server mode |
| `accept(fd, addr, len)` | Blocking accept from connection queue |
| `send/recv/sendto/recvto` | Data transfer |

## Known Issues

1. **No IP fragmentation** — packets > MTU dropped
2. **No TCP/UDP TX checksum** — real stacks may drop packets
3. **ARP entries never expire** — stale entries accumulate
4. **No ICMP redirect handling**
5. **Fixed-size socket arrays** — no dynamic growth
