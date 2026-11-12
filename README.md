# Censor Proxy

## Description
A proxy server (called the Overseer) saves users from offensive language!
When a client connects to the server, messages are delivered to **one** other client, free from offensive language.

## Installation
1. Clone the repository
2. In project root type `make`

## Usage

### Step 1: Starting the Overseer
In project root directory:
`./bin/proxy 9090` (or another free port you desire)

### Step 2: Connecting as a client

In another terminal there are a few ways you can connect interactively.
Netcat:
`nc 127.0.0.1 9090`

Telnet:
`telnet 127.0.0.1 9090`

socat:
`socat - TCP:127.0.0.1:9090`

## Contributing
Feel free to submit pull requests.
