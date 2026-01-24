#pragma once

class FileDescriptor {
  int m_fd;

public:
  explicit FileDescriptor(int fd) : m_fd(fd) {}
  int value() const { return m_fd; }
  bool is_valid() const { return m_fd >= 0; }
};
