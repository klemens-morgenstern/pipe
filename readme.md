# Pipe Library

An asio based library for everything pipe related. 
Includes a PoC for the `std::process` pipe design.

| *OS* | Pipe (Process) | Asio Pipe | Named Pipe | Pipe Server |
|------|----------------|-----------|------------|-------------|
| Windows | `CreatePipe` | `CreateNamedPipe` | `CreateNamedPipe` | `CreateNamedPipe` |
| Unix | `pipe` | `pipe` | `mkfifo` | unix domain sockets |

# `std::pipe`  

```cpp
void == boost::process::pipe
std::pstream = boost::process::pstream
std::pipe == boost::process::async_pipe 
```

```
auto [rpstr, wpstr] = std::pstream();
auto [rp, wp] = std::pipe();
```