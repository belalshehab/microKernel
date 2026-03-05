# GossipNode/nim/gossip_node.nimble
packageName = "gossip_node"
version     = "0.1.0"
author      = "Belal Shehab"
description = "Nim gossip node"
license     = "MIT"

backend = "c"

requires "nim >= 2.0.0"
requires "libp2p"
requires "chronos"
