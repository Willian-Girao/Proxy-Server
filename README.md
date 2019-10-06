# C Proxy Server

C implementation of a proxy server. The proxy here makes web content requisitions in the name of a client while it verifies the existence of forbidden content (e.g. pornography) within the required server reply. The content is verified through the indexing of the replied content and a banned word list. If such banned content is present, the access to the target server is blocked and the user is notified. If no banned content is found, the proxy makes the required content available to the user. 

The proxy implemented here can be used as a way of monitoring web content and prevent unwanted material to be accessed.

## How to Execute (Linux)

1. Compile the the proxy source code

```shell
gcc proxy.c -o <proxy_out_file>
```

2. Run the compiled file along with the port where the proxy will be listening to

```shell
./<proxy_out_file> <port_numer>
```

3. Through the command line, run the command **telnet** in order to make a HTTP requisition (GET) using the proxy

```shell
telnet localhost <port_numer> GET http://icomp.ufam.edu.br/site/
```

## Test Script

A python script is provided in order to perform tests on the C proxy code. Some of the test urls (**pub_urls** array within **proxy_tester.py**) might not be available anymore, in which case you will need to change them to any other url you would like to perform the tests on. To test, run the following on the command line:

```shell
./python_tester.py <proxy_out_file> <port_numer>
```