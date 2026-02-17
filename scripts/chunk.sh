#!/bin/bash

printf 'POST /api/ HTTP/1.1\r\nHost: example.com\r\nTransfer-Encoding: chunked\r\nContent-Type: text/plain\r\n\r\n28\r\nThis is a test body for chunked transfer\r\n0\r\n\r\n'  | nc localhost 8080