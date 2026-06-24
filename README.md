*This project has been created as part of the 42 curriculum by [Mohammed Elmahfoudi (maelmahf)](https://profile-v3.intra.42.fr/users/maelmahf), [Mohamed Ben Talha (mohben-t)](https://profile-v3.intra.42.fr/users/mohben-t), [Anass Elboussaili (aelbouss)](https://profile-v3.intra.42.fr/users/aelbouss).*

---

# Webserv

## Description

**Webserv** is a fully functional HTTP/1.1 web server written from scratch in **C++98**, inspired by the architecture and configuration style of [nginx](https://nginx.org/). It is a core milestone of the 42 school curriculum, designed to give students a deep understanding of how web servers work under the hood.

The server handles real-world HTTP communication in a **single-threaded, non-blocking** event loop powered by `poll()`. It supports multiple virtual hosts, location-based routing, CGI script execution, file uploads, static file serving, and persistent connections — all without relying on threads or blocking I/O on the main loop.

### Key Features

- **Non-blocking I/O** via `poll()` — single event loop, no threads, no blocking syscalls
- **HTTP/1.1** compliance: persistent connections (`keep-alive`), correct status codes, headers
- **Multiple virtual servers** with independent `listen`, `server_name`, `root`, and `error_page` directives
- **Location blocks** with per-route method restrictions, `autoindex`, `index`, and `return` (redirect)
- **CGI support** (Python, Bash, PHP) — fully asynchronous, pipe fds registered in the poll loop
- **CGI timeout** — unresponsive child processes are killed and reaped after 10 seconds; a `504 Gateway Timeout` is returned
- **File uploads** via `POST` with `client_max_body_size` enforcement
- **Static file serving** with auto-generated directory listings (`autoindex on`)
- **HTTP Range requests** — `206 Partial Content` for video streaming
- **Session & cookie management** — multiple `Set-Cookie` headers, `HttpOnly` flag support
- **Custom error pages** per server block
- **Graceful shutdown** with `SIGINT`/`SIGQUIT` handling

---

## Instructions

### Requirements

- A C++98-compatible compiler (`c++` or `g++`)
- GNU `make`
- A POSIX-compliant OS (Linux or macOS)

### Compilation

```bash
git clone https://github.com/aelbouss/Webserv.git
cd Webserv
make
```

This produces a `webserv` binary in the project root. The build uses `-Wall -Wextra -Werror` flags throughout.

### Running the Server

```bash
./webserv [config_file]
```

If no config file is provided, the server falls back to `configs/default.conf`.

**Example:**

```bash
./webserv configs/default.conf
```

Then open your browser at `http://localhost:8002/` or test with `curl`:

```bash
curl -v --http1.1 http://localhost:8002/
```

### Configuration File Format

The configuration syntax follows an nginx-inspired block structure:

```nginx
server {
    listen       8080;
    server_name  localhost;
    host         127.0.0.1;
    root         docs/fusion_web/;
    client_max_body_size 3000000;
    index        index.html;
    error_page   404 error_pages/404.html;

    location / {
        allow_methods  GET POST DELETE;
        autoindex      off;
    }

    location /uploads {
        allow_methods  GET POST;
        autoindex      on;
    }

    location /cgi-bin {
        allow_methods  GET POST;
        cgi_extension  .py .sh;
    }
}
```

**Supported directives:**

| Directive | Scope | Description |
|---|---|---|
| `listen` | server | Port to bind |
| `server_name` | server | Virtual host name |
| `host` | server | IP address to bind |
| `root` | server / location | Document root |
| `index` | server / location | Default index file |
| `client_max_body_size` | server | Maximum request body size (bytes) |
| `error_page` | server | Custom error page per status code |
| `allow_methods` | location | Allowed HTTP methods (GET, POST, DELETE) |
| `autoindex` | location | Directory listing (`on` / `off`) |
| `cgi_extension` | location | File extensions handled by CGI |
| `return` | location | HTTP redirect to another path |

### Stress Testing

```bash
# Install siege
apt-get install siege   # Debian/Ubuntu
brew install siege      # macOS

# Run a load test (255 concurrent users for 30 seconds)
siege -c255 -t30S http://localhost:8002/
```

Target: **≥ 99.5% availability** with no crashes or hung connections.

### Cleanup

```bash
make clean    # remove object files
make fclean   # remove object files and binary
make re       # full rebuild
```

---

## Resources

### HTTP & Networking

- [RFC 7230 — HTTP/1.1: Message Syntax and Routing](https://datatracker.ietf.org/doc/html/rfc7230)
- [RFC 7231 — HTTP/1.1: Semantics and Content](https://datatracker.ietf.org/doc/html/rfc7231)
- [RFC 7233 — HTTP/1.1: Range Requests](https://datatracker.ietf.org/doc/html/rfc7233)
- [Beej's Guide to Network Programming](https://beej.us/guide/bgnet/)
- [MDN Web Docs — HTTP](https://developer.mozilla.org/en-US/docs/Web/HTTP)
- [nginx documentation](https://nginx.org/en/docs/)

### System Calls & I/O Multiplexing

- `man poll`, `man socket`, `man accept`, `man recv`, `man send`
- `man fork`, `man execve`, `man waitpid`, `man pipe`
- [The C10K Problem — Dan Kegel](http://www.kegel.com/c10k.html)
- [IBM — Non-blocking I/O and select()](https://www.ibm.com/docs/en/i/7.5?topic=designs-using-poll-instead-select)

### CGI

- [RFC 3875 — The Common Gateway Interface (CGI/1.1)](https://datatracker.ietf.org/doc/html/rfc3875)

### Tutorials & Articles

- [How to build a simple HTTP server in C](https://medium.com/from-the-scratch/http-server-what-do-you-need-to-know-to-build-a-simple-http-server-from-scratch-d1ef8945e4fa)
- [Writing a Web Server from Scratch (series)](https://www.codequoi.com/en/creating-and-killing-child-processes-in-c/)

---

### AI Usage

AI was used to help us understand the project structure and clarify some concepts.

It also helped us fix small mistakes during development.
