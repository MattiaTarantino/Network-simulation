The goal of this tasks is to gain familiarity with the ns-3 working environment, along with Wireshark, and the simulation workflow.

# Simulation 1

Simulate the following network:

![Simulation 1](images/Simulation-1.png)


Regarding the details of the upper layers of the network, is required to create three different configurations that can be dynamically set via the command line using a parameter called "configuration," which is an integer that can only take one of the following values: 0, 1, 2.

Simulation time: **20 seconds for each run**. For all configurations enable PCAP packet tracing only on nodes n0, n5, and n7 and enable ASCII tracing only on the clients and servers.

- **Specifications for configuration 0**:
  - TCP Sink on node n1, port 2600
  - TCP OnOff Client on node n9.
    - Start data transmission: 3 seconds
    - End data transmission: 15 seconds
    - Packet size: 1500 bytes
- **Specifications for configuration 1**:
  - TCP Sink on:
    - n1, port 2600
    - n2, port 7777
  - TCP OnOff Client on node n9 sending data to n1:
    - Start data transmission: 5 seconds
    - End data transmission: 15 seconds
    - Packet size: 2500 bytes
  - TCP OnOff Client on node n8 sending data to n2:
    - Start data transmission: 2 seconds
    - End data transmission: 9 seconds
    - Packet size: 5000 bytes
      
- **Specifications for configuration 2**:
  - UDP Echo Server on node n2, port 63
  - UDP Echo Client on node n8:
    - Send 5 packets at 3s, 4s, 7s, 9s
    - Text to be sent: "message"
    - Packet size: 2560 bytes
  - TCP Sink on node n1, port 2600
  - UDP Sink on node n3, port 2500
  - TCP OnOff Client on node n9:
    - Start data transmission: 3s
    - End data transmission: 9s
    - Packet size: 3000 bytes
  - UDP OnOff Client on node n8:
    - Start data transmission: 5s
    - End data transmission: 15s
    - Packet size: 3000 bytes
   
  
The goal of this tasks is to study the 802.11 protocol family and NetAnim.

# Simulation 2
Simulation a Wireless Local Area Network (WLAN) operating in Ad-hoc mode with 5 nodes. The nodes move following the 2D Random Walk mobility model within a rectangular area defined by its bottom-left corner (coordinates x = -90 m, y = -90 m) and its top-right corner (x = 90 m, y = 90 m). Consider the following specifications.

- **Channel**: Default wireless channel in ns-3.
- **Physical Layer**:
  - Default parameters defined by the IEEE 802.11G standard.
  - Adaptive rate control determined by the AARF algorithm (default).
- **Link Layer**:
  - Standard MAC without any Quality of Service control.
- **Network Layer**:
  - Standard IPv4.
  - Address range: 192.168.1.0/24.
  - Assume that each node acts as an ideal router and exchanges its routing table in the background.
- **Transport Layer**:
  - UDP.
- **Application Layer**:
  - UDP Echo Server on Node 0:
    - Port 20.
  - UDP Echo Client on Node 4:
    - Sends 2 UDP Echo packets to the server at times 1s and 2s.
  - UDP Echo Client on Node 3:
    - Sends 2 UDP Echo packets to the server at times 2s and 4s.
  - Packet size: 512 bytes.

- **Additional details:**
  - The packet tracer should be placed exclusively on Node 2.
  - **NetAnim**: if enabled, the simulation should be able to generate a file named "wireless-task1-rts-<state>.xml" (where <state> is "on" if the useRtsCts parameter is true, otherwise "off"). The simulation should enable packet metadata and trace PHY and MAC counters. The nodes should be marked as follows:
    - Red for the node with the UDP Echo Server with the description "SRV-<id>"
    - Green for the nodes with the UDP Echo Clients with the description "CLI-<id>"
    - Blue for the other nodes with the description "HOC-<id>"
    - <id>represents the Node ID of each node (e.g., "1", "2", etc.).
  - The simulation should accept three different command-line parameters:
    - **useRtsCts**: a boolean (default value: false). If true, it enforces the use of the RTS/CTS handshake by the network.
    - **verbose**: a boolean (default value: false). If true, it enables logging for the UDP Echo Application server and clients.
    - **useNetAnim**: a boolean (default value: false). If true, it generates all the relevant files for NetAnim.
   
# Simulation 3
Simulate a Wireless Local Area Network (WLAN) that operates in Infrastructure mode with 5 nodes and an Access Point (AP). The nodes move following the 2D Random Walk mobility model within a rectangular area defined by its bottom-left corner (coordinates x = -90 m, y = -90 m) and its top-right corner (x = 90 m, y = 90 m).

- **Channel**: Default wireless channel in ns-3.
- **Physical Layer**:
  - Default parameters defined by the IEEE 802.11G standard.
  - Adaptive rate control determined by the AARF algorithm (default).
- **Link Layer**:
  - Standard MAC without any Quality of Service control.
- **Network Layer**:
  - Standard IPv4.
  - Address range: 192.168.1.0/24.
  - Assume that each node acts as an ideal router and exchanges its routing table in the background.
- **Transport Layer**:
  - UDP.
- **Application Layer**:
  - UDP Echo Server on Node 0:
    - Port 21.
  - UDP Echo Client on Node 3:
    - Sends 2 UDP Echo packets to the server at times 2s and 4s.
  - UDP Echo Client on Node 4:
    - Sends 2 UDP Echo packets to the server at times 1s and 4s.
  - Packet size: 512 bytes.

- **Additional details:**
  - The packet tracer should be placed exclusively on Node 4 and on the AP.
  - **NetAnim**: if enabled, the simulation should be able to generate a file named "wireless-task1-rts-<state>.xml" (where <state> is "on" if the useRtsCts parameter is true, otherwise "off"). The simulation should enable packet metadata and trace PHY and MAC counters. The nodes should be marked as follows:
    - Red for the node with the UDP Echo Server with the description "SRV-<id>"
    - Green for the nodes with the UDP Echo Clients with the description "CLI-<id>"
    - Blue for the other nodes with the description "HOC-<id>"
    - Dark purple (RGB = (66,49,137)) for the Access Point node with the label 'AP'.
    - <id>represents the Node ID of each node (e.g., "1", "2", etc.).
  - The simulation should accept three different command-line parameters:
    - **useRtsCts**: a boolean (default value: false). If true, it enforces the use of the RTS/CTS handshake by the network.
    - **verbose**: a boolean (default value: false). If true, it enables logging for the UDP Echo Application server and clients.
    - **useNetAnim**: a boolean (default value: false). If true, it generates all the relevant files for NetAnim.
