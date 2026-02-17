 #!/bin/bash
 
 {
  printf 'POST /api/ HTTP/1.1\r\n'
  printf 'Host: localhost:8080\r\n'
  printf 'Content-Type: multipart/form-data; boundary=----abcde\r\n'
  printf 'Transfer-Encoding: chunked\r\n\r\n'

  # Chunk 1 (7A)
  printf '7A\r\n------abcde\r\nContent-Disposition: form-data; name="file"; filename="test.html"\r\nContent-Type: text/html\r\n\r\n<h1>test</h1>\r\n\r\n'
  sleep 1

  # Chunk 2 (73)
  printf '73\r\n------abcde\r\nContent-Disposition: form-data; name="file"; filename="test.txt"\r\nContent-Type: text/plain\r\n\r\nHello!\r\n\r\n'
  sleep 1

  # Chunk 3 (0F)
  printf '0F\r\n------abcde--\r\n\r\n'
    sleep 1
    printf '0\r\n\r\n'

} | nc localhost 8080