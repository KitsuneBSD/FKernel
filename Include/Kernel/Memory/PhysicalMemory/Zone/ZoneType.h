#pragma once 

enum class ZoneType {
    DMA, // <= 16MB
    NORMAL, // <= Common Random Access Memory
    HIGH, // <= Random Access Memory after 4GB
};