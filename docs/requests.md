# Request
An HTTP request is a message a client sends to a server to ask it to perform an action on a resource.
## Request Format

### 1. Request Line

The request line is the first line of an HTTP request. It defines **what action is being requested**, **on which resource**, and **which HTTP version is being used**.

**Format:**

```
<Method> <URI> <HTTP-Version>\r\n
```

* `\r\n` represents a **CRLF (Carriage Return + Line Feed)** and marks the end of the line.
* All parts of the start line are separated by a single space (`SP`).

### Supported HTTP Versions

The server accepts the following HTTP versions:

* `HTTP/0`
* `HTTP/1.1`

Any other HTTP version will be rejected.

### Supported Methods

The server currently supports the following HTTP methods:

#### GET

* Retrieves a resource from the server.

#### POST

* Sends data to the server.
* Used to create resources.
* The request body contains the data being submitted.

#### DELETE

* Removes a specified resource from the server.
* The target resource is identified by the URI.

Requests using unsupported methods will result in an error response.

---

### 2. Headers (Field Lines)

Headers provide additional information about the request.
Each header is written on its own line and follows this format:

```
<name>:<OWF><content><OWF>\r\n
```

* `OWF` (Optional Whitespace) = space (` `) or horizontal tab (`\t`)
* Header lines are terminated by `\r\n`
* Header names are **case-insensitive**

#### Empty Header Values

* A header **may have empty content**:

  ```
  Header-Name:
  ```

  or

  ```
  Header-Name: <OWF>
  ```
* The following headers **must NOT be empty**:

  * `Content-Type`
  * `Content-Length`
  * `Transfer-Encoding`

#### Duplicate Headers

The following headers **must not appear more than once** in a request:

* `Host`
* `User-Agent`
* `Content-Length`
* `Transfer-Encoding`
* `Content-Type`
* `Connection`

Duplicate occurrences result in a `400 Bad Request`.

#### Case Sensitivity Rules

* Header **names** are case-insensitive.
* Header **values**:

  * Case-insensitive for:

    * `Host`
    * `Content-Type` (except multipart boundary)
    * `Transfer-Encoding`
    * `Connection`
  * Case-sensitive for:

    * `User-Agent`
    * `Content-Type` boundary parameter

#### Description of Headers

##### Host

* **Required in HTTP/1.1**
* Used to determine the target virtual host.

##### Content-Length

* Specifies the size of the request body in bytes.
* Required to correctly read the request body when no `Transfer-Encoding` is present.

##### Content-Type

* Describes the format of the request body (e.g. JSON, form data).
* Required when a request body is present.

##### Transfer-Encoding

* Indicates that the body is sent in chunks (e.g. `chunked`).
* Cannot be used together with `Content-Length`, which will be ignored.

##### User-Agent

* Identifies the client making the request.
* Not required, but useful for logging and debugging.

##### Accept ?????

* Indicates which response formats the client can handle.
* Helps the server decide the response content type.

---

#### Server Processing Logic

After parsing headers, the server must:

1. Determine connection behavior:

   * `Connection: keep-alive`
   * `Connection: close`

2. Validate the `Host` header.

3. Match the request to a server block using:

   * IP address
   * Port
   * Server Name (virtual hosting)

4. Resolve the request location:

   * Start with the full path
   * Reduce path segments until a matching location is found

5. Check for `return` directive:

   * If present, it has priority and ends processing

6. Determine whether the request should be handled by CGI.

---

### 3. Message Body

The message body contains the data sent by the client.

#### General Rules

* A message body is **required for `POST` requests**.
* A message body is **ignored for `GET` and `DELETE` requests**.
* A body may arrive in **multiple read calls**.
* If a body is present but the `Content-Type` is **not supported**, return **`415 Unsupported Media Type`**.

---

### Transfer-Encoding: chunked

When `Transfer-Encoding: chunked` is present:

* **Ignore `Content-Length` if it exists**
* The body is sent in chunks using the following format:

```
<chunk-size-in-hex>\r\n
<chunk-data>\r\n
...
0\r\n
\r\n
```

* `chunk-size` is expressed in **hexadecimal**

  * Example: `c` = 12 bytes
* The server must remain in `READING_BODY` state **until** the terminating:

  ```
  0\r\n\r\n
  ```

  is received.

* If the request is detected as malformed **before** identifying `Transfer-Encoding: chunked`: Immediately **close the client file descriptor**

---

### Non-Chunked Body

#### Standard Body (Content-Length)

* If `Transfer-Encoding` is not present:

  * The body size must match `Content-Length`
* If `Content-Length` is missing when a body is required:

  * Return **`411 Length Required`**

---

### Multipart Body

#### Content-Type

```
Content-Type: multipart/form-data; boundary=<boundary>
```

* `multipart/form-data` is **case-insensitive**
* `boundary` parameter name is **case-insensitive**
* Boundary value is **case-sensitive**
* Boundary may contain only:

  * letters
  * digits
  * `. _ -`

#### Multipart Format

```
--<boundary>\r\n
<headers>\r\n
\r\n
<body>\r\n
--<boundary>\r\n
...
--<boundary>--\r\n (as many)
```

#### Multipart Headers

* `Content-Disposition` **must be present**

| Element                             | Case rule   |
| ----------------------------------- | ----------- |
| Header name                    | Insensitive |
| `Content-Disposition` token         | Insensitive |
| Parameter name (`name`, `filename`) | Insensitive |
| Parameter value                     | Sensitive   |
| Media type (`text/plain`)           | Insensitive |

* Required format:

  ```
  Content-Disposition: form-data; name=<key>
  ```
* For file uploads:

  * `filename` parameter must be present
  * `Content-Type` must be validated against the filename

---

#### Mixed Cases

* **Multipart + chunked**

  * Treated as chunked
  * Ignore `Content-Length`
* **Multipart only**

  * Validate against `Content-Length`
* **Body without multipart**

  * Validate against `Content-Length`

---

### Error Summary

| Error Condition                     | Status Code |
| ----------------------------------- | ----------- |
| Malformed request                   | 400         |
| Unsupported Content-Type            | 415         |
| Body size exceeds maximum limit     | 413         |
| Content-Length required but missing | 411         |
| Method not allowed                  | 405         |

---

---

## Method Handlers

### Path Resolution (Common to All Methods)

* If the request uses the **root location**:

  ```
  resolved_path = root + URI (+ trailing slash if present)
  ```
* If the request uses an **alias**:

  ```
  resolved_path = alias + URI (+ trailing slash if present)
  ```

Trailing slashes are preserved and affect directory handling.

---

### GET

#### Autoindex Behavior

| Autoindex | Index file | Request      | Result                      |
| --------- | ---------- | ------------ | --------------------------- |
| Disabled  | Absent     | `GET /test/` | 403 Forbidden               |
| Enabled   | Absent     | `GET /test/` | 200 OK + HTML listing       |
| Disabled  | Present    | `GET /test/` | 200 OK + index file content |
| Enabled   | Present    | `GET /test/` | 200 OK + index file content |

---

#### GET Request Processing

##### If the URI ends with a trailing slash

* Resolve the full path
* If the path is a **directory**:

  * Check directory permissions → `403 Forbidden` if denied
  * Look for an index file

    * If found and readable → serve it
    * If not found:

      * If autoindex is enabled → generate listing
      * Otherwise → `403 Forbidden`
* If the path is a **file**:

  * Check parent directory permissions

    * Non-existent → `404 Not Found`
    * No access → `403 Forbidden`
  * Check file permissions and serve

---

##### If the URI does NOT end with a trailing slash

* Check permissions on the resolved path
* If access denied → `403 Forbidden`
* Attempt to serve index file

  * If found → serve
  * If not found:

    * If autoindex enabled → generate listing
    * Otherwise → `403 Forbidden`

---

### POST

#### General Rules

* `POST` **requires a request body**
* Requests **must not end with a trailing**
* If the body exists but `Content-Type` is missing or unsupported → `415 Unsupported Media Type`

---

#### Multipart POST (`multipart/form-data`)

* Validate location configuration
* Parse multipart body
* For each file part:

  * Check if `filename` is present
  * If filename already exists:

    * ???
  * Create the file in the target location

---

#### Non-Multipart POST

* Validate supported `Content-Type`
* Determine file extension based on content type
* Resolve target location and check permissions
* Create a new file:

  * Name format: `upload_<n>`
  * `<n>` chosen to avoid collisions
* Write body content to file

---

### DELETE

#### General Rules

* URI must specify a **file or directory name**
* Permissions on the parent directory must allow deletion
* Behavior is similar to the UNIX `rm` command

---

#### DELETE Processing

* Resolve the target path
* If resource does not exist → `404 Not Found`
* If not permitted → `403 Forbidden`
* If target is a file:

  * Delete file
* If target is a directory:

  * Recursively delete contents
  * Then delete the directory itself


---

#### DELETE Response Codes

| Situation                    | Status           |
| ---------------------------- | ---------------- |
| No resource specified        | 400 Bad Request  |
| Resource doesn’t exist       | 404 Not Found    |
| Authenticated, no permission | 403 Forbidden    |
| Delete succeeded             | 204 No Content   |


---

## Implementation Checklist

- [ ] DELETE: verify root directory is a folder
- [ ] DELETE: handle non-regular files (symlink, socket, etc.)
- [ ] `stat()` protection
- [ ] Overflow protection
- [ ] If `Content-Length` or `Transfer-Encoding` exists but no body: `0` → OK // `>0` → 400 Bad Request
- [ ] If body exists, require `Content-Type`
- [ ] Support reading body in multiple reads
- [ ] Multipart header case-sensitivity rules
- [ ] Multipart: ensure `filename` is present when required
