# Format
1. Start Line
  - Method sp URI sp HTTP/version CRLF
  - (CRLF = \r\n)
2. Headers or field line
  - name: OWF content OWF
  - OWF = space ad tab
  - name is case insensitive
  - content is case insensitive for host, content-type, tranfer-encoding, connection
  - Host - (Required in HTTP/1.1; used for virtual hosts)
  - Content-Length - Required to read request body correctly
  - Content-Type - Needed to interpret request body (JSON, form, etc.)
  - User-Agent - Not required, but useful for debugging
  - Accept - Helps decide response format
  - Transfer-Encoding: chunked (in this case ignore Content-Length)
    - nb of bytes\r\n[...]\r\n .... 0\r\n\r\n
    - nb of bytes = chunk isze in hexa so if c = 12
    - as long as we don't see the 0\r\n\r state hould be READING_CHUNK
    - if before the request is not well formed before we identify the transfer encoding is chunked -> close the clientfd just in case

  - Decision to take:
    - What if I have Content-Lentgh or Transfer-Encoding but no body ? -> if it is 0 ok otherwise 400
3. message body
  - needed for POST
  - ignored for GET

# GET

# POST

- *Returns appropriate status code depending on the result of processing the POST request ???* Pas compris
  - 206: Partial Content
  - 304: Not Modified
  - 416: Range not Satisfiable
  - 201: Created (for a single or several resources created on the origin of the server)
  - 200: OK (in case of a cache (wont handle))
  - 303: See Other

# DELETE

- Similar to the `rm` command in UNIX
*If the target ressource has one or more current reprensentations, they might not be destroyed by the origin server*
- 202: Accepted (Should be the response)
- 204: No Content
- 200: Ok
