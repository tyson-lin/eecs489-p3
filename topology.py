#!/usr/bin/env python
"""
Measurement topology for EECS489, Winter 2024, Assignment 1
"""

from mininet.cli import CLI
from mininet.net import Mininet
from mininet.link import TCLink
from mininet.topo import Topo
from mininet.log import setLogLevel

import os
import subprocess
import secrets
import sys

original_stdout = sys.stdout
original_stderr = sys.stderr

class AssignmentNetworks(Topo):
    def __init__(self, **opts):
        Topo.__init__(self, **opts)
        # This part adds each individual host
        # and returns an object of each one
        h1 = self.addHost('h1')
        h2 = self.addHost('h2')

        # This part adds each switch in the network
        s1 = self.addSwitch('s1')
        s2 = self.addSwitch('s2')

        # This part specifies the links from host to switch
        self.addLink(h1, s1)
        self.addLink(h2, s2)

        # This specifies the links between switches, with additional
        # information for the bandwidth and delay specified
        self.addLink(s1, s2, bw=1000, delay='1ms')
        
        
if __name__ == '__main__':
    if (len(sys.argv) != 3):
        print("Usage: sudo python3 topology.py [reciever iterations] [sender iterations per receiver]")
        print("\tn receivers are run with a random RWND, and m senders with a random SWND are run for each receiver. Thus there will be m*n total tests ran")
        exit()

    setLogLevel( 'output' )

    os.system("make clean")
    os.system("make")
    os.system("sudo ./clean.sh")

    reciever_iterations = int(sys.argv[1])
    sender_iterations = int(sys.argv[2])
    
    total_iterations = reciever_iterations * sender_iterations
    successes = 0

    for i in range(0,reciever_iterations):
        RWND = secrets.randbelow(100) + 2

        # Create data network
        topo = AssignmentNetworks()
        net = Mininet(topo=topo, link=TCLink, autoSetMacs=True,
            autoStaticArp=True)
        net.start()
        h1 = net.get('h1')
        h2 = net.get('h2')

        # Generate a random integer between 2 and 100
        
        h1_cmd = "./wReceiver-base 8888 " + str(RWND) + " /out receiver-log.txt &"
        h1.cmd(h1_cmd)

        print("\n\nRWND\tSWND\tSTATUS")
        for j in range(0,sender_iterations):
            # Generate a random integer between 2 and 100
            SWND = secrets.randbelow(100) + 2
            h2_cmd = "./wSender-base 10.0.0.1 8888 " + str(SWND) + " test.txt sender-log.txt &"
            h2.cmd(h2_cmd)

            outfile = "out/File-" + str(i*sender_iterations + j) + ".out"
            result = subprocess.run(["diff", outfile, "test.txt"], capture_output=True, text=True)
            log = str(RWND) + "\t" + str(SWND) + "\t"
            if not result.stdout:  # If stdout is empty, the files are the same
                print(log + "PASS")
                successes += 1
            else:
                print(log + "FAIL")
        print("\n\n")

        net.stop()
        os.system("sudo mn -c")

    print("\n\n\nSummary: " + str(successes) + "/" + str(total_iterations) + " tests passed!")
    