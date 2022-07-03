#include <iostream>

class CPU{
private:
    using uint = unsigned int;
    static uint str_to_uint(const std::string &str) {
        uint ans = 0;
        for (int i = 0; i < str.length(); ++i) {
            ans <<= 4;
            if (str[i] >= '0' && str[i] <= '9') ans += str[i] - '0';
            else ans += str[i] - 'A' + 10;
        }
        return ans;
    }
    static uint ch_to_uint(char s) { return (s >= '0' && s <= '9') ? (s - '0') : (s - 'A' + 10); }

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

    template<class T, int Length = 100>
    class Queue{
    private:
        T array[Length * 2 + 5] {};
        int head = 0;
        int tail = -1;
        int next = 0;  //编号，一直往下加
        int headentry = 1;
    public:
        void push(T data){
            array[++tail] = data;
            adjust();
            next++;
        }
        T &top(){ return array[head]; }
        T pop_front(){
            adjust();
            headentry++;
            return array[head++];
        }
        void pop(int index){
            for (int i = head + index; i < tail; ++i)
                array[i] = array[i + 1];
            tail--;
        }
        T &find(int entry){ return array[head + entry - headentry]; }
        T &operator[](int index){ return array[head + index]; }
        bool empty() const { return head == tail + 1; }
        bool full() const { return tail - head + 1 == Length; }
        int num() const { return tail - head + 1; }
        int nextNum() const { return next; }
        void clear() { head = 0, tail = -1, next = 0, headentry = 1; }
    private:
        void adjust(){
            if (head == Length || tail == 2 * Length) {
                for (int i = head, k = 0; i <= tail;)
                    array[k++] = array[i++];
                tail = tail - head;
                head = 0;
            }
        }
    };

    class Register{
    private:
        uint reg[32] = {0};
    public:
        uint PC = 0x00000000;
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

    enum State{ Pending, Ready };

    struct Code {
        uint code = 0x00000000;
        uint pc = 0x00000000;
        Code() = default;
        Code(uint _code, uint _pc):code(_code), pc(_pc){}
    };

    struct SingleCommand{
        //uint instruction = 0;
        //uint pc = 0;
        CommandFormat format {};
        CommandType type {};
        uint rd {};
        uint rs1 {};
        uint rs2 {};
        uint shamt {};
        uint imm{};
    };

    struct ReorderBuffer{
        int entry = 0;
        Code code {};
        State state = Pending;
        int destType = -1; //0:内存；1:寄存器; 2:PC; 3:Jump指令
        uint destination = 0;
        uint value = 0;
        uint pcvalue = 0;
        //int nextEntry = 0;
    };

    struct ReservationStations{
        bool ready = false;
        int only_sl = 3;
        uint code = 0;
        CommandType op {};
        uint vj = 0, qj = 0;
        uint vk = 0, qk = 0;
        uint pc = 0;
        int dest = 0;
        uint A = 0x00000000;
    };

    struct cdbContent{
        int rob_entry = 0;
        int destType = -1; //0;1;2;3:jump类型指令
        uint value = 0x00000000;
        uint pcvalue = 0x00000000;
        void clear(){
            rob_entry = 0;
            destType = -1;
            value = 0x00000000;
            pcvalue = 0x00000000;
        }
    } CDB, slCDB;

    struct Regfile{
        int entry = 0;
        uint value = 0x00000000;
    } regfile[32];

    bool stop = false;
    uint fetchPC = 0x00000000;

    Code code_from_if_to_issue;
    Code code_from_issue_to_rob;
    Code code_from_issue_to_rs;
    Code code_from_issue_to_slb;
    Code code_from_rs_to_ex;
    Code code_from_rob_to_commit;
    SingleCommand cmd_after_issue;
    ReservationStations RS_from_rs_to_ex;
    ReorderBuffer ROB_from_rob_to_commit;
    Queue<Code, 50> instruction_queue;
    Queue<ReorderBuffer, 50> ROB_queue;
    Queue<ReservationStations, 50> RS_queue;
    Queue<ReservationStations, 50> SLB_queue;
    static uint get_opcode(uint cmd) { return cmd & (uint) 0x0000007f; }
    static uint get_funct3(uint cmd) { return (cmd & (uint) 0x00007000) >> 12; }
    static uint get_funct7(uint cmd) { return (cmd & (uint) 0xff000000) >> 25; }
    static uint get_rd(uint cmd) { return (cmd & (uint) 0x00000f80) >> 7; }
    static uint get_rs1(uint cmd){ return (cmd & (uint) 0x000f8000) >> 15; }
    static uint get_rs2(uint cmd) { return (cmd & (uint) 0x01f00000) >> 20; }
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
                if ((cmd & (0x80000000)) == 0x80000000) ret |= (0xfffff000);
                break;
            case U:
                ret = (cmd & (0xfffff000));
                break;
            case UJ:
                ret = (cmd & (0x7fe00000)) >> 20;
                ret |= (cmd & (0x000ff000));
                ret |= (cmd & (0x00100000)) >> 9;
                if ((cmd & (uint) 0x80000000) == 0x80000000)ret |= 0xfff00000;
                break;
            default:
                break;
        }
        return ret;
    }
    static uint get_shamt(uint cmd) { return (cmd & (uint) 0x01f00000) >> 20; }
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

public:

    void scan(){ mem.initialize(std::cin); }

    void run(){
        int clock = 0;
        while(true){
            update();
            run_rob();
            if(code_from_rob_to_commit.code == 0x0ff00513){
                clear_newPC();
                std::cout << (Reg[10] & 255u) << '\n';
                return;
            }
            run_slbuffer();
            run_reservation();
            run_inst_fetch_queue();
            run_ex();
            run_issue();
            run_commit();
            clock++;
            //todo
            //用于debug
            //if (clock == 1000) break;
        }
    }

private:

    int oldEntry = 0;

    void run_inst_fetch_queue(){
        if (!stop && instruction_queue.empty()) {
            while (!instruction_queue.full()) {
                uint cmd = mem[fetchPC++];
                cmd |= (mem[fetchPC++] << 8);
                cmd |= (mem[fetchPC++] << 16);
                cmd |= (mem[fetchPC++] << 24);
                Code code(cmd, fetchPC - uint(4));
                instruction_queue.push(code);
            }
        }
        if (code_from_if_to_issue.code == 0)
            code_from_if_to_issue = instruction_queue.pop_front();
    }

    void run_issue() {
        code_from_issue_to_rob.code = code_from_issue_to_rs.code = code_from_issue_to_slb.code = 0x00000000;
        if (stop || ROB_queue.full() || RS_queue.full() || SLB_queue.full()) return;
        code_from_issue_to_rob = code_from_if_to_issue;
        cmd_after_issue = parse_single(code_from_if_to_issue);
        switch (cmd_after_issue.type) {
            case LB:
            case LBU:
            case LH:
            case LHU:
            case LW:
            case SB:
            case SH:
            case SW:
                code_from_issue_to_slb = code_from_if_to_issue;
                break;
            default:
                code_from_issue_to_rs = code_from_if_to_issue;
                break;
        }
        code_from_if_to_issue.code = 0x00000000;
        if (code_from_issue_to_rob.code != 0x00000000)
            ROB_queue.push(from_issue_to_rob());
        if (code_from_issue_to_rs.code != 0x00000000)
            RS_queue.push(from_issue_to_rs());
        if (code_from_issue_to_slb.code != 0x00000000)
            SLB_queue.push(from_issue_to_slb());
    }

    void run_rob(){
        if (!ROB_queue.empty()) {
            ReorderBuffer t = ROB_queue.top();
            if (t.state == Ready) {
                code_from_rob_to_commit = t.code;
                ROB_from_rob_to_commit = ROB_queue.pop_front();
            }
        }
    }

    void run_reservation(){
        code_from_rs_to_ex.code = 0x00000000;
        if (!RS_queue.empty()) {
            for (int i = 0; i < RS_queue.num(); ++i) {
                if (RS_queue[i].ready) {
                    code_from_rs_to_ex.code = RS_queue[i].code;
                    code_from_rs_to_ex.pc = RS_queue[i].pc;
                    RS_from_rs_to_ex = RS_queue[i];
                    RS_queue.pop(i);
                    break;
                }
            }
        }
    }

    void run_ex(){
        if (code_from_rs_to_ex.code != 0x00000000) {
            ReservationStations rs = RS_from_rs_to_ex;
            CDB.rob_entry = rs.dest;
            switch (rs.op) {
                case LUI:
                    CDB.destType = 1;
                    CDB.value = rs.A;
                    break;
                case AUIPC:
                    CDB.destType = 1;
                    CDB.value = rs.pc + rs.A;
                    break;
                case ADDI:
                    CDB.destType = 1;
                    CDB.value = rs.vj + rs.A;
                    break;
                case ANDI:
                    CDB.destType = 1;
                    CDB.value = rs.vj & rs.A;
                    break;
                case ORI:
                    CDB.destType = 1;
                    CDB.value = rs.vj | rs.A;
                    break;
                case XORI:
                    CDB.destType = 1;
                    CDB.value = rs.vj ^ rs.A;
                    break;
                case SLTI:
                    CDB.destType = 1;
                    CDB.value = ((int)rs.vj < (int)rs.A) ? 1 : 0;
                    break;
                case SLTIU:
                    CDB.destType = 1;
                    CDB.value = (rs.vj < rs.A) ? 1 : 0;
                    break;
                case SLLI:
                    CDB.destType = 1;
                    CDB.value = rs.vj << rs.A;
                    break;
                case SRLI:
                    CDB.destType = 1;
                    CDB.value = rs.vj >> rs.A;
                    break;
                case SRAI: {
                    CDB.destType = 1;
                    bool flag = true;
                    if ((rs.vj & (0x80000000)) != 0x80000000) flag = false;
                    CDB.value = rs.vj >> rs.A;
                    if (flag) {
                        for (int i = 31; i > 31 - rs.A; --i)
                            CDB.value |= (1 << i);
                    }
                    break;
                }
                case SLT:
                    CDB.destType = 1;
                    CDB.value = ((int)rs.vj < (int)rs.vk) ? 1 : 0;
                    break;
                case SLTU:
                    CDB.destType = 1;
                    CDB.value = (rs.vj < rs.vk) ? 1 : 0;
                    break;
                case SLL:
                    CDB.destType = 1;
                    CDB.value = (rs.vj << (rs.vk & 0x0000001f));
                    break;
                case SRL:
                    CDB.destType = 1;
                    CDB.value = (rs.vj >> (rs.vk & 0x0000001f));
                    break;
                case SRA: {
                    CDB.destType = 1;
                    bool flag = true;
                    uint shamt = rs.vk & 0x0000001f;
                    if ((rs.vj & 0x80000000) != 0x80000000) flag = false;
                    CDB.value = rs.vj >> shamt;
                    if (flag) {
                        for (int i = 31; i > 31 - shamt; --i)
                            CDB.value |= (1 << i);
                    }
                    break;
                }
                case ADD:
                    CDB.destType = 1;
                    CDB.value = rs.vj + rs.vk;
                    break;
                case AND:
                    CDB.destType = 1;
                    CDB.value = rs.vj & rs.vk;
                    break;
                case SUB:
                    CDB.destType = 1;
                    CDB.value = rs.vj - rs.vk;
                    break;
                case OR:
                    CDB.destType = 1;
                    CDB.value = rs.vj | rs.vk;
                    break;
                case XOR:
                    CDB.destType = 1;
                    CDB.value = rs.vj ^ rs.vk;
                    break;
                case BEQ:
                    CDB.destType = 2;
                    if (rs.vj == rs.vk)
                        CDB.pcvalue = rs.pc + rs.A;
                    else
                        CDB.pcvalue = rs.pc + 4;
                    break;
                case BNE:
                    CDB.destType = 2;
                    if (rs.vj != rs.vk)
                        CDB.pcvalue = rs.pc + rs.A;
                    else
                        CDB.pcvalue = rs.pc + 4;
                    break;
                case BLT:
                    CDB.destType = 2;
                    if ((int)rs.vj < (int)rs.vk)
                        CDB.pcvalue = rs.pc + rs.A;
                    else
                        CDB.pcvalue = rs.pc + 4;
                    break;
                case BGE:
                    CDB.destType = 2;
                    if ((int)rs.vj >= (int)rs.vk)
                        CDB.pcvalue = rs.pc + rs.A;
                    else
                        CDB.pcvalue = rs.pc + 4;
                    break;
                case BLTU:
                    CDB.destType = 2;
                    if (rs.vj < rs.vk)
                        CDB.pcvalue = rs.pc + rs.A;
                    else
                        CDB.pcvalue = rs.pc + 4;
                    break;
                case BGEU:
                    CDB.destType = 2;
                    if (rs.vj >= rs.vk)
                        CDB.pcvalue = rs.pc + rs.A;
                    else
                        CDB.pcvalue = rs.pc + 4;
                    break;
                case JAL:
                    CDB.destType = 3;
                    CDB.value = rs.pc + uint(4);
                    CDB.pcvalue = rs.pc + rs.A;
                    break;
                case JALR:
                    CDB.destType = 3;
                    CDB.value = rs.pc + uint(4);
                    CDB.pcvalue = rs.vj + rs.A;
                    break;
                default:
                    break;
            }
            code_from_rs_to_ex.code = 0x00000000;
        }
    }

    void run_slbuffer() {
        slCDB.rob_entry = 0;
        if (!SLB_queue.empty()) {
            ReservationStations sl = SLB_queue.top();
            if (!sl.ready) return;
            if (sl.op == SB || sl.op == SH || sl.op == SW) {
                if (sl.only_sl == 0) {
                    switch (sl.op) {
                        case SB:
                            mem[sl.vj + sl.A] = (sl.vk & 0x000000ff);
                            break;
                        case SH:
                            mem[sl.vj + sl.A] = (sl.vk & 0x0000ffff);
                            break;
                        case SW:
                            mem[sl.vj + sl.A] = (sl.vk & 0xffffffff);
                            break;
                        default:
                            break;
                    }
                    SLB_queue.pop_front();
                } else if (sl.only_sl != 3) {
                    SLB_queue[0].only_sl--;
                }
            } else {
                if (sl.only_sl == 1) { //执行完毕，更新slCDB并推出
                    slCDB.rob_entry = sl.dest;
                    slCDB.destType = 1;
                    switch (sl.op) {
                        case LB: {
                            uint loc = sl.vj + sl.A;
                            slCDB.value = sign_readIn(loc, 1);
                            break;
                        }
                        case LH: {
                            uint loc = sl.vj + sl.A;
                            slCDB.value = sign_readIn(loc, 2);
                            break;
                        }
                        case LW: {
                            uint loc = sl.vj + sl.A;
                            slCDB.value = sign_readIn(loc, 4);
                            break;
                        }
                        case LBU: {
                            uint loc = sl.vj + sl.A;
                            slCDB.value = zero_readIn(loc, 1);
                            break;
                        }
                        case LHU: {
                            uint loc = sl.vj + sl.A;
                            slCDB.value = zero_readIn(loc, 2);
                            break;
                        }
                        default:
                            break;
                    }
                    SLB_queue.pop_front();
                } else {
                    SLB_queue[0].only_sl--;
                }
            }
        }
    }

    void update(){
        if (CDB.rob_entry != 0) {
            int entry = CDB.rob_entry;
            CDB.rob_entry = 0;
            if (!ROB_queue.empty()) {
                switch (CDB.destType) {
                    case 1:
                        ROB_queue.find(entry).value = CDB.value;
                        ROB_queue.find(entry).state = Ready;
                        break;
                    case 2:
                        ROB_queue.find(entry).pcvalue = CDB.pcvalue;
                        ROB_queue.find(entry).state = Ready;
                        break;
                    case 3:
                        ROB_queue.find(entry).value = CDB.value;
                        ROB_queue.find(entry).pcvalue = CDB.pcvalue;
                        ROB_queue.find(entry).state = Ready;
                        break;
                    default:
                        break;
                }
                if (CDB.destType == 1 || CDB.destType == 3) {
                    uint destination = ROB_queue.find(entry).destination;
                    regfile[destination].value = CDB.value;
                    if (regfile[destination].entry == entry)
                        regfile[destination].entry = 0;
                }
            }
            if (!RS_queue.empty()) {
                for (int i = 0; i < RS_queue.num(); ++i) {
                    if (RS_queue[i].qj == entry) {
                        RS_queue[i].vj = CDB.value;
                        RS_queue[i].qj = 0;
                    }
                    if (RS_queue[i].qk == entry) {
                        RS_queue[i].vk = CDB.value;
                        RS_queue[i].qk = 0;
                    }
                    if (RS_queue[i].qj == 0 && RS_queue[i].qk == 0) RS_queue[i].ready = true;
                }
            }
            if (!SLB_queue.empty()) {
                for (int i = 0; i < SLB_queue.num(); ++i) {
                    if (SLB_queue[i].qj == entry) {
                        SLB_queue[i].vj = CDB.value;
                        SLB_queue[i].qj = 0;
                    }
                    if (SLB_queue[i].qk == entry) {
                        SLB_queue[i].vk = CDB.value;
                        SLB_queue[i].qk = 0;
                    }
                    if (SLB_queue[i].qj == 0 && SLB_queue[i].qk == 0) SLB_queue[i].ready = true;
                }
            }
        }
        if (slCDB.rob_entry != 0) {
            int entry = slCDB.rob_entry;
            slCDB.rob_entry = 0;
            if (!ROB_queue.empty()) {
                ROB_queue.find(entry).value = slCDB.value;
                ROB_queue.find(entry).state = Ready;
                uint destination = ROB_queue.find(entry).destination;
                regfile[destination].value = slCDB.value;
                if (regfile[destination].entry == entry)
                    regfile[destination].entry = 0;
            }
            if (!RS_queue.empty()) {
                for (int i = 0; i < RS_queue.num(); ++i) {
                    if (RS_queue[i].qj == entry) {
                        RS_queue[i].vj = slCDB.value;
                        RS_queue[i].qj = 0;
                    }
                    if (RS_queue[i].qk == entry) {
                        RS_queue[i].vk = slCDB.value;
                        RS_queue[i].qk = 0;
                    }
                    if (RS_queue[i].qj == 0 && RS_queue[i].qk == 0) RS_queue[i].ready = true;
                }
            }
            if (!SLB_queue.empty()) {
                for (int i = 0; i < SLB_queue.num(); ++i) {
                    if (SLB_queue[i].qj == entry) {
                        SLB_queue[i].vj = slCDB.value;
                        SLB_queue[i].qj = 0;
                    }
                    if (SLB_queue[i].qk == entry) {
                        SLB_queue[i].vk = slCDB.value;
                        SLB_queue[i].qk = 0;
                    }
                    if (SLB_queue[i].qj == 0 && SLB_queue[i].qk == 0) SLB_queue[i].ready = true;
                }
            }
        }
    }

    void run_commit(){
        if (code_from_rob_to_commit.code != 0x00000000) {
            //todo
            //用于debug
            //std::cout << code_from_rob_to_commit.code << '\n';
            ReorderBuffer t = ROB_from_rob_to_commit;
            switch (t.destType) {
                case 0:
                    slb_commit(t);
                    Reg.PC = code_from_rob_to_commit.pc + uint(4);
                    break;
                case 1:
                    if (t.destination != 0)
                        Reg[t.destination] = t.value;
                    Reg.PC = code_from_rob_to_commit.pc + uint(4);
                    break;
                case 2:
                    if (ROB_queue.empty() || ROB_queue[0].code.pc != t.pcvalue) {
                        clear_newPC();
                        fetchPC = t.pcvalue;
                    }
                    Reg.PC = t.pcvalue;
                    /*
                    clear_newPC();
                    Reg.PC = t.pcvalue;
                    fetchPC = Reg.PC;
                    stop = false;
                     */
                    break;
                case 3:
                    if (t.destination != 0)
                        Reg[t.destination] = t.value;
                    clear_newPC();
                    Reg.PC = t.pcvalue;
                    fetchPC = Reg.PC;
                    break;
                default:
                    break;
            }
            code_from_rob_to_commit.code = 0x00000000;
        }
    }

    //通知slBuffer commit 指令(store型)
    void slb_commit(ReorderBuffer t){
        for (int i = 0; i < SLB_queue.num(); ++i) {
            if (SLB_queue[i].dest == t.entry) {
                SLB_queue[i].only_sl--;
                break;
            }
        }
    }

    void clear_newPC() {
        stop = false;
        while (!SLB_queue.empty() && (SLB_queue[0].op == SB || SLB_queue[0].op == SH || SLB_queue[0].op == SW) && SLB_queue[0].only_sl != 3) {
            //跳转或程序终止后，完成SLBuffer的操作
            ReservationStations sl = SLB_queue[0];
            switch (sl.op) {
                case SB:
                    mem[sl.vj + sl.A] = (sl.vk & 0x000000ff);
                    break;
                case SH:
                    mem[sl.vj + sl.A] = (sl.vk & 0x0000ffff);
                    break;
                case SW:
                    mem[sl.vj + sl.A] = (sl.vk & 0xffffffff);
                    break;
                default:
                    break;
            }
            SLB_queue.pop_front();
        }
        instruction_queue.clear();
        ROB_queue.clear();
        RS_queue.clear();
        SLB_queue.clear();
        CDB.clear(), slCDB.clear();
        code_from_if_to_issue.code = code_from_issue_to_rob.code = code_from_issue_to_slb.code = code_from_issue_to_rs.code = code_from_rs_to_ex.code = code_from_rob_to_commit.code = 0x00000000;
        for (int i = 0; i < 32; ++i) {
            regfile[i].entry = 0;
            regfile[i].value = Reg[i];
        }
    }

    //

    static SingleCommand parse_single(Code cmd){
        SingleCommand singleCmd;
        //singleCmd.pc = cmd.pc;
        //singleCmd.instruction = cmd.code;
        uint code = get_opcode(cmd.code);
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
                code |= (get_funct3(cmd.code) << 7);
                singleCmd.type = (CommandType) code;
                break;
            case 0b0010011:{
                uint f3 = get_funct3(cmd.code);
                code |= (f3 << 7);
                if (cmd.code == (0b101))
                    code |= (get_funct7(cmd.code) << 10);
                singleCmd.type = (CommandType) code;
                break;
            }
            default:
                code |= (get_funct3(cmd.code) << 7);
                code |= (get_funct7(cmd.code) << 10);
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
        singleCmd.rd = get_rd(cmd.code);
        singleCmd.rs1 = get_rs1(cmd.code);
        singleCmd.rs2 = get_rs2(cmd.code);
        singleCmd.shamt = get_shamt(cmd.code);
        singleCmd.imm = get_immediate(cmd.code, singleCmd.format);
        return singleCmd;
    }

    void branch_prediction() {
        instruction_queue.clear();
        fetchPC = code_from_issue_to_rob.pc + cmd_after_issue.imm;
    }

    ReorderBuffer from_issue_to_rob(){
        ReorderBuffer res;
        if (code_from_issue_to_rob.code != 0x00000000) {
            res.entry = ROB_queue.nextNum() + 1;
            res.code = code_from_issue_to_rob;
            switch (cmd_after_issue.type) {
                case SB:
                case SH:
                case SW:
                    res.destType = 0;
                    res.state = Ready;
                    break;
                case BEQ:
                case BGE:
                case BGEU:
                case BLT:
                case BLTU:
                case BNE:
                    res.destType = 2;
                    branch_prediction();
                    //stop = true;
                    break;
                case JAL:
                case JALR:
                    res.destType = 3;
                    res.destination = cmd_after_issue.rd;
                    oldEntry = regfile[cmd_after_issue.rd].entry;
                    regfile[cmd_after_issue.rd].entry = res.entry;
                    stop = true;
                    break;
                default:
                    res.destType = 1;
                    res.destination = cmd_after_issue.rd;
                    oldEntry = regfile[cmd_after_issue.rd].entry;
                    regfile[cmd_after_issue.rd].entry = res.entry;
                    break;
            }
            code_from_issue_to_rob.code = 0x00000000;
        }
        return res;
    }

    ReservationStations from_issue_to_rs(){
        ReservationStations res;
        if (code_from_issue_to_rs.code != 0x00000000) {
            res.code = code_from_issue_to_rs.code;
            res.pc = code_from_issue_to_rs.pc;
            res.op = cmd_after_issue.type;
            res.dest = ROB_queue.nextNum();
            switch (cmd_after_issue.type) {
                case LUI:
                case AUIPC:
                    res.A = cmd_after_issue.imm;
                    res.ready = true;
                    break;
                case ADDI:
                case ANDI:
                case ORI:
                case XORI:
                case SLTI:
                case SLTIU:
                    res.A = cmd_after_issue.imm;
                    if (!regfile[cmd_after_issue.rs1].entry) {
                        res.vj = regfile[cmd_after_issue.rs1].value;
                        res.ready = true;
                    } else if (regfile[cmd_after_issue.rs1].entry == res.dest) {
                        if (!oldEntry) {
                            res.vj = regfile[cmd_after_issue.rs1].value;
                            res.ready = true;
                        } else {
                            res.qj = oldEntry;
                        }
                    } else {
                        res.qj = regfile[cmd_after_issue.rs1].entry;
                    }
                    break;
                case SLLI:
                case SRLI:
                case SRAI:
                    res.A = cmd_after_issue.shamt;
                    if (!regfile[cmd_after_issue.rs1].entry) {
                        res.vj = regfile[cmd_after_issue.rs1].value;
                        res.ready = true;
                    } else if (regfile[cmd_after_issue.rs1].entry == res.dest) {
                        if (!oldEntry) {
                            res.vj = regfile[cmd_after_issue.rs1].value;
                            res.ready = true;
                        } else {
                            res.qj = oldEntry;
                        }
                    } else {
                        res.qj = regfile[cmd_after_issue.rs1].entry;
                    }
                    break;
                case SLT:
                case SLTU:
                case SLL:
                case SRL:
                case SRA:
                case ADD:
                case AND:
                case SUB:
                case OR:
                case XOR:
                    if (!regfile[cmd_after_issue.rs1].entry) {
                        res.vj = regfile[cmd_after_issue.rs1].value;
                    } else if (regfile[cmd_after_issue.rs1].entry == res.dest) {
                        if (!oldEntry) {
                            res.vj = regfile[cmd_after_issue.rs1].value;
                        } else {
                            res.qj = oldEntry;
                        }
                    } else {
                        res.qj = regfile[cmd_after_issue.rs1].entry;
                    }
                    if (!regfile[cmd_after_issue.rs2].entry) {
                        res.vk = regfile[cmd_after_issue.rs2].value;
                    } else if (regfile[cmd_after_issue.rs2].entry == res.dest) {
                        if (!oldEntry) {
                            res.vk = regfile[cmd_after_issue.rs2].value;
                        } else {
                            res.qk = oldEntry;
                        }
                    } else {
                        res.qk = regfile[cmd_after_issue.rs2].entry;
                    }
                    if (res.qj == 0 && res.qk == 0) res.ready = true;
                    break;
                case BEQ:
                case BNE:
                case BLT:
                case BGE:
                case BLTU:
                case BGEU:
                    res.A = cmd_after_issue.imm;
                    if (!regfile[cmd_after_issue.rs1].entry) {
                        res.vj = regfile[cmd_after_issue.rs1].value;
                    } else if (regfile[cmd_after_issue.rs1].entry == res.dest) {
                        if (!oldEntry) {
                            res.vj = regfile[cmd_after_issue.rs1].value;
                        } else {
                            res.qj = oldEntry;
                        }
                    } else {
                        res.qj = regfile[cmd_after_issue.rs1].entry;
                    }
                    if (!regfile[cmd_after_issue.rs2].entry) {
                        res.vk = regfile[cmd_after_issue.rs2].value;
                    } else if (regfile[cmd_after_issue.rs2].entry == res.dest) {
                        if (!oldEntry) {
                            res.vk = regfile[cmd_after_issue.rs2].value;
                        } else {
                            res.qk = oldEntry;
                        }
                    } else {
                        res.qk = regfile[cmd_after_issue.rs2].entry;
                    }
                    if (res.qj == 0 && res.qk == 0) res.ready = true;
                    break;
                case JAL:
                    res.A = cmd_after_issue.imm;
                    res.ready = true;
                    break;
                case JALR:
                    res.A = cmd_after_issue.imm;
                    if (!regfile[cmd_after_issue.rs1].entry) {
                        res.vj = regfile[cmd_after_issue.rs1].value;
                        res.ready = true;
                    } else if (regfile[cmd_after_issue.rs1].entry == res.dest) {
                        if (!oldEntry) {
                            res.vj = regfile[cmd_after_issue.rs1].value;
                            res.ready = true;
                        } else {
                            res.qj = oldEntry;
                        }
                    } else {
                        res.qj = regfile[cmd_after_issue.rs1].entry;
                    }
                    break;
                default:
                    break;
            }
            code_from_issue_to_rs.code = 0x00000000;
        }
        return res;
    }

    ReservationStations from_issue_to_slb() {
        ReservationStations res;
        if (code_from_issue_to_slb.code != 0x00000000) {
            res.code = code_from_issue_to_slb.code;
            res.pc = code_from_issue_to_rs.pc;
            res.op = cmd_after_issue.type;
            res.dest = ROB_queue.nextNum();
            res.A = cmd_after_issue.imm;
            switch (cmd_after_issue.type) {
                case LB:
                case LBU:
                case LH:
                case LHU:
                case LUI:
                case LW:
                    if (!regfile[cmd_after_issue.rs1].entry) {
                        res.vj = regfile[cmd_after_issue.rs1].value;
                        res.ready = true;
                    } else if (regfile[cmd_after_issue.rs1].entry == res.dest) {
                        if (!oldEntry) {
                            res.vj = regfile[cmd_after_issue.rs1].value;
                            res.ready = true;
                        } else {
                            res.qj = oldEntry;
                        }
                    } else {
                        res.qj = regfile[cmd_after_issue.rs1].entry;
                    }
                    break;
                case SB:
                case SH:
                case SW:
                    if (!regfile[cmd_after_issue.rs1].entry) {
                        res.vj = regfile[cmd_after_issue.rs1].value;
                    } else if (regfile[cmd_after_issue.rs1].entry == res.dest) {
                        if (!oldEntry) {
                            res.vj = regfile[cmd_after_issue.rs1].value;
                        } else {
                            res.qj = oldEntry;
                        }
                    } else {
                        res.qj = regfile[cmd_after_issue.rs1].entry;
                    }
                    if (!regfile[cmd_after_issue.rs2].entry) {
                        res.vk = regfile[cmd_after_issue.rs2].value;
                    } else if (regfile[cmd_after_issue.rs2].entry == res.dest) {
                        if (!oldEntry) {
                            res.vk = regfile[cmd_after_issue.rs2].value;
                        } else {
                            res.qk = oldEntry;
                        }
                    } else {
                        res.qk = regfile[cmd_after_issue.rs2].entry;
                    }
                    if (res.qj == 0 && res.qk == 0) res.ready = true;
                    break;
                default:
                    break;
            }
            code_from_issue_to_slb.code = 0x00000000;
        }
        return res;
    }

};

int main() {
    //freopen("sample.data", "r", stdin);
    //freopen("../testcases/magic.data", "r", stdin);
    //freopen("gyj.out", "w", stdout);
    CPU cpu;
    cpu.scan();
    cpu.run();
}
