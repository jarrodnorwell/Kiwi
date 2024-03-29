#include "cpu/cpu.hpp"

bool CPU::implied(MainBus& bus, std::uint8_t opcode) {
    switch (static_cast<OperationImplied>(opcode)) {
    case NOP:
        break;
    case BRK:
        interrupt(bus, BRK_INTERRUPT);
        break;
    case JSR:
        // Push address of next instruction - 1, thus register_PC + 1
        // instead of register_PC + 2 since register_PC and
        // register_PC + 1 are address of subroutine
        push_stack(bus, static_cast<std::uint8_t>((register_PC + 1) >> 8));
        push_stack(bus, static_cast<std::uint8_t>(register_PC + 1));
        register_PC = read_address(bus, register_PC);
        break;
    case RTS:
        register_PC = pop_stack(bus);
        register_PC |= pop_stack(bus) << 8;
        ++register_PC;
        break;
    case RTI: {
        flags.byte = pop_stack(bus);
    }
            register_PC = pop_stack(bus);
            register_PC |= pop_stack(bus) << 8;
            break;
    case JMP:
        register_PC = read_address(bus, register_PC);
        break;
    case JMPI: {
        std::uint16_t location = read_address(bus, register_PC);
        // 6502 has a bug such that the when the vector of an indirect
        // address begins at the last byte of a page, the second byte
        // is fetched from the beginning of that page rather than the
        // beginning of the next
        // Recreating here:
        std::uint16_t Page = location & 0xff00;
        register_PC = bus.read(location) |
            bus.read(Page | ((location + 1) & 0xff)) << 8;
    }
             break;
    case PHP: {
        push_stack(bus, flags.byte);
    }
            break;
    case PLP: {
        flags.byte = pop_stack(bus);
    }
            break;
    case PHA:
        push_stack(bus, register_A);
        break;
    case PLA:
        register_A = pop_stack(bus);
        set_ZN(register_A);
        break;
    case DEY:
        --register_Y;
        set_ZN(register_Y);
        break;
    case DEX:
        --register_X;
        set_ZN(register_X);
        break;
    case TAY:
        register_Y = register_A;
        set_ZN(register_Y);
        break;
    case INY:
        ++register_Y;
        set_ZN(register_Y);
        break;
    case INX:
        ++register_X;
        set_ZN(register_X);
        break;
    case CLC:
        flags.bits.C = false;
        break;
    case SEC:
        flags.bits.C = true;
        break;
    case CLI:
        flags.bits.I = false;
        break;
    case SEI:
        flags.bits.I = true;
        break;
    case CLD:
        flags.bits.D = false;
        break;
    case SED:
        flags.bits.D = true;
        break;
    case TYA:
        register_A = register_Y;
        set_ZN(register_A);
        break;
    case CLV:
        flags.bits.V = false;
        break;
    case TXA:
        register_A = register_X;
        set_ZN(register_A);
        break;
    case TXS:
        register_SP = register_X;
        break;
    case TAX:
        register_X = register_A;
        set_ZN(register_X);
        break;
    case TSX:
        register_X = register_SP;
        set_ZN(register_X);
        break;
    default:
        return false;
    };
    return true;
}

bool CPU::branch(MainBus& bus, std::uint8_t opcode) {
    if ((opcode & BRANCH_INSTRUCTION_MASK) == BRANCH_INSTRUCTION_MASK_RESULT) {
        // branch is initialized to the condition required (for the flag
        // specified later)
        bool branch = opcode & BRANCH_CONDITION_MASK;

        // set branch to true if the given condition is met by the given flag
        // We use xnor here, it is true if either both operands are true or
        // false
        switch (opcode >> BRANCH_ON_FLAG_SHIFT) {
        case NEGATIVE:
            branch = !(branch ^ flags.bits.N);
            break;
        case OVERFLOW:
            branch = !(branch ^ flags.bits.V);
            break;
        case CARRY:
            branch = !(branch ^ flags.bits.C);
            break;
        case ZERO:
            branch = !(branch ^ flags.bits.Z);
            break;
        default:
            return false;
        }

        if (branch) {
            int8_t offset = bus.read(register_PC++);
            ++skip_cycles;
            auto newPC = static_cast<std::uint16_t>(register_PC + offset);
            set_page_crossed(register_PC, newPC, 2);
            register_PC = newPC;
        }
        else
            ++register_PC;
        return true;
    }
    return false;
}

bool CPU::type0(MainBus& bus, std::uint8_t opcode) {
    if ((opcode & INSTRUCTION_MODE_MASK) == 0x0) {
        std::uint16_t location = 0;
        switch (static_cast<AddrMode2>((opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT)) {
        case M2_IMMEDIATE:
            location = register_PC++;
            break;
        case M2_ZERO_PAGE:
            location = bus.read(register_PC++);
            break;
        case M2_ABSOLUTE:
            location = read_address(bus, register_PC);
            register_PC += 2;
            break;
        case M2_INDEXED:
            // Address wraps around in the zero page
            location = (bus.read(register_PC++) + register_X) & 0xff;
            break;
        case M2_ABSOLUTE_INDEXED:
            location = read_address(bus, register_PC);
            register_PC += 2;
            set_page_crossed(location, location + register_X);
            location += register_X;
            break;
        default:
            return false;
        }
        std::uint16_t operand = 0;
        switch (static_cast<Operation0>((opcode & OPERATION_MASK) >> OPERATION_SHIFT)) {
        case BIT:
            operand = bus.read(location);
            flags.bits.Z = !(register_A & operand);
            flags.bits.V = operand & 0x40;
            flags.bits.N = operand & 0x80;
            break;
        case STY:
            bus.write(location, register_Y);
            break;
        case LDY:
            register_Y = bus.read(location);
            set_ZN(register_Y);
            break;
        case CPY: {
            std::uint16_t diff = register_Y - bus.read(location);
            flags.bits.C = !(diff & 0x100);
            set_ZN(diff);
        }
                break;
        case CPX: {
            std::uint16_t diff = register_X - bus.read(location);
            flags.bits.C = !(diff & 0x100);
            set_ZN(diff);
        }
                break;
        default:
            return false;
        }

        return true;
    }
    return false;
}

bool CPU::type1(MainBus& bus, std::uint8_t opcode) {
    if ((opcode & INSTRUCTION_MODE_MASK) == 0x1) {
        std::uint16_t location = 0; //Location of the operand, could be in RAM
        auto op = static_cast<Operation1>((opcode & OPERATION_MASK) >> OPERATION_SHIFT);
        switch (static_cast<AddrMode1>((opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT)) {
        case M1_INDEXED_INDIRECT_X: {
            std::uint8_t zero_address = register_X + bus.read(register_PC++);
            //Addresses wrap in zero page mode, thus pass through a mask
            location = bus.read(zero_address & 0xff) | bus.read((zero_address + 1) & 0xff) << 8;
        }
                                  break;
        case M1_ZERO_PAGE:
            location = bus.read(register_PC++);
            break;
        case M1_IMMEDIATE:
            location = register_PC++;
            break;
        case M1_ABSOLUTE:
            location = read_address(bus, register_PC);
            register_PC += 2;
            break;
        case M1_INDIRECT_Y: {
            std::uint8_t zero_address = bus.read(register_PC++);
            location = bus.read(zero_address & 0xff) | bus.read((zero_address + 1) & 0xff) << 8;
            if (op != STA)
                set_page_crossed(location, location + register_Y);
            location += register_Y;
        }
                          break;
        case M1_INDEXED_X:
            // Address wraps around in the zero page
            location = (bus.read(register_PC++) + register_X) & 0xff;
            break;
        case M1_ABSOLUTE_Y:
            location = read_address(bus, register_PC);
            register_PC += 2;
            if (op != STA)
                set_page_crossed(location, location + register_Y);
            location += register_Y;
            break;
        case M1_ABSOLUTE_X:
            location = read_address(bus, register_PC);
            register_PC += 2;
            if (op != STA)
                set_page_crossed(location, location + register_X);
            location += register_X;
            break;
        default:
            return false;
        }

        switch (op) {
        case ORA:
            register_A |= bus.read(location);
            set_ZN(register_A);
            break;
        case AND:
            register_A &= bus.read(location);
            set_ZN(register_A);
            break;
        case EOR:
            register_A ^= bus.read(location);
            set_ZN(register_A);
            break;
        case ADC: {
            std::uint8_t operand = bus.read(location);
            std::uint16_t sum = register_A + operand + flags.bits.C;
            //Carry forward or UNSIGNED overflow
            flags.bits.C = sum & 0x100;
            //SIGNED overflow, would only happen if the sign of sum is
            //different from BOTH the operands
            flags.bits.V = (register_A ^ sum) & (operand ^ sum) & 0x80;
            register_A = static_cast<std::uint8_t>(sum);
            set_ZN(register_A);
        }
                break;
        case STA:
            bus.write(location, register_A);
            break;
        case LDA:
            register_A = bus.read(location);
            set_ZN(register_A);
            break;
        case SBC: {
            //High carry means "no borrow", thus negate and subtract
            std::uint16_t subtrahend = bus.read(location),
                diff = register_A - subtrahend - !flags.bits.C;
            //if the ninth bit is 1, the resulting number is negative => borrow => low carry
            flags.bits.C = !(diff & 0x100);
            //Same as ADC, except instead of the subtrahend,
            //substitute with it's one complement
            flags.bits.V = (register_A ^ diff) & (~subtrahend ^ diff) & 0x80;
            register_A = diff;
            set_ZN(diff);
        }
                break;
        case CMP: {
            std::uint16_t diff = register_A - bus.read(location);
            flags.bits.C = !(diff & 0x100);
            set_ZN(diff);
        }
                break;
        default:
            return false;
        }
        return true;
    }
    return false;
}

bool CPU::type2(MainBus& bus, std::uint8_t opcode) {
    if ((opcode & INSTRUCTION_MODE_MASK) == 2) {
        std::uint16_t location = 0;
        auto op = static_cast<Operation2>((opcode & OPERATION_MASK) >> OPERATION_SHIFT);
        auto address_mode =
            static_cast<AddrMode2>((opcode & ADRESS_MODE_MASK) >> ADDRESS_MODE_SHIFT);
        switch (address_mode) {
        case M2_IMMEDIATE:
            location = register_PC++;
            break;
        case M2_ZERO_PAGE:
            location = bus.read(register_PC++);
            break;
        case M2_ACCUMULATOR:
            break;
        case M2_ABSOLUTE:
            location = read_address(bus, register_PC);
            register_PC += 2;
            break;
        case M2_INDEXED: {
            location = bus.read(register_PC++);
            std::uint8_t index;
            if (op == LDX || op == STX)
                index = register_Y;
            else
                index = register_X;
            //The mask wraps address around zero page
            location = (location + index) & 0xff;
        }
                       break;
        case M2_ABSOLUTE_INDEXED: {
            location = read_address(bus, register_PC);
            register_PC += 2;
            std::uint8_t index;
            if (op == LDX || op == STX)
                index = register_Y;
            else
                index = register_X;
            set_page_crossed(location, location + index);
            location += index;
        }
                                break;
        default:
            return false;
        }

        std::uint16_t operand = 0;
        switch (op) {
        case ASL:
        case ROL:
            if (address_mode == M2_ACCUMULATOR) {
                auto prev_C = flags.bits.C;
                flags.bits.C = register_A & 0x80;
                register_A <<= 1;
                //If Rotating, set the bit-0 to the the previous carry
                register_A = register_A | (prev_C && (op == ROL));
                set_ZN(register_A);
            }
            else {
                auto prev_C = flags.bits.C;
                operand = bus.read(location);
                flags.bits.C = operand & 0x80;
                operand = operand << 1 | (prev_C && (op == ROL));
                set_ZN(operand);
                bus.write(location, operand);
            }
            break;
        case LSR:
        case ROR:
            if (address_mode == M2_ACCUMULATOR) {
                auto prev_C = flags.bits.C;
                flags.bits.C = register_A & 1;
                register_A >>= 1;
                //If Rotating, set the bit-7 to the previous carry
                register_A = register_A | (prev_C && (op == ROR)) << 7;
                set_ZN(register_A);
            }
            else {
                auto prev_C = flags.bits.C;
                operand = bus.read(location);
                flags.bits.C = operand & 1;
                operand = operand >> 1 | (prev_C && (op == ROR)) << 7;
                set_ZN(operand);
                bus.write(location, operand);
            }
            break;
        case STX:
            bus.write(location, register_X);
            break;
        case LDX:
            register_X = bus.read(location);
            set_ZN(register_X);
            break;
        case DEC: {
            auto tmp = bus.read(location) - 1;
            set_ZN(tmp);
            bus.write(location, tmp);
        }
                break;
        case INC: {
            auto tmp = bus.read(location) + 1;
            set_ZN(tmp);
            bus.write(location, tmp);
        }
                break;
        default:
            return false;
        }
        return true;
    }
    return false;
}

void CPU::reset(std::uint16_t start_address) {
    skip_cycles = cycles = 0;
    register_A = register_X = register_Y = 0;
    flags.bits.I = true;
    flags.bits.C = flags.bits.D = flags.bits.N = flags.bits.V = flags.bits.Z = false;
    register_PC = start_address;
    register_SP = 0xfd; //documented startup state
}

void CPU::interrupt(MainBus& bus, InterruptType type) {
    if (flags.bits.I && type != NMI_INTERRUPT && type != BRK_INTERRUPT)
        return;
    // Add one if BRK, a quirk of 6502
    if (type == BRK_INTERRUPT)
        ++register_PC;
    // push values on to the stack
    push_stack(bus, register_PC >> 8);
    push_stack(bus, register_PC);
    push_stack(bus, flags.byte | 0b00100000 | (type == BRK_INTERRUPT) << 4);
    // set the interrupt flag
    flags.bits.I = true;
    // handle the kind of interrupt
    switch (type) {
    case IRQ_INTERRUPT:
    case BRK_INTERRUPT:
        register_PC = read_address(bus, IRQ_VECTOR);
        break;
    case NMI_INTERRUPT:
        register_PC = read_address(bus, NMI_VECTOR);
        break;
    }
    // add the number of cycles to handle the interrupt
    skip_cycles += 7;
}

void CPU::cycle(MainBus& bus) {
    // increment the number of cycles
    ++cycles;
    // if in a skip cycle, return
    if (skip_cycles-- > 1)
        return;
    // reset the number of skip cycles to 0
    skip_cycles = 0;
    // read the opcode from the bus and lookup the number of cycles
    std::uint8_t op = bus.read(register_PC++);
    // Using short-circuit evaluation, call the other function only if the
    // first failed. ExecuteImplied must be called first and ExecuteBranch
    // must be before ExecuteType0
    if (implied(bus, op) || branch(bus, op) || type1(bus, op) || type2(bus, op) || type0(bus, op))
        skip_cycles += OPERATION_CYCLES[op];
    else
        return; // std::cout << "failed to execute opcode: " << std::hex << +op << std::endl;
}