#!/bin/bash

(
  printf "POST /api/ HTTP/1.1\r\nHost: localhost\r\nTransfer-Encoding: chunked\r\nContent-Type: text/plain\r\n\r\n"; sleep 1; printf "4\r";
  sleep 1;
  printf "\nWiki\r\n5\r\n";
  sleep 1;
  printf "pedia\r\n0\r\n\r\n";
) | nc localhost 8080