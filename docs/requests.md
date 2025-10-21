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
