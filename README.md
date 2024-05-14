## Overview
**"Accelerating Handover in Mobile Satellite Network", INFOCOM 2024**

**Link:** https://arxiv.org/abs/2403.11502

In order to support handover in 5G SA, we firstly implement **handover based on Xn interface** in UERANSIM. Further, we modified the Open5GS and UERANSIM implementations to support the proposed fast handover.

UERANSIM is a open source state-of-the-art 5G UE and RAN (gNodeB) simulator. (https://github.com/aligungr/UERANSIM)

Open5GS is a C-language Open Source implementation of 5G Core and EPC. (https://github.com/open5gs/open5gs)

## handover based on Xn interface

The diagram shows a handover process based on the Xn interface (including proposed changes). Steps 1-10 are a standard process.
<p align="center">
  <a href="https://github.com/OreoFroyo/UERANSIM_beforehandHO"><img src="/.github/handover_procedure.png" width="700" title="UERANSIM"></a>
</p>
In this project, we implemented the contents on the UERANSIM side, including Xn connection, RRC Reconfiguration and other processes. 

For changes to the core network, please refer to another repository (https://github.com/OreoFroyo/open5gs_iupf [branch: pure])

To enable gnb to support Xn-based handover, we implemented in UERANSIM:
- Xn interface in gnb
- Complete signaling process between UE, gnbs and core network

### Usage

The deployment and startup of UERANSIM and Open5GS is unchanged, please refer to the URL provided above.

Environment preparation: two gnbs, one UE, one core network

Make sure the ue has connected to the core network through one gnb and can discover another gnb, the gnb that has been connected to the UE is called **Source gnb** and the other is called **Target gnb** below.

Firstly, view the source gnb id and enter the command line interface:
``` 
user@pc:~/UERANSIM/build$ ./nr-cli UERANSIM-gnb-001-01-1 
--------------------------------------------------------------------------------------------
$ commands
amf-info    | Show some status information about the given AMF
amf-list    | List all AMFs associated with the gNB
info        | Show some information about the gNB
status      | Show some status information about the gNB
ue-count    | Print the total number of UEs connected the this gNB
ue-list     | List all UEs associated with the gNB
xnap-setup  | setup a xnap connection between gnbs
--------------------------------------------------------------------------------------------  
```

Then connect to target gnb:
```
$ xnap-setup --ip 192.168.247.001(change to your Target gnb ip) --port 1478
```
In target gnb, the same method is used to establish a connection to source gnb because it's a both-way connection:

``` 
user@pc:~/UERANSIM/build$ ./nr-cli UERANSIM-gnb-001-01-2

$ xnap-setup --ip 192.168.247.002(change to your source gnb ip) --port 1478
```
Last, enter UE command line interface execute handover:
```
user@pc:~/UERANSIM/build$ ./nr-cli imsi-001010000000001
$ handover
```
In order to test whether the handover is successful, we can send the data packet through port **uesimtun0** and check whether the data is sent to the core network through the target gnb:
```
ping -I uesimtun0 www.google.com
```

## Beforehand-handover in NTN
In order to realize the optimization scheme in paper **"Accelerating Handover in Mobile Satellite Network"**, we further modified the handover process. To be specific, source gnb can send path switch signaling to the core network through target gnb at some point before handover (maybe some time before handover), and the core network stores the signaling but does not trigger it. Correspond to steps 2,3,5,8 in the figure. 

When UE really needs to handover, in the right half of the figure, the signaling stored by the AMF will be triggered at the same time, ensuring a timely handover between the uplink and downlink.

<p align="center">
  <a href="https://github.com/OreoFroyo/UERANSIM_beforehandHO"><img src="/.github/beforehand-HO.png" width="1000" title="UERANSIM"></a>
</p>

### Usage
Environment preparation: two gnbs, one UE, one core network
Make sure the ue has connected to the core network through one gnb and can discover another gnb, the gnb that has been connected to the UE is called **Source gnb** and the other is called **Target gnb** below.

Firstly, view the source gnb id and enter the command line interface:
``` 
user@pc:~/UERANSIM/build$ ./nr-cli UERANSIM-gnb-001-01-1 
--------------------------------------------------------------------------------------------
$ commands
amf-info             | Show some status information about the given AMF
amf-list             | List all AMFs associated with the gNB
info                 | Show some information about the gNB
status               | Show some status information about the gNB
ue-count             | Print the total number of UEs connected the this gNB
ue-list              | List all UEs associated with the gNB
xnap-setup           | setup a xnap connection between gnbs
beforehand-HO-toCore | send beforehand-handover sinal to core via target gnb (for downlink)
--------------------------------------------------------------------------------------------  
```

Then connect to target gnb:
```
$ xnap-setup --ip 192.168.247.001(change to your Target gnb ip) --port 1478
```
In target gnb, the same method is used to establish a connection to source gnb because it's a both-way connection:

``` 
user@pc:~/UERANSIM/build$ ./nr-cli UERANSIM-gnb-001-01-2

$ xnap-setup --ip 192.168.247.002(change to your source gnb ip) --port 1478
```
After Xn connection is established, the source gnb can send the beforehand-HO-toCore signaling to the AMF via the target gnb:
```
$ beforehand-HO-toCore --ueid 1
```
After receiving the signaling message, AMF displays "gnb and message store" in the log.

We implemented timer trigger functionality in a higher level framework, so when only UERANSIM and Open5GS are used, functions that would normally be triggered by timers need to be manually triggered.

For UE command line interface:
```
$ beforehand-HO-toTargetGNB
```
This command triggers the process that starts at 1 in the right half of the diagram. At the same time, we can use the following command to trigger the signaling stored in AMF (on your computer running the core network):
```
sudo sendip -p ipv4 -p udp -p rip -d 0123 -ud 38413 192.168.247.163(your AMF address)
```
Now that handover process is complete, you can of course verify the handover as before:
```
ping -I uesimtun0 www.google.com
```

