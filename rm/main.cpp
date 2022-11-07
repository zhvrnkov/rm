#include <iostream>
#include <vector>
#include <numeric>
#include <bitset>
#include <array>
#include <fstream>
#include <sstream>
#include <unordered_map>

using namespace std;

using Word = uint8_t;

struct WordBitsView {
    uint8_t b0: 1;
    uint8_t b1: 1;
    uint8_t b2: 1;
    uint8_t b3: 1;
    uint8_t b4: 1;
    uint8_t b5: 1;
    uint8_t b6: 1;
    uint8_t b7: 1;
};

struct FlagsRegisterView {
    uint8_t eq: 1;
    uint8_t lt: 1;
    uint8_t b2: 1;
    uint8_t b3: 1;
    uint8_t b4: 1;
    uint8_t b5: 1;
    uint8_t b6: 1;
    uint8_t b7: 1;
};

struct Machine;

enum Operation : Word {
    OP_MOV_R_I = 0,  // mov r, imm
    OP_MOV_R_R,      // mov r, r
    OP_MOV_R_MR,     // mov r, [r]
    
    OP_MOV_M_I,      // mov m, imm
    OP_MOV_M_R,      // mov m, r
    OP_MOV_M_MR,     // mov m, [r]
    
    OP_MOV_MR_I,     // mov [r], imm
    OP_MOV_MR_R,     // mov [r], r
    OP_MOV_MR_MR,    // mov [r], [r]
    
    OP_ADD_R_I,      // add r, imm
    OP_ADD_R_R,      // add r, r

    OP_SUB_R_I,      // sub r, imm
    OP_SUB_R_R,      // sub r, r
    
    OP_MUL_R_I,      // mul r, imm
    OP_MUL_R_R,      // mul r, r

    OP_DIV_R_I,      // div r, imm
    OP_DIV_R_R,      // div r, r
    
    OP_PUSH_I,
    OP_PUSH_R,
    
    OP_POP_R,
    
    OP_JMPR_I,       // jmp relative imm (imm addr)
    OP_JMPR_R,       // jmp relative r (imm addr)
    
    OP_JMPA_I,       // jmp absolute imm
    OP_JMPA_R,       // jmp absolute r
    
    OP_CMP_R_R,
    OP_CMP_R_I,          // cmp r, r
    
    OP_JER_I,
    OP_JER_R,
    OP_JEA_I,
    OP_JEA_R,
    
};

constexpr size_t InstructionWordsCount = 4;
constexpr size_t MemoryAmount = numeric_limits<Word>::max() + 1;
constexpr size_t RegCount = 8;

union Instruction {
    struct {
        Operation operation;
        Word operand1;
        Word operand2;
        Word operand3;
    } ins;
    Word words[InstructionWordsCount];
};

enum Error {
    ERR_INVALID_IP = 0,
};

namespace R {
constexpr size_t IP = 0;
constexpr size_t SP = 1;
constexpr size_t SB = 2;
constexpr size_t A = 3;
constexpr size_t B = 4;
constexpr size_t C = 5;
constexpr size_t D = 6;
constexpr size_t F = 7; // flags
};

struct Machine {
    
    array<Word, MemoryAmount> memory = {0};
    array<Word, RegCount> registers = {0};
    
    Machine() {
        ip = &(registers[R::IP]);
        rsp = &(registers[R::SP]);
        rbp = &(registers[R::SB]);
        flags = (FlagsRegisterView*)&(registers[R::F]);
    };
    
    ~Machine() = default;
    
    void load(const vector<Instruction>& instructions) {
        *ip = 0;
        if (instructions.empty())
            return;
        for (auto instr = instructions.rbegin(); instr != instructions.rend(); instr++) {
            encode(*instr);
        }
        *rsp = *rbp = *ip;
        (*ip) -= 1;
    }
    
    void execute() {
        while (*ip != 0) {
            step();
            dumpState();
            int i = 0;
        }
    }
    
    void step() {
        if (*ip < 0)
            throw Error::ERR_INVALID_IP;
        execute(decodeNext());
    }
    
    void dumpState() {
        cout << "Stack:\n";
        Word stackBase = *rbp;
        Word stackEnd = *rsp;
        for (int i = stackBase; i <= stackEnd; i++)
            cout << " " << (int)memory[i] << "\n";
        vector<pair<size_t, string>> regs = {
            { R::IP, "rip" },
            { R::SP, "rsp" },
            { R::SB, "rsb" },
            { R::A,  "rax" },
            { R::B,  "rbx" },
            { R::C,  "rcx" },
            { R::D,  "rdx" },
            { R::F,  "rfx" },
        };
        cout << "Registers:\n";
        for (const auto& [index, name] : regs) {
            cout << " " << name << " = " << (int)registers[index] << "\n";
        }
    }
    
    Word *ip;  // instruction pointer
    Word *rsp; // stack pointer
    Word *rbp; // stack base pointer
    FlagsRegisterView *flags;
    
private:
    
    Instruction decodeNext() {
        Instruction instr;
        for (int i = InstructionWordsCount - 1; i >= 0; i--) {
            instr.words[i] = memory[*ip];
            (*ip) -= 1;
        }
        return instr;
    }
    
    void encode(const Instruction &instr) {
        for (int i = 0; i < InstructionWordsCount; i++) {
            memory[*ip] = instr.words[i];
            (*ip) += 1;
        }
    }
    
    void execute(Instruction instr) {
        const auto& ins = instr.ins;
        switch (ins.operation) {
            case OP_MOV_R_I:
                registers.at(ins.operand1) = ins.operand2;
                break;
            case OP_MOV_R_R:
                registers.at(ins.operand1) = registers.at(ins.operand2);
                break;
            case OP_MOV_R_MR:
                registers.at(ins.operand1) = memory_at(registers.at(ins.operand2));
                break;

            case OP_MOV_M_I:
                memory_at(ins.operand1) = ins.operand2;
                break;
            case OP_MOV_M_R:
                memory_at(ins.operand1) = registers.at(ins.operand2);
                break;
            case OP_MOV_M_MR:
                memory_at(ins.operand1) = memory_at(registers.at(ins.operand2));
                break;
                
            case OP_MOV_MR_I:
                memory_at(registers.at(ins.operand1)) = ins.operand2;
                break;
            case OP_MOV_MR_R:
                memory_at(registers.at(ins.operand1)) = registers.at(ins.operand2);
                break;
            case OP_MOV_MR_MR:
                memory_at(registers.at(ins.operand1)) = memory_at(registers.at(ins.operand2));
                break;
                
            case OP_ADD_R_I:
                registers.at(ins.operand1) += ins.operand2;
                break;
            case OP_ADD_R_R:
                registers.at(ins.operand1) += registers.at(ins.operand2);
                break;
                
            case OP_SUB_R_I:
                registers.at(ins.operand1) -= ins.operand2;
                break;
            case OP_SUB_R_R:
                registers.at(ins.operand1) -= registers.at(ins.operand2);
                break;
                
            case OP_MUL_R_I:
                registers.at(ins.operand1) *= ins.operand2;
                break;
            case OP_MUL_R_R:
                registers.at(ins.operand1) *= registers.at(ins.operand2);
                break;

            case OP_DIV_R_I:
                registers.at(ins.operand1) /= ins.operand2;
                break;
            case OP_DIV_R_R:
                registers.at(ins.operand1) /= registers.at(ins.operand2);
                break;

                
            case OP_PUSH_I:
                execute({ OP_MOV_MR_I, R::SP, ins.operand1 });
                execute({ OP_ADD_R_I, R::SP, 1 });
                break;
            case OP_PUSH_R:
                execute({ OP_MOV_MR_R, R::SP, ins.operand1 });
                execute({ OP_ADD_R_I, R::SP, 1 });
                break;

            case OP_POP_R:
                execute({ OP_SUB_R_I, R::SP, 1 });
                execute({ OP_MOV_R_MR, ins.operand1, R::SP });
                break;
                
            case OP_JMPR_I:
                (*ip) -= ins.operand1;
                break;
            case OP_JMPR_R:
                (*ip) -= registers.at(ins.operand1);
                break;
                
            case OP_JMPA_I:
                (*ip) = ins.operand1;
                break;
            case OP_JMPA_R:
                (*ip) = registers.at(ins.operand1);
                break;
                
            case OP_CMP_R_R: {
                auto op1 = registers.at(ins.operand1);
                auto op2 = registers.at(ins.operand2);
                flags->eq = op1 == op2 ? 1 : 0;
                flags->lt = op1 < op2 ? 1 : 0;
                break;
            }
            case OP_CMP_R_I: {
                auto op1 = registers.at(ins.operand1);
                auto op2 = ins.operand2;
                flags->eq = op1 == op2 ? 1 : 0;
                flags->lt = op1 < op2 ? 1 : 0;
                break;
            }
                
            case OP_JER_I:
                if (flags->eq) (*ip) -= ins.operand1;
                break;
            case OP_JER_R:
                if (flags->eq) (*ip) -= registers.at(ins.operand1);
                break;
            case OP_JEA_I:
                if (flags->eq) (*ip) = ins.operand1;
                break;
            case OP_JEA_R:
                if (flags->eq) (*ip) = registers.at(ins.operand1);
                break;

        }
    }
    
    Word& memory_at(int index) {
        if (index < *rbp)
            throw std::out_of_range("index is less then stackBaseRegister");
        return memory.at(index);
    }
};

struct Operand {
    enum Type {
        IMM,
        REG,
        REG_VAL
    };
    Type type;
    Word word;
};
static const unordered_map<std::string, Operand> operands = {
    {"rip", {Operand::REG, R::IP}},
    {"rsp", {Operand::REG, R::SP}},
    {"rsb", {Operand::REG, R::SB}},
    {"rax", {Operand::REG, R::A}},
    {"rbx", {Operand::REG, R::B}},
    {"rcx", {Operand::REG, R::C}},
    {"rdx", {Operand::REG, R::D}},
    {"rfx", {Operand::REG, R::F}},
};

ostream& operator<<(ostream& os, const Operand::Type& operand_type) {
    switch (operand_type) {
        case Operand::IMM:
            return os << "imm";
        case Operand::REG:
            return os << "reg";
        case Operand::REG_VAL:
            return os << "[reg]";
            
    }
}

ostream& operator<<(ostream& os, const Operand& operand) {
    return os << "{ " << operand.type << ", " << (int)operand.word << " }";
}

Operand parseOperand(const std::string& operand) {
    try {
        int imm = stoi(operand);
        return { Operand::IMM, static_cast<Word>(imm) };
    }
    catch (invalid_argument const& ex) {
        if (operand.front() == '[' && operand.back() == ']') {
            auto output = operands.at(operand.substr(1, 3));
            output.type = Operand::REG_VAL;
            return output;
        }
        else {
            return operands.at(operand);
        }
    }
}

typedef Instruction (*InstructionContructor)(const vector<Operand>&);
using OpTypes = unordered_map<Operand::Type, Operation>;
using BinaryOpTypes = unordered_map<Operand::Type, OpTypes>;

Instruction constructBinary(const vector<Operand>& ops,
                            const BinaryOpTypes& opTypes) {
    const auto& op1 = ops.at(0);
    const auto& op2 = ops.at(1);
    return { opTypes.at(op1.type).at(op2.type), op1.word, op2.word };
}

Instruction constructMov(const vector<Operand>& ops) {
    static const OpTypes regTypes = {
        { Operand::IMM,     OP_MOV_R_I },
        { Operand::REG,     OP_MOV_R_R },
        { Operand::REG_VAL, OP_MOV_R_MR },
    };
    static const OpTypes immTypes = {
        { Operand::IMM,     OP_MOV_M_I },
        { Operand::REG,     OP_MOV_M_R },
        { Operand::REG_VAL, OP_MOV_M_MR },
    };
    static const OpTypes regValTypes = {
        { Operand::IMM,     OP_MOV_MR_I },
        { Operand::REG,     OP_MOV_MR_R },
        { Operand::REG_VAL, OP_MOV_MR_MR },
    };
    static const BinaryOpTypes opTypes = {
        { Operand::REG, regTypes },
        { Operand::IMM, immTypes },
        { Operand::REG_VAL, regValTypes }
    };
    return constructBinary(ops, opTypes);
}

Instruction constructArith(const vector<Operand>& ops,
                           const OpTypes& opTypes) {
    const auto& op1 = ops.at(0);
    const auto& op2 = ops.at(1);
    assert(op1.type == Operand::REG && "Not implemented");
    return { opTypes.at(op2.type), op1.word, op2.word };
}

Instruction constructAdd(const vector<Operand>& ops) {
    const static OpTypes opTypes = {
        { Operand::REG, OP_ADD_R_R },
        { Operand::IMM, OP_ADD_R_I }
    };
    return constructArith(ops, opTypes);
}
Instruction constructSub(const vector<Operand>& ops) {
    const static OpTypes opTypes = {
        { Operand::REG, OP_SUB_R_R },
        { Operand::IMM, OP_SUB_R_I }
    };
    return constructArith(ops, opTypes);
}
Instruction constructMul(const vector<Operand>& ops) {
    const static OpTypes opTypes = {
        { Operand::REG, OP_MUL_R_R },
        { Operand::IMM, OP_MUL_R_I }
    };
    return constructArith(ops, opTypes);
}
Instruction constructDiv(const vector<Operand>& ops) {
    const static OpTypes opTypes = {
        { Operand::REG, OP_DIV_R_R },
        { Operand::IMM, OP_DIV_R_I }
    };
    return constructArith(ops, opTypes);
}

Instruction constructUnary(const vector<Operand>& ops,
                           const OpTypes& opTypes) {
    assert(ops.size() && "unary operation have more than 1 operand");
    const auto& op = ops.front();
    return { opTypes.at(op.type), op.word };
}

Instruction constructPush(const vector<Operand>& ops) {
    static const OpTypes opTypes = {
        { Operand::IMM, OP_PUSH_I },
        { Operand::REG, OP_PUSH_R }
    };
    return constructUnary(ops, opTypes);
}

Instruction constructPop(const vector<Operand>& ops) {
    static const OpTypes opTypes = {
        { Operand::REG, OP_POP_R }
    };
    return constructUnary(ops, opTypes);
}

Instruction constructJmpR(const vector<Operand>& ops) {
    static const OpTypes opTypes = {
        { Operand::IMM, OP_JMPR_I },
        { Operand::REG, OP_JMPR_R },
    };
    return constructUnary(ops, opTypes);
}

Instruction constructJmpA(const vector<Operand>& ops) {
    static const OpTypes binOpTypes = {
        { Operand::IMM, OP_JMPA_I },
        { Operand::REG, OP_JMPA_R },
    };
    return constructUnary(ops, binOpTypes);
}

Instruction constructCmp(const vector<Operand>& ops) {
    static const BinaryOpTypes binOpTypes = {
        { Operand::REG, {
            { Operand::REG, OP_CMP_R_R },
            { Operand::IMM, OP_CMP_R_I },
        }}
    };
    return constructBinary(ops, binOpTypes);
}

Instruction constructJeR(const vector<Operand>& ops) {
    static const OpTypes opTypes = {
        { Operand::IMM, OP_JER_I },
        { Operand::REG, OP_JER_R },
    };
    return constructUnary(ops, opTypes);
}

Instruction constructJeA(const vector<Operand>& ops) {
    static const OpTypes opTypes = {
        { Operand::IMM, OP_JEA_I },
        { Operand::REG, OP_JEA_R },
    };
    return constructUnary(ops, opTypes);
}


static const unordered_map<string, InstructionContructor> constructors = {
    { "mov",  constructMov },
    { "add",  constructAdd },
    { "sub",  constructSub },
    { "mul",  constructMul },
    { "div",  constructDiv },
    { "push", constructPush },
    { "pop",  constructPop },
    { "jmpr", constructJmpR },
    { "jmpa", constructJmpA },
    { "cmp",  constructCmp },
    { "jer",  constructJeR },
    { "jea",  constructJeA },
};

Instruction constructInstruction(const string& mnemonic, const vector<Operand>& ops) {
    return (constructors.at(mnemonic))(ops);
};

Instruction parseInstruction(const std::string& line) {
    stringstream stream(line);
    string mnemomic;
    stream >> mnemomic;
    
    string operand1;
    stream >> operand1;
    
    string operand2;
    stream >> operand2;
    
    if (operand1.back() == ',') operand1.pop_back();
    
    vector<Operand> operands;
    operands.reserve(2);
    operands.push_back(parseOperand(operand1));
    if (operand2.empty() == false) operands.push_back(parseOperand(operand2));
    
    return constructInstruction(mnemomic, operands);
}

vector<Instruction> parseInstructions(std::string filePath) {
    vector<Instruction> program;
    ifstream file(filePath);
    
    string line;
    while (getline(file, line)) {
        program.push_back(parseInstruction(line));
    }
    return program;
}

int main(int argc, const char * argv[]) {
    const auto program = parseInstructions("/Users/vzhavoronkov/fun/rm/rm/add");
    Machine machine = Machine();

    machine.load(program);
    machine.execute();
    
    return 0;
}
