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
    setLogLevel( 'info' )

    # Create data network
    topo = AssignmentNetworks()
    net = Mininet(topo=topo, link=TCLink, autoSetMacs=True,
           autoStaticArp=True)

    # Run network
    net.start()
    h1 = net.get('h1')
    h2 = net.get('h2')

    os.system("sudo ./clean.sh")
    os.system("make clean")
    os.system("make")

    net.stop()
    
    os.system("sudo mn -c")