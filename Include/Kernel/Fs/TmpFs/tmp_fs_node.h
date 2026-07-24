#pragma once
#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Container/vector.h>

class TmpFsNode : public Node {
public:
  virtual ~TmpFsNode() override = default;
  virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buffer) override;
  virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t offset, size_t size, const uint8_t* buffer) override;
  virtual size_t size() const override;

protected:
  fk::containers::Vector<uint8_t> m_data;
};
