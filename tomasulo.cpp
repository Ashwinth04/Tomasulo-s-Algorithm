#include <iostream>
#include <vector>
#include <queue>
#include <string>
#include <map>

using namespace std;

// Instruction types
enum InstructionType {
    IADD, IMUL, FADD, FMUL, LD, ST, LOGIC, NOP
};

// Instruction structure
struct Instruction {
    InstructionType type;
    int dest;
    int src1;
    int src2;
    int memAddr;
    int cycles;
    int issueTime;
    int execStartTime;
    int execCompTime;
    int commitTime;
    bool isIssued;
    bool isExecStarted;
    bool isExecCompleted;
    bool isCommitted;

    Instruction(InstructionType t, int d = -1, int s1 = -1, int s2 = -1, int mem = -1) {
        type = t;
        dest = d;
        src1 = s1;
        src2 = s2;
        memAddr = mem;
        
        // Set cycle count based on instruction type
        switch(type) {
            case IADD: cycles = 6; break;
            case IMUL: cycles = 12; break;
            case FADD: cycles = 18; break;
            case FMUL: cycles = 30; break;
            case LD: cycles = 1; break;
            case ST: cycles = 1; break;
            case LOGIC: cycles = 1; break;
            case NOP: cycles = 1; break;
        }
        
        issueTime = -1;
        execStartTime = -1;
        execCompTime = -1;
        commitTime = -1;
        isIssued = false;
        isExecStarted = false;
        isExecCompleted = false;
        isCommitted = false;
    }
};

// Reservation Station
struct ReservationStation {
    bool busy;
    InstructionType op;
    int Vj, Vk;
    int Qj, Qk;
    int dest;
    int cycles;
    int instrIndex;

    ReservationStation() {
        busy = false;
        Vj = Vk = 0;
        Qj = Qk = -1;
        dest = -1;
        cycles = 0;
        instrIndex = -1;
    }
};

// Reorder Buffer Entry
struct ROBEntry {
    bool ready;
    int instrIndex;
    int dest;
    int value;
    
    ROBEntry() {
        ready = false;
        instrIndex = -1;
        dest = -1;
        value = 0;
    }
};

// Register Status
struct RegisterStatus {
    int robIndex;  // -1 if register is ready
    
    RegisterStatus() {
        robIndex = -1;
    }
};

class TomasuloSimulator {
private:
    int clock;
    vector<int> registers;
    vector<RegisterStatus> regStatus;
    vector<ReservationStation> iadd_rs;
    vector<ReservationStation> imul_rs;
    vector<ReservationStation> fadd_rs;
    vector<ReservationStation> fmul_rs;
    vector<ReservationStation> mem_rs;
    vector<ReservationStation> logic_rs;
    vector<ROBEntry> rob;
    int robHead, robTail;
    int robSize;
    vector<Instruction> instructions;
    int pc;
    
    // Performance metrics
    int structuralStalls;
    int dataStalls;
    int completedInstructions;
    vector<int> instructionLatencies;

public:
    TomasuloSimulator(int numRegs = 32, int robSize = 16) {
        clock = 0;
        registers.resize(numRegs, 0);
        regStatus.resize(numRegs);
        
        // Initialize reservation stations
        iadd_rs.resize(2);
        imul_rs.resize(2);
        fadd_rs.resize(2);
        fmul_rs.resize(2);
        mem_rs.resize(2);
        logic_rs.resize(2);
        
        // Initialize reorder buffer
        this->robSize = robSize;
        rob.resize(robSize);
        robHead = robTail = 0;
        
        pc = 0;
        
        // Initialize performance metrics
        structuralStalls = 0;
        dataStalls = 0;
        completedInstructions = 0;
    }
    
    void addInstruction(Instruction instr) {
        instructions.push_back(instr);
    }
    
    void parseAssembly(string assembly) {
        // Improved parser that handles multi-digit register numbers
        if (assembly.find("IADD") != string::npos) {
            size_t rPos = assembly.find("R");
            size_t comma1 = assembly.find(",");
            size_t comma2 = assembly.find(",", comma1 + 1);
            
            int dest = stoi(assembly.substr(rPos + 1, comma1 - (rPos + 1)));
            
            rPos = assembly.find("R", comma1);
            int src1 = stoi(assembly.substr(rPos + 1, comma2 - (rPos + 1)));
            
            rPos = assembly.find("R", comma2);
            int src2 = stoi(assembly.substr(rPos + 1));
            
            addInstruction(Instruction(IADD, dest, src1, src2));
        } else if (assembly.find("IMUL") != string::npos) {
            size_t rPos = assembly.find("R");
            size_t comma1 = assembly.find(",");
            size_t comma2 = assembly.find(",", comma1 + 1);
            
            int dest = stoi(assembly.substr(rPos + 1, comma1 - (rPos + 1)));
            
            rPos = assembly.find("R", comma1);
            int src1 = stoi(assembly.substr(rPos + 1, comma2 - (rPos + 1)));
            
            rPos = assembly.find("R", comma2);
            int src2 = stoi(assembly.substr(rPos + 1));
            
            addInstruction(Instruction(IMUL, dest, src1, src2));
        } else if (assembly.find("FADD") != string::npos) {
            size_t rPos = assembly.find("R");
            size_t comma1 = assembly.find(",");
            size_t comma2 = assembly.find(",", comma1 + 1);
            
            int dest = stoi(assembly.substr(rPos + 1, comma1 - (rPos + 1)));
            
            rPos = assembly.find("R", comma1);
            int src1 = stoi(assembly.substr(rPos + 1, comma2 - (rPos + 1)));
            
            rPos = assembly.find("R", comma2);
            int src2 = stoi(assembly.substr(rPos + 1));
            
            addInstruction(Instruction(FADD, dest, src1, src2));
        } else if (assembly.find("FMUL") != string::npos) {
            size_t rPos = assembly.find("R");
            size_t comma1 = assembly.find(",");
            size_t comma2 = assembly.find(",", comma1 + 1);
            
            int dest = stoi(assembly.substr(rPos + 1, comma1 - (rPos + 1)));
            
            rPos = assembly.find("R", comma1);
            int src1 = stoi(assembly.substr(rPos + 1, comma2 - (rPos + 1)));
            
            rPos = assembly.find("R", comma2);
            int src2 = stoi(assembly.substr(rPos + 1));
            
            addInstruction(Instruction(FMUL, dest, src1, src2));
        } else if (assembly.find("LD") != string::npos) {
            size_t rPos = assembly.find("R");
            size_t comma = assembly.find(",");
            size_t mPos = assembly.find("M");
            
            int dest = stoi(assembly.substr(rPos + 1, comma - (rPos + 1)));
            int memAddr = stoi(assembly.substr(mPos + 1));
            
            addInstruction(Instruction(LD, dest, -1, -1, memAddr));
        } else if (assembly.find("ST") != string::npos) {
            size_t mPos = assembly.find("M");
            size_t comma = assembly.find(",");
            size_t rPos = assembly.find("R", comma);
            
            int memAddr = stoi(assembly.substr(mPos + 1, comma - (mPos + 1)));
            int src = stoi(assembly.substr(rPos + 1));
            
            addInstruction(Instruction(ST, -1, src, -1, memAddr));
        } else if (assembly.find("AND") != string::npos || assembly.find("OR") != string::npos || assembly.find("XOR") != string::npos) {
            size_t rPos = assembly.find("R");
            size_t comma1 = assembly.find(",");
            size_t comma2 = assembly.find(",", comma1 + 1);
            
            int dest = stoi(assembly.substr(rPos + 1, comma1 - (rPos + 1)));
            
            rPos = assembly.find("R", comma1);
            int src1 = stoi(assembly.substr(rPos + 1, comma2 - (rPos + 1)));
            
            rPos = assembly.find("R", comma2);
            int src2 = stoi(assembly.substr(rPos + 1));
            
            addInstruction(Instruction(LOGIC, dest, src1, src2));
        } else if (assembly.find("NOP") != string::npos) {
            addInstruction(Instruction(NOP));
        }
    }
    
    bool isFull(vector<ReservationStation>& rs) {
        for (auto& station : rs) {
            if (!station.busy) return false;
        }
        return true;
    }
    
    bool isRobFull() {
        return (robTail + 1) % robSize == robHead;
    }
    
    int getFreeRs(vector<ReservationStation>& rs) {
        for (int i = 0; i < rs.size(); i++) {
            if (!rs[i].busy) return i;
        }
        return -1;
    }
    
    void issue() {
        if (pc >= instructions.size()) return;
        
        Instruction& instr = instructions[pc];
        
        // Check if ROB is full
        if (isRobFull()) {
            structuralStalls++; // Track structural stall
            return;
        }
        
        // Find appropriate reservation station
        vector<ReservationStation>* rs_ptr = nullptr;
        switch(instr.type) {
            case IADD: rs_ptr = &iadd_rs; break;
            case IMUL: rs_ptr = &imul_rs; break;
            case FADD: rs_ptr = &fadd_rs; break;
            case FMUL: rs_ptr = &fmul_rs; break;
            case LD:
            case ST: rs_ptr = &mem_rs; break;
            case LOGIC: rs_ptr = &logic_rs; break;
            case NOP: pc++; return;  // NOP doesn't need reservation station
        }
        
        // Check if RS is full
        if (isFull(*rs_ptr)) {
            structuralStalls++; // Track structural stall
            return;
        }
        
        // Assign to reservation station
        int rs_index = getFreeRs(*rs_ptr);
        ReservationStation& rs = (*rs_ptr)[rs_index];
        rs.busy = true;
        rs.op = instr.type;
        rs.instrIndex = pc;
        rs.cycles = instr.cycles;
        
        // Get operand values or dependencies
        if (instr.src1 != -1) {
            if (regStatus[instr.src1].robIndex == -1) {
                rs.Vj = registers[instr.src1];
                rs.Qj = -1;
            } else {
                rs.Qj = regStatus[instr.src1].robIndex;
                // If operand is not ready, track data hazard
                dataStalls++;
            }
        }
        
        if (instr.src2 != -1) {
            if (regStatus[instr.src2].robIndex == -1) {
                rs.Vk = registers[instr.src2];
                rs.Qk = -1;
            } else {
                rs.Qk = regStatus[instr.src2].robIndex;
                // If operand is not ready, track data hazard
                dataStalls++;
            }
        }
        
        // Add to ROB
        rob[robTail].instrIndex = pc;
        rob[robTail].dest = instr.dest;
        rob[robTail].ready = false;
        
        // Update register status for destination register
        if (instr.dest != -1) {
            regStatus[instr.dest].robIndex = robTail;
        }
        
        rs.dest = robTail;
        
        // Update instruction status
        instr.isIssued = true;
        instr.issueTime = clock;
        
        // Increment ROB tail and PC
        robTail = (robTail + 1) % robSize;
        pc++;
    }
    
    void execute() {
        vector<vector<ReservationStation>*> all_rs = {&iadd_rs, &imul_rs, &fadd_rs, &fmul_rs, &mem_rs, &logic_rs};
        
        for (auto rs_ptr : all_rs) {
            for (auto& rs : *rs_ptr) {
                if (rs.busy) {
                    // Check if operands are ready
                    if (rs.Qj == -1 && rs.Qk == -1) {
                        // Start execution if operands are ready
                        Instruction& instr = instructions[rs.instrIndex];
                        
                        if (!instr.isExecStarted) {
                            instr.isExecStarted = true;
                            instr.execStartTime = clock;
                        }
                        
                        // Decrement cycles
                        rs.cycles--;
                        
                        // Check if execution completed
                        if (rs.cycles <= 0 && !instr.isExecCompleted) {
                            instr.isExecCompleted = true;
                            instr.execCompTime = clock;
                            
                            // Mark ROB entry as ready - but ensure at least one cycle between exec completion and commit
                            rob[rs.dest].ready = true;
                            
                            // Free reservation station
                            rs.busy = false;
                        }
                    }
                }
            }
        }
    }
    
    void commit() {
        // Only consider commit if the ROB entry at head is ready
        // AND it wasn't just marked ready in this cycle
        if (rob[robHead].ready) {
            int instrIndex = rob[robHead].instrIndex;
            Instruction& instr = instructions[instrIndex];
            
            // Ensure at least one cycle gap between execution completion and commit
            if (instr.execCompTime < clock) {
                // Update register file
                if (instr.dest != -1) {
                    registers[instr.dest] = rob[robHead].value;
                    
                    // Update register status
                    if (regStatus[instr.dest].robIndex == robHead) {
                        regStatus[instr.dest].robIndex = -1;
                    }
                }
                
                // Update instruction status
                instr.isCommitted = true;
                instr.commitTime = clock;
                
                // Calculate and store instruction latency
                int latency = instr.commitTime - instr.issueTime;
                instructionLatencies.push_back(latency);
                
                // Increment completed instructions counter
                completedInstructions++;
                
                // Increment ROB head
                robHead = (robHead + 1) % robSize;
            }
        }
    }
    
    void updateDependencies() {
        vector<vector<ReservationStation>*> all_rs = {&iadd_rs, &imul_rs, &fadd_rs, &fmul_rs, &mem_rs, &logic_rs};
        
        for (auto rs_ptr : all_rs) {
            for (auto& rs : *rs_ptr) {
                if (rs.busy) {
                    // Check if Qj is ready
                    if (rs.Qj != -1 && rob[rs.Qj].ready) {
                        rs.Vj = rob[rs.Qj].value;
                        rs.Qj = -1;
                    }
                    
                    // Check if Qk is ready
                    if (rs.Qk != -1 && rob[rs.Qk].ready) {
                        rs.Vk = rob[rs.Qk].value;
                        rs.Qk = -1;
                    }
                }
            }
        }
    }
    
    bool allInstructionsCommitted() {
        for (auto& instr : instructions) {
            if (!instr.isCommitted) return false;
        }
        return true;
    }
    
    void runSimulation() {
        while (!allInstructionsCommitted()) {
            cout << "Clock Cycle: " << clock << endl;
            
            // Pipeline stages in reverse order to prevent same-cycle issue-execute-commit
            // First commit (uses results from previous cycle)
            commit();
            
            // Then execute (uses issue results from previous cycle)
            execute();
            
            // Then update dependencies
            updateDependencies();
            
            // Finally issue (results available next cycle)
            issue();
            
            // Print status
            printStatus();
            
            clock++;
        }
        
        // Calculate and print performance metrics
        printPerformanceMetrics();
        
        cout << "Total clock cycles: " << clock << endl;
    }
    
    void printPerformanceMetrics() {
        cout << "\n====== Performance Metrics ======\n";
        
        // Calculate IPC
        double ipc = static_cast<double>(completedInstructions) / clock;
        cout << "Instructions Per Cycle (IPC): " << ipc << endl;
        
        // Calculate average instruction latency
        int totalLatency = 0;
        for (int latency : instructionLatencies) {
            totalLatency += latency;
        }
        double avgLatency = static_cast<double>(totalLatency) / instructionLatencies.size();
        cout << "Average Instruction Latency: " << avgLatency << " cycles" << endl;
        
        // Print stall statistics
        cout << "Structural Hazard Stalls: " << structuralStalls << endl;
        cout << "Data Hazard Stalls: " << dataStalls << endl;
    }
    
    void printStatus() {
        cout << "Instruction Status:" << endl;
        for (int i = 0; i < instructions.size(); i++) {
            cout << "Instruction " << i << ": ";
            if (instructions[i].isIssued) cout << "Issued at " << instructions[i].issueTime;
            if (instructions[i].isExecStarted) cout << ", Exec started at " << instructions[i].execStartTime;
            if (instructions[i].isExecCompleted) cout << ", Exec completed at " << instructions[i].execCompTime;
            if (instructions[i].isCommitted) cout << ", Committed at " << instructions[i].commitTime;
            cout << endl;
        }
        
        cout << "Reservation Stations:" << endl;
        printReservationStations(iadd_rs, "IADD");
        printReservationStations(imul_rs, "IMUL");
        printReservationStations(fadd_rs, "FADD");
        printReservationStations(fmul_rs, "FMUL");
        printReservationStations(mem_rs, "MEM");
        printReservationStations(logic_rs, "LOGIC");
        
        cout << "Reorder Buffer:" << endl;
        for (int i = 0; i < robSize; i++) {
            cout << "ROB[" << i << "]: ";
            if (i == robHead) cout << "(Head) ";
            if (i == robTail) cout << "(Tail) ";
            if (rob[i].instrIndex != -1) {
                cout << "Instr " << rob[i].instrIndex << ", Dest: R" << rob[i].dest;
                cout << ", Ready: " << (rob[i].ready ? "Yes" : "No");
            } else {
                cout << "Empty";
            }
            cout << endl;
        }
        
        cout << "Register Status:" << endl;
        for (int i = 0; i < regStatus.size(); i++) {
            if (regStatus[i].robIndex != -1) {
                cout << "R" << i << " -> ROB[" << regStatus[i].robIndex << "]" << endl;
            }
        }
        cout << endl;
    }
    
    void printReservationStations(vector<ReservationStation>& rs, string name) {
        for (int i = 0; i < rs.size(); i++) {
            cout << name << "[" << i << "]: ";
            if (rs[i].busy) {
                cout << "Busy, Instr: " << rs[i].instrIndex;
                cout << ", Qj: " << rs[i].Qj << ", Qk: " << rs[i].Qk;
                cout << ", Vj: " << rs[i].Vj << ", Vk: " << rs[i].Vk;
                cout << ", Dest: " << rs[i].dest;
                cout << ", Cycles left: " << rs[i].cycles;
            } else {
                cout << "Free";
            }
            cout << endl;
        }
    }
};

// Test program
int main() {
    TomasuloSimulator simulator;
    
    // Sample assembly program
    vector<string> program = {
        "LD R1, M10",
        "LD R2, M20",
        "IMUL R3, R1, R2",
        "IADD R4, R1, R3",
        "FADD R5, R2, R4",
        "ST M30, R5"
    };
    
    // Parse assembly program
    for (const auto& instruction : program) {
        simulator.parseAssembly(instruction);
    }
    
    // Run simulation
    simulator.runSimulation();
    
    return 0;
}