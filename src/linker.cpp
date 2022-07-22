#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <stdio.h>
#include <string.h>
#include <sstream>
#include <vector>
#include <map>
#include <unordered_map>
#include <iomanip>
#include <queue>

using namespace std;

struct Token {
    string token;
    int offset;
    int line;
};

vector <Token> _token;

#pragma region Globla Variable
unordered_map <string, int> _symbolVal; // (key, val) = (the def list's symbol, def list's vak) ; pass1
map <string, bool> _symbolUseBefore; // (key, val) = (used(/ not) befor, T/F) ; pass1 
map <string, bool> _duplicateUse; // (symbol, use before or not)
map <string, int> _symbolAddr; // (symbol, module)
map <int, int> _searchAddress; // (key, val) = (# of module ,# of instruction) ; pass1
map <int, string> _userListVal; // (key, val) = (No. of uselist, userlist's symbol) ; pass2
map <int, int> _memoryMap; // (key, val) = (the order from program text, the val of program text) ; pass2
map <int, string> _errorMessage;
map <string, bool> _checkUseList;
map <string, bool> _checkDefList;
map <int, string> _warningMessage; // (modulenumber, uselist)

#pragma endregion Globla Variable

#pragma region ____parseerror
void __parseerror(Token token,int errcode) {
    static char* errstr[] = {
        "NUM_EXPECTED", "SYM_EXPECTED", "ADDR_EXPECTED", "SYM_TOO_LONG",
        "TOO_MANY_DEF_IN_MODULE","TOO_MANY_USE_IN_MODULE", "TOO_MANY_INSTR",
    };
    printf ("Parse Error line %d offset %d: %s\n", token.line, token.offset, errstr[errcode]);
    exit(0);
}
#pragma endregion ____parseerror

#pragma region readInt
int readInt(Token x) {
    int ans = 0;

    try {
        ans = stoi(x.token);
    }
    catch(exception ex) {
        __parseerror(x,0);
    }
    return ans;
}
#pragma endregion readInt

#pragma region readSymbol 
string readSymbol(Token x) {
    for (int i = 0; i < x.token.size(); i++) {
        if (i==0 && !isalpha(x.token[i])) {
            __parseerror(x,1);
        }

        if(!isalnum(x.token[i]) && !isalpha(x.token[i])) {
            __parseerror(x,1);
        }
    }
    if (x.token.size() > 16) {
        __parseerror(x,3);
    }
    return x.token;
}
#pragma endregion readSymbol

#pragma region readIEAR
string readIEAR(Token x) {
    if (x.token != "I" && x.token != "E" && x.token !=  "A" && x.token !=  "R") {
        __parseerror(x,2);
    }
    return x.token;
}
#pragma endregion readIEAR

#pragma region getToken
void getToken(string file_name) {
    string line;
    int currline = 1;
    int linesize = 0;
    fstream myfile (file_name);
    if (myfile.is_open()) {   
        while ( getline (myfile,line)) {   
            linesize = line.size();
            // for the empty line.
            if (line.empty()) {
                currline++;
                continue;
            }
            stringstream ss (line);
            string tempLine;
            int length;
            
            while (ss >> tempLine) {
                Token tempToken;
                length = line.find(tempLine);
                tempToken.token = tempLine;
                tempToken.offset = length + 1;
                tempToken.line = currline;
                _token.push_back (tempToken);
            }
            currline++;
        }
        Token last;
        last.line = currline - 1;
        last.offset = ++linesize;
        last.token = " ";
        _token.push_back(last); 
    }
    else cout << "Unable to open file" << endl;
    myfile.close();
}
#pragma endregion getToken

#pragma region pass1Algorithm
void pass1Algorithm() {
    int curr = 0;
    int module_num = 0;
    int instr = 0;
    vector <string> symbolKey;
    map <string, bool> tooBigWarning; // too big
    map <string, bool> WarnBeofre;


    while(curr < _token.size() - 1) {   
        Token curToken;
        module_num++;
        unordered_map <string, int> _currAllToken;
        vector<pair<string, int>> save;// symbol, val
        vector<pair<string, int>> localsymtable; // for the too big warning.
        map <string, int> WhichMoudle; // (sym, current_instcount)
        map <int, int> MoudleInstruction; // (moudule.)
        
        // Definition List
        int defCount = readInt(_token[curr]);
        if (defCount > 16) {
            __parseerror(_token[curr],4);
        }
        curr++;
        for (int i = 0; i<defCount; i++) {
            string sym = readSymbol(_token[curr]);
            curr++;
            if ( _symbolUseBefore[sym]) {
                _duplicateUse[sym] = true;
                int val = readInt(_token[curr]);
                localsymtable.push_back({sym, val}); // too big 
                curr++;
                save.push_back({sym, val});
                continue; // rerun, do not add instr again!
            }
            if(_duplicateUse[sym] == false) {   
                _checkDefList[sym] = false; //test
                _symbolAddr[sym] = module_num; // record the symbol in which module
            }
            int val = readInt(_token[curr]);
            save.push_back({sym, val});
            symbolKey.push_back(sym);
            localsymtable.push_back({sym, val});
            curr++;
            _symbolUseBefore[sym] = true;
            _currAllToken[sym] = val;
            if (val > 512) {
            cout << "Warning: Module " << module_num << ":" << sym << "too big (max = 512)" << "assume zero relative"<<endl;
            } 
        }
        

        // User List
        int useCount = readInt(_token[curr]);

        if (useCount > 16) {
            __parseerror(_token[curr],5);
        }
        curr++;
        for (int i = 0; i<useCount; i++) {
            string sym = readSymbol(_token[curr]);
            curr++;
        }
        
        // Program Text
        int instCount = readInt(_token[curr]);
        

        //cout << "size:" << _symbolVal.size() << endl;
        for (int i = 0; i < save.size(); i++) {
            auto temp = save[i];
            // if (tooBigWarning[temp.first]== true && _duplicateUse[temp.first] == true) {
            //     temp.second = 0;
            //     if (temp.second > instr + instCount - 1) {
            //         cout << "Warning: Module " << module_num << ": " << temp.first << " too big " << temp.second << " (max=" << instCount-1 << ") assume zero relative\n";
            //     }
            // }
            
            if (_symbolVal.find(temp.first) != _symbolVal.end()){
                temp.second = _symbolVal[temp.first];
                if (temp.second > instr + instCount - 1) {
                    cout << "Warning: Module " << module_num << ": " << temp.first << " too big " << 0 << " (max=" << instCount-1 << ") assume zero relative\n";
                }
                continue;
            }
            
            if (temp.second > instCount - 1) {
                cout << "Warning: Module " << module_num << ": " << temp.first << " too big " << temp.second << " (max=" << instCount-1 << ") assume zero relative\n";
                temp.second = 0;
                // tooBigWarning[temp.first] = true;// too big 
            }
            temp.second += instr;
            _symbolVal[temp.first] = temp.second;
        }
        //!!!!!!!!!
        // for (int i = 0; i < save.size(); i++) {
        //     auto temp = save[i];
        //     // if (tooBigWarning[temp.first]== true && _duplicateUse[temp.first] == true) {
        //     //     temp.second = 0;
        //     //     if (temp.second > instr + instCount - 1) {
        //     //         cout << "Warning: Module " << module_num << ": " << temp.first << " too big " << temp.second << " (max=" << instCount-1 << ") assume zero relative\n";
        //     //     }
        //     // }
        //     if (temp.second > instCount - 1) {
        //         cout << "Warning: Module " << module_num << ": " << temp.first << " too big " << temp.second << " (max=" << instCount-1 << ") assume zero relative\n";
        //         temp.second = 0;
        //         // tooBigWarning[temp.first] = true;// too big 
        //     }
        //     temp.second += instr;
        //     _symbolVal[temp.first] = temp.second;
        // }        


        // for (int i = 0; i < save.size(); i++) {
        //     auto temp = save[i];
        //     if (tooBigWarning[temp.first] == true) {
        //         temp.second = 0;
        //     }

        //     if (temp.second > instCount - 1) {
        //         // tooBigWarning[temp.first]= 0;
        //         cout << "Warning: Module " << module_num << ": " << temp.first << " too big " << 0 << " (max=" << instCount-1 << ") assume zero relative\n";
        //         // too big
        //         cout << "Warning: Module " << module_num << ": " << temp.first << " too big " << temp.second << " (max=" << instCount-1 << ") assume zero relative\n";
        //         temp.second = 0;
        //         tooBigWarning[temp.first] = true;// too big 
        //     }
        //     temp.second += instr;
        //     _symbolVal[temp.first] = temp.second;
        // }
        
        instr += instCount;
        if (instr > 512) {
            __parseerror(_token[curr],6);
        }
        curr++;
        for (int i = 0; i < instCount; i++) {
            string addressmode = readIEAR(_token[curr]);
            curr++;
            int operand = readInt(_token[curr]);
            curr++;
            _searchAddress[module_num] = instr;
        }
    }

    cout << "Symbol Table" << endl;

    for (int i = 0; i < symbolKey.size(); i ++) {
        cout << symbolKey[i]<< "=" << _symbolVal[symbolKey[i]];
        if (_duplicateUse[symbolKey[i]]) {
            cout << " Error: This variable is multiple times defined; first value used";
        }
        cout<<endl;
    }
    cout << endl;
}

#pragma endregion pass1Algorithm

#pragma region pass2Algorithm
void pass2Algorithm(string file_name) {
    // Maintaining the Memory Map's index.
    int curr = 0;
    // Maintaining the Module's numbering. 
    int module_num = 0;
    // Maintaining the Module's order.
    int MemoryTableIndex = 0;
    int opcode;
    int operand;
    int instr = 0;
    string temp;
    ifstream myfile(file_name.c_str(), ios::in);
    map <string, int> judgeModule;
    vector <string> symbolKeyTwo; //test

    cout << "Memory Map" << endl;

    while(myfile >> temp) {   
        module_num++;

        // Definition List
        int defCount = stoi(temp);
        for (int i = 0; i < defCount; i++) {
            myfile >> temp;
            if(judgeModule.count(temp)){
                myfile >> temp;
                continue;
            }
            symbolKeyTwo.push_back(temp);
            judgeModule[temp] = module_num;
            myfile >> temp;
        }

        // User List
        myfile >> temp;
        int useCount = stoi(temp);
        for (int i = 0; i<useCount; i++) {
            myfile >> temp;
            string sym = temp;
            _userListVal[i] = sym; // tag userlist's symbol.
            _checkUseList[sym] = false; // default all sym as false.
        }

        // Programe Text
        myfile >> temp;
        int instCount = stoi(temp);
        instr += instCount;
        
        for (int i = 0; i < instCount; i++) {   
            int finalVal = 0;
            myfile >> temp;
            string addressmode = temp;
            myfile >> temp;
            int instructionVal = stoi(temp);
            opcode = instructionVal / 1000;
            operand = instructionVal % 1000;

            if (addressmode == "I") {
                if (instructionVal>=10000) {
                    _memoryMap[MemoryTableIndex] = 9999;
                    _errorMessage[MemoryTableIndex] = " Error: Illegal immediate value; treated as 9999";
                }
                else {
                    _memoryMap[MemoryTableIndex] = instructionVal;
                }
                
            }
            else if (addressmode == "A") {
                if(opcode >= 10) {
                    finalVal = 9999;
                    _errorMessage[MemoryTableIndex] = " Error: Illegal opcode; treated as 9999";
                    _memoryMap[MemoryTableIndex] = finalVal;
                }else if (operand >= 512) {
                    _memoryMap[MemoryTableIndex] = opcode*1000;
                    _errorMessage[MemoryTableIndex] = " Error: Absolute address exceeds machine size; zero used";
                }
                else {
                    _memoryMap[MemoryTableIndex] = instructionVal;
                }
            }
            else if (addressmode == "E") {
                //rule 6:If an external address is too large to reference an entry in the use list, 
                //print an error message and treat the address as immediate.
                if (operand >= useCount) {
                    finalVal = instructionVal;
                    _errorMessage[MemoryTableIndex] = " Error: External address exceeds length of uselist; treated as immediate";
                    _memoryMap[MemoryTableIndex]=finalVal;
                }
                // rule3: If a symbol is used in an E-instruction but not defined anywhere, 
                // print an error message and use the value absolute zero.
                else if(!_symbolVal.count(_userListVal[instructionVal%1000])) {
                    finalVal = opcode *1000;
                    _errorMessage[MemoryTableIndex] = " Error: " + _userListVal[instructionVal%1000] + " is not defined; zero used";
                    _checkUseList[_userListVal[instructionVal%1000]] = true;
                    _memoryMap[MemoryTableIndex]=finalVal;
                } 
                else {
                    _checkUseList[_userListVal[instructionVal%1000]] = true;
                    finalVal = _symbolVal[_userListVal[instructionVal%1000]] + opcode*1000;
                    _memoryMap[MemoryTableIndex]=finalVal;
                }
                symbolKeyTwo.push_back(_userListVal[operand]);//test
                _checkDefList[_userListVal[instructionVal%1000]] = true;
                // _memoryMap[MemoryTableIndex]=finalVal;
            }
            else if(addressmode == "R") {
                if( instCount < operand) {
                    finalVal = opcode*1000 + instr - instCount;
                    _errorMessage[MemoryTableIndex] = " Error: Relative address exceeds module size; zero used";

                }
                else if(opcode >= 10) {
                    finalVal = 9999;
                    _errorMessage[MemoryTableIndex] = " Error: Illegal opcode; treated as 9999";
                }
                else {
                    finalVal = instr - instCount + instructionVal;
                }
                _memoryMap[MemoryTableIndex]=finalVal;
            }
            cout << setw(3) << setfill('0') << MemoryTableIndex << ": " << setw(4) << setfill('0') << _memoryMap[MemoryTableIndex];
            cout << _errorMessage[MemoryTableIndex] << endl;
            MemoryTableIndex++;
        }
        for (int i = 0; i < useCount; i++ ) {
            if(_checkUseList[_userListVal[i]] == false) {
                _warningMessage[module_num] = _userListVal[i];
                cout <<"Warning: Module " << module_num << ": " << _userListVal[i] << " appeared in the uselist but was not actually used" << endl;
            }
        }
    }
    cout << endl;
    
    for (int i = 0; i < symbolKeyTwo.size(); i++) {
        if(!_checkDefList[symbolKeyTwo[i]]) {
            cout << "Warning: Module "<< judgeModule[symbolKeyTwo[i]] << ": " << symbolKeyTwo[i] << " was defined but never used";
            if(i != symbolKeyTwo.size()-1) {
                cout << endl;
            }
        }
    }
}
#pragma endregion pass2Algorithm

int main(int argc, char** argv) {
    if(argc < 2) {
        cout << "wrong input" << endl;
        return 0;
    }
    getToken(argv[1]);
    pass1Algorithm();
    pass2Algorithm(argv[1]);
    return 0;
}