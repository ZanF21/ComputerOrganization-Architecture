#include <bits/stdc++.h>
using namespace std;

#define IF 0
#define ID 1
#define EX 2
#define MM 3
#define WB 4
#define ADD 0
#define SUB 1
#define MUL 2
#define INC 3
#define AND 4
#define OR 5
#define NOT 6
#define XOR 7
#define LD 8
#define ST 9
#define JMP 10
#define BEQZ 11
#define HALT 15

class Cache
{
	private:
		int cacheSize = 256;
		int blockSize = 4;
		ifstream * srcFile;
		vector<int> sets;
	public:
		Cache(ifstream * fp);
		int readBlock(unsigned char address);
		unsigned char readByte(unsigned char address);
		void writeBlock(unsigned char address, int data);
		void writeByte(unsigned char address, unsigned char data);
		void dumpCache(string filename);
};
Cache::Cache(ifstream * fp) 
{
	this->srcFile = fp;
	sets = vector<int> (cacheSize, 0);
	string hexCode;
	int value;
	int blockNum = 0;
	int offset = 0;
	while(*fp >> hexCode)
	{
		if(offset == 4)
		{
			offset = 0;
			blockNum++;
		}
		value = stoi(hexCode,0,16);
		for(int i = 0; i < offset; i++)
			value = value << 8;
		sets[blockNum] += value;
		offset++;
	}
    fp->close();
}
int Cache::readBlock(unsigned char address)
{
	return sets[address >> 2];
}
unsigned char Cache::readByte(unsigned char address)
{
	int data = readBlock(address);
	int offset = address & (blockSize-1);
	for(int i = offset; i < 3; i++)
	{
		data = data << 8;
	}
	for(int i = 0; i < 3; i++)
	{
		data = data >> 8;
	}
	return (unsigned char)data;
}
void Cache::writeBlock(unsigned char address, int data)
{
	sets[address >> 2] = data;
}
void Cache::writeByte(unsigned char address, unsigned char data)
{
	int temp  = (int)data;
	unsigned char blockNum = address >> 2;
	unsigned char offset = address & (3);
	int mask = cacheSize-1;
	for(int i = 0; i < (int)offset; i++)
	{
		mask = mask << 8;
		temp = temp << 8;
	}
	mask = UINT_MAX - mask;
	sets[blockNum] = (sets[blockNum] & mask) + temp;
}
void Cache::dumpCache(string filename)  
{
	ofstream outfile;
	outfile.open(filename, ofstream::trunc);
	outfile << setfill('0') << setw(2) << hex;
	for(int i = 0; i < cacheSize; i++)  
	{
		outfile << setw(2) << ((int) readByte(i)) << "\n";
	}
	outfile.close();
}

class RegFile
{
	int regSize = 16;
	ifstream * srcFile;
	vector<unsigned char> RF;
	vector<bool> status;
public:
	RegFile(ifstream * fp);
	unsigned char read(unsigned char index);
	void write(unsigned char index, unsigned char data);
	void setStatus(unsigned char index, bool currStatus);
	bool isOpen(unsigned char index);
};
RegFile::RegFile(ifstream * fp)
{
	this->srcFile = fp;
	RF = vector<unsigned char> (regSize);
	status = vector<bool> (regSize,false);
	string hexCode;
	unsigned char value;
	int regNum = 0;
	while(*fp >> hexCode)
	{
		value = stoi(hexCode,0,16);
		RF[regNum] = value;
		regNum++;
	}
    fp->close();
};
unsigned char RegFile::read(unsigned char index)
{
	return RF[index];
}
void RegFile::write(unsigned char index,unsigned char data)
{
    if(index == 0)  {return;}
	RF[index] = data;
}
bool RegFile::isOpen(unsigned char index)
{
	return status[index];
}
void RegFile::setStatus(unsigned char index, bool currStatus)
{
	status[index] = currStatus;
}

class Processor  
{
	private:
		Cache* iCache;	//cache object pointers to store instruction and data caches
		Cache* dCache;
		RegFile* regFile;	//register file object pointer to allot the object later
		bool haltScheduled = false;
		bool halted = false;
		bool stallIF = false;
		bool stallID = false;
		bool stallEX = false;
		bool stallMM = false;
		bool stallWB = false;
		unsigned char REG_IF_PC = 0u; // first instruction
		bool IF_run = true;
		void IFstage();
		int REG_ID_IR = 0u; // add R0 to R0 and store in R0
		unsigned char  REG_ID_PC = 0u;
		bool  ID_run = false;
		void IDstage();
		int REG_EX_IR = 0u;
		unsigned char  REG_EX_PC = 0u;
		unsigned char  REG_EX_A  = 0u;
		unsigned char  REG_EX_B  = 0u;
		bool  EX_run = false;
		void EXstage();
		int REG_MM_IR = 0u;
		unsigned char  REG_MM_AO = 0u;
		bool  REG_MM_COND = false;
		bool  MM_run = false;
		void MEMstage();
		int REG_WB_IR   = 0u;
		unsigned char  REG_WB_AO   = 0u;
		unsigned char  REG_WB_LMD  = 0u;
		bool  REG_WB_COND = false;
		bool  WB_run = false;
		void WBstage();
		void flushPipeline();
	public:
		Processor(ifstream * Icache, ifstream * Dcache, ifstream * RegFile);
		~Processor();
		void run();
		void cycle();
		bool isHalted();
		void dumpdata(string fnameCache, string fnameOut);
		int stat_instruction_count = 0;
		int stat_instruction_count_arith = 0;
		int stat_instruction_count_logic = 0;
		int stat_instruction_count_data = 0;
		int stat_instruction_count_control = 0;
		int stat_instruction_count_halt = 0;
		int stat_cycles = 0;
		int stat_stalls = 0;
		int stat_stalls_data = 0;
		int stat_stalls_control = 0;
};
Processor::Processor(ifstream * Icache, ifstream * Dcache, ifstream * regFile)  
{
	iCache = new Cache(Icache);
	dCache = new Cache(Dcache);
	this->regFile = new RegFile(regFile);
}
Processor::~Processor()  
{
	delete iCache;
	delete dCache;
	delete regFile;
}
void Processor::run()  
{
	while(!isHalted())
		cycle();	//call the cycle function, the equivalent of one cycle of the processor

	stat_instruction_count = stat_instruction_count_arith 
							+ stat_instruction_count_control 
							+ stat_instruction_count_data 
							+ stat_instruction_count_logic 
							+ stat_instruction_count_halt;
}
void Processor::cycle()  
{
	stat_cycles++;
	WBstage();
	MEMstage();
	EXstage();
	IDstage();
	IFstage();
}
bool Processor::isHalted()  
{
	return halted;
}
void Processor::EXstage()
{
	if(!EX_run || stallEX)  
		return;

	int opCode = REG_EX_IR >> 12;
	switch(opCode)
	{
		case ADD :	REG_MM_AO = REG_EX_A + REG_EX_B; 
					stat_instruction_count_arith++;	
					break;

		case SUB :	REG_MM_AO = REG_EX_A - REG_EX_B;
					stat_instruction_count_arith++; 
					break;

		case MUL :	REG_MM_AO = REG_EX_A * REG_EX_B; 
					stat_instruction_count_arith++; 
					break;

		case INC :	REG_MM_AO = REG_EX_A + 1; 		 
					stat_instruction_count_arith++; 
					break;

		case AND :	REG_MM_AO = REG_EX_A & REG_EX_B; 
					stat_instruction_count_logic++; 	
					break;

		case OR :	REG_MM_AO = REG_EX_A | REG_EX_B; 
					stat_instruction_count_logic++; 	
					break;

		case NOT :	REG_MM_AO = ~REG_EX_A;
					stat_instruction_count_logic++; 	
					break;

		case XOR :	REG_MM_AO = REG_EX_A ^ REG_EX_B;
					stat_instruction_count_logic++; 	
					break;

		case LD :	REG_MM_AO = REG_EX_A + REG_EX_B;
					stat_instruction_count_data++;	
					break;
						
		case ST :	REG_MM_AO = REG_EX_A + REG_EX_B;
					stat_instruction_count_data++;	
					break;

		case JMP:	stallEX = true;
					stat_instruction_count_control++;	//counting the control instruction
					REG_MM_AO = REG_EX_PC + (unsigned char) ((int) REG_EX_A << 1);
					break;

		case BEQZ:	stallEX = true;
					stat_instruction_count_control++;	//counting the control instruction
					if((int)REG_EX_A == 0)
						REG_MM_AO =  REG_EX_PC +((int) ( REG_EX_B << 1));
					else
						REG_MM_AO =  REG_EX_PC;
					break;

		default:	break;
	}
	REG_MM_IR = REG_EX_IR;
	MM_run = true;	//marking that next instruction to be implemented is memmory 
	EX_run = false;	//setting that the stage is finished
}
void Processor::IFstage()  
{
	if(!IF_run) // due to backward nature, if ID_run is true, it has been stalling
		return;
	if(ID_run || stallID)  
	{ // due to backward nature, if ID_run is true, it has been stalling
		IF_run = true;
		return;
	}
	REG_ID_IR = (((int) iCache->readByte(REG_IF_PC)) << 8) + ((int) iCache->readByte(REG_IF_PC+1));
	ID_run = true;
	REG_IF_PC += 2;
}
void Processor::IDstage()  
{
	if((!ID_run) || stallID)  
		return;
	if(EX_run || stallEX)  
	{
		ID_run = true;
		return;
	}

	REG_EX_IR = REG_ID_IR;
	unsigned char opcode = (REG_ID_IR & 0xf000) >> 12;
	if(opcode == HALT)  
	{
		IF_run = false;
		ID_run = false;
		EX_run = true;
		stat_instruction_count_halt++;	//counting number of halt instructions
		// let the other pipeline stages (previous instructions) complete
		return;
	}
	ID_run = false;
	EX_run = true;
	if(opcode == JMP)  
	{
		stallID = true;
		REG_EX_A = (REG_ID_IR & 0x0ff0) >> 4;
		REG_EX_PC = REG_IF_PC;
		stat_stalls_control += 2;
		return;
	}
	if(opcode == BEQZ)  
	{
		stallID = true;
		if(regFile->isOpen((REG_ID_IR & 0x0f00) >> 8))
		{
			stat_stalls_data++;
			EX_run = false;
			ID_run = true;
            stallID = false; // RAW stalling is different
			return;
		}
		stat_stalls_control += 2;
		REG_EX_PC = REG_IF_PC;
		REG_EX_A = regFile->read((REG_ID_IR & 0x0f00) >> 8);
		REG_EX_B = REG_ID_IR & 0x00ff;
		return;
	}
	int addr1;
	int addr2;
	if(opcode == ST)  
	{
		addr1 = (REG_ID_IR & 0x00f0) >> 4;
		if(regFile->isOpen(addr1))
		{
			stat_stalls_data++;
			EX_run = false;
			ID_run = true;
			return;
		}

		REG_EX_A = regFile->read((REG_ID_IR & 0x00f0) >> 4);
		REG_EX_B = REG_ID_IR & 0x000f;
		return;
	}

	if((opcode == LD))  
	{
		addr1 = (REG_ID_IR & 0x00f0) >> 4;
		if(regFile->isOpen(addr1))
		{
			stat_stalls_data++;
			EX_run = false;
			ID_run = true;
			return;
		}
		REG_EX_A = regFile->read((REG_ID_IR & 0x00f0) >> 4);
		REG_EX_B = REG_ID_IR & 0x000f;

        regFile->setStatus((unsigned char) ((REG_ID_IR & 0x0f00) >> 8), true);
		return;
	}
	if((opcode >= ADD) && (opcode <= MUL))  
	{
		addr1 = (REG_ID_IR & 0x00f0) >> 4;
		addr2 = REG_ID_IR & 0x000f;
		if(regFile->isOpen(addr1) || regFile->isOpen(addr2))
		{
			stat_stalls_data++;
			EX_run = false;
			ID_run = true;
			return;
		}

		REG_EX_A = regFile->read((REG_ID_IR & 0x00f0) >> 4);
		REG_EX_B = regFile->read(REG_ID_IR & 0x000f);
        regFile->setStatus((unsigned char) ((REG_ID_IR & 0x0f00) >> 8), true);
		return;
	}
	if(opcode == INC)  
	{
		addr1 = (REG_ID_IR & 0x0f00) >> 8;
		if(regFile->isOpen(addr1))
		{
			stat_stalls_data++;
			EX_run = false;
			ID_run = true;
			return;
		}

		REG_EX_A = regFile->read((REG_ID_IR & 0x0f00) >> 8);
        regFile->setStatus((unsigned char) ((REG_ID_IR & 0x0f00) >> 8), true);
		return;
	}
	if((opcode != NOT))  
	{
		addr1 = (REG_ID_IR & 0x00f0) >> 4;
		addr2 = REG_ID_IR & 0x000f;
		if(regFile->isOpen(addr1) || regFile->isOpen(addr2))
		{
			stat_stalls_data++;
			EX_run = false;
			ID_run = true;
			return;
		}

		REG_EX_A = regFile->read((REG_ID_IR & 0x00f0) >> 4);
		REG_EX_B = regFile->read(REG_ID_IR & 0x000f);

        regFile->setStatus((unsigned char) ((REG_ID_IR & 0x0f00) >> 8), true);
		return;
	}

	// here, opcode == NOT
	if(opcode == NOT)	
	{
		addr1 = (REG_ID_IR & 0x00f0) >> 4;
        if(regFile->isOpen(addr1))
        {
        	stat_stalls_data++;
            EX_run = false;
            ID_run = true;
            return;
        }

		REG_EX_A = regFile->read((REG_ID_IR & 0x00f0) >> 4);

        regFile->setStatus((unsigned char) ((REG_ID_IR & 0x0f00) >> 8), true);
	}
}
void Processor::MEMstage(){
	if(!MM_run || stallMM)
		return;
	MM_run = false;
	int opCode = REG_MM_IR >> 12;
	unsigned char offset = (unsigned char) ((REG_MM_IR & 0x0f00) >> 8);
	if(opCode == HALT)
	{
		WB_run = true;
		REG_WB_IR = REG_MM_IR;
		return;
	}
	else if((opCode == JMP) || (opCode == BEQZ))  
	{
		// if PC didn't change, carry on with decoding
		if(REG_IF_PC == REG_MM_AO)  
		{
			ID_run = false;
			stallID = false;
			stallEX = false;
		}  
		else  
		{
			REG_IF_PC = REG_MM_AO;
			flushPipeline();
		}
		WB_run = false;
		return;
	}
	
	else if(opCode == ST)
	{
		dCache->writeByte(REG_MM_AO,regFile->read((REG_MM_IR & 0x0f00) >> 8));
		MM_run = false;
        REG_WB_AO = 0u;
        REG_WB_IR = 0u;
        REG_WB_LMD = 0u;
        REG_WB_COND = false;
        WB_run = true;
		return;
	}
	else if(opCode == LD)
	{
		REG_WB_LMD = dCache->readByte(REG_MM_AO);
		REG_WB_IR = REG_MM_IR;
		REG_WB_AO = REG_MM_AO;
		WB_run = true;
	}  
	else  
	{
		MM_run = false;
        REG_WB_AO = REG_MM_AO;
        REG_WB_IR = REG_MM_IR;
        REG_WB_LMD = 0u;
        REG_WB_COND = REG_MM_COND;
        WB_run = true;
		return;
	}

	MM_run = false;
	return;
}
void Processor::WBstage()
{
	if(!WB_run || stallWB)
		return;
	int opCode = REG_WB_IR >> 12;
	unsigned char offset = (unsigned char) ((REG_WB_IR & 0x0f00) >> 8);
	if(opCode == HALT)
	{
		WB_run = false;
		halted = true;
		return;
	}
	if(opCode == LD) 
		regFile->write(offset, REG_WB_LMD);
	else 
		regFile->write(offset, REG_WB_AO);
	regFile->setStatus(offset, false);
	WB_run = false;
}
void Processor::flushPipeline()  
{
	IF_run = true;
	ID_run = false;
	EX_run = false;
	MM_run = false;
	WB_run = false;
	REG_ID_IR = 0u;
	REG_ID_PC = 0u;
	REG_EX_A = 0u;
	REG_EX_B = 0u;
	REG_EX_IR = 0u;
	REG_EX_PC = 0u;
	REG_MM_AO = 0u;
	REG_MM_IR = 0u;
	REG_WB_AO = 0u;
	REG_WB_IR = 0u;
	REG_WB_COND = 0;
	REG_WB_LMD = 0u;
	stallIF = false;
	stallID = false;
	stallEX = false;
	stallMM = false;
	stallWB = false;
}
void Processor::dumpdata(string fnameCache, string fnameOut)  
{
	dCache->dumpCache(fnameCache);
	ofstream outFile;
	outFile.open(fnameOut);
	outFile << "Total number of instructions executed: " << stat_instruction_count << "\n";
	outFile << "Number of instructions in each class" << "\n";
	outFile << "Arithmetic instructions              : " << stat_instruction_count_arith << "\n";
	outFile << "Logical instructions                 : " << stat_instruction_count_logic << "\n";
	outFile << "Data instructions                    : " << stat_instruction_count_data << "\n";
	outFile << "Control instructions                 : " << stat_instruction_count_control << "\n";
	outFile << "Halt instructions                    : " << stat_instruction_count_halt << "\n";
	outFile << "Cycles Per Instruction               : " << ((double) stat_cycles / stat_instruction_count) << "\n";
	outFile << "Total number of stalls               : " << stat_stalls_data+stat_stalls_control << "\n";
	outFile << "Data stalls (RAW)                    : " << stat_stalls_data << "\n";
	outFile << "Control stalls                       : " << stat_stalls_control << "\n";
}

int main()
{
	ifstream Icache, Dcache, RegFile;	//creating objects for file handling
	Icache.open("ICache.txt");	//getting the filepointer of the Instruction cache file
	Dcache.open("DCache.txt");	//getting the filepointer of the data cache file
	RegFile.open("RF.txt");	//the input for the register file
	Processor processor(&Icache, &Dcache, &RegFile);	//sending the adress class as pointers/*change class name*/
	processor.run();	//running the processor
	processor.dumpdata("DCache.out.txt", "Output.txt");
	return 0;
}