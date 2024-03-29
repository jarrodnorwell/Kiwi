#pragma once

#include "mappers/mapper.hpp"

class MapperCNROM : public Mapper {

private:
    /// whether there are 1 or 2 banks
    bool is_one_bank;
    /// TODO: what is this value
    std::uint16_t select_chr;

public:
    /// Create a new mapper with a cartridge.
    ///
    /// @param cart a reference to a cartridge for the mapper to access
    ///
    MapperCNROM(Cartridge& cart);

    /// Read a byte from the PRG RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in PRG RAM
    ///
    std::uint8_t readPRG(std::uint16_t address);

    /// Write a byte to an address in the PRG RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    inline void writePRG(std::uint16_t address, std::uint8_t value) { select_chr = value & 0x3; };

    /// Read a byte from the CHR RAM.
    ///
    /// @param address the 16-bit address of the byte to read
    /// @return the byte located at the given address in CHR RAM
    ///
    inline std::uint8_t readCHR(std::uint16_t address) { return cartridge.getVROM()[address | (select_chr << 13)]; };

    /// Write a byte to an address in the CHR RAM.
    ///
    /// @param address the 16-bit address to write to
    /// @param value the byte to write to the given address
    ///
    void writeCHR(std::uint16_t address, std::uint8_t value);

    /// Return the page pointer for the given address.
    ///
    /// @param address the address of the page pointer to get
    /// @return the page pointer at the given address
    ///
    const std::uint8_t* getPagePtr(std::uint16_t address);

};