#pragma once

#include <LibFK/Core/Result.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Types/types.h>

class ATATransferStrategy {
public:
    virtual ~ATATransferStrategy() = default;

    virtual fk::core::Result<size_t, fk::core::Error>
    read_sectors(uint64_t start_sector, size_t count, uint8_t* buffer) = 0;

    virtual fk::core::Result<size_t, fk::core::Error>
    write_sectors(uint64_t start_sector, size_t count, const uint8_t* buffer) = 0;

    virtual const char* name() const = 0;
};
