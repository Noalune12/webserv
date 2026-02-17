#!/bin/bash

BOUNDARY="----BOUNDARY123"
HOST="localhost:8080"
FILE="usr/share/documentation/404.png"
FILENAME="404.png"

BODY=$(mktemp)

{
    printf -- "--%s\r\n" "$BOUNDARY"
    printf 'Content-Disposition: form-data; name="file"; filename="%s"\r\n' "$FILENAME"
    printf 'Content-Type: image/png\r\n\r\n'
    cat "$FILE"
    printf "\r\n--%s--\r\n" "$BOUNDARY"
} > "$BODY"

LENGTH=$(stat -c %s "$BODY")

{
    printf 'POST /api HTTP/1.1\r\n'
    printf 'Host: %s\r\n' "$HOST"
    printf 'Content-Type: multipart/form-data; boundary=%s\r\n' "$BOUNDARY"
    printf 'Content-Length: %d\r\n' "$LENGTH"
    printf '\r\n'
    cat "$BODY"
} | nc localhost 8080

rm "$BODY"