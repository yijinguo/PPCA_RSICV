//一级流水，用于对拍

#include <iostream>

class CPU{
private:
    using uint = unsigned int;
    using uc = unsigned char;

    static uint str_to_uint(const std::string &str) {
        uint ans = 0;
        for (int i = 0; i < str.length(); ++i) {
            ans <<= 4;
            if (str[i] >= '0' && str[i] <= '9') ans += str[i] - '0';
            else ans += str[i] - 'A' + 10;
        }
        return ans;
    }

    static uint ch_to_uint(char s) {
        return (s >= '0' && s <= '9') ? (s - '0') : (s - 'A' + 10);
    }

    enum CommandFormat{
        R, I, S, B, U, UJ, None
    };

    enum CommandType{
        LUI = 0b0110111,
        AUIPC = 0b0010111,
        JAL = 0b1101111,
        JALR = 0b1100111,
        BEQ = 0b1100011 + (0b000 << 7),
        BNE = 0b1100011 + (0b001 << 7),
        BLT = 0b1100011 + (0b100 << 7),
        BGE = 0b1100011 + (0b101 << 7),
        BLTU = 0b1100011 + (0b110 << 7),
        BGEU = 0b1100011 + (0b111 << 7),
        LB = 0b0000011 + (0b000 << 7),
        LH = 0b0000011 + (0b001 << 7),
        LW = 0b0000011 + (0b010 << 7),
        LBU = 0b0000011 + (0b100 << 7),
        LHU = 0b0000011 + (0b101 << 7),
        SB = 0b0100011 + (0b000 << 7),
        SH = 0b0100011 + (0b001 << 7),
        SW = 0b0100011 + (0b010 << 7),
        ADDI = 0b0010011 + (0b000 << 7),
        SLTI = 0b0010011 + (0b010 << 7),
        SLTIU = 0b0010011 + (0b011 << 7),
        XORI = 0b0010011 + (0b100 << 7),
        ORI = 0b0010011 + (0b110 << 7),
        ANDI = 0b0010011 + (0b111 << 7),
        SLLI = 0b0010011 + (0b001 << 7),
        SRLI = 0b0010011 + (0b101 << 7) + (0b0000000 << 10),
        SRAI = 0b0010011 + (0b101 << 7) + (0b0100000 << 10),
        ADD = 0b0110011 + (0b000 << 7) + (0b0000000 << 10),
        SUB = 0b0110011 + (0b000 << 7) + (0b0100000 << 10),
        SLL = 0b0110011 + (0b001 << 7),
        SLT = 0b0110011 + (0b010 << 7),
        SLTU = 0b0110011 + (0b011 << 7),
        XOR = 0b0110011 + (0b100 << 7),
        SRL = 0b0110011 + (0b101 << 7) + (0b0000000 << 10),
        SRA = 0b0110011 + (0b101 << 7) + (0b0100000 << 10),
        OR = 0b0110011 + (0b110 << 7),
        AND = 0b0110011 + (0b111 << 7)
    };

    static uint get_opcode(uint cmd) {
        return cmd & (uint) 0x0000007f;
    }

    static uint get_funct3(uint cmd) {
        return (cmd & (uint) 0x00007000) >> 12;
    }

    static uint get_funct7(uint cmd) {
        return (cmd & (uint) 0xff000000) >> 25;
    }

    static uint get_rd(uint cmd) {
        return (cmd & (uint) 0x00000f80) >> 7;
    }

    static uint get_rs1(uint cmd){
        return (cmd & (uint) 0x000f8000) >> 15;
    }

    static uint get_rs2(uint cmd) {
        return (cmd & (uint) 0x01f00000) >> 20;
    }

    static uint get_immediate(uint cmd, CommandFormat format) {
        uint ret = 0;
        switch (format) {
            case R:
                break;
            case I:
                ret = (cmd & (0xfff00000)) >> 20;
                if ((cmd & (0x80000000)) == 0x80000000) ret |= (0xfffff000);
                break;
            case S:
                ret = (cmd & (0x00000f80)) >> 7;
                ret |= (cmd & (0xfe000000)) >> 20;
                if ((cmd & (0x80000000)) == 0x80000000) ret |= (0xfffff000);
                break;
            case B:
                ret = (cmd & (0x00000f00)) >> 7;
                ret |= (cmd & (0x7e000000)) >> 20;
                ret |= (cmd & (0x00000080)) << 4;
                //ret |= (cmd & (0x80000000)) >> 20;
                if ((cmd & (0x80000000)) == 0x80000000) ret |= (0xfffff000);
                break;
            case U:  //todo
                ret = (cmd & (0xfffff000));
                break;
            case UJ:
                ret = (cmd & (0x7fe00000)) >> 20;
                ret |= (cmd & (0x000ff000));
                ret |= (cmd & (0x00100000)) >> 9;
                //ret |= (cmd & (0x80000000)) >> 12;
                if ((cmd & (uint) 0x80000000) == 0x80000000)ret |= 0xfff00000;
                break;
            default:
                break;
        }
        return ret;
    }

    static uint get_shamt(uint cmd) {
        return (cmd & (uint) 0x01f00000) >> 20;
    }

    static uint sign_extend(int new_bit, uint imm) {
        if ((imm >> 31) == 1) {
            uint ret = 0, i = 32;
            do {
                ret |= (1 << i);
                i++;
            } while (i != new_bit);
            imm |= ret;
        }
        return imm;
    }

    static uint abs(uint imm) {
        if ((imm >> 31) == 1) {
            imm = ((imm - 1) ^ (0xffffffff));
        }
        return imm;
    }

    class Register{
    private:
        uint reg[32] = {0};
    public:
        uint PC {};
        uint &operator[](uint addr) {
            return reg[addr];
        }
        const uint &operator[](uint addr) const  {
            return reg[addr];
        }
    } Reg;

    class Memory{
    private:
        static constexpr uint MEMORY_SIZE = 5e5;
        uint memory[MEMORY_SIZE] = {0};
    public:
        void initialize(std::istream &is){
            char temp;
            uint address;
            std::string address_str;
            while (is >> temp) {
                if (temp == '@') {
                    is >> address_str;
                    address = str_to_uint(address_str);
                } else if (temp == '#') {
                    break;
                } else {
                    memory[address] |= ch_to_uint(temp) << 4;
                    is >> temp;
                    memory[address++] |= ch_to_uint(temp);
                }
            }
        }
        uint &operator[](uint addr) {
            return memory[addr];
        }
        const uint &operator[](uint addr) const  {
            return memory[addr];
        }
    } mem;

    struct SingleCommand{
        uint instruction = 0;
        CommandFormat format {};
        CommandType type {};
        uint rd {};
        uint rs1 {};
        uint rs2 {};
        uint shamt {};
        uint imm{};
    } singleCmd;

public:

    void execute(){
        mem.initialize(std::cin);
        Reg.PC = (0x00000000);
        while (true) {
            uint cmd = 0;
            uint loc = Reg.PC;
            cmd = mem[loc];
            cmd |= (mem[loc + 1] << 8);
            cmd |= (mem[loc + 2] << 16);
            cmd |= (mem[loc + 3] << 24);
            if (cmd == (0x0ff00513)) {
                std::cout << (Reg[10] & 255u) << '\n';
                break;
            }
            parse_single(cmd);
            std::cout << cmd << '\n';
            //execute_print();
            execute_single();
            if (Reg.PC == loc) Reg.PC +=4;
        }
    }

    uint sign_readIn(uint loc, int bite) {
        uint ans = 0;
        if (bite == 1) {
            ans |= mem[loc];
            if ((ans & (0x00000080)) == 0x00000080) //符号扩展
                ans |= 0xffffff00;
        } else if (bite == 2) {
            ans |= mem[loc];
            ans |= (mem[loc + 1] << 8);
            if ((ans & (0x00008000)) == 0x00008000)
                ans |= 0xffff0000;
        } else if (bite == 4) {
            ans |= mem[loc];
            ans |= (mem[loc + 1] << 8);
            ans |= (mem[loc + 2] << 16);
            ans |= (mem[loc + 3] << 24);
            if ((ans & (0x80000000)) == 0x80000000)
                ans |= 0x80000000;
        }
        return ans;
    }

    uint zero_readIn(uint loc, int bite) {
        uint ans = 0;
        if (bite == 1) {
            ans |= mem[loc];
        } else if (bite == 2) {
            ans |= mem[loc];
            ans |= (mem[loc + 1] << 8);
        } else if (bite == 4) {
            ans |= mem[loc];
            ans |= (mem[loc + 1] << 8);
            ans |= (mem[loc + 2] << 16);
            ans |= (mem[loc + 3] << 24);
        }
        ans |= 0x00000000;
        return ans;
    }

private:

    void parse_single(uint cmd){
        singleCmd.instruction = cmd;
        uint code = get_opcode(cmd);
        switch (code) {
            case 0b0110111:
            case 0b0010111:
            case 0b1101111:
            case 0b1100111:
                singleCmd.type = (CommandType) code;
                break;
            case 0b1100011:
            case 0b0000011:
            case 0b0100011:
                code |= (get_funct3(cmd) << 7);
                singleCmd.type = (CommandType) code;
                break;
            case 0b0010011:{
                uint f3 = get_funct3(cmd);
                code |= (f3 << 7);
                if (cmd == (0b101))
                    code |= (get_funct7(cmd) << 10);
                singleCmd.type = (CommandType) code;
                break;
            }
            default:
                code |= (get_funct3(cmd) << 7);
                code |= (get_funct7(cmd) << 10);
                singleCmd.type = (CommandType) code;
                break;
        }
        switch (singleCmd.type) {
            case LUI:
            case AUIPC:
                singleCmd.format = U;
                break;
            case JAL:
                singleCmd.format = UJ;
                break;
            case BEQ:
            case BNE:
            case BLT:
            case BGE:
            case BLTU:
            case BGEU:
                singleCmd.format = B;
                break;
            case JALR:
            case LB:
            case LH:
            case LW:
            case LBU:
            case LHU:
            case ADDI:
            case SLTI:
            case SLTIU:
            case XORI:
            case ORI:
            case ANDI:
            case SLLI:
            case SRLI:
            case SRAI:
                singleCmd.format = I;
                break;
            case SB:
            case SH:
            case SW:
                singleCmd.format = S;
                break;
            default:
                singleCmd.format = R;
                break;
        }
        singleCmd.rd = get_rd(cmd);
        singleCmd.rs1 = get_rs1(cmd);
        singleCmd.rs2 = get_rs2(cmd);
        singleCmd.shamt = get_shamt(cmd);
        singleCmd.imm = get_immediate(cmd, singleCmd.format);
    }

    void execute_single() {
        switch (singleCmd.type) {
            case LUI:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = singleCmd.imm;
                break;
            case AUIPC:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg.PC + singleCmd.imm;
                break;
            case JAL:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg.PC + uint(4);
                Reg.PC += singleCmd.imm;
                break;
            case JALR:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg.PC + uint(4);
                Reg.PC = Reg[singleCmd.rs1] + singleCmd.imm;
                break;
            case BEQ:
                if (Reg[singleCmd.rs1] == Reg[singleCmd.rs2])
                    Reg.PC += singleCmd.imm;
                break;
            case BNE:
                if (Reg[singleCmd.rs1] != Reg[singleCmd.rs2])
                    Reg.PC += singleCmd.imm;
                break;
            case BLT:
                if ((int)Reg[singleCmd.rs1] < (int)Reg[singleCmd.rs2])
                    Reg.PC += singleCmd.imm;
                break;
            case BGE:
                if ((int)Reg[singleCmd.rs1] >= (int)Reg[singleCmd.rs2])
                    Reg.PC += singleCmd.imm;
                break;
            case BLTU:
                if (Reg[singleCmd.rs1] < Reg[singleCmd.rs2])
                    Reg.PC += singleCmd.imm;
                break;
            case BGEU:
                if (Reg[singleCmd.rs1] >= Reg[singleCmd.rs2])
                    Reg.PC += singleCmd.imm;
                break;
            case LB: {
                uint loc = Reg[singleCmd.rs1] + singleCmd.imm;
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = sign_readIn(loc, 1);
                break;
            }
            case LH: {
                uint loc = Reg[singleCmd.rs1] + singleCmd.imm;
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = sign_readIn(loc, 2);
                break;
            }
            case LW: {
                uint loc = Reg[singleCmd.rs1] + singleCmd.imm;
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = sign_readIn(loc, 4);
                break;
            }
            case LBU: {
                uint loc = Reg[singleCmd.rs1] + singleCmd.imm;
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = zero_readIn(loc, 1);
                break;
            }
            case LHU: {
                uint loc = Reg[singleCmd.rs1] + singleCmd.imm;
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = zero_readIn(loc, 2);
                break;
            }
            case SB:
                mem[Reg[singleCmd.rs1] + singleCmd.imm] = (Reg[singleCmd.rs2] & (0x000000ff));
                break;
            case SH:
                mem[Reg[singleCmd.rs1] + singleCmd.imm] = (Reg[singleCmd.rs2] & (0x0000ffff));
                break;
            case SW:
                mem[Reg[singleCmd.rs1] + singleCmd.imm] = (Reg[singleCmd.rs2] & (0xffffffff));
                break;
            case ADDI:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] + singleCmd.imm;
                break;
            case SLT:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = ((int)Reg[singleCmd.rs1] < (int)Reg[singleCmd.rs2]) ? 1 : 0;
                break;
            case SLTU:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = (Reg[singleCmd.rs1] < Reg[singleCmd.rs2]) ? 1 : 0;
                break;
            case SLTI:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = ((int)Reg[singleCmd.rs1] < (int)singleCmd.imm) ? 1 : 0;
                break;
            case SLTIU:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = (Reg[singleCmd.rs1] < singleCmd.imm) ? 1 : 0;
                break;
            case ANDI:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] & singleCmd.imm;
                break;
            case ADD:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] + Reg[singleCmd.rs2];
                break;
            case SUB:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] - Reg[singleCmd.rs2];
                break;
            case SLL:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] << (Reg[singleCmd.rs2] & (0x0000001f));
                break;
            case SLLI:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] << singleCmd.shamt;
                break;
            case SRL:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] >> (Reg[singleCmd.rs2] & (0x0000001f));
                break;
            case SRLI:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] >> singleCmd.shamt;
                break;
            case SRA: {
                if (singleCmd.rd != 0) {
                    bool flag = true;
                    uint shamt = Reg[singleCmd.rs2] & (0x0000001f);
                    if ((Reg[singleCmd.rs1] & (0x80000000)) != 0x80000000) flag = false;
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] >> shamt;
                    if (flag) {
                        for (int i = 31; i > 31 - shamt; --i)
                            Reg[singleCmd.rd] |= (1 << i);
                    }
                }
                break;
            }
            case SRAI: {
                if (singleCmd.rd != 0) {
                    bool flag = true;
                    if ((Reg[singleCmd.rs1] & (0x80000000)) != 0x80000000) flag = false;
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] >> singleCmd.shamt;
                    if (flag) {
                        for (int i = 31; i > 31 - singleCmd.shamt; --i)
                            Reg[singleCmd.rd] |= (1 << i);
                    }
                }
                break;
            }
            case XOR:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] ^ Reg[singleCmd.rs2];
                break;
            case XORI:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] ^ singleCmd.imm;
                break;
            case OR:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] | Reg[singleCmd.rs2];
                break;
            case ORI:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] | singleCmd.imm;
                break;
            case AND:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] & Reg[singleCmd.rs2];
                break;
            default:
                break;
        }
    }

    void execute_print(){
        switch (singleCmd.type) {
            case LUI:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = singleCmd.imm;
                std::cout << "lui " << singleCmd.rd << ' ' << Reg[singleCmd.rd];
                break;
            case AUIPC:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg.PC + singleCmd.imm;
                std::cout << "auipc " << singleCmd.rd << ' ' << Reg[singleCmd.rd];
                break;
            case JAL:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg.PC + uint(4);
                Reg.PC += singleCmd.imm;
                std::cout << "jal " << singleCmd.rd << ',' << Reg.PC;
                break;
            case JALR:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg.PC + uint(4);
                Reg.PC = Reg[singleCmd.rs1] + singleCmd.imm;
                std::cout << "jalr " << singleCmd.rd << ',' << Reg.PC;
                break;
            case BEQ:
                if (Reg[singleCmd.rs1] == Reg[singleCmd.rs2])
                    Reg.PC += singleCmd.imm;
                std::cout << "beq " << singleCmd.rs1 << ',' << singleCmd.rs2 << ',' << Reg.PC;
                break;
            case BNE:
                if (Reg[singleCmd.rs1] != Reg[singleCmd.rs2])
                    Reg.PC += singleCmd.imm;
                std::cout << "bne " << singleCmd.rs1 << ',' << singleCmd.rs2 << ',' << Reg.PC;
                break;
            case BLT:
                if ((int)Reg[singleCmd.rs1] < (int)Reg[singleCmd.rs2])
                    Reg.PC += singleCmd.imm;
                std::cout << "blt " << singleCmd.rs1 << ',' << singleCmd.rs2 << ',' << Reg.PC;
                break;
            case BGE:
                if ((int)Reg[singleCmd.rs1] >= (int)Reg[singleCmd.rs2])
                    Reg.PC += singleCmd.imm;
                std::cout << "bge " << singleCmd.rs1 << ',' << singleCmd.rs2 << ',' << Reg.PC;
                break;
            case BLTU:
                if (Reg[singleCmd.rs1] < Reg[singleCmd.rs2])
                    Reg.PC += singleCmd.imm;
                std::cout << "bltu " << singleCmd.rs1 << ',' << singleCmd.rs2 << ',' << Reg.PC;
                break;
            case BGEU:
                if (Reg[singleCmd.rs1] >= Reg[singleCmd.rs2])
                    Reg.PC += singleCmd.imm;
                std::cout << "bgeu " << singleCmd.rs1 << ',' << singleCmd.rs2 << ',' << Reg.PC;
                break;
            case LB: {
                uint loc = Reg[singleCmd.rs1] + singleCmd.imm;
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = sign_readIn(loc, 1);
                std::cout << "lb " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.imm;
                break;
            }
            case LH: {
                uint loc = Reg[singleCmd.rs1] + singleCmd.imm;
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = sign_readIn(loc, 2);
                std::cout << "lh " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.imm;
                break;
            }
            case LW: {
                uint loc = Reg[singleCmd.rs1] + singleCmd.imm;
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = sign_readIn(loc, 4);
                std::cout << "lw " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.imm;
                break;
            }
            case LBU: {
                uint loc = Reg[singleCmd.rs1] + singleCmd.imm;
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = zero_readIn(loc, 1);
                std::cout << "lbu " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.imm;
                break;
            }
            case LHU: {
                uint loc = Reg[singleCmd.rs1] + singleCmd.imm;
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = zero_readIn(loc, 2);
                std::cout << "lhu " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.imm;
                break;
            }
            case SB:
                mem[Reg[singleCmd.rs1] + singleCmd.imm] = (Reg[singleCmd.rs2] & (0x000000ff));
                std::cout << "sb " << singleCmd.rs1 << ',' << singleCmd.imm << ' ' << singleCmd.rs2;
                break;
            case SH:
                mem[Reg[singleCmd.rs1] + singleCmd.imm] = (Reg[singleCmd.rs2] & (0x0000ffff));
                std::cout << "sh " << singleCmd.rs1 << ',' << singleCmd.imm << ' ' << singleCmd.rs2;
                break;
            case SW:
                mem[Reg[singleCmd.rs1] + singleCmd.imm] = (Reg[singleCmd.rs2] & (0xffffffff));
                std::cout << "sw " << singleCmd.rs1 << ',' << singleCmd.imm << ' ' << singleCmd.rs2;
                break;
            case ADDI:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] + singleCmd.imm;
                std::cout << "addi " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.imm;
                break;
            case SLT:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = ((int)Reg[singleCmd.rs1] < (int)Reg[singleCmd.rs2]) ? 1 : 0;
                std::cout << "slt " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.rs2;
                break;
            case SLTU:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = (Reg[singleCmd.rs1] < Reg[singleCmd.rs2]) ? 1 : 0;
                std::cout << "sltu " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.rs2;
                break;
            case SLTI:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = ((int)Reg[singleCmd.rs1] < (int)singleCmd.imm) ? 1 : 0;
                std::cout << "slti " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.imm;
                break;
            case SLTIU:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = (Reg[singleCmd.rs1] < singleCmd.imm) ? 1 : 0;
                std::cout << "sltiu " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.imm;
                break;
            case ANDI:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] & singleCmd.imm;
                std::cout << "andi " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.imm;
                break;
            case ADD:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] + Reg[singleCmd.rs2];
                std::cout << "add " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.rs2;
                break;
            case SUB:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] - Reg[singleCmd.rs2];
                std::cout << "sub " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.rs2;
                break;
            case SLL:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] << (Reg[singleCmd.rs2] & (0x0000001f));
                std::cout << "sll " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.rs2;
                break;
            case SLLI:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] << singleCmd.shamt;
                std::cout << "slli " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.rs2;
                break;
            case SRL:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] >> (Reg[singleCmd.rs2] & (0x0000001f));
                std::cout << "srl " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.rs2;
                break;
            case SRLI:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] >> singleCmd.shamt;
                std::cout << "srli " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.shamt;
                break;
            case SRA: {
                if (singleCmd.rd != 0) {
                    bool flag = true;
                    uint shamt = Reg[singleCmd.rs2] & (0x0000001f);
                    if ((Reg[singleCmd.rs1] & (0x80000000)) != 0x80000000) flag = false;
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] >> shamt;
                    if (flag) {
                        for (int i = 31; i > 31 - shamt; --i)
                            Reg[singleCmd.rd] |= (1 << i);
                    }
                }
                std::cout << "sra " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.rs2;
                break;
            }
            case SRAI: {
                if (singleCmd.rd != 0) {
                    bool flag = true;
                    if ((Reg[singleCmd.rs1] & (0x80000000)) != 0x80000000) flag = false;
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] >> singleCmd.shamt;
                    if (flag) {
                        for (int i = 31; i > 31 - singleCmd.shamt; --i)
                            Reg[singleCmd.rd] |= (1 << i);
                    }
                }
                std::cout << "srai " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.shamt;
                break;
            }
            case XOR:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] ^ Reg[singleCmd.rs2];
                std::cout << "xor " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.rs2;
                break;
            case XORI:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] ^ singleCmd.imm;
                std::cout << "xori " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.imm;
                break;
            case OR:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] | Reg[singleCmd.rs2];
                std::cout << "or " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.rs2;
                break;
            case ORI:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] | singleCmd.imm;
                std::cout << "ori " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.imm;
                break;
            case AND:
                if (singleCmd.rd != 0)
                    Reg[singleCmd.rd] = Reg[singleCmd.rs1] & Reg[singleCmd.rs2];
                std::cout << "and " << singleCmd.rd << ' ' << singleCmd.rs1 << ',' << singleCmd.rs2;
                break;
            default:
                break;

        }

    }

};

int main() {
    //freopen("../testcases/expr.data", "r", stdin);
    freopen("../testcases/array_test1.data", "r", stdin);
    freopen("print.out", "w", stdout);
    //freopen("../testcases/basicopt1.data", "r", stdin);
    CPU cpu;
    cpu.execute();
}